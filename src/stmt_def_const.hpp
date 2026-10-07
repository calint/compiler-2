#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_const.hpp"
#include "stmt_def_var.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class stmt_def_const final : public statement {
    token name_tk_;
    token equals_tk_;
    stmt_const const_;

  public:
    stmt_def_const(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, name_tk_{tz.next_token()} {

        if (name_tk_.text().empty()) {
            throw compiler_exception{name_tk_, "expected name of constant"};
        }

        toc::assert_valid_name(name_tk_);

        equals_tk_ = tz.is_next_char_token('=');

        if (equals_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '=' followed by a constant value"};
        }

        const_ = {tc, tz, 0};

        if (not const_.has_value()) {
            throw compiler_exception{const_.tok(), "expected constant value"};
        }

        set_type(tc.get_type_void());

        tc.add_const(name_tk_, 0, name_tk_.text(), const_.value());
    }

    stmt_def_const() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);
        const_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        tc.add_const(name_tk_, indent, name_tk_.text(), const_.value());
    }

    //
    // statics
    //

    // 'tk' is the 'let' keyword; 'let x = 5' defines a compile-time constant,
    // anything else a variable that is read-only once initialized
    [[nodiscard]] static auto parse_let(toc& tc, tokenizer& tz, const token tk)
        -> std::unique_ptr<statement> {

        if (is_constant_definition(tc, tz)) {
            return std::make_unique<stmt_def_const>(tc, tk, tz);
        }

        return std::make_unique<stmt_def_var>(tc, tk, tz);
    }

  private:
    //
    // statics
    //

    [[nodiscard]] static auto has_constant_initializer(const toc& tc,
                                                       tokenizer& tz) -> bool {

        const token name_tk{tz.next_token()};

        if (name_tk.text().empty() or tz.is_next_char_token('=').is_empty()) {
            return true;
        }

        // an array of the default type, e.g. 'let a = []{1, 2}'
        if (tz.peek_char_after_whitespace() == '[') {
            return false;
        }

        const unary_ops uops{tz};
        const token literal_tk{tz.next_token()};
        const std::string_view text{literal_tk.text()};

        if (text.empty()) {
            return true;
        }

        const bool is_constant_operand{
            tc.has_const(text) or constant_parser::is_character_literal(text) or
                (text.front() >= '0' and text.front() <= '9'),
        };

        if (not is_constant_operand) {
            return false;
        }

        // an operator or postfix continues the expression, e.g. 'let x = 1 +
        // y'; no statement starts with one of these characters
        constexpr std::string_view continues_expression{"+-*/%&|^<>=!([."};

        return not continues_expression.contains(
            tz.peek_char_after_whitespace());
    }

    // after 'let', e.g. 'x = 5' and 'y = -x' define a compile-time constant
    // while 'z = a + 1' and 'p = point{1, 2}' define a read-only variable;
    // malformed definitions count as constants so they report the constant
    // errors; the tokenizer position is restored
    [[nodiscard]] static auto is_constant_definition(const toc& tc,
                                                     tokenizer& tz) -> bool {

        const token start_tk{tz.cur_position_token()};
        const bool is_constant{has_constant_initializer(tc, tz)};
        tz.rewind_to_position(start_tk);
        return is_constant;
    }
};
