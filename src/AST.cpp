#include "../include/AST.hpp"
#include <print>

ASTNode::~ASTNode() = default;

LiteralASTNode::LiteralASTNode(const std::string_view val) : value(val) {}

constexpr std::unique_ptr<ASTNode> LiteralASTNode::clone() const {
  return std::make_unique<LiteralASTNode>(value);
}

VariableExprASTNode::VariableExprASTNode(const std::string_view n) : name(n) {}

constexpr std::unique_ptr<ASTNode> VariableExprASTNode::clone() const {
  return std::make_unique<VariableExprASTNode>(name);
}

VarDeclASTNode::VarDeclASTNode(std::vector<ParameterASTNode> params,
                               std::vector<std::unique_ptr<ASTNode>> i)
    : parameters(std::move(params)), initializers(std::move(i)) {}

constexpr std::unique_ptr<ASTNode> VarDeclASTNode::clone() const {
  std::vector<ParameterASTNode> cloned_params;
  cloned_params.reserve(parameters.size());
  for (const auto &params : parameters) {
    std::unique_ptr<ParameterASTNode> p(
        dynamic_cast<ParameterASTNode *>(params.clone().release()));
    cloned_params.emplace_back(std::move(*p));
  }

  std::vector<std::unique_ptr<ASTNode>> cloned_inits;
  cloned_inits.reserve(initializers.size());

  for (const auto &init : initializers)
    cloned_inits.emplace_back(init ? init->clone() : nullptr);

  return std::make_unique<VarDeclASTNode>(std::move(cloned_params),
                                          std::move(cloned_inits));
}

AssignmentASTNode::AssignmentASTNode(std::unique_ptr<ASTNode> lhs, Token const& op_tok,
                                     std::unique_ptr<ASTNode> val)
    : left_side(std::move(lhs)), op(op_tok), new_value(std::move(val)) {}

constexpr std::unique_ptr<ASTNode> AssignmentASTNode::clone() const {
  return std::make_unique<AssignmentASTNode>(
      left_side ? left_side->clone() : nullptr, op,
      new_value ? new_value->clone() : nullptr);
}

CallASTNode::CallASTNode(const std::string_view name,
                         std::vector<std::unique_ptr<ASTNode>> args)
    : call(name), arguments(std::move(args)) {}

constexpr std::unique_ptr<ASTNode> CallASTNode::clone() const {
  std::vector<std::unique_ptr<ASTNode>> cloned_args;
  cloned_args.reserve(arguments.size());
  for (const auto &arg : arguments) {
    cloned_args.emplace_back(arg ? arg->clone() : nullptr);
  }
  return std::make_unique<CallASTNode>(call, std::move(cloned_args));
}

IndexASTNode::IndexASTNode(std::unique_ptr<ASTNode> arr, Token const& op_tok,
                           std::unique_ptr<ASTNode> idx)
    : arr_expr(std::move(arr)), op(op_tok), iexpr(std::move(idx)) {}

constexpr std::unique_ptr<ASTNode> IndexASTNode::clone() const {
  return std::make_unique<IndexASTNode>(arr_expr ? arr_expr->clone() : nullptr,
                                        op, iexpr ? iexpr->clone() : nullptr);
}

ArrayLiteralASTNode::ArrayLiteralASTNode(
    std::vector<std::unique_ptr<ASTNode>> elems)
    : elements(std::move(elems)) {}

constexpr std::unique_ptr<ASTNode> ArrayLiteralASTNode::clone() const {
  std::vector<std::unique_ptr<ASTNode>> cloned_elems;
  cloned_elems.reserve(elements.size());
  for (const auto &elem : elements) {
    cloned_elems.emplace_back(elem ? elem->clone() : nullptr);
  }
  return std::make_unique<ArrayLiteralASTNode>(std::move(cloned_elems));
}

RangeLiteralASTNode::RangeLiteralASTNode(std::unique_ptr<ASTNode> start_node,
                                         Token const& opTok,
                                         std::unique_ptr<ASTNode> end_node)
    : start(std::move(start_node)), op(opTok), end(std::move(end_node)) {}

