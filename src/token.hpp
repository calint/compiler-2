#pragma once
// reviewed: 2025-09-28

#include <ostream>
#include <regex>
#include <string>

class token final {
    std::string_view ws_left_;  // whitespace left of token text
    size_t start_ix_{};         // token text start index in source
    std::string_view name_;     // token text
    size_t end_ix_{};           // token text end index in source
    std::string_view ws_right_; // whitespace right of token text
    size_t at_line_{};
    bool is_str_{}; // true if 'stmt_def_field' was a string

  public:
    token(std::string_view ws_left, size_t start_ix, std::string_view name,
          size_t end_ix, std::string_view ws_right, size_t at_line,
          bool is_str = false)
        : ws_left_{ws_left}, start_ix_{start_ix}, name_{name}, end_ix_{end_ix},
          ws_right_{ws_right}, at_line_(at_line), is_str_{is_str} {}

    token() = default;
    token(const token&) = default;
    token(token&&) = default;
    auto operator=(const token&) -> token& = default;
    auto operator=(token&&) -> token& = default;

    ~token() = default;

    auto source_to(std::ostream& os) const -> void {
        if (not is_str_) {
            os << ws_left_ << name_ << ws_right_;
            return;
        }
        std::string const name_str{name_};
        os << ws_left_ << '"'
           << std::regex_replace(name_str, std::regex("\n"), "\\n") << '"'
           << ws_right_;
    }

    auto compile_to(std::ostream& os) const -> void {
        if (not is_str_) {
            os << name_;
            return;
        }
        //? temporary fix to handle strings
        os << std::regex_replace(std::string{name_}, std::regex(R"(\\n)"),
                                 "',10,'");
    }

    [[nodiscard]] auto is_name(std::string_view s) const -> bool {
        return name_ == s;
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    [[nodiscard]] auto start_index() const -> size_t { return start_ix_; }

    [[nodiscard]] auto end_index() const -> size_t { return end_ix_; }

    [[nodiscard]] auto is_empty() const -> bool {
        return ws_left_.empty() and name_.empty() and ws_right_.empty();
    }

    [[nodiscard]] auto total_length_in_chars() const -> size_t {
        return ws_left_.length() + name_.length() + ws_right_.length();
    }

    [[nodiscard]] auto is_string() const -> bool { return is_str_; }

    [[nodiscard]] auto at_line() const -> size_t { return at_line_; }
};
