#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
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

// the name of the default type, e.g. 'i64' on x86_64 and 'i32' on rv32i
inline constexpr std::string_view default_type_name{"int"};

// the boolean values
inline constexpr std::string_view true_value{"true"};
inline constexpr std::string_view false_value{"false"};

// a name that is a keyword would be read as the keyword
inline constexpr std::array<std::string_view, 21> keywords{
    "func", "noinline", "mut", "type", "dat",      "var",   "let",
    "foo",  "loop",     "if",  "else", "continue", "break", "return",
    "self", "and",      "or",  "not",  "true",     "false", "include",
};

// the value a built-in function call has: none for a statement, a count at
// the default type, or a 'bool'
enum class builtin_value : uint8_t { none, count, boolean };

// where a call of a built-in may stand: alone as a statement, in a value, or
// either, e.g. 'read' whose count may be dropped
enum class builtin_use : uint8_t { statement, value, either };

struct builtin_function {
    std::string_view name;
    builtin_value value{};
    builtin_use use{};
};

// a variable or function with the name of a built-in function would be read
// as a call of the built-in
inline constexpr std::array<builtin_function, 8> builtin_functions{
    builtin_function{
        .name{"array_copy"},
        .value{builtin_value::none},
        .use{builtin_use::statement},
    },
    builtin_function{
        .name{"array_length"},
        .value{builtin_value::count},
        .use{builtin_use::value},
    },
    builtin_function{
        .name{"arrays_equal"},
        .value{builtin_value::boolean},
        .use{builtin_use::value},
    },
    builtin_function{
        .name{"read"},
        .value{builtin_value::count},
        .use{builtin_use::either},
    },
    builtin_function{
        .name{"write"},
        .value{builtin_value::count},
        .use{builtin_use::either},
    },
    builtin_function{
        .name{"sector_read"},
        .value{builtin_value::none},
        .use{builtin_use::statement},
    },
    builtin_function{
        .name{"sector_write"},
        .value{builtin_value::none},
        .use{builtin_use::statement},
    },
    builtin_function{
        .name{"exit"},
        .value{builtin_value::none},
        .use{builtin_use::statement},
    },
};

// only a statement has no value
static_assert(std::ranges::all_of(builtin_functions,
                                  [](const builtin_function& fn) -> bool {
                                      return (fn.value ==
                                              builtin_value::none) ==
                                             (fn.use == builtin_use::statement);
                                  }));

// the built-in function named 'name', null for any other name
[[nodiscard]] constexpr auto find_builtin_function(const std::string_view name)
    -> const builtin_function* {

    for (const builtin_function& fn : builtin_functions) {
        if (fn.name == name) {
            return &fn;
        }
    }

    return nullptr;
}

} // namespace reserved_names

// a digit starts a number, a name starts with a letter or '_'
[[nodiscard]] inline auto starts_number(const std::string_view text) -> bool {
    return not text.empty() and text.front() >= '0' and text.front() <= '9';
}

// a word of the source with the whitespace and comments around it and its
// place (file, indexes, line), kept by the node that parsed it, so the source
// is written back from the tokens ('source_to') and an error points at one
// (README 7.2); an empty token has no place, e.g. one the compiler made
class token final {
    size_t file_ix_{};          // index of the source file of the token
    std::string_view ws_left_;  // whitespace left of token text
    size_t start_ix_{};         // token text start index in source
    std::string_view text_;     // token text
    size_t end_ix_{};           // token text end index in source
    std::string_view ws_right_; // whitespace right of token text
    size_t at_line_{};          // line number in the source
    bool is_str_{};             // true if the text is a string literal

    // what a backslash in string or character text starts, see 'escape_at'
    struct escape {
        // the characters after the backslash
        size_t size{};
        // empty for a continuation or an unsupported escape
        std::optional<char> decoded;
        bool is_continuation{};
    };

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

    // no location, e.g. a 'token{}' of something the compiler made; a token
    // the source may leave out is a 'std::optional<token>'
    [[nodiscard]] auto is_empty() const -> bool {
        return ws_left_.empty() and text_.empty() and ws_right_.empty();
    }

    // no word was read here: the source ends or a delimiter follows; the
    // string '""' is a token
    [[nodiscard]] auto is_none() const -> bool {
        return text_.empty() and not is_str_;
    }

    // a word that starts with a digit, e.g. '12' or '0x1f'
    [[nodiscard]] auto is_number() const -> bool {
        return is_word() and starts_number(text_);
    }

