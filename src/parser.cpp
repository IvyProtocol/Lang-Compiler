#include "../include/Parser.hpp"
#include <print>

ASTNode::~ASTNode() = default;

LiteralASTNode::LiteralASTNode(std::string_view val) : value(std::move(val)) {}

StubASTNode::StubASTNode(std::string_view n) : name(n) {}

VariableExprASTNode::VariableExprASTNode(std::string_view n) : name(n) {}

VarDeclASTNode::VarDeclASTNode(std::string_view id, TypeSpecifier t,
                               std::unique_ptr<ASTNode> init)
    : identifier(id), type(t), initializer(std::move(init)) {}

AssignmentASTNode::AssignmentASTNode(std::string_view name,
                                     std::unique_ptr<ASTNode> val)
    : variable_name(name), new_value(std::move(val)) {}

CallASTNode::CallASTNode(std::string_view name,
                         std::vector<std::unique_ptr<ASTNode>> args)
    : call(name), arguments(std::move(args)) {}

ArrayLiteralASTNode::ArrayLiteralASTNode(
    std::vector<std::unique_ptr<ASTNode>> elems)
    : elements(std::move(elems)) {}

BinaryExprASTNode::BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token o,
                                     std::unique_ptr<ASTNode> r)
    : left(std::move(l)), op(o), right(std::move(r)) {}

UnaryExprASTNode::UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token o)
    : operand(std::move(opnd)), op(o) {}

BlockASTNode::BlockASTNode(std::vector<std::unique_ptr<ASTNode>> stmts)
    : statements(std::move(stmts)) {}

IfASTNode::IfASTNode(
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
        brs,
    std::unique_ptr<ASTNode> el_br)
    : branches(std::move(brs)), else_branch(std::move(el_br)) {}

void LiteralASTNode::debug_print([[maybe_unused]]const std::string &prefix) const {
  std::println("[Literal]: {}", value);
}

void StubASTNode::debug_print([[maybe_unused]]const std::string &prefix) const {
  std::println("[Stub]: {}", name);
}

void VariableExprASTNode::debug_print([[maybe_unused]]const std::string &prefix) const {
  std::println("[Variable]: {}", name);
}

// --- Declaration Nodes ---
void VarDeclASTNode::debug_print(const std::string &prefix) const {
  std::println("[VarDecl]: {}", identifier);
  std::string next_prefix = prefix + "    ";

  std::println("{}├── Type: {}{}", prefix, type.is_const ? "const " : "",
               type.base_types);
  if (type.is_array) {
    std::println("{}├── Array-Size: {}", prefix, type.arr_size);
  }

  if (initializer) {
    std::print("{}└── Initializer: ", prefix);
    initializer->debug_print(next_prefix);
  }
}

void AssignmentASTNode::debug_print(const std::string &prefix) const {
  std::println("[Assignment]: {}", variable_name);
  if (new_value) {
    std::print("{}└── Value: ", prefix);
    new_value->debug_print(prefix + "    ");
  }
}

void CallASTNode::debug_print(const std::string &prefix) const {
  std::println("[Call]: {}", call);
  for (size_t i = 0; i < arguments.size(); ++i) {
    bool is_last = (i == arguments.size() - 1);
    std::print("{}{}", prefix, is_last ? "└── " : "├── ");
    arguments[i]->debug_print(prefix + (is_last ? "    " : "│   "));
  }
}

void ArrayLiteralASTNode::debug_print(const std::string &prefix) const {
  std::println("[Array]");
  for (size_t i = 0; i < elements.size(); ++i) {
    bool is_last = (i == elements.size() - 1);
    std::print("{}{}", prefix, is_last ? "└── " : "├── ");
    elements[i]->debug_print(prefix + (is_last ? "    " : "│   "));
  }
}

void BinaryExprASTNode::debug_print(const std::string &prefix) const {
  std::println("[Binary]: {}", op.value);

  std::print("{}├── ", prefix);
  if (left)
    left->debug_print(prefix + "│   ");
  else
    std::println("(null)");

  std::print("{}└── ", prefix);
  if (right)
    right->debug_print(prefix + "    ");
  else
    std::println("(null)");
}

void UnaryExprASTNode::debug_print(const std::string &prefix) const {
  std::println("[Unary]: {}", op.value);
  std::print("{}└── ", prefix);
  if (operand)
    operand->debug_print(prefix + "    ");
  else
    std::println("(null)");
}

void BlockASTNode::debug_print(const std::string &prefix) const {
  std::println("[Block]");
  for (size_t i = 0; i < statements.size(); ++i) {
    bool is_last = (i == statements.size() - 1);
    std::print("{}{}", prefix, is_last ? "└── " : "├── ");
    statements[i]->debug_print(prefix + (is_last ? "    " : "│   "));
  }
}

