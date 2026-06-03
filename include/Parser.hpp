#include "Token.hpp"
#include <memory>
#include <string>
#include <vector>

enum class Precedence : std::int64_t {
  NONE,
  ASSIGNMENT,   // :=, +=, -=, *=, /=
  LOGICAL_OR,   // ||
  LOGICAL_AND,  // &&
  EQUALITY,     // ==, !=
  COMPARISON,   // <, >, <=, >=
  BITSHIFT,     // <<, >>
  TERM,         // +, -
  FACTOR,       // *, /, %
  UNARY,        // !, -, ++prefix, --prefix
  CALL_INDEX,   // function(), array[index]
  MEMBER_ACCESS // ., ::
};

struct ASTNode {
  virtual ~ASTNode();
  virtual void debug_print(const std::string &prefix = "") const = 0;
};

struct LiteralASTNode : public ASTNode {
  std::string value;
  LiteralASTNode(std::string_view val);

  void debug_print(const std::string &prefix = "") const override;
};

struct StubASTNode : public ASTNode {
  std::string name;
  explicit StubASTNode(std::string_view n);
  void debug_print(const std::string &prefix = "") const override;
};

struct BinaryExprASTNode : public ASTNode {
  std::unique_ptr<ASTNode> left;
  Token op;
  std::unique_ptr<ASTNode> right;

  BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token o,
                    std::unique_ptr<ASTNode> r);
  void debug_print(const std::string &prefix = "") const override;
};

struct UnaryExprASTNode : public ASTNode {
  std::unique_ptr<ASTNode> operand;
  Token op;
  UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token o);
  void debug_print(const std::string &prefix = "") const override;
};

struct TypeSpecifier {
  std::string base_types;
  int arr_size{};
  bool is_const{false};
  bool is_array{false};
  long : (8 * 2);
};

struct VarDeclASTNode : public ASTNode {
  std::string identifier;
  TypeSpecifier type;
  std::unique_ptr<ASTNode> initializer;

  VarDeclASTNode(std::string_view id, TypeSpecifier t,
                 std::unique_ptr<ASTNode> init);
  void debug_print(const std::string &prefix = "") const override;
};

struct VariableExprASTNode : public ASTNode {
  std::string name;
  VariableExprASTNode(std::string_view n);
  void debug_print(const std::string &prefix = "") const override;
};

struct AssignmentASTNode : public ASTNode {
  std::string variable_name;
  std::unique_ptr<ASTNode> new_value;

  AssignmentASTNode(std::string_view name, std::unique_ptr<ASTNode> val);
  void debug_print(const std::string &prefix = "") const override;
};

struct CallASTNode : public ASTNode {
  std::string call;
  std::vector<std::unique_ptr<ASTNode>> arguments;

  CallASTNode(std::string_view name,
              std::vector<std::unique_ptr<ASTNode>> args);
  void debug_print(const std::string &prefix = "") const override;
};

struct ArrayLiteralASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> elements;
  ArrayLiteralASTNode(std::vector<std::unique_ptr<ASTNode>> elements);
  void debug_print(const std::string &prefix = "") const override;
};

struct BlockASTNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements;

  BlockASTNode(std::vector<std::unique_ptr<ASTNode>> stmts);
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
  void debug_print(const std::string &prefix = "") const override;
};

struct Parser;

using NudFunc = std::unique_ptr<ASTNode> (Parser::*)();
using LedFunc =
    std::unique_ptr<ASTNode> (Parser::*)(std::unique_ptr<ASTNode> left);

struct ParserRule {
  Precedence precedence;
  NudFunc nud;
  LedFunc led;
};

struct Parser {
  std::vector<Token> tokens;
  size_t current{};

  Parser(std::vector<Token> tokens_list);

  bool is_at_end() const {
    return current >= tokens.size() ||
           tokens[current].type == TokenType::END_OF_FILE;
  }

  Token peek() const {
    if (is_at_end())
      return Token{TokenType::END_OF_FILE, "", 0, 0};
    return tokens[current];
  }

  bool check(TokenType type) const {
    if (is_at_end())
      return false;
    return peek().type == type;
  }

  Token advance() {
    if (!is_at_end())
      current++;
    return tokens[current - 1];
  }

  Token peek_next() const {
    if (current + 1 >= tokens.size())
      return Token{TokenType::END_OF_FILE, "", 0, 0};
    return tokens[current + 1];
  }

  TypeSpecifier parse_type();
  std::vector<std::unique_ptr<ASTNode>> parse_program();
  std::unique_ptr<ASTNode> parse_variable_declaration();
  std::unique_ptr<ASTNode> parse_array_literal();
  std::unique_ptr<ASTNode> parse_expression(Precedence min_precedence);
  ParserRule get_rule(TokenType type);
  void expect_semicolon();
  void synchronize();

  std::unique_ptr<ASTNode> parse_literal();
  std::unique_ptr<ASTNode> parse_identifier();
  std::unique_ptr<ASTNode> parse_prefix();
  std::unique_ptr<ASTNode> parse_binary(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_memeber_access(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_grouping();
  std::unique_ptr<ASTNode> parse_call(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_index(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_block();
  std::unique_ptr<ASTNode> parse_paren();
  std::unique_ptr<ASTNode> parse_paren_expression();
  std::unique_ptr<ASTNode> parse_if_statement();
  std::unique_ptr<ASTNode> parse_statement();
  std::unique_ptr<ASTNode> parse_primary();
};
