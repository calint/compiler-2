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
    stmt_identifier src_;
    token src_delim_tk_;
    stmt_identifier dst_;
    token dst_delim_tk_;
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

        src_ = {tc, {}, tz.next_token(), tz};

        src_delim_tk_ = tz.is_next_char_token(',');
        if (src_delim_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected ',' followed by 'to' and 'count'"};
        }

        dst_ = {tc, {}, tz.next_token(), tz};

        dst_delim_tk_ = tz.is_next_char_token(',');
        if (dst_delim_tk_.is_empty()) {
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
        src_.source_to(os);
        src_delim_tk_.source_to(os);
        dst_.source_to(os);
        dst_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info array_src_info{tc.make_ident_info(src_)};
        const ident_info array_dst_info{tc.make_ident_info(dst_)};

        if (array_src_info.type_ref().name() !=
            array_dst_info.type_ref().name()) {
            throw compiler_exception{
                tok(),
                std::format("source type '{}' does not match destination "
                            "type '{}'",
                            array_src_info.type_ref().name(),
                            array_dst_info.type_ref().name())};
        }

        const operand count_register{x.begin_array_copy(tok(), indent)};

        std::vector<operand> allocated_scratch_registers;

        x.comment(count_.tok(), indent, statement::trimmed_source(count_));

        count_.compile(tc, indent,
                       toc::make_ident_info_from_register(count_register));

        x.comment(src_.tok(), indent, statement::trimmed_source(src_));

        const operand src_operand{src_.compile_lea(
            tc, indent, src_.first_token(), allocated_scratch_registers,
            count_register, array_src_info.lea_path,
            x.array_copy_source_register())};

        x.set_array_copy_source(indent, src_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.comment(dst_.tok(), indent, statement::trimmed_source(dst_));

        allocated_scratch_registers.clear();

        const operand dst_operand{dst_.compile_lea(
            tc, indent, dst_.first_token(), allocated_scratch_registers,
            count_register, array_dst_info.lea_path,
            x.array_copy_destination_register())};

        x.set_array_copy_destination(indent, dst_operand);

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);

        x.end_array_copy(tok(), indent, array_src_info.type_ref().size_bytes());
    }

    // the copied elements are not tracked, so the destination stays unassigned
    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        src_.visit_reads(var, reader);
        dst_.visit_index_reads(var, reader);
        count_.visit_reads(var, reader);
    }
};
