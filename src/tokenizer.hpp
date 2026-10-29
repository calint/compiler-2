#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "operand.hpp"
#include "token.hpp"

// reads the tokens of one source file, keeping the whitespace and comments
// in the tokens so that the source can be reproduced
class tokenizer final {
    std::string_view src_;
    size_t char_ix_{}; // current char index in 'src_'
    size_t at_line_{1};
    size_t file_ix_{}; // index of the source file of 'src_'

    static constexpr std::string_view delimiters_{
        " \t\r\n(){}[]=,.:+-*/%&|^<>!~#"};

    // the operators of expressions, the longer first so that '<<' and '<='
    // are not read as '<'; '=' and '!' alone are not operators
    static constexpr std::array<std::string_view, 17> operators_{
        "<<", ">>", "<=", ">=", "==", "!=", "+", "-", "*",
        "/",  "%",  "&",  "|",  "^",  "<",  ">", "~",
    };

    // note: the longer first, an operator is never after one that is its
    //       prefix, e.g. '<<' before '<'

    static_assert([] -> bool {
        for (size_t i{}; i < operators_.size(); ++i) {
            for (size_t j{i + 1}; j < operators_.size(); ++j) {
                if (operators_.at(j).starts_with(operators_.at(i))) {
                    return false;
                }
            }
        }

        return true;
    }());

    // note: the operators are the texts of the spelling tables, so a text
    //       missing here would end an expression silently

    static_assert(std::ranges::all_of(
        binary_operators, [](const binary_operator_traits& traits) -> bool {
            return std::ranges::contains(operators_, traits.text);
        }));
    static_assert(std::ranges::all_of(
        unary_spellings, [](const arithmetic_spelling& spelling) -> bool {
            return std::ranges::contains(operators_, spelling.text);
        }));
    static_assert(std::ranges::all_of(
        comparison_spellings, [](const comparison_spelling& spelling) -> bool {
            return std::ranges::contains(operators_, spelling.text);
        }));

  public:
    tokenizer(const size_t file_ix, const std::string_view src)
        : src_{src}, file_ix_{file_ix} {}

    // continues at a position of an earlier pass over the same source, 'src' is
    // the text of the file of the position
    tokenizer(const std::string_view src, const token& position_tk)
        : src_{src}, char_ix_{position_tk.start_index()},
          at_line_{position_tk.at_line()}, file_ix_{position_tk.file_index()} {

        assert(char_ix_ <= src_.size());
    }

    // the argument of a conversion: a ')' where an argument starts is a
    // missing one, e.g. 'int()'
    auto assert_no_missing_argument(const std::string_view name,
                                    const size_t count) const -> void {

        if (peek_char_after_whitespace() != ')') {
            return;
        }

        throw compile_error{*this, argument_count_text(name, count, count)};
    }

    // a ',' where the last of 'count' arguments ends is a surplus one, e.g.
    // 'i8(1, 2)', or a trailing ','; without parameters anything
    // but ')' is, e.g. 'f(1)' of 'func f()'
    auto assert_no_surplus_argument(const std::string_view name,
                                    const size_t count) const -> void {

        const char next{peek_char_after_whitespace()};

        if (next == ')' or (count != 0 and next != ',')) {
            return;
        }

        // e.g. 'f(1, 2,)' has no surplus argument, the ',' of 'f(,)' follows
        // no argument
        if (count != 0) {
            tokenizer probe{*this};

            const std::optional<token> delimiter_tk{
                probe.is_next_char_token(','),
            };

            assert(delimiter_tk);

            probe.assert_no_trailing_delimiter(*delimiter_tk, ')', "a call");
        }

        throw compile_error{*this, argument_count_text(name, count, count)};
    }

    // e.g. the last ',' of 'f(1, 2,)', 'close' ends the list; a braced list
    // may end with a ',', see the value literals and the type definitions
    auto assert_no_trailing_delimiter(const token& delimiter_tk,
                                      const char close,
                                      const std::string_view list) const
        -> void {

        if (peek_char_after_whitespace() != close) {
            return;
        }

        throw compile_error{
            delimiter_tk,
            std::format("a trailing ',' is not allowed in {}", list)};
    }

    // 'next_token' made no text: the source ended or a delimiter is next, which
    // no statement or definition starts with
    auto assert_not_at_delimiter() const -> void {
        if (is_eos()) {
            return;
        }

        // e.g. the second '==' of 'x == 1 == true', an operator is quoted
        // whole
        const std::string_view op{operator_at_cursor()};

        const std::string_view delimiter{
            op.empty() ? std::string_view{&src_.at(char_ix_), 1} : op,
        };

        throw compile_error{*this,
                            std::format("unexpected {}", in_quotes(delimiter))};
    }

    // a marker at the cursor, without text or whitespace
    [[nodiscard]] auto cur_position_token() const -> token {
        return token::position(file_ix_, char_ix_, at_line_);
    }

