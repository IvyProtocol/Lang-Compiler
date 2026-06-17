#include "Lexer.hpp"
#include "span"
#include <fstream>
#include <string>
#include <ranges>
#include <sstream>
#include <print>

auto main(int argc, char *argv[]) -> std::int32_t
{
  std::span<char const *const> _arguments
  {
    std::views::counted
    (
      argv, argc
    )
  };

  if (_arguments.size() < 2) {
    std::println("Error: A file input pathway is required.");
    exit(1);
  }

  std::string_view _args = _arguments[1];

  std::stringstream contents;
  std::ifstream file
  {
    static_cast<std::string>(_args), std::ios::ate
  };

  std::string file_text;
  file_text.reserve(static_cast<std::size_t>(file.tellg()));

  file.seekg(0, std::ios::beg);

  file_text.assign
  (
    (
      std::istreambuf_iterator<char>(file)
    ),
    std::istreambuf_iterator<char>()
  );

  std::vector<Token::TokenData> Tok = Lexer::Lex(file_text);
}
