#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
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

// a command line option that the target rejects, e.g. a '--stack' larger than
// its memory; 'main' reports it like the errors of the option parse
class option_error final : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

class compile_error final : public std::exception {
  public:
    // running out of scratch registers depends on the code compiled, another
    // version of the same code may not run out
    // ('machine::emit_most_efficient'); both it and the limit of inlined calls
    // depend on the arguments of a call, so a function that nothing calls
    // cannot be judged by them
    enum class error_kind : uint8_t {
        source,
        out_of_registers,
        inlining_limit,
    };

    // a place the error was found through: an inlined call or the instance of
    // a generic definition
    struct call_frame {
        size_t file_ix{};
        size_t line{};
        size_t start_index{};
        size_t end_index{};
        std::string text;
        std::string reason;
    };

    // a token without a place in the source, line 0, reports like a problem of
    // the whole file
    compile_error(const token& src_loc_tk, std::string message)
        : msg{std::move(message)}, file_ix{src_loc_tk.file_index()},
          line{src_loc_tk.at_line()}, start_index{src_loc_tk.start_index()},
          end_index{src_loc_tk.end_index()} {}

    // defined in 'tokenizer.hpp', the error is at the start of the token that
    // was found instead of the expected one, past the whitespace and comments
    // after the last token read
    compile_error(const tokenizer& tz, std::string message);

    std::string msg;
    size_t file_ix{};
    size_t line{};
    size_t start_index{};
    size_t end_index{};

    // innermost call first, each call inlined the one before it
    std::vector<call_frame> call_frames;

    // more text for the user after the call frames, e.g. the use of registers
    std::string detail;

    error_kind kind{error_kind::source};

    auto add_call_frame(const token& call_tk, std::string text,
                        std::string reason = "called from") -> void {

        call_frames.push_back({
            .file_ix{call_tk.file_index()},
            .line{call_tk.at_line()},
            .start_index{call_tk.start_index()},
            .end_index{call_tk.end_index()},
            .text{std::move(text)},
            .reason{std::move(reason)},
        });
    }

    // whether the error can be told only with the arguments of a call, see
    // 'stmt_def_func::check_body'
    [[nodiscard]] auto depends_on_call() const -> bool {
        return kind != error_kind::source;
    }

    //
    // statics
    //

    // a problem of the whole file, e.g. a missing 'main', has line 0 and no
    // position, every other exception points at the token that caused it
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

// a count with its noun for a message, e.g. '1 argument' or '2 arguments'
[[nodiscard]] inline auto counted(const size_t count,
                                  const std::string_view noun) -> std::string {

    return std::format("{} {}{}", count, noun, count == 1 ? "" : "s");
}
