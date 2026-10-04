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
