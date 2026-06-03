#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
enum class TokenType : std::int32_t {
  IMPORT,             // import <>
  FUNCTION,           // function name(type: Parmaeter) -> r_type: {}
  IF,                 // if (condition) {}
  ELSEIF,             // elseif (condition) {}
  ELSE,               // else {}
  FOR,                // for ( condition ) {}
  WHILE,              // while ( condition ) {}
  LET,                // let (x, y, z)
  RETURN,             // return(x, y, z)
  BREAK,              // break
  CONTINUE,           // continue
  OR,                 // or ||
  AND,                // and &&
  TRUE,               // true
  FALSE,              // false
  IN,                 // in
  MAIN,               // function main(type: o_Param) -> int: {}
  SWITCH,             // switch (variable or something) { (options) => do; }
  UNWRAP,             // unwrap(variable)
  EXIT,               // exit(i)
  LENGTH,             // len()
  JOIN,               // join()
  EVAL,               // eval()
  HASH,               // #
  AMPERSAND,          // &
  ARROW,              // ->
  RANGE,              // ...
  PLUS_PLUS,          // ++
  MINUS_MINUS,        // --
  PLUS_EQUAL,         // +=
  MINUS_EQUAL,        // -=
  MUL_EQUAL,          // *=
  DIV_EQUAL,          // /=
  R_S_O,              // <<
  L_S_O,              // >>
  LTE,                // <=
  GTE,                // >=
  EQ,                 // ==
  NEQ,                // !=
  MAP_ARROW,          // =>
  DOUBLE_COLON,       // ::
  ASSIGN,             // :=
  PLUS,               // +
  MINUS,              // -
  MUL,                // *
  DIV,                // '/'
  MOD,                // %
  LT,                 // <
  GT,                 // >
  DOT,                // '.'
  BANG,               // !
  QUESTION,           // ?
  COLON,              // :
  SEMI_COLON,         // ;
  COMMA,              // ,
  L_PAREN,            // (
  R_PAREN,            // )
  LBRACE,             // {
  RBRACE,             // }
  LBRACKET,           // [
  RBRACKET,           // ]
  FLOAT_LITERAL,      // 0.0
  INT_LITERAL,        // 0
  STRING_LITERAL,     // "hello"
  RAW_STRING_LITERAL, // 'hello'
  BACK_TICK_LITERAL,  // '`'
  BOOL,               // bool
  AUTO,               // auto
  CONST,              // const
  VOID,               // void
  IDENTIFIER,         //
  END_OF_FILE
};

struct Token {
  TokenType type;
  long : (8 * 4);
  std::string_view value;
  size_t line;
  size_t column;
};