constexpr std::unique_ptr<ASTNode> RangeLiteralASTNode::clone() const {
  return std::make_unique<RangeLiteralASTNode>(
      start ? start->clone() : nullptr, op, end ? end->clone() : nullptr);
}

BinaryExprASTNode::BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token const& o,
                                     std::unique_ptr<ASTNode> r)
    : left(std::move(l)), op(o), right(std::move(r)) {}

constexpr std::unique_ptr<ASTNode> BinaryExprASTNode::clone() const {
  return std::make_unique<BinaryExprASTNode>(left ? left->clone() : nullptr, op,
                                             right ? right->clone() : nullptr);
}

UnaryExprASTNode::UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token const& o,
                                   const bool postfix, const bool mut)
    : operand(std::move(opnd)), op(o), is_postfix(postfix), is_mut(mut) {}

constexpr std::unique_ptr<ASTNode> UnaryExprASTNode::clone() const {
  return std::make_unique<UnaryExprASTNode>(
      operand ? operand->clone() : nullptr, op, is_postfix, is_mut);
}

BlockASTNode::BlockASTNode(std::vector<std::unique_ptr<ASTNode>> stmts)
    : statements(std::move(stmts)) {}

constexpr std::unique_ptr<ASTNode> BlockASTNode::clone() const {
  std::vector<std::unique_ptr<ASTNode>> cloned_stmts;
  cloned_stmts.reserve(statements.size());
  for (const auto &stmt : statements) {
    cloned_stmts.emplace_back(stmt ? stmt->clone() : nullptr);
  }
  return std::make_unique<BlockASTNode>(std::move(cloned_stmts));
}


ImportASTNode::ImportASTNode(std::vector<std::string> path)
    : mod_path(std::move(path)) {}

constexpr std::unique_ptr<ASTNode> ImportASTNode::clone() const {
  return std::make_unique<ImportASTNode>(mod_path);
}


IfBranch::IfBranch(std::unique_ptr<ASTNode> init, std::unique_ptr<ASTNode>  cond, std::unique_ptr<ASTNode> b)
  : initializer(std::move(init)), condition(std::move(cond)), body(std::move(b)) {}

constexpr IfBranch IfBranch::clone() const
{
  return {
    initializer ? initializer->clone() : nullptr,
    condition ? condition->clone() : nullptr,
    body ? body->clone() : nullptr
  };
}

IfASTNode::IfASTNode(
    std::vector<IfBranch>
        brs,
    std::unique_ptr<ASTNode> el_br)
    : branches(std::move(brs)), else_branch(std::move(el_br)) {}

constexpr std::unique_ptr<ASTNode> IfASTNode::clone() const {
  std::vector<IfBranch> cloned_branches;
  cloned_branches.reserve(branches.size());
  for (const auto &branch : branches) {
    cloned_branches.emplace_back(branch.clone());
  }
  return std::make_unique<IfASTNode>(
      std::move(cloned_branches), else_branch ? else_branch->clone() : nullptr);
}

ParameterASTNode::ParameterASTNode(std::unique_ptr<ASTNode> n,
                                   TypeSpecifier t,
                                   std::unique_ptr<RangeLiteralASTNode> r,
                                   std::unique_ptr<ASTNode> v)
    : node(std::move(n)), type(std::move(t)), defaultValue(std::move(v)),
      range(std::move(r)) {}

constexpr std::unique_ptr<ASTNode> ParameterASTNode::clone() const {
  std::unique_ptr<RangeLiteralASTNode> cloned_range = nullptr;
  if (range) {
    cloned_range.reset(
        dynamic_cast<RangeLiteralASTNode *>(range->clone().release()));
  }
  return std::make_unique<ParameterASTNode>(
      node ? node->clone() : nullptr, type.clone(), std::move(cloned_range),
      defaultValue ? defaultValue->clone() : nullptr);
}

FunctionASTNode::FunctionASTNode(std::string n, std::vector<ParameterASTNode> params,
                                 std::vector<TypeSpecifier> ret,
                                 std::unique_ptr<BlockASTNode> b)
    : name(std::move(n)), parameters(std::move(params)), return_type(std::move(ret)),
      body(std::move(b)) {}

