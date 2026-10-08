#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <charconv>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <system_error>

// the names that have a meaning for the compiler, a program cannot use them
// for what it defines
namespace reserved_names {

// the receiver of a method and the value built by a constructor
inline constexpr std::string_view self{"self"};

// the iterator over elements
inline constexpr std::string_view foo{"foo"};

// the function the program starts in
inline constexpr std::string_view main{"main"};

// the boolean values
inline constexpr std::string_view true_value{"true"};
inline constexpr std::string_view false_value{"false"};

} // namespace reserved_names

class token final {
    size_t file_ix_{};          // index of the source file of the token
    std::string_view ws_left_;  // whitespace left of token text
    size_t start_ix_{};         // token text start index in source
    std::string_view text_;     // token text
    size_t end_ix_{};           // token text end index in source
    std::string_view ws_right_; // whitespace right of token text
    size_t at_line_{};          // line number in the source
    bool is_str_{};             // true if the text is a string literal

  public:
    token(const size_t file_ix, const std::string_view ws_left,
          const size_t start_ix, const std::string_view name,
          const size_t end_ix, const std::string_view ws_right,
          const size_t at_line, const bool is_str)
        : file_ix_{file_ix}, ws_left_{ws_left}, start_ix_{start_ix},
          text_{name}, end_ix_{end_ix}, ws_right_{ws_right}, at_line_{at_line},
          is_str_{is_str} {}

    token() = default;

    [[nodiscard]] auto at_line() const -> size_t { return at_line_; }

    [[nodiscard]] auto end_index() const -> size_t { return end_ix_; }

    [[nodiscard]] auto file_index() const -> size_t { return file_ix_; }

    [[nodiscard]] auto has_whitespace_before() const -> bool {
        return not ws_left_.empty();
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return ws_left_.empty() and text_.empty() and ws_right_.empty();
    }

    [[nodiscard]] auto is_string() const -> bool { return is_str_; }

    // a string literal is never a keyword or a name, whatever it says
    [[nodiscard]] auto is_text(const std::string_view s) const -> bool {
        return not is_str_ and text_ == s;
    }

    // the indexes include the quotes that string token text excludes
    [[nodiscard]] auto source_begin_index() const -> size_t {
        return start_ix_ - ws_left_.length();
    }

    [[nodiscard]] auto source_end_index() const -> size_t {
        return end_ix_ + ws_right_.length();
    }

    auto source_to(std::ostream& os) const -> void {
        if (not is_str_) {
            std::print(os, "{}{}{}", ws_left_, text_, ws_right_);
            return;
        }

        std::print(os, "{}\"{}\"{}", ws_left_, text_, ws_right_);
    }

    [[nodiscard]] auto start_index() const -> size_t { return start_ix_; }

    [[nodiscard]] auto string_size_bytes() const -> size_t {
        return decode_string(string_text()).size();
    }

    // string text with each backslash before a line end removed together
    // with the line end, so the string continues on the next line
    [[nodiscard]] auto string_text() const -> std::string {
        std::string joined;
        joined.reserve(text_.size());
        for (size_t i{}; i < text_.size(); ++i) {
            if (text_.at(i) != '\\' or i + 1 >= text_.size()) {
                // note: +1 because a backslash needs a character after it

                joined += text_.at(i);
                continue;
            }

            if (text_.at(i + 1) == '\n') {
                i += 1;
                // note: 1 skips the backslash, the loop skips the line end

                continue;
            }

            // crlf line ends
            if (text_.substr(i + 1).starts_with("\r\n")) {
                i += 2;
                // note: 2 skips '\' and '\r', the loop skips the '\n'

                continue;
            }

            // keep the pair so an escaped backslash is not taken as a
            // continuation
            joined += text_.at(i);
            joined += text_.at(i + 1);
            i += 1;
            // note: 1 skips the backslash, the loop skips the escaped char
        }

        return joined;
    }

    [[nodiscard]] auto text() const -> std::string_view { return text_; }

