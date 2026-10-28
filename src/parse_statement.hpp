#pragma once

// the grammar: a definition of the top level ('parse_definition'), a
// statement of a block ('parse_statement') and an operand of an expression
// ('parse_operand'), the last two declared in 'decouple.hpp' because
// 'stmt_block' and 'expr_arith', which call them, are included by the
// statements they make; 'program' keeps the files and their includes

#include <cassert>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "decouple.hpp"
#include "parse_lookahead.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_break.hpp"
#include "stmt_builtin_array_copy.hpp"
#include "stmt_builtin_array_length.hpp"
#include "stmt_builtin_arrays_equal.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_builtin_exit.hpp"
#include "stmt_builtin_foo.hpp"
#include "stmt_builtin_io.hpp"
#include "stmt_call.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_dat.hpp"
#include "stmt_def_func.hpp"
#include "stmt_def_type.hpp"
#include "stmt_def_var.hpp"
#include "stmt_identifier.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"
#include "stmt_return.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

// a call of a function without a result where a value is needed, e.g. 'x =
// array_copy(a, b, 1)'
[[noreturn]] inline auto throw_no_value(const token& call_tk) -> void {
    throw compile_error{call_tk, "function does not return a value"};
}

// a call with a result as a statement of its own, e.g. 'f(x)'
[[noreturn]] inline auto throw_discarded_result(const token& call_tk) -> void {
    throw compile_error{call_tk, "return value is discarded"};
}

// a built-in is not generic, e.g. 'read<i8>(0, buf)'
inline auto assert_builtin_without_type_args(const token& tk,
                                             const tokenizer& tz) -> void {

    if (reserved_names::find_builtin_function(tk.text()) == nullptr or
        tz.peek_char_after_whitespace() != '<') {

        return;
    }

    throw compile_error{
        tk, std::format("built-in function {} takes no type arguments",
                        in_quotes(tk.text()))};
}

// e.g. 'i8(x)'; 'bool' has no conversion
[[nodiscard]] inline auto is_integer_conversion(const toc& tc, const token& tk,
                                                const term_kind tm) -> bool {

    return tm == term_kind::conversion and
           tc.types().get(tk.text()).is_integer();
}

