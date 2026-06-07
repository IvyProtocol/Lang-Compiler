#include "../include/Lexer.hpp"
#include "../include/Parser.hpp"
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <span>
#include <sstream>

auto main(int argc, char *argv[]) -> std::int32_t {
  // absolutely gorgeous
  std::span<char const *const> _arguments{std::views::counted(argv, argc)};

  if (_arguments.size() < 2) {
    std::println("Error: A file input pathway is required.");
    exit(1);
  }

  std::string_view _args = _arguments[1];

  std::stringstream contents;
  std::ifstream file{static_cast<std::string>(_args), std::ios::ate};

  if (!file.is_open()) {
    std::println("Couldn't open the execution file context.");
    exit(1);
  }

  std::string file_text;
  file_text.reserve(static_cast<size_t>(file.tellg()));
  file.seekg(0, std::ios::beg);

  file_text.assign((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
  auto tokens = tokenize(file_text);

  std::println("--- LEXER LOGS ---");
  for (const auto &token : tokens) {
    std::println("Line {:<3} | Col {:<3} | {:<20} = {:?}", token.line,
                 token.column, token_return(token.type), token.value);
  }

  std::println("-------------");
  std::println("Successfully parsed {} tokens", tokens.size());

  std::vector<std::string> source_lines;
  std::string line;
  std::istringstream stream(file_text);
  while (std::getline(stream, line))
    source_lines.emplace_back(line);

  Parser parser(tokens, _args, std::move(source_lines));
  std::vector<std::unique_ptr<ASTNode>> ast_tree = parser.parse_program();
  std::println("\n --- PARSER LOGS ---");

  for (const auto &node : ast_tree) {
    node->debug_print();
    std::println("");
  }
  auto ast = parser.parse_primary();

  std::println("\n Successfully parsed {} AST nodes.", ast_tree.size());
  return (0);
}
