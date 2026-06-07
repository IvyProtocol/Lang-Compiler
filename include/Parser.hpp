#ifndef PARSER_HPP
#define PARSER_HPP

#include "AST.hpp"
#include <optional>
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

struct ParserRule {
  Precedence precedence;
  NudFunc nud;
  LedFunc led;
};

struct Parser {
  struct ParameterOptsASTNode {
    bool allow_assignment = false;
    bool allow_ranges = false;
    bool fatal_on_assign = false;

    char pad[6]{};
  };

  std::vector<Token> tokens;
  size_t current{};

  std::string filename;
  std::vector<std::string> source_lines;

  Parser(std::vector<Token> tokens_list, std::string_view file_path,
         std::vector<std::string> lines);
  std::vector<std::unique_ptr<ASTNode>> parse_program();

  bool is_at_end() const {
    return current >= tokens.size() ||
           tokens[current].type == TokenType::END_OF_FILE;
  }

  const Token& peek() const;
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

  std::optional<Token> consume_token(TokenType type, std::string_view msg);
  bool consume(TokenType type, std::string_view msg);
  bool parse_parameter_list(std::vector<ParameterASTNode> &params,
                            const ParameterOptsASTNode &opts,
                            TokenType term_type);
  bool parser_function_parameter_group(std::vector<ParameterASTNode> &params);
  bool parser_variable_parameter_group(std::vector<ParameterASTNode> &params);
  bool is_statement_start(TokenType type);
  ParserRule get_rule(TokenType type);
  void report_error(const Token &tok, std::string_view msg);
  void synchronize();

  TypeSpecifier parse_type();
  std::unique_ptr<ASTNode> parse_variable_declaration();
  std::unique_ptr<ASTNode> parse_assignment(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_array_literal();
  std::unique_ptr<ASTNode> parse_range_literal();
  std::unique_ptr<ASTNode> parse_expression(Precedence min_precedence);

  std::unique_ptr<ASTNode> parse_literal();
  std::unique_ptr<ASTNode> parse_identifier();
  std::unique_ptr<ASTNode> parse_prefix();
  std::unique_ptr<ASTNode> parse_postfix(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_binary(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_member_access(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_grouping();
  std::unique_ptr<ASTNode> parse_call(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_index(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parse_block();
  std::unique_ptr<ASTNode> parse_paren();
  std::unique_ptr<ASTNode> parse_import_statement();
  std::unique_ptr<ASTNode> parse_paren_expression();
  std::unique_ptr<ASTNode> parse_if_body();
  std::unique_ptr<ASTNode> parse_if_statement();
  std::unique_ptr<ASTNode> parse_function_statement();
  std::unique_ptr<ASTNode> parse_statement();
  std::unique_ptr<ASTNode> parse_primary();
};

#endif
