#pragma once
// reviewed: 2025-09-28
// refactored: pointer-free implementation

#include <cassert>
#include <cctype>
#include <string>
#include <string_view>

#include "token.hpp"

class tokenizer final {
    const std::string_view delimiters_{" \t\r\n(){}[]=,.:+-*/%&|^<>!\0"};
    const std::string& src_;  // the string to be tokenized
    std::string_view src_sv_; // source as a string view
    size_t char_ix_{};        // current char index in 'src_'
    const char* pos{};        // position in string used for easier debugging
    size_t at_line_{1};

  public:
    explicit tokenizer(const std::string& src) : src_{src}, src_sv_{src_} {}

    tokenizer() = delete;
    tokenizer(const tokenizer&) = default;
    tokenizer(tokenizer&&) = default;
    auto operator=(const tokenizer&) -> tokenizer& = delete;
    auto operator=(tokenizer&&) -> tokenizer& = delete;

    ~tokenizer() = default;

    [[nodiscard]] auto is_eos() const -> bool {
        return char_ix_ >= src_sv_.size();
    }

    auto next_token() -> token {
        const std::string_view ws_before{next_whitespace()};
        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};
        if (is_next_char('"')) {
            // string token
            while (true) {
                if (is_next_char('"')) {
                    const size_t end_ix{char_ix_};
                    const std::string_view ws_after{next_whitespace()};
                    return token{
                        ws_before,
                        bgn_ix,
                        src_sv_.substr(bgn_ix + 1, end_ix - bgn_ix - 2),
                        end_ix,
                        ws_after,
                        at_line,
                        true};
                    // note: +1 and -2 does not include the leading and trailing
                    // quotation
                }
                (void)next_char();
            }
        }
        // not a string
        const std::string_view txt{next_token_str()};
        const size_t end_ix{char_ix_};
        const std::string_view ws_after{next_whitespace()};
        return {ws_before, bgn_ix, txt, end_ix, ws_after, at_line};
    }

    // returns a token which is a marker at current position with empty name
    // and whitespaces
    [[nodiscard]] auto current_position_token() const -> token {
        return {"", char_ix_, "", char_ix_, "", at_line_};
    }

    auto rewind_to_position(const token& pos_tk) -> void {
        const size_t new_pos{pos_tk.start_index()};

        assert(new_pos <= src_sv_.size());

        const size_t n{char_ix_ - new_pos};
        move_back(n);
    }

    auto put_back_token(const token& t) -> void {
        //? todo. validate token is same as source
        move_back(t.total_length_in_chars());
    }

    auto put_back_char(const char ch) -> void {
        assert(char_ix_ > 0 and src_sv_[char_ix_ - 1] == ch);
        move_back(1);
    }

    auto next_whitespace_token() -> token {
        const size_t at_line{at_line_};
        return {next_whitespace(), char_ix_, "", char_ix_, "", at_line};
    }

    auto is_next_char(const char ch) -> bool {
        if (is_eos() or src_sv_[char_ix_] != ch) {
            return false;
        }
        return next_char();
    }

    [[nodiscard]] auto is_peek_char(const char ch) const -> bool {
        return not is_eos() and src_sv_[char_ix_] == ch;
    }

    [[nodiscard]] auto is_peek_char2(const char ch) const -> bool {
        return char_ix_ + 1 < src_sv_.size() and src_sv_[char_ix_ + 1] == ch;
    }

    [[nodiscard]] auto peek_char() const -> char {
        return is_eos() ? '\0' : src_sv_[char_ix_];
    }

    auto read_rest_of_line() -> std::string_view {
        const size_t bgn{char_ix_};
        while (not is_eos()) {
            if (src_sv_[char_ix_] == '\n') {
                break;
            }
            char_ix_++;
        }
        const size_t len{char_ix_ - bgn};
        if (not is_eos()) {
            char_ix_++; // skip the '\n'
            at_line_++;
        }
        pos = &src_[char_ix_];
        return src_sv_.substr(bgn, len);
    }

    auto next_char() -> char {
        assert(not is_eos());
        // note: just for easier debugging
        const char ch{src_sv_[char_ix_++]};
        if (ch == '\n') {
            at_line_++;
        }
        pos = &src_sv_[char_ix_];
        return ch;
    }

    [[nodiscard]] auto current_char_index_in_source() const -> size_t {
        return char_ix_;
    }

    [[nodiscard]] auto current_line() const -> size_t { return at_line_; }

  private:
    auto next_whitespace() -> std::string_view {
        if (is_eos()) {
            return "";
        }
        const size_t bgn_ix{char_ix_};
        while (not is_eos()) {
            const char ch{src_sv_[char_ix_]};
            if (not std::isspace(ch)) {
                break;
            }
            if (ch == '\n') {
                at_line_++;
            }
            char_ix_++;
        }
        pos = &src_[char_ix_];
        const size_t len{char_ix_ - bgn_ix};
        return src_sv_.substr(bgn_ix, len);
    }

    auto next_token_str() -> std::string_view {
        if (is_eos()) {
            return "";
        }
        const size_t bgn_ix{char_ix_};
        while (not is_eos()) {
            const char ch{src_sv_[char_ix_]};
            if (delimiters_.contains(ch)) {
                break;
            }
            char_ix_++;
        }
        const size_t len{char_ix_ - bgn_ix};
        return src_sv_.substr(bgn_ix, len);
    }

    auto move_back(size_t nchars) -> void {
        assert(char_ix_ >= nchars);

        while (nchars--) {
            char_ix_--;
            if (src_sv_[char_ix_] == '\n') {
                at_line_--;
            }
        }

        pos = &src_sv_[char_ix_];
    }
};
