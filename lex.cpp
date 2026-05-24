#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <vector>
#include <cctype>
#include <print>

enum class TokenType {
  DECLARE,      // declare()
  IMPORT,       // import()
  FUNCTION,     // function name(type: Parmaeter) -> r_type: {}
  IF,           // if (condition) {}
  ELSEIF, // elseif (condition) {}
  ELSE, // else {}
  FOR, // for ( condition ) {}
  WHILE, // while ( condition ) {}
  LET, // let (x, y, z)
  RETURN, // return(x, y, z)
  TYPE_CAST, // typedef()
  BREAK, // break
  CONTINUE, // continue
  OR, // or ||
  AND, // and &&
  TRUE, // true
  FALSE, // false
  MAIN, // function main(type: o_Param) -> int: {}
  SWITCH, // switch (variable or something) { (options) => do; }
  UNWRAP, // unwrap(variable)
  EXIT, // exit(i)
  EVAL,
  ARROW, // ->
  RANGE, // ..
  PLUS_PLUS, // ++
  MINUS_MINUS, // --
  PLUS_EQUAL, // +=
  MINUS_EQUAL, // -=
  MUL_EQUAL, // *=
  DIV_EQUAL, // /=
  R_S_O,
  L_S_O,
  LTE, // <=
  GTE, // >=
  EQ, // ==
  NEQ, // !=
  MAP_ARROW, // =>
  ASSIGN, // :=
  PLUS, // +
  MINUS, // -
  MUL, // *
  DIV, // '/'
  MOD, // %
  LT, // <
  GT, // >
  DOT, // '.'
  BANG, // !
  QUESTION, // ?
  COLON, // :
  SEMI_COLON, // ;
  COMMA, // ,
  L_PAREN, // (
  R_PAREN, // )
  LBRACE, // {
  RBRACE, // }
  LBRACKET, // [
  RBRACKET, // ]
  FLOAT, // float
  FLOAT_LITERAL, // 0.0
  INT, // int
  INT_LITERAL, // 0
  STRING, // str
  STRING_LITERAL, // "hello"
  RAW_STRING_LITERAL, // 'hello'
  BOOL, // bool
  AUTO, // auto
  CONST, // const
  IDENTIFIER // lkdsjf;saljf;lkdsajf
};

struct Token {
  TokenType type;
  long : 32;
  std::vector<std::string> value;
};

inline bool peek_match (
    const std::string& text,
    size_t current_index,
    char expected
)
{
    if ( current_index + 1 >= text.length() ) return false;
    return text.at(current_index + 1) == expected;
}

inline TokenType identifier_or_keyword (const std::string& word) {
    if ( word == "declare" ) return TokenType::DECLARE;
    if ( word == "import" ) return TokenType::IMPORT;
    if ( word == "function" ) return TokenType::FUNCTION;
    if ( word == "if" ) return TokenType::IF;
    if ( word == "elseif" ) return TokenType::ELSEIF;
    if ( word == "else" ) return TokenType::ELSE;
    if ( word == "for" ) return TokenType::FOR;
    if ( word == "while" ) return TokenType::WHILE;
    if ( word == "let" ) return TokenType::LET;
    if ( word == "return" ) return TokenType::RETURN;
    if ( word == "typedef") return TokenType::TYPE_CAST;
    if ( word == "break" ) return TokenType::BREAK;
    if ( word == "continue" ) return TokenType::CONTINUE;
    if ( word == "or" ) return TokenType::OR;
    if ( word == "and" ) return TokenType::AND;
    if ( word == "true" ) return TokenType::TRUE;
    if ( word == "false" ) return TokenType::FALSE;
    if ( word == "main" ) return TokenType::MAIN;
    if ( word == "switch" ) return TokenType::SWITCH;
    if ( word == "unwrap" ) return TokenType::UNWRAP;
    if ( word == "exit" ) return TokenType::EXIT;
    if ( word == "eval" ) return TokenType::EVAL;
    if ( word == "int" ) return TokenType::INT;
    if ( word == "float" ) return TokenType::FLOAT;
    if ( word == "str" ) return TokenType::STRING;
    if ( word == "bool" ) return TokenType::BOOL;
    if ( word == "auto" ) return TokenType::AUTO;
    if ( word == "const" ) return TokenType::CONST;
    return TokenType::IDENTIFIER;
}

