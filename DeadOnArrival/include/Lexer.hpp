#ifndef LEXER_HPP
#define LEXER_HPP

#include "Token.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace Lexer {
using TokenType = Token::TokenType;

class KeywordPair {
public:
  std::string_view text;
  TokenType type;
  std::byte _pad[4]{};
};

constexpr std::array KeyWords
{
KeywordPair{"!", TokenType::Bang},
KeywordPair{"!=", TokenType::NotEqual},
KeywordPair{"%", TokenType::Mod},
KeywordPair{"%=", TokenType::ModEqual},
KeywordPair{"&", TokenType::Ampersand},
KeywordPair{"&&", TokenType::And},
KeywordPair{"(", TokenType::LeftParen},
KeywordPair{")", TokenType::RightParen},
KeywordPair{"*", TokenType::Mul},
KeywordPair{"*=", TokenType::MulEqual},
KeywordPair{"+", TokenType::Plus},
KeywordPair{"++", TokenType::PlusPlus},
KeywordPair{"+=", TokenType::PlusEqual},
KeywordPair{",", TokenType::Comma},
KeywordPair{"-", TokenType::Minus},
KeywordPair{"--", TokenType::MinusMinus},
KeywordPair{"-=", TokenType::MinusEqual},
KeywordPair{"->", TokenType::Arrow},
KeywordPair{".", TokenType::Dot},
KeywordPair{"...", TokenType::Range},
KeywordPair{"/", TokenType::Div},
KeywordPair{"/=", TokenType::DivEqual},
KeywordPair{":", TokenType::Colon},
KeywordPair{"::", TokenType::DoubleColon},
KeywordPair{":=", TokenType::Assign},
KeywordPair{";", TokenType::SemiColon},
KeywordPair{"<", TokenType::LessThan},
KeywordPair{"<<", TokenType::LeftShift},
KeywordPair{"<=", TokenType::LessOrEqual},
KeywordPair{"==", TokenType::Equal},
KeywordPair{"=>", TokenType::MapArrow},
KeywordPair{">", TokenType::GreaterThan},
KeywordPair{">=", TokenType::GreaterOrEqual},
KeywordPair{">>", TokenType::RightShift},
KeywordPair{"?", TokenType::Question},
KeywordPair{"[", TokenType::LeftBracket},
KeywordPair{"]", TokenType::RightBracket},
KeywordPair{"auto", TokenType::Auto},
KeywordPair{"break", TokenType::Break},
KeywordPair{"class", TokenType::Class},
KeywordPair{"const", TokenType::Const},
KeywordPair{"constexpr", TokenType::ConstExpr},
KeywordPair{"continue", TokenType::Continue},
KeywordPair{"else", TokenType::Else},
KeywordPair{"elseif", TokenType::ElseIf},
KeywordPair{"enum", TokenType::Enum},
KeywordPair{"eval", TokenType::Eval},
KeywordPair{"exit", TokenType::Exit},
KeywordPair{"false", TokenType::False},
KeywordPair{"for", TokenType::For},
KeywordPair{"func", TokenType::Function},
KeywordPair{"if", TokenType::If},
KeywordPair{"import", TokenType::Import},
KeywordPair{"in", TokenType::In},
KeywordPair{"join", TokenType::Join},
KeywordPair{"len", TokenType::Length},
KeywordPair{"let", TokenType::Let},
KeywordPair{"main", TokenType::Main},
KeywordPair{"mut", TokenType::Mut},
KeywordPair{"namespace", TokenType::Namespace},
KeywordPair{"private", TokenType::Private},
KeywordPair{"public", TokenType::Public},
KeywordPair{"return", TokenType::Return},
KeywordPair{"switch", TokenType::Switch},
KeywordPair{"true", TokenType::True},
KeywordPair{"unwrap", TokenType::Unwrap},
KeywordPair{"void", TokenType::Void},
KeywordPair{"while", TokenType::While},
KeywordPair{"{", TokenType::LeftBrace},
KeywordPair{"|", TokenType::Pipe},
KeywordPair{"||", TokenType::Or},
KeywordPair{"}", TokenType::RightBrace},
KeywordPair{"~+", TokenType::MatchAdvance},
KeywordPair{"~=", TokenType::RegMatch}
};

inline constexpr TokenType findKeywordOrIdentifier(std::string_view Toks) {
  auto it =
      std::lower_bound(KeyWords.begin(), KeyWords.end(), Toks,
                       [](const KeywordPair &pair, std::string_view value) {
                         return pair.text < value;
                       });

  if (it != KeyWords.end() && it->text == Toks)
    return it->type;

  return TokenType::Identifier;
}

std::vector<Token::TokenData> Lex(std::string_view Toks);


} // namespace Lexer

#endif
