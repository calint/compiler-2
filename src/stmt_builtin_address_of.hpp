#pragma once

#include <ostream>
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
    token open_paren_tk_;
    token close_paren_tk_;

  public:
    stmt_builtin_address_of(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        if (not statement::get_unary_ops().is_empty()) {
            throw compiler_exception{tok(), "unary operations are not allowed "
                                            "on this built-in function"};
        }

        set_type(tc.get_type_default());

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected '(' and identifier"};
        }

        stmt_ident_ = {tc, {}, tz.next_token(), tz};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the argument"};
        }
    }

    stmt_builtin_address_of() = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        stmt_ident_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        tc.comment_source(*this, os, indent);

        if (dst_info.is_const()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "destination cannot be a constant"};
        }

        if (dst_info.type().name() != tc.get_type_default().name()) {
            throw compiler_exception{tok(), "destination must be type 'i64'"};
        }

        const ident_info src_info{tc.make_ident_info(stmt_ident_)};

        if (not src_info.is_var()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must be a variable"};
        }

        std::vector<std::string> allocated_registers;

        const operand oper{stmt_identifier::compile_effective_address(
            stmt_ident_.first_token(), tc, os, indent, stmt_ident_.elems(),
            allocated_registers, "", src_info.lea_path)};

        if (dst_info.is_register()) {
            toc::asm_lea(os, indent, dst_info.operand.address_str(),
                         oper.address_str());
        } else {
            // destination is memory location
            const std::string reg{tc.alloc_scratch_register(
                tok(), os, indent, tc.get_type_default())};
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
