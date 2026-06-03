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
    return {Precedence::TERM, &Parser::parse_prefix, &Parser::parse_binary};

  case TokenType::MINUS:
    return {Precedence::TERM, &Parser::parse_prefix, &Parser::parse_binary};

  case TokenType::MUL:
  case TokenType::DIV:
  case TokenType::MOD:
    return {Precedence::FACTOR, nullptr, &Parser::parse_binary};

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
  default:
    return {Precedence::NONE, nullptr, nullptr};
  }
}

std::unique_ptr<ASTNode> Parser::parse_literal() {
  return std::make_unique<LiteralASTNode>(tokens[current - 1].value);
}
std::unique_ptr<ASTNode> Parser::parse_identifier() {
  return std::make_unique<VariableExprASTNode>(tokens[current - 1].value);
}
std::unique_ptr<ASTNode> Parser::parse_prefix() {
  Token operator_token = tokens[current - 1];
  auto operand = Parser::parse_expression(Precedence::UNARY);
  return std::make_unique<UnaryExprASTNode>(std::move(operand), operator_token);
}

std::unique_ptr<ASTNode> Parser::parse_binary(std::unique_ptr<ASTNode> left) {
  Token operator_token = tokens[current - 1];
  Precedence precedence = Parser::get_rule(operator_token.type).precedence;
  auto next_precedence =
      static_cast<Precedence>(static_cast<std::int64_t>(precedence) + 1);
  auto right = Parser::parse_expression(next_precedence);
  return std::make_unique<BinaryExprASTNode>(std::move(left), operator_token,
                                             std::move(right));
}

std::unique_ptr<ASTNode>
Parser::parse_memeber_access(std::unique_ptr<ASTNode> /* left */) {
  Token member = advance();
  return std::make_unique<StubASTNode>(
      std::format("Member Access: {} on left node", member.value));
}

std::unique_ptr<ASTNode> Parser::parse_grouping() {
  auto expr = Parser::parse_expression(Precedence::NONE);
  if (check(TokenType::R_PAREN))
    advance();
  return expr;
}

std::unique_ptr<ASTNode> Parser::parse_call(std::unique_ptr<ASTNode> left) {
  Token call = tokens[current - 1];
  std::vector<std::unique_ptr<ASTNode>> args;
  if (!check(TokenType::END_OF_FILE) && !check(TokenType::R_PAREN)) {
    do {
      auto arg = Parser::parse_expression(Precedence::NONE);

      if (!arg || dynamic_cast<StubASTNode *>(arg.get()) != nullptr) {
        std::println("[ERR]: Call parameter syntax error at Line {}, Column "
                     "{}. Syncing out call frame.",
                     call.line, call.column);
        synchronize();

        std::string func_name = "ambiguous";
        if (auto *var_node = dynamic_cast<VariableExprASTNode *>(left.get())) {
          func_name = var_node->name;
        }
        return std::make_unique<CallASTNode>(func_name, std::move(args));
      }
      args.emplace_back(std::move(arg));
      if (check(TokenType::COMMA))
        advance();
      else
        break;
    } while (!is_at_end() && !check(TokenType::R_PAREN));
  }

  if (check(TokenType::R_PAREN)) {
    Token close_paren = advance();
    std::println(
        "[MOV]: Tracked function call closure ')' at Line {}, Column {}",
        close_paren.line, close_paren.column);
  } else {
    std::println(
        "[ERR]: Expected ')' after function arguments at Line {}, Column {}",
        peek().line, peek().column);
  }
  std::string func_name = "ambiguous";
  if (auto *var_node = dynamic_cast<VariableExprASTNode *>(left.get())) {
    func_name = var_node->name;
  }

  return std::make_unique<CallASTNode>(func_name, std::move(args));
}

std::unique_ptr<ASTNode> Parser::parse_index(std::unique_ptr<ASTNode> left) {
  Token open_bracket = tokens[current - 1];
  auto index_expr = Parser::parse_expression(Precedence::NONE);
  if (check(TokenType::RBRACKET)) {
    Token close_bracket = advance(); // consume ']'
    std::println(
        "[MOV]: Tracked array subscription closure ']' at Line {}, Column {}",
        close_bracket.line, close_bracket.column);
  } else {
    std::println("[ERR]: Expected ']' after array index at Line {}, Column {}",
                 peek().line, peek().column);
  }

  return std::make_unique<BinaryExprASTNode>(std::move(left), open_bracket,
                                             std::move(index_expr));
}

