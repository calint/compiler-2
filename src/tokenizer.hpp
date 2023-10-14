#pragma once
#include "token.hpp"

class tokenizer final {
  const string &src_{}; // the string to be tokenized
  const char *ptr_{};   // pointer to current position
  size_t char_ix_{};    // current char index in 'src_'
  char last_char_{-1};  // last read character (-1 before any characters read)

public:
  inline explicit tokenizer(const string &src)
      : src_{src}, ptr_{src_.c_str()} {}

  inline tokenizer() = delete;
  inline tokenizer(const tokenizer &) = delete;
  inline tokenizer(tokenizer &&) = delete;
  inline auto operator=(const tokenizer &) -> tokenizer & = delete;
  inline auto operator=(tokenizer &&) -> tokenizer & = delete;

  inline ~tokenizer() = default;

  [[nodiscard]] inline auto is_eos() const -> bool {
    return last_char_ == '\0';
  }

  inline auto next_token() -> token {
    const string &ws_before{next_whitespace()};
    const size_t bgn_ix{char_ix_};
    if (is_next_char('"')) {
      // string token
      string txt;
      while (true) {
        if (is_next_char('"')) {
          const size_t end{char_ix_};
          const string &ws_after{next_whitespace()};
          return token{ws_before, bgn_ix, txt, end, ws_after, true};
        }
        txt.push_back(next_char());
      }
    }
    // not a string
    const string &txt{next_token_str()};
    const size_t end_ix{char_ix_};
    const string &ws_after{next_whitespace()};
    return {ws_before, bgn_ix, txt, end_ix, ws_after};
  }

  inline void put_back_token(const token &t) {
    //? validate token is same as source
    move_back(t.total_length_in_chars());
  }

  inline void put_back_char(const char ch) {
    //? validate char is same as source
    move_back(sizeof(ch));
  }

  inline auto next_whitespace_token() -> token {
    return {next_whitespace(), char_ix_, "", char_ix_, ""};
  }

  inline auto is_next_char(const char ch) -> bool {
    if (*ptr_ != ch) {
      return false;
    }
    (void)next_char(); // return ignored
    return true;
  }

  [[nodiscard]] inline auto is_peek_char(const char ch) const -> bool {
    return *ptr_ == ch;
  }

  [[nodiscard]] inline auto is_peek_char2(const char ch) const -> bool {
    return *ptr_ == 0 ? false : *(ptr_ + 1) == ch;
  }

  [[nodiscard]] inline auto peek_char() const -> char { return *ptr_; }

  inline auto read_rest_of_line() -> string {
    const char *bgn{ptr_};
    while (true) {
      if (*ptr_ == '\n') {
        break;
      }
      if (*ptr_ == '\0') {
        break;
      }
      ptr_++;
    }
    const string &s{bgn, size_t(ptr_ - bgn)};
    ptr_++;
    char_ix_ += size_t(ptr_ - bgn);
    return s;
  }

  inline auto next_char() -> char {
    assert(last_char_);
    last_char_ = *ptr_;
    ptr_++;
    char_ix_++;
    return last_char_;
  }

  [[nodiscard]] inline auto current_char_index_in_source() const -> size_t {
    return char_ix_;
  }

private:
  inline auto next_whitespace() -> string {
    if (is_eos()) {
      return "";
    }
    const size_t bgn_ix{char_ix_};
    while (true) {
      const char ch{next_char()};
      if (is_char_whitespace(ch)) {
        continue;
      }
      move_back(1);
      break;
    }
    const size_t len{char_ix_ - bgn_ix};
    return {ptr_ - len, len};
  }

  inline auto next_token_str() -> string {
    if (is_eos()) {
      return "";
    }
    const size_t bgn_ix{char_ix_};
    while (true) {
      const char ch{next_char()};
      if (is_char_whitespace(ch) or ch == '\0' or ch == '(' or ch == ')' or
          ch == '{' or ch == '}' or ch == '=' or ch == ',' or ch == ':' or
          ch == ';' or ch == '+' or ch == '-' or ch == '*' or ch == '/' or
          ch == '%' or ch == '&' or ch == '|' or ch == '^' or ch == '<' or
          ch == '>' or ch == '!') {
        move_back(1);
        break;
      }
    }
    const size_t len{char_ix_ - bgn_ix};
    return {ptr_ - len, len};
  }

  inline void move_back(const size_t nchars) {
    assert(char_ix_ >= nchars);

    ptr_ -= nchars;
    char_ix_ = size_t(char_ix_ - nchars);
  }

  inline static auto is_char_whitespace(const char ch) -> bool {
    return ch == ' ' or ch == '\t' or ch == '\r' or ch == '\n';
  }
};
