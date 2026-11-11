#pragma once

// what the next tokens are, decided before they are parsed: 'classify_term'
// tells what the first token of a value or a statement denotes; a name
// followed by a character has one meaning only, e.g. 'point{' is a record
// literal and 'show<' a generic call; 'builtin_value_type' gives the type of
// the value of a built-in; the name after a '.' is decided by
// 'stmt_identifier::is_method_name', a function head by 'func_head'; the
// rejections that run before a parse would misread ('assert_*') and the
// predicates of the shape of what follows ('continues_*', 'is_*',
// 'starts_value'); 'throw_literal_not_operand' words a literal or a call
// result used where only storage is an operand

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>

#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

// e.g. '[]{1, 2}' has the default type, no expression starts with '['
[[nodiscard]] inline auto is_default_array_literal(const tokenizer& tz)
    -> bool {

    return tz.peek_char_after_whitespace() == '[';
}

// a comparison or a connective follows, which makes a condition, e.g. the
// '==' of 'p == q' or the 'and' of 'b and c'
[[nodiscard]] inline auto continues_condition(const tokenizer& tz) -> bool {
    if (comparison_of(tz.peek_operator())) {
        return true;
    }

    tokenizer probe{tz};
    const token next_tk{probe.next_token()};

    return connective_of(next_tk).has_value();
}

// a binary operator or a connective follows, which would continue the
// expression, e.g. the '+' of 'x + 1' or the 'and' of 'b and c'
[[nodiscard]] inline auto continues_expression(const tokenizer& tz) -> bool {
    return tz.is_binary_operator_next() or continues_condition(tz);
}

[[nodiscard]] inline auto is_closer_or_end(const char ch) -> bool {
    return ch == ')' or ch == ']' or ch == '}' or ch == '\0';
}

// a ',' where the last of 'count' arguments ends is a surplus one, or a
// trailing ','; without parameters anything but a closer or the end is, e.g.
// 'f(1)' of 'func f()'; a missing ')' is the caller's
inline auto assert_no_surplus_argument(const tokenizer& tz,
                                       const std::string_view name,
                                       const size_t count) -> void {

    const char next{tz.peek_char_after_whitespace()};

    if (is_closer_or_end(next)) {
        return;
    }

    if (count != 0 and next != ',') {
        return;
    }

    // e.g. 'f(1, 2,)' has no surplus argument, the ',' of 'f(,)' follows
    // no argument
    if (count != 0) {
        tokenizer probe{tz};
        const token delimiter_tk{probe.seen_char_token(',')};
        probe.assert_no_trailing_delimiter(delimiter_tk, ')', "a call");
    }

    throw compile_error{tz, argument_count_text(name, count, count)};
}

// a value can start at the next token, not a closer, a ',', the end of the
// source or a keyword no value starts with, e.g. not at the ')' of
// '[2]{1, 2, )' or at the 'var' of the next statement
[[nodiscard]] inline auto starts_value(const tokenizer& tz) -> bool {
    const char next{tz.peek_char_after_whitespace()};

    if (next == ',' or is_closer_or_end(next)) {
        return false;
    }

    tokenizer probe{tz};
    const token tk{probe.next_token()};

    if (not std::ranges::contains(reserved_names::keywords, tk.text())) {
        return true;
    }

    return tk.is_text(reserved_names::self) or tk.is_text("not") or
           constant_parser::is_bool_literal(tk.text());
}

// a string is in read-only data that an array parameter would refer to, a
// string for a number is rejected where the number is parsed
inline auto assert_not_string_argument(tokenizer& tz) -> void {
    if (tz.peek_char_after_whitespace() != '"') {
        return;
    }

    throw compile_error{tz.next_token(), "a string is not an argument, it "
                                         "would refer to read-only data"};
}

