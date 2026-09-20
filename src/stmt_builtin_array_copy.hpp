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

class stmt_builtin_array_copy final : public statement {
    token open_paren_tk_;
    stmt_identifier from_;
    token from_delim_tk_;
    stmt_identifier to_;
    token to_delim_tk_;
    expr_any count_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_copy(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, open_paren_tk_{tz.is_next_char_token('(')} {

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '(', 'from', 'to', 'count', and ')'"};
        }

        set_type(tc.get_type_void());

        from_ = {tc, {}, tz.next_token(), tz};

        from_delim_tk_ = tz.is_next_char_token(',');
        if (from_delim_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected ',' followed by 'to' and 'count'"};
        }

        to_ = {tc, {}, tz.next_token(), tz};

        to_delim_tk_ = tz.is_next_char_token(',');
        if (to_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'count'"};
        }

        count_ = {tc, tz, tc.get_type_default(), true, false, 0};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the arguments"};
        }
    }

    stmt_builtin_array_copy() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        from_.source_to(os);
        from_delim_tk_.source_to(os);
        to_.source_to(os);
        to_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info from_info{tc.make_ident_info(from_)};
        const ident_info to_info{tc.make_ident_info(to_)};

        if (from_info.type_ref().name() != to_info.type_ref().name()) {
            throw compiler_exception{
                tok(),
                std::format("source type '{}' does not match destination "
                            "type '{}'",
                            from_info.type_ref().name(),
                            to_info.type_ref().name())};
        }

        const operand count_register{x.begin_array_copy(tok(), indent)};

        std::vector<operand> allocated_scratch_registers;

        x.comment(count_.tok(), indent, statement::trimmed_source(count_));

        count_.compile(tc, indent,
                       toc::make_ident_info_from_register(count_register));

        x.comment(from_.tok(), indent, statement::trimmed_source(from_));

        const operand from_operand{from_.compile_lea(
            tc, indent, from_.first_token(), allocated_scratch_registers,
            count_register, from_info.lea_path)};

        x.set_array_copy_source(indent, from_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.comment(to_.tok(), indent, statement::trimmed_source(to_));

        allocated_scratch_registers.clear();

        const operand to_operand{to_.compile_lea(
            tc, indent, to_.first_token(), allocated_scratch_registers,
            count_register, to_info.lea_path)};

        x.set_array_copy_destination(indent, to_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.end_array_copy(tok(), indent, from_info.type_ref().size());
    }
};
