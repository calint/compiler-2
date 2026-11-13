#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "token.hpp"

// 'tokenizer' throws this, so it is only declared here
class tokenizer;

// an error of the user without a place in the source, e.g. a file that cannot
// be read; 'main' reports it as the user's, any other 'std::exception' as a
// defect of the compiler
class unlocated_error final : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

// a wrong command line option, found by the option parse in 'main', or by the
// backend and 'storage_layout' when it is wrong only with the target or the
// sources, e.g. a '--stack' larger than its memory
class option_error final : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

// where in the source: the file, the line and the bytes of a token
struct source_span {
    size_t file_ix{};
    size_t line{};
    size_t start_index{};
    size_t end_index{};

    //
    // statics
    //

    // 'src_loc_tk' has a place in the source, line 0 is a token without one
    [[nodiscard]] static auto at(const token& src_loc_tk) -> source_span {
        assert(src_loc_tk.at_line() != 0);

        return {
            .file_ix{src_loc_tk.file_index()},
            .line{src_loc_tk.at_line()},
            .start_index{src_loc_tk.start_index()},
            .end_index{src_loc_tk.end_index()},
        };
    }
};

// an error of the program at the token that caused it, with the inlined calls
// and generic instances it was found through ('call_frames', added while it
// unwinds); the first one ends the compile, no code recovers from it
// (README 1.9, 7.8)
class compile_error final : public std::exception {
  public:
    // a limit of the compile, not a rule of the program: running out of
    // scratch registers or of variable storage depends on the code compiled,
    // another version of the same code may not run out
    // ('machine::emit_most_efficient'), and they and the limit of inlined
    // calls depend on the arguments of a call; the readers, the only code
    // that changes an error instead of ending with it:
    //   1. 'machine::compile_available_version' drops a limit of a version
    //   2. 'inline_call::compile_within_limits' rewords 'out_of_registers'
    //      in an inlined recursion
    //   3. 'inline_call::check_recursion' drops a limit in a recursion of the
    //      check of a body that takes from a call, but not 'inlining_limit'
    //      in one that nests as deep at every call
    //   4. 'compile_toc::dry_run' rethrows an error as a 'source' one, so
    //      that none of the others drops it
    enum class error_kind : uint8_t {
        source,
        out_of_registers,
        out_of_storage,
        inlining_limit,
    };

    // a place the error was found through: an inlined call or the instance of
    // a generic definition
    struct call_frame {
        source_span where{};
        std::string text;
        std::string reason;

        //
        // statics
        //

        // 'text' found through 'src_loc_tk', e.g. a call or an instance
        [[nodiscard]] static auto at(const token& src_loc_tk, std::string text,
                                     std::string reason) -> call_frame {

            return {
                .where{source_span::at(src_loc_tk)},
                .text{std::move(text)},
                .reason{std::move(reason)},
            };
        }
    };

    // a token without a place gives an error without one like 'file_level'
    compile_error(const token& src_loc_tk, std::string message_in,
                  const error_kind kind_in = error_kind::source)
        : message{std::move(message_in)}, kind{kind_in} {

        if (src_loc_tk.at_line() != 0) {
            where = source_span::at(src_loc_tk);
        }
    }

    // defined in 'tokenizer.hpp', the error is at the start of the token that
    // was found instead of the expected one, past the whitespace and comments
    // after the last token read
    compile_error(const tokenizer& tz, std::string message_in);

    std::string message;
    // none for a problem of the whole file ('file_level')
    std::optional<source_span> where;

    // innermost first, see 'call_frame'
    std::vector<call_frame> call_frames;

    // more text for the user after the call frames, e.g. the use of registers
    std::string detail;

    error_kind kind{error_kind::source};

    // 'instance' names the generic instance that was called, empty for a
    // function written in the source, e.g. 'use<big>'
    auto add_call_frame(const token& call_tk, std::string text,
                        std::string_view instance) -> void;

    // see 'error_kind'
    [[nodiscard]] auto is_limit() const -> bool {
        return kind != error_kind::source;
    }

    //
    // statics
    //

    // a problem of the whole file, e.g. a missing 'main', has no place, every
    // other error points at the token that caused it
    [[nodiscard]] static auto file_level(std::string message) -> compile_error {
        return compile_error{token{}, std::move(message)};
    }
};

// a byte below ' ' or DEL, which would break a message or a path
[[nodiscard]] inline auto is_control_char(const char ch) -> bool {
    const unsigned char byte{static_cast<unsigned char>(ch)};
    // note: unsigned, the bytes of UTF-8 are above DEL

    return byte < ' ' or byte == '\x7f';
}

