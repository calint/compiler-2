#pragma once

#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_array_equal final : public expression {
    stmt_identifier from_;
    stmt_identifier to_;
    expr_any count_;
    token ws1_; // whitespace after ')'
  public:
    stmt_array_equal(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)} {

        set_type(tc.get_type_bool());

        if (not statement::get_unary_ops().is_empty()) {
            throw compiler_exception{tok(), "unary operations are not allowed "
                                            "on this built-in function"};
        }

        if (not tz.is_next_char('(')) {
            throw compiler_exception{
                tz, "expected '(' and 'source', 'compare' and 'count'"};
        }

        from_ = {tc, {}, tz.next_token(), tz};

        if (not tz.is_next_char(',')) {
            throw compiler_exception{tz,
                                     "expected ',' then 'compare' and 'count'"};
        }

        to_ = {tc, {}, tz.next_token(), tz};

        if (not tz.is_next_char(',')) {
            throw compiler_exception{tz, "expected ',' then 'count'"};
        }

        // expr_any(toc& tc, tokenizer& tz, const type& tp, bool in_args)
        count_ = {tc, tz, tc.get_type_default(), true};

        if (not tz.is_next_char(')')) {
            throw compiler_exception{tok(), "expected ')' after the argument"};
        }

        ws1_ = tz.next_whitespace_token();
    }

    ~stmt_array_equal() override = default;

    stmt_array_equal() = default;
    stmt_array_equal(const stmt_array_equal&) = default;
    stmt_array_equal(stmt_array_equal&&) = default;
    auto operator=(const stmt_array_equal&) -> stmt_array_equal& = default;
    auto operator=(stmt_array_equal&&) -> stmt_array_equal& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::print(os, "(");
        from_.source_to(os);
        std::print(os, ",");
        to_.source_to(os);
        std::print(os, ",");
        count_.source_to(os);
        std::print(os, ")");
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);

        // allocate the register for rep movs
        tc.alloc_named_register_or_throw(*this, os, indent, "rsi");
        tc.alloc_named_register_or_throw(*this, os, indent, "rdi");
        tc.alloc_named_register_or_throw(*this, os, indent, "rcx");

        std::vector<std::string> allocated_scratch_registers;

        // from operand to rsi
        const std::string from_operand =
            stmt_identifier::compile_effective_address(
                from_.first_token(), tc, os, indent, from_.elems(),
                allocated_scratch_registers);

        tc.asm_cmd(tok(), os, indent, "lea", "rsi",
                   std::format("[{}]", from_operand));

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            tc.free_scratch_register(os, indent, reg);
        }

        // to operand to 'rdi'
        allocated_scratch_registers.clear();
        const std::string to_operand =
            stmt_identifier::compile_effective_address(
                to_.first_token(), tc, os, indent, to_.elems(),
                allocated_scratch_registers);

        tc.asm_cmd(tok(), os, indent, "lea", "rdi",
                   std::format("[{}]", to_operand));

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            tc.free_scratch_register(os, indent, reg);
        }

        // size to 'rcx'
        count_.compile(tc, os, indent, "rcx");

        const ident_info from_info{tc.make_ident_info(from_, false)};
        const ident_info to_info{tc.make_ident_info(to_, false)};

        if (from_info.type_ref.name() != to_info.type_ref.name()) {
            throw compiler_exception{
                tok(), std::format("source and destination types are not the "
                                   "same. source is '{}' vs destination '{}'",
                                   from_info.type_ref.name(),
                                   to_info.type_ref.name())};
        }

        const size_t type_size{from_info.type_ref.size()};

        if (type_size > 1) {
            // check whether it is possible to shift left instead of
            // multiplication
            if (std::optional<int> shl{
                    stmt_identifier::get_shift_amount(type_size)};
                shl) {
                tc.asm_cmd(tok(), os, indent, "shl", "rcx",
                           std::format("{}", *shl));
            } else {
                tc.asm_cmd(tok(), os, indent, "imul", "rcx",
                           std::format("{}", type_size));
            }
        }

        // copy
        toc::asm_repe_cmps(tok(), os, indent, 'b');

        tc.free_named_register(os, indent, "rcx");
        tc.free_named_register(os, indent, "rdi");
        tc.free_named_register(os, indent, "rsi");

        // the labels
        const std::string lbl_if_equal{
            toc::create_unique_label(tc, tok(), "cmps_eq")};
        const std::string lbl_end{
            toc::create_unique_label(tc, tok(), "cmps_end")};

        toc::asm_jxx(tok(), os, indent, "e", lbl_if_equal); // je
        tc.asm_cmd(tok(), os, indent, "mov", dst, "false");
        toc::asm_jmp(tok(), os, indent, lbl_end);
        toc::asm_label(tok(), os, indent, lbl_if_equal);
        tc.asm_cmd(tok(), os, indent, "mov", dst, "true");
        toc::asm_label(tok(), os, indent, lbl_end);
    }
};
