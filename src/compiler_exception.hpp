#pragma once
// reviewed: 2025-09-28

#include "token.hpp"
#include "tokenizer.hpp"

class compiler_exception final : public std::exception {
  public:
    std::string msg;
    size_t start_index{};
    size_t end_index{};

    compiler_exception(const token& tk, std::string message)
        : msg{std::move(message)}, start_index{tk.start_index()},
          end_index{tk.end_index()} {}

    compiler_exception(const tokenizer& tz, std::string message)
        : msg{std::move(message)},
          start_index{tz.current_char_index_in_source()},
          end_index{tz.current_char_index_in_source()} {}

    [[nodiscard]] auto what() const noexcept -> const char* override {
        return msg.c_str();
    }
};