// the type of the value of the built-in at 'tk', classified as 'tm', e.g. 'i8'
// of 'i8(1)', none when 'tk' names no built-in with a value;
// 'parse_builtin' makes a statement for each
[[nodiscard]] inline auto builtin_value_type(const toc& tc, const token& tk,
                                             const term_kind tm)
    -> const type* {

    if (tm == term_kind::conversion) {
        if (not is_integer_conversion(tc, tk, tm)) {
            return nullptr;
        }

        return &tc.get_type_or_throw(tk);
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

// where a call of a built-in is parsed: in a value, or alone as a statement
enum class builtin_place : uint8_t { value, statement };

// a call of the built-in at 'tk', classified as 'tm', e.g. 'i8(x)',
// 'array_copy(a, b, 4)' or 'read(0, b, n)', null when 'tk' names none; the
// catalog says where it may stand, e.g. 'exit' has no value and
// 'arrays_equal' is no statement; 'uops' are taken only by a statement made
inline auto parse_builtin(toc& tc, tokenizer& tz, const token& tk,
                          const term_kind tm, unary_ops& uops,
                          const builtin_place at)
    -> std::unique_ptr<statement> {

    assert_builtin_without_type_args(tk, tz);

    const reserved_names::builtin_function* const fn{
        reserved_names::find_builtin_function(tk.text()),
    };

    if (fn == nullptr and not is_integer_conversion(tc, tk, tm)) {
        return nullptr;
    }

    // a conversion has a value only
    const reserved_names::builtin_use use{
        fn == nullptr ? reserved_names::builtin_use::value : fn->use,
    };

    if (at == builtin_place::value and
        use == reserved_names::builtin_use::statement) {

        throw_no_value(tk);
    }

    if (at == builtin_place::statement and
        use == reserved_names::builtin_use::value) {

        throw_discarded_result(tk);
    }

    if (const stmt_builtin_io::io_function* const io{
            stmt_builtin_io::find(tk.text()),
        }) {

        return std::make_unique<stmt_builtin_io>(tc, *io, std::move(uops), tk,
                                                 tz);
    }

    if (tk.is_text("array_copy")) {
        return std::make_unique<stmt_builtin_array_copy>(tc, tk, tz);
    }

    if (tk.is_text("exit")) {
        return std::make_unique<stmt_builtin_exit>(tc, tk, tz);
    }

    if (tk.is_text("array_length")) {
        return std::make_unique<stmt_builtin_array_length>(tc, std::move(uops),
                                                           tk, tz);
    }

    if (tk.is_text("arrays_equal")) {
        return std::make_unique<stmt_builtin_arrays_equal>(tc, std::move(uops),
                                                           tk, tz);
    }

    // note: every built-in of 'reserved_names::builtin_functions' has a
    //       branch above, what is left is an integer conversion

    assert(fn == nullptr);

    return std::make_unique<stmt_builtin_convert>(tc, std::move(uops), tk, tz);
}

// a statement that starts with a keyword or a built-in, null for a name
inline auto parse_keyword_statement(toc& tc, tokenizer& tz, const token tk)
    -> std::unique_ptr<statement> {

    if (tk.is_text("var") or tk.is_text("let")) {
        return make_stmt_def_var(tc, tk, tz);
    }

    // rejected before its initializer or body, whose errors would hide this
    // one
    if (tk.is_text("dat") or tk.is_text("func") or tk.is_text("type")) {
        throw compile_error{
            tk, std::format("{} can only be declared in global scope",
                            in_quotes(tk.text()))};
    }

    if (tk.is_text("include")) {
        throw compile_error{tk, "'include' can only be in global scope"};
    }

    // e.g. a second 'else', or an 'else' after a statement that is not 'if'
    if (tk.is_text("else")) {
        throw compile_error{tk, "'else' without 'if'"};
    }

    if (tk.is_text("break")) {
        return std::make_unique<stmt_break>(tc, tk);
    }

    if (tk.is_text("continue")) {
        return std::make_unique<stmt_continue>(tc, tk);
    }

    if (tk.is_text("return")) {
        return std::make_unique<stmt_return>(tk);
    }

    if (tk.is_text("loop")) {
        return std::make_unique<stmt_loop>(tc, tk, tz);
    }

    if (tk.is_text("if")) {
        return std::make_unique<stmt_if>(tc, tk, tz);
    }

    if (tk.is_text(reserved_names::foo)) {
        return std::make_unique<stmt_builtin_foo>(tc, tk, tz);
    }

    // e.g. 'arrays_equal(a, b, 4)' or 'i8(x)' alone is rejected, its value is
    // all it does
    unary_ops no_uops;

    return parse_builtin(tc, tz, tk, classify_term(tc, tk, tz), no_uops,
                         builtin_place::statement);
}

// e.g. 'show<name>(x)' where 'show' is not generic, says so instead of reading
// the '<' as an operator; a variable or a constant of that name is an operand
// of '<'
inline auto assert_no_type_args_for_plain_func(const token& tk,
                                               const term_kind tm,
                                               const tokenizer& tz) -> void {

    if (tm != term_kind::function_name or
        tz.peek_char_after_whitespace() != '<') {

        return;
    }

    throw compile_error{
        tk, std::format("function {} is not generic, it takes no type "
                        "arguments",
                        in_quotes(tk.text()))};
}

// a call in an expression is a value, a function without a result has none
inline auto value_call(const toc& tc, std::unique_ptr<stmt_call> call)
    -> std::unique_ptr<statement> {

    if (call->get_type().is_same(tc.builtins().void_type())) {
        throw_no_value(call->tok());
    }

    return call;
}

// a call, or an identifier that is not the receiver of a method
using call_or_identifier =
    std::variant<std::unique_ptr<stmt_call>, std::unique_ptr<stmt_identifier>>;

// parses what follows the token 'tk', classified as 'tm', as a call of a
// function, a generic function, a constructor or a method, otherwise as an
// identifier, 'uops' go to the call or the identifier and 'expected_type' is
// the type the call is assigned to or null
inline auto parse_call_or_identifier(toc& tc, tokenizer& tz, const token& tk,
                                     const term_kind tm, unary_ops uops,
                                     const type* const expected_type)
    -> call_or_identifier {

    const char next{tz.peek_char_after_whitespace()};

    // e.g. 'point{1, 2}' after 'var point = 1', the value hides the type
    if (tm == term_kind::value and tc.types().has(tk.text()) and next == '{') {
        throw compile_error{
            tk, std::format("{} names a value here, which hides the type "
                            "{}",
                            in_quotes(tk.text()), in_quotes(tk.text()))};
    }

    // e.g. '1 (2)', a number is not called
    if (tk.is_number() and next == '(') {
        throw compile_error{tk, std::format("{} is a number, not a function",
                                            in_quotes(tk.text()))};
    }

    // e.g. 'bool(5)', only the integer types convert
    if (tm == term_kind::conversion and tc.types().get(tk.text()).is_bool()) {
        throw compile_error{tk, "'bool' has no conversion, compare "
                                "instead, e.g. 'x != 0'"};
    }

    // e.g. 'point(5)'
    if (tm == term_kind::type_called) {
        throw compile_error{
            tk, std::format("{} is a type, not a function, an instance "
                            "is made with '{}{{...}}'",
                            in_quotes(tk.text()), tk.text())};
    }

    // e.g. 'f(x)' or 'show<name>(x)', the call reads the type arguments
    if (next == '(' or tm == term_kind::generic_call) {
        return std::make_unique<stmt_call>(tc, std::move(uops), tk, tz,
                                           expected_type);
    }

    assert_no_type_args_for_plain_func(tk, tm, tz);

    // e.g. 'point.at(1, 2)'
    if (tm == term_kind::constructor_call) {
        return std::make_unique<stmt_call>(constructor_call_tag{}, tc,
                                           std::move(uops), tk, tz);
    }

    // e.g. the 'f' of 'x = f + 1'; the resolver tells a generic function
    // that it needs type arguments
    if (tm == term_kind::function_name and
        not tc.generics().has_func(tk.text())) {

        throw_function_as_value(tk, tk.text());
    }

    // e.g. 'x', 'p.x', 'a[i]' or the literal '0x80'
    stmt_identifier ident{tc, tk, tz};

    // e.g. '-lst.size()' negates the result, not the receiver
    if (ident.is_method_receiver()) {
        return std::make_unique<stmt_call>(tc, std::move(uops),
                                           std::move(ident), tz, expected_type);
    }

    // note: the ops are the identifier's only now, a receiver's go to its
    //       call

    ident.set_unary_ops(std::move(uops));

    return std::make_unique<stmt_identifier>(std::move(ident));
}

// a statement that starts with a name: a call as in an expression, e.g.
// 'f(x)', 'point.at(1, 2)' or 'lst.add(x)', otherwise an assignment, e.g.
// 'p1 = p2'
// note: 'unary_ops' not allowed before destination identifier
inline auto parse_name_statement(toc& tc, tokenizer& tz, const token tk)
    -> std::unique_ptr<statement> {

    // no destination: no type argument is deduced, and a result is
    // discarded, rejected here like the result of a built-in
    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tk, classify_term(tc, tk, tz),
                                 unary_ops{}, nullptr),
    };

    if (std::unique_ptr<stmt_call>* const call{
            std::get_if<std::unique_ptr<stmt_call>>(&parsed),
        }) {

        if (not(*call)->get_type().is_same(tc.builtins().void_type())) {
            throw_discarded_result((*call)->tok());
        }

        return std::move(*call);
    }

    stmt_identifier& dst{*std::get<std::unique_ptr<stmt_identifier>>(parsed)};

    // e.g. 'p1 = p2'
    if (const std::optional<token> equals_tk{tz.is_next_char_token('=')}) {
        return std::make_unique<stmt_assign_var>(tc, tz, std::move(dst),
                                                 *equals_tk);
    }

    // at the name, the next token can be on a later line
    throw compile_error{
        dst.tok(),
        std::format("expected '=' for assignment or '(' for function call "
                    "after {}",
                    in_quotes(statement::trimmed_source(dst)))};
}

