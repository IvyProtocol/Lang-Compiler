#include "Parser.hpp"
#include "AST.hpp"
#include "Logger.hpp"
#include <tuple>

Parser::Parser(std::vector<Token> tokens_list, std::string_view file_path,
               std::vector<std::string> lines)
    : tokens(std::move(tokens_list)), filename(file_path),
      source_lines(std::move(lines)) {}

const Token &Parser::peek() const {
  if (is_at_end()) {
    static const Token eof_token{TokenType::END_OF_FILE, "EOF", 0, 0};
    return eof_token;
  }
  return tokens[current];
}

std::expected<Token, ParseError> Parser::consume_token(TokenType type,
                                                       std::string_view msg) {
  if (check(type))
    return advance();

  const std::string_view token_val = is_at_end() ? "EOF" : peek().value;
  report_error(peek(), std::format("{} but got '{}' ", msg, token_val));
  return std::unexpected(ParseError::UnexpectedToken);
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

          std::ignore = parse_expression(Precedence::NONE);

        } else if (opts.allow_assignment) {
          advance();

          auto default_value_res = parse_expression(Precedence::NONE);
          if (!default_value_res.has_value())
            return false;

          default_value = std::move(default_value_res.value());
        }
      }

      if (opts.allow_ranges && check(TokenType::RANGE)) {
        Token op_tok = advance();
        std::unique_ptr<ASTNode> end_node = nullptr;

        if (!check(TokenType::COMMA) && !check(term_type)) {
          auto end_node_res = parse_expression(Precedence::NONE);

          if (!end_node_res.has_value()) {
            report_error(
                peek(),
                std::format("Range expression error: Expected a valid bounding "
                            "expression after the '...' operator "
                            "for parameter '{}'. Found '{}' which is not a "
                            "valid expression.",
                            param_name->value, peek().value));
            return false;
          }

          end_node = std::move(end_node_res.value());
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
  // Reset sequence at the end of the first println
  std::println("\033[1m{}:{}:{}:\033[0m {}{}: {}{}\033[0m", filename, tok.line,
               tok.column, ConsoleColor::BOLD_RED, "error", ConsoleColor::RESET,
               msg);

  if (tok.line == 0 || tok.line > source_lines.size())
    return;

  const std::string &sl = source_lines[tok.line - 1];
  std::println("{:>5} | {}", tok.line, sl);

  std::string padding(tok.column > 0 ? tok.column - 1 : 0, ' ');
  std::string underline = "^";
  if (tok.value.length() > 1)
    underline += std::string(tok.value.length() - 1, '~');

  // Explicitly reset after the underline
  std::println("      | {}{}{}{}", padding, ConsoleColor::BOLD_RED, underline,
               ConsoleColor::RESET);
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
    auto node = parse_statement();
    if (node) {
      program_nodes.emplace_back(std::move(node.value()));
    } else {
      synchronize();
    }
  }

  return program_nodes;
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_assignment(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];

  // Ensure the target is a variable, an array index or an object property
  // access. For example, separated with 'is.array'?

  if (!dynamic_cast<VariableExprASTNode *>(left.get()) &&
      !dynamic_cast<IndexASTNode *>(left.get()) &&
      !dynamic_cast<MemberAccessASTNode *>(left.get())) {
    report_error(op, "Invalid left-hand side target in assignment expression");
    return std::unexpected(ParseError::InvalidAssignmentTarget);
  }

  Precedence right_precedence = static_cast<Precedence>(
      static_cast<std::int64_t>(Precedence::ASSIGNMENT) - 1);

  auto right_res = parse_expression(right_precedence);
  if (!right_res)
    return std::unexpected(right_res.error());

  return std::make_unique<AssignmentASTNode>(std::move(left), op,
                                             std::move(*right_res));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_variable_declaration() {
  const Token let_token = advance();

  std::vector<ParameterASTNode> all_vars;
  if (!parser_variable_parameter_group(all_vars)) {
    synchronize();
    return std::unexpected(ParseError::InvalidDeclarationTarget);
  }

  std::vector<std::unique_ptr<ASTNode>> initializers;

  if (check(TokenType::ASSIGN)) {
    advance(); // Consume ':='

    while (!check(TokenType::SEMI_COLON) && !is_at_end()) {
      auto expr_res = parse_expression(Precedence::NONE);

      if (!expr_res) {
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

      initializers.emplace_back(std::move(expr_res.value()));

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
      size_t excess_count = initializers.size() - all_vars.size();

      report_error(
          peek(),
          std::format("Assignment count mismatch: Expected {} expressions to "
                      "fully map to the declared variables, but found {} "
                      "excess expression(s). "
                      "Remove the extra {} intiialization value(s)",
                      all_vars.size(), excess_count, excess_count));

      report_error(let_token, "In the variable declaration starting here");
      synchronize();

      if (check(TokenType::SEMI_COLON))
        advance();

      return std::unexpected(ParseError::AssignmentCountMismatch);
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

      return std::unexpected(ParseError::AssignmentCountMismatch);
    }
  }

  if (!check(TokenType::SEMI_COLON)) {
    report_error(peek(), "Expected ';' at the end of variable declaration");
    report_error(let_token, "To complete this variable declaration");

    synchronize();
    return std::unexpected(ParseError::MissingSemiColon);
  }
  advance(); // Safely consume ';'

  return std::make_unique<VarDeclASTNode>(std::move(all_vars),
                                          std::move(initializers));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_array_literal() {
  const Token open_bracket = tokens[current - 1]; // Points to '['
  std::vector<std::unique_ptr<ASTNode>> elements;

  while (!is_at_end() && !check(TokenType::RBRACKET)) {
    auto elem_res = parse_expression(Precedence::NONE);

    if (!elem_res) {
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

    elements.emplace_back(std::move(elem_res.value()));
    if (!check(TokenType::COMMA))
      break;
    advance(); // Safely consume the comma
  }

  if (!check(TokenType::RBRACKET)) {
    report_error(peek(), "Expected ']' to close this array literal");
    report_error(open_bracket, "To match this opening square bracket");
    synchronize();
    return std::unexpected(ParseError::InvalidArrayMalformed);
  }
  advance(); // Safely consume the validated ']'
  return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_range_literal() {
  auto left_res = parse_primary();

  if (!left_res.has_value())
    return std::unexpected(left_res.error());

  std::unique_ptr<ASTNode> left_node = std::move(left_res.value());
  if (!check(TokenType::RANGE))
    return left_node;

  const Token op_tok = advance();

  std::unique_ptr<ASTNode> right_node = nullptr;

  if (check(TokenType::R_PAREN) || check(TokenType::COMMA)) {
  }

  else if (check(TokenType::INT_LITERAL)) {
    auto right_res = parse_primary();
    if (!right_res.has_value())
      return std::unexpected(right_res.error());

    right_node = std::move(right_res.value());
  } else {
    report_error(peek(),
                 std::format("Range operator '...' followed by "
                             "invalid token '{}' ."
                             "Expected an integer literal for a bounded range, "
                             "or nothing for an infinite range.",
                             peek().value));
    return std::unexpected(ParseError::MalformedRangeLiteral);
  }

  return std::make_unique<RangeLiteralASTNode>(std::move(left_node), op_tok,
                                               std::move(right_node));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_expression(Precedence min_prec) {
  const Token token = advance();
  const ParserRule rule = get_rule(token.type);

  if (peek().type == TokenType::END_OF_FILE)
    return std::unexpected(ParseError::UnexpectedToken);

  if (!rule.nud) {
    report_error(token,
                 std::format("Expected start of an expression, but got '{}'",
                             token.value));
    return std::unexpected(ParseError::InvalidExpression);
  }

  auto left_res = (this->*rule.nud)();

  if (!left_res)
    return std::unexpected(left_res.error());

  std::unique_ptr<ASTNode> left = std::move(left_res.value());

  while (!is_at_end() && min_prec < get_rule(peek().type).precedence) {
    const Token next = advance();
    const LedFunc led = get_rule(next.type).led;
    if (led) {
      auto led_res = (this->*led)(std::move(left));
      if (!led_res.has_value())
        return std::unexpected(led_res.error());

      left = std::move(led_res.value());
    }
  }
  return left;
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_literal() {
  return std::make_unique<LiteralASTNode>(tokens[current - 1].value);
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_identifier() {
  return std::make_unique<VariableExprASTNode>(tokens[current - 1].value);
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_prefix() {
  Token op = tokens[current - 1];
  auto operand_res = parse_expression(Precedence::UNARY);

  if (!operand_res.has_value())
    return std::unexpected(operand_res.error());

  return std::make_unique<UnaryExprASTNode>(std::move(operand_res.value()), op,
                                            false);
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_postfix(std::unique_ptr<ASTNode> left) {
  return std::make_unique<UnaryExprASTNode>(std::move(left),
                                            tokens[current - 1], true);
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_binary(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];
  const Precedence next_prec = static_cast<Precedence>(
      static_cast<std::int64_t>(get_rule(op.type).precedence) + 1);
  auto right = parse_expression(next_prec);

  if (!right.has_value())
    return std::unexpected(right.error());

  return std::make_unique<BinaryExprASTNode>(std::move(left), op,
                                             std::move(right.value()));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_member_access(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];

  if (!check(TokenType::IDENTIFIER)) {
    report_error(
        peek(),
        std::format("Invalid member access: After operator '{}', expected an "
                    "identifier (field or method name) but got '{}'.",
                    op.value, peek().value));

    return std::unexpected(ParseError::MalformedMemberAccess);
  }

  const Token member = advance();

  if (!left) {
    report_error(
        op, "Member access operator applied to an invalid or null expression.");
    return std::unexpected(ParseError::InvalidMemberAccessOperand);
  }
  return std::make_unique<MemberAccessASTNode>(std::move(left), op, member);
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_grouping() {
  const Token open_paren = tokens[current - 1];
  auto expr = parse_expression(Precedence::NONE);

  if (!expr)
    return std::unexpected(expr.error());

  auto expr_rr = std::move(expr.value());
  if (!check(TokenType::R_PAREN)) {
    report_error(peek(), "Expected closing ')' after grouping expression");
    report_error(open_paren, "To match this opening paranthesis");
    synchronize();

    return std::unexpected(ParseError::MissingClosingParen);
  }
  advance();
  return expr_rr;
}

static std::string qualified_callee_name(const ASTNode *node) noexcept {
  if (!node)
    return "";
  if (const auto *v = dynamic_cast<const VariableExprASTNode *>(node))
    return v->name;
  if (const auto *m = dynamic_cast<const MemberAccessASTNode *>(node))
    return qualified_callee_name(m->left_side.get()) + m->op.value +
           m->member.value;
  return "ambiguous";
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_call(std::unique_ptr<ASTNode> left) {
  if (!left)
    return std::unexpected(ParseError::InvalidCallTarget);

  const Token open_paren = tokens[current - 1]; // Points to the '('
  std::vector<std::unique_ptr<ASTNode>> args;

  while (!is_at_end() && !check(TokenType::R_PAREN)) {
    auto arg = parse_expression(Precedence::NONE);

    if (!arg) {
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

    args.emplace_back(std::move(arg.value()));

    if (!check(TokenType::COMMA))
      break;
    advance();
  }

  if (!check(TokenType::R_PAREN)) {
    report_error(peek(), "Expected closing ')' after function arguments");
    report_error(open_paren, "To match this opening after paranthesis");
    synchronize();
    return std::unexpected(ParseError::MissingClosingParen);
  }
  advance();

  std::string fn = qualified_callee_name(left.get());
  return std::make_unique<CallASTNode>(fn, std::move(args));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_index(std::unique_ptr<ASTNode> left) {
  const Token open_bracket = tokens[current - 1];
  auto index_expr = parse_expression(Precedence::NONE);
  if (!index_expr) {
    report_error(peek(), "Invalid expression inside array subscript.");
    return std::unexpected(index_expr.error());
  }

  if (!consume(TokenType::RBRACKET, "Expected ']' after array subscript,"))
    return std::unexpected(ParseError::MissingClosingBracket);

  log_mov("Array subscript closed ']'", tokens[current - 1]);

  return std::make_unique<IndexASTNode>(std::move(left), open_bracket,
                                        std::move(index_expr.value()));
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_block() {
  if (!check(TokenType::LBRACE)) {
    report_error(peek(),
                 std::format("Block initialization failed: Expected '{{' to "
                             "open a block, but encountered '{}'",
                             peek().value));
    return std::unexpected(ParseError::MissingOpeningBrace);
  }

  const Token ob = advance();
  log_mov("Block opened '{'", ob);

  std::vector<std::unique_ptr<ASTNode>> stmts;

  while (!is_at_end() && !check(TokenType::RBRACE)) {
    auto stmt = parse_statement();
    if (stmt)
      stmts.emplace_back(std::move(stmt.value()));
    else
      synchronize();
  }

  if (!consume(TokenType::RBRACE,
               std::format("Unterminated block: Started at line {}, but "
                           "missing '}}' to close the scope",
                           ob.line)))
    return std::unexpected(ParseError::MissingClosingBrace);

  log_mov("Block closed '}'", tokens[current - 1]);

  return std::make_unique<BlockASTNode>(std::move(stmts));
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_paren() {
  const Token op = peek();
  if (!consume(TokenType::L_PAREN, std::format("Expected '(' line {}, but "
                                               "missing '(' to open the scope.",
                                               op.line)))
    return std::unexpected(ParseError::MissingOpeningParen);

  log_mov("Opening '('", op);

  auto node = parse_expression(Precedence::NONE);

  if (!node)
    return std::unexpected(node.error());

  if (!consume(TokenType::R_PAREN,
               std::format("Parenthesized expression opened at "
                           "Line {}, Column {} is missing closing ')'",
                           op.line, op.column)))
    return std::unexpected(ParseError::MissingClosingParen);

  log_mov("Closing ')'", tokens[current - 1]);

  return std::move(node.value());
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_import_statement() {
  advance();
  advance();

  const Token lt_tok = peek();

  if (!consume(TokenType::LT, "Malformed Import path must start with '<'"))
    return std::unexpected(ParseError::InvalidImportPath);

  log_mov("Import '<'", lt_tok.line, lt_tok.column);

  std::vector<std::string> path_parts;
  while (!is_at_end()) {
    std::expected<Token, ParseError> id_tok = consume_token(
        TokenType::IDENTIFIER, "Expected module identifier in import path");
    if (!id_tok)
      return std::unexpected(id_tok.error());

    path_parts.emplace_back(id_tok->value);

    if (check(TokenType::DOUBLE_COLON))
      advance();
    else
      break;
  }

  if (!consume(TokenType::GT, "Import path must end with '>'"))
    return std::unexpected(ParseError::InvalidImportPath);

  return std::make_unique<ImportASTNode>(std::move(path_parts));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_paren_expression() {
  auto result = parse_expression(Precedence::NONE);
  if (!result)
    return std::unexpected(result.error());

  if (!consume(TokenType::R_PAREN, "Unclosed parenthesize "
                                   "expression. Expected closing ')'"))
    return std::unexpected(ParseError::MissingClosingParen);

  return std::move(result.value());
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_if_body() {
  // Brace block — always takes precedence.
  if (check(TokenType::LBRACE))
    return parse_block();

  // Optional `:` separator — consume it and fall through to a single statement
  // or brace block on the next line.
  if (check(TokenType::COLON)) {
    advance();

    if (check(TokenType::LBRACE))
      return parse_block();

    auto stmt = parse_statement();

    if (!stmt) {
      report_error(peek(), "Expected a statement or block after ':'");
      return std::unexpected(stmt.error());
    }
    return std::move(stmt.value());
  }

  // Inline single statement with no colon.
  auto stmt = parse_statement();
  if (!stmt) {
    report_error(peek(), "Expected a valid statement as the body of 'if'");
    return std::unexpected(stmt.error());
  }

  return std::move(stmt.value());
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_if_statement() {
  const Token if_tok = advance(); // consume 'if'

  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;

  std::unique_ptr<ASTNode> else_branch = nullptr;
  // if-branch
  auto if_cond = parse_paren();

  if (!if_cond) {
    report_error(if_tok, "'if' keyword must be followed by a "
                         "condition in parantheses '()'.");
    return std::unexpected(if_cond.error());
  }

  auto if_body = parse_if_body();
  if (!if_body)
    return std::unexpected(
        if_body.error()); // Error already handled by body parser

  branches.emplace_back(std::move(if_cond.value()), std::move(if_body.value()));

  while (check(TokenType::ELSEIF)) {
    advance(); // Consume 'else if'

    auto ei_cond = parse_paren();
    if (!ei_cond) {
      report_error(peek(),
                   "'else if' requires a condition in parantheses '()'.");
      return std::unexpected(ei_cond.error());
    }
    auto ei_body = parse_if_body();

    if (!ei_body)
      return std::unexpected(ei_cond.error());

    branches.emplace_back(std::move(ei_cond.value()),
                          std::move(ei_body.value()));
  }

  if (check(TokenType::ELSE)) {
    advance(); // consume 'else'
    auto else_body = parse_if_body();

    if (!else_body) {
      report_error(peek(), "'else' keyword must be followed by a "
                           "statement or block.");
      return std::unexpected(else_body.error());
    }
    else_branch = std::move(else_body.value());
  }

  return std::make_unique<IfASTNode>(std::move(branches),
                                     std::move(else_branch));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_function_statement() {
  advance(); // Consume function

  Token name_tok = peek();
  if (check(TokenType::IDENTIFIER) || check(TokenType::MAIN)) {
    name_tok = advance();
  } else {
    report_error(peek(), std::format(" Expected function name (identifier "
                                     "or 'main'), but found '{}' instead.",
                                     peek().value));
    return std::unexpected(ParseError::ExpectedIdentifier);
  }

  if (!consume(TokenType::L_PAREN, " Expected '(' after function name"))
    return std::unexpected(ParseError::MissingOpeningParen);

  std::vector<ParameterASTNode> params;

  if (check(TokenType::VOID))
    advance();

  else if (!check(TokenType::R_PAREN)) {
    if (!parser_function_parameter_group(params))
      return std::unexpected(
          ParseError::MalformedParameters); // Erro reported inside parameter
                                            // group parser
  }

  if (!consume(TokenType::R_PAREN, " Expected ')' after parameter list"))
    return std::unexpected(ParseError::MissingClosingParen);

  if (!consume(TokenType::ARROW, " Expected '->' return type indicator"))
    return std::unexpected(ParseError::MissingArrow);

  TypeSpecifier ret = parse_type();

  if (ret.is_unknown()) {
    report_error(peek(), " Invalid or missing return type.");
    return std::unexpected(ParseError::InvalidType);
  }

  if (!consume(TokenType::COLON, " Expected ':' before function body"))
    return std::unexpected(ParseError::MissingColon);

  auto body = parse_block();
  if (!body)
    return std::unexpected(body.error());

  std::unique_ptr<ASTNode> body_ptr = std::move(body.value());
  auto *block_ptr = dynamic_cast<BlockASTNode *>(body_ptr.get());

  if (!block_ptr) {
    report_error(peek(), "Function body is not a block.");
    return std::unexpected(ParseError::InternalError);
  }

  auto block_body = std::unique_ptr<BlockASTNode>(
      static_cast<BlockASTNode *>(body_ptr.release()));

  return std::make_unique<FunctionASTNode>(std::string(name_tok.value),
                                           std::move(params), ret,
                                           std::move(block_body));
}

std::expected<std::unique_ptr<ASTNode>, ParseError>
Parser::parse_return_statement() {
  advance();
  std::vector<std::unique_ptr<ASTNode>> return_vals;

  if (check(TokenType::L_PAREN)) {
    advance();

    while (!check(TokenType::R_PAREN) && !is_at_end()) {
      auto val = parse_expression(Precedence::NONE);

      if (!val)
        return std::unexpected(std::move(val.error()));

      if (check(TokenType::COMMA))
        advance();
      else if (!check(TokenType::R_PAREN)) {
        report_error(peek(), "Expected ',' or ')' in return list.");
        return std::unexpected(ParseError::UnexpectedToken);
      }
    }

    if (!consume(TokenType::R_PAREN, "Expected ')' after return list."))
      return std::unexpected(ParseError::MissingClosingParen);
  } else {
    auto val = parse_expression(Precedence::NONE);
    if (!val)
      return std::unexpected(val.error());

    return_vals.emplace_back(std::move(val.value()));
  }

  if (!consume(TokenType::SEMI_COLON, "Expected ';' after return."))
    return std::unexpected(ParseError::MissingSemiColon);

  return std::make_unique<ReturnASTNode>(std::move(return_vals));
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_statement() {
  // #import <...>
  if (check(TokenType::HASH) && peek_next().type == TokenType::IMPORT) {
    return parse_import_statement();
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
  switch (peek().type) {
  case TokenType::LET: {
    auto decl = parse_variable_declaration();
    if (!decl)
      return std::unexpected(decl.error());
    return std::move(decl.value());
  }
  case TokenType::IF:
    return parse_if_statement();

  case TokenType::FUNCTION:
    return parse_function_statement();

  case TokenType::RETURN:
    return parse_return_statement();
  }
#pragma GCC diagnostic pop

  auto expr = parse_primary();

  if (!expr)
    return std::unexpected(ParseError::UnexpectedToken);

  if (!check(TokenType::SEMI_COLON)) {
    report_error(peek(), "Expected ';' at the end of variable declaration");
  } else {
    const Token semi = advance();
    log_mov("Tracking ';'", semi);
  }

  return std::move(expr.value());
}

std::expected<std::unique_ptr<ASTNode>, ParseError> Parser::parse_primary() {
  return parse_expression(Precedence::NONE);
}