inline std::vector<Token> tokenize(const std::string& Toks) {
    std::vector<Token> Tokens;
    std::string buf = "";

    for ( size_t i {}; i < Toks.length(); i++ )
    {
        char c = Toks.at(i);

        if ( std::isspace(static_cast<unsigned char>(c)) )
        {
            continue;
        }


        if ( c == '.' )
        {
            if ( peek_match(Toks, i, '.') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::RANGE, {".."}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::DOT, {"."}
                    }
                );
            }
            continue;
        }

        if ( c == '+' )
        {
            if ( peek_match(Toks, i, '+') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::PLUS_PLUS, {"++"}
                    }
                );
                i++;
            }
            else if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::PLUS_EQUAL, {"+="}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::PLUS, {"+"}
                    }
                );
            }
            continue;
        }

        if ( c == '-' )
        {
            if ( peek_match(Toks, i, '>') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::ARROW, {"->"}
                    }
                );
                i++;
            }

            else if ( peek_match(Toks, i, '-') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::MINUS_MINUS, {"--"}
                    }
                );
                i++;
            }

            else if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::MINUS_EQUAL, {"-="}
                    }
                );
                i++;
            }

            else {
                Tokens.push_back
                (
                    {
                        TokenType::MINUS, {"-"}
                    }
                );
            }
            continue;
        }

        if ( c == '*' )
        {
            if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::MUL_EQUAL, {"*="}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::MUL, {"*"}
                    }
                );
            }
            continue;
        }

        if ( c == '/' )
        {
            if ( peek_match(Toks, i, '/') )
            {
                i++;
                while ( i < Toks.length() && Toks.at(i) != '\n')
                {
                    i++;
                }
            }
            else if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::DIV_EQUAL, {"/="}
                    }
                );
                i++;
            }
            else if ( peek_match(Toks, i, '*') )
            {
                i += 2;
                bool closed = false;
                while ( i < Toks.length() )
                {
                    if ( Toks.at(i) == '*' && peek_match(Toks, i, '/') )
                    {
                        i++;
                        closed = true;
                        break;
                    }
                    i++;
                }
                if ( ! closed ) {
                    std::println("Did you forget to add '/' beside *?");
                    exit(1);
                }
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::DIV, {"/"}
                    }
                );
            }
            continue;
        }

        if ( c == '<' )
        {
            if ( peek_match(Toks, i, '<') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::L_S_O, {"<<"}
                    }
                );
                i++;
            }
            else if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::LTE, {"<="}
                    }
                );
                i++;
            }

            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::LT, {"<"}
                    }
                );
            }
            continue;
        }

        if ( c == '>' )
        {
            if ( peek_match(Toks, i, '>') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::R_S_O, {">>"}
                    }
                );
                i++;
            }
            else if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::GTE, {">="}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::GT, {">"}
                    }
                );
            }
            continue;
        }

        if ( c == '=' )
        {
            if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::EQ, {"=="}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::MAP_ARROW, {"=>"}
                    }
                );
                i++;
            }
            continue;
        }

        if ( c == '!' )
        {
            if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::NEQ, {"!="}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::BANG, {"!"}
                    }
                );
            }
            continue;
        }

        if ( c == ':' )
        {
            if ( peek_match(Toks, i, '=') )
            {
                Tokens.push_back
                (
                    {
                        TokenType::ASSIGN, {":="}
                    }
                );
                i++;
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::COLON, {":"}
                    }
                );
            }
            continue;
        }

        if ( c == '%' )
        {
            Tokens.push_back
            (
                {
                    TokenType::MOD, {"%"}
                }
            );
            continue;
        }

        if ( c == '?' )
        {
            Tokens.push_back
            (
                {
                    TokenType::QUESTION, {"?"}
                }
            );
            continue;
        }

        if ( c == ';' )
        {
            Tokens.push_back
            (
                {
                    TokenType::SEMI_COLON, {";"}
                }
            );
        }

        if ( c == '(' )
        {
            Tokens.push_back
            (
                {
                    TokenType::L_PAREN, {"("}
                }
            );
            continue;
        }

        if ( c == ')' )
        {
            Tokens.push_back
            (
                {
                    TokenType::R_PAREN, {")"}
                }
            );
            continue;
        }

        if ( c == '{' )
        {
            Tokens.push_back
            (
                {
                    TokenType::LBRACE, {"{"}
                }
            );
            continue;
        }

        if ( c == '}' )
        {
            Tokens.push_back
            (
                {
                    TokenType::RBRACE, {"}"}
                }
            );
            continue;
        }

        if ( c == '[' )
        {
            Tokens.push_back
            (
                {
                    TokenType::LBRACKET, {"["}
                }
            );
            continue;
        }

        if ( c == ']' )
        {
            Tokens.push_back
            (
                {
                    TokenType::RBRACKET, {"]"}
                }
            );
            continue;
        }

        if ( c == ',' )
        {
            Tokens.push_back
            (
                {
                    TokenType::COMMA, {","}
                }
            );
            continue;
        }

        if ( c == '"' )
        {
            std::string str_val = "";
            i++;
            bool closed = false;

            while ( i < Toks.length() )
            {
                if ( Toks.at(i) == '"' )
                {
                    closed = true;
                    break;
                }
                str_val += Toks.at(i);
                i++;
            }

            if ( ! closed )
            {
                std::println("ERR: Unclosed string literal!");
                exit(1);
            }
            Tokens.push_back
            (
                {
                    TokenType::STRING_LITERAL, {str_val}
                }
            );
            continue;
        }

        if ( c == '\'')
        {
            i++;
            std::string raw_str_val = "";
            bool closed = false;

            while ( i < Toks.length() )
            {
                if ( Toks.at(i) == '\'' )
                {
                    closed = true;
                    break;
                }
                raw_str_val += Toks.at(i);
                i++;
            }

            if ( ! closed )
            {
                std::println("[ERR] Unclosed raw-string literal!");
                exit(1);
            }

            Tokens.push_back
            (
                {
                    TokenType::RAW_STRING_LITERAL, {raw_str_val}
                }
            );
            continue;

        }

        if ( std::isdigit( static_cast<unsigned char>(c) ) )
        {
            std::string num_val = "";
            bool is_float = false;

            while ( i < Toks.length() )
            {
                char current = Toks.at(i);
                if ( std::isdigit( static_cast<unsigned char>(current) ) )
                {
                    num_val += current;
                }
                else if ( current == '.' && !is_float )
                {
                    if ( peek_match(Toks, i, '.') )
                    {
                        break;
                    }
                    is_float = true;
                    num_val += current;
                }
                else
                {
                    break;
                }
                i++;
            }
            i--;

            if ( ! is_float )
            {
                Tokens.push_back
                (
                    {
                        TokenType::FLOAT_LITERAL, {num_val}
                    }
                );
            }
            else
            {
                Tokens.push_back
                (
                    {
                        TokenType::INT_LITERAL, {num_val}
                    }
                );
            }
            continue;
        }

        if ( std::isalpha( static_cast<unsigned char>(c) ) )
        {
            buf.push_back(c);
            i++;

            while ( i < Toks.length() && std::isalnum(static_cast<unsigned char>(Toks.at(i))) )
            {
                buf.push_back(Toks.at(i));
                i++;
            }
            i--;

            std::string word
            (
                buf.begin(), buf.end()
            );
            Tokens.push_back
            (
                {
                    TokenType::IDENTIFIER, {word}
                }
            );
            buf.clear();
        }
    }
    return Tokens;
}

auto main ( void ) -> std::int32_t {
    std::stringstream contents;
    std::ifstream file {
        static_cast<std::string>(
            std::getenv("HOME")
        ) + "/Templates/Nullix/rofilauncher.ts"
    };

    contents << file.rdbuf();
    tokenize(contents.str());

}
