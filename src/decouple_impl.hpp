#pragma once

// the statement factories declared in 'decouple.hpp': they create every
// statement class, and 'stmt_block' and 'expr_arith', which call them, are
// included by those classes

#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_type.hpp"
#include "generics.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_builtin_array_length.hpp"
#include "stmt_builtin_arrays_equal.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_builtin_exit.hpp"
#include "stmt_builtin_foo.hpp"
#include "stmt_builtin_io.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// called from 'stmt_block' to solve circular dependencies with 'loop', 'if',
// 'exit', the io functions and 'foo', null for any other keyword
inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                           const token tk)
    -> std::unique_ptr<statement> {

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

    return nullptr;
}

// e.g. 'show<name>(x)' where 'show' is not generic, says so instead of reading
// the '<' as an operator
inline auto assert_no_type_args_for_plain_func(const toc& tc, const token& tk,
                                               tokenizer& tz) -> void {

    // a variable of that name is an operand of '<'
    if (not tc.is_func(tk.text()) or tc.generics().has_func(tk.text()) or
        tc.scopes().is_var_or_alias(tk.text()) or
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
// the call and 'expected_type' is the type the call is assigned to or null
inline auto parse_call_or_identifier(toc& tc, tokenizer& tz, const token& tk,
                                     unary_ops uops,
                                     const type* const expected_type)
    -> call_or_identifier {

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

    // e.g. 0x80, rax, identifiers, constants
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

    // a call statement has no destination to deduce a type argument from
    // the discarded result is rejected at compile time like any other call
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
inline auto create_statement_in_expr_arith(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement> {

    unary_ops uops{tz};
    const token tk{tz.next_token()};

    // array destinations parse strings in 'expr_any'
    if (tk.is_string()) {
        throw compile_error{
            tk, "a string can only be assigned to an array of type 'i8'"};
    }

    if (tk.text().empty()) {
        throw compile_error{tk,
                            "expected constant, identifier, or function call"};
    }

    const stmt_builtin_io::io_function* const io{
        stmt_builtin_io::find(tk.text()),
    };

    if (io and io->has_result()) {
        return std::make_unique<stmt_builtin_io>(tc, *io, std::move(uops), tk,
                                                 tz);
    }

    // 'exit' ends the program and the sector functions only transfer, they
    // have no value to use
    if ((tk.is_text("exit") or io) and tz.peek_char_after_whitespace() == '(') {

        throw compile_error{tk, "function does not return a value"};
    }

    if (tk.is_text("array_length")) {
        return std::make_unique<stmt_builtin_array_length>(tc, std::move(uops),
                                                           tk, tz);
    }

    if (tk.is_text("arrays_equal")) {
        return std::make_unique<stmt_builtin_arrays_equal>(tc, std::move(uops),
                                                           tk, tz);
    }

    if (tc.builtins().is_integer_name(tk.text()) and
        tz.peek_char_after_whitespace() == '(') {

        return std::make_unique<stmt_builtin_convert>(tc, std::move(uops), tk,
                                                      tz);
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tk, uops, nullptr),
    };

    if (parsed.call) {
        return value_call(tc, std::move(parsed.call));
    }

    parsed.identifier->set_unary_ops(std::move(uops));

    return std::move(parsed.identifier);
}
