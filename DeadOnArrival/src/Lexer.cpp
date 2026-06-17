#include "Lexer.hpp"
#include "Token.hpp"
#include <ctre.hpp>
#include <cctype>

namespace Lexer {
// 1. Operators
static constexpr auto TokenPattern = ctll::fixed_string
{
    R"((?<comment>//[^\n]*))"
    R"(|(?<float_literal>\d+\.\d+)|(?<int_literal>\d+))"

    R"(|(?<string_literal>"(?:[^"\\]|\\.)*")|(?<raw_string_literal>'(?:[^'\\]|\\.)*')|(?<back_tick_literal>`(?:[^`\\]|\\.)*`))"

    R"(|(?<ident>[a-zA-Z_][a-zA-Z0-9_]*))"
    R"(|(?<op>\.\.\.|->|=>|\+\+|--|:=|\+=|-=|\*=|\/=|%=|~=|~\+|>>|<<|<=|>=|==|!=|&&|\|\||::|&|\||#|\.|\+|-|\*|\/|%|<|>|!|\?|:|;|,|\(|\)|\{|\}|\[|\]))"
};

std::vector<Token::TokenData> Lex(const std::string_view Toks)
{
  std::vector<Token::TokenData> Tokens;

  Tokens.reserve(Toks.length() / 5);

  std::size_t _st_fileLine_ = 1;
  std::size_t _st_fileColumn_ = 1;

  std::size_t i = {0};
  while (i < Toks.length())
  {
    std::string_view _sv_fileSlice_ = Toks.substr(i);

    char _sv_CfileSlice_zI = _sv_fileSlice_[0];

    // Handle whitespace
    if (std::isspace(static_cast<unsigned char>(_sv_CfileSlice_zI)))
    {
      if (_sv_CfileSlice_zI == '\n')
      {
        _st_fileLine_++;
        _st_fileColumn_ = 1;
      } else if (_sv_CfileSlice_zI == '\t')
        _st_fileColumn_ += 4;
      else
        _st_fileColumn_++;

      i++;
      continue;
    }

    if (auto match = ctre::starts_with<TokenPattern>(_sv_fileSlice_))
    {
      std::string_view _sv_Mstr_ = match.to_view();

      Token::TokenType type = Token::TokenType::Unknown;
      if (match.get<"comment">())
      {
        i += _sv_Mstr_.length();
        _st_fileColumn_ += _sv_Mstr_.length();
        continue;
      }
      else if (match.get<"float_literal">())
        type = Token::TokenType::FloatLiteral;

      else if (match.get<"int_literal">())
        type = Token::TokenType::IntLiteral;

      else if (match.get<"string_literal">())
        type = Token::TokenType::StringLiteral;

      else if (match.get<"raw_string_literal">())
        type = Token::TokenType::RawStringLiteral;

      else if (match.get<"back_tick_literal">())
        type = Token::TokenType::BackTickLiteral;
      else if (match.get<"ident">() || match.get<"op">())
        type = findKeywordOrIdentifier(_sv_Mstr_);

      Tokens.emplace_back(type, _sv_Mstr_, _st_fileLine_, _st_fileColumn_);

      for (char ch : _sv_Mstr_ )
        if (ch == '\n')
        {
          _st_fileLine_++;
          _st_fileColumn_ = 1;
        }
        else if (ch == '\t')
          _st_fileColumn_ += 4;
        else
          _st_fileColumn_++;

      i += _sv_Mstr_.length();
    } else
    {
      Tokens.emplace_back(Token::TokenType::Unknown, _sv_fileSlice_.substr(0, 1), _st_fileLine_, _st_fileColumn_);
      i++;
      _st_fileColumn_++;
    }
  }

  // 4. EOF: safely appended Once reaches the end of file.
  Tokens.emplace_back(Token::TokenType::EndOfFile, "", _st_fileLine_, _st_fileColumn_);
  return Tokens;
}

} // namespace Lexer
