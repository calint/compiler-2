#pragma once

#include <cassert>
#include <format>
#include <vector>

#include "compiler_exception.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// a parameter of a generic definition, e.g. 'T type' is a type and 'capacity'
// is a constant
struct generic_param {
    token name_tk;
    bool is_type{};

    //
    // statics
    //

    // e.g. '<T type, capacity>', the '<' is next
    [[nodiscard]] static auto parse(tokenizer& tz)
        -> std::vector<generic_param> {
        const token open_tk{tz.is_next_char_token('<')};

        assert(not open_tk.is_empty());

        std::vector<generic_param> params;

        while (true) {
            const token name_tk{tz.next_token()};
            if (name_tk.text().empty()) {
                throw compiler_exception{
                    tz, "expected a name for the generic parameter"};
            }

            const char next{tz.peek_char_after_whitespace()};
            const bool is_type{next != ',' and next != '>'};

            if (is_type and not tz.next_token().is_text("type")) {
                throw compiler_exception{
                    name_tk,
                    std::format("expected 'type' after generic parameter '{}'",
                                name_tk.text())};
            }

            params.push_back({.name_tk{name_tk}, .is_type{is_type}});

            if (not tz.is_next_char_token('>').is_empty()) {
                return params;
            }

            if (tz.is_next_char_token(',').is_empty()) {
                throw compiler_exception{
                    tz, std::format("expected ',' or '>' after generic "
                                    "parameter '{}'",
                                    name_tk.text())};
            }
        }
    }
};

// e.g. '<127, name>' of 'text<127, name>'
struct generic_arguments {
    std::vector<token> arg_tks;
    // the '<', the arguments, the delimiters and the '>' in source order
    std::vector<token> list_tks;

    //
    // statics
    //

    // 'open_tk' is the '<' that has been read
    [[nodiscard]] static auto parse(tokenizer& tz, const token& open_tk)
        -> generic_arguments {

        generic_arguments args;
        args.list_tks.emplace_back(open_tk);

        while (true) {
            const token arg_tk{tz.next_token()};
            if (arg_tk.text().empty()) {
                throw compiler_exception{tz, "expected a generic argument"};
            }

            args.arg_tks.emplace_back(arg_tk);
            args.list_tks.emplace_back(arg_tk);

            const token close_tk{tz.is_next_char_token('>')};
            if (not close_tk.is_empty()) {
                args.list_tks.emplace_back(close_tk);

                return args;
            }

            const token delim_tk{tz.is_next_char_token(',')};
            if (delim_tk.is_empty()) {
                throw compiler_exception{
                    tz, "expected ',' or '>' after generic argument"};
            }

            args.list_tks.emplace_back(delim_tk);
        }
    }
};
