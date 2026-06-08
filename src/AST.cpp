#include "../include/AST.hpp"
#include <print>

ASTNode::~ASTNode() = default;

LiteralASTNode::LiteralASTNode(std::string_view val) : value(std::move(val)) {}

VariableExprASTNode::VariableExprASTNode(std::string_view n) : name(n) {}

VarDeclASTNode::VarDeclASTNode(std::vector<ParameterASTNode> params,
                               std::vector<std::unique_ptr<ASTNode>> i)
    : parameters(std::move(params)), initializers(std::move(i)) {}

AssignmentASTNode::AssignmentASTNode(std::unique_ptr<ASTNode> lhs, Token op_tok,
                                     std::unique_ptr<ASTNode> val)
    : left_side(std::move(lhs)), op(op_tok), new_value(std::move(val)) {}

CallASTNode::CallASTNode(std::string_view name,
                         std::vector<std::unique_ptr<ASTNode>> args)
    : call(name), arguments(std::move(args)) {}

IndexASTNode::IndexASTNode(std::unique_ptr<ASTNode> arr, Token op_tok,
                           std::unique_ptr<ASTNode> idx)
    : arr_expr(std::move(arr)), op(op_tok), iexpr(std::move(idx)) {}

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

UnaryExprASTNode::UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token o,
                                   bool postfix)
    : operand(std::move(opnd)), op(o), is_postfix(postfix) {}

BlockASTNode::BlockASTNode(std::vector<std::unique_ptr<ASTNode>> stmts)
    : statements(std::move(stmts)) {}

ImportASTNode::ImportASTNode(std::vector<std::string> path)
    : mod_path(std::move(path)) {}

IfASTNode::IfASTNode(
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
        brs,
    std::unique_ptr<ASTNode> el_br)
    : branches(std::move(brs)), else_branch(std::move(el_br)) {}

ParameterASTNode::ParameterASTNode(std::unique_ptr<ASTNode> ne,
                                   TypeSpecifier ty,
                                   std::unique_ptr<RangeLiteralASTNode> rl,
                                   std::unique_ptr<ASTNode> vl)
    : node(std::move(ne)), type(std::move(ty)), defaultValue(std::move(vl)),
      range(std::move(rl)) {}

FunctionASTNode::FunctionASTNode(std::string n, std::vector<ParameterASTNode> p,
                                 std::vector<TypeSpecifier> ret,
                                 std::unique_ptr<BlockASTNode> b)
    : name(std::move(n)), parameters(std::move(p)), return_type(std::move(ret)),
      body(std::move(b)) {}

MemberAccessASTNode::MemberAccessASTNode(std::unique_ptr<ASTNode> lhs,
                                         Token op_tok, Token member_tok)
    : left_side(std::move(lhs)), op(op_tok), member(member_tok) {}

ReturnASTNode::ReturnASTNode(std::vector<std::unique_ptr<ASTNode>> expr)
    : expression(std::move(expr)) {}

NamespaceASTNode::NamespaceASTNode(std::string p,
                                   std::unique_ptr<BlockASTNode> b)
    : parent(std::move(p)), body(std::move(b)) {}

LoopConditionASTNode::LoopConditionASTNode(std::unique_ptr<ASTNode> i,
                                           std::unique_ptr<ASTNode> c,
                                           std::unique_ptr<ASTNode> p)
    : initialize(std::move(i)), condition(std::move(c)), prefix(std::move(p)) {}

ForLoopASTNode::ForLoopASTNode(std::unique_ptr<LoopConditionASTNode> c,
                               std::unique_ptr<ASTNode> b)
    : condition(std::move(c)), block(std::move(b)) {}

// ─── Internal logging helpers
// ───────────────────────────────────────────────── Centralised here so every
// format string is in one place; a mismatch in arguments causes a compile-time
// error in exactly one spot rather than silently blowing up at runtime across
// dozens of call-sites.

bool TypeSpecifier::is_unknown() const {
  return base_types == "unknown" || base_types.empty();
}

void LiteralASTNode::debug_print(
    [[maybe_unused]] const std::string &prefix) const {
  std::println("[Literal]: {}", value);
}

void VariableExprASTNode::debug_print(
    [[maybe_unused]] const std::string &prefix) const {
  std::println("[Variable]: {}", name);
}

