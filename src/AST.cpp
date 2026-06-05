#include "../include/AST.hpp"
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
