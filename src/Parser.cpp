#include "Parser.hpp"
#include "AST.hpp"
#include "Logger.hpp"

Parser::Parser(std::vector<Token> tokens_list, std::string_view file_path,
               std::vector<std::string> lines)
    : tokens(std::move(tokens_list)), filename(file_path),
      source_lines(std::move(lines)) {}

const Token& Parser::peek() const {
  if (is_at_end()) {
    static const Token eof_token{TokenType::END_OF_FILE, "EOF", 0, 0};
    return eof_token;
  }
  return tokens[current];
}

std::optional<Token> Parser::consume_token(TokenType type,
                                           std::string_view msg) {
  if (check(type))
    return advance();

  const std::string_view token_val = is_at_end() ? "EOF" : peek().value;
  report_error(peek(), std::format("{} but got '{}' ", msg, token_val));
  return std::nullopt;
}

// For when I need to verify a symbol ( e.g.. '(', '->') Yeah, no. I can't type.
bool Parser::consume(TokenType type, std::string_view msg) {
  if (check(type)) {
    advance();
    return true;
  }

  const std::string_view token_val = is_at_end() ? "EOF" : peek().value;
  report_error(peek(), std::format("{} but got '{}'", msg, token_val));
  return false;
}

bool Parser::is_statement_start(TokenType type) {
  switch (type) {
  case TokenType::IMPORT:
  case TokenType::FUNCTION:
  case TokenType::IF:
  case TokenType::ELSEIF:
  case TokenType::ELSE:
  case TokenType::FOR:
  case TokenType::WHILE:
  case TokenType::LET:
  case TokenType::RETURN:
  case TokenType::BREAK:
  case TokenType::CONTINUE:
  case TokenType::MAIN:
  case TokenType::SWITCH:
  case TokenType::UNWRAP:
  case TokenType::EXIT:
  case TokenType::LENGTH:
  case TokenType::JOIN:
  case TokenType::EVAL:
  case TokenType::NAMESPACE:
  case TokenType::RBRACE:
    return true;

  case TokenType::OR:
  case TokenType::AND:
  case TokenType::TRUE:
  case TokenType::FALSE:
  case TokenType::IN:
  case TokenType::HASH:
  case TokenType::AMPERSAND:
  case TokenType::ARROW:
  case TokenType::RANGE:
  case TokenType::PLUS_PLUS:
  case TokenType::MINUS_MINUS:
  case TokenType::PLUS_EQUAL:
  case TokenType::MINUS_EQUAL:
  case TokenType::MUL_EQUAL:
  case TokenType::DIV_EQUAL:
  case TokenType::L_S_O:
  case TokenType::R_S_O:
  case TokenType::LTE:
  case TokenType::GTE:
  case TokenType::EQ:
  case TokenType::NEQ:
  case TokenType::MAP_ARROW:
  case TokenType::DOUBLE_COLON:
  case TokenType::ASSIGN:
  case TokenType::PLUS:
  case TokenType::MINUS:
  case TokenType::MUL:
  case TokenType::DIV:
  case TokenType::MOD:
  case TokenType::LT:
  case TokenType::GT:
  case TokenType::DOT:
  case TokenType::BANG:
  case TokenType::QUESTION:
  case TokenType::COLON:
  case TokenType::SEMI_COLON:
  case TokenType::COMMA:
  case TokenType::L_PAREN:
  case TokenType::R_PAREN:
  case TokenType::LBRACE:
  case TokenType::LBRACKET:
  case TokenType::RBRACKET:
  case TokenType::FLOAT_LITERAL:
  case TokenType::INT_LITERAL:
  case TokenType::STRING_LITERAL:
  case TokenType::RAW_STRING_LITERAL:
  case TokenType::BACK_TICK_LITERAL:
  case TokenType::AUTO:
  case TokenType::CONST:
  case TokenType::VOID:
  case TokenType::IDENTIFIER:
  case TokenType::END_OF_FILE:
    return false;
  }
  return false;
}