    // the token of the character 'ch' when it is next, otherwise an error at
    // the cursor
    [[nodiscard]] auto expect_char_token(const char ch,
                                         const std::string_view message)
        -> token {

        const std::optional<token> tk{is_next_char_token(ch)};

        if (not tk) {
            throw compile_error{*this, std::string{message}};
        }

        return *tk;
    }

    // the token of the character 'ch' when it is next, otherwise an error at
    // 'src_loc_tk', e.g. the name before a missing '(', since the next token
    // can be on a later line
    [[nodiscard]] auto expect_char_token_after(const token& src_loc_tk,
                                               const char ch,
                                               const std::string_view message)
        -> token {

        const std::optional<token> tk{is_next_char_token(ch)};

        if (not tk) {
            throw compile_error{src_loc_tk, std::string{message}};
        }

        return *tk;
    }

    // a binary operator or a comparison follows, e.g. the '+' of 'x + 1'
    [[nodiscard]] auto is_binary_operator_next() const -> bool {
        const std::string_view text{peek_operator()};
        return binary_operator_of(text) or comparison_of(text);
    }

    // the token of 'ch' with the whitespace around it when 'ch' is next, the
    // cursor stays when not
    [[nodiscard]] auto is_next_char_token(const char ch)
        -> std::optional<token> {

        const std::string_view ws_before{next_whitespace()};

        if (not is_peek_char(ch)) {
            move_back(ws_before.size());
            return std::nullopt;
        }

        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};
        const std::string_view txt{src_.substr(char_ix_, 1)};
        ++char_ix_;
        const std::string_view ws_after{next_trailing_whitespace()};

