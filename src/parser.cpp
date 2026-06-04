#include "../include/Parser.hpp"
#include "../include/Logger.hpp"

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

RangeLiteralASTNode::RangeLiteralASTNode(std::unique_ptr<ASTNode> st_node,
                                         Token opTok,
                                         std::unique_ptr<ASTNode> en_node)
    : start(std::move(st_node)), op(opTok), end(std::move(en_node)) {}

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

ParameterASTNode::ParameterASTNode(std::unique_ptr<ASTNode> ne,
                                   TypeSpecifier ty,
                                   std::unique_ptr<RangeLiteralASTNode> rl)
    : node(std::move(ne)), type(ty), range(std::move(rl)) {}

FunctionASTNode::FunctionASTNode(std::string_view n,
                                 std::vector<ParameterASTNode> p,
                                 TypeSpecifier ret,
                                 std::unique_ptr<BlockASTNode> b)
    : name(n), parameters(std::move(p)), return_type(ret), body(std::move(b)) {}

// ─── Internal logging helpers
// ───────────────────────────────────────────────── Centralised here so every
// format string is in one place; a mismatch in arguments causes a compile-time
// error in exactly one spot rather than silently blowing up at runtime across
// dozens of call-sites.

void LiteralASTNode::debug_print(
    [[maybe_unused]] const std::string &prefix) const {
  std::println("[Literal]: {}", value);
}

void StubASTNode::debug_print(
    [[maybe_unused]] const std::string &prefix) const {
  std::println("[Stub]: {}", name);
}

void VariableExprASTNode::debug_print(
    [[maybe_unused]] const std::string &prefix) const {
  std::println("[Variable]: {}", name);
}

