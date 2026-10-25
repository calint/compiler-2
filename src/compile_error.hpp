#pragma once

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

// a command line option that is wrong only with the target or the sources,
// e.g. a '--stack' larger than its memory; 'main' reports it like the errors
// of the option parse
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

class compile_error final : public std::exception {
  public:
    // running out of scratch registers depends on the code compiled, another
    // version of the same code may not run out
    // ('machine::emit_most_efficient'); both it and the limit of inlined calls
    // depend on the arguments of a call, so a function that nothing calls
    // cannot be judged by them; the readers: 'machine::compile_version' drops
    // 'out_of_registers', 'stmt_call' turns it into a recursion error in an
    // inlined recursion, 'stmt_def_func::check_body' drops both when the body
    // takes from a call; 'toc::throw_dry_run_error' resets the kind to
    // 'source' so that none of them drops the error of a dry run
    enum class error_kind : uint8_t {
        source,
        out_of_registers,
        inlining_limit,
    };

    // a place the error was found through: an inlined call or the instance of
    // a generic definition
    struct call_frame {
        source_span where;
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

    // a token without a place, e.g. of a backend test, gives an error without
    // one like 'file_level'; the compiler always passes a token of the source
    compile_error(const token& src_loc_tk, std::string message)
        : msg{std::move(message)} {

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

    auto add_call_frame(const token& call_tk, std::string text,
                        std::string reason = "called from") -> void {

        call_frames.push_back(
            call_frame::at(call_tk, std::move(text), std::move(reason)));
    }

    // whether the error can be told only with the arguments of a call, see
    // 'stmt_def_func::check_body'
    [[nodiscard]] auto depends_on_call() const -> bool {
        return kind != error_kind::source;
    }

    //
    // statics
    //

    // a problem of the whole file, e.g. a missing 'main', has no place, every
    // other error points at the token that caused it
    [[nodiscard]] static auto file_level(std::string message) -> compile_error {
        return compile_error{std::move(message)};
    }

  private:
    explicit compile_error(std::string message) : msg{std::move(message)} {}
};

// a byte below ' ' or DEL, which would break a message or a path
[[nodiscard]] inline auto is_control_char(const char ch) -> bool {
    const unsigned char byte{static_cast<unsigned char>(ch)};
    // note: unsigned, the bytes of UTF-8 are above DEL

    return byte < ' ' or byte == '\x7f';
}

// a control byte as '\xNN', so a message that quotes the source stays on its
// line
[[nodiscard]] inline auto printable(const std::string_view text)
    -> std::string {

    std::string result;

    for (const char ch : text) {
        if (is_control_char(ch)) {
            result += std::format("\\x{:02x}", static_cast<unsigned char>(ch));
            continue;
        }

        result.push_back(ch);
    }

    return result;
}

// a source text in a message, in quotes unless it holds a character literal,
// which brings its own, e.g. -'\xff'; a message never puts quotes around
// source text itself ('qa/lint/check-messages.py')
[[nodiscard]] inline auto in_quotes(const std::string_view source)
    -> std::string {

    if (source.contains('\'')) {
        return std::string{source};
    }

    return std::format("'{}'", source);
}

// the token as the source writes it, a string with its double quotes
[[nodiscard]] inline auto in_quotes(const token& tk) -> std::string {
    if (tk.is_string()) {
        return in_quotes(std::format("\"{}\"", tk.text()));
    }

    return in_quotes(tk.text());
}

// a count with its noun for a message, e.g. '1 argument' or '2 arguments'
[[nodiscard]] inline auto counted(const size_t count,
                                  const std::string_view noun) -> std::string {

    return std::format("{} {}{}", count, noun, count == 1 ? "" : "s");
}
