#ifndef AST_HPP
#define AST_HPP

#include "Token.hpp"
#include <memory>
#include <string>
#include <vector>

#define MAKE_AST_NODE_UNCOPYABLE(ClassName)                                    \
  ClassName(const ClassName &) = delete;                                       \
  ClassName &operator=(const ClassName &) = delete;                            \
  ClassName(ClassName &&) = default;                                           \
  ClassName &operator=(ClassName &&) = default;

struct ASTNode {
  ASTNode() = default;
  virtual ~ASTNode();

  MAKE_AST_NODE_UNCOPYABLE(ASTNode)
  virtual void debug_print(const std::string &prefix = "") const = 0;
  [[nodiscard]] constexpr virtual std::unique_ptr<ASTNode> clone() const = 0;
};

struct alignas(8) TypeSpecifier {
  std::string base_types;
  std::unique_ptr<ASTNode> arr_size{};
  bool is_mutable{false};
  bool is_const{false};
  bool is_constexpr{false};
  bool is_array{false};

  std::byte _padding[4]{};
  [[nodiscard]] constexpr bool is_unknown() const {
    return base_types == "unknown" || base_types.empty();
  }

  [[nodiscard]] constexpr TypeSpecifier clone() const {
    return {base_types, arr_size ? arr_size->clone() : nullptr, is_const,
            is_array};
  }
};

struct LiteralASTNode : public ASTNode {
  std::string value;
  explicit LiteralASTNode(std::string_view val);