// a '(' or a unary operator makes a value and a 'not' a condition where
// 'what' is expected, e.g. the '(' of 'array_length((a))', the '-' of
// 'q = -p' for "an instance of 'pt'" or the 'not' of 'f(not bs)'
inline auto assert_no_value_operator(const tokenizer& tz,
                                     const std::string_view what) -> void {

    tokenizer probe{tz};

    if (const std::optional<token> not_tk{probe.accept_word_token("not")}) {
        throw_not_makes_condition(*not_tk, what);
    }

    if (tz.peek_char_after_whitespace() == '(') {
        throw compile_error{
            tz, std::format("parentheses make a value, not {}", what)};
    }

    if (unary_operator_of(tz.peek_operator())) {
        throw compile_error{
            tz, std::format("a unary operator makes a value, not {}", what)};
    }
}

// an operator after an array makes a value, e.g. the '+' of
// 'array_length(a + 1)' or of 'f(a + 1)' for an array parameter
inline auto assert_no_operator_after_array(const tokenizer& tz) -> void {
    if (not continues_expression(tz)) {
        return;
    }

    throw compile_error{tz, "an operator makes a value, not an array"};
}

// '{}' follows, the zero value of a typed destination
[[nodiscard]] inline auto is_zero_value(const tokenizer& tz) -> bool {
    tokenizer probe{tz};

    return probe.accept_char_token('{') and
           probe.peek_char_after_whitespace() == '}';
}

// what the token at the start of a value or a statement is, see
// 'classify_term'
enum class term_kind : uint8_t {
    // no token, e.g. the end of the source or a delimiter
    none,
    // e.g. '"abc"'
    string,
    // e.g. '12', 'a', 'true'
    literal,
    // e.g. 'array_length' or 'read'
    builtin_function,
    // a built-in type followed by '(', e.g. 'i8(x)', only the integer types
    // convert
    conversion,
    // a built-in type alone, e.g. the 'i8' of 'var x = i8'
    builtin_type,
    // a type followed by '[', e.g. 'i8[]{1, 2}'
    array_literal,
    // a user type followed by '{', e.g. 'point{1, 2}'
    record_literal,
    // a type followed by '.', e.g. 'point.at(1, 2)'
    constructor_call,
    // a user type alone, e.g. the 'point' of 'var p = point'
    record_type,
    // a name followed by '(', e.g. 'f(x)' or 'point(3)' of a function 'point'
    call,
    // a user type followed by '(' with no function of its name, e.g.
    // 'point(5)', which is an error where a value is expected
    type_called,
    // a generic function followed by '<' or '(', e.g. 'show<i8>(x)'
    generic_call,
    // a variable, a parameter or a constant, e.g. 'x' of 'x + 1'
    value,
    // a function that is not called, e.g. 'f' of 'f + 1' or 'f<i8>' of a
    // function that is not generic
    function_name,
    // a keyword where a value is expected, e.g. 'else' of 'x = else'
    keyword,
    // a name that names nothing here
    unknown,
};

// a type where a value or an array is expected: a user or built-in type that
// is not converted, called or made an instance of by a constructor, or the
// start of an array or instance literal
[[nodiscard]] inline auto names_type(const term_kind kind) -> bool {
    return kind == term_kind::builtin_type or kind == term_kind::record_type or
           kind == term_kind::array_literal or
           kind == term_kind::record_literal;
}

// what a built-in type denotes, 'next' is the character after it
[[nodiscard]] inline auto classify_builtin_type_term(const char next)
    -> term_kind {

    if (next == '(') {
        return term_kind::conversion;
    }

    if (next == '[') {
        return term_kind::array_literal;
    }

    // e.g. 'i8.at', reported like a missing constructor of a user type
    if (next == '.') {
        return term_kind::constructor_call;
    }

    return term_kind::builtin_type;
}

// what a user type that is not called denotes, 'next' is the character after
// it
[[nodiscard]] inline auto classify_record_type_term(const char next)
    -> term_kind {

    if (next == '{') {
        return term_kind::record_literal;
    }

    if (next == '[') {
        return term_kind::array_literal;
    }

    if (next == '.') {
        return term_kind::constructor_call;
    }

    return term_kind::record_type;
}

