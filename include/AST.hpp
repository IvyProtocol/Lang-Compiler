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
};

struct LiteralASTNode : public ASTNode {
  std::string value;
  LiteralASTNode(std::string_view val);

  MAKE_AST_NODE_UNCOPYABLE(LiteralASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct BinaryExprASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left;
  Token op;
  std::unique_ptr<ASTNode> right;

  BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token o,
                    std::unique_ptr<ASTNode> r);
  MAKE_AST_NODE_UNCOPYABLE(BinaryExprASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct UnaryExprASTNode : public ASTNode {
  std::unique_ptr<ASTNode> operand;
  Token op;
  bool is_postfix;
  UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token o, bool postifx);
  MAKE_AST_NODE_UNCOPYABLE(UnaryExprASTNode)
  void debug_print(const std::string &prefix = "") const override;
  char pad[7]{};
};

struct TypeSpecifier {
  std::string base_types;
  int arr_size{};
  bool is_const{false};
  bool is_array{false};

  bool is_unknown() const;
  long : (8 * 2);
};

struct RangeLiteralASTNode : public ASTNode {
  std::unique_ptr<ASTNode> start; /* Could be a VariableASTNode or anything..
                                     Phew I am safe! */
  Token op;
  std::unique_ptr<ASTNode> end;
  MAKE_AST_NODE_UNCOPYABLE(RangeLiteralASTNode)

  RangeLiteralASTNode(std::unique_ptr<ASTNode> start_node, Token op_tok,
                      std::unique_ptr<ASTNode> end_node);
  void debug_print(const std::string &prefix = "") const override;
};

struct ParameterASTNode : public ASTNode {
  std::unique_ptr<ASTNode> node;
  TypeSpecifier type;
  std::unique_ptr<ASTNode> defaultValue;
  // Optional: No range!
  std::unique_ptr<RangeLiteralASTNode> range;
  ParameterASTNode(std::unique_ptr<ASTNode> n, TypeSpecifier t,
                   std::unique_ptr<RangeLiteralASTNode> r = nullptr,
                   std::unique_ptr<ASTNode> v = nullptr);

  MAKE_AST_NODE_UNCOPYABLE(ParameterASTNode)

  void debug_print(const std::string &prefix = "") const override;
};

struct ImportASTNode : public ASTNode {
  std::vector<std::string> mod_path;

  ImportASTNode(std::vector<std::string> path);
  MAKE_AST_NODE_UNCOPYABLE(ImportASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct VarDeclASTNode : public ASTNode {
  std::vector<ParameterASTNode> parameters;
  std::vector<std::unique_ptr<ASTNode>> initializers;

  VarDeclASTNode(std::vector<ParameterASTNode> params,
                 std::vector<std::unique_ptr<ASTNode>> i);
  MAKE_AST_NODE_UNCOPYABLE(VarDeclASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct VariableExprASTNode : public ASTNode {
  std::string name;
  VariableExprASTNode(std::string_view n);
  MAKE_AST_NODE_UNCOPYABLE(VariableExprASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct AssignmentASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left_side;
  Token op;
  std::unique_ptr<ASTNode> new_value;

  AssignmentASTNode(std::unique_ptr<ASTNode> lhs, Token op_tok,
                    std::unique_ptr<ASTNode> val);
  MAKE_AST_NODE_UNCOPYABLE(AssignmentASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct CallASTNode : public ASTNode {
  std::string call;
  std::vector<std::unique_ptr<ASTNode>> arguments;

  CallASTNode(std::string_view name,
              std::vector<std::unique_ptr<ASTNode>> args);
  MAKE_AST_NODE_UNCOPYABLE(CallASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct IndexASTNode : public ASTNode {
  std::unique_ptr<ASTNode> arr_expr;
  Token op;
  std::unique_ptr<ASTNode> iexpr;

  IndexASTNode(std::unique_ptr<ASTNode> arr, Token op_tok,
               std::unique_ptr<ASTNode> idx);
  MAKE_AST_NODE_UNCOPYABLE(IndexASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct ArrayLiteralASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> elements;
  ArrayLiteralASTNode(std::vector<std::unique_ptr<ASTNode>> elements);
  MAKE_AST_NODE_UNCOPYABLE(ArrayLiteralASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct BlockASTNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements;

  BlockASTNode(std::vector<std::unique_ptr<ASTNode>> stmts);
  MAKE_AST_NODE_UNCOPYABLE(BlockASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct IfASTNode : ASTNode {
  std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
      branches;
  std::unique_ptr<ASTNode> else_branch;

  IfASTNode(
      std::vector<std::pair<std::unique_ptr<ASTNode>, std::unique_ptr<ASTNode>>>
          brs,
      std::unique_ptr<ASTNode> el_br);
  MAKE_AST_NODE_UNCOPYABLE(IfASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct FunctionASTNode : public ASTNode {
  std::string name;
  std::vector<ParameterASTNode> parameters;
  TypeSpecifier return_type;
  std::unique_ptr<BlockASTNode> body;

  FunctionASTNode(std::string name, std::vector<ParameterASTNode> params,
                  TypeSpecifier ret, std::unique_ptr<BlockASTNode> body);
  MAKE_AST_NODE_UNCOPYABLE(FunctionASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct MemberAccessASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left_side; // The object or namespace tree
  Token op;                           // Captured '.' or '::' token
  Token member;                       // The field or method name identifier

  MemberAccessASTNode(std::unique_ptr<ASTNode> lhs, Token op_tok,
                      Token member_tok);
  MAKE_AST_NODE_UNCOPYABLE(MemberAccessASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

struct ReturnASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> expression;

  ReturnASTNode(std::vector<std::unique_ptr<ASTNode>> expr);
  MAKE_AST_NODE_UNCOPYABLE(ReturnASTNode)
  void debug_print(const std::string &prefix = "") const override;
};

#endif