bool Parser::parse_parameter_list(std::vector<ParameterASTNode> &params,
                                  const ParameterOptsASTNode &opts,
                                  TokenType term_type) {
  // OUTER LOOP: Iterates through each type-group (e.g., "int: ..." or "str:
  // ...")
  while (!is_at_end() && !check(term_type)) {
    TypeSpecifier group_type = parse_type();

    // If a colon is missing, it would call synchronize() globally and discard
    // the block.
    if (!check(TokenType::COLON)) {
      report_error(
          peek(),
          std::format("Parameter declaration syntax error: Expected a colon "
                      "':' directly after the type specifier '{}' "
                      "to map it to variable names. Encountered '{}' instead.",
                      group_type.base_types, peek().value));
      return false;
    }
    advance(); // Safely consume the verified ':'

    // INNER LOOP: Iterates through the comma-separated variables mapped to the
    // current group_type
    while (!is_at_end() && !check(term_type)) {

      // Manually check the identifier to bypass global synchronization traps.
      if (!check(TokenType::IDENTIFIER)) {
        report_error(
            peek(),
            std::format("Invalid parameter target: Expected a valid identifier "
                        "name for the parameter of type '{}'. "
                        "Found '{}' instead. Please ensure you are providing a "
                        "valid variable name.",
                        group_type.base_types, peek().value));

        // Localized Recovery: Clear out junk tokens up to the next parameter
        // mapping delimiter.
        while (!is_at_end() && !check(TokenType::COMMA) && !check(term_type))
          advance();

        if (check(TokenType::COMMA)) {
          advance();
          continue; // Local boundary found! Skip the broken token and try
                    // parsing the next element.
        }
        break; // Hit terminal symbol or EOF safely
      }

      std::optional<Token> param_name = advance();
      std::unique_ptr<ASTNode> default_value = nullptr;

      std::unique_ptr<ASTNode> param_target =
          std::make_unique<VariableExprASTNode>(param_name->value);

      std::unique_ptr<RangeLiteralASTNode> param_range = nullptr;

      if (check(TokenType::ASSIGN)) {
        if (opts.fatal_on_assign) {
          report_error(
              peek(),
              std::format("Forbidden assignment: Default values via (':=') are "
                          "strictly prohibited for the parameter '{}'. "
                          "Function signatures and similar contexts do not "
                          "allow inline defaults here.",
                          param_name->value));
          // Minor local step-over to prevent cascading errors across parameters
          advance();
          parse_expression(Precedence::NONE);
        } else if (opts.allow_assignment) {
          advance();
          default_value = parse_expression(Precedence::NONE);
        }
      }

      if (opts.allow_ranges && check(TokenType::RANGE)) {
        Token op_tok = advance();
        std::unique_ptr<ASTNode> end_node = nullptr;

        if (!check(TokenType::COMMA) && !check(term_type)) {
          end_node = parse_expression(Precedence::NONE);

          if (!end_node || dynamic_cast<StubASTNode *>(end_node.get())) {
            report_error(
                peek(),
                std::format("Range expression error: Expected a valid bounding "
                            "expression after the '...' operator "
                            "for parameter '{}'. Found '{}' which is not a "
                            "valid expression.",
                            param_name->value, peek().value));
            return false;
          }
        }

        std::unique_ptr<ASTNode> range_start =
            std::make_unique<VariableExprASTNode>(param_name->value);

        param_range = std::make_unique<RangeLiteralASTNode>(
            std::move(range_start), op_tok, std::move(end_node));
      }

      params.emplace_back(std::move(param_target), group_type,
                          std::move(param_range), std::move(default_value));

      // Passive exit, if assignments are not parsed inline here. Stop
      // collecting names immediately so VarDecl can handle the token
      // downstream.
      if (!opts.allow_assignment && check(TokenType::ASSIGN))
        return true;

      if (!check(TokenType::COMMA))
        break; // No comma means we are done with this parameter group entirely.

      // --- DYNAMIC LOOKAHEAD ---
      // We are at a COMMA. We need to know if the next items belong to THIS
      // type group (e.g. `, y`), or if a NEW type group is starting (e.g. `,
      // str: y`). Your old `current + 2` logic breaks if a type has multiple
      // tokens (like `const int:`). This scan safely looks ahead for a colon
      // without consuming tokens.
      bool next_is_new_type_group = false;
      size_t scan_idx = current; // 'current' is looking right at the COMMA
      scan_idx++;                // Skip the comma itself

      while (scan_idx < tokens.size()) {
        if (tokens[scan_idx].type == TokenType::COLON) {
          next_is_new_type_group = true;
          break; // Found a colon before the next comma, meaning a new type is
                 // being declared!
        }
        if (tokens[scan_idx].type == TokenType::COMMA ||
            tokens[scan_idx].type == term_type) {
          break; // Hit another comma or end symbol first, so it's just another
                 // variable of the SAME type.
        }
        scan_idx++;
      }

      advance(); // Safely consume the COMMA

      if (next_is_new_type_group) {
        break; // Break the INNER loop so the OUTER loop can parse the new type
               // specifier
      }
      // Otherwise, loop continues to grab the next variable for the CURRENT
      // type
    }
  }
  return true;
}

bool Parser::parser_function_parameter_group(
    std::vector<ParameterASTNode> &params) {
  ParameterOptsASTNode opts;
  opts.allow_assignment = false;
  opts.allow_ranges = true;
  opts.fatal_on_assign = true;
  return parse_parameter_list(params, opts, TokenType::R_PAREN);
}

