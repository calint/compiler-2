#pragma once

// the statement factories declared in 'decouple.hpp': they create the
// statements of a block and the elements of an expression, and 'stmt_block'
// and 'expr_arith', which call them, are included by those classes; the
// definitions of the top level are made by 'program::parse_definition'

#include <algorithm>
#include <format>
#include <memory>
#include <utility>

#include "compile_error.hpp"
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
#include "stmt_def_var.hpp"
#include "stmt_identifier.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"
#include "stmt_return.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

// 'exit' ends the program, 'array_copy' and the sector functions only
// transfer, they have no value to use
inline auto assert_builtin_has_value(const token& tk, const tokenizer& tz)
    -> void {

    const stmt_builtin_io::io_function* const io{
        stmt_builtin_io::find(tk.text()),
    };

    const bool has_no_value{
        tk.is_text("exit") or tk.is_text("array_copy") or
            (io != nullptr and not io->has_result()),
    };

    if (has_no_value and tz.peek_char_after_whitespace() == '(') {
        throw compile_error{tk, "function does not return a value"};
    }
}

// a built-in is not generic, e.g. 'read<i8>(0, buf)'
inline auto assert_builtin_without_type_args(const token& tk,
                                             const tokenizer& tz) -> void {

    if (not std::ranges::contains(reserved_names::builtin_functions,
                                  tk.text()) or
        tz.peek_char_after_whitespace() != '<') {

        return;
    }

    throw compile_error{
        tk, std::format("built-in function '{}' takes no type arguments",
                        tk.text())};
}

// the type of the value of the built-in at 'tk', e.g. 'i8' of 'i8(1)', none
// when 'tk' names no built-in with a value; 'create_statement_in_expr_arith'
// makes a statement for each
[[nodiscard]] inline auto builtin_value_type(const toc& tc, const token& tk,
                                             const tokenizer& tz)
    -> const type* {

    if (tk.is_text("array_length")) {
        return &tc.builtins().default_type();
    }

    if (tk.is_text("arrays_equal")) {
        return &tc.builtins().boolean();
    }

    // e.g. the count of 'read(0, buf, 4)'
    const stmt_builtin_io::io_function* const io{
        stmt_builtin_io::find(tk.text()),
    };

    if (io != nullptr and io->has_result()) {
        return &tc.builtins().default_type();
    }

    if (names_integer_type(tc, tk) and tz.peek_char_after_whitespace() == '(') {

        return &stmt_builtin_convert::conversion_type(tc, tk);
    }

    return nullptr;
}

// every statement of a block that starts with a keyword or a built-in, null
// for a name; called from 'stmt_block', which the statements with a block
// include
inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                           const token tk)
    -> std::unique_ptr<statement> {

    assert_builtin_without_type_args(tk, tz);

    if (tk.is_text("var") or tk.is_text("let")) {
        return make_stmt_def_var(tc, tk, tz);
    }

    // rejected before its initializer or body, whose errors would hide this
    // one
    if (tk.is_text("dat") or tk.is_text("func") or tk.is_text("type")) {
        throw compile_error{
            tk, std::format("'{}' can only be declared in global scope",
                            tk.text())};
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
        return std::make_unique<stmt_return>(tc, tk);
    }

    if (tk.is_text("array_copy")) {
        return std::make_unique<stmt_builtin_array_copy>(tc, tk, tz);
    }

    if (tk.is_text("loop")) {
        return std::make_unique<stmt_loop>(tc, tk, tz);
    }

    if (tk.is_text("if")) {
        return std::make_unique<stmt_if>(tc, tk, tz);
    }

    if (tk.is_text("exit")) {
        return std::make_unique<stmt_builtin_exit>(tc, tk, tz);
    }

    if (const stmt_builtin_io::io_function* const io{
            stmt_builtin_io::find(tk.text()),
        }) {

        return std::make_unique<stmt_builtin_io>(tc, *io, unary_ops{}, tk, tz);
    }

    if (tk.is_text(reserved_names::foo)) {
        return std::make_unique<stmt_builtin_foo>(tc, tk, tz);
    }

    // e.g. 'arrays_equal(a, b, 4)' alone, its value is all it does
    if (builtin_value_type(tc, tk, tz) != nullptr) {
        throw compile_error{tk, "return value is discarded"};
    }

    return nullptr;
}

// e.g. 'show<name>(x)' where 'show' is not generic, says so instead of reading
// the '<' as an operator
inline auto assert_no_type_args_for_plain_func(const toc& tc, const token& tk,
                                               const tokenizer& tz) -> void {

    // a variable or a constant of that name is an operand of '<'
    if (not tc.is_func(tk.text()) or tc.generics().has_func(tk.text()) or
        is_hidden_by_value(tc, tk.text()) or
        tz.peek_char_after_whitespace() != '<') {

        return;
    }

    throw compile_error{
        tk, std::format("function '{}' is not generic, it takes no type "
                        "arguments",
                        tk.text())};
}

// a call in an expression is a value, a function without a result has none
inline auto value_call(const toc& tc, std::unique_ptr<stmt_call> call)
    -> std::unique_ptr<statement> {

    if (call->get_type().is_same(tc.builtins().void_type())) {
        throw compile_error{call->tok(), "function does not return a value"};
    }

    return call;
}

