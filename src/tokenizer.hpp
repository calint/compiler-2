#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "compiler_exception.hpp"
#include "token.hpp"

class tokenizer final {
    std::string_view src_;
    size_t char_ix_{}; // current char index in 'src_'
    size_t at_line_{1};
    size_t file_ix_{}; // index of the source file of 'src_'
    size_t nesting_{};

    static constexpr std::string_view delimiters_{
        " \t\r\n(){}[]=,.:+-*/%&|^<>!#"};

  public:
    // one level of nesting while it lives
    class nesting_scope final {
        tokenizer& tz_;

      public:
        explicit nesting_scope(tokenizer& tz) : tz_{tz} {
            if (tz_.nesting_ >= max_nesting) {
                throw compiler_exception{
                    tz_, std::format("nesting is too deep, the limit is {} "
                                     "levels",
                                     max_nesting)};
            }

            ++tz_.nesting_;
        }

        nesting_scope(const nesting_scope&) = delete;
        nesting_scope(nesting_scope&&) = delete;
        auto operator=(const nesting_scope&) -> nesting_scope& = delete;
        auto operator=(nesting_scope&&) -> nesting_scope& = delete;

        ~nesting_scope() { --tz_.nesting_; }
    };

    tokenizer(const size_t file_ix, const std::string_view src)
        : src_{src}, file_ix_{file_ix} {}

    // continues at a position of an earlier pass over the same source, 'src' is
    // the text of the file of the position
    tokenizer(const std::string_view src, const token& position_tk)
        : src_{src}, char_ix_{position_tk.start_index()},
          at_line_{position_tk.at_line()}, file_ix_{position_tk.file_index()} {

        assert(char_ix_ <= src_.size());
    }

    // the most levels of blocks and expressions inside each other, the parsers
    // recurse for each of them so the limit keeps the source from exhausting
    // the stack
    static constexpr size_t max_nesting{128};

    // 'next_token' made no text: the source ended or a delimiter is next, which
    // no statement or definition starts with
    auto assert_not_at_delimiter() const -> void {
        if (is_eos()) {
            return;
        }

        throw compiler_exception{*this,
                                 std::format("unexpected '{}'", peek_char())};
    }

    [[nodiscard]] auto cur_char_index_in_source() const -> size_t {
        return char_ix_;
    }

    [[nodiscard]] auto cur_line() const -> size_t { return at_line_; }

    // returns a token, which is a marker at the current position with empty
    // name and whitespace
    [[nodiscard]] auto cur_position_token() const -> token {
        return token::position(file_ix_, char_ix_, at_line_);
    }

    // the token of the character 'ch' when it is next, otherwise an error at
    // the cursor
    [[nodiscard]] auto expect_char_token(const char ch,
                                         const std::string_view message)
        -> token {

        const token tk{is_next_char_token(ch)};

        if (tk.is_empty()) {
            throw compiler_exception{*this, std::string{message}};
        }

        return tk;
    }

    [[nodiscard]] auto file_index() const -> size_t { return file_ix_; }

    [[nodiscard]] auto is_eos() const -> bool {
        return char_ix_ >= src_.size();
    }

    [[nodiscard]] auto is_next_char(const char ch) -> bool {
        if (not is_peek_char(ch)) {
            return false;
        }

        return next_char();
    }

    [[nodiscard]] auto is_next_char_token(const char ch) -> token {
        const std::string_view ws_before{next_whitespace()};

        if (not is_peek_char(ch)) {
            move_back(ws_before.size());
            return {};
        }

        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};
        const std::string_view txt{src_.substr(char_ix_, 1)};
        ++char_ix_;
        const size_t end_ix{char_ix_};
        const std::string_view ws_after{next_trailing_whitespace()};