bool Parser::parser_variable_parameter_group(
    std::vector<ParameterASTNode> &params) {
  ParameterOptsASTNode opts;
  opts.allow_assignment = false;
  opts.allow_ranges = false;
  opts.fatal_on_assign = false;
  return parse_parameter_list(params, opts, TokenType::SEMI_COLON);
}

void Parser::synchronize() {
  while (!is_at_end()) {
    if (current > 0 && tokens[current - 1].type == TokenType::SEMI_COLON)
      return;
    if (is_statement_start(peek().type))
      return;
    advance();
  }
}

void Parser::report_error(const Token &tok, std::string_view msg) {
  std::println("\033[1m{}:{}:{}: \033[31merror:\033[0m \033[1m{}\033[0m",
               filename, tok.line, tok.column, msg);
  if (tok.line == 0 || tok.line > source_lines.size())
    return;

  const std::string &sl = source_lines[tok.line - 1];
  std::println("{:>5} | {}", tok.line, sl);

  std::string padding(tok.column > 0 ? tok.column - 1 : 0, ' ');
  std::string underline = "^";
  if (tok.value.length() > 1)
    underline += std::string(tok.value.length() - 1, '~');

  std::println("      | {}{}", padding, underline); // 6 spaces + "| " = 8 chars
}

ParserRule Parser::get_rule(TokenType type) {
  switch (type) {
  case TokenType::INT_LITERAL:
  case TokenType::FLOAT_LITERAL:
  case TokenType::STRING_LITERAL:
  case TokenType::RAW_STRING_LITERAL:
  case TokenType::BACK_TICK_LITERAL:
  case TokenType::TRUE:
  case TokenType::FALSE:
    return {Precedence::NONE, &Parser::parse_literal, nullptr};

  case TokenType::IDENTIFIER:
    return {Precedence::NONE, &Parser::parse_identifier, nullptr};

  case TokenType::BANG:
    return {Precedence::NONE, &Parser::parse_prefix, nullptr};

  case TokenType::PLUS_PLUS:
  case TokenType::MINUS_MINUS:
    return {Precedence::UNARY, &Parser::parse_prefix, &Parser::parse_postfix};

  case TokenType::ASSIGN:
  case TokenType::PLUS_EQUAL:
  case TokenType::MINUS_EQUAL:
  case TokenType::MUL_EQUAL:
  case TokenType::DIV_EQUAL:
    return {Precedence::ASSIGNMENT, nullptr, &Parser::parse_assignment};

  case TokenType::OR:
    return {Precedence::LOGICAL_OR, nullptr, &Parser::parse_binary};
  case TokenType::AND:
    return {Precedence::LOGICAL_AND, nullptr, &Parser::parse_binary};

  case TokenType::EQ:
  case TokenType::NEQ:
    return {Precedence::EQUALITY, nullptr, &Parser::parse_binary};

  case TokenType::LT:
  case TokenType::GT:
  case TokenType::LTE:
  case TokenType::GTE:
    return {Precedence::COMPARISON, nullptr, &Parser::parse_binary};

  case TokenType::L_S_O:
  case TokenType::R_S_O:
    return {Precedence::BITSHIFT, nullptr, &Parser::parse_binary};

  case TokenType::PLUS:
  case TokenType::MINUS:
    return {Precedence::TERM, &Parser::parse_prefix, &Parser::parse_binary};

  case TokenType::MUL:
  case TokenType::DIV:
  case TokenType::MOD:
    return {Precedence::FACTOR, &Parser::parse_prefix, &Parser::parse_binary};

  case TokenType::DOT:
  case TokenType::DOUBLE_COLON:
    return {Precedence::MEMBER_ACCESS, nullptr, &Parser::parse_member_access};

  case TokenType::L_PAREN:
    return {Precedence::CALL_INDEX, &Parser::parse_paren_expression,
            &Parser::parse_call};
  case TokenType::LBRACKET:
    return {Precedence::CALL_INDEX, &Parser::parse_array_literal,
            &Parser::parse_index};

  case TokenType::IMPORT:
  case TokenType::FUNCTION:
  case TokenType::MAIN:
  case TokenType::IF:
  case TokenType::ELSEIF:
  case TokenType::ELSE:
  case TokenType::FOR:
  case TokenType::WHILE:
  case TokenType::LET:
  case TokenType::RETURN:
  case TokenType::BREAK:
  case TokenType::CONTINUE:
  case TokenType::IN:
  case TokenType::SWITCH:
  case TokenType::UNWRAP:
  case TokenType::EXIT:
  case TokenType::LENGTH:
  case TokenType::JOIN:
  case TokenType::EVAL:
  case TokenType::NAMESPACE:
  case TokenType::HASH:
  case TokenType::AMPERSAND:
  case TokenType::ARROW:
  case TokenType::RANGE:
  case TokenType::MAP_ARROW:
  case TokenType::QUESTION:
  case TokenType::COLON:
  case TokenType::SEMI_COLON:
  case TokenType::COMMA:
  case TokenType::R_PAREN:
  case TokenType::RBRACE:
  case TokenType::LBRACE:
  case TokenType::RBRACKET:
  case TokenType::AUTO:
  case TokenType::CONST:
  case TokenType::VOID:
  case TokenType::END_OF_FILE:
    return {Precedence::NONE, nullptr, nullptr};
  }
  return {Precedence::NONE, nullptr, nullptr};
}

