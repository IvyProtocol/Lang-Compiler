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

void log_err(std::string_view msg, size_t line, size_t col) {
  std::println("[ERR]: {} at Line {}, Column {}", msg, line, col);
}
[[maybe_unused]] void log_err(std::string_view msg, const Token &t) {
  log_err(msg, t.line, t.column);
}

} // namespace

#endif
