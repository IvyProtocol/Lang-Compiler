#ifndef LEXER_HPP
#define LEXER_HPP
#include "Token.hpp"
#include <vector>

TokenType identifier_or_keyword(const std::string_view &word);
std::vector<Token> tokenize(const std::string_view &Toks);
std::string token_return(TokenType type);

#endif