// what 'tk', with 'tz' after it, denotes where a value or a statement starts;
// the rules of hiding: a variable, a parameter or a constant hides a user type
// and a generic function of its name, a call 'f(...)' still calls the
// function; built-in types, built-in functions and keywords are reserved
// names, nothing hides them
[[nodiscard]] inline auto classify_term(const toc& tc, const token& tk,
                                        const tokenizer& tz) -> term_kind {

    if (tk.is_string()) {
        return term_kind::string;
    }

    if (tk.is_none()) {
        return term_kind::none;
    }

    const std::string_view text{tk.text()};

    if (constant_parser::is_literal(tk)) {
        return term_kind::literal;
    }

    if (reserved_names::find_builtin_function(text) != nullptr) {
        return term_kind::builtin_function;
    }

    const char next{tz.peek_char_after_whitespace()};

    if (tc.types().has(text) and tc.types().get(text).is_builtin()) {
        return classify_builtin_type_term(next);
    }

    // a type parameter hides a global of its name, e.g. the function 'T' in
    // the 'T(5)' of 'func f<T type>()'
    const bool is_type_param{tc.type_param_named(text) != nullptr};

    // e.g. 'let point = 3' makes 'point' in 'point + 1' the constant, not the
    // type
    const bool is_hidden{
        not is_type_param and tc.scopes().lookup(text).is_value(),
    };

    // a keyword before '(' is not a call, e.g. 'not (a == 2)'
    // note: 'self' is the one keyword that names a value
    if (not is_hidden and
        std::ranges::contains(reserved_names::keywords, text)) {

        return term_kind::keyword;
    }

    // note: a call 'f()' still calls a hidden function, a hidden 'f<'
    //       starts a comparison

    if (not is_type_param and tc.generics().has_func(text) and
        (next == '(' or (tz.is_type_args_next() and not is_hidden))) {

        return term_kind::generic_call;
    }

    if (next == '(') {
        // note: a function may be named like a type, e.g. 'point(3)'
        if (is_type_param or (not is_hidden and tc.types().has(text) and
                              not tc.funcs().has(text))) {

            return term_kind::type_called;
        }

        return term_kind::call;
    }

    if (is_hidden) {
        return term_kind::value;
    }

    if (tc.types().has(text)) {
        return classify_record_type_term(next);
    }

    if (tc.is_func(text)) {
        return term_kind::function_name;
    }

    return term_kind::unknown;
}

// the integer type of a conversion, e.g. 'i8' of 'i8(x)'; 'bool' and 'void'
// have none, 'get_type_or_throw' rejects 'void' and a type wider than the
// target
[[nodiscard]] inline auto conversion_type(const toc& tc, const token& tk)
    -> const type& {

    const type& converted{tc.get_type_or_throw(tk)};

    if (not converted.is_integer()) {
        throw compile_error{tk, "'bool' has no conversion, compare "
                                "instead, e.g. 'x != 0'"};
    }

    return converted;
}

// the type of the value of the built-in at 'tk', classified as 'tm', e.g. 'i8'
// of 'i8(1)', none when 'tk' names no built-in with a value;
// 'parse_builtin' makes a statement for each
[[nodiscard]] inline auto builtin_value_type(const toc& tc, const token& tk,
                                             const term_kind tm)
    -> const type* {

    if (tm == term_kind::conversion) {
        return &conversion_type(tc, tk);
    }

    if (tm != term_kind::builtin_function) {
        return nullptr;
    }

    const reserved_names::builtin_value value{
        reserved_names::find_builtin_function(tk.text())->value,
    };

    // e.g. the count of 'read(0, buf, 4)'
    if (value == reserved_names::builtin_value::count) {
        return &tc.builtins().default_type();
    }

    if (value == reserved_names::builtin_value::boolean) {
        return &tc.builtins().boolean();
    }

    return nullptr;
}

