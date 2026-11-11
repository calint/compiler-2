#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <expected>
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
        comparison_operators, [](const comparison_traits& traits) -> bool {
            return std::ranges::contains(operators_, traits.text);
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
    // no statement or definition starts with; a skip stops before the
    // whitespace of the closer that ends it, e.g. the ')' of '{ f() ) }'
    auto assert_not_at_delimiter() const -> void {
        const tokenizer probe{past_whitespace()};

        if (probe.is_eos()) {
            return;
        }

        // e.g. the '==' of 'exit(0) == 1', an operator is quoted whole
        const std::string_view op{probe.operator_at_cursor()};

        const std::string_view delimiter{
            op.empty() ? src_.substr(probe.char_ix_, 1) : op,
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

    // a binary operator or a comparison follows, e.g. the '+' of 'x + 1'
    [[nodiscard]] auto is_binary_operator_next() const -> bool {
        const std::string_view text{peek_operator()};
        return binary_operator_of(text) or comparison_of(text);
    }

    // a '=' follows that does not start a '==', e.g. the '=' of 'x = 1' but
    // not of 'x == 1'
    [[nodiscard]] auto is_lone_equals_next() const -> bool {
        return peek_char_after_whitespace() == '=' and peek_operator() != "==";
    }

    // the token of 'ch' with the whitespace around it when 'ch' is next, the
    // cursor stays when not
    [[nodiscard]] auto is_next_char_token(const char ch)
        -> std::optional<token> {

        const std::string_view ws_before{next_whitespace()};

        if (peek_char() != ch) {
            move_back(ws_before.size());
            return std::nullopt;
        }

        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};
        ++char_ix_;

        return finish_token(ws_before, at_line, bgn_ix);
    }

    // the token of the word 'word' when it is next, the cursor stays when not,
    // e.g. the 'else' after a branch
    [[nodiscard]] auto is_next_word_token(const std::string_view word)
        -> std::optional<token> {

        const token tk{next_token()};

        if (not tk.is_text(word)) {
            put_back_token(tk);
            return std::nullopt;
        }

        return tk;
    }

    // a '<' follows that can start type arguments, e.g. the '<' of
    // 'show<i8>(x)' but not of the operators of 'show <= x' or 'show << x'
    [[nodiscard]] auto is_type_args_next() const -> bool {
        return peek_operator() == "<";
    }

    // e.g. '  # note\n  x = 1' gives the token 'x' with the whitespace and
    // comment before it
    [[nodiscard]] auto next_token() -> token {
        const std::string_view ws_before{next_whitespace()};
        const size_t at_line{at_line_};
        const size_t bgn_ix{char_ix_};

        if (is_next_char('"') or is_next_char('\'')) {
            return finish_quoted_token(ws_before, at_line, bgn_ix);
        }

        skip_to_delimiter();

        return finish_token(ws_before, at_line, bgn_ix);
    }

    // the position of the next token, past the whitespace and comments
    [[nodiscard]] auto next_token_position() const -> token {
        return past_whitespace().cur_position_token();
    }

    // the whitespace before a value, as the own token of a wrapper node (see
    // 'statement')
    [[nodiscard]] auto next_whitespace_token() -> token {
        const std::string_view ws_before{next_whitespace()};
        return {file_ix_, ws_before, char_ix_, "", "", at_line_};
    }

    // the character after the whitespace, comments and newlines included,
    // without reading; '\0' at the end of the source
    [[nodiscard]] auto peek_char_after_whitespace() const -> char {
        return past_whitespace().peek_char();
    }

    // the operator after the whitespace without reading it, e.g. the '<<' of
    // ' << 2'; empty when none is next
    [[nodiscard]] auto peek_operator() const -> std::string_view {
        return past_whitespace().operator_at_cursor();
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
    auto skip_argument() -> void { std::ignore = skip_to_unnested(",)"); }

    // skips the braced block that starts at the next '{', strings and
    // character literals may contain brackets; false when no '}' closes it
    // e.g. '{ a = { 1 } b = 2 }', a closer that does not close the last open
    // bracket ends it early
    [[nodiscard]] auto skip_braced_block() -> bool {
        return skip_enclosed('{', '}');
    }

    // skips the next braced block and returns the source text from the end of
    // 'tk' to the end of the block
    auto skip_braced_block_after(const token& tk) -> std::string_view {
        if (not skip_braced_block()) {
            // e.g. the ')' of '{ x ) }', as in a block that is parsed
            assert_not_at_delimiter();

            throw compile_error{*this, "expected '}' to close block"};
        }

        return text_after(tk);
    }

    // skips from the 'open' that is next to the 'close' that ends it, e.g.
    // the '[2 * n]' of 'i8[2 * n]{}'; false when no 'close' ends it
    [[nodiscard]] auto skip_enclosed(const char open, const char close)
        -> bool {

        // note: every caller has seen the 'open'
        std::ignore = seen_char_token(open);

        if (not skip_to_unnested(std::string_view{&close, 1})) {
            return false;
        }

        std::ignore = seen_char_token(close);

        return true;
    }

    // skips up to and including the ')' that closes a '(' that is already read
    // e.g. 'b, f(c)) + d', the ')' before ' + d'; stops at the end of the
    // source or at a closer that does not close the last open bracket
    auto skip_to_close_paren() -> void {
        if (skip_to_unnested(")")) {
            std::ignore = seen_char_token(')');
        }
    }

    // the source text from the end of 'tk' to the current position
    [[nodiscard]] auto text_after(const token& tk) const -> std::string_view {
        const size_t begin_ix{tk.source_end_index()};
        // note: the text starts after 'tk' because its statement prints 'tk'

        return src_.substr(begin_ix, char_ix_ - begin_ix);
    }

    //
    // statics
    //

    // the source of file 'file_ix' has no bytes that an editor shows other
    // than the tokenizer reads them: a byte order mark, which hides in the
    // first token, and a carriage return without a line feed, which ends a
    // line on screen but not a comment
    static auto assert_source_well_formed(const size_t file_ix,
                                          const std::string_view src) -> void {

        constexpr std::string_view byte_order_mark{"\xef\xbb\xbf"};

        if (src.starts_with(byte_order_mark)) {
            const token mark_tk{
                token::synthetic(file_ix, byte_order_mark, 0, 1),
            };

            throw compile_error{mark_tk, "the source starts with a byte order "
                                         "mark, save it without one"};
        }

        size_t line{1};

        for (size_t i{}; i < src.size(); ++i) {
            if (src.at(i) == '\n') {
                ++line;
                continue;
            }

            const bool is_lone_carriage_return{
                src.at(i) == '\r' and not src.substr(i + 1).starts_with('\n'),
            };

            if (is_lone_carriage_return) {
                const token cr_tk{
                    token::synthetic(file_ix, src.substr(i, 1), i, line),
                };

                throw compile_error{
                    cr_tk, "a carriage return is not followed by a line feed"};
            }
        }
    }

  private:
    // the opening quote has been read, the text keeps both quotes; a
    // character literal resolves like a numeric constant, e.g. 'a' or '\n'
    [[nodiscard]] auto finish_quoted_token(const std::string_view ws_before,
                                           const size_t at_line,
                                           const size_t bgn_ix) -> token {

        const char quote{src_.at(bgn_ix)};
        const bool is_string{quote == '"'};

        // points at the opening quote, where the literal starts
        const token open_quote_tk{token::position(file_ix_, bgn_ix, at_line)};

        skip_past_closing_quote(quote, open_quote_tk);

        const token literal_tk{finish_token(ws_before, at_line, bgn_ix)};

        // an unsupported escape is reported before the length of a character
        // literal, e.g. the '\X' of '\X41' and the '\q' of 'a\q'
        assert_escapes_supported(literal_tk);

        // checked where it is read like a string, also in a generic body that
        // no instance parses
        if (not is_string) {
            std::ignore =
                constant_parser::parse_character(literal_tk, literal_tk.text());
        }

        return literal_tk;
    }

    // the token of the text from 'bgn_ix' to the cursor, with the whitespace
    // after it that 'next_trailing_whitespace' gives
    [[nodiscard]] auto finish_token(const std::string_view ws_before,
                                    const size_t at_line, const size_t bgn_ix)
        -> token {

        const std::string_view text{src_.substr(bgn_ix, char_ix_ - bgn_ix)};
        const std::string_view ws_after{next_trailing_whitespace()};
        return {file_ix_, ws_before, bgn_ix, text, ws_after, at_line};
    }

    [[nodiscard]] auto is_eos() const -> bool {
        return char_ix_ >= src_.size();
    }

    [[nodiscard]] auto is_next_char(const char ch) -> bool {
        if (peek_char() != ch) {
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

    // a copy of this tokenizer past the whitespace and comments, for a look
    // at what follows them
    [[nodiscard]] auto past_whitespace() const -> tokenizer {
        tokenizer probe{*this};

        std::ignore = probe.next_whitespace();

        return probe;
    }

    [[nodiscard]] auto peek_char() const -> char {
        return is_eos() ? '\0' : src_.at(char_ix_);
    }

    // reads a character the caller has no use for
    auto skip_char() -> void { std::ignore = next_char(); }

    // the text of a literal opened by 'quote' up to and with its closing
    // quote, which an escape does not close
    auto skip_past_closing_quote(const char quote, const token& open_quote_tk)
        -> void {

        const bool is_string{quote == '"'};

        const std::string_view unterminated{
            is_string ? "unterminated string"
                      : "unterminated character literal",
        };

        while (true) {
            // a literal cannot span lines, the source is echoed in comments
            if (is_eos() or peek_char() == '\n') {
                throw compile_error{open_quote_tk, std::string{unterminated}};
            }

            const char ch{next_char()};

            if (ch == quote) {
                return;
            }

            if (ch != '\\' or is_eos()) {
                continue;
            }

            // a line end after the backslash continues a string, a
            // character literal is unterminated there
            const bool is_line_end{peek_char() == '\n' or peek_char() == '\r'};

            if (is_line_end and not is_string) {
                continue;
            }

            // the escaped character may be a quote; a carriage return is
            // followed by a line feed ('assert_source_well_formed')
            std::ignore = is_next_char('\r');
            skip_char();
        }
    }

    // moves to the next delimiter, e.g. past the 'abc' of 'abc+d'; stays at a
    // delimiter
    auto skip_to_delimiter() -> void {
        const size_t delimiter{src_.find_first_of(delimiters_, char_ix_)};

        char_ix_ =
            delimiter == std::string_view::npos ? src_.size() : delimiter;
    }

    // the newline is left for 'next_whitespace' so it counts the line
    // e.g. the rest of '# c\n' is '# c'
    auto skip_to_end_of_line() -> void {
        const size_t newline{src_.find('\n', char_ix_)};
        char_ix_ = newline == std::string_view::npos ? src_.size() : newline;
    }

    // skips tokens up to a character of 'stops' that is outside of brackets
    // opened by the skipped tokens, the character is not read; false when
    // the source ends or a closer that does not close the last open bracket
    // ends the skip, e.g. the '}' of 'a[1 }'
    [[nodiscard]] auto skip_to_unnested(const std::string_view stops) -> bool {
        constexpr std::string_view openers{"([{"};
        constexpr std::string_view closers{")]}"};

        // the closers of the open brackets, the innermost last
        std::string expected_closers;

        while (not is_eos()) {
            const char next{peek_char_after_whitespace()};

            if (expected_closers.empty() and stops.contains(next)) {
                return true;
            }

            const bool is_closer{closers.contains(next)};

            if (is_closer and not expected_closers.ends_with(next)) {
                return false;
            }

            if (is_closer) {
                expected_closers.pop_back();
            }

            const size_t opener_ix{openers.find(next)};

            if (opener_ix != std::string_view::npos) {
                expected_closers.push_back(closers.at(opener_ix));
            }

            skip_token();
        }

        return false;
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

    //
    // statics
    //

    // the escapes of the string or character literal 'literal_tk' decode
    static auto assert_escapes_supported(const token& literal_tk) -> void {
        const std::expected<std::string, token> bytes{literal_tk.decoded()};

        if (not bytes) {
            throw compile_error{bytes.error(),
                                std::format("unsupported escape {}",
                                            in_quotes(bytes.error().text()))};
        }
    }
};

// declared in 'compile_error.hpp'
inline compile_error::compile_error(const tokenizer& tz, std::string message)
    : compile_error{tz.next_token_position(), std::move(message)} {}