        return {file_ix_, ws_before, bgn_ix,  txt,
                end_ix,   ws_after,  at_line, false};
    }

    [[nodiscard]] auto is_peek_char(const char ch) const -> bool {
        return not is_eos() and src_.at(char_ix_) == ch;
    }

    [[nodiscard]] auto is_peek_char2(const char ch) const -> bool {
        return char_ix_ + 1 < src_.size() and src_.at(char_ix_ + 1) == ch;
        // note: +1 because the character after the current one is peeked
    }

    [[nodiscard]] auto next_char() -> char {
        assert(not is_eos());

        const char ch{src_.at(char_ix_)};
        ++char_ix_;

        if (ch == '\n') {
            ++at_line_;
        }

        return ch;
    }

    // e.g. '  # note\n  x = 1' gives the token 'x' with the whitespace and
    // comment before it
    [[nodiscard]] auto next_token() -> token {
        const std::string_view ws_before{next_whitespace()};
        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};

        if (is_next_char('"')) {
            return finish_string_token(ws_before, at_line, bgn_ix);
        }

        if (is_next_char('\'')) {
            return finish_character_literal_token(ws_before, at_line, bgn_ix);
        }

        const std::string_view txt{next_token_str()};
        const size_t end_ix{char_ix_};
        const std::string_view ws_after{next_trailing_whitespace()};

        return {file_ix_, ws_before, bgn_ix,  txt,
                end_ix,   ws_after,  at_line, false};
    }

    [[nodiscard]] auto next_whitespace_token() -> token {
        const size_t at_line{at_line_};

        return {file_ix_, next_whitespace(), char_ix_, "", char_ix_,
                "",       at_line,           false};
    }

    [[nodiscard]] auto peek_char() const -> char {
        return is_eos() ? '\0' : src_.at(char_ix_);
    }

    // trailing whitespace ends at a newline so lookahead past a token skips
    // the next line's indentation
    [[nodiscard]] auto peek_char_after_whitespace() -> char {
        const std::string_view ws{next_whitespace()};
        const char ch{peek_char()};
        move_back(ws.size());
        return ch;
    }

    auto put_back_char(const char ch) -> void {
        assert(char_ix_ > 0 and src_.at(char_ix_ - 1) == ch);

        move_back(1);
    }

    // only the last read token can be put back, otherwise the rewind lands
    // at an unrelated position
    auto put_back_token(const token& t) -> void {
        assert(t.source_end_index() == char_ix_);
        assert(is_token_text_at_source(t));

        move_back(char_ix_ - t.source_begin_index());
    }

    auto rewind_to_position(const token& pos_tk) -> void {
        const size_t new_pos{pos_tk.start_index()};

        assert(new_pos <= src_.size());

        const size_t char_count{char_ix_ - new_pos};
        move_back(char_count);
    }

    // skips an argument of a call, up to the ',' or ')' that ends it
    // e.g. 'a + f(b, c), d)' stops at the ',' before 'd'
    auto skip_argument() -> void { skip_to_unnested(",)"); }

    // skips up to and including the '}' that closes the next '{', strings and
    // character literals may contain braces
    // e.g. '{ a = { 1 } b = 2 }', the depth counts the braces that are open
    auto skip_braced_block() -> void {
        size_t depth{};

        while (true) {
            if (not is_next_char_token('{').is_empty()) {
                ++depth;
                continue;
            }

            const token close_tk{is_next_char_token('}')};

            if (close_tk.is_empty()) {
                if (is_eos()) {
                    throw compiler_exception{*this,
                                             "expected '}' to end block"};
                }

                skip_token();
                continue;
            }

            if (depth == 0) {
                throw compiler_exception{close_tk,
                                         "expected '{' to begin block"};
            }

            if (--depth == 0) {
                return;
            }
        }
    }

    // skips the next braced block and returns the source text from the end of
    // 'tk' to the end of the block
    auto skip_braced_block_after(const token& tk) -> std::string_view {
        skip_braced_block();

        const size_t begin_ix{tk.source_end_index()};
        // note: the text starts after 'tk' because its statement prints 'tk'

        return src_.substr(begin_ix, char_ix_ - begin_ix);
    }

    // reads a character the caller has no use for
    auto skip_char() -> void { std::ignore = next_char(); }

    // skips up to and including the ')' that closes a '(' that is already read
    // e.g. 'b, f(c)) + d', the ')' before ' + d'
    auto skip_to_close_paren() -> void {
        skip_to_unnested(")");

        if (not is_eos()) {
            skip_token();
        }
    }

  private:
    // the opening quote has been read, the text keeps both quotes so it
    // resolves like a numeric constant, e.g. 'a' or '\n'
    [[nodiscard]] auto
    finish_character_literal_token(const std::string_view ws_before,
                                   const size_t at_line, const size_t bgn_ix)
        -> token {

        while (true) {
            // points at the opening quote since the end of the line is
            // reported as column 0
            if (is_eos() or is_peek_char('\n')) {
                throw compiler_exception{
                    token::position(file_ix_, bgn_ix, at_line),
                    "unterminated character literal"};
            }

            const char ch{next_char()};

            if (ch == '\'') {
                break;
            }

            // the escaped character may be a quote
            if (ch == '\\' and not is_eos() and not is_peek_char('\n')) {
                skip_char();
            }
        }

        const size_t end_ix{char_ix_};
        const std::string_view ws_after{next_trailing_whitespace()};

        return {
            file_ix_, ws_before, bgn_ix,  src_.substr(bgn_ix, end_ix - bgn_ix),
            end_ix,   ws_after,  at_line, false};
    }

    // the opening quote has been read, the text excludes both quotes
    [[nodiscard]] auto finish_string_token(const std::string_view ws_before,
                                           const size_t at_line,
                                           const size_t bgn_ix) -> token {

        // points at the opening quote, the string may run to the end of the
        // source
        const token open_quote_tk{token::position(file_ix_, bgn_ix, at_line)};

        while (true) {
            if (is_next_char('\\')) {
                // read the escaped character
                if (is_eos()) {
                    throw compiler_exception{open_quote_tk,
                                             "unterminated string"};
                }

                skip_char();
                continue;
            }

            if (is_next_char('"')) {
                break;
            }

            if (is_eos()) {
                throw compiler_exception{open_quote_tk, "unterminated string"};
            }

            skip_char();
        }

        const size_t end_ix{char_ix_};
        const std::string_view ws_after{next_trailing_whitespace()};

        token string_tk{file_ix_, ws_before,
                        bgn_ix,   src_.substr(bgn_ix + 1, end_ix - bgn_ix - 2),
                        end_ix,   ws_after,
                        at_line,  true};
        // note: +1 and -2 because the text is between the quotes

        if (const std::optional<token> escape_tk{
                string_tk.unsupported_escape_position(),
            }) {

            throw compiler_exception{
                *escape_tk, std::format("unsupported escape in string \"{}\"",
                                        string_tk.text())};
        }

        return string_tk;
    }

    [[nodiscard]] auto is_token_text_at_source(const token& t) const -> bool {
        // the text of a string token excludes its quotes
        const size_t quote_count{t.is_string() ? 1UZ : 0UZ};

        return src_.substr(t.start_index() + quote_count,
                           t.end_index() - t.start_index() -
                               (2 * quote_count)) == t.text();
    }

    auto move_back(size_t nchars) -> void {
        assert(char_ix_ >= nchars);

        while (nchars--) {
            --char_ix_;

            if (src_.at(char_ix_) == '\n') {
                --at_line_;
            }
        }
    }

    // the text up to the next delimiter, e.g. 'abc+d' gives 'abc', a delimiter
    // at the start gives an empty text
    [[nodiscard]] auto next_token_str() -> std::string_view {
        if (is_eos()) {
            return "";
        }

        const size_t bgn_ix{char_ix_};
        const size_t delimiter{src_.find_first_of(delimiters_, char_ix_)};

        char_ix_ =
            delimiter == std::string_view::npos ? src_.size() : delimiter;

        const size_t len{char_ix_ - bgn_ix};

        return src_.substr(bgn_ix, len);
    }

    // the next line's indentation and comments belong to the next token, the
    // whitespace at the end of the source stays with the last token
    // e.g. ' # c\n    y' gives ' # c\n', the indentation of 'y' stays
    [[nodiscard]] auto next_trailing_whitespace() -> std::string_view {
        const size_t bgn_ix{char_ix_};
        const size_t len{next_whitespace().size()};
        const size_t newline{src_.substr(bgn_ix, len).find('\n')};

        if (is_eos() or newline == std::string_view::npos) {
            return src_.substr(bgn_ix, len);
        }

        move_back(len - newline - 1);

        return src_.substr(bgn_ix, newline + 1);
        // note: +1 to include the line end
    }

    // comments are part of the whitespace so parsers never see them and
    // 'source_to' reproduces them with the surrounding tokens
    // e.g. '  # c\n  x' gives '  # c\n  '
    [[nodiscard]] auto next_whitespace() -> std::string_view {
        if (is_eos()) {
            return "";
        }

        const size_t bgn_ix{char_ix_};
        while (not is_eos()) {
            const char ch{src_.at(char_ix_)};

            if (ch == '#') {
                skip_to_end_of_line();
                continue;
            }

            if (not std::string_view{" \t\r\n"}.contains(ch)) {
                break;
            }

            if (ch == '\n') {
                ++at_line_;
            }

            ++char_ix_;
        }

        const size_t len{char_ix_ - bgn_ix};

        return src_.substr(bgn_ix, len);
    }

    // the newline is left for 'next_whitespace' so it counts the line
    // e.g. the rest of '# c\n' is '# c'
    auto skip_to_end_of_line() -> void {
        const size_t newline{src_.find('\n', char_ix_)};
        char_ix_ = newline == std::string_view::npos ? src_.size() : newline;
    }

    // skips tokens up to a character of 'stops' that is outside of brackets
    // opened by the skipped tokens, the character is not read
    auto skip_to_unnested(const std::string_view stops) -> void {
        size_t depth{};

        while (not is_eos()) {
            const char next{peek_char_after_whitespace()};

            if (depth == 0 and stops.contains(next)) {
                return;
            }

            if (next == '(' or next == '[' or next == '{') {
                ++depth;
            } else if (next == ')' or next == ']' or next == '}') {
                // a closer without an opener ends the skip
                if (depth == 0) {
                    return;
                }

                --depth;
            }

            skip_token();
        }
    }

    // skips a token, or the character of a delimiter, which has an empty token
    // text
    // e.g. 'ab+c' skips 'ab', then '+', then 'c'
    auto skip_token() -> void {
        if (next_token().text().empty() and not is_eos()) {
            skip_char();
        }
    }
};

// declared in 'compiler_exception.hpp'
inline compiler_exception::compiler_exception(const tokenizer& tz,
                                              std::string message)
    : msg{std::move(message)}, file_ix{tz.file_index()}, line{tz.cur_line()},
      start_index{tz.cur_char_index_in_source()},
      end_index{tz.cur_char_index_in_source()} {}