TypeSpecifier Parser::parse_type() {
  TypeSpecifier type_info;
  std::string full_type_name;

  if (check(TokenType::CONST)) {
    const Token t = advance();
    type_info.is_const = true;
    log_mov("Tracking 'const'", t);
    full_type_name = "const ";
  }

  if (check(TokenType::IDENTIFIER))
    full_type_name += advance().value;

  // Track < > Generics
  // Track () (Tuple / Function Types)
  // Track [] (Fixed or dynamic arrays)
  int angle_depth = 0;
  int paren_depth = 0;
  int square_depth = 0;

  while (!is_at_end()) {
    TokenType current_type = peek().type;

    if (current_type == TokenType::SEMI_COLON)
      break;

    if (angle_depth == 0 && paren_depth == 0 && square_depth == 0)
      if (current_type == TokenType::COLON ||
          current_type == TokenType::LBRACE ||
          current_type == TokenType::R_PAREN ||
          current_type == TokenType::COMMA ||
          current_type == TokenType::ARROW ||
          current_type == TokenType::SEMI_COLON)
        break;

    Token t = advance();

    if (t.type == TokenType::LT)
      angle_depth++;
    else if (t.type == TokenType::GT)
      angle_depth--;
    else if (t.type == TokenType::L_PAREN)
      paren_depth++;
    else if (t.type == TokenType::R_PAREN)
      paren_depth--;
    else if (t.type == TokenType::LBRACKET)
      square_depth++;
    else if (t.type == TokenType::RBRACKET)
      square_depth--;

    // Sanity validation: Reject blatant operaors out-of-hand if they land
    // outside template params.
    if (angle_depth == 0 && paren_depth == 0 && square_depth == 0)
      if (t.type == TokenType::PLUS || t.type == TokenType::MINUS ||
          t.type == TokenType::MUL || t.type == TokenType::DIV ||
          t.type == TokenType::ASSIGN) {
        report_error(
            t, std::format("Unexpected operator '{}' inside type specifier",
                           t.value));
        continue; // Skip the junk token.
      }

    // unsigned int instead of unsignedint
    if (!full_type_name.empty() &&
        std::isalnum(static_cast<u_char>(full_type_name.back())) &&
        std::isalnum(static_cast<u_char>(t.value[0])))
      full_type_name += " ";

    full_type_name += t.value;
  }

  if (angle_depth != 0 || paren_depth != 0 || square_depth != 0)
    report_error(
        peek(), "Malformed type signature: Unbalanced brackets or parantheses");

  if (full_type_name.empty() || full_type_name == "const ") {
    report_error(
        peek(),
        std::format("Expected a valid type specifier, got '{}'", peek().value));
    type_info.base_types = "unknown";
  } else {
    type_info.base_types = full_type_name;
    log_mov(std::format("Tracking type '{}'", full_type_name),
            tokens[current - 1]);
  }

  return type_info;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parse_program() {
  std::vector<std::unique_ptr<ASTNode>> program_nodes;

  while (!is_at_end()) {
     std::unique_ptr<ASTNode> node = parse_statement();
    if (node) {
      program_nodes.emplace_back(std::move(node));
    } else {
      synchronize();
    }
  }

  return program_nodes;
}

std::unique_ptr<ASTNode>
Parser::parse_assignment(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];

  // Ensure the target is a variable, an array index or an object property
  // access. For example, separated with 'is.array'?

  if (!dynamic_cast<VariableExprASTNode *>(left.get()) &&
      !dynamic_cast<IndexASTNode *>(left.get()) &&
      !dynamic_cast<MemberAccessASTNode *>(left.get())) {
    report_error(op, "Invalid left-hand side target in assignment expression");
    return nullptr;
  }

  Precedence right_precedence = static_cast<Precedence>(
      static_cast<std::int64_t>(Precedence::ASSIGNMENT) - 1);

  std::unique_ptr<ASTNode> right = parse_expression(right_precedence);
  if (!right)
    return nullptr;

  return std::make_unique<AssignmentASTNode>(std::move(left), op,
                                             std::move(right));
}

