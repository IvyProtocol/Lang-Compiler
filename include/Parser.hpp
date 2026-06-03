#include "Token.hpp"
#include <format>
#include <memory>
#include <print>
#include <string>
#include <vector>

enum class Precedence : std::int64_t {
    NONE,
    ASSIGNMENT,    // :=, +=, -=, *=, /=
    LOGICAL_OR,    // ||
    LOGICAL_AND,   // &&
    EQUALITY,      // ==, !=
    COMPARISON,    // <, >, <=, >=
    BITSHIFT,      // <<, >>
    TERM,          // +, -
    FACTOR,        // *, /, %
    UNARY,         // !, -, ++prefix, --prefix
    CALL_INDEX,    // function(), array[index]
    MEMBER_ACCESS  // ., ::
};

struct ASTNode {
  virtual ~ASTNode();
  virtual void debug_print() const = 0;
};

struct LiteralASTNode : public ASTNode {
  std::string value;
  LiteralASTNode(std::string_view val);

  void debug_print() const override;
};

struct StubASTNode : public ASTNode {
  std::string name;
  explicit StubASTNode(std::string_view n);
  void debug_print() const override;
};


struct BinaryExprASTNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    Token op;
    std::unique_ptr<ASTNode> right;

    BinaryExprASTNode(std::unique_ptr<ASTNode> l, Token o, std::unique_ptr<ASTNode> r);
    void debug_print() const override;
};

struct UnaryExprASTNode : public ASTNode {
    std::unique_ptr<ASTNode> operand;
    Token op;
    UnaryExprASTNode(std::unique_ptr<ASTNode> opnd, Token o);
    void debug_print() const override;
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
  void debug_print() const override;
};

struct VariableExprASTNode : public ASTNode {
  std::string name;
  VariableExprASTNode(std::string_view n);
  void debug_print() const override;
};

struct AssignmentASTNode : public ASTNode {
  std::string variable_name;
  std::unique_ptr<ASTNode> new_value;

  AssignmentASTNode(std::string_view name, std::unique_ptr<ASTNode> val);
  void debug_print() const override;
};

struct CallASTNode : public ASTNode {
  std::string call;
  std::vector<std::unique_ptr<ASTNode>> arguments;

  CallASTNode(std::string_view name,
              std::vector<std::unique_ptr<ASTNode>> args);
  void debug_print() const override;
};

struct ArrayLiteralASTNode : public ASTNode {
  std::vector<std::unique_ptr<ASTNode>> elements;
  ArrayLiteralASTNode(std::vector<std::unique_ptr<ASTNode>> elements);
  void debug_print() const override;
};


struct Parser;

using NudFunc = std::unique_ptr<ASTNode> (Parser::*)();
using LedFunc = std::unique_ptr<ASTNode> (Parser::*)(std::unique_ptr<ASTNode> left);

struct ParserRule {
  Precedence precedence;
  NudFunc nud;
  LedFunc led;
};


struct Parser {
  std::vector<Token> tokens;
  size_t current{};

  Parser(std::vector<Token> tokens_list) : tokens(std::move(tokens_list)) {}

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

  TypeSpecifier parse_type() {
    TypeSpecifier type_info;

    if (check(TokenType::CONST)) {
      Token t = advance();
      type_info.is_const = true;
      std::println("[MOV]: Tracking 'CONST'. Line: {}, Column: {}", t.line,
      t.column);
    }

    if (check(TokenType::IDENTIFIER)) {
      Token t = advance();
      type_info.base_types = std::string(t.value);
      std::println("[MOV]: Tracking '{}', Line: {}, Column: {}", t.value,
                   t.line, t.column);
    } else if (check(TokenType::AUTO) || check(TokenType::VOID)) {
      Token t = advance();
      type_info.base_types = std::string(t.value);
      std::println("[MOV]: Tracking '{}', Line: {}, Column: {}", t.value,
                   t.line, t.column);
    } else {
      std::println("[ERR]: Expected a type but got '{}' at Line {}, Column {}",
                   peek().value, peek().line, peek().column);
      type_info.base_types = "unknown";
    }
    return type_info;
  }

