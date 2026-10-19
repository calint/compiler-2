#pragma once

// what the next tokens are, decided before they are parsed: a type name or a
// generic function name followed by a character has one meaning only, e.g.
// 'point{' is a record literal and 'show<' a generic call; a variable of that
// name hides the type

#include <string_view>

#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// e.g. 'show<name>(x)', a generic function name followed by '<' has no other
// meaning
[[nodiscard]] inline auto is_generic_call(const toc& tc,
                                          const std::string_view name,
                                          tokenizer& tz) -> bool {

    return tc.generics().has_func(name) and
           tz.peek_char_after_whitespace() == '<';
}

// a type name hidden by a var or alias is an identifier instead
[[nodiscard]] inline auto is_type_name_followed_by(const toc& tc,
                                                   const token& tk,
                                                   tokenizer& tz,
                                                   const char next) -> bool {

    return tc.types().has(tk.text()) and
           not tc.scopes().is_var_or_alias(tk.text()) and
           tz.peek_char_after_whitespace() == next;
}

// e.g. 'i8[]{1, 2}' or 'point[]{{1, 2}}', a type name followed by '[' has no
// other meaning
[[nodiscard]] inline auto is_array_literal(const toc& tc, const token& tk,
                                           tokenizer& tz) -> bool {

    return is_type_name_followed_by(tc, tk, tz, '[');
}

// e.g. '[]{1, 2}' has the default type, no expression starts with '['
[[nodiscard]] inline auto is_default_array_literal(tokenizer& tz) -> bool {
    return tz.peek_char_after_whitespace() == '[';
}

// e.g. 'point.at(1, 2)', a type name followed by '.' has no other meaning
[[nodiscard]] inline auto is_constructor_call(const toc& tc, const token& tk,
                                              tokenizer& tz) -> bool {

    return is_type_name_followed_by(tc, tk, tz, '.');
}

// e.g. 'point{1, 2}', a user type name followed by '{' has no other meaning
[[nodiscard]] inline auto is_record_literal(const toc& tc, const token& tk,
                                            tokenizer& tz) -> bool {

    if (not is_type_name_followed_by(tc, tk, tz, '{')) {
        return false;
    }

    return not tc.get_type_or_throw(tk, tk.text()).is_builtin();
}

// e.g. 'point' in 'var p = point' is 'point{}', a user type name not followed
// by '{', '[' or '.' has no other meaning
[[nodiscard]] inline auto is_bare_record_type(const toc& tc, const token& tk,
                                              tokenizer& tz) -> bool {

    if (not tc.types().has(tk.text()) or
        tc.scopes().is_var_or_alias(tk.text())) {

        return false;
    }

    if (tc.get_type_or_throw(tk, tk.text()).is_builtin()) {
        return false;
    }

    const char next{tz.peek_char_after_whitespace()};

    return next != '{' and next != '[' and next != '.';
}

// e.g. 'i8' in 'var x = i8' is 'i8(0)' and 'bool' in 'var b = bool' is 'false',
// a builtin type name not followed by '(' or '[' has no other meaning
[[nodiscard]] inline auto is_bare_builtin_type(const toc& tc, const token& tk,
                                               tokenizer& tz) -> bool {

    if (not(tc.builtins().is_integer_name(tk.text()) or tk.is_text("bool")) or
        tc.scopes().is_var_or_alias(tk.text()) or
        tc.scopes().has_const(tk.text())) {

        return false;
    }

    const char next{tz.peek_char_after_whitespace()};

    // an arithmetic operator would continue the expression, e.g. 'i8 + 1'
    return next != '(' and next != '[' and
           not std::string_view{"+-*/%&|^<>"}.contains(next);
}
