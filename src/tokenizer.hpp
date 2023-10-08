#pragma once
#include "token.hpp"

class tokenizer final {
public:
  inline explicit tokenizer(string str)
      : src_{std::move(str)}, ptr_{src_.c_str()} {}

  inline tokenizer() = default;
  inline tokenizer(const tokenizer &) = default;
  inline tokenizer(tokenizer &&) = default;
  inline auto operator=(const tokenizer &) -> tokenizer & = default;
  inline auto operator=(tokenizer &&) -> tokenizer & = default;

  inline ~tokenizer() = default;

  [[nodiscard]] inline auto is_eos() const -> bool { return !last_char_; }

  inline auto next_token() -> token {
    const string &wspre{next_whitespace()};
    const size_t bgn{nchar_};
    if (is_next_char('"')) {
      string s;
      while (true) {
        if (is_next_char('"')) {
          const size_t end{nchar_};
          const string &wsaft{next_whitespace()};
          return token{wspre, bgn, s, end, wsaft, true};
        }
        s.push_back(next_char());
      }
    }
    const string &tkn{next_token_str()};
    const size_t end{nchar_};
    const string &wsaft{next_whitespace()};
    return {wspre, bgn, tkn, end, wsaft};
  }

  inline void put_back_token(const token &t) {
    // ? validate token is same as source
    seek(-off_t(t.total_length_in_chars()));
  }

  inline void put_back_char([[maybe_unused]] const char ch) {
    // ? validate char is same as source
    seek(-off_t(1));
  }

  inline auto next_whitespace_token() -> token {
    return {next_whitespace(), nchar_, "", nchar_, ""};
  }

  inline auto is_next_char(const char ch) -> bool {
    if (*ptr_ != ch) {
      return false;
    }
    next_char(); // return ignored
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
    nchar_ += size_t(ptr_ - bgn);
    return s;
  }

  inline auto next_char() -> char {
    assert(last_char_);
    last_char_ = *ptr_;
    ptr_++;
    nchar_++;
    return last_char_;
  }

  [[nodiscard]] inline auto current_char_index_in_source() const -> size_t {
    return nchar_;
  }

private:
  inline auto next_whitespace() -> string {
    if (is_eos()) {
      return "";
    }
    const size_t nchar_bm_{nchar_};
    while (true) {
      const char ch{next_char()};
      if (is_char_whitespace(ch)) {
        continue;
      }
      seek(-1);
      break;
    }
    const size_t len{nchar_ - nchar_bm_};
    return {ptr_ - len, len};
  }

  inline auto next_token_str() -> string {
    if (is_eos()) {
      return "";
    }
    const size_t nchar_bm_{nchar_};
    while (true) {
      const char ch{next_char()};
      if (is_char_whitespace(ch) || ch == 0 || ch == '(' || ch == ')' ||
          ch == '{' || ch == '}' || ch == '=' || ch == ',' || ch == ':' ||
          ch == ';' || ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
          ch == '%' || ch == '&' || ch == '|' || ch == '^' || ch == '<' ||
          ch == '>' || ch == '!') {
        seek(-1);
        break;
      }
    }
    const size_t len{nchar_ - nchar_bm_};
    return {ptr_ - len, len};
  }

  inline void seek(const off_t nch) {
    assert(ssize_t(src_.size()) >= (ssize_t(nchar_) + nch) and
           (ssize_t(nchar_) + nch) >= 0);
    ptr_ += nch;
    nchar_ = size_t(ssize_t(nchar_) + nch);
  }

  inline static auto is_char_whitespace(const char ch) -> bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
  }

  string src_;
  const char *ptr_{};
  size_t nchar_{};
  char last_char_{-1};
};