constexpr std::unique_ptr<ASTNode> FunctionASTNode::clone() const {
  std::vector<ParameterASTNode> cloned_params;
  cloned_params.reserve(parameters.size());
  for (const auto &param : parameters) {
    // 1. Clone returns a unique_ptr.
    // 2. Dereference it (*) to get the raw object value for the vector.
    auto cloned_param_ptr = param.clone();
    if (cloned_param_ptr) {
      // Cast if clone() returns unique_ptr<ASTNode> instead of
      // unique_ptr<ParameterASTNode>
      auto *raw_param = dynamic_cast<ParameterASTNode *>(cloned_param_ptr.get());
      cloned_params.emplace_back(std::move(*raw_param));
    }
  }

  std::vector<TypeSpecifier> cloned_ret;
  cloned_ret.reserve(return_type.size());
  for (const auto &ret : return_type) {
    cloned_ret.emplace_back(ret.clone());
  }

  std::unique_ptr<BlockASTNode> cloned_body = nullptr;
  if (body) {
    cloned_body.reset(dynamic_cast<BlockASTNode *>(body->clone().release()));
  }

  // Finish the return statement that was cut off
  return std::make_unique<FunctionASTNode>(name, std::move(cloned_params),
                                           std::move(cloned_ret),
                                           std::move(cloned_body));
}

MemberAccessASTNode::MemberAccessASTNode(std::unique_ptr<ASTNode> lhs,
                                         Token const& op_tok, Token const& member_tok)
    : left_side(std::move(lhs)), op(op_tok), member(member_tok) {}

constexpr std::unique_ptr<ASTNode> MemberAccessASTNode::clone() const {
  return std::make_unique<MemberAccessASTNode>(
      left_side ? left_side->clone() : nullptr, op, member);
}

BreakASTNode::BreakASTNode() = default;

constexpr std::unique_ptr<ASTNode> BreakASTNode::clone() const {
  return std::make_unique<BreakASTNode>();
}

ContinueASTNode::ContinueASTNode() = default;

constexpr std::unique_ptr<ASTNode> ContinueASTNode::clone() const {
  return std::make_unique<ContinueASTNode>();
}

LengthASTNode::LengthASTNode(std::unique_ptr<ASTNode> len)
    : len_name(std::move(len)) {}

constexpr std::unique_ptr<ASTNode> LengthASTNode::clone() const {
  return std::make_unique<LengthASTNode>(len_name->clone());
}

ReturnASTNode::ReturnASTNode(std::vector<std::unique_ptr<ASTNode>> expr)
    : expression(std::move(expr)) {}

constexpr std::unique_ptr<ASTNode> ReturnASTNode::clone() const {
  std::vector<std::unique_ptr<ASTNode>> cloned_exprs;
  cloned_exprs.reserve(expression.size());
  for (const auto &expr : expression) {
    cloned_exprs.emplace_back(expr ? expr->clone() : nullptr);
  }
  return std::make_unique<ReturnASTNode>(std::move(cloned_exprs));
}

JoinASTNode::JoinASTNode(std::vector<std::unique_ptr<ASTNode>> expr)
    : expression(std::move(expr)) {}

constexpr std::unique_ptr<ASTNode> JoinASTNode::clone() const {
  std::vector<std::unique_ptr<ASTNode>> cloned_exprs;
  cloned_exprs.reserve(expression.size());
  for (const auto &expr : expression) {
    cloned_exprs.emplace_back(expr ? expr->clone() : nullptr);
  }
  return std::make_unique<JoinASTNode>(std::move(cloned_exprs));
}

NamespaceASTNode::NamespaceASTNode(std::string p,
                                   std::unique_ptr<BlockASTNode> b)
    : parent(std::move(p)), body(std::move(b)) {}

constexpr std::unique_ptr<ASTNode> NamespaceASTNode::clone() const {
  std::unique_ptr<BlockASTNode> cloned_body = nullptr;
  if (body) {
    cloned_body.reset(dynamic_cast<BlockASTNode *>(body->clone().release()));
  }
  return std::make_unique<NamespaceASTNode>(parent, std::move(cloned_body));
}