std::unique_ptr<ASTNode> Parser::parse_block() {
  std::vector<std::unique_ptr<ASTNode>> statements;

  if (check(TokenType::LBRACE)) {
    Token open_brace = advance();
    std::println("[MOV]: Entered block structure '{{' at Line {}, Column {}",
                 open_brace.line, open_brace.column);
  } else {
    std::println(
        "[ERR]: Expected '{{' to start block frame at Line {}, Column {}",
        peek().line, peek().column);
    return nullptr;
  }

  while (!is_at_end() && !check(TokenType::RBRACE)) {
    auto stmt = parse_statement();
    if (stmt) {
      statements.emplace_back(std::move(stmt));
    } else {
      synchronize();
    }
  }

  if (check(TokenType::RBRACE)) {
    Token close_brace = advance();
    std::println("[MOV]: Closed block structure '}}' at Line {}, Column {}",
                 close_brace.line, close_brace.column);
  } else {
    std::println(
        "[ERR]: Expected '}}' to close block frame at Line {}, Column {}",
        peek().line, peek().column);
    synchronize();
  }

  return std::unique_ptr<ASTNode>(
      std::make_unique<BlockASTNode>(std::move(statements)));
}

std::unique_ptr<ASTNode> Parser::parse_paren() {
  if (!check(TokenType::L_PAREN)) {
    std::println("[ERR]: Expected '(' at Line {}, Column {}", peek().line,
                 peek().column);
    return nullptr;
  }

  Token open_paren = advance();
  std::println("[MOV]: Tracked opening '(' at Line {}, Column {}",
               open_paren.line, open_paren.column);

  auto condition_node = check(TokenType::LET)
                            ? parse_variable_declaration()
                            : parse_expression(Precedence::NONE);

  if (!check(TokenType::R_PAREN)) {
    std::println("[ERR]: Expected closing ')' at Line {}, Column {}",
                 peek().line, peek().column);
    return nullptr;
  }
  advance();
  return condition_node;
}

std::unique_ptr<ASTNode> Parser::parse_paren_expression() {
  auto result = parse_expression(Precedence::NONE);

  if (!check(TokenType::R_PAREN)) {
    std::println("[ERR]: Expected closing ')' at Line {}, Column {}",
                 peek().line, peek().column);
    return nullptr;
  }
  advance();
  return result;
}

std::unique_ptr<ASTNode> Parser::parse_if_statement() {
  advance(); // consume 'if' I am stupid.
  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;
  std::unique_ptr<ASTNode> else_branch = nullptr;

  auto if_cond = parse_paren();
  auto if_body = (check(TokenType::LBRACE)) ? parse_block() : parse_statement();
  branches.emplace_back(std::move(if_cond), std::move(if_body));

  while (check(TokenType::ELSEIF)) {
    advance();
    auto elseif_cond = parse_paren();
    auto elseif_body =
        check(TokenType::LBRACE) ? parse_block() : parse_statement();
    branches.emplace_back(std::move(elseif_cond), std::move(elseif_body));
  }

  if (check(TokenType::ELSE)) {
    advance();
    else_branch = check(TokenType::LBRACE) ? parse_block() : parse_statement();
  }

  return std::unique_ptr<ASTNode>(
      std::make_unique<IfASTNode>(std::move(branches), std::move(else_branch)));
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
  std::unique_ptr<ASTNode> stmt_result;

  if (check(TokenType::HASH) && peek_next().type == TokenType::IMPORT) {
    Token start_token = peek();
    advance();
    advance();
    if (check(TokenType::LT)) {
      std::println("[MOV]: Advanced to '<'. Line: {}, Column: {}", peek().line,
                   peek().column);
      advance();
      while (!is_at_end() && !check(TokenType::GT))
        advance();
      if (check(TokenType::GT)) {
        std::println("[MOV]: Advanced to '>'. Line: {}, Column: {}",
                     peek().line, peek().column);
        advance();
      } else {
        std::println("[ERR]: Could not advance to '>', is it missing? Line: "
                     "{}, Column: {}",
                     peek().line, peek().column);
      }
    } else {
      std::println("[ERR]: Could not advance to '<', is it missing? Line: "
                   "{}, Column: {}",
                   peek().line, peek().column);
    }
    stmt_result = std::make_unique<StubASTNode>(
        std::format("Import Statement: Line {}, Column {}", start_token.line,
                    start_token.column));
    return stmt_result;
  }

  if (check(TokenType::LET)) {
    stmt_result = Parser::parse_variable_declaration();
    if (!stmt_result) {
      return nullptr;
    }
    if (!check(TokenType::SEMI_COLON)) {
      expect_semicolon();
      return nullptr;
    }
    expect_semicolon();
    return stmt_result;
  }

  if (check(TokenType::IF)) {
    return Parser::parse_if_statement();
  }

  if (check(TokenType::IDENTIFIER) && peek_next().type == TokenType::ASSIGN) {
    Token target_var = advance();
    advance();

    auto value = parse_expression(Precedence::NONE);

    if (!check(TokenType::SEMI_COLON)) {
      expect_semicolon();
      return nullptr;
    }
    expect_semicolon();
    stmt_result =
        std::make_unique<AssignmentASTNode>(target_var.value, std::move(value));
    return stmt_result;
  }

  stmt_result = parse_primary();
  if (!check(TokenType::SEMI_COLON)) {
    expect_semicolon();
    return nullptr;
  }
  expect_semicolon();
  return stmt_result;
}
std::unique_ptr<ASTNode> Parser::parse_primary() {
  return parse_expression(Precedence::NONE);
}
