#pragma once

#include <format>
#include <ostream>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_array_size_of final : public expression {
    token open_paren_tk_;
    stmt_identifier stmt_ident_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_size_of(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        set_type(tc.get_type_default());

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tz,
                                     "expected '(' followed by an identifier"};
        }

        stmt_ident_ = {tc, {}, tz.next_token(), tz};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the argument"};
        }
    }

    stmt_builtin_array_size_of() = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        stmt_ident_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, x86& x, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        x.comment_source(tc, *this, indent);

        if (dst_info.is_const()) {
            throw compiler_exception{tok(), "destination cannot be a constant"};
        }

        if (dst_info.type().name() != tc.get_type_default().name()) {
            throw compiler_exception{tok(), "destination must be an 'i64'"};
        }

        const ident_info src_info{tc.make_ident_info(x, stmt_ident_)};

        if (not src_info.is_var()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must be a variable"};
        }

        if (not src_info.is_array) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must refer to an array"};
        }

        // variable, register or field
        const std::string dst_op{dst_info.operand.str()};
        x.mov(tc, tok(), indent, dst_op,
                 std::format("{}", src_info.array_size));

        get_unary_ops().compile(tc, x, indent, dst_op);
    }
};