void VarDeclASTNode::debug_print(const std::string &prefix) const {
  std::println("[VarDecl]");

  // Scenario 1: Clean Parallel Mapping (e.g., let int: x, y := 1, 2;)
  if (parameters.size() == initializers.size()) {
    size_t total_elements = parameters.size() * 2;
    size_t current_element = 0;

    for (size_t i = 0; i < parameters.size(); ++i) {
      // A. Print Target Parameter
      std::print("{}├── Target: ", prefix);
      parameters[i].debug_print(prefix + "│   ");
      current_element++;

      // B. Print Associated Value
      bool val_is_last = (current_element == total_elements - 1);
      std::string val_branch = val_is_last ? "└── Value:  " : "├── Value:  ";
      std::print("{}{}", prefix, val_branch);

      std::string val_prefix = prefix + (val_is_last ? "    " : "│   ");
      if (initializers[i]) {
        initializers[i]->debug_print(val_prefix);
      } else {
        std::println("[Null Initializer]");
      }
      current_element++;
    }
  }
  // Scenario 2: Tuple Unpacking or Cardinality Mismatch (e.g., let int: x, y :=
  // get_pair();)
  else {
    size_t total_elements = parameters.size() + initializers.size();
    size_t current_element = 0;

    // Print all variable targets first
    for (size_t i = 0; i < parameters.size(); ++i) {
      std::print("{}├── Target: ", prefix);
      parameters[i].debug_print(prefix + "│   ");
      current_element++;
    }

    // Print the remaining initializer expression block (e.g., the tuple object)
    for (size_t i = 0; i < initializers.size(); ++i) {
      bool is_last = (current_element == total_elements - 1);
      std::string branch = is_last ? "└── Source: " : "├── Source: ";
      std::print("{}{}", prefix, branch);

      std::string next_prefix = prefix + (is_last ? "    " : "│   ");
      if (initializers[i]) {
        initializers[i]->debug_print(next_prefix);
      } else {
        std::println("[Null Initializer]");
      }
      current_element++;
    }
  }
}

