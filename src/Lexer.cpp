#include "../include/Lexer.hpp"
#include "../include/Token.hpp"
#include <cctype>
#include <cstdlib>
#include <print>
#include <vector>

using u_char = unsigned char;

static bool peek_match(const std::string_view &text, size_t current_index,
                       char expected) {
  if (current_index + 1 >= text.length())
    return false;
  return text.at(current_index + 1) == expected;
}

TokenType identifier_or_keyword(const std::string_view &word) {
  if (word == "import")
    return TokenType::IMPORT;
  if (word == "function")
    return TokenType::FUNCTION;
  if (word == "if")
    return TokenType::IF;
  if (word == "elseif")
    return TokenType::ELSEIF;
  if (word == "else")
    return TokenType::ELSE;
  if (word == "for")
    return TokenType::FOR;
  if (word == "while")
    return TokenType::WHILE;
  if (word == "let")
    return TokenType::LET;
  if (word == "return")
    return TokenType::RETURN;
  if (word == "break")
    return TokenType::BREAK;
  if (word == "continue")
    return TokenType::CONTINUE;
  if (word == "true")
    return TokenType::TRUE;
  if (word == "false")
    return TokenType::FALSE;
  if (word == "main")
    return TokenType::MAIN;
  if (word == "switch")
    return TokenType::SWITCH;
  if (word == "unwrap")
    return TokenType::UNWRAP;
  if (word == "exit")
    return TokenType::EXIT;
  if (word == "len")
    return TokenType::LENGTH;
  if (word == "join")
    return TokenType::JOIN;
  if (word == "eval")
    return TokenType::EVAL;
  if (word == "namespace")
    return TokenType::NAMESPACE;
  if (word == "auto")
    return TokenType::AUTO;
  if (word == "const")
    return TokenType::CONST;
  if (word == "void")
    return TokenType::VOID;
  return TokenType::IDENTIFIER;
}

