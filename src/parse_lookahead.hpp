#pragma once

// what the next tokens are, decided before they are parsed: 'classify_term'
// tells what the first token of a value or a statement denotes; a name
// followed by a character has one meaning only, e.g. 'point{' is a record
// literal and 'show<' a generic call; the name after a '.' is decided by
// 'stmt_identifier::is_method_name', a function head by 'func_head'

#include <algorithm>
#include <cstdint>
#include <string_view>

#include "constant_parser.hpp"

#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// e.g. '[]{1, 2}' has the default type, no expression starts with '['
[[nodiscard]] inline auto is_default_array_literal(const tokenizer& tz)
    -> bool {

    return tz.peek_char_after_whitespace() == '[';
}

// a binary operator or a connective follows, which would continue the
// expression, e.g. the '+' of 'x + 1' or the 'and' of 'b and c'
[[nodiscard]] inline auto continues_expression(const tokenizer& tz) -> bool {
    if (tz.is_binary_operator_next()) {
        return true;
    }

    tokenizer probe{tz};
    const token next_tk{probe.next_token()};

    return connective_of(next_tk).has_value();
}

// '{}' follows, the zero value of a typed destination
[[nodiscard]] inline auto is_zero_value(const tokenizer& tz) -> bool {
    tokenizer probe{tz};

    return probe.is_next_char_token('{') and
           probe.peek_char_after_whitespace() == '}';
}

// what the token at the start of a value or a statement is, see
// 'classify_term'
enum class term_kind : uint8_t {
    // no token, e.g. the end of the source or a delimiter
    none,
    // e.g. "abc"
    string,
    // e.g. '12', 'a', 'true'
    literal,
    // e.g. 'array_length' or 'read'
    builtin_function,
    // a built-in type followed by '(', e.g. 'i8(x)', only the integer types
    // convert
    conversion,
    // a built-in type that is not converted or the element type of an array
    // literal, e.g. the 'i8' of 'var x = i8'
    builtin_type,
    // a type followed by '[', e.g. 'i8[]{1, 2}'
    array_literal,
    // a user type followed by '{', e.g. 'point{1, 2}'
    record_literal,
    // a user type followed by '.', e.g. 'point.at(1, 2)'
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

// what 'classify_term' found
struct term {
    term_kind kind{term_kind::none};
    // the type of a type kind, e.g. 'i8' for a type parameter bound to it;
    // set for 'conversion', 'builtin_type', 'array_literal', 'record_literal',
    // 'constructor_call', 'record_type' and 'type_called', null otherwise
    // note: what the name means; a value of the type is asked for by
    //       'toc::get_type_or_throw', which also rejects 'void' and a type
    //       wider than the target computes, e.g. 'i64' on rv32i
    const type* named_type{};

    [[nodiscard]] auto is(const term_kind other) const -> bool {
        return kind == other;
    }

    // a user or built-in type that is not converted, called or made an
    // instance of by a constructor
    [[nodiscard]] auto is_bare_type() const -> bool {
        return kind == term_kind::builtin_type or
               kind == term_kind::record_type;
    }
};

// what 'tk', with 'tz' after it, denotes where a value or a statement starts;
// the rules of hiding: a variable, a parameter or a constant
// hides a user type and a generic function of its name, a call 'f(...)'
// still calls the function; built-in types, built-in functions and keywords
// are reserved names, nothing hides them
[[nodiscard]] inline auto classify_term(const toc& tc, const token& tk,
                                        const tokenizer& tz) -> term {

    if (tk.is_string()) {
        return {
            .kind{term_kind::string},
            .named_type{},
        };
    }

    if (tk.is_none()) {
        return {
            .kind{term_kind::none},
            .named_type{},
        };
    }

    const std::string_view text{tk.text()};

    if (constant_parser::is_literal(tk)) {

        return {
            .kind{term_kind::literal},
            .named_type{},
        };
    }

    if (reserved_names::find_builtin_function(text) != nullptr) {
        return {
            .kind{term_kind::builtin_function},
            .named_type{},
        };
    }

    const char next{tz.peek_char_after_whitespace()};

    if (tc.types().has(text) and tc.types().get(text).is_builtin()) {
        const type* const builtin{&tc.types().get(text)};

        if (next == '(') {
            return {
                .kind{term_kind::conversion},
                .named_type{builtin},
            };
        }

        if (next == '[') {
            return {
                .kind{term_kind::array_literal},
                .named_type{builtin},
            };
        }

        return {
            .kind{term_kind::builtin_type},
            .named_type{builtin},
        };
    }

    // e.g. 'let point = 3' makes 'point' in 'point + 1' the constant, not the
    // type
    const bool is_hidden{tc.scopes().lookup(text).is_value()};

    if (not is_hidden and tc.generics().has_func(text) and
        (next == '<' or next == '(')) {

        return {
            .kind{term_kind::generic_call},
            .named_type{},
        };
    }

    if (next == '(') {
        // note: a function may be named like a type, e.g. 'point(3)'
        if (not is_hidden and tc.types().has(text) and
            not tc.funcs().has(text)) {

            return {
                .kind{term_kind::type_called},
                .named_type{&tc.types().get(text)},
            };
        }

        return {
            .kind{term_kind::call},
            .named_type{},
        };
    }

    if (is_hidden) {
        return {
            .kind{term_kind::value},
            .named_type{},
        };
    }

    if (not tc.types().has(text)) {
        if (tc.is_func(text)) {
            return {
                .kind{term_kind::function_name},
                .named_type{},
            };
        }

        if (std::ranges::contains(reserved_names::keywords, text)) {
            return {
                .kind{term_kind::keyword},
                .named_type{},
            };
        }

        return {
            .kind{term_kind::unknown},
            .named_type{},
        };
    }

    const type* const named{&tc.types().get(text)};

    if (next == '{') {
        return {
            .kind{term_kind::record_literal},
            .named_type{named},
        };
    }

    if (next == '[') {
        return {
            .kind{term_kind::array_literal},
            .named_type{named},
        };
    }

    if (next == '.') {
        return {
            .kind{term_kind::constructor_call},
            .named_type{named},
        };
    }

    return {
        .kind{term_kind::record_type},
        .named_type{named},
    };
}

// a call of a generic function without type arguments is the whole value,
// e.g. 'make()' of 'b = make()', its result type is that of the destination
[[nodiscard]] inline auto is_whole_generic_call(const toc& tc,
                                                const tokenizer& tz) -> bool {

    tokenizer probe{tz};
    const token tk{probe.next_token()};

    if (not classify_term(tc, tk, probe).is(term_kind::generic_call) or
        not probe.is_next_char_token('(')) {

        return false;
    }

    probe.skip_to_close_paren();

    return not continues_expression(probe);
}

// e.g. 'i8' in 'var x = i8' is 'i8(0)' and 'bool' in 'var b = bool' is 'false',
// 'i8 + 1' or 'bool and true' continue an expression
[[nodiscard]] inline auto is_bare_builtin_type(const toc& tc, const token& tk,
                                               const tokenizer& tz) -> bool {

    return classify_term(tc, tk, tz).is(term_kind::builtin_type) and
           not continues_expression(tz);
}