    // the marker at the first unsupported escape of a string token, as
    // 'decode_string' rejects it
    [[nodiscard]] auto unsupported_escape_position() const
        -> std::optional<token> {

        size_t line{at_line_};
        for (size_t i{}; i < text_.size(); ++i) {
            if (text_.at(i) == '\n') {
                ++line;
                continue;
            }

            if (text_.at(i) != '\\') {
                continue;
            }

            const std::string_view escape{text_.substr(i + 1)};
            // note: +1 because the escape follows the backslash

            // a backslash before a line end continues the string
            if (escape.starts_with('\n') or escape.starts_with("\r\n")) {
                continue;
            }

            const size_t escape_size{escape_length(escape)};

            if (not decode_escape(escape.substr(0, escape_size))) {
                const size_t backslash_index{start_ix_ + 1 + i};
                // note: +1 because the text starts after the opening quote

                return position(file_ix_, backslash_index, line);
            }

            i += escape_size;
        }

        return std::nullopt;
    }

    //
    // statics
    //

    // 'escape' is the text after the backslash, e.g. "n" or "x41"
    // shared by string data and character literals so both accept the same
    // escapes
    [[nodiscard]] static auto decode_escape(const std::string_view escape)
        -> std::optional<char> {

        if (escape.starts_with('x')) {
            return decode_hex_escape(escape.substr(1));
            // note: 1 because the digits follow the 'x'
        }

        if (escape.size() != 1) {
            return std::nullopt;
        }

        switch (escape.at(0)) {
        case '0':
            return '\0';

        case 'a':
            return '\a';

        case 'b':
            return '\b';

        case 't':
            return '\t';

        case 'n':
            return '\n';

        case 'v':
            return '\v';

        case 'f':
            return '\f';

        case 'r':
            return '\r';

        case 'e':
            return '\x1b';

        case '\\':
        case '\'':
        case '"':
        case '`':
            return escape.at(0);

        default:
            return std::nullopt;
        }
    }

    // the bytes of string text such as "a\n", whose escapes the tokenizer
    // has checked
    [[nodiscard]] static auto decode_string(const std::string_view text)
        -> std::string {

        std::string bytes;
        for (size_t i{}; i < text.size(); ++i) {
            if (text.at(i) != '\\') {
                bytes += text.at(i);
                continue;
            }

            const size_t escape_size{escape_length(text.substr(i + 1))};

            const std::optional<char> decoded{
                decode_escape(text.substr(i + 1, escape_size)),
            };

            assert(decoded);

            bytes += *decoded;
            i += escape_size;
        }

        return bytes;
    }

    // string text that 'decode_string' turns back into 'bytes', bytes that
    // are not printable are written as hex escapes
    [[nodiscard]] static auto encode_string(const std::string_view bytes)
        -> std::string {

        std::string text;
        for (const char c : bytes) {
            if (c == '\\' or c == '"') {
                text += '\\';
                text += c;
                continue;
            }

            if (c >= ' ' and c <= '~') {
                text += c;
                continue;
            }

            text += std::format("\\x{:02x}", static_cast<unsigned char>(c));
        }

        return text;
    }

    // the characters after a backslash that make up its escape: a hex escape
    // spans the 'x' and two digits, any other one character
    [[nodiscard]] static auto escape_length(const std::string_view escape)
        -> size_t {

        return escape.starts_with('x') ? 3UZ : 1UZ;
    }

    // a marker at 'index' with empty text and whitespace
    [[nodiscard]] static auto position(const size_t file_ix, const size_t index,
                                       const size_t line) -> token {

        return synthetic(file_ix, "", index, line);
    }

    // a token the compiler makes, located at 'index' in the source, without
    // whitespace
    [[nodiscard]] static auto synthetic(const size_t file_ix,
                                        const std::string_view text,
                                        const size_t index, const size_t line)
        -> token {

        return {file_ix, "", index, text, index, "", line, false};
    }

  private:
    //
    // statics
    //

    [[nodiscard]] static auto decode_hex_escape(const std::string_view digits)
        -> std::optional<char> {

        if (digits.size() != 2) {
            return std::nullopt;
        }

        unsigned int decoded{};
        const char* const end{std::to_address(digits.end())};

        const std::from_chars_result parsed{
            std::from_chars(std::to_address(digits.begin()), end, decoded, 16),
        };

        if (parsed.ec != std::errc{} or parsed.ptr != end) {
            return std::nullopt;
        }

        return static_cast<char>(decoded);
    }
};