std::vector<Token> tokenize(const std::string_view &Toks) {
  std::vector<Token> Tokens;
  std::vector<size_t> indent_stack = {0};

  size_t c_line = 1;
  size_t c_column = 1;
  size_t i = 0;
  while (i < Toks.length()) {
    char c = Toks[i];

    if (std::isspace(static_cast<u_char>(c))) {
      if (c == '\n') {
        c_line++;
        c_column = 1;
      } else if (c == '\t')
        c_column += 4;
      else {
        c_column++;
      }
      i++;
      continue;
    }

    [[maybe_unused]] size_t t_s_column = c_column;

    if (c == '&') {
      if (peek_match(Toks, i, '&')) {
        Tokens.emplace_back(TokenType::AND, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::AMPERSAND, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '|') {
      if (peek_match(Toks, i, '|')) {
        Tokens.emplace_back(TokenType::OR, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        std::println(
            "Syntax Error: Single '|' is unrecognized. Line {}, Col {}", c_line,
            t_s_column);
        exit(1);
      }
      continue;
    }

    if (c == '#') {
      Tokens.emplace_back(TokenType::HASH, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '.') {
      if (peek_match(Toks, i, '.') && peek_match(Toks, i + 1, '.')) {
        if (i + 3 < Toks.length() && Toks[i + 3] == '.') {
          std::println("Syntax Error: Quad-dot operator is unregistered. Line: "
                       "{}, Column: {}",
                       c_line, t_s_column);
          exit(1);
        }
        Tokens.emplace_back(TokenType::RANGE, Toks.substr(i, 3), c_line,
                            t_s_column);
        i += 3;
        c_column += 3;
      } else {
        Tokens.emplace_back(TokenType::DOT, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '+') {
      if (peek_match(Toks, i, '+')) {
        Tokens.emplace_back(TokenType::PLUS_PLUS, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::PLUS_EQUAL, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::PLUS, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '-') {
      if (peek_match(Toks, i, '>')) {
        Tokens.emplace_back(TokenType::ARROW, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '-')) {
        Tokens.emplace_back(TokenType::MINUS_MINUS, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::MINUS_EQUAL, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::MINUS, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '*') {
      if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::MUL_EQUAL, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::MUL, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '/') {
      if (peek_match(Toks, i, '/')) {
        i += 2;
        while (i < Toks.length() && Toks[i] != '\n') {
          i++;
        }
      } else if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::DIV_EQUAL, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '*')) {
        i += 2;
        bool closed = false;
        while (i < Toks.length()) {
          if (Toks[i] == '*' && peek_match(Toks, i, '/')) {
            i += 2;
            closed = true;
            break;
          }

          if (Toks[i] == '\n') {
            c_line++;
            c_column = 1;
          } else {
            c_column++;
          }

          i++;
        }
        if (!closed) {
          std::println("Did you forget to add '/' beside *?");
          exit(1);
        }
      } else {
        Tokens.emplace_back(TokenType::DIV, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '<') {
      if (peek_match(Toks, i, '<')) {
        Tokens.emplace_back(TokenType::L_S_O, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::LTE, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::LT, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '>') {
      if (peek_match(Toks, i, '>')) {
        Tokens.emplace_back(TokenType::R_S_O, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::GTE, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::GT, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '=') {
      if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::EQ, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, '>')) {
        Tokens.emplace_back(TokenType::MAP_ARROW, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        std::println("ERR: Unexpected single '=' at line: {}, column: {}",
                     c_line, t_s_column);
        exit(1);
      }
      continue;
    }

    if (c == '!') {
      if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::NEQ, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::BANG, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == ':') {
      if (peek_match(Toks, i, '=')) {
        Tokens.emplace_back(TokenType::ASSIGN, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else if (peek_match(Toks, i, ':')) {
        Tokens.emplace_back(TokenType::DOUBLE_COLON, Toks.substr(i, 2), c_line,
                            t_s_column);
        i += 2;
        c_column += 2;
      } else {
        Tokens.emplace_back(TokenType::COLON, Toks.substr(i, 1), c_line,
                            t_s_column);
        i++;
        c_column++;
      }
      continue;
    }

    if (c == '%') {
      Tokens.emplace_back(TokenType::MOD, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '?') {
      Tokens.emplace_back(TokenType::QUESTION, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == ';') {
      Tokens.emplace_back(TokenType::SEMI_COLON, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '(') {
      Tokens.emplace_back(TokenType::L_PAREN, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == ')') {
      Tokens.emplace_back(TokenType::R_PAREN, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '{') {
      Tokens.emplace_back(TokenType::LBRACE, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '}') {
      Tokens.emplace_back(TokenType::RBRACE, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '[') {
      Tokens.emplace_back(TokenType::LBRACKET, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == ']') {
      Tokens.emplace_back(TokenType::RBRACKET, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == ',') {
      Tokens.emplace_back(TokenType::COMMA, Toks.substr(i, 1), c_line,
                          t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '"') {
      size_t start = i + 1;
      i++;
      bool closed = false;

      while (i < Toks.length()) {
        if (Toks[i] == '"') {
          closed = true;
          break;
        }

        if (Toks[i] == '\n') {
          c_line++;
          c_column = 1;
        } else {
          c_column++;
        }
        i++;
      }

      if (!closed) {
        std::println("ERR: Unclosed string literal at line: {}, column: {}",
                     c_line, t_s_column);
        exit(1);
      }
      Tokens.emplace_back(TokenType::STRING_LITERAL,
                          Toks.substr(start, i - start), c_line, t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '\'') {
      size_t start = i + 1;
      i++;
      bool closed = false;

      while (i < Toks.length()) {
        if (Toks[i] == '\'') {
          closed = true;
          break;
        }

        if (Toks[i] == '\n') {
          c_line++;
          c_column = 1;
        } else {
          c_column++;
        }
        i++;
      }

      if (!closed) {
        std::println("[ERR] Unclosed raw-string literal!");
        exit(1);
      }

      Tokens.emplace_back(TokenType::RAW_STRING_LITERAL,
                          Toks.substr(start, i - start), c_line, t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (c == '`') {
      size_t start = i + 1;
      i++;
      bool closed = false;

      while (i < Toks.length()) {
        if (Toks[i] == '`') {
          closed = true;
          break;
        }

        if (Toks[i] == '\n') {
          c_line++;
          c_column = 1;
        } else {
          c_column++;
        }
        i++;
      }

      if (!closed) {
        std::println("[ERR] Unclosed back-tick literal at line: {}, column: {}",
                     c_line, t_s_column);
        exit(1);
      }

      Tokens.emplace_back(TokenType::BACK_TICK_LITERAL,
                          Toks.substr(start, i - start), c_line, t_s_column);
      i++;
      c_column++;
      continue;
    }

    if (std::isdigit(static_cast<u_char>(c))) {
      size_t start = i;
      bool is_float = false;

      while (i < Toks.length()) {
        char current = Toks[i];
        if (std::isdigit(static_cast<u_char>(current))) {
        } else if (current == '.' && !is_float) {
          if (peek_match(Toks, i, '.'))
            break;
          is_float = true;
        } else
          break;
        i++;
        c_column++;
      }
      size_t length = i - start;

      TokenType type =
          is_float ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL;
      Tokens.emplace_back(type, Toks.substr(start, length), c_line, t_s_column);

      continue;
    }

    if (std::isalpha(static_cast<u_char>(c)) || c == '_') {
      size_t start = i;

      while (i < Toks.length()) {
        char current = Toks[i];

        if (std::isalnum(static_cast<u_char>(current)) || current == '_') {
        } else
          break;
        i++;
        c_column++;
      }
      size_t length = i - start;

      Tokens.emplace_back(identifier_or_keyword(Toks.substr(start, length)),
                          Toks.substr(start, length), c_line, t_s_column);
      continue;
    }

    std::println("Syntax Error: Unknown token choice '{}' encountered at Line "
                 "{}, Column {}",
                 c, c_line, t_s_column);
    exit(1);
  }

  Tokens.emplace_back(TokenType::END_OF_FILE, "", c_line, c_column + 1);
  return Tokens;
}

std::string token_return(TokenType type) {
  switch (type) {
  case TokenType::IMPORT:
    return "IMPORT";
  case TokenType::FUNCTION:
    return "FUNCTION";
  case TokenType::IF:
    return "IF";
  case TokenType::ELSEIF:
    return "ELSEIF";
  case TokenType::ELSE:
    return "ELSE";
  case TokenType::FOR:
    return "FOR";
  case TokenType::WHILE:
    return "WHILE";
  case TokenType::LET:
    return "LET";
  case TokenType::RETURN:
    return "RETURN";
  case TokenType::BREAK:
    return "BREAK";
  case TokenType::CONTINUE:
    return "CONTINUE";
  case TokenType::OR:
    return "OR";
  case TokenType::AND:
    return "AND";
  case TokenType::TRUE:
    return "TRUE";
  case TokenType::FALSE:
    return "FALSE";
  case TokenType::IN:
    return "IN";
  case TokenType::MAIN:
    return "MAIN";
  case TokenType::SWITCH:
    return "SWITCH";
  case TokenType::UNWRAP:
    return "UNWRAP";
  case TokenType::EXIT:
    return "EXIT";
  case TokenType::LENGTH:
    return "LENGTH";
  case TokenType::JOIN:
    return "JOIN";
  case TokenType::EVAL:
    return "EVAL";
  case TokenType::NAMESPACE:
    return "NAMESPACE";
  case TokenType::HASH:
    return "HASH";
  case TokenType::AMPERSAND:
    return "AMPERSAND";
  case TokenType::ARROW:
    return "ARROW";
  case TokenType::RANGE:
    return "RANGE";
  case TokenType::PLUS_PLUS:
    return "PLUS_PLUS";
  case TokenType::MINUS_MINUS:
    return "MINUS_MINUS";
  case TokenType::PLUS_EQUAL:
    return "PLUS_EQUAL";
  case TokenType::MINUS_EQUAL:
    return "MINUS_EQUAL";
  case TokenType::R_S_O:
    return "R_S_O";
  case TokenType::L_S_O:
    return "L_S_O";
  case TokenType::MUL_EQUAL:
    return "MUL_EQUAL";
  case TokenType::DIV_EQUAL:
    return "DIV_EQUAL";
  case TokenType::LTE:
    return "LTE";
  case TokenType::GTE:
    return "GTE";
  case TokenType::EQ:
    return "EQ";
  case TokenType::NEQ:
    return "NEQ";
  case TokenType::MAP_ARROW:
    return "MAP_ARROW";
  case TokenType::DOUBLE_COLON:
    return "DOUBLE_COLON";
  case TokenType::ASSIGN:
    return "ASSIGN";
  case TokenType::PLUS:
    return "PLUS";
  case TokenType::MINUS:
    return "MINUS";
  case TokenType::MUL:
    return "MUL";
  case TokenType::DIV:
    return "DIV";
  case TokenType::MOD:
    return "MOD";
  case TokenType::LT:
    return "LT";
  case TokenType::GT:
    return "GT";
  case TokenType::DOT:
    return "DOT";
  case TokenType::BANG:
    return "BANG";
  case TokenType::QUESTION:
    return "QUESTION";
  case TokenType::COLON:
    return "COLON";
  case TokenType::SEMI_COLON:
    return "SEMI_COLON";
  case TokenType::COMMA:
    return "COMMA";
  case TokenType::L_PAREN:
    return "L_PAREN";
  case TokenType::R_PAREN:
    return "R_PAREN";
  case TokenType::LBRACE:
    return "LBRACE";
  case TokenType::RBRACE:
    return "RBRACE";
  case TokenType::LBRACKET:
    return "LBRACKET";
  case TokenType::RBRACKET:
    return "RBRACKET";
  case TokenType::FLOAT_LITERAL:
    return "FLOAT_LITERAL";
  case TokenType::INT_LITERAL:
    return "INT_LITERAL";
  case TokenType::STRING_LITERAL:
    return "STRING_LITERAL";
  case TokenType::RAW_STRING_LITERAL:
    return "RAW_STRING_LITERAL";
  case TokenType::BACK_TICK_LITERAL:
    return "BACK_TICK_LITERAL";
  case TokenType::AUTO:
    return "AUTO";
  case TokenType::CONST:
    return "CONST";
  case TokenType::VOID:
    return "VOID";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::END_OF_FILE:
    return "END_OF_FILE";
  default:
    return "UNKNOWN";
  }
}