std::unique_ptr<ASTNode> Parser::parse_variable_declaration() {
  const Token let_token = advance();

  std::vector<ParameterASTNode> all_vars;
  if (!parser_variable_parameter_group(all_vars)) {
    synchronize();
    return std::make_unique<StubASTNode>(
        "Malformed variable declaration targets");
  }

  std::vector<std::unique_ptr<ASTNode>> initializers;

  if (check(TokenType::ASSIGN)) {
    advance(); // Consume ':='

    while (!check(TokenType::SEMI_COLON) && !is_at_end()) {
      std::unique_ptr<ASTNode> expr = parse_expression(Precedence::NONE);

      if (!expr || dynamic_cast<StubASTNode *>(expr.get())) {
        report_error(peek(), "Expected valid expression in initializers list");

        while (!is_at_end() && !check(TokenType::COMMA) &&
               !check(TokenType::SEMI_COLON))
          advance();

        if (check(TokenType::COMMA)) {
          advance();
          continue;
        }
        break;
      }

      initializers.emplace_back(std::move(expr));

      if (check(TokenType::COMMA)) {
        advance();

        // Guard against trailing commas: let x := 1, ;
        if (check(TokenType::SEMI_COLON)) {
          report_error(peek(), "Trailing comma detected in initializers list");
          break;
        }
      } else
        break;
    }

    // Assignment cardinality rules
    // Leave this alone.
    // The backend will see multiple targets but only one
    // source expression, signaling that it needs to perform a tuple unpack.
    if (all_vars.size() > 1 && initializers.size() == 1) {
      // Left alone: Backend signals tuple unpack
    }

    // x, y := a, b, c (Truncation)
    // Slice off any excess expression that don't have matching variables.
    else if (initializers.size() > all_vars.size()) {
      initializers.resize(all_vars.size());
    }

    // x, y, z := a, b (Strict Mismatch Error)
    // Provides multiple expression, but not enough to fill the variables.
    // throw a compile-time failure.
    else if (initializers.size() < all_vars.size()) {
      size_t missing_count = all_vars.size() - initializers.size();

      report_error(
          peek(),
          std::format("Assignment count mismatch: Expected {} expressions to "
                      "fully map to the declared variables, but only found {}. "
                      "You are missing {} initialization value(s).",
                      all_vars.size(), initializers.size(), missing_count));

      report_error(let_token, "In the variable declaration starting here");
      synchronize();

      if (check(TokenType::SEMI_COLON))
        advance();

      return std::make_unique<StubASTNode>(
          "Malformed Variable Declaration (Count Mismatch)");
    }
  }

  if (!check(TokenType::SEMI_COLON)) {
    report_error(peek(), "Expected ';' at the end of variable declaration");
    report_error(let_token, "To complete this variable declaration");

    synchronize();
    return std::make_unique<StubASTNode>(
        "Missing semicolon in variable declaration");
  }
  advance(); // Safely consume ';'

  return std::make_unique<VarDeclASTNode>(std::move(all_vars),
                                          std::move(initializers));
}

std::unique_ptr<ASTNode> Parser::parse_array_literal() {
  const Token open_bracket = tokens[current - 1]; // Points to '['
  std::vector<std::unique_ptr<ASTNode>> elements;

  while (!is_at_end() && !check(TokenType::RBRACKET)) {
    std::unique_ptr<ASTNode> elem = parse_expression(Precedence::NONE);

    if (!elem || dynamic_cast<StubASTNode *>(elem.get())) {
      report_error(peek(),
                   "Array element parse failed; syncing out of bracket group");

      while (!check(TokenType::COMMA) && !check(TokenType::RBRACKET) &&
             !is_at_end())
        advance();

      if (check(TokenType::COMMA)) {
        advance();
        continue;
      }
      break;
    }

    elements.emplace_back(std::move(elem));
    if (!check(TokenType::COMMA))
      break;
    advance(); // Safely consume the comma
  }

  if (!check(TokenType::RBRACKET)) {
    report_error(peek(), "Expected ']' to close this array literal");
    report_error(open_bracket, "To match this opening square bracket");
    synchronize();
    return std::make_unique<StubASTNode>("Malformed array literal");
  }
  advance(); // Safely consume the validated ']'
  return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
}

