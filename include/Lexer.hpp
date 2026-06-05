#pragma once
#ifndef LEXER_HPP
#define LEXER_HPP
#include <vector>
#include "Token.hpp"

TokenType identifier_or_keyword(const std::string_view &word);
std::vector<Token> tokenize(const std::string_view &Toks);
std::string token_return(TokenType type);

#endif
