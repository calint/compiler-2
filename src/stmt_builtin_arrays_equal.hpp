#pragma once

#include <format>
#include <ostream>
#include <string>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_arrays_equal final : public expression {
    token open_paren_tk_;
    stmt_identifier lhs_;
    token lhs_delim_tk_;
    stmt_identifier rhs_;
    token rhs_delim_tk_;
    expr_any count_;
    token close_paren_tk_;

  public:
    stmt_builtin_arrays_equal(toc& tc, unary_ops uops, const token tk,
                              tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        if (not statement::get_unary_ops().is_empty()) {
            throw compiler_exception{
                tok(),
                "this built-in function does not accept unary operations"};
        }

        set_type(tc.get_type_bool());

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '(', 'source', 'compare', 'count', and ')'"};
        }

        lhs_ = {tc, {}, tz.next_token(), tz};

        lhs_delim_tk_ = tz.is_next_char_token(',');
        if (lhs_delim_tk_.is_empty()) {
            throw compiler_exception{tz,
                                     "expected ',' then 'compare' and 'count'"};
        }

        rhs_ = {tc, {}, tz.next_token(), tz};

        rhs_delim_tk_ = tz.is_next_char_token(',');
        if (rhs_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'count'"};
        }

        count_ = {tc, tz, tc.get_type_default(), true, false, 0};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tok(), "expected ')' after the arguments"};
        }
    }

    stmt_builtin_arrays_equal() = default;

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return true;
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        lhs_.source_to(os);
        lhs_delim_tk_.source_to(os);
        rhs_.source_to(os);
        rhs_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        compile_boolean(tc, indent, dst_info.operand, false);
    }

    auto compile_boolean(toc& tc, const size_t indent, const operand& dst,
                         const bool inverted) const -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info lhs_info{tc.make_ident_info(lhs_)};
        const ident_info rhs_info{tc.make_ident_info(rhs_)};

        if (lhs_info.type_ref().name() != rhs_info.type_ref().name()) {
            throw compiler_exception{
                tok(),
                std::format("source type '{}' does not match compare type '{}'",
                            lhs_info.type_ref().name(),
                            rhs_info.type_ref().name())};
        }

        if (dst.type_ref().name() != get_type().name()) {
            throw compiler_exception{
                tok(), std::format("destination type must be '{}', not '{}'",
                                   get_type().name(), dst.type_ref().name())};
        }

        const operand count_register{x.begin_memory_equal(tok(), indent)};

        std::vector<operand> allocated_scratch_registers;

        x.comment(count_.tok(), indent, statement::trimmed_source(count_));

        count_.compile(tc, indent,
                       toc::make_ident_info_from_register(count_register));

        x.comment(lhs_.tok(), indent, statement::trimmed_source(lhs_));

        const operand lhs_operand{lhs_.compile_lea(
            tc, indent, lhs_.first_token(), allocated_scratch_registers,
            count_register, lhs_info.lea_path, x.memory_equal_left_register())};

        x.set_memory_equal_left(indent, lhs_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.comment(rhs_.tok(), indent, statement::trimmed_source(rhs_));

        allocated_scratch_registers.clear();

        const operand rhs_operand{rhs_.compile_lea(
            tc, indent, rhs_.first_token(), allocated_scratch_registers,
            count_register, rhs_info.lea_path,
            x.memory_equal_right_register())};

        x.set_memory_equal_right(indent, rhs_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.end_arrays_equal(tok(), indent, lhs_info.type_ref().size_bytes(), dst,
                           inverted);
    }
};
