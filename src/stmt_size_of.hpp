#pragma once

#include "compiler_exception.hpp"
#include "expression.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_size_of : public expression {
    stmt_identifier stmt_ident_;
    token ws1_; // whitespace after ')'

  public:
    stmt_size_of(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)} {

        set_type(tc.get_type_default());

        if (not get_unary_ops().is_empty()) {
            throw compiler_exception{tok(), "unary operations are not allowed "
                                            "on this built-in function"};
        }

        if (not tz.is_next_char('(')) {
            throw compiler_exception{tok(), "expected '(' and identifier"};
        }

        stmt_ident_ = {tc, {}, tz.next_token(), tz};

        if (not tz.is_next_char(')')) {
            throw compiler_exception{tok(), "expected ')' after the argument"};
        }

        ws1_ = tz.next_whitespace_token();
    }

    ~stmt_size_of() override = default;

    stmt_size_of() = default;
    stmt_size_of(const stmt_size_of&) = default;
    stmt_size_of(stmt_size_of&&) = default;
    auto operator=(const stmt_size_of&) -> stmt_size_of& = default;
    auto operator=(stmt_size_of&&) -> stmt_size_of& = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        os << "(";
        stmt_ident_.source_to(os);
        os << ")";
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        if (dst_info.type_ref.name() != tc.get_type_default().name()) {
            throw compiler_exception{tok(), "destination must be type i64"};
        }

        const ident_info src_info{tc.make_ident_info(
            stmt_ident_.tok(), stmt_ident_.identifier(), false)};

        // variable, register or field
        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                   std::format("{}", src_info.size));
    }
};
