#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <cstdint>
#include <string_view>

namespace Token {
enum class TokenType : std::int32_t {

  /*
   * Keywords
   */

  Import,    // ImportKeyword
  Function,  // FunctionKeyword
  If,        // IfKeyword
  ElseIf,    // ElseIfKeyword
  Else,      // ElseKeyword
  For,       // ForKeyword
  While,     // WhileKeyword
  Let,       // LetKeyword
  Return,    // ReturnKeyword
  Break,     // BreakKeyword
  Continue,  // ContinueKeyword
  True,      // TrueKeyword
  False,     // FalseKeyword
  In,        // InKeyword
  Main,      // MainKeyword
  Switch,    // SwitchKeyword
  Unwrap,    // UnwrapKeywrod
  Exit,      // ExitKeyword
  Length,    // LengthKeyword
  Join,      // JoinKeyword
  Eval,      // EvalKeyword
  Namespace, // NamespaceKeyword
  Class,     // ClassKeyword
  Enum,      // EnumKeyword
  Public,    // PublicKeyword
  Private,   // PrivateKeyword

  /*
   * Operators
   */

  Arrow,      // ArrowOperator
  MapArrow,   // MapArrowOperator
  Range,      // RangeOperator
  PlusPlus,   // PlusPlusOperator
  MinusMinus, // MinusMinusOperator

  /*
   * Assignment Operators
   */

  Assign,       // AssignOperator
  PlusEqual,    // PlusEqualOperator
  MinusEqual,   // MatchEqualOperator
  MulEqual,     // MulEqualOperator
  DivEqual,     // DivEqualOperator
  ModEqual,     // ModEqualOperator
  RegMatch,     // RegMatchOperator
  MatchAdvance, // MatchAdvanceOperator

  /*
   * BitWise Operators
   */

  RightShift, // RightShiftOperator
  LeftShift,  // LeftShiftOperator
  Ampersand,  // AmpersandOperator
  Pipe,       // PipeOperator

  /*
   * Comparison Operators
   */

  LessOrEqual,    // LessOrEqualOperator
  GreaterOrEqual, // GreaterOrEqualOperator
  Equal,          // EqualOperator
  NotEqual,       // NotEqualOperator
  GreaterThan,    // GreaterThanOperator
  LessThan,       // LessThanOperator
  And,            // AndOperator
  Or,             // OrOperator
  Bang,           // BangOperator

  /*
   * Mathematical Operators
   */

  Plus,  // PlusOperator
  Minus, // MinusOperator
  Mul,   // MulOperator
  Div,   // DivOperator
  Mod,   // ModOperator

  /*
   * Ternary Operators
   */

  Question, // QuestionOperator
  Colon,    // ColonOperator

  /*
   * Separator/Temrinator
   */

  Comma,       // CommaSeparator
  DoubleColon, // DoubleColonSeparator
  Dot,         // DotSeparator
  SemiColon,   // SemiColonTerminator

  /*
   * Brackets
   */

  LeftParen,  // LeftParen
  RightParen, // RightParen

  LeftBrace,  // Leftbrace
  RightBrace, // RightBrace

  LeftBracket,  // LeftBracket
  RightBracket, // RightBracket

  /*
   * Literals
   */
  FloatLiteral,     // FloatLiteral
  IntLiteral,       // IntLiteral
  StringLiteral,    // StringLiteral
  RawStringLiteral, // RawStringLiteral
  BackTickLiteral,  // BackTickLiteral

  /*
   * Primitive default Types
   */
  Auto,      // AutoType
  ConstExpr, // ConstExpr
  Const,     // ConstType
  Void,      // VoidType
  Mut,       // MutType

  /*
   * Default
   */

  Identifier, // IdentifierMarker
  Unknown,    // UnknownMarker
  EndOfFile,  // EndOfFileMarker

};

struct TokenData {
  TokenType Type;
  long : (8 * 4);
  std::string_view Sv_Val_;
  std::size_t Lexer_Size_t_Line_;
  std::size_t Lexer_Size_t_Column_;
};
} // namespace Token

#endif
