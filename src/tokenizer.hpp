#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cctype>
#include <string>

#include "token.hpp"

class tokenizer final {
    const std::string_view delimiters_{" \t\r\n(){}[]=,.:+-*/%&|^<>!\0"};
    const std::string& src_; // the string to be tokenized
    const char* ptr_{};      // pointer to current position
    size_t char_ix_{};       // current char index in 'src_'

  public:
    explicit tokenizer(const std::string& src)
        : src_{src}, ptr_{src_.c_str()} {}

    tokenizer() = delete;
    tokenizer(const tokenizer&) = default;
    tokenizer(tokenizer&&) = default;
    auto operator=(const tokenizer&) -> tokenizer& = delete;
    auto operator=(tokenizer&&) -> tokenizer& = delete;

    ~tokenizer() = default;

    [[nodiscard]] auto is_eos() const -> bool { return *ptr_ == '\0'; }

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

    // returns an token which is a marker at current position with empty name
    // and whitespaces
    [[nodiscard]] auto current_position_token() const -> token {
        return {"", char_ix_, "", char_ix_, ""};
    }

    auto rewind_to_position(const token& pos_tk) -> void {
        //? todo. assert if ok
        const size_t new_pos{pos_tk.start_index()};
        ptr_ = ptr_ - (char_ix_ - new_pos);
        char_ix_ = new_pos;
    }

    auto put_back_token(const token& t) -> void {
        //? todo. validate token is same as source
        move_back(t.total_length_in_chars());
    }

    auto put_back_char(const char ch) -> void {
        assert(char_ix_ > 0 and *(ptr_ - 1) == ch);
        move_back(sizeof(ch));
    }

    auto next_whitespace_token() -> token {
        return {next_whitespace(), char_ix_, "", char_ix_, ""};
    }

    auto is_next_char(const char ch) -> bool {
        if (*ptr_ != ch) {
            return false;
        }
        (void)next_char(); // return ignored
        return true;
    }

    [[nodiscard]] auto is_peek_char(const char ch) const -> bool {
        return *ptr_ == ch;
    }

    [[nodiscard]] auto is_peek_char2(const char ch) const -> bool {
        return *ptr_ == 0 ? false : *(ptr_ + 1) == ch;
    }

    [[nodiscard]] auto peek_char() const -> char { return *ptr_; }

    auto read_rest_of_line() -> std::string {
        const char* bgn{ptr_};
        while (true) {
            if (*ptr_ == '\n') {
                break;
            }
            if (*ptr_ == '\0') {
                break;
            }
            ptr_++;
        }
        const std::string s{bgn, static_cast<size_t>(ptr_ - bgn)};
        ptr_++;
        char_ix_ += static_cast<size_t>(ptr_ - bgn);
        return s;
    }

    auto next_char() -> char {
        const char ch{*ptr_};
        ptr_++;
        char_ix_++;
        return ch;
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
        while (true) {
            const char ch{next_char()};
            if (std::isspace(ch)) {
                continue;
            }
            move_back(1);
            break;
        }
        const size_t len{char_ix_ - bgn_ix};
        return {ptr_ - len, len};
    }

    auto next_token_str() -> std::string {
        if (is_eos()) {
            return "";
        }
        const size_t bgn_ix{char_ix_};
        while (true) {
            const char ch{next_char()};
            if (delimiters_.contains(ch)) {
                move_back(1);
                break;
            }
        }
        const size_t len{char_ix_ - bgn_ix};
        return {ptr_ - len, len};
    }

    auto move_back(const size_t nchars) -> void {
        assert(char_ix_ >= nchars);

        ptr_ -= nchars;
        char_ix_ = static_cast<size_t>(char_ix_ - nchars);
    }
};