  std::vector<std::unique_ptr<ASTNode>> parse_program() {
    std::vector<std::unique_ptr<ASTNode>> program_nodes;

    while (!is_at_end()) {
      auto node = parse_statement();

      if (node) {
        program_nodes.emplace_back(std::move(node));
      } else {
        advance();
      }
    }
    return program_nodes;
  }

  std::unique_ptr<ASTNode> parse_variable_declaration() {
    advance(); // Eat 'let'

    TypeSpecifier var_type = parse_type();
    if (check(TokenType::LBRACKET)) {
      advance();
      var_type.is_array = true; // Marking it as an array!

      if (check(TokenType::INT_LITERAL)) {
        Token size_token = advance();
        var_type.arr_size = std::stoi(std::string(size_token.value));
        std::println("[MOV]: Array-Declaration Size {} at Line {}, Column {}",
                     size_token.value, size_token.line, size_token.column);
      } else {
        var_type.arr_size = -1;
        std::println(
            "[MOV]: Unsized Array-Declaration processed at Line {}, Column {}",
            peek().line, peek().column);
      }

      if (check(TokenType::RBRACKET)) {
        Token r_bracket = advance();
        std::println(
            "[MOV]: Array-Declaration ended with ']' at Line {}, Column {}",
            r_bracket.line, r_bracket.line, r_bracket.value);
      } else {
        std::println("[ERR]: Expected ']' after Array-Declaration '[' at Line "
                     "{}, Column {}",
                     peek().line, peek().column);
      }
      std::println("[MOV]: Array-Variable has been initialized with an Type.");
    }
    if (check(TokenType::COLON)) {
      advance();
    } else {
      std::println(
          "[ERR]: Expected ':' after type-specifier at Line {}, Column {}",
          peek().line, peek().column);
    }

    if (!check(TokenType::IDENTIFIER)) {
      std::println(
          "[ERR]: Expected variable name after ':' at Line {}, Column {}",
          peek().line, peek().column);
      return nullptr;
    }
    Token token_name = advance();

    std::unique_ptr<ASTNode> initializer_expr = nullptr;
    if (check(TokenType::ASSIGN)) {
      Token assign_tok = advance();
      std::println("[MOV]: Tracking previous definer to assingment ':=' at "
                   "Line {}, Column {}",
                   assign_tok.line, assign_tok.column);
      Token value_token = peek();
      initializer_expr = parse_primary();
      std::println("[MOV]: Assigner found body {} at Line {}, Column {}",
                   value_token.value, value_token.line, value_token.column);
    }
    expect_semicolon();

    return std::make_unique<VarDeclASTNode>(token_name.value, var_type,
                                            std::move(initializer_expr));
  }

  std::unique_ptr<ASTNode> parse_array_literal() {

    std::vector<std::unique_ptr<ASTNode>> elements;

    if (!check(TokenType::END_OF_FILE) && !check(TokenType::RBRACKET)) {
      do {
        std::unique_ptr<ASTNode> arrays = parse_primary();
        elements.emplace_back(std::move(arrays));

        if (check(TokenType::COMMA))
          advance();
        else
          break;

      } while (!check(TokenType::END_OF_FILE) && !check(TokenType::RBRACKET));
    }

    if (check(TokenType::RBRACKET))
      advance();
    else
      std::println(
          "[ERR]: Expected closing ']' for array literal at Line {}, Column {}",
          peek().line, peek().column);

    return std::make_unique<ArrayLiteralASTNode>(std::move(elements));
  }

  std::unique_ptr<ASTNode> parse_expression(Precedence min_precedence) {
    Token token = advance();
    NudFunc nud_fn = get_rule(token.type).nud;

    if ( ! nud_fn ) {
      return std::make_unique<StubASTNode>(
        std::format("[ERR]: Parse-Error, expected start of a expression, got {} at Line {}, Column {}",
             token.value, token.line, token.column)
      );
    }

    std::unique_ptr<ASTNode> left = (this->*nud_fn) ();
    while ( ! is_at_end() && min_precedence < get_rule(peek().type).precedence) {
      Token next_token = advance();
      LedFunc led_fn = get_rule(next_token.type).led;

      if (led_fn) left = (this->*led_fn)(std::move(left));
    }
    return left;
  }