void VarDeclASTNode::debug_print(const std::string &prefix) const {
  std::println("[VarDecl]: {}", identifier);
  const std::string child = prefix + "    ";

  std::println("{}├── Type: {}{}", prefix, type.is_const ? "const " : "",
               type.base_types);
  if (type.is_array)
    std::println("{}├── Array-Size: {}", prefix, type.arr_size);

  if (initializer) {
    std::print("{}└── Initializer: ", prefix);
    initializer->debug_print(child);
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
    const bool last = (i == arguments.size() - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");
    arguments[i]->debug_print(prefix + (last ? "    " : "│   "));
  }
}

void ArrayLiteralASTNode::debug_print(const std::string &prefix) const {
  std::println("[Array]");
  for (size_t i = 0; i < elements.size(); ++i) {
    const bool last = (i == elements.size() - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");
    elements[i]->debug_print(prefix + (last ? "    " : "│   "));
  }
}

void RangeLiteralASTNode::debug_print(const std::string &prefix) const {
  std::println("[Range]: {}", op.value);

  std::print("{}├── Start: ", prefix);
  start ? start->debug_print(prefix + "│   ") : std::println("(null)");

  std::print("{}└── End: ", prefix);
  end ? end->debug_print(prefix + "    ") : std::println("(null)");
}

void BinaryExprASTNode::debug_print(const std::string &prefix) const {
  std::println("[Binary]: {}", op.value);

  std::print("{}├── ", prefix);
  left ? left->debug_print(prefix + "│   ") : std::println("(null)");

  std::print("{}└── ", prefix);
  right ? right->debug_print(prefix + "    ") : std::println("(null)");
}

void UnaryExprASTNode::debug_print(const std::string &prefix) const {
  std::println("[Unary]: {}", op.value);
  std::print("{}└── ", prefix);
  operand ? operand->debug_print(prefix + "    ") : std::println("(null)");
}

void BlockASTNode::debug_print(const std::string &prefix) const {
  std::println("[Block]");
  for (size_t i = 0; i < statements.size(); ++i) {
    const bool last = (i == statements.size() - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");
    statements[i]->debug_print(prefix + (last ? "    " : "│   "));
  }
}

void IfASTNode::debug_print(const std::string &prefix) const {
  std::println("[If Statement]");
  for (size_t i = 0; i < branches.size(); ++i) {
    const bool last_branch = (i == branches.size() - 1 && !else_branch);
    std::println("{}{}Branch {}", prefix, last_branch ? "└── " : "├── ", i);

    std::print("{}    ├── Cond: ", prefix);
    branches[i].first ? branches[i].first->debug_print(prefix + "    │   ")
                      : std::println("[Invalid Condition]");

    std::print("{}    └── Body: ", prefix);
    branches[i].second ? branches[i].second->debug_print(prefix + "        ")
                       : std::println("[Invalid Body]");
  }

  if (else_branch) {
    std::print("{}└── Else: ", prefix);
    else_branch->debug_print(prefix + "    ");
  }
}

void ParameterASTNode::debug_print(const std::string &prefix) const {
  std::println("[Parameter]");

  std::print("{}├── Target: ", prefix);
  if (node)
    node->debug_print(prefix + "│   ");
  else
    std::println("[null]");

  std::println("{}├── Type: {}", prefix, type.base_types);

  std::print("{}└── Range: ", prefix);
  if (range)
    range->debug_print(prefix + "    ");
  else
    std::println("[none]");
}

void FunctionASTNode::debug_print(const std::string &prefix) const {
  std::println("[Function]: {}", name);
  std::println("{}└── Returns: {}", prefix, return_type.base_types);
  for (size_t i{}; i < parameters.size(); ++i) {
    const bool last = (i == parameters.size() - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");

    parameters[i].debug_print(prefix + (last ? "    " : "│   "));
  }
  if (body) {
    std::print("{}└── Body: ", prefix);
    body->debug_print(prefix + "    ");
  } else {
    std::print("[Invalid Body]");
  }
}

Parser::Parser(std::vector<Token> tokens_list)
    : tokens(std::move(tokens_list)) {}

std::optional<Token> Parser::consume_token(TokenType type,
                                           std::string_view msg) {
  if (check(type))
    return advance();
  std::string_view token_val = is_at_end() ? "EOF" : peek().value;
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
    if (is_at_end())
      return;
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
    case TokenType::NAMESPACE:
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
    default:
      advance();
      break;
    }
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
  default:
    return {Precedence::NONE, nullptr, nullptr};
  }
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
    auto node = parse_statement();
    if (node) {
      program_nodes.emplace_back(std::move(node));
    } else {
      log_err("Syntax error detected", peek().line, peek().column);
      synchronize();
    }
  }

  return program_nodes;
}

std::unique_ptr<ASTNode> Parser::parse_variable_declaration() {
  advance(); // consume 'let'

  TypeSpecifier var_type = parse_type();

  if (check(TokenType::LBRACKET)) {
    advance();
    var_type.is_array = true;

    if (check(TokenType::INT_LITERAL)) {
      const Token size_tok =
          *consume_token(TokenType::INT_LITERAL, "Expected array size");

      var_type.arr_size = std::stoi(std::string(size_tok.value));
      log_mov(std::format("Array-Declaration size {}", size_tok.value),
              size_tok);
    } else {
      var_type.arr_size = -1;
      log_mov("Unsized array-declaration", peek().line, peek().column);
    }

    const Token rb = peek();
    if (consume(TokenType::RBRACKET,
                "Expected ']' after array-declaration size")) {
      log_mov("Array-Declaration closed ']'", rb);
    }

    log_mov("Array variable type resolved", peek().line, peek().column);
  }

  if (!consume(TokenType::COLON, "Expected ':' after type-specifier")) {
    return nullptr;
  }

  if (!check(TokenType::IDENTIFIER)) {
    consume(TokenType::IDENTIFIER, "Expected variable name after ':'");
    return nullptr;
  }

  const Token name_tok = advance();

  std::unique_ptr<ASTNode> initializer = nullptr;
  if (check(TokenType::ASSIGN)) {
    const Token assign_tok = advance();
    log_mov("Tracking assignment ':='", assign_tok);
    const Token preview = peek();
    initializer = parse_primary();
    log_mov(std::format("Initializer value '{}'", preview.value), preview);
  }

  return std::make_unique<VarDeclASTNode>(name_tok.value, var_type,
                                          std::move(initializer));
}

std::unique_ptr<ASTNode> Parser::parse_array_literal() {
  const Token open_bracket = tokens[current - 1];
  std::vector<std::unique_ptr<ASTNode>> elements;

  while (!check(TokenType::END_OF_FILE) && !check(TokenType::RBRACKET)) {
    auto elem = parse_primary();
    if (!elem || dynamic_cast<StubASTNode *>(elem.get())) {
      log_err("Array element parse failed; syncing out of bracket group",
              open_bracket);
      synchronize();
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
  auto left = parse_primary();
  if (!left)
    return nullptr;
  if (check(TokenType::RANGE)) {
    Token op_tok = advance();

    auto right = parse_primary();
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

  auto left = (this->*nud)();

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
  const Token op = tokens[current - 1];
  auto operand = parse_expression(Precedence::UNARY);
  return std::make_unique<UnaryExprASTNode>(std::move(operand), op);
}

std::unique_ptr<ASTNode> Parser::parse_binary(std::unique_ptr<ASTNode> left) {
  const Token op = tokens[current - 1];
  const auto next_prec = static_cast<Precedence>(
      static_cast<std::int64_t>(get_rule(op.type).precedence) + 1);
  auto right = parse_expression(next_prec);
  return std::make_unique<BinaryExprASTNode>(std::move(left), op,
                                             std::move(right));
}

std::unique_ptr<ASTNode> Parser::parse_memeber_access(
    [[__maybe_unused__]] std::unique_ptr<ASTNode> left) {
  const Token member = advance();
  return std::make_unique<StubASTNode>(
      std::format("Member Access: {} on left node", member.value));
}

std::unique_ptr<ASTNode> Parser::parse_grouping() {
  auto expr = parse_expression(Precedence::NONE);
  consume(TokenType::R_PAREN, "Expected closing ')' after grouping expression");
  return expr;
}

std::unique_ptr<ASTNode> Parser::parse_call(std::unique_ptr<ASTNode> left) {
  const Token open_paren = tokens[current - 1];
  std::vector<std::unique_ptr<ASTNode>> args;

  while (!is_at_end() && !check(TokenType::R_PAREN)) {
    auto arg = parse_expression(Precedence::NONE);

    if (!arg || dynamic_cast<StubASTNode *>(arg.get())) {
      log_err("Call argument parse failed; syncing out of call frame",
              open_paren);
      synchronize();

      std::string fn = "ambiguous";
      if (auto *v = dynamic_cast<VariableExprASTNode *>(left.get()))
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
  if (auto *v = dynamic_cast<VariableExprASTNode *>(left.get()))
    fn = v->name;

  return std::make_unique<CallASTNode>(fn, std::move(args));
}

bool Parser::parser_parameter_group(std::vector<ParameterASTNode> &params) {
  TypeSpecifier group_type = parse_type();

  if (!consume(TokenType::COLON, "Expected ':' after type in parameter group"))
    return false;

  while (!is_at_end()) {
    auto param_name =
        consume_token(TokenType::IDENTIFIER, "Expected parameter name");
    if (!param_name)
      return false;

    std::unique_ptr<ASTNode> param_target =
        std::make_unique<VariableExprASTNode>(param_name->value);

    std::unique_ptr<RangeLiteralASTNode> param_range = nullptr;
    if (check(TokenType::RANGE)) {
      Token op_tok = advance();

      std::unique_ptr<ASTNode> end_node = nullptr;

      if (!check(TokenType::COMMA) && !check(TokenType::R_PAREN)) {
        end_node = parse_primary();
        if (!end_node || dynamic_cast<StubASTNode *>(end_node.get())) {
          log_err("Expected expression after '...' in parameter range", peek());
          return false;
        }
      }

      auto range_start =
          std::make_unique<VariableExprASTNode>(param_name->value);

      param_range = std::make_unique<RangeLiteralASTNode>(
          std::move(range_start), op_tok, std::move(end_node));
    }
    params.emplace_back(std::move(param_target), group_type,
                        std::move(param_range));

    if (!check(TokenType::COMMA))
      break;

    // If there is a comma, look at the token after the comma.
    // If that token is followed by a colon, the the comma
    // we are looking at belongs to the next parameter group. We must stop here!
    // This shit caused me sooo much!
    if (current + 2 < tokens.size() &&
        tokens[current + 2].type == TokenType::COLON)
      break;
    advance(); // Consume the comma safely
  }
  return true;
}

std::unique_ptr<ASTNode> Parser::parse_index(std::unique_ptr<ASTNode> left) {
  const Token open_bracket = tokens[current - 1];
  auto index_expr = parse_expression(Precedence::NONE);

  const Token cb = peek();
  if (consume(TokenType::RBRACKET, "Expected ']' after array index")) {
    log_mov("Array subscript closed ']'", cb);
  }

  return std::make_unique<BinaryExprASTNode>(std::move(left), open_bracket,
                                             std::move(index_expr));
}

std::unique_ptr<ASTNode> Parser::parse_block() {
  const Token ob = peek();
  if (!consume(TokenType::LBRACE, "Expected '{' to open block")) {
    return nullptr;
  }
  log_mov("Block opened '{'", ob);

  std::vector<std::unique_ptr<ASTNode>> stmts;
  while (!is_at_end() && !check(TokenType::RBRACE)) {
    auto stmt = parse_statement();
    if (stmt)
      stmts.emplace_back(std::move(stmt));
    else
      synchronize();
  }
  const Token cb = peek();
  if (consume(TokenType::RBRACE, "Expected '}' to close block"))
    log_mov("Block closed '}'", cb);

  return std::make_unique<BlockASTNode>(std::move(stmts));
}

std::unique_ptr<ASTNode> Parser::parse_paren() {
  const Token op = peek();
  if (!consume(TokenType::L_PAREN, "Expected '('")) {
    return nullptr;
  }
  log_mov("Opening '('", op);

  auto node = check(TokenType::LET) ? parse_variable_declaration()
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
  auto result = parse_expression(Precedence::NONE);
  if (!consume(TokenType::R_PAREN, "Expected closing ')'"))
    return nullptr;
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Weverything"
  return result;
  #pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_if_statement() {
  advance(); // consume 'if'

  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;
  std::unique_ptr<ASTNode> else_branch;

  // if-branch
  auto if_cond = parse_paren();
  if (!if_cond) {
    return nullptr;
  }

  auto if_body = check(TokenType::LBRACE) ? parse_block() : parse_statement();
  if (!if_body) {
    return nullptr;
  }

  branches.emplace_back(std::move(if_cond), std::move(if_body));

  // elseif-branches
  while (check(TokenType::ELSEIF)) {
    advance();
    auto ei_cond = parse_paren();
    if (!ei_cond) {
      return nullptr;
    }

    auto ei_body = check(TokenType::LBRACE) ? parse_block() : parse_statement();
    if (!ei_body) {
      return nullptr;
    }

    branches.emplace_back(std::move(ei_cond), std::move(ei_body));
  }

  // else-branch
  if (check(TokenType::ELSE)) {
    advance();
    else_branch = check(TokenType::LBRACE) ? parse_block() : parse_statement();
    if (!else_branch) {
      return nullptr;
    }
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

  if (!consume(TokenType::L_PAREN, "Expected '(' after function name") &&
      !is_at_end())
    return nullptr;
  std::vector<ParameterASTNode> params;

  if (check(TokenType::VOID))
    advance();
  else if (!check(TokenType::R_PAREN)) {

    while (!is_at_end()) {
      if (!parser_parameter_group(params))
        return nullptr;
      if (check(TokenType::COMMA)) {
        // Lookahead check: If the token after the comma is the closing paren.
        // then it is a trailing comma and break out.
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

  auto body = parse_block();
  if (!body)
    return nullptr;

  auto block_body = std::unique_ptr<BlockASTNode>(
      static_cast<BlockASTNode *>(body.release()));
  return std::make_unique<FunctionASTNode>(name_tok.value, std::move(params),
                                           ret, std::move(block_body));
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
    auto decl = parse_variable_declaration();
    if (!decl)
      return nullptr;
    if (!check(TokenType::SEMI_COLON)) {
      expect_semicolon();
      return nullptr;
    }
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

  // simple assignment:  identifier = expr ;
  if (check(TokenType::IDENTIFIER) && peek_next().type == TokenType::ASSIGN) {
    const Token target = advance();
    advance(); // consume '='
    auto val = parse_expression(Precedence::NONE);
    if (!check(TokenType::SEMI_COLON)) {
      expect_semicolon();
      return nullptr;
    }
    expect_semicolon();
    return std::make_unique<AssignmentASTNode>(target.value, std::move(val));
  }

  // expression statement
  auto expr = parse_primary();
  if (!check(TokenType::SEMI_COLON)) {
    expect_semicolon();
    return nullptr;
  }
  expect_semicolon();
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Weverything"
  return expr;
  #pragma GCC diagnostic pop
}

std::unique_ptr<ASTNode> Parser::parse_primary() {
  return parse_expression(Precedence::NONE);
}
