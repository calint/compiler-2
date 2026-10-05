#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstddef>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "token.hpp"

// 'tokenizer' throws this, so it is only declared here
class tokenizer;

class compiler_exception final : public std::exception {
  public:
    // a place the error was found through: an inlined call or the instance of
    // a generic definition
    struct call_frame {
        size_t line{};
        size_t start_index{};
        size_t end_index{};
        std::string text;
        std::string reason;
    };

    compiler_exception(const token& src_loc_tk, std::string message)
        : msg{std::move(message)}, line{src_loc_tk.at_line()},
          start_index{src_loc_tk.start_index()},
          end_index{src_loc_tk.end_index()} {

        assert(line != 0);
    }

    // defined in 'tokenizer.hpp', the error is at the cursor: the start of the
    // token that was found instead of the expected one, after the whitespace
    // that follows the last token read
    compiler_exception(const tokenizer& tz, std::string message);

    std::string msg;
    size_t line{};
    size_t start_index{};
    size_t end_index{};

    // innermost call first, each call inlined the one before it
    std::vector<call_frame> call_frames;

    // more text for the user after the call frames, e.g. the use of registers
    std::string detail;

    auto add_call_frame(const token& call_tk, std::string text,
                        std::string reason = "called from") -> void {

        call_frames.push_back({
            .line{call_tk.at_line()},
            .start_index{call_tk.start_index()},
            .end_index{call_tk.end_index()},
            .text{std::move(text)},
            .reason{std::move(reason)},
        });
    }

    //
    // statics
    //

    // a problem of the whole file, e.g. a missing 'main', has line 0 and no
    // position, every other exception points at the token that caused it
    [[nodiscard]] static auto file_level(std::string message)
        -> compiler_exception {

        return compiler_exception{std::move(message)};
    }

  private:
    explicit compiler_exception(std::string message)
        : msg{std::move(message)} {}
};
