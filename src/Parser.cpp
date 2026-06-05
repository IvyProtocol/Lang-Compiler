#include "Parser.hpp"
#include "AST.hpp"
#include "Logger.hpp"

Parser::Parser(std::vector<Token> tokens_list)
    : tokens(std::move(tokens_list)) {}

std::optional<Token> Parser::consume_token(TokenType type,
                                           std::string_view msg) {
  if (check(type))
    return advance();
  const std::string_view token_val = is_at_end() ? "EOF" : peek().value;
  log_err(std::format("{} but got '{}' ", msg, token_val), peek());
  synchronize();
  return std::nullopt;
}

// For when I need to verify a symbol ( e.g.. '(', '->') Yeah, no. I can't type.
bool Parser::consume(TokenType type, std::string_view msg) {
  if (check(type)) {
    advance();
    return true;
  }

  log_err(std::format("{} but got '{}'", msg, peek().value), peek());
  synchronize();
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
  case TokenType::BOOL:
  case TokenType::AUTO:
  case TokenType::CONST:
  case TokenType::VOID:
  case TokenType::IDENTIFIER:
  case TokenType::END_OF_FILE:
    return false;
  }
  return false;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
bool Parser::is_mutating_operator(TokenType type) {
  switch (type) {
  case TokenType::ASSIGN:
  case TokenType::PLUS_EQUAL:
  case TokenType::MINUS_EQUAL:
  case TokenType::MUL_EQUAL:
  case TokenType::DIV_EQUAL:
  case TokenType::PLUS_PLUS:
  case TokenType::MINUS_MINUS:
    return true;
  default:
    return false;
  }
}
#pragma GCC diagnostic pop

void Parser::expect_semicolon() {
  if (!check(TokenType::SEMI_COLON)) {
    log_err("Expected ';'", peek());
  } else {
    const Token semi = advance();
    log_mov("Tracking ';'", semi);
  }
}

void Parser::synchronize() {
  advance();
  while (!is_at_end()) {
    if (tokens[current - 1].type == TokenType::SEMI_COLON)
      return;
    if (is_statement_start(peek().type))
      return;
    advance();
  }
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
    return {Precedence::MEMBER_ACCESS, nullptr, &Parser::parse_memeber_access};

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
  case TokenType::BOOL:
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
  }

  while (!check(TokenType::COLON) && !check(TokenType::LBRACE) &&
         !check(TokenType::R_PAREN) && !check(TokenType::COMMA) &&
         !check(TokenType::ARROW) && !is_at_end()) {
    const Token t = advance();
    full_type_name += t.value;
  }

  if (full_type_name.empty()) {
    log_err(std::format("Expected a type, got '{}'", peek().value), peek());
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
      log_err("Syntax error detected", peek().line, peek().column);
      synchronize();
    }
  }

  return program_nodes;
}

std::unique_ptr<ASTNode>
Parser::parse_assignment(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];

  std::unique_ptr<ASTNode> right = parse_expression(Precedence::NONE);
  return std::make_unique<BinaryExprASTNode>(std::move(left), op,
                                             std::move(right));
}

std::unique_ptr<ASTNode> Parser::parse_variable_declaration() {
  advance();
  std::vector<ParameterASTNode> all_vars;
  while (!check(TokenType::SEMI_COLON) && !is_at_end()) {
    if (!parser_variable_parameter_group(all_vars))
      return nullptr;

    if (check(TokenType::COMMA))
      advance();
  }
  return std::make_unique<VarDeclASTNode>(std::move(all_vars));
}

std::unique_ptr<ASTNode> Parser::parse_array_literal() {
  const Token open_bracket = tokens[current - 1];
  std::vector<std::unique_ptr<ASTNode>> elements;

  while (!check(TokenType::END_OF_FILE) && !check(TokenType::RBRACKET)) {
    std::unique_ptr<ASTNode> elem = parse_primary();
    if (!elem || dynamic_cast<StubASTNode *>(elem.get())) {
      log_err("Array element parse failed; syncing out of bracket group",
              open_bracket);
      return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
    }
    elements.emplace_back(std::move(elem));
    if (!check(TokenType::COMMA))
      break;
    advance();
  }
  consume(TokenType::RBRACKET, "Expected ']' to close array literal");
  return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
}

