#pragma once

#include <iostream>
#include <regex>

class token final {
public:
  inline token(string wb, size_t start_ix, string tk, size_t end_ix, string wa,
               bool is_str = false)
      : ws_left_{std::move(wb)}, start_ix_{start_ix}, name_{std::move(tk)},
        end_ix_{end_ix}, ws_right_{std::move(wa)}, is_str_{is_str} {}

  inline token() = default;
  inline token(const token &) = default;
  inline token(token &&) = default;
  inline auto operator=(const token &) -> token & = default;
  inline auto operator=(token &&) -> token & = default;

  inline ~token() = default;

  inline void source_to(ostream &os) const {
    if (not is_str_) {
      os << ws_left_ << name_ << ws_right_;
      return;
    }
    os << ws_left_ << '"' << regex_replace(name_, regex("\n"), "\\n") << '"'
       << ws_right_;
  }

  inline void compile_to(ostream &os) const {
    if (not is_str_) {
      os << name_;
      return;
    }
    // ? temporary fix
    os << regex_replace(name_, regex("\\\\n"), "',10,'");
  }

  [[nodiscard]] inline auto is_name(const string &s) const -> bool {
    return name_ == s;
  }

  [[nodiscard]] inline auto name() const -> const string & { return name_; }

  [[nodiscard]] inline auto char_index() const -> size_t { return start_ix_; }

  [[nodiscard]] inline auto char_index_end() const -> size_t { return end_ix_; }

  [[nodiscard]] inline auto is_empty() const -> bool {
    return ws_left_.empty() and name_.empty() and ws_right_.empty();
  }

  // inline bool is_empty()const{return name_.empty();}

  [[nodiscard]] inline auto total_length_in_chars() const -> size_t {
    return ws_left_.length() + name_.length() + ws_right_.length();
  }

  [[nodiscard]] inline auto is_string() const -> bool { return is_str_; }

private:
  string ws_left_{};
  size_t start_ix_{};
  string name_{};
  size_t end_ix_{};
  string ws_right_{};
  bool is_str_{};
};