// called from 'expr_arith', whose elements include it, e.g. a call
inline auto parse_operand(toc& tc, tokenizer& tz, unary_ops uops,
                          const type* const destination_type)
    -> std::unique_ptr<statement> {

    const token tk{tz.next_token()};

    // array destinations parse strings in 'expr_any'
    if (tk.is_string()) {
        throw compile_error{
            tk, "a string is not a number, it fills only an array of 'i8'"};
    }

    // e.g. the '{}' of '1 + {}', which the destination types
    if (tk.is_none() and is_zero_value(tz)) {
        throw compile_error{
            tk,
            "'{}' is the zero value of a typed destination, not an operand"};
    }

    if (tk.is_none()) {
        throw compile_error{tk, "expected a constant, a name or a call"};
    }

    // e.g. the 'not b' of 'int(not b)' or of 'a == not b'
    if (tk.is_text("not")) {
        throw compile_error{tk, "'not' makes a condition, which is not an "
                                "operand here, store it in a 'bool' first"};
    }

    const term_kind tm{classify_term(tc, tk, tz)};

    // e.g. the 'point{1, 2}' of 'p == point{1, 2}', an instance is compared
    // as memory, which only a variable has
    if (tm == term_kind::record_literal) {
        throw compile_error{
            tk, std::format("an instance literal of {} is not an operand "
                            "here, store it in a variable first",
                            in_quotes(tk.text()))};
    }

    // e.g. the 'i8' of 'i8 + 1', the 'point' of '1 + point' or the 'i8[]' of
    // 'x + i8[]{1}'
    if (is_bare_type(tm) or tm == term_kind::array_literal) {
        throw compile_error{
            tk, std::format("{} is a type, not a value", in_quotes(tk.text()))};
    }

    if (std::unique_ptr<statement> builtin{
            parse_builtin(tc, tz, tk, tm, uops, builtin_place::value),
        }) {

        return builtin;
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tk, tm, std::move(uops),
                                 destination_type),
    };

    if (std::unique_ptr<stmt_call>* const call{
            std::get_if<std::unique_ptr<stmt_call>>(&parsed),
        }) {

        return value_call(tc, std::move(*call));
    }

    return std::move(std::get<std::unique_ptr<stmt_identifier>>(parsed));
}