void IfASTNode::debug_print(const std::string &prefix) const {
  std::println("[If Statement]");
  for (size_t i = 0; i < branches.size(); ++i) {
    bool is_last_branch = (i == branches.size() - 1 && !else_branch);

    std::println("{}{}Branch {}", prefix, is_last_branch ? "└── " : "├── ", i);

    std::print("{}    ├── Cond: ", prefix);
    if (branches[i].first) branches[i].first->debug_print(prefix + "    │   ");
    else std::println("[Invalid Condition]");

    std::print("{}    └── Body: ", prefix);
    if (branches[i].second) branches[i].second->debug_print(prefix + "        ");
    else std::println("[Invalid Body]");
  }

  if (else_branch) {
    std::print("{}└── Else: ", prefix);
    else_branch->debug_print(prefix + "    ");
  }
}

void Parser::expect_semicolon() {
  if (!check(TokenType::SEMI_COLON)) {
    std::println("[ERR]: Expected ';' at Line {}, Column {}", peek().line,
                 peek().column);
  } else {
    Token semi = advance();
    std::println("[MOV]: Tracking ';' at Line {}, Column {}", semi.line,
                 semi.column);
  }
}

void Parser::synchronize() {
  advance();

  while (!is_at_end()) {
    if (tokens[current - 1].type == TokenType::SEMI_COLON)
      return;
    switch (peek().type) {
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
      return;

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
    case TokenType::RBRACE:
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
      advance();
      break;
    }
  }
}
Parser::Parser(std::vector<Token> tokens_list)
    : tokens(std::move(tokens_list)) {}