  ParserRule get_rule(TokenType type) {
    switch ( type ) {
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::RAW_STRING_LITERAL:
        case TokenType::BACK_TICK_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE: return {
            Precedence::NONE, &Parser::parse_literal, nullptr
        };
        case TokenType::IDENTIFIER: return {
            Precedence::NONE, &Parser::parse_identifier, nullptr
        };
        case TokenType::BANG:
        case TokenType::PLUS_PLUS:
        case TokenType::MINUS_MINUS: return {
            Precedence::NONE, &Parser::parse_prefix, nullptr
        };

        case TokenType::ASSIGN:
        case TokenType::PLUS_EQUAL:
        case TokenType::MINUS_EQUAL:
        case TokenType::MUL_EQUAL:
        case TokenType::DIV_EQUAL: return {
            Precedence::ASSIGNMENT, nullptr, &Parser::parse_binary
        };
        case TokenType::OR: return {
            Precedence::LOGICAL_OR, nullptr, &Parser::parse_binary
        };
        case TokenType::AND: return {
            Precedence::LOGICAL_AND, nullptr, &Parser::parse_binary
        };
        case TokenType::EQ:
        case TokenType::NEQ: return {
            Precedence::EQUALITY, nullptr, &Parser::parse_binary
        };
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LTE:
        case TokenType::GTE: return {
            Precedence::COMPARISON, nullptr, &Parser::parse_binary
        };
        case TokenType::L_S_O:
        case TokenType::R_S_O: return {
            Precedence::BITSHIFT, nullptr, &Parser::parse_binary
        };

        case TokenType::PLUS: return {
            Precedence::TERM, nullptr, &Parser::parse_binary
        };

        case TokenType::MINUS: return {
            Precedence::TERM, &Parser::parse_prefix, &Parser::parse_binary
        };

        case TokenType::MUL:
        case TokenType::DIV:
        case TokenType::MOD: return {
            Precedence::FACTOR, nullptr, &Parser::parse_binary
        };

        case TokenType::DOT:
        case TokenType::DOUBLE_COLON: return {
            Precedence::MEMBER_ACCESS, nullptr, &Parser::parse_memeber_access
        };
        case TokenType::L_PAREN: return {
            Precedence::CALL_INDEX, &Parser::parse_grouping, &Parser::parse_call
        };
        case TokenType::LBRACKET: return {
            Precedence::CALL_INDEX, &Parser::parse_array_literal, &Parser::parse_index
        };
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
        case TokenType::END_OF_FILE: return {
            Precedence::NONE, nullptr, nullptr
        };
        default: return {
            Precedence::NONE, nullptr, nullptr
        };
    }
  }

  void expect_semicolon() {
    if ( !check(TokenType::SEMI_COLON) ) {
      std::println("[ERR]: Expected ';' at Line {}, Column {}",
        peek().line, peek().column);
    } else {
        advance();
        std::println("[MOV]: Tracking ';' at Line {}, Column {}",
        peek().line, peek().column);
    }
  }
  std::unique_ptr<ASTNode> parse_literal() {
    return std::make_unique<LiteralASTNode>(tokens[current - 1].value);
  }
  std::unique_ptr<ASTNode> parse_identifier() {
    return std::make_unique<VariableExprASTNode>(tokens[current - 1].value);
  }
  std::unique_ptr<ASTNode> parse_prefix() {
    Token operator_token = tokens[current - 1];
    auto operand = parse_expression(Precedence::UNARY);
    return std::make_unique<UnaryExprASTNode>(std::move(operand), operator_token);
  }

  std::unique_ptr<ASTNode> parse_binary(std::unique_ptr<ASTNode> left) {
    Token operator_token = tokens[current - 1];
    Precedence precedence = get_rule(operator_token.type).precedence;
    auto right = parse_expression(precedence);
    return std::make_unique<BinaryExprASTNode>(std::move(left), operator_token, std::move(right));
  }