inline auto parse_statement(toc& tc, tokenizer& tz, const token tk)
    -> std::unique_ptr<statement> {

    // e.g. the '2' of 'return 2', which takes no value, or ''a' = 1'
    if (tk.is_string() or constant_parser::is_literal(tk)) {
        throw compile_error{tk, "expected a statement"};
    }

    if (std::unique_ptr<statement> built{parse_keyword_statement(tc, tz, tk)}) {
        return built;
    }

    // e.g. 'lst.add(1)'
    return parse_name_statement(tc, tz, tk);
}

// a definition of the top level, 'tk' is its first token; only definitions
// are allowed there
[[nodiscard]] inline auto parse_definition(toc& tc, tokenizer& tz,
                                           const token& tk)
    -> std::unique_ptr<statement> {

    if (tk.is_text("func")) {
        return make_stmt_def_func(tc, tk, tz);
    }

    if (tk.is_text("type")) {
        return make_stmt_def_type(tc, tk, tz);
    }

    if (tk.is_text("var") or tk.is_text("let")) {
        return make_stmt_def_var(tc, tk, tz);
    }

    if (tk.is_text("dat")) {
        return std::make_unique<stmt_def_dat>(tc, tk, tz);
    }

    if (tk.is_text("include")) {
        throw compile_error{
            tk, "'include' must be before the definitions of the file"};
    }

    throw compile_error{
        tk, std::format("expected a definition ('func', 'type', 'var', "
                        "'let' or 'dat'), got {}",
                        in_quotes(tk))};
}
