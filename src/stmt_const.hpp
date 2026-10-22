#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

// the constant size of an array, e.g. the '4' of 'i8[4]' or the 'n' of a
// field 'i8[n]' where 'n' is a constant; folded when parsed and never
// compiled, empty for 'i8[]'; not a constant definition ('stmt_def_const')
class stmt_const final : public statement {
    unary_ops uops_;
    token literal_tk_;
    int64_t value_{};

  public:
    // an empty literal has no value, e.g. the size in 'i8[]'
    stmt_const(const toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()}, uops_{tz},
          literal_tk_{tz.next_token()} {

        set_type(tc.builtins().default_type());

        if (literal_tk_.has_whitespace_before() or literal_tk_.is_string()) {
            throw compile_error{
                tok(), "expected a constant value or a constant identifier"};
        }

        if (not has_value()) {
            // e.g. 'i8[-]', the operators need a value
            if (not uops_.is_empty()) {
                throw compile_error{
                    tz, "expected a constant value or a constant identifier"};
            }

            return;
        }

        if (const std::optional<int64_t> named{
                tc.scopes().const_value(literal_tk_.text()),
            };
            named) {

            value_ = uops_.evaluate_constant(*named);
            return;
        }

        // the unary ops cannot prefix the quoted text as they do a number
        if (constant_parser::is_character_literal(literal_tk_.text())) {
            value_ = uops_.evaluate_constant(constant_parser::parse_character(
                literal_tk_, literal_tk_.text()));

            return;
        }

        // a name starts with a letter or '_', e.g. the 'm' of 'i8[m]' where
        // 'm' is a variable
        const std::string_view text{literal_tk_.text()};

        if (text.front() < '0' or text.front() > '9') {
            if (tc.scopes().lookup(text).is_value()) {
                throw compile_error{
                    literal_tk_, std::format("'{}' is not a constant", text)};
            }

            throw compile_error{
                literal_tk_,
                std::format("cannot resolve identifier '{}'", text)};
        }

        // a lone '-' prefixes a decimal number as text so that the most
        // negative value parses, other operators are applied to the value
        const bool is_text_prefixed{
            not constant_parser::has_base_prefix(literal_tk_.text()) and
                (uops_.is_empty() or uops_.is_only_negated()),
        };

        const std::string num_str{
            std::format("{}{}", is_text_prefixed ? uops_.to_string() : "",
                        literal_tk_.text()),
        };

        const std::optional<int64_t> num{
            constant_parser::parse_constant(literal_tk_, num_str),
        };

        if (not num.has_value()) {
            throw compile_error{literal_tk_,
                                std::format("cannot parse constant '{}{}'",
                                            uops_.to_string(),
                                            literal_tk_.text())};
        }

        value_ = is_text_prefixed ? *num : uops_.evaluate_constant(*num);
    }

    stmt_const() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        uops_.source_to(os);
        literal_tk_.source_to(os);
    }

    //
    // class methods
    //

    // a written size of at least 1, e.g. the '4' of 'i8[4]'
    [[nodiscard]] auto array_size() const -> size_t {
        if (not has_value() or value_ < 1) {
            throw compile_error{
                tok(), "expected a constant array size greater than 0"};
        }

        return static_cast<size_t>(value_);
    }

    [[nodiscard]] auto has_value() const -> bool {
        return not literal_tk_.is_none();
    }
};