std::unique_ptr<ASTNode> Parser::parse_range_literal() {
  std::unique_ptr<ASTNode> left = parse_primary();
  if (!left)
    return nullptr;

  if (check(TokenType::RANGE)) {
    const Token op_tok = advance();
    std::unique_ptr<ASTNode> right = parse_primary();
    if (!right) {
      log_err("Expected expression after '...'", peek());
      return nullptr;
    }

    return std::make_unique<RangeLiteralASTNode>(std::move(left), op_tok,
                                                 std::move(right));
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return left;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_expression(Precedence min_prec) {
  const Token token = advance();
  const NudFunc nud = get_rule(token.type).nud;

  if (!nud) {
    return std::make_unique<StubASTNode>(
        std::format("[ERR]: Expected start of expression, got '{}' "
                    "at Line {}, Column {}",
                    token.value, token.line, token.column));
  }

  std::unique_ptr<ASTNode> left = (this->*nud)();

  while (!is_at_end() && min_prec < get_rule(peek().type).precedence) {
    const Token next = advance();
    const LedFunc led = get_rule(next.type).led;
    if (led)
      left = (this->*led)(std::move(left));
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
  return std::make_unique<BinaryExprASTNode>(std::move(left), op,
                                             std::move(right));
}

std::unique_ptr<ASTNode>
Parser::parse_memeber_access([[maybe_unused]] std::unique_ptr<ASTNode> left) {
  const Token member = advance();
  return std::make_unique<StubASTNode>(
      std::format("Member Access: {} on left node", member.value));
}

std::unique_ptr<ASTNode> Parser::parse_grouping() {
  std::unique_ptr<ASTNode> expr = parse_expression(Precedence::NONE);
  consume(TokenType::R_PAREN, "Expected closing ')' after grouping expression");
  return expr;
}

std::unique_ptr<ASTNode> Parser::parse_call(std::unique_ptr<ASTNode> left) {
  const Token open_paren = tokens[current - 1];
  std::vector<std::unique_ptr<ASTNode>> args;

  while (!is_at_end() && !check(TokenType::R_PAREN)) {
    std::unique_ptr<ASTNode> arg = parse_expression(Precedence::NONE);

    if (!arg || dynamic_cast<StubASTNode *>(arg.get())) {
      log_err("Call argument parse failed; syncing out of call frame",
              open_paren);

      std::string fn = "ambiguous";
      if (const VariableExprASTNode *v =
              dynamic_cast<VariableExprASTNode *>(left.get()))
        fn = v->name;
      return std::make_unique<CallASTNode>(fn, std::move(args));
    }

    args.emplace_back(std::move(arg));
    if (!check(TokenType::COMMA))
      break;
    advance();
  }

  const Token cp = peek();
  if (consume(TokenType::R_PAREN, "Expected ')' after function arguments")) {
    log_mov("Function call closed ')'", cp);
  }

  std::string fn = "ambiguous";
  if (const VariableExprASTNode *v =
          dynamic_cast<VariableExprASTNode *>(left.get()))
    fn = v->name;

  return std::make_unique<CallASTNode>(fn, std::move(args));
}

bool Parser::parse_parameter_list(std::vector<ParameterASTNode> &params,
                                  bool allow_ranges, TokenType term_type) {
  TypeSpecifier group_type = parse_type();

  if (!consume(TokenType::COLON, "Expected ':' after type in parameter group"))
    return false;

  while (!is_at_end()) {
    std::optional<Token> param_name =
        consume_token(TokenType::IDENTIFIER, "Expected parameter name");
    if (!param_name)
      return false;

    std::unique_ptr<ASTNode> default_value = nullptr;
    std::unique_ptr<ASTNode> param_target =
        std::make_unique<VariableExprASTNode>(param_name->value);
    std::unique_ptr<RangeLiteralASTNode> param_range = nullptr;

    if (check(TokenType::ASSIGN)) {
      advance();
      default_value = parse_primary();
    }

    if (allow_ranges && check(TokenType::RANGE)) {
      Token op_tok = advance();

      std::unique_ptr<ASTNode> end_node = nullptr;

      if (!check(TokenType::COMMA) && !check(term_type)) {
        end_node = parse_primary();
        if (!end_node || dynamic_cast<StubASTNode *>(end_node.get())) {
          log_err("Expected expression after '...' in parameter_range", peek());
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
    if (!check(TokenType::COMMA))
      break;

    // If the token two positions ahead is a colon, the comma introduces the
    // next parameter group — stop consuming names in this group.
    if (current + 2 < tokens.size() &&
        tokens[current + 2].type == TokenType::COLON)
      break;
    advance();
  }
  return true;
}

bool Parser::parser_function_parameter_group(
    std::vector<ParameterASTNode> &params) {
  return parse_parameter_list(params, true, TokenType::R_PAREN);
}

bool Parser::parser_variable_parameter_group(
    std::vector<ParameterASTNode> &params) {
  return parse_parameter_list(params, false, TokenType::SEMI_COLON);
}

std::unique_ptr<ASTNode> Parser::parse_index(std::unique_ptr<ASTNode> left) {
  const Token open_bracket = tokens[current - 1];
  std::unique_ptr<ASTNode> index_expr = parse_expression(Precedence::NONE);

  const Token cb = peek();
  if (consume(TokenType::RBRACKET, "Expected ']' after array index"))
    log_mov("Array subscript closed ']'", cb);

  return std::make_unique<BinaryExprASTNode>(std::move(left), open_bracket,
                                             std::move(index_expr));
}

std::unique_ptr<ASTNode> Parser::parse_block() {
  if (!check(TokenType::LBRACE)) {
    log_err("Expected '{' to open block", peek());
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

  if (!consume(TokenType::RBRACE, "Expected '}' to close block"))
    return nullptr;

  log_mov("Block closed '}'", peek());

  return std::make_unique<BlockASTNode>(std::move(stmts));
}

std::unique_ptr<ASTNode> Parser::parse_paren() {
  const Token op = peek();
  if (!consume(TokenType::L_PAREN, "Expected '('"))
    return nullptr;
  log_mov("Opening '('", op);

  std::unique_ptr<ASTNode> node = check(TokenType::LET)
                                      ? parse_variable_declaration()
                                      : parse_expression(Precedence::NONE);
  const Token cp = peek();
  if (!consume(TokenType::R_PAREN, "Expected closing ')'"))
    return nullptr;
  log_mov("Closing ')'", cp);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return node;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_paren_expression() {
  std::unique_ptr<ASTNode> result = parse_expression(Precedence::NONE);
  if (!consume(TokenType::R_PAREN, "Expected closing ')'"))
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
    // Author may write `if (cond): { ... }` — handle that gracefully.
    if (check(TokenType::LBRACE))
      return parse_block();
    return parse_statement();
  }

  // Inline single statement with no colon.
  return parse_statement();
}

std::unique_ptr<ASTNode> Parser::parse_if_statement() {
  advance(); // consume 'if'

  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;
  std::unique_ptr<ASTNode> else_branch = nullptr;

  // if-branch
  std::unique_ptr<ASTNode> if_cond = parse_paren();
  std::unique_ptr<ASTNode> if_body = parse_if_body();
  if (!if_cond || !if_body)
    return nullptr;

  branches.emplace_back(std::move(if_cond), std::move(if_body));

  while (check(TokenType::ELSEIF)) {
    advance();
    std::unique_ptr<ASTNode> ei_cond = parse_paren();
    std::unique_ptr<ASTNode> ei_body = parse_if_body();

    if (!ei_cond || !ei_body)
      return nullptr;
    branches.emplace_back(std::move(ei_cond), std::move(ei_body));
  }

  if (check(TokenType::ELSE)) {
    advance();
    else_branch = parse_if_body();

    if (!else_branch)
      return nullptr;
  }

  return std::make_unique<IfASTNode>(std::move(branches),
                                     std::move(else_branch));
}

std::unique_ptr<ASTNode> Parser::parse_function_statement() {
  advance(); // Consume function

  Token name_tok;
  if (check(TokenType::IDENTIFIER) || check(TokenType::MAIN)) {
    name_tok = advance();
  } else {
    log_err("Expected function name (identifier or 'main')", peek());
    return nullptr;
  }

  if (!consume(TokenType::L_PAREN, "Expected '(' after function name"))
    return nullptr;

  std::vector<ParameterASTNode> params;

  if (check(TokenType::VOID))
    advance();

  else if (!check(TokenType::R_PAREN)) {
    while (!is_at_end()) {
      if (!parser_function_parameter_group(params))
        return nullptr;
      if (check(TokenType::COMMA)) {
        // Trailing comma: comma immediately followed by ')'.
        if (tokens[current + 1].type == TokenType::R_PAREN) {
          advance();
          log_err("Trailing closing paren after comma", peek());
          break;
        }
        advance();
      } else {
        break;
      }
    }
  }

  if (!consume(TokenType::R_PAREN, "Expected ')' after parameters"))
    return nullptr;

  if (!consume(TokenType::ARROW, "Expected '->' before return type"))
    return nullptr;

  TypeSpecifier ret = parse_type();
  if (!consume(TokenType::COLON, "Expected ':' after return type"))
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
    const Token start = peek();
    advance();
    advance(); // consume '#' and 'import'

    const Token lt_tok = peek();
    if (consume(TokenType::LT, "Missing '<' in import directive")) {
      log_mov("Import '<'", lt_tok.line, lt_tok.column);

      while (!is_at_end() && !check(TokenType::GT))
        advance();

      const Token gt_tok = peek();
      if (consume(TokenType::GT, "Missing '>' in import directive"))
        log_mov("Import '>'", gt_tok.line, gt_tok.column);
    }

    return std::make_unique<StubASTNode>(std::format(
        "Import Statement: Line {}, Column {}", start.line, start.column));
  }

  // let declaration
  if (check(TokenType::LET)) {
    std::unique_ptr<ASTNode> decl = parse_variable_declaration();
    if (!decl)
      return nullptr;
    expect_semicolon();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
    return decl;
#pragma GCC diagnostic pop
  }

  // if statement
  if (check(TokenType::IF))
    return parse_if_statement();

  if (check(TokenType::FUNCTION))
    return parse_function_statement();

  if (check(TokenType::RETURN)) {
    advance();
    if (!check(TokenType::SEMI_COLON))
      parse_primary(); // parse and discard until semicolon is implemented
    expect_semicolon();
    return std::make_unique<StubASTNode>("Return ASTNode");
  }

  // expression statement
  auto expr = parse_primary();
  expect_semicolon();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
  return expr;
#pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_primary() {
  return parse_expression(Precedence::NONE);
}