// a code point of 'continuation_count' continuation bytes is in their
// shortest form, no surrogate and not past U+10FFFF, as UTF-8 requires
[[nodiscard]] inline auto is_well_formed_utf8(const uint32_t code_point,
                                              const size_t continuation_count)
    -> bool {

    constexpr std::array<uint32_t, 4> shortest_from{0, 0x80, 0x800, 0x10000};

    // note: indexed by the continuation count, a code point below the entry
    //       has a shorter form

    constexpr uint32_t first_surrogate{0xd800};
    constexpr uint32_t last_surrogate{0xdfff};
    constexpr uint32_t last_code_point{0x10ffff};

    const bool is_surrogate{
        code_point >= first_surrogate and code_point <= last_surrogate,
    };

    return code_point >= shortest_from.at(continuation_count) and
           code_point <= last_code_point and not is_surrogate;
}

// a character that a terminal reads as a control or that looks like a space
// or like nothing is escaped as the control bytes are, e.g. U+009B starts a
// control sequence, the em space U+2003 makes 'var\xe2\x80\x83x' read
// 'var x', U+202E reverses the rest of the line
[[nodiscard]] inline auto is_escaped_code_point(const uint32_t code_point)
    -> bool {

    // first and last code point of each range
    constexpr std::array<std::pair<uint32_t, uint32_t>, 15> escaped{
        {
            {0x80, 0xa0},       // C1 controls, no-break space
            {0xad, 0xad},       // soft hyphen
            {0x61c, 0x61c},     // arabic letter mark
            {0x115f, 0x1160},   // hangul fillers
            {0x1680, 0x1680},   // ogham space mark
            {0x180e, 0x180e},   // mongolian vowel separator
            {0x2000, 0x200f},   // spaces, zero width, direction marks
            {0x2028, 0x202f},   // line separators, bidi, narrow no-break
            {0x205f, 0x2064},   // math space, word joiner, invisible operators
            {0x2066, 0x2069},   // bidi isolates
            {0x3000, 0x3000},   // ideographic space
            {0x3164, 0x3164},   // hangul filler
            {0xfeff, 0xfeff},   // zero width no-break space
            {0xffa0, 0xffa0},   // halfwidth hangul filler
            {0xe0000, 0xe007f}, // tags
        },
    };

    return std::ranges::any_of(
        escaped, [&](const std::pair<uint32_t, uint32_t>& range) -> bool {
            return code_point >= range.first and code_point <= range.second;
        });
}

// the bytes of the whole UTF-8 character at 'index' of 'text', 0 for a
// control character or a byte that starts no whole character, e.g. a lone
// continuation byte, a truncated or an overlong character
[[nodiscard]] inline auto shown_char_size(const std::string_view text,
                                          const size_t index) -> size_t {

    const char lead{text.at(index)};
    const unsigned char lead_byte{static_cast<unsigned char>(lead)};

    if (is_control_char(lead)) {
        return 0;
    }

    constexpr unsigned char first_non_ascii{0x80};

    if (lead_byte < first_non_ascii) {
        return 1;
    }

    const size_t continuation_count{token::utf8_continuation_count(lead)};

    if (continuation_count == 0 or index + continuation_count >= text.size()) {
        return 0;
    }

    constexpr uint32_t payload_bits{0x3f};
    constexpr uint32_t payload_shift{6};

    uint32_t code_point{lead_byte & (payload_bits >> continuation_count)};
    // note: a lead byte of 'n' continuations has '6 - n' payload bits

    for (size_t i{1}; i <= continuation_count; ++i) {
        const char continuation{text.at(index + i)};

        if (not token::is_utf8_continuation(continuation)) {
            return 0;
        }

        const uint32_t byte{static_cast<unsigned char>(continuation)};

        code_point = (code_point << payload_shift) | (byte & payload_bits);
    }

    if (not is_well_formed_utf8(code_point, continuation_count)) {
        return 0;
    }

    if (is_escaped_code_point(code_point)) {
        return 0;
    }

    return 1 + continuation_count;
}

// a control byte or a byte outside a whole UTF-8 character as '\xNN', so a
// message that quotes the source stays on its line and is valid UTF-8
[[nodiscard]] inline auto printable(const std::string_view text)
    -> std::string {

    std::string result;

    size_t i{};

    while (i < text.size()) {
        const size_t size{shown_char_size(text, i)};

        if (size == 0) {
            const unsigned char byte{static_cast<unsigned char>(text.at(i))};
            result += std::format("\\x{:02x}", byte);
            ++i;
            continue;
        }

        result += text.substr(i, size);
        i += size;
    }

    return result;
}

