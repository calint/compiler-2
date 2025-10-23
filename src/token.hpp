#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <ostream>
#include <print>
#include <regex>
#include <string>
#include <string_view>

class token final {
    std::string_view ws_left_;  // whitespace left of token text
    size_t start_ix_{};         // token text start index in source
    std::string_view text_;     // token text
    size_t end_ix_{};           // token text end index in source
    std::string_view ws_right_; // whitespace right of token text
    size_t at_line_{};          // line number in the source
    bool is_str_{};             // true if 'stmt_def_field' was a string

  public:
    token(const std::string_view ws_left, const size_t start_ix,
          const std::string_view name, const size_t end_ix,
          const std::string_view ws_right, const size_t at_line,
          const bool is_str = false)
        : ws_left_{ws_left}, start_ix_{start_ix}, text_{name}, end_ix_{end_ix},
          ws_right_{ws_right}, at_line_(at_line), is_str_{is_str} {}

    ~token() = default;

    token() = default;
    token(const token&) = default;
    token(token&&) = default;
    auto operator=(const token&) -> token& = default;
    auto operator=(token&&) -> token& = default;

    auto source_to(std::ostream& os) const -> void {
        if (not is_str_) {
            std::print(os, "{}{}{}", ws_left_, text_, ws_right_);
            return;
        }
        const std::string name_str{text_};
        std::print(os, "{}\"{}\"{}", ws_left_,
                   std::regex_replace(name_str, std::regex(R"(\n)"), "\\n"),
                   ws_right_);
    }

    auto compile_to(std::ostream& os) const -> void {
        if (not is_str_) {
            std::print(os, "{}", text_);
            return;
        }
        //? temporary fix to handle strings
        // NASM encoding of data as string
        std::string str{text_};
        str = std::regex_replace(str, std::regex(R"(')"), "', \"'\", '");
        str = std::regex_replace(str, std::regex(R"(\\")"), "\"");
        str = std::regex_replace(str, std::regex(R"(\\n)"), "', 10,'");
        std::print(os, "{}", str);
    }

    [[nodiscard]] auto is_text(std::string_view s) const -> bool {
        return text_ == s;
    }

    [[nodiscard]] auto text() const -> std::string_view { return text_; }

    [[nodiscard]] auto start_index() const -> size_t { return start_ix_; }

    [[nodiscard]] auto end_index() const -> size_t { return end_ix_; }

    [[nodiscard]] auto is_empty() const -> bool {
        return ws_left_.empty() and text_.empty() and ws_right_.empty();
    }

    [[nodiscard]] auto total_length_in_chars() const -> size_t {
        return ws_left_.length() + text_.length() + ws_right_.length();
    }

    [[nodiscard]] auto is_string() const -> bool { return is_str_; }

    [[nodiscard]] auto at_line() const -> size_t { return at_line_; }
};
