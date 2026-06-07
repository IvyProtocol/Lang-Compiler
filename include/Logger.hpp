#ifndef LOGGER_HPP
#define LOGGER_HPP
#include "Token.hpp"
#include <print>
#include <string_view>

namespace {

void log_mov(std::string_view msg, size_t line, size_t col) {
  std::println("[MOV]: {} at Line {}, Column {}", msg, line, col);
}
[[maybe_unused]] void log_mov(std::string_view msg, const Token &t) {
  log_mov(msg, t.line, t.column);
}

} // namespace

namespace ConsoleColor {
inline const std::string RED = "\033[1;31m";
inline const std::string GREEN = "\033[1;32m";
inline const std::string YELLOW = "\033[1;33m";
inline const std::string RESET = "\033[0m";

inline const std::string BOLD_RED = "\033[1;31m";
inline const std::string BOLD_GREEN = "\033[1;32m";
inline const std::string BOLD_YELLOW = "\033[1;33m";
} // namespace ConsoleColor
#endif