// a source text in a message, in quotes unless it holds a character literal,
// which brings its own, e.g. -'\xff', not a lone apostrophe, e.g. 'don't'; a
// message never puts quotes around source text itself
// ('qa/lint/check-messages.py')
[[nodiscard]] inline auto in_quotes(const std::string_view source)
    -> std::string {

    if (std::ranges::count(source, '\'') > 1) {
        return std::string{source};
    }

    return std::format("'{}'", source);
}

inline auto compile_error::add_call_frame(const token& call_tk,
                                          std::string text,
                                          const std::string_view instance)
    -> void {

    std::string reason{"called from"};

    if (not instance.empty()) {
        reason = std::format("{} called from", in_quotes(instance));
    }

    call_frames.push_back(
        call_frame::at(call_tk, std::move(text), std::move(reason)));
}

// a count with its noun for a message, e.g. '1 argument' or '2 arguments'
[[nodiscard]] inline auto counted(const size_t count,
                                  const std::string_view noun) -> std::string {

    return std::format("{} {}{}", count, noun, count == 1 ? "" : "s");
}

// e.g. "'f' takes 1 argument", "'sector_read' takes 2 or 3 arguments" or
// "'write' takes 2 to 4 arguments"
[[nodiscard]] inline auto argument_count_text(const std::string_view name,
                                              const size_t min_count,
                                              const size_t max_count)
    -> std::string {

    if (min_count == max_count) {
        return std::format("{} takes {}", in_quotes(name),
                           counted(min_count, "argument"));
    }

    const std::string_view conjunction{
        max_count == min_count + 1 ? "or" : "to",
    };

    return std::format("{} takes {} {} {} arguments", in_quotes(name),
                       min_count, conjunction, max_count);
}

// the last argument of a call is not followed by the ')', e.g. 'f(1 2)' of
// 'func f(x)'
inline constexpr std::string_view missing_close_paren_text{
    "expected ')' after the arguments",
};

// an array where an element is needed, e.g. the 'a' of 'a + 1'
[[noreturn]] inline auto throw_unindexed_array(const token& src_loc_tk,
                                               const std::string_view name)
    -> void {

    throw compile_error{
        src_loc_tk, std::format("array {} must be indexed", in_quotes(name))};
}

// a call of a function without a result where a value is needed, e.g. 'x =
// array_copy(a, b, 1)'
[[noreturn]] inline auto throw_no_value(const token& call_tk,
                                        const std::string_view name) -> void {

    throw compile_error{
        call_tk, std::format("function {} has no result", in_quotes(name))};
}

// the end of a message about a generic type used where an instance is needed
[[nodiscard]] inline auto instance_hint(const std::string_view name)
    -> std::string {

    return std::format("name an instance first, e.g. 'type my_{} = {}<...>'",
                       name, name);
}

// a generic type where an instance is needed, e.g. the 'box' of 'box(1)' of
// 'type box<T type> {x T}'; 'role' is what is needed, e.g. "a function"
[[noreturn]] inline auto throw_generic_type_not(const token& src_loc_tk,
                                                const std::string_view name,
                                                const std::string_view role)
    -> void {

    throw compile_error{src_loc_tk, std::format("generic type {} is not {}, {}",
                                                in_quotes(name), role,
                                                instance_hint(name))};
}

// a name called that names no function, e.g. the 'x' of 'x(1)' after 'var x =
// 1'; 'kind' is what it names, e.g. "value" or "constant"
[[noreturn]] inline auto throw_not_a_function(const token& src_loc_tk,
                                              const std::string_view kind,
                                              const std::string_view name)
    -> void {

    throw compile_error{src_loc_tk, std::format("{} is a {}, not a function",
                                                in_quotes(name), kind)};
}

// a value of the type 'got' where the type 'expected' is needed
[[noreturn]] inline auto throw_type_mismatch(const token& src_loc_tk,
                                             const std::string_view expected,
                                             const std::string_view got)
    -> void {

    throw compile_error{src_loc_tk,
                        std::format("expected type {}, got {}",
                                    in_quotes(expected), in_quotes(got))};
}