    [[nodiscard]] auto is_string() const -> bool { return is_str_; }

    // a string literal is never a keyword or a name, whatever it says
    [[nodiscard]] auto is_text(const std::string_view s) const -> bool {
        return not is_str_ and text_ == s;
    }

    // a name, a keyword or a number, not a string or no token
    [[nodiscard]] auto is_word() const -> bool {
        return not text_.empty() and not is_str_;
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

    // a marker where this token starts, e.g. the position of a node whose
    // whitespace its first token holds
    [[nodiscard]] auto start_position() const -> token {
        return position(file_index(), start_index(), at_line());
    }

    // the bytes of a string literal, its escapes decoded, e.g. '\n' is one
    // byte; a backslash before a line end is dropped with the line end, so
    // the string continues on the next line
    [[nodiscard]] auto string_bytes() const -> std::string {
        std::string bytes;

        for (size_t i{}; i < text_.size(); ++i) {
            if (text_.at(i) != '\\') {
                bytes += text_.at(i);
                continue;
            }

            // note: the tokenizer reads a character after every backslash and
            //       rejected an unsupported escape ('unsupported_escape')

            assert(i + 1 < text_.size());

            // a whole escape is consumed, so an escaped backslash is not
            // taken as a continuation
            const escape esc{escape_at(text_, i)};

            if (not esc.is_continuation) {
                assert(esc.decoded);

                bytes += *esc.decoded;
            }

            i += esc.size;
        }

        return bytes;
    }

    [[nodiscard]] auto string_size_bytes() const -> size_t {
        return string_bytes().size();
    }

    [[nodiscard]] auto text() const -> std::string_view { return text_; }

    // the first unsupported escape of a string token, as 'decode_escape'
    // rejects it, located at its backslash, e.g. '\q'
    [[nodiscard]] auto unsupported_escape() const -> std::optional<token> {
        size_t line{at_line_};
        for (size_t i{}; i < text_.size(); ++i) {
            if (text_.at(i) == '\n') {
                ++line;
                continue;
            }

            if (text_.at(i) != '\\') {
                continue;
            }

            const escape esc{escape_at(text_, i)};

            // the line end of a continuation is counted by the loop
            if (esc.is_continuation) {
                continue;
            }

            if (not esc.decoded) {
                const size_t backslash_index{start_ix_ + 1 + i};
                // note: +1 because the text starts after the opening quote

                return synthetic(file_ix_, text_.substr(i, 1 + esc.size),
                                 backslash_index, line);
            }

            i += esc.size;
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

    // the escape whose backslash is at 'backslash_index' of 'text': a line
    // continuation, a backslash before a line end, or a character escape,
    // decoded unless unsupported; 'size' counts the characters after the
    // backslash, e.g. 3 for '\x41' and 2 for a crlf continuation
    [[nodiscard]] static auto escape_at(const std::string_view text,
                                        const size_t backslash_index)
        -> escape {

        assert(text.at(backslash_index) == '\\');

        const std::string_view after{text.substr(backslash_index + 1)};
        // note: +1 because the escape follows the backslash

        if (after.starts_with('\n')) {
            return {
                .size{1},
                .decoded{},
                .is_continuation{true},
            };
        }

        if (after.starts_with("\r\n")) {
            return {
                .size{2},
                .decoded{},
                .is_continuation{true},
            };
        }

        const size_t size{escape_length(after)};

        return {
            .size{size},
            .decoded{decode_escape(after.substr(0, size))},
            .is_continuation{},
        };
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

    // an optional token of the syntax prints only when it was written, e.g.
    // the '(' of 'i8(x)' but not of 'i8'
    static auto source_to(std::ostream& os, const std::optional<token>& tk)
        -> void {

        if (tk) {
            tk->source_to(os);
        }
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

// an operator as read, its token with the whitespace before it and what it
// spells, e.g. the '<<' of 'a << 2'
template <class Op> struct spelled_operator {
    token tk;
    Op op;
};

// what joins the comparisons of a condition
enum class connective : uint8_t { logical_and, logical_or };

// the connective 'tk' spells, none for any other token
[[nodiscard]] inline auto connective_of(const token& tk)
    -> std::optional<connective> {

    if (tk.is_text("and")) {
        return connective::logical_and;
    }

    if (tk.is_text("or")) {
        return connective::logical_or;
    }

    return std::nullopt;
}