NamespaceAliasASTNode::NamespaceAliasASTNode(std::string alias, std::string target)
    : alias_name(std::move(alias)), target_name(std::move(target)) {}

constexpr std::unique_ptr<ASTNode> NamespaceAliasASTNode::clone() const {
  return std::make_unique<NamespaceAliasASTNode>(alias_name, target_name);
}

LoopConditionASTNode::LoopConditionASTNode(std::unique_ptr<ASTNode> i,
                                           std::unique_ptr<ASTNode> c,
                                           std::unique_ptr<ASTNode> p)
    : initialize(std::move(i)), condition(std::move(c)), prefix(std::move(p)) {}

constexpr std::unique_ptr<ASTNode> LoopConditionASTNode::clone() const {
  return std::make_unique<LoopConditionASTNode>(
      initialize ? initialize->clone() : nullptr,
      condition ? condition->clone() : nullptr,
      prefix ? prefix->clone() : nullptr);
}

ForLoopASTNode::ForLoopASTNode(std::unique_ptr<LoopConditionASTNode> c,
                               std::unique_ptr<ASTNode> b)
    : condition(std::move(c)), block(std::move(b)) {}

constexpr std::unique_ptr<ASTNode> ForLoopASTNode::clone() const {
  std::unique_ptr<LoopConditionASTNode> cloned_cond = nullptr;
  if (condition) {
    cloned_cond.reset(
        dynamic_cast<LoopConditionASTNode *>(condition->clone().release()));
  }
  return std::make_unique<ForLoopASTNode>(std::move(cloned_cond),
                                          block ? block->clone() : nullptr);
}