  MAKE_AST_NODE_UNCOPYABLE(LiteralASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct VariableExprASTNode : public ASTNode {
  std::string name;
  explicit VariableExprASTNode(std::string_view n);

  MAKE_AST_NODE_UNCOPYABLE(VariableExprASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct BinaryExprASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left;
  Token op;
  std::unique_ptr<ASTNode> right;

  explicit BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token const& o,
                    std::unique_ptr<ASTNode> r);
  MAKE_AST_NODE_UNCOPYABLE(BinaryExprASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct UnaryExprASTNode : public ASTNode {
  std::unique_ptr<ASTNode> operand;
  Token op;
  bool is_postfix;
  bool is_mut;

  explicit UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token const& o, bool postifx, bool mut);

  MAKE_AST_NODE_UNCOPYABLE(UnaryExprASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
  char pad[6]{};
};

struct MemberAccessASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left_side; // The object or namespace tree
  Token op;                           // Captured '.' or '::' token
  Token member;                       // The field or method name identifier

  explicit MemberAccessASTNode(std::unique_ptr<ASTNode> lhs, Token const& op_tok,
                      Token const& member_tok);
  MAKE_AST_NODE_UNCOPYABLE(MemberAccessASTNode)
  void debug_print(const std::string &prefix ) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct CallASTNode : public ASTNode {
  std::string call;
  std::vector<std::unique_ptr<ASTNode>> arguments;

  explicit CallASTNode(std::string_view name,
              std::vector<std::unique_ptr<ASTNode>> args);
  MAKE_AST_NODE_UNCOPYABLE(CallASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct IndexASTNode : public ASTNode {
  std::unique_ptr<ASTNode> arr_expr;
  Token op;
  std::unique_ptr<ASTNode> iexpr;

  explicit IndexASTNode(std::unique_ptr<ASTNode> arr, Token const& op_tok,
               std::unique_ptr<ASTNode> idx);
  MAKE_AST_NODE_UNCOPYABLE(IndexASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct ArrayLiteralASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> elements;
  explicit ArrayLiteralASTNode(std::vector<std::unique_ptr<ASTNode>> elements);
  MAKE_AST_NODE_UNCOPYABLE(ArrayLiteralASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct RangeLiteralASTNode : public ASTNode {
  std::unique_ptr<ASTNode> start; /* Could be a VariableASTNode or anything..
                                     Phew I am safe! */
  Token op;
  std::unique_ptr<ASTNode> end;
  MAKE_AST_NODE_UNCOPYABLE(RangeLiteralASTNode)

  explicit RangeLiteralASTNode(std::unique_ptr<ASTNode> start_node, Token const& op_tok,
                      std::unique_ptr<ASTNode> end_node);
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct ParameterASTNode : public ASTNode {
  std::unique_ptr<ASTNode> node;
  TypeSpecifier type;
  std::unique_ptr<ASTNode> defaultValue;
  // Optional: No range!
  std::unique_ptr<RangeLiteralASTNode> range;
  explicit ParameterASTNode(std::unique_ptr<ASTNode> n, TypeSpecifier t,
                   std::unique_ptr<RangeLiteralASTNode> r = nullptr,
                   std::unique_ptr<ASTNode> v = nullptr);

  MAKE_AST_NODE_UNCOPYABLE(ParameterASTNode)

  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct LoopConditionASTNode : public ASTNode {
  std::unique_ptr<ASTNode> initialize;
  std::unique_ptr<ASTNode> condition;
  std::unique_ptr<ASTNode> prefix;

  explicit LoopConditionASTNode(std::unique_ptr<ASTNode> i, std::unique_ptr<ASTNode> c,
                       std::unique_ptr<ASTNode> p);

  MAKE_AST_NODE_UNCOPYABLE(LoopConditionASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct ForLoopASTNode : public ASTNode {
  std::unique_ptr<LoopConditionASTNode> condition;
  std::unique_ptr<ASTNode> block;

  explicit ForLoopASTNode(std::unique_ptr<LoopConditionASTNode> c,
                 std::unique_ptr<ASTNode> b);
  MAKE_AST_NODE_UNCOPYABLE(ForLoopASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct VarDeclASTNode : public ASTNode {
  std::vector<ParameterASTNode> parameters;
  std::vector<std::unique_ptr<ASTNode>> initializers;

  explicit VarDeclASTNode(std::vector<ParameterASTNode> params,
                 std::vector<std::unique_ptr<ASTNode>> i);
  MAKE_AST_NODE_UNCOPYABLE(VarDeclASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct BlockASTNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements;

  explicit BlockASTNode(std::vector<std::unique_ptr<ASTNode>> stmts);
  MAKE_AST_NODE_UNCOPYABLE(BlockASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct FunctionASTNode : public ASTNode {
  std::string name;
  std::vector<ParameterASTNode> parameters;
  std::vector<TypeSpecifier> return_type;
  std::unique_ptr<BlockASTNode> body;

  explicit FunctionASTNode(std::string name, std::vector<ParameterASTNode> params,
                  std::vector<TypeSpecifier> ret,
                  std::unique_ptr<BlockASTNode> body);
  MAKE_AST_NODE_UNCOPYABLE(FunctionASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct ImportASTNode : public ASTNode {
  std::vector<std::string> mod_path;

  explicit ImportASTNode(std::vector<std::string> path);
  MAKE_AST_NODE_UNCOPYABLE(ImportASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct NamespaceASTNode : public ASTNode {
  std::string parent;
  std::unique_ptr<BlockASTNode> body;

  explicit NamespaceASTNode(std::string p, std::unique_ptr<BlockASTNode> b);
  MAKE_AST_NODE_UNCOPYABLE(NamespaceASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct NamespaceAliasASTNode : public ASTNode
{
  std::string alias_name;
  std::string target_name;

  explicit NamespaceAliasASTNode(std::string alias, std::string target);
  MAKE_AST_NODE_UNCOPYABLE(NamespaceAliasASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct JoinASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> expression;

  explicit JoinASTNode(std::vector<std::unique_ptr<ASTNode>> expr);
  MAKE_AST_NODE_UNCOPYABLE(JoinASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct AssignmentASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left_side;
  Token op;
  std::unique_ptr<ASTNode> new_value;

  explicit AssignmentASTNode(std::unique_ptr<ASTNode> lhs, const Token& op_tok,
                    std::unique_ptr<ASTNode> val);
  MAKE_AST_NODE_UNCOPYABLE(AssignmentASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct BreakASTNode : public ASTNode {
  explicit BreakASTNode();
  MAKE_AST_NODE_UNCOPYABLE(BreakASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct ContinueASTNode : public ASTNode {
  explicit ContinueASTNode();
  MAKE_AST_NODE_UNCOPYABLE(ContinueASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct LengthASTNode : public ASTNode {
  std::unique_ptr<ASTNode> len_name;
  explicit LengthASTNode(std::unique_ptr<ASTNode> len);

  MAKE_AST_NODE_UNCOPYABLE(LengthASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct ReturnASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> expression;

  explicit ReturnASTNode(std::vector<std::unique_ptr<ASTNode>> expr);
  MAKE_AST_NODE_UNCOPYABLE(ReturnASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

struct IfBranch  {
  std::unique_ptr<ASTNode> initializer;
  std::unique_ptr<ASTNode> condition;
  std::unique_ptr<ASTNode> body;

  IfBranch(std::unique_ptr<ASTNode> init, std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> b);
  [[nodiscard]] constexpr IfBranch clone() const;
};

struct IfASTNode : ASTNode {
  std::vector<IfBranch> branches;
  std::unique_ptr<ASTNode> else_branch;

  explicit IfASTNode(
      std::vector<IfBranch> brs,
      std::unique_ptr<ASTNode> el_br);

  MAKE_AST_NODE_UNCOPYABLE(IfASTNode)
  void debug_print(const std::string &prefix) const override;
  [[nodiscard]] constexpr std::unique_ptr<ASTNode> clone() const override;
};

#endif
