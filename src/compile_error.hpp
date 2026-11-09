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
// backend and 'toc' when it is wrong only with the target or the sources, e.g.
// a '--stack' larger than its memory
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
    //   4. 'toc::dry_run' rethrows an error as a 'source' one, so that none
    //      of the others drops it
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
    compile_error(const token& src_loc_tk, std::string message,
                  const error_kind kind_in = error_kind::source)
        : msg{std::move(message)}, kind{kind_in} {

        if (src_loc_tk.at_line() != 0) {
            where = source_span::at(src_loc_tk);
        }
    }

    // defined in 'tokenizer.hpp', the error is at the start of the token that
    // was found instead of the expected one, past the whitespace and comments
    // after the last token read
    compile_error(const tokenizer& tz, std::string message);

    std::string msg;
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

    // the C1 controls, e.g. U+009B that a terminal reads as the start of a
    // control sequence, are escaped as the control bytes are
    constexpr uint32_t first_c1_control{0x80};
    constexpr uint32_t last_c1_control{0x9f};

    if (code_point >= first_c1_control and code_point <= last_c1_control) {
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

// an enclosed list met 'next_tk', which is not its ')', e.g. the 'exit' of
// 'x = (1 + 2 exit(x)'
[[noreturn]] inline auto throw_unclosed_list(const token& next_tk) -> void {
    throw compile_error{next_tk, "expected ')' to close expression"};
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

// a user type where a number is needed, e.g. the 'p' of 'p + 1'
[[noreturn]] inline auto throw_not_a_number(const token& src_loc_tk,
                                            const std::string_view type_name)
    -> void {

    throw compile_error{
        src_loc_tk,
        std::format("an instance of {} is not a number", in_quotes(type_name))};
}
