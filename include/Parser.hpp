#ifndef PARSER_HPP
#define PARSER_HPP

#include "AST.hpp"
#include <expected>
#include <vector>

struct Parser;

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

enum class ParseError {
  UnexpectedToken,
  MissingSemiColon,
  MissingOpeningParen,
  MissingOpeningBrace,
  MissingClosingParen,
  MissingClosingBracket,
  MissingClosingBrace,
  MissingArrow,
  MissingColon,
  MalformedParameters,
  MalformedRangeLiteral,
  MalformedMemberAccess,
  InvalidImportPath,
  InvalidExpression,
  InvalidDeclarationTarget,
  InvalidCallTarget,
  InvalidType,
  AssignmentCountMismatch,
  InvalidAssignmentTarget,
  InvalidArrayMalformed,
  InvalidMemberAccessOperand,
  ExpectedIdentifier,
  InternalError,
  UnexpectedEndOfFile
};

using NudFunc =
    std::expected<std::unique_ptr<ASTNode>, ParseError> (Parser::*)();
using LedFunc = std::expected<std::unique_ptr<ASTNode>, ParseError> (Parser::*)(
    std::unique_ptr<ASTNode> left);

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

  bool is_at_end() const {
    return current >= tokens.size() ||
           tokens[current].type == TokenType::END_OF_FILE;
  }

  const Token &peek() const;
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

  std::expected<Token, ParseError> consume_token(TokenType type,
                                                 std::string_view msg);
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
  std::vector<std::unique_ptr<ASTNode>> parse_program();
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_variable_declaration();
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_assignment(std::unique_ptr<ASTNode> left);
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_array_literal();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_range_literal();
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_expression(Precedence min_precedence);

  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_literal();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_identifier();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_prefix();
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_postfix(std::unique_ptr<ASTNode> left);
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_binary(std::unique_ptr<ASTNode> left);
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_member_access(std::unique_ptr<ASTNode> left);
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_grouping();
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_call(std::unique_ptr<ASTNode> left);
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_index(std::unique_ptr<ASTNode> left);
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_block();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_paren();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_import_statement();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_paren_expression();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_if_body();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_if_statement();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_return_statement();
  std::expected<std::unique_ptr<ASTNode>, ParseError>
  parse_function_statement();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_statement();
  std::expected<std::unique_ptr<ASTNode>, ParseError> parse_primary();
};

#endif
