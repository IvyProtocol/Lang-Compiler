#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>
#include <string_view>
enum class TokenType : std::int32_t {
  IMPORT,   // import <> Done
  FUNCTION, // function name(type: Parmaeter) -> r_type: {} Done
  IF,       // if (condition) {} Done
  ELSEIF,   // elseif (condition) {} Done
  ELSE,     // else {} Done
  FOR,      // for ( condition ) {} Done
  WHILE,    // while ( condition ) {}
  LET,      // let (x, y, z) Done
  RETURN,   // return(x, y, z) Done
  BREAK,    // break Done
  CONTINUE, // continue Done
  OR,       // or || Kind of done
  AND,      // and && Done
  TRUE,     // true
  FALSE,    // false
  IN,       // in Done
  MAIN,     // function main(type: o_Param) -> int: {}
  SWITCH,   // switch (variable or something) { (options) => do; }
  UNWRAP,   // unwrap(variable)
  EXIT,     // exit(i)
  LENGTH,   // len() // done
  JOIN,     // join() // done
  EVAL,     // eval()
  NAMESPACE,
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
  REG_MATCH,          // ~=
  MATCH_ADVANCE,      // ~+
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
  AUTO,               // auto
  CONSTEXPR,
  CONST,              // const
  VOID,               // void
  MUT,                // mut
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

#endif