        return token{file_ix_, ws_before, bgn_ix, txt, ws_after, at_line};
    }

    [[nodiscard]] auto is_peek_char(const char ch) const -> bool {
        return not is_eos() and src_.at(char_ix_) == ch;
    }

    // a prefix operator follows, e.g. the '-' of '-x'
    [[nodiscard]] auto is_unary_operator_next() const -> bool {
        return unary_operator_of(peek_operator()).has_value();
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
        const std::string_view ws_after{next_trailing_whitespace()};

        return {file_ix_, ws_before, bgn_ix, txt, ws_after, at_line};
    }

    // the position of the next token, past the whitespace and comments
    [[nodiscard]] auto next_token_position() const -> token {
        tokenizer probe{*this};

        std::ignore = probe.next_whitespace();

        return probe.cur_position_token();
    }

    // the whitespace before a value, as the own token of a wrapper node (see
    // 'statement')
    [[nodiscard]] auto next_whitespace_token() -> token {
        const std::string_view ws_before{next_whitespace()};
        return {file_ix_, ws_before, char_ix_, "", "", at_line_};
    }

    // the character after the whitespace, comments and newlines included,
    // without reading
    [[nodiscard]] auto peek_char_after_whitespace() const -> char {
        tokenizer probe{*this};

        std::ignore = probe.next_whitespace();

        return probe.peek_char();
    }

    // the operator after the whitespace without reading it, e.g. the '<<' of
    // ' << 2'; empty when none is next
    [[nodiscard]] auto peek_operator() const -> std::string_view {
        tokenizer probe{*this};

        std::ignore = probe.next_whitespace();

        return probe.operator_at_cursor();
    }

    // only the last read token can be put back, otherwise the rewind lands
    // at an unrelated position
    auto put_back_token(const token& tk) -> void {
        assert(tk.source_end_index() == char_ix_);
        assert(is_token_text_at_source(tk));

        move_back(char_ix_ - tk.source_begin_index());
    }

    // the token of 'ch' that the caller saw next, e.g. by
    // 'peek_char_after_whitespace'
    [[nodiscard]] auto seen_char_token(const char ch) -> token {
        const std::optional<token> tk{is_next_char_token(ch)};

        assert(tk);

        return *tk;
    }

    // the token of the operator that the caller saw with 'peek_operator', with
    // the whitespace before it; the whitespace after it belongs to what
    // follows
    [[nodiscard]] auto seen_operator() -> token {
        const std::string_view ws_before{next_whitespace()};
        const std::string_view text{operator_at_cursor()};

        assert(not text.empty());

        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};
        char_ix_ += text.size();

        return token{file_ix_, ws_before, bgn_ix, text, "", at_line};
    }

    // skips an argument of a call, up to the ',' or ')' that ends it
    // e.g. 'a + f(b, c), d)' stops at the ',' before 'd'
    auto skip_argument() -> void { skip_to_unnested(",)"); }

    // skips the braced block that starts at the next '{', strings and
    // character literals may contain brackets
    // e.g. '{ a = { 1 } b = 2 }', a closer without an opener ends it early
    auto skip_braced_block() -> void {
        // note: every caller has seen the '{'
        std::ignore = seen_char_token('{');

        skip_to_unnested("}");

        std::ignore = expect_char_token('}', "expected '}' to close block");
    }

    // skips the next braced block and returns the source text from the end of
    // 'tk' to the end of the block
    auto skip_braced_block_after(const token& tk) -> std::string_view {
        skip_braced_block();
        return text_after(tk);
    }

    // skips up to and including the ')' that closes a '(' that is already read
    // e.g. 'b, f(c)) + d', the ')' before ' + d'
    auto skip_to_close_paren() -> void {
        skip_to_unnested(")");

        if (not is_eos()) {
            skip_token();
        }
    }

    // the source text from the end of 'tk' to the current position
    [[nodiscard]] auto text_after(const token& tk) const -> std::string_view {
        const size_t begin_ix{tk.source_end_index()};
        // note: the text starts after 'tk' because its statement prints 'tk'

        return src_.substr(begin_ix, char_ix_ - begin_ix);
    }

  private:
    // the opening quote has been read, the text keeps both quotes so it
    // resolves like a numeric constant, e.g. 'a' or '\n'
    [[nodiscard]] auto
    finish_character_literal_token(const std::string_view ws_before,
                                   const size_t at_line, const size_t bgn_ix)
        -> token {

        while (true) {
            // points at the opening quote, where the literal starts
            if (is_eos() or is_peek_char('\n')) {
                throw compile_error{token::position(file_ix_, bgn_ix, at_line),
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

        const token literal_tk{
            file_ix_, ws_before, bgn_ix, src_.substr(bgn_ix, end_ix - bgn_ix),
            ws_after, at_line,
        };

        // checked where it is read like a string, also in a generic body that
        // no instance parses
        std::ignore =
            constant_parser::parse_character(literal_tk, literal_tk.text());

        return literal_tk;
    }

    // the opening quote has been read, the text keeps both quotes
    [[nodiscard]] auto finish_string_token(const std::string_view ws_before,
                                           const size_t at_line,
                                           const size_t bgn_ix) -> token {

        // points at the opening quote, the string may run to the end of the
        // source
        const token open_quote_tk{token::position(file_ix_, bgn_ix, at_line)};

        while (true) {
            if (is_next_char('\\')) {
                // a backslash at the end of the source escapes nothing
                if (is_eos()) {
                    throw compile_error{open_quote_tk, "unterminated string"};
                }

                const bool is_carriage_return{is_peek_char('\r')};
                skip_char();

                // a crlf line end after the backslash continues the string
                if (is_carriage_return) {
                    std::ignore = is_next_char('\n');
                }

                continue;
            }

            if (is_next_char('"')) {
                break;
            }

            // a string cannot span lines, the source is echoed in comments
            if (is_eos() or is_peek_char('\n')) {
                throw compile_error{open_quote_tk, "unterminated string"};
            }

            skip_char();
        }

        const size_t end_ix{char_ix_};
        const std::string_view ws_after{next_trailing_whitespace()};

        const token string_tk{
            file_ix_, ws_before, bgn_ix, src_.substr(bgn_ix, end_ix - bgn_ix),
            ws_after, at_line,
        };

        if (const std::optional<token> escape_tk{
                string_tk.unsupported_escape(),
            }) {

            throw compile_error{*escape_tk,
                                std::format("unsupported escape {}",
                                            in_quotes(escape_tk->text()))};
        }

        return string_tk;
    }

    [[nodiscard]] auto is_eos() const -> bool {
        return char_ix_ >= src_.size();
    }

    [[nodiscard]] auto is_next_char(const char ch) -> bool {
        if (not is_peek_char(ch)) {
            return false;
        }

        skip_char();

        return true;
    }

    [[nodiscard]] auto is_token_text_at_source(const token& tk) const -> bool {
        return src_.substr(tk.start_index(),
                           tk.end_index() - tk.start_index()) == tk.text();
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

    [[nodiscard]] auto next_char() -> char {
        assert(not is_eos());

        const char ch{src_.at(char_ix_)};
        ++char_ix_;

        if (ch == '\n') {
            ++at_line_;
        }

        return ch;
    }

    // the text up to the next delimiter, e.g. 'abc+d' gives 'abc', a delimiter
    // at the start gives an empty text
    [[nodiscard]] auto next_token_str() -> std::string_view {
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

    // the longest operator at the cursor, a view of the source; empty when
    // none
    [[nodiscard]] auto operator_at_cursor() const -> std::string_view {
        const std::string_view rest{src_.substr(char_ix_)};

        for (const std::string_view op : operators_) {
            if (rest.starts_with(op)) {
                return rest.substr(0, op.size());
            }
        }

        return {};
    }

    [[nodiscard]] auto peek_char() const -> char {
        return is_eos() ? '\0' : src_.at(char_ix_);
    }

    // reads a character the caller has no use for
    auto skip_char() -> void { std::ignore = next_char(); }

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
        const token tk{next_token()};

        if (tk.is_none() and not is_eos()) {
            skip_char();
        }
    }
};

// declared in 'compile_error.hpp'
inline compile_error::compile_error(const tokenizer& tz, std::string message)
    : compile_error{tz.next_token_position(), std::move(message)} {}
