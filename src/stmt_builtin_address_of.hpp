#pragma once

#include <format>
#include <ostream>
#include <ranges>
#include <string>
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
            throw compiler_exception{
                tok(),
                "this built-in function does not accept unary operations"};
        }

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

    stmt_builtin_address_of() = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        stmt_ident_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        if (dst_info.is_const()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "destination cannot be a constant"};
        }

        if (dst_info.type_ref().name() != tc.get_type_default().name()) {
            throw compiler_exception{tok(),
                                     std::format("destination must be an '{}'",
                                                 tc.get_type_default().name())};
        }

        const ident_info src_info{tc.make_ident_info(stmt_ident_)};

        if (not src_info.is_var()) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must be a variable"};
        }

        std::vector<operand> allocated_registers;

        const operand oper{stmt_identifier::compile_effective_address(
            tc, indent, stmt_ident_.first_token(), stmt_ident_.elems(),
            allocated_registers, {}, src_info.lea_path)};

        x.address_of(tok(), indent, dst_info.operand, oper);

        for (const operand& reg : allocated_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }
    }
};
