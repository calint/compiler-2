#pragma once
#include "token.hpp"
#include "tokenizer.hpp"

class compiler_exception final : public exception {
public:
  inline compiler_exception(const token &tk, string message)
      : msg{move(message)},
        start_char{tk.char_index()}, end_char{tk.char_index_end()} {}

  // inline compiler_exception(const statement &st, const string &message)
  //     : compiler_exception{st.tok(), message} {}

  inline compiler_exception(const tokenizer &t, string message)
      : msg{move(message)}, start_char{t.current_char_index_in_source()},
        end_char{t.current_char_index_in_source()} {}

  const string msg{};
  size_t start_char{};
  size_t end_char{};

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return msg.c_str();
  }
};
