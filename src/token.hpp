#pragma once
// reviewed: 2025-09-28

#include <charconv>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <system_error>

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
          const bool is_str)
        : ws_left_{ws_left}, start_ix_{start_ix}, text_{name}, end_ix_{end_ix},
          ws_right_{ws_right}, at_line_{at_line}, is_str_{is_str} {}

    token() = default;

    auto source_to(std::ostream& os) const -> void {
        if (not is_str_) {
            std::print(os, "{}{}{}", ws_left_, text_, ws_right_);

            return;
        }

        std::print(os, "{}\"{}\"{}", ws_left_, text_, ws_right_);
    }

    [[nodiscard]] auto is_text(const std::string_view s) const -> bool {
        return text_ == s;
    }

    [[nodiscard]] auto text() const -> std::string_view { return text_; }

    [[nodiscard]] auto start_index() const -> size_t { return start_ix_; }

    [[nodiscard]] auto end_index() const -> size_t { return end_ix_; }

    [[nodiscard]] auto is_empty() const -> bool {
        return ws_left_.empty() and text_.empty() and ws_right_.empty();
    }

    [[nodiscard]] auto has_whitespace_before() const -> bool {
        return not ws_left_.empty();
    }

    [[nodiscard]] auto total_length_in_chars() const -> size_t {
        return ws_left_.length() + text_.length() + ws_right_.length();
    }

    [[nodiscard]] auto is_string() const -> bool { return is_str_; }

    [[nodiscard]] auto at_line() const -> size_t { return at_line_; }

    [[nodiscard]] auto string_size_bytes() const -> size_t {
        size_t len{};
        const std::string_view s{text_};
        for (size_t i{}; i < s.size(); ++i, ++len) {
            if (s[i] == '\\' and i + 1 < s.size()) {
                if (s[i + 1] == 'x' and i + 3 < s.size()) {
                    i += 3; // skip \xHH
                } else {
                    ++i; // skip 2-character escape sequence
                }
            }
        }

        return len;
    }

    // 'escape' is the text after the backslash, e.g. "n" or "x41"
    // shared by string data and character literals so both accept the same
    // escapes
    [[nodiscard]] static auto decode_escape(const std::string_view escape)
        -> std::optional<char> {

        if (escape.starts_with('x')) {
            return decode_hex_escape(escape.substr(1));
        }

        if (escape.size() != 1) {
            return std::nullopt;
        }

        switch (escape[0]) {
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
            return escape[0];

        default:
            return std::nullopt;
        }
    }

    // the bytes of string text such as "a\n", empty at an unsupported escape
    [[nodiscard]] static auto decode_string(const std::string_view text)
        -> std::optional<std::string> {

        std::string bytes;
        for (size_t i{}; i < text.size(); ++i) {
            if (text[i] != '\\') {
                bytes += text[i];
                continue;
            }

            // a hex escape spans the 'x' and two digits
            const size_t escape_size{text.substr(i + 1).starts_with('x') ? 3UZ
                                                                         : 1UZ};

            const std::optional<char> decoded{
                decode_escape(text.substr(i + 1, escape_size))};

            if (not decoded) {
                return std::nullopt;
            }

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

  private:
    [[nodiscard]] static auto decode_hex_escape(const std::string_view digits)
        -> std::optional<char> {

        if (digits.size() != 2) {
            return std::nullopt;
        }

        unsigned int decoded{};
        const char* const end{std::to_address(digits.end())};
        const std::from_chars_result parsed{
            std::from_chars(std::to_address(digits.begin()), end, decoded, 16)};

        if (parsed.ec != std::errc{} or parsed.ptr != end) {
            return std::nullopt;
        }

        return static_cast<char>(decoded);
    }
};