// a call of a generic function without type arguments is the whole value,
// e.g. 'make()' of 'b = make()', its result type is that of the destination
[[nodiscard]] inline auto is_whole_generic_call(const toc& tc,
                                                const tokenizer& tz) -> bool {

    tokenizer probe{tz};
    const token tk{probe.next_token()};

    if (classify_term(tc, tk, probe) != term_kind::generic_call or
        not probe.accept_char_token('(')) {

        return false;
    }

    probe.skip_to_close_paren();

    return not continues_expression(probe);
}

// e.g. 'i8' in 'var x = i8' is 'i8(0)' and 'bool' in 'var b = bool' is 'false',
// 'i8 + 1' or 'bool and true' continue an expression
[[nodiscard]] inline auto is_bare_builtin_type(const toc& tc, const token& tk,
                                               const tokenizer& tz) -> bool {

    return classify_term(tc, tk, tz) == term_kind::builtin_type and
           not continues_expression(tz);
}

// skips the literal at 'probe': an optional '[...]', then an optional
// '{...}', e.g. the '[2]{1, 2}' of '[2]{1, 2} == a'; false when one does not
// end
[[nodiscard]] inline auto skip_literal(tokenizer& probe) -> bool {
    if (probe.peek_char_after_whitespace() == '[' and
        not probe.skip_enclosed('[', ']')) {

        return false;
    }

    return probe.peek_char_after_whitespace() != '{' or
           probe.skip_braced_block();
}

// the literal at 'tz', see 'skip_literal', is not an operand of a condition,
// e.g. 'i8[2]{1, 2}' or '[2] + 1', not '[2]{1, 2} == a'; 'tz' is after the
// type name of a typed literal
// note: a literal that does not end is parsed as a whole, which reports it
[[nodiscard]] inline auto is_literal_value(const tokenizer& tz) -> bool {
    tokenizer after_literal{tz};

    return not skip_literal(after_literal) or
           not continues_condition(after_literal);
}

// a user type alone or an instance literal that is not an operand of a
// condition, e.g. 'point' or 'point{1, 2} + 1', not 'point{1, 2} == p'
[[nodiscard]] inline auto is_record_value(const term_kind tm,
                                          const tokenizer& tz) -> bool {

    return (tm == term_kind::record_type or tm == term_kind::record_literal) and
           is_literal_value(tz);
}

// a literal ends where 'tz' is and a field or an element of it is named, e.g.
// the '.x' of 'pt{1, 2}.x' or the '[1]' of 'i8[]{1, 2}[1]'
[[nodiscard]] inline auto is_reached_into(const tokenizer& tz) -> bool {
    const char next{tz.peek_char_after_whitespace()};
    return next == '.' or next == '[';
}

// an array or instance literal at 'tz' that is reached into, e.g.
// 'pt{1, 2}.x'; 'tz' is after the type name of a typed literal
[[nodiscard]] inline auto is_literal_reached_into(const tokenizer& tz) -> bool {
    tokenizer after_literal{tz};
    return skip_literal(after_literal) and is_reached_into(after_literal);
}

// a literal or a call result has no storage, e.g. 'literal' is "an array
// literal", "an instance literal of 'point'" or "'mk()'"
[[noreturn]] inline auto
throw_literal_not_operand(const token& tk, const std::string_view literal)
    -> void {

    throw compile_error{tk, std::format("{} is not an operand here, store it "
                                        "in a variable first",
                                        literal)};
}

// an instance literal of 'type_name' has no storage, e.g. 'point{1, 2}.x'
[[noreturn]] inline auto
throw_instance_literal_not_operand(const token& tk,
                                   const std::string_view type_name) -> void {

    throw_literal_not_operand(
        tk, std::format("an instance literal of {}", in_quotes(type_name)));
}

// a value named like a type hides the type, e.g. 'point' after
// 'var point = 1'
[[noreturn]] inline auto throw_value_hides_type(const token& tk,
                                                const std::string_view name)
    -> void {

    throw compile_error{tk, std::format("{} names a value here, which hides "
                                        "the type {}",
                                        in_quotes(name), in_quotes(name))};
}