std::unique_ptr<ASTNode> Parser::parse_range_literal() {
  std::unique_ptr<ASTNode> left = parse_primary();
  if (!left)
    return nullptr;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  if (!check(TokenType::RANGE))
    return left;

  const Token op_tok = advance();

  std::unique_ptr<ASTNode> right = nullptr;

  if (check(TokenType::R_PAREN) || check(TokenType::COMMA)) {
  }

  else if (check(TokenType::INT_LITERAL))
    right = parse_primary();

  else {
    report_error(peek(),
                 std::format("Syntax Error: Range operator '...' followed by "
                             "invalid token '{}' ."
                             "Expected an integer literal for a bounded range, "
                             "or nothing for an infinite range.",
                             peek().value));
    return nullptr;
  }

  return std::make_unique<RangeLiteralASTNode>(std::move(left), op_tok,
                                               std::move(right));
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_expression(Precedence min_prec) {
  const Token token = advance();
  const NudFunc nud = get_rule(token.type).nud;

  if (!nud) {
    report_error(token,
                 std::format("Expected start of an expression, but got '{}'",
                             token.value));
    return std::make_unique<StubASTNode>("Malformed Expression Prefix");
  }

  std::unique_ptr<ASTNode> left = (this->*nud)();

  if (!left || dynamic_cast<StubASTNode *>(left.get()))
    return left;

  while (!is_at_end() && min_prec < get_rule(peek().type).precedence) {
    const Token next = advance();
    const LedFunc led = get_rule(next.type).led;
    if (led) {
      left = (this->*led)(std::move(left));

      if (!left || dynamic_cast<StubASTNode *>(left.get()))
        return left;
    }
  }
  return left;
}

std::unique_ptr<ASTNode> Parser::parse_literal() {
  return std::make_unique<LiteralASTNode>(tokens[current - 1].value);
}

std::unique_ptr<ASTNode> Parser::parse_identifier() {
  return std::make_unique<VariableExprASTNode>(tokens[current - 1].value);
}

std::unique_ptr<ASTNode> Parser::parse_prefix() {
  Token op = tokens[current - 1];
  std::unique_ptr<ASTNode> operand = parse_expression(Precedence::UNARY);

  if (!operand || dynamic_cast<StubASTNode *>(operand.get()))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
    return operand;
#pragma GCC diagnostic pop
  return std::make_unique<UnaryExprASTNode>(std::move(operand), op, false);
}

std::unique_ptr<ASTNode> Parser::parse_postfix(std::unique_ptr<ASTNode> left) {
  return std::make_unique<UnaryExprASTNode>(std::move(left),
                                            tokens[current - 1], true);
}

std::unique_ptr<ASTNode> Parser::parse_binary(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];
  const Precedence next_prec = static_cast<Precedence>(
      static_cast<std::int64_t>(get_rule(op.type).precedence) + 1);
  std::unique_ptr<ASTNode> right = parse_expression(next_prec);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  if (!right || dynamic_cast<StubASTNode *>(right.get()))
    return right;
#pragma GCC diagnostic pop
  return std::make_unique<BinaryExprASTNode>(std::move(left), op,
                                             std::move(right));
}

std::unique_ptr<ASTNode>
Parser::parse_member_access([[maybe_unused]] std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];

  if (!check(TokenType::IDENTIFIER)) {
    report_error(
        peek(),
        std::format("Invalid member access: After operator '{}', expected an "
                    "identifier (field or method name) but got '{}'.",
                    op.value, peek().value));

    return std::make_unique<StubASTNode>("Malformed member access");
  }

  const Token member = advance();

  if (!left || dynamic_cast<StubASTNode *>(left.get())) {
    report_error(
        op, "Member access operator applied to an invalid or null expression.");
    return left;
  }
  return std::make_unique<MemberAccessASTNode>(std::move(left), op, member);
}

