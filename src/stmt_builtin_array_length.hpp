#pragma once

#include <format>
#include <ostream>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_array_length final : public expression {
    token open_paren_tk_;
    stmt_identifier stmt_ident_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_length(toc& tc, unary_ops uops, const token tk,
                              tokenizer& tz)
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

    stmt_builtin_array_length() = default;

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
            throw compiler_exception{tok(), "destination cannot be a constant"};
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

        if (not src_info.is_array) {
            throw compiler_exception{stmt_ident_.first_token(),
                                     "argument must refer to an array"};
        }

        // variable, register or field
        x.copy_value(
            tok(), indent, dst_info.operand,
            operand::imm(std::format("{}", src_info.array_len), get_type()));

        get_unary_ops().compile(tc, indent, dst_info.operand);
    }

    // the length is known at compile time so the value is not read
    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        stmt_ident_.visit_index_reads(var, reader);
    }
};