  std::unique_ptr<ASTNode> parse_memeber_access(std::unique_ptr<ASTNode> /* left */ ) {
    Token member = advance();
    return std::make_unique<StubASTNode>(std::format("Member Access: {} on left node", member.value));
  }

  std::unique_ptr<ASTNode> parse_grouping() {
    auto expr = parse_expression(Precedence::NONE);
    if (check(TokenType::R_PAREN)) advance();
    return expr;
  }

  std::unique_ptr<ASTNode> parse_call(std::unique_ptr<ASTNode> left ) {
    std::vector<std::unique_ptr<ASTNode>> args;
    if (!check(TokenType::END_OF_FILE) && !check(TokenType::R_PAREN)) {
      do {
        args.emplace_back(parse_expression(Precedence::NONE));
        if (check(TokenType::COMMA)) advance();
        else break;
      } while (!check(TokenType::END_OF_FILE) && !check(TokenType::R_PAREN));
    }

    if (check(TokenType::R_PAREN)) {
        Token close_paren = advance();
        std::println("[MOV]: Tracked function call closure ')' at Line {}, Column {}",
            close_paren.line, close_paren.column);
    } else {
        std::println("[ERR]: Expected ')' after function arguments at Line {}, Column {}",
            peek().line, peek().column);
    }
    std::string func_name = "ambiguous";
    if (auto* var_node = dynamic_cast<VariableExprASTNode*>(left.get())) {
        func_name = var_node->name;
    }

    return std::make_unique<CallASTNode>(func_name, std::move(args));
  }

  std::unique_ptr<ASTNode> parse_index(std::unique_ptr<ASTNode> left ) {
    Token open_bracket = tokens[current - 1];
    auto index_expr = parse_expression(Precedence::NONE);
    if (check(TokenType::RBRACKET)) {
        Token close_bracket = advance(); // consume ']'
        std::println("[MOV]: Tracked array subscription closure ']' at Line {}, Column {}",
            close_bracket.line, close_bracket.column);
    } else {
        std::println("[ERR]: Expected ']' after array index at Line {}, Column {}",
            peek().line, peek().column);
    }

    return std::make_unique<BinaryExprASTNode>(std::move(left), open_bracket, std::move(index_expr));
  }


  std::unique_ptr<ASTNode> parse_statement() {
    if (check(TokenType::HASH) && peek_next().type == TokenType::IMPORT) {
      Token start_token = peek();

      advance();
      advance();
      if (check(TokenType::LT)) {
        std::println("[MOV]: Advanced to '<'. Line: {}, Column: {}",
                     peek().line, peek().column);
        advance();
        while (!is_at_end() && !check(TokenType::GT))
          advance();
        if (check(TokenType::GT)) {
          std::println("[MOV]: Advanced to '>'. Line: {}, Column: {}",
                       peek().line, peek().column);
          advance();
        } else {
          std::println("[ERR]: Could not advance to '>', is it missing? Line: "
                       "{}, Column: {}",
                       peek().line, peek().column);
        }
      } else {
        std::println("[ERR]: Could not advance to '<', is it missing? Line: "
                     "{}, Column: {}",
                     peek().line, peek().column);
      }
      return std::make_unique<StubASTNode>(
          std::format("Import Statement: Line {}, Column {}", start_token.line,
                      start_token.column));
    }

    if (check(TokenType::LET)) {
      auto node = parse_variable_declaration();
      expect_semicolon();
      return node;
    }

    if (check(TokenType::IDENTIFIER) && peek_next().type == TokenType::ASSIGN) {
      Token target_var = advance();
      advance();

      auto value = parse_expression(Precedence::NONE);
      expect_semicolon();
      return std::make_unique<AssignmentASTNode>(target_var.value,
                                                 std::move(value));
    }

    auto expr = parse_primary();
    expect_semicolon();
    return expr;
  }
  std::unique_ptr<ASTNode> parse_primary() {
    return parse_expression(Precedence::NONE);
  }
};
