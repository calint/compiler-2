#pragma once
#include "token.hpp"
#include "tokenizer.hpp"

class compiler_exception final : public exception {
public:
  const string &msg{};
  size_t start_char{};
  size_t end_char{};

  inline compiler_exception(const token &tk, const string &message)
      : msg{message}, start_char{tk.char_index()}, end_char{
                                                       tk.char_index_end()} {}

  // inline compiler_exception(const statement &st, const string &message)
  //     : compiler_exception{st.tok(), message} {}

  inline compiler_exception(const tokenizer &tz, const string &message)
      : msg{message}, start_char{tz.current_char_index_in_source()},
        end_char{tz.current_char_index_in_source()} {}

  [[nodiscard]] inline auto what() const noexcept -> const char * override {
    return msg.c_str();
  }
};
