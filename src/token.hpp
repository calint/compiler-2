#pragma once

class token final {
  string ws_left_{};  // whitespace left of token text
  size_t start_ix_{}; // token text start index in source
  string name_{};     // token text
  size_t end_ix_{};   // token text end index in source
  string ws_right_{}; // whitespace right of token text
  bool is_str_{};     // true if 'stmt_def_field' was a string

public:
  inline token(string ws_left, size_t start_ix, string name, size_t end_ix,
               string ws_right, bool is_str = false)
      : ws_left_{move(ws_left)}, start_ix_{start_ix}, name_{move(name)},
        end_ix_{end_ix}, ws_right_{move(ws_right)}, is_str_{is_str} {}

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
    //? temporary fix to handle strings
    os << regex_replace(name_, regex(R"(\\n)"), "',10,'");
  }

  [[nodiscard]] inline auto is_name(const string &s) const -> bool {
    return name_ == s;
  }

  [[nodiscard]] inline auto name() const -> const string & { return name_; }

  [[nodiscard]] inline auto start_index() const -> size_t { return start_ix_; }

  [[nodiscard]] inline auto end_index() const -> size_t { return end_ix_; }

  [[nodiscard]] inline auto is_empty() const -> bool {
    return ws_left_.empty() and name_.empty() and ws_right_.empty();
  }

  // inline bool is_empty()const{return name_.empty();}

  [[nodiscard]] inline auto total_length_in_chars() const -> size_t {
    return ws_left_.length() + name_.length() + ws_right_.length();
  }

  [[nodiscard]] inline auto is_string() const -> bool { return is_str_; }
};
