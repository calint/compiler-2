#pragma once

#include <ostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_address_of final : public expression {
    stmt_identifier stmt_ident_;
    token ws1_; // whitespace after ')'
  public:
    stmt_builtin_address_of(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)} {

        set_type(tc.get_type_default());

        if (not statement::get_unary_ops().is_empty()) {
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

    ~stmt_builtin_address_of() override = default;

    stmt_builtin_address_of() = default;
    stmt_builtin_address_of(const stmt_builtin_address_of&) = default;
    stmt_builtin_address_of(stmt_builtin_address_of&&) = default;
    auto operator=(const stmt_builtin_address_of&)
        -> stmt_builtin_address_of& = default;
    auto operator=(stmt_builtin_address_of&&)
        -> stmt_builtin_address_of& = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        std::print(os, "(");
        stmt_ident_.source_to(os);
        std::print(os, ")");
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        tc.comment_source(*this, os, indent);

        if (dst_info.is_const()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "destination cannot be a constant"};
        }

        if (dst_info.type_ptr->name() != tc.get_type_default().name()) {
            throw compiler_exception{tok(), "destination must be type i64"};
        }

        const ident_info src_info{tc.make_ident_info(stmt_ident_.first_token(),
                                                     stmt_ident_.identifier())};

        if (not src_info.is_var()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must be a variable"};
        }

        std::vector<std::string> allocated_registers;

        const operand oper = stmt_identifier::compile_effective_address(
            stmt_ident_.first_token(), tc, os, indent, stmt_ident_.elems(),
            allocated_registers, "", src_info.lea_path);

        if (dst_info.is_register()) {
            toc::asm_lea(os, indent, dst_info.operand.address_str(),
                         oper.address_str());
        } else {
            // destination is memory location
            const std::string reg{tc.alloc_scratch_register(tok(), os, indent)};
            toc::asm_lea(os, indent, reg, oper.address_str());
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.operand.str(), reg);
            tc.free_scratch_register(tok(), os, indent, reg);
        }

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }
    }
};