void AssignmentASTNode::debug_print(const std::string &prefix) const {
  std::println("[Assignment]");

  if (left_side) {
    std::print("{}├── Target: ", prefix);
    // Pass a continuing pipe downward so nested parts of the target line up
    left_side->debug_print(prefix + "│   ");
  }

  if (new_value) {
    std::print("{}└── Value: ", prefix);
    // Pass empty spaces downward because this is the last child of this node
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

void IndexASTNode::debug_print(const std::string &prefix) const {
  std::println("[IndexAccess] {}", op.value);

  if (arr_expr) {
    std::print("{}├── ", prefix);
    arr_expr->debug_print(prefix + "│   ");
  }

  if (iexpr) {
    std::print("{}└── ", prefix);
    iexpr->debug_print(prefix + "    ");
  }
}

void ArrayLiteralASTNode::debug_print(const std::string &prefix) const {
  std::println("[Array]");
  for (size_t i{}; i < elements.size(); ++i) {
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

  std::print("{}├── RHS (Source): ", prefix);
  right ? right->debug_print(prefix + "│   ") : std::println("(null)");

  std::print("{}└── LHS (Target): ", prefix);
  left ? left->debug_print(prefix + "    ") : std::println("(null)");
}

void UnaryExprASTNode::debug_print(const std::string &prefix) const {
  std::string kind = is_postfix ? "Postfix" : "Prefix";
  std::println("[Unary]: {}", kind);
  std::print("{}└── {}", prefix, op.value);
  operand ? operand->debug_print(prefix + "    ") : std::println("(null)");
}

void BlockASTNode::debug_print(const std::string &prefix) const {
  std::println("[Block]");
  for (size_t i{}; i < statements.size(); ++i) {
    const bool last = (i == statements.size() - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");
    statements[i]->debug_print(prefix + (last ? "    " : "│   "));
  }
}

void ImportASTNode::debug_print(const std::string &prefix) const {
  // We prepend the prefix to ensure this line aligns perfectly
  // with the parent's indentation structure.
  std::print("{}Import: ", prefix);

  for (size_t i = 0; i < mod_path.size(); ++i) {
    std::print("{}", mod_path[i]);
    if (i < mod_path.size() - 1) {
      std::print("::");
    }
  }
  std::println(""); // Final newline for the line
}

void IfASTNode::debug_print(const std::string &prefix) const {
  std::println("[If Statement]");
  for (size_t i{}; i < branches.size(); ++i) {
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
  if (type.is_array && type.arr_size) {
    std::print("{}├── Size: ", prefix); // New branch for the array size
    type.arr_size->debug_print(prefix + "│       ");
  } else {
    std::println("{}├── Size: [none]", prefix);
  }

  bool has_range = (range != nullptr);
  std::string default_branch = has_range ? "├──" : "└──";

  std::print("{}{} Default: ", prefix, default_branch);
  if (defaultValue) {
    defaultValue->debug_print(prefix + (has_range ? "│   " : "    "));
  } else {
    std::println("[none]");
  }

  if (has_range) {
    std::print("{}└── Range: ", prefix);
    // Important: Pass the prefix for the LAST item ("    ")
    range->debug_print(prefix + "    ");
  }
}

void FunctionASTNode::debug_print(const std::string &prefix) const {
  std::println("[Function]: {}", name);

  std::string r_str;

  if (return_type.empty())
    r_str = "void";
  else if (return_type.size() == 1)
    r_str = return_type[0].base_types;
  else {
    r_str = "(";
    for (size_t i{}; i < return_type.size(); ++i) {
      r_str += return_type[i].base_types;
      if (i + 1 < return_type.size())
        r_str += ", ";
    }
    r_str += ")";
  }

  std::println("{}├── Returns: {}", prefix, r_str);
  bool has_body = (body != nullptr);
  size_t total_ch = parameters.size() + (has_body ? 1 : 0);

  for (size_t i{}; i < parameters.size(); ++i) {
    const bool last = (i == total_ch - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");
    parameters[i].debug_print(prefix + (last ? "    " : "│   "));
  }
  if (has_body) {
    std::print("{}└── Body: ", prefix);
    body->debug_print(prefix + "    ");
  } else {
    std::print("[Invalid Body]");
  }
}

void MemberAccessASTNode::debug_print(const std::string &prefix) const {
  std::println("[MemberAccess] ({})", op.value);

  if (left_side) {
    std::print("{}├── ", prefix);
    left_side->debug_print(prefix + "│   ");
  }

  std::println("{}└── [Identifier]: {}", prefix, member.value);
}

void ReturnASTNode::debug_print(const std::string &prefix) const {
  std::println("[Return]");

  for (size_t i{}; i < expression.size(); ++i) {
    const bool last = (i == expression.size() - 1);
    std::print("{}{}", prefix, last ? "└── " : "├── ");

    if (expression[i]) {
      expression[i]->debug_print(prefix + (last ? "    " : "│   "));
    } else {
      std::println("[null]");
    }
  }
}

void NamespaceASTNode::debug_print(const std::string &prefix) const {
  std::println("[Namespace]");
  std::print("{}└──", prefix);

  if (body)
    body->debug_print(prefix + "    ");
  else
    std::println("[null]");
}

void LoopConditionASTNode::debug_print(const std::string &indent) const {
  std::println("[LoopCondition]");

  // Collect existing nodes to manage the tree branches
  struct Child {
    std::string label;
    const ASTNode *node;
  };
  std::vector<Child> children;
  if (initialize)
    children.emplace_back("Init", initialize.get());
  if (condition)
    children.emplace_back("Cond", condition.get());
  if (this->prefix)
    children.emplace_back("Prefix", this->prefix.get());

  for (size_t i{}; i < children.size(); ++i) {
    const bool last = (i == children.size() - 1);
    std::print("{}{}{}: ", indent, last ? "└── " : "├── ", children[i].label);

    // Pass the indentation forward
    children[i].node->debug_print(indent + (last ? "    " : "│   "));
  }
}

void ForLoopASTNode::debug_print(const std::string &indent) const {
  std::println("[ForLoop]");

  // 1. Print the Condition (LoopConditionASTNode)
  std::print("{}├── Condition: ", indent);
  if (condition) {
    // We treat the LoopCondition as a child
    condition->debug_print(indent + "│   ");
  } else {
    std::println("[Empty Condition]");
  }

  // 2. Print the Block (Body)
  std::print("{}└── Body: ", indent);
  if (block) {
    block->debug_print(indent + "    ");
  } else {
    std::println("[Invalid Body]");
  }
}
