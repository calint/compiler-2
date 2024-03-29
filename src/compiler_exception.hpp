#pragma once
#include "token.hpp"
#include "tokenizer.hpp"

class compiler_exception final : public exception {
public:
  string msg{};
  size_t start_index{};
  size_t end_index{};

  inline compiler_exception(const token &tk, string message)
      : msg{move(message)},
        start_index{tk.start_index()}, end_index{tk.end_index()} {}

  inline compiler_exception(const tokenizer &tz, string message)
      : msg{move(message)}, start_index{tz.current_char_index_in_source()},
        end_index{tz.current_char_index_in_source()} {}

  [[nodiscard]] inline auto what() const noexcept -> const char * override {
    return msg.c_str();
  }
};
