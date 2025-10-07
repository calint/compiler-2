#pragma once
// reviewed: 2025-09-28
// refactored: pointer-free implementation

#include <cassert>
#include <cctype>
#include <string>

#include "token.hpp"

class tokenizer final {
    const std::string_view delimiters_{" \t\r\n(){}[]=,.:+-*/%&|^<>!\0"};
    const std::string& src_; // the string to be tokenized
    size_t char_ix_{};       // current char index in 'src_'
    const char* pos{};       // position in string used for easier debugging

  public:
    explicit tokenizer(const std::string& src) : src_{src} {}

    tokenizer() = delete;
    tokenizer(const tokenizer&) = default;
    tokenizer(tokenizer&&) = default;
    auto operator=(const tokenizer&) -> tokenizer& = delete;
    auto operator=(tokenizer&&) -> tokenizer& = delete;

    ~tokenizer() = default;

    [[nodiscard]] auto is_eos() const -> bool {
        return char_ix_ >= src_.size();
    }

    auto next_token() -> token {
        const std::string ws_before{next_whitespace()};
        const size_t bgn_ix{char_ix_};
        if (is_next_char('"')) {
            // string token
            std::string txt;
            while (true) {
                if (is_next_char('"')) {
                    const size_t end{char_ix_};
                    const std::string ws_after{next_whitespace()};
                    return token{ws_before, bgn_ix, txt, end, ws_after, true};
                }
                txt.push_back(next_char());
            }
        }
        // not a string
        const std::string txt{next_token_str()};
        const size_t end_ix{char_ix_};
        const std::string ws_after{next_whitespace()};
        return {ws_before, bgn_ix, txt, end_ix, ws_after};
    }

    // returns a token which is a marker at current position with empty name
    // and whitespaces
    [[nodiscard]] auto current_position_token() const -> token {
        return {"", char_ix_, "", char_ix_, ""};
    }

    auto rewind_to_position(const token& pos_tk) -> void {
        const size_t new_pos{pos_tk.start_index()};
        assert(new_pos <= src_.size());
        char_ix_ = new_pos;
    }

    auto put_back_token(const token& t) -> void {
        //? todo. validate token is same as source
        move_back(t.total_length_in_chars());
    }

    auto put_back_char(const char ch) -> void {
        assert(char_ix_ > 0 and src_[char_ix_ - 1] == ch);
        move_back(1);
    }

    auto next_whitespace_token() -> token {
        return {next_whitespace(), char_ix_, "", char_ix_, ""};
    }

    auto is_next_char(const char ch) -> bool {
        if (is_eos() or src_[char_ix_] != ch) {
            return false;
        }
        (void)next_char(); // return ignored
        return true;
    }

    [[nodiscard]] auto is_peek_char(const char ch) const -> bool {
        return not is_eos() and src_[char_ix_] == ch;
    }

    [[nodiscard]] auto is_peek_char2(const char ch) const -> bool {
        return char_ix_ + 1 < src_.size() and src_[char_ix_ + 1] == ch;
    }

    [[nodiscard]] auto peek_char() const -> char {
        return is_eos() ? '\0' : src_[char_ix_];
    }

    auto read_rest_of_line() -> std::string {
        const size_t bgn{char_ix_};
        while (not is_eos()) {
            if (src_[char_ix_] == '\n') {
                break;
            }
            char_ix_++;
        }
        const size_t len{char_ix_ - bgn};
        if (not is_eos()) {
            char_ix_++; // skip the '\n'
        }
        return src_.substr(bgn, len);
    }

    auto next_char() -> char {
        assert(not is_eos());
        // note: just for easier debugging
        pos = &src_[char_ix_ + 1];
        return src_[char_ix_++];
    }

    [[nodiscard]] auto current_char_index_in_source() const -> size_t {
        return char_ix_;
    }

  private:
    auto next_whitespace() -> std::string {
        if (is_eos()) {
            return "";
        }
        const size_t bgn_ix{char_ix_};
        while (not is_eos()) {
            const char ch{src_[char_ix_]};
            if (not std::isspace(ch)) {
                break;
            }
            char_ix_++;
        }
        const size_t len{char_ix_ - bgn_ix};
        return src_.substr(bgn_ix, len);
    }

    auto next_token_str() -> std::string {
        if (is_eos()) {
            return "";
        }
        const size_t bgn_ix{char_ix_};
        while (not is_eos()) {
            const char ch{src_[char_ix_]};
            if (delimiters_.contains(ch)) {
                break;
            }
            char_ix_++;
        }
        const size_t len{char_ix_ - bgn_ix};
        return src_.substr(bgn_ix, len);
    }

    auto move_back(const size_t nchars) -> void {
        assert(char_ix_ >= nchars);
        char_ix_ -= nchars;
    }
};
