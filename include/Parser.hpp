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
  MissingComma,
  MalformedParameters,
  MalformedRangeLiteral,
  MalformedMemberAccess,
  MalformedLoopCondition,
  ExpectedBraceOrColon,
  InvalidImportPath,
  InvalidExpression,
  InvalidDeclarationTarget,
  InvalidCallTarget,
  InvalidType,
  InvalidLoop,
  InvalidStatement,
  AssignmentCountMismatch,
  InvalidAssignmentTarget,
  InvalidArrayMalformed,
  InvalidMemberAccessOperand,
  ExpectedIdentifier,
  InternalError,
  UnexpectedEndOfFile
};

using ast_node = ASTNode;
using NudFunc =
    std::expected<std::unique_ptr<ast_node>, ParseError> (Parser::*)();
using LedFunc = std::expected<std::unique_ptr<ast_node>, ParseError> (Parser::*)(
    std::unique_ptr<ast_node> left);

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
    bool allow_void_assignment = false;
    char pad[6]{};
  };

  struct LoopOptsASTNode {
    bool allow_C_style = false;
    bool allow_Loop_Range_style = false;
    bool allow_body = false;
  };

  struct alignas(8) ParsedIfHeader {
    std::unique_ptr<ast_node> initializer{nullptr};
    std::unique_ptr<ast_node> condition{nullptr};
    bool has_paren{false};
    bool enforce_brace{false};

    std::byte _padding[6]{};
  };

  std::vector<Token> tokens;
  size_t current{};

  std::string filename;
  std::vector<std::string> source_lines;

  Parser(std::vector<Token> tokens_list, std::string_view file_path,
         std::vector<std::string> lines);

  [[nodiscard]]bool is_at_end() const {
    return current >= tokens.size() ||
           tokens[current].type == TokenType::END_OF_FILE;
  }

  [[nodiscard]]const Token &peek() const;
  [[nodiscard]]bool check(TokenType type) const {
    if (is_at_end())
      return false;
    return peek().type == type;
  }

  Token advance() {
    if (!is_at_end())
      current++;
    return tokens[current - 1];
  }

  [[nodiscard]]Token peek_next() const {
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

  bool parse_loopParam_list(const LoopOptsASTNode &cond,
                            std::vector<LoopConditionASTNode> &c_block);

  bool parser_for_variable_group(std::vector<ParameterASTNode> &params);
  bool parser_function_parameter_group(std::vector<ParameterASTNode> &params);
  bool parser_variable_parameter_group(std::vector<ParameterASTNode> &params);
  bool parser_for_condition_group(std::vector<LoopConditionASTNode> &params);
  constexpr static bool is_statement_start(TokenType type);
  constexpr static ParserRule get_rule(TokenType type);
  void report_error(const Token &tok, std::string_view msg);
  void synchronize();

  std::expected<TypeSpecifier, ParseError> parse_type();

  std::vector<std::unique_ptr<ast_node>> parse_program();

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_variable_declaration(bool let_allowed, bool semi_allowed);

  constexpr std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_assignment(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_array_literal();

  constexpr std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_range_infix(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_expression(Precedence min_precedence);

  constexpr std::expected<std::unique_ptr<ast_node>, ParseError> parse_literal();

  constexpr std::expected<std::unique_ptr<ast_node>, ParseError> parse_identifier();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_prefix();

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_postfix(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_binary(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_member_access(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_grouping();

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_call(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_index(std::unique_ptr<ast_node> left);

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_block();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_paren();
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_paren_expression();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_import_statement();

  std::expected<ParsedIfHeader, ParseError> parse_if_header();
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_if_body(const ParsedIfHeader& header);
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_if_statement();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_for_statement();
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_break_statement();
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_continue_statement();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_length_statement();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_return_statement();
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_join();

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_function_statement();

  std::expected<std::unique_ptr<ast_node>, ParseError>
  parse_namespace_statement();

  std::expected<std::unique_ptr<ast_node>, ParseError> parse_statement();
  std::expected<std::unique_ptr<ast_node>, ParseError> parse_primary();
};

#endif