TypeSpecifier Parser::parse_type() {
  TypeSpecifier type_info;

  if (check(TokenType::CONST)) {
    Token t = advance();
    type_info.is_const = true;
    std::println("[MOV]: Tracking 'CONST'. Line: {}, Column: {}", t.line,
                 t.column);
  }

  if (check(TokenType::IDENTIFIER)) {
    Token t = advance();
    type_info.base_types = std::string(t.value);
    std::println("[MOV]: Tracking '{}', Line: {}, Column: {}", t.value, t.line,
                 t.column);
  } else if (check(TokenType::AUTO) || check(TokenType::VOID)) {
    Token t = advance();
    type_info.base_types = std::string(t.value);
    std::println("[MOV]: Tracking '{}', Line: {}, Column: {}", t.value, t.line,
                 t.column);
  } else {
    std::println("[ERR]: Expected a type but got '{}' at Line {}, Column {}",
                 peek().value, peek().line, peek().column);
    type_info.base_types = "unknown";
  }
  return type_info;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parse_program() {
  std::vector<std::unique_ptr<ASTNode>> program_nodes;

  while (!is_at_end()) {
    auto node = parse_statement();

    if (node) {
      program_nodes.emplace_back(std::move(node));
    } else {
      std::println("[ERR]: Syntax error detected at Line {}, Column {}",
                   peek().line, peek().column);
      synchronize();
    }
  }
  return program_nodes;
}

std::unique_ptr<ASTNode> Parser::parse_variable_declaration() {
  advance(); // Eat 'let'

  TypeSpecifier var_type = parse_type();
  if (check(TokenType::LBRACKET)) {
    advance();
    var_type.is_array = true; // Marking it as an array!

    if (check(TokenType::INT_LITERAL)) {
      Token size_token = advance();
      var_type.arr_size = std::stoi(std::string(size_token.value));
      std::println("[MOV]: Array-Declaration Size {} at Line {}, Column {}",
                   size_token.value, size_token.line, size_token.column);
    } else {
      var_type.arr_size = -1;
      std::println(
          "[MOV]: Unsized Array-Declaration processed at Line {}, Column {}",
          peek().line, peek().column);
    }

    if (check(TokenType::RBRACKET)) {
      Token r_bracket = advance();
      std::println(
          "[MOV]: Array-Declaration ended with ']' at Line {}, Column {}",
          r_bracket.line, r_bracket.line, r_bracket.value);
    } else {
      std::println("[ERR]: Expected ']' after Array-Declaration '[' at Line "
                   "{}, Column {}",
                   peek().line, peek().column);
    }
    std::println("[MOV]: Array-Variable has been initialized with an Type.");
  }
  if (check(TokenType::COLON)) {
    advance();
  } else {
    std::println(
        "[ERR]: Expected ':' after type-specifier at Line {}, Column {}",
        peek().line, peek().column);
    return nullptr;
  }

  if (!check(TokenType::IDENTIFIER)) {
    std::println(
        "[ERR]: Expected variable name after ':' at Line {}, Column {}",
        peek().line, peek().column);
    return nullptr;
  }
  Token token_name = advance();

  std::unique_ptr<ASTNode> initializer_expr = nullptr;
  if (check(TokenType::ASSIGN)) {
    Token assign_tok = advance();
    std::println("[MOV]: Tracking previous definer to assingment ':=' at "
                 "Line {}, Column {}",
                 assign_tok.line, assign_tok.column);
    Token value_token = peek();
    initializer_expr = parse_primary();
    std::println("[MOV]: Assigner found body {} at Line {}, Column {}",
                 value_token.value, value_token.line, value_token.column);
  }

  return std::make_unique<VarDeclASTNode>(token_name.value, var_type,
                                          std::move(initializer_expr));
}

std::unique_ptr<ASTNode> Parser::parse_array_literal() {
  Token arr = tokens[current - 1];
  std::vector<std::unique_ptr<ASTNode>> elements;

  if (!check(TokenType::END_OF_FILE) && !check(TokenType::RBRACKET)) {
    do {
      std::unique_ptr<ASTNode> arrays = parse_primary();

      if (!arrays || dynamic_cast<StubASTNode *>(arrays.get()) != nullptr) {
        std::println("[ERR]: Array context broken at Line {}, Column {}. "
                     "Syncing out of bracket group.",
                     arr.line, arr.column);
        synchronize();
        return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
      }
      elements.emplace_back(std::move(arrays));

      if (check(TokenType::COMMA))
        advance();
      else
        break;

    } while (!check(TokenType::END_OF_FILE) && !check(TokenType::RBRACKET));
  }

  if (check(TokenType::RBRACKET))
    advance();
  else
    std::println(
        "[ERR]: Expected closing ']' for array literal at Line {}, Column {}",
        peek().line, peek().column);

  return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
}

std::unique_ptr<ASTNode> Parser::parse_expression(Precedence min_precedence) {
  Token token = advance();
  NudFunc nud_fn = get_rule(token.type).nud;

  if (!nud_fn) {
    return std::make_unique<StubASTNode>(
        std::format("[ERR]: Parse-Error, expected start of a expression, got "
                    "{} at Line {}, Column {}",
                    token.value, token.line, token.column));
  }

  std::unique_ptr<ASTNode> left = (this->*nud_fn)();
  while (!is_at_end() && min_precedence < get_rule(peek().type).precedence) {
    Token next_token = advance();
    LedFunc led_fn = get_rule(next_token.type).led;

    if (led_fn)
      left = (this->*led_fn)(std::move(left));
  }
  return left;
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
  case TokenType::PLUS_PLUS:
  case TokenType::MINUS_MINUS:
    return {Precedence::NONE, &Parser::parse_prefix, nullptr};

  case TokenType::ASSIGN:
  case TokenType::PLUS_EQUAL:
  case TokenType::MINUS_EQUAL:
  case TokenType::MUL_EQUAL:
  case TokenType::DIV_EQUAL:
    return {Precedence::ASSIGNMENT, nullptr, &Parser::parse_binary};
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
  return { std::move(condition_node) };
}

std::unique_ptr<ASTNode> Parser::parse_paren_expression() {
  auto result = parse_expression(Precedence::NONE);

  if (!check(TokenType::R_PAREN)) {
    std::println("[ERR]: Expected closing ')' at Line {}, Column {}",
                 peek().line, peek().column);
    return {nullptr};
  }
  advance();
  return { std::move(result) };
}

std::unique_ptr<ASTNode> Parser::parse_if_statement() {
  advance(); // consume 'if' I am stupid.
  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;
  std::unique_ptr<ASTNode> else_branch = nullptr;

  auto if_cond = parse_paren();
  if ( ! if_cond ) {
    synchronize();
    return nullptr;
  }
  auto if_body = (check(TokenType::LBRACE)) ? parse_block() : parse_statement();
  if ( ! if_body ) {
    synchronize();
    return nullptr;
  }

  branches.emplace_back(std::move(if_cond), std::move(if_body));

  while (check(TokenType::ELSEIF)) {
    advance();
    auto elseif_cond = parse_paren();
    if ( ! elseif_cond ) {
        synchronize();
        return nullptr;
    }
    auto elseif_body =
        check(TokenType::LBRACE) ? parse_block() : parse_statement();
    if ( ! elseif_body ) {
        synchronize();
        return nullptr;
    }
    branches.emplace_back(std::move(elseif_cond), std::move(elseif_body));
  }

  if (check(TokenType::ELSE)) {
    advance();
    else_branch = check(TokenType::LBRACE) ? parse_block() : parse_statement();
    if ( ! else_branch ) {
        synchronize();
        return nullptr;
    }

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