// ─── Internal logging helpers
// ───────────────────────────────────────────────── Centralised here so every
// format string is in one place; a mismatch in arguments causes a compile-time
// error in exactly one spot rather than silently blowing up at runtime across
// dozens of call-sites.

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
    const size_t total_elements = parameters.size() * 2;
    size_t current_element = 0;

    for (size_t i = 0; i < parameters.size(); ++i) {
      // A. Print Target Parameter
      std::print("{}├── Target: ", prefix);
      parameters[i].debug_print(prefix + "│   ");
      current_element++;

      // B. Print Associated Value
      const bool val_is_last = (current_element == total_elements - 1);
      std::println("{}{}", prefix, val_is_last ? "└── Value:  " : "├── Value:  ");

      std::string val_prefix = prefix + (val_is_last ? "    " : "│   ");
      if (initializers[i]) {
        std::print("{}└── ", val_prefix);
        initializers[i]->debug_print(val_prefix + "    ");
      } else {
        std::println("[Null Initializer]");
      }
      current_element++;
    }
  }
  // Scenario 2: Tuple Unpacking or Cardinality Mismatch (e.g., let int: x, y :=
  // get_pair();)
  else {
    const size_t total_elements = parameters.size() + initializers.size();
    size_t current_element = 0;

    // Print all variable targets first
    for (const auto & parameter : parameters) {
      std::print("{}├── Target: ", prefix);
      parameter.debug_print(prefix + "│   ");
      current_element++;
    }

    // Print the remaining initializer expression block (e.g., the tuple object)
    for (const auto & initializer : initializers) {
      const bool is_last = (current_element == total_elements - 1);
      std::println("{}{}", prefix, is_last ? "└── Source: " : "├── Source: ");

      std::string next_prefix = prefix + (is_last ? "    " : "│   ");
      if (initializer) {
        std::print("{}└── ", next_prefix);
        initializer->debug_print(next_prefix + "    ");
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
  std::println("[Call]");
  std::println("{}└── Name: {}", prefix, call);

  std::string op_prefix = prefix + "    ";
  for (size_t i = 0; i < arguments.size(); ++i) {
    const bool last = (i == arguments.size() - 1);
    std::print("{}{}", op_prefix, last ? "└── " : "├── ");
    arguments[i]->debug_print(op_prefix + (last ? "    " : "│   "));
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
  std::println("[Range]");
  std::println("{}└── Operator: {}", prefix, op.value);

  std::string op_prefix = prefix + "    ";
  std::print("{}├── Start: ", op_prefix);
  start ? start->debug_print(op_prefix + "│   ") : std::println("(null)");

  std::print("{}└── End: ", op_prefix);
  end ? end->debug_print(op_prefix + "    ") : std::println("(null)");
}

void BinaryExprASTNode::debug_print(const std::string &prefix) const {
  std::println("[Binary]");
  std::println("{}└── Operator: {}", prefix, op.value);

  std::string op_prefix = prefix + "    ";

  std::print("{}├── RHS ", op_prefix);
  right ? right->debug_print(op_prefix + "│   ") : std::println("(null)");

  std::print("{}└── LHS ", op_prefix);
  left ? left->debug_print(op_prefix + "    ") : std::println("(null)");
}

void UnaryExprASTNode::debug_print(const std::string &prefix) const {
  std::string kind = is_postfix ? "Postfix" : "Prefix";
  std::println("[Unary]: {} '{}'", kind, op.value);
  std::print("{}└── Operand: ", prefix);
  (operand) ? operand->debug_print(prefix + "    ") : std::println("(null)");
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

  for (size_t i = 0; i < branches.size(); ++i) {
    const bool is_last_branch = (i == branches.size() - 1 && !else_branch);

    // 1. Print the Branch header
    std::println("{}{}Branch {}", prefix, is_last_branch ? "└── " : "├── ", i);

    // 2. The vertical line for the contents of THIS branch
    std::string branch_content_prefix = prefix + (is_last_branch ? "    " : "│   ");

    // --- INIT ---
    // Init has siblings (Cond, Body), so it carries a pipe "│   "
    std::println("{}├── Init:", branch_content_prefix);
    std::string init_prefix = branch_content_prefix + "│   ";
    if (branches[i].initializer) {
      std::print("{}└── ", init_prefix);
      branches[i].initializer->debug_print(init_prefix + "    ");
    } else {
      std::println("{}└── [No initialization]", init_prefix);
    }

    // --- COND ---
    // Cond has a sibling (Body), so it carries a pipe "│   "
    std::println("{}├── Cond:", branch_content_prefix);
    std::string cond_prefix = branch_content_prefix + "│   ";
    if (branches[i].condition) {
      std::print("{}└── ", cond_prefix);
      branches[i].condition->debug_print(cond_prefix + "    ");
    } else {
      std::println("{}└── [Invalid Condition]", cond_prefix);
    }

    // --- BODY ---
    // Body is the last child of the branch, so it carries spaces "    "
    std::println("{}└── Body:", branch_content_prefix);
    std::string body_prefix = branch_content_prefix + "    ";
    if (branches[i].body) {
      std::print("{}└── ", body_prefix);
      branches[i].body->debug_print(body_prefix + "    ");
    } else {
      std::println("{}└── [Invalid Body]", body_prefix);
    }
  }

  // ELSE
  if (else_branch) {
    std::println("{}└── Else:", prefix);
    std::string else_prefix = prefix + "    ";
    std::print("{}└── ", else_prefix);
    else_branch->debug_print(else_prefix + "    ");
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

  const bool has_range = (range != nullptr);
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
  std::println("[Function]");
  std::println("{}├── Name: {}", prefix, name);

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
  const bool has_body = (body != nullptr);
  const size_t total_ch = parameters.size() + (has_body ? 1 : 0);

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

void JoinASTNode::debug_print(const std::string &prefix) const {
  std::println("[Join]");

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

void NamespaceAliasASTNode::debug_print(const std::string& prefix) const {
  std::println("[Namespace-Alias]");
  std::println("{}├── Alias: {}", prefix, alias_name);
  std::println("{}└── Target: {}", prefix, target_name);
}

void BreakASTNode::debug_print([[maybe_unused]]const std::string& prefix) const {
  std::println("[Break]");
}

void ContinueASTNode::debug_print([[maybe_unused]]const std::string& prefix) const {
  std::println("[Continue]");
}

void LengthASTNode::debug_print(const std::string &prefix) const {
  std::println("[LengthASTNode]");
  std::print("{}└── Target: ", prefix);
  const std::string child_prefix = prefix + "  ";
  len_name->debug_print(child_prefix);
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