// a call, or an identifier that is not the receiver of a method
struct call_or_identifier {
    std::unique_ptr<stmt_call> call;
    std::unique_ptr<stmt_identifier> identifier;
};

// parses what follows the token 'tk' as a call of a function, a generic
// function, a constructor or a method, otherwise as an identifier, 'uops' go to
// the call or the identifier and 'expected_type' is the type the call is
// assigned to or null
inline auto parse_call_or_identifier(toc& tc, tokenizer& tz, const token& tk,
                                     unary_ops uops,
                                     const type* const expected_type)
    -> call_or_identifier {

    // e.g. 'point{1, 2}' after 'var point = 1', the value hides the type
    if (tc.types().has(tk.text()) and is_hidden_by_value(tc, tk.text()) and
        tz.peek_char_after_whitespace() == '{') {

        throw compile_error{
            tk, std::format("'{}' names a value here, which hides the type "
                            "'{}'",
                            tk.text(), tk.text())};
    }

    // e.g. 'foo(x)' or 'show<name>(x)', the call reads the type arguments
    if (tz.peek_char_after_whitespace() == '(' or
        is_generic_call(tc, tk.text(), tz)) {

        return {
            .call{
                std::make_unique<stmt_call>(tc, std::move(uops), tk, tz,
                                            expected_type),
            },
            .identifier{},
        };
    }

    assert_no_type_args_for_plain_func(tc, tk, tz);

    // e.g. 'point.at(1, 2)'
    if (is_constructor_call(tc, tk, tz)) {
        return {
            .call{
                std::make_unique<stmt_call>(constructor_call_tag{}, tc,
                                            std::move(uops), tk, tz),
            },
            .identifier{},
        };
    }

    // e.g. 'x', 'p.x', 'a[i]' or the literal '0x80'
    stmt_identifier si{tc, {}, tk, tz};

    // e.g. '-lst.size()' negates the result, not the receiver
    if (si.is_method_receiver()) {
        return {
            .call{
                std::make_unique<stmt_call>(tc, std::move(uops), std::move(si),
                                            tz, expected_type),
            },
            .identifier{},
        };
    }

    si.set_unary_ops(std::move(uops));

    return {
        .call{},
        .identifier{std::make_unique<stmt_identifier>(std::move(si))},
    };
}

// called from 'stmt_block'
// a call as in an expression, e.g. 'f(x)', 'point.at(1, 2)' or 'lst.add(x)',
// otherwise an assignment, e.g. 'p1 = p2'
// note: 'unary_ops' not allowed before destination identifier
inline auto create_statement_of_identifier(toc& tc, tokenizer& tz,
                                           const token tk)
    -> std::unique_ptr<statement> {

    // no destination: no type argument is deduced, and a discarded result is
    // rejected at compile time like any other call
    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tk, unary_ops{}, nullptr),
    };

    if (parsed.call) {
        return std::move(parsed.call);
    }

    // e.g. 'p1 = p2'
    if (const token equals_tk{tz.is_next_char_token('=')};
        not equals_tk.is_empty()) {

        return std::make_unique<stmt_assign_var>(
            tc, tz, std::move(*parsed.identifier), equals_tk);
    }

    throw compile_error{
        tz, "unexpected character, expected '=' for assignment or '(' "
            "for function call"};
}

// called from 'expr_arith' to solve circular dependencies with function
// calls
inline auto create_statement_in_expr_arith(toc& tc, tokenizer& tz,
                                           const type* const destination_type)
    -> std::unique_ptr<statement> {

    unary_ops uops{tz};
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
        throw compile_error{tk,
                            "expected constant, identifier, or function call"};
    }

    // e.g. the 'not b' of 'int(not b)' or of 'a == not b'
    if (tk.is_text("not")) {
        throw compile_error{tk, "'not' makes a condition, which is not an "
                                "operand here, store it in a 'bool' first"};
    }

    // e.g. the 'point{1, 2}' of 'p == point{1, 2}', an instance is compared
    // as memory, which only a variable has
    if (is_record_literal(tc, tk, tz)) {
        throw compile_error{
            tk, std::format("an instance literal of '{}' is not an operand "
                            "here, store it in a variable first",
                            tk.text())};
    }

    assert_builtin_without_type_args(tk, tz);

    const stmt_builtin_io::io_function* const io{
        stmt_builtin_io::find(tk.text()),
    };

    if (io != nullptr and io->has_result()) {
        return std::make_unique<stmt_builtin_io>(tc, *io, std::move(uops), tk,
                                                 tz);
    }

    assert_builtin_has_value(tk, tz);

    if (tk.is_text("array_length")) {
        return std::make_unique<stmt_builtin_array_length>(tc, std::move(uops),
                                                           tk, tz);
    }

    if (tk.is_text("arrays_equal")) {
        return std::make_unique<stmt_builtin_arrays_equal>(tc, std::move(uops),
                                                           tk, tz);
    }

    if (names_integer_type(tc, tk) and tz.peek_char_after_whitespace() == '(') {

        return std::make_unique<stmt_builtin_convert>(tc, std::move(uops), tk,
                                                      tz);
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tk, std::move(uops), destination_type),
    };

    if (parsed.call) {
        return value_call(tc, std::move(parsed.call));
    }

    return std::move(parsed.identifier);
}
