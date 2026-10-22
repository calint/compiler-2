#pragma once

// what the next tokens are, decided before they are parsed: a type name or a
// generic function name followed by a character has one meaning only, e.g.
// 'point{' is a record literal and 'show<' a generic call; a variable, a
// parameter or a constant of that name hides the type or the function

#include <string_view>

#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// e.g. 'let point = 3' makes 'point' in 'point + 1' the constant, not the type
[[nodiscard]] inline auto is_hidden_by_value(const toc& tc,
                                             const std::string_view name)
    -> bool {

    return tc.scopes().lookup(name).is_value();
}

// e.g. 'show<name>(x)', a generic function name followed by '<' has no other
// meaning
[[nodiscard]] inline auto is_generic_call(const toc& tc,
                                          const std::string_view name,
                                          const tokenizer& tz) -> bool {

    return tc.generics().has_func(name) and not is_hidden_by_value(tc, name) and
           tz.peek_char_after_whitespace() == '<';
}

[[nodiscard]] inline auto is_type_name_followed_by(const toc& tc,
                                                   const token& tk,
                                                   const tokenizer& tz,
                                                   const char next) -> bool {

    return tc.types().has(tk.text()) and
           not is_hidden_by_value(tc, tk.text()) and
           tz.peek_char_after_whitespace() == next;
}

// e.g. 'i8[]{1, 2}' or 'point[]{{1, 2}}', a type name followed by '[' has no
// other meaning
[[nodiscard]] inline auto is_array_literal(const toc& tc, const token& tk,
                                           const tokenizer& tz) -> bool {

    return is_type_name_followed_by(tc, tk, tz, '[');
}

// e.g. '[]{1, 2}' has the default type, no expression starts with '['
[[nodiscard]] inline auto is_default_array_literal(const tokenizer& tz)
    -> bool {

    return tz.peek_char_after_whitespace() == '[';
}

// e.g. 'point.at(1, 2)', a type name followed by '.' has no other meaning
[[nodiscard]] inline auto is_constructor_call(const toc& tc, const token& tk,
                                              const tokenizer& tz) -> bool {

    return is_type_name_followed_by(tc, tk, tz, '.');
}

// e.g. 'point{1, 2}', a user type name followed by '{' has no other meaning
[[nodiscard]] inline auto is_record_literal(const toc& tc, const token& tk,
                                            const tokenizer& tz) -> bool {

    if (not is_type_name_followed_by(tc, tk, tz, '{')) {
        return false;
    }

    return not tc.get_type_or_throw(tk).is_builtin();
}

// e.g. 'point' in 'var p = point' is 'point{}', a user type name not followed
// by '{', '[', '.' or '(' has no other meaning, e.g. 'point(3)' calls a
// function named like the type
[[nodiscard]] inline auto is_bare_record_type(const toc& tc, const token& tk,
                                              const tokenizer& tz) -> bool {

    if (not tc.types().has(tk.text()) or is_hidden_by_value(tc, tk.text())) {
        return false;
    }

    if (tc.get_type_or_throw(tk).is_builtin()) {
        return false;
    }

    const char next{tz.peek_char_after_whitespace()};

    return next != '{' and next != '[' and next != '.' and next != '(';
}

// an operator follows, which would continue the expression, e.g. the '+' of
// 'x + 1' or the 'and' of 'b and c'
[[nodiscard]] inline auto continues_expression(const tokenizer& tz) -> bool {
    const char next{tz.peek_char_after_whitespace()};

    if (std::string_view{"+-*/%&|^<>=!"}.contains(next)) {
        return true;
    }

    tokenizer probe{tz};
    const token next_tk{probe.next_token()};

    return next_tk.is_text("and") or next_tk.is_text("or");
}

// '{}' follows, the zero value of a typed destination
[[nodiscard]] inline auto is_zero_value(const tokenizer& tz) -> bool {
    tokenizer probe{tz};

    return not probe.is_next_char_token('{').is_empty() and
           probe.peek_char_after_whitespace() == '}';
}

// e.g. 'i8' in 'var x = i8' is 'i8(0)' and 'bool' in 'var b = bool' is 'false',
// a builtin type name not followed by '(' or '[' has no other meaning
[[nodiscard]] inline auto is_bare_builtin_type(const toc& tc, const token& tk,
                                               const tokenizer& tz) -> bool {

    if (not(tc.builtins().is_integer_name(tk.text()) or tk.is_text("bool")) or
        is_hidden_by_value(tc, tk.text())) {

        return false;
    }

    const char next{tz.peek_char_after_whitespace()};

    // e.g. 'i8 + 1' or 'bool and true' continue an expression
    return next != '(' and next != '[' and not continues_expression(tz);
}