// parentheses around one plain value at 'open_paren_tk', e.g. '(x)', group
// nothing; 'value_source' is the value as written
[[noreturn]] inline auto
throw_parentheses_group_nothing(const token& open_paren_tk,
                                const std::string_view value_source) -> void {

    throw compile_error{
        open_paren_tk,
        std::format("parentheses around a single value group nothing, write {}",
                    in_quotes(value_source))};
}

// a condition where a number operand is needed, e.g. the '>' of 'int(a > 2)';
// 'maker' is what makes it, e.g. "a comparison" or "'not'"
[[noreturn]] inline auto
throw_condition_not_operand(const token& src_loc_tk,
                            const std::string_view maker) -> void {

    throw compile_error{src_loc_tk,
                        std::format("{} makes a condition, which is not an "
                                    "operand here, store it in a 'bool' first",
                                    maker)};
}

// a literal or a call result has no storage, e.g. 'literal' is "an array
// literal", "an instance literal of 'point'" or "'mk()'"
[[noreturn]] inline auto
throw_literal_not_operand(const token& tk, const std::string_view literal)
    -> void {

    throw compile_error{tk, std::format("{} is not an operand here, store it "
                                        "in a variable first",
                                        literal)};
}

// an instance literal of 'type_name' has no storage, e.g. 'point{1, 2}.x'
[[noreturn]] inline auto
throw_instance_literal_not_operand(const token& tk,
                                   const std::string_view type_name) -> void {

    throw_literal_not_operand(
        tk, std::format("an instance literal of {}", in_quotes(type_name)));
}

// a value named like a type hides the type, e.g. 'point' after
// 'var point = 1'
[[noreturn]] inline auto throw_value_hides_type(const token& tk,
                                                const std::string_view name)
    -> void {

    throw compile_error{tk, std::format("{} names a value here, which hides "
                                        "the type {}",
                                        in_quotes(name), in_quotes(name))};
}

// a 'not' where 'what' is expected, e.g. the 'not' of 'q = not b' for "an
// instance of 'pt'"
[[noreturn]] inline auto throw_not_makes_condition(const token& not_tk,
                                                   const std::string_view what)
    -> void {

    throw compile_error{not_tk,
                        std::format("'not' makes a condition, not {}", what)};
}

// a second definition of 'name' in its namespace, e.g. the second 'x' of
// 'type p { x i8, x i8 }'; 'kind' names it, e.g. "field", 'existing_where'
// is where the first one is ('parse_toc::where')
[[noreturn]] inline auto
throw_already_defined(const token& src_loc_tk, const std::string_view kind,
                      const std::string_view name,
                      const std::string_view existing_where) -> void {

    throw compile_error{src_loc_tk,
                        std::format("{} {} already defined at {}", kind,
                                    in_quotes(name), existing_where)};
}

// an enclosed list met 'next_tk', which is not its ')', e.g. the 'exit' of
// 'x = (1 + 2 exit(x)'
[[noreturn]] inline auto throw_unclosed_list(const token& next_tk) -> void {
    throw compile_error{next_tk, "expected ')' to close expression"};
}

// a keyword of 'reserved_names::definition_keywords' outside the global
// scope, e.g. the 'func' after a body without its '}'
[[noreturn]] inline auto throw_not_global(const token& keyword_tk) -> void {
    throw compile_error{keyword_tk,
                        std::format("{} can only be in global scope",
                                    in_quotes(keyword_tk.text()))};
}

// a keyword where a value or an array is expected, e.g. the 'else' of 'x =
// else'
[[noreturn]] inline auto throw_unexpected_keyword(const token& keyword_tk)
    -> void {

    throw compile_error{keyword_tk, std::format("unexpected keyword {}",
                                                in_quotes(keyword_tk.text()))};
}

// type arguments to a callee that takes none, e.g. the '<' of 'f<i8>(1)';
// 'kind' is how the message names the callee: "function", "method",
// "constructor" or "built-in function"
[[noreturn]] inline auto throw_not_generic(const token& src_loc_tk,
                                           const std::string_view kind,
                                           const std::string_view name)
    -> void {

    throw compile_error{
        src_loc_tk,
        std::format("{} {} is not generic, it takes no type arguments", kind,
                    in_quotes(name))};
}

// an instance or an array where a number is needed, e.g. the 'p' of 'p + 1';
// 'kind' is how the message names it: "an instance" or "an array"
[[noreturn]] inline auto throw_not_a_number(const token& src_loc_tk,
                                            const std::string_view kind,
                                            const std::string_view type_name)
    -> void {

    throw compile_error{src_loc_tk, std::format("{} of {} is not a number",
                                                kind, in_quotes(type_name))};
}
