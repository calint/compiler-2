#pragma once

#include <format>
#include <ostream>
#include <string>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_equal final : public expression {
    token open_paren_tk_;
    stmt_identifier lhs_;
    token lhs_delim_tk_;
    stmt_identifier rhs_;
    token close_paren_tk_;

  public:
    stmt_builtin_equal(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
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
                tz, "expected '(', 'source', 'compare', and ')'"};
        }

        lhs_ = {tc, {}, tz.next_token(), tz};

        lhs_delim_tk_ = tz.is_next_char_token(',');
        if (lhs_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'compare'"};
        }

        rhs_ = {tc, {}, tz.next_token(), tz};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tok(), "expected ')' after the arguments"};
        }
    }

    stmt_builtin_equal() = default;

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return true;
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        lhs_.source_to(os);
        lhs_delim_tk_.source_to(os);
        rhs_.source_to(os);
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
        if (lhs_info.is_const()) {
            throw compiler_exception{lhs_.tok(), "constant not supported"};
        }
        const ident_info rhs_info{tc.make_ident_info(rhs_)};
        if (rhs_info.is_const()) {
            throw compiler_exception{rhs_.tok(), "constant not supported"};
        }

        if (lhs_info.type_ref().name() != rhs_info.type_ref().name()) {
            throw compiler_exception{
                tok(), std::format("source and compare types are not the "
                                   "same. source is '{}' and compare is '{}'",
                                   lhs_info.type_ref().name(),
                                   rhs_info.type_ref().name())};
        }

        size_t size_bytes{lhs_info.type_ref().size_bytes()};

        // check comparing 2 arrays of the same size without indexing
        if (lhs_info.is_array and not lhs_.is_indexed() and
            rhs_info.is_array and not rhs_.is_indexed()) {

            if (lhs_info.array_len != rhs_info.array_len) {
                throw compiler_exception{lhs_.tok(),
                                         "cannot compare arrays of different "
                                         "sizes"};
            }

            size_bytes = multiply_storage_size(size_bytes, lhs_info.array_len);
        }

        x.begin_memory_equal(tok(), indent);

        std::vector<operand> allocated_scratch_registers;

        x.comment(lhs_.tok(), indent, statement::trimmed_source(lhs_));

        const operand lhs_operand{lhs_.compile_lea(
            tc, indent, lhs_.first_token(), allocated_scratch_registers, {},
            lhs_info.lea_path, {})};

        x.set_memory_equal_left(indent, lhs_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.comment(rhs_.tok(), indent, statement::trimmed_source(rhs_));

        allocated_scratch_registers.clear();

        const operand rhs_operand{rhs_.compile_lea(
            tc, indent, rhs_.first_token(), allocated_scratch_registers, {},
            rhs_info.lea_path, {})};

        x.set_memory_equal_right(indent, rhs_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.end_memory_equal(tok(), indent, size_bytes,
                           lhs_info.type_ref().alignment(), dst, inverted);
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        lhs_.visit_reads(var, reader);
        rhs_.visit_reads(var, reader);
    }
};