std::unique_ptr<ASTNode> Parser::parse_grouping() {
  const Token open_paren = tokens[current - 1];
  std::unique_ptr<ASTNode> expr = parse_expression(Precedence::NONE);

  if (!check(TokenType::R_PAREN)) {
    report_error(peek(), "Expected closing ')' after grouping expression");
    report_error(open_paren, "To match this opening paranthesis");
    synchronize();

    return std::make_unique<StubASTNode>("Malformed grouping expression");
  }
  advance();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return expr;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_call(std::unique_ptr<ASTNode> left) {
  if (!left || dynamic_cast<StubASTNode *>(left.get()))
    return left;

  const Token open_paren = tokens[current - 1]; // Points to the '('
  std::vector<std::unique_ptr<ASTNode>> args;

  while (!is_at_end() && !check(TokenType::R_PAREN)) {
    std::unique_ptr<ASTNode> arg = parse_expression(Precedence::NONE);

    if (!arg || dynamic_cast<StubASTNode *>(arg.get())) {
      report_error(peek(),
                   "Call argument parse failed; syncing out of call frame");

      while (!check(TokenType::COMMA) && !check(TokenType::R_PAREN) &&
             !is_at_end())
        advance();

      if (check(TokenType::COMMA)) {
        advance();
        continue;
      }
      break; // Hit ')' or EOF
    }

    args.emplace_back(std::move(arg));

    if (!check(TokenType::COMMA))
      break;
    advance();
  }

  if (!check(TokenType::R_PAREN)) {
    report_error(peek(), "Expected closing ')' after function arguments");
    report_error(open_paren, "To match this opening after paranthesis");
    synchronize();
    return std::make_unique<StubASTNode>("Malformed function call");
  }
  advance();

  std::string fn = "ambiguous";
  if (const VariableExprASTNode *v =
          dynamic_cast<VariableExprASTNode *>(left.get()))
    fn = v->name;
  else if (const MemberAccessASTNode *m = dynamic_cast<MemberAccessASTNode *>(left.get()))
    fn = m->member.value;

  return std::make_unique<CallASTNode>(fn, std::move(args));
}

std::unique_ptr<ASTNode> Parser::parse_index(std::unique_ptr<ASTNode> left) {
  const Token open_bracket = tokens[current - 1];
  std::unique_ptr<ASTNode> index_expr = parse_expression(Precedence::NONE);
  if (!index_expr) {
    report_error(peek(), "Invalid expression inside array subscript.");
    return nullptr;
  }

  if (!consume(TokenType::RBRACKET,
               "Syntax Error: Expected ']' after array subscript,"))
    return nullptr;

  log_mov("Array subscript closed ']'", tokens[current - 1]);

  return std::make_unique<IndexASTNode>(std::move(left), open_bracket,
                                        std::move(index_expr));
}

std::unique_ptr<ASTNode> Parser::parse_block() {
  if (!check(TokenType::LBRACE)) {
    report_error(peek(),
                 std::format("Block initialization failed: Expected '{{' to "
                             "open a block, but encountered '{}'",
                             peek().value));
    return nullptr;
  }

  const Token ob = advance();
  log_mov("Block opened '{'", ob);

  std::vector<std::unique_ptr<ASTNode>> stmts;

  while (!is_at_end() && !check(TokenType::RBRACE)) {
    std::unique_ptr<ASTNode> stmt = parse_statement();
    if (stmt)
      stmts.emplace_back(std::move(stmt));
    else
      synchronize();
  }

  if (!consume(TokenType::RBRACE,
               std::format("Unterminated block: Started at line {}, but "
                           "missing '}}' to close the scope",
                           ob.line)))
    return nullptr;

  log_mov("Block closed '}'", tokens[current - 1]);

  return std::make_unique<BlockASTNode>(std::move(stmts));
}

std::unique_ptr<ASTNode> Parser::parse_paren() {
  const Token op = peek();
  if (!consume(TokenType::L_PAREN, "Syntax Error: Expected '('"))
    return nullptr;

  log_mov("Opening '('", op);

  std::unique_ptr<ASTNode> node = parse_expression(Precedence::NONE);

  if (!node)
    return nullptr;

  if (!consume(TokenType::R_PAREN,
               std::format("Syntax Error: Parenthesized expression opened at "
                           "Line {}, Column {} is missing closing ')'",
                           op.line, op.column)))
    return nullptr;

  log_mov("Closing ')'", tokens[current - 1]);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return node;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_import_statement() {
  advance();
  advance();

  const Token lt_tok = peek();

  if (!consume(TokenType::LT, "Syntax Error: Import path must start with '<'"))
    return nullptr;

  log_mov("Import '<'", lt_tok.line, lt_tok.column);

  std::vector<std::string> path_parts;
  while (!is_at_end()) {
    auto id_tok = consume_token(
        TokenType::IDENTIFIER,
        "Syntax Error: Expected module identifier in import path");
    if (!id_tok)
      return nullptr;

    path_parts.emplace_back(id_tok->value);

    if (check(TokenType::DOUBLE_COLON))
      advance();
    else
      break;
  }

  if (!consume(TokenType::GT, "Syntax Error: Import path must end with '>'"))
    return nullptr;

  return std::make_unique<ImportASTNode>(std::move(path_parts));
}

std::unique_ptr<ASTNode> Parser::parse_paren_expression() {
  std::unique_ptr<ASTNode> result = parse_expression(Precedence::NONE);
  if (!result)
    return nullptr;

  if (!consume(TokenType::R_PAREN, "Syntax Error: Unclosed parenthesize "
                                   "expression. Expected closing ')'"))
    return nullptr;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return result;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_if_body() {
  // Brace block — always takes precedence.
  if (check(TokenType::LBRACE))
    return parse_block();

  // Optional `:` separator — consume it and fall through to a single statement
  // or brace block on the next line.
  if (check(TokenType::COLON)) {
    advance();

    if (check(TokenType::LBRACE))
      return parse_block();

    std::unique_ptr<ASTNode> stmt = parse_statement();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
    if (!stmt) {
      report_error(peek(),
                   "Syntax Error: Expected a statement or block after ':'");
      return nullptr;
    }
    return stmt;
  }

  // Inline single statement with no colon.
  auto stmt = parse_statement();
  if (!stmt) {
    report_error(
        peek(), "Syntax Error: Expected a valid statement as the body of 'if'");
    return nullptr;
  }

  return stmt;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_if_statement() {
  const Token if_tok = advance(); // consume 'if'

  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;
  std::unique_ptr<ASTNode> else_branch = nullptr;

  // if-branch
  std::unique_ptr<ASTNode> if_cond = parse_paren();

  if (!if_cond) {
    report_error(if_tok, "Syntax Error: 'if' keyword must be followed by a "
                         "condition in parantheses '()'.");
    return nullptr;
  }

  std::unique_ptr<ASTNode> if_body = parse_if_body();
  if (!if_body)
    return nullptr; // Error already handled by body parser

  branches.emplace_back(std::move(if_cond), std::move(if_body));

  while (check(TokenType::ELSEIF)) {
    advance(); // Consume 'else if'

    std::unique_ptr<ASTNode> ei_cond = parse_paren();
    if (!ei_cond) {
      report_error(
          peek(),
          "Syntax Error: 'else if' requires a condition in parantheses '()'.");
      return nullptr;
    }
    std::unique_ptr<ASTNode> ei_body = parse_if_body();

    if (!ei_body)
      return nullptr;

    branches.emplace_back(std::move(ei_cond), std::move(ei_body));
  }

  if (check(TokenType::ELSE)) {
    advance(); // consume 'else'
    else_branch = parse_if_body();

    if (!else_branch) {
      report_error(peek(), "Syntax Error: 'else' keyword must be followed by a "
                           "statement or block.");
      return nullptr;
    }
  }

  return std::make_unique<IfASTNode>(std::move(branches),
                                     std::move(else_branch));
}

std::unique_ptr<ASTNode> Parser::parse_function_statement() {
  advance(); // Consume function

  Token name_tok = peek();
  if (check(TokenType::IDENTIFIER) || check(TokenType::MAIN)) {
    name_tok = advance();
  } else {
    report_error(peek(),
                 std::format("Syntax Error: Expected function name (identifier "
                             "or 'main'), but found '{}' instead.",
                             peek().value));
    return nullptr;
  }

  if (!consume(TokenType::L_PAREN,
               "Syntax Error: Expected '(' after function name"))
    return nullptr;

  std::vector<ParameterASTNode> params;

  if (check(TokenType::VOID))
    advance();

  else if (!check(TokenType::R_PAREN)) {
    if (!parser_function_parameter_group(params))
      return nullptr; // Erro reported inside parameter group parser
  }

  if (!consume(TokenType::R_PAREN,
               "Syntax Error: Expected ')' after parameter list"))
    return nullptr;

  if (!consume(TokenType::ARROW,
               "Syntax Error: Expected '->' return type indicator"))
    return nullptr;

  TypeSpecifier ret = parse_type();

  if (ret.is_unknown()) {
    report_error(peek(), "Syntax Error: Invalid or missing return type.");
    return nullptr;
  }

  if (!consume(TokenType::COLON,
               "Syntax Error: Expected ':' before function body"))
    return nullptr;

  std::unique_ptr<ASTNode> body = parse_block();
  if (!body)
    return nullptr;

  std::unique_ptr<BlockASTNode> block_body = std::unique_ptr<BlockASTNode>(
      static_cast<BlockASTNode *>(body.release()));
  return std::make_unique<FunctionASTNode>(std::string(name_tok.value),
                                           std::move(params), ret,
                                           std::move(block_body));
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
  // #import <...>
  if (check(TokenType::HASH) && peek_next().type == TokenType::IMPORT) {
    return parse_import_statement();
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  switch (peek().type) {
  case TokenType::LET: {
    std::unique_ptr<ASTNode> decl = parse_variable_declaration();
    if (!decl)
      return nullptr;
    return decl;
  }
  case TokenType::IF:
    return parse_if_statement();

  case TokenType::FUNCTION:
    return parse_function_statement();

  case TokenType::RETURN: {
    advance();
    std::unique_ptr<ASTNode> value = nullptr;
    if (!check(TokenType::SEMI_COLON))
      value = parse_primary();
    consume(TokenType::SEMI_COLON, "Expected ';'");
    return std::make_unique<StubASTNode>("Return ASTNode");
  }

  }
#pragma GCC diagnostic pop

  // expression statement
  std::unique_ptr<ASTNode> expr = parse_primary();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return expr;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_primary() {
  return parse_expression(Precedence::NONE);
}
