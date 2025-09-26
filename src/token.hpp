#pragma once

#include <ostream>
#include <regex>
#include <string>

class token final {
    std::string ws_left_;  // whitespace left of token text
    size_t start_ix_{};    // token text start index in source
    std::string name_;     // token text
    size_t end_ix_{};      // token text end index in source
    std::string ws_right_; // whitespace right of token text
    bool is_str_{};        // true if 'stmt_def_field' was a string

  public:
    token(std::string ws_left, size_t start_ix, std::string name, size_t end_ix,
          std::string ws_right, bool is_str = false)
        : ws_left_{std::move(ws_left)}, start_ix_{start_ix},
          name_{std::move(name)}, end_ix_{end_ix},
          ws_right_{std::move(ws_right)}, is_str_{is_str} {}

    token() = default;
    token(const token&) = default;
    token(token&&) = default;
    auto operator=(const token&) -> token& = default;
    auto operator=(token&&) -> token& = default;

    ~token() = default;

    void source_to(std::ostream& os) const {
        if (not is_str_) {
            os << ws_left_ << name_ << ws_right_;
            return;
        }
        os << ws_left_ << '"' << regex_replace(name_, std::regex("\n"), "\\n")
           << '"' << ws_right_;
    }

    void compile_to(std::ostream& os) const {
        if (not is_str_) {
            os << name_;
            return;
        }
        //? temporary fix to handle strings
        os << regex_replace(name_, std::regex(R"(\\n)"), "',10,'");
    }

    [[nodiscard]] auto is_name(const std::string& s) const -> bool {
        return name_ == s;
    }

    [[nodiscard]] auto name() const -> const std::string& { return name_; }

    [[nodiscard]] auto start_index() const -> size_t { return start_ix_; }

    [[nodiscard]] auto end_index() const -> size_t { return end_ix_; }

    [[nodiscard]] auto is_empty() const -> bool {
        return ws_left_.empty() and name_.empty() and ws_right_.empty();
    }

    [[nodiscard]] auto total_length_in_chars() const -> size_t {
        return ws_left_.length() + name_.length() + ws_right_.length();
    }

    [[nodiscard]] auto is_string() const -> bool { return is_str_; }
};
