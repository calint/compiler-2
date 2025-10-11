#pragma once

#include <ranges>

#include "compiler_exception.hpp"
#include "expression.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_address_of final : public expression {
    stmt_identifier stmt_ident_;
    token ws1_; // whitespace after ')'
  public:
    stmt_address_of(toc& tc, unary_ops uops, token tk, tokenizer& tz)
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

    ~stmt_address_of() override = default;

    stmt_address_of() = default;
    stmt_address_of(const stmt_address_of&) = default;
    stmt_address_of(stmt_address_of&&) = default;
    auto operator=(const stmt_address_of&) -> stmt_address_of& = default;
    auto operator=(stmt_address_of&&) -> stmt_address_of& = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        std::print(os, "(");
        stmt_ident_.source_to(os);
        std::print(os, ")");
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        if (dst_info.is_const()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "destination cannot be a constant"};
        }

        if (dst_info.type_ref.name() != tc.get_type_default().name()) {
            throw compiler_exception{tok(), "destination must be type i64"};
        }

        const ident_info src_info{tc.make_ident_info(
            stmt_ident_.first_token(), stmt_ident_.identifier(), false)};

        if (not src_info.is_var()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must be a variable"};
        }

        std::vector<std::string> allocated_registers;
        const std::string operand = stmt_identifier::compile_effective_address(
            stmt_ident_.first_token(), tc, os, indent, stmt_ident_.elems(),
            allocated_registers);

        if (dst_info.is_register()) {
            tc.asm_cmd(tok(), os, indent, "lea", dst_info.id_nasm,
                       std::format("[{}]", operand));
        } else {
            // destination is memory location
            const std::string reg{tc.alloc_scratch_register(tok(), os, indent)};
            tc.asm_cmd(tok(), os, indent, "lea", reg,
                       std::format("[{}]", operand));
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm, reg);
            tc.free_scratch_register(os, indent, reg);
        }

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(os, indent, reg);
        }
    }
};
