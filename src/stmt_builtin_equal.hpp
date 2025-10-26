#pragma once

#include <format>
#include <ostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_equal final : public expression {
    stmt_identifier lhs_;
    stmt_identifier rhs_;
    token ws1_; // whitespace after ')'
  public:
    stmt_builtin_equal(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)} {

        set_type(tc.get_type_bool());

        if (not statement::get_unary_ops().is_empty()) {
            throw compiler_exception{tok(), "unary operations are not allowed "
                                            "on this built-in function"};
        }

        if (not tz.is_next_char('(')) {
            throw compiler_exception{
                tz, "expected '(' then 'source' and 'compare'"};
        }

        lhs_ = {tc, {}, tz.next_token(), tz};

        if (not tz.is_next_char(',')) {
            throw compiler_exception{tz, "expected ',' then 'compare'"};
        }

        rhs_ = {tc, {}, tz.next_token(), tz};

        if (not tz.is_next_char(')')) {
            throw compiler_exception{tok(), "expected ')' after the argument"};
        }

        ws1_ = tz.next_whitespace_token();
    }

    ~stmt_builtin_equal() override = default;

    stmt_builtin_equal() = default;
    stmt_builtin_equal(const stmt_builtin_equal&) = default;
    stmt_builtin_equal(stmt_builtin_equal&&) = default;
    auto operator=(const stmt_builtin_equal&) -> stmt_builtin_equal& = default;
    auto operator=(stmt_builtin_equal&&) -> stmt_builtin_equal& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::print(os, "(");
        lhs_.source_to(os);
        std::print(os, ",");
        rhs_.source_to(os);
        std::print(os, ")");
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        // allocate the register for rep movs
        tc.alloc_named_register_or_throw(tok(), os, indent, "rsi");
        tc.alloc_named_register_or_throw(tok(), os, indent, "rdi");
        tc.alloc_named_register_or_throw(tok(), os, indent, "rcx");

        std::vector<std::string> allocated_scratch_registers;

        const ident_info lhs_info{tc.make_ident_info(lhs_)};
        const ident_info rhs_info{tc.make_ident_info(rhs_)};

        // from operand to rsi
        tc.comment_source(lhs_, os, indent);
        const std::string lhs_operand{
            stmt_identifier::compile_effective_address(
                lhs_.first_token(), tc, os, indent, lhs_.elems(),
                allocated_scratch_registers, "", lhs_info.lea_path)};

        toc::asm_lea(os, indent, "rsi", lhs_operand);

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }

        // to operand to 'rdi'
        allocated_scratch_registers.clear();
        tc.comment_source(rhs_, os, indent);
        const std::string rhs_operand{
            stmt_identifier::compile_effective_address(
                rhs_.first_token(), tc, os, indent, rhs_.elems(),
                allocated_scratch_registers, "", rhs_info.lea_path)};

        toc::asm_lea(os, indent, "rdi", rhs_operand);

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }

        if (lhs_info.type_ptr->name() != rhs_info.type_ptr->name()) {
            throw compiler_exception{
                tok(), std::format("source and destination types are not the "
                                   "same. source is '{}' vs destination '{}'",
                                   lhs_info.type_ptr->name(),
                                   rhs_info.type_ptr->name())};
        }

        const size_t type_size{lhs_info.type_ptr->size()};

        char rep_size{'b'};
        size_t rcx{type_size};

        // check comparing 2 arrays of same size without indexing
        if (lhs_info.is_array and not lhs_.is_indexed() and
            rhs_info.is_array and not rhs_.is_indexed()) {

            if (lhs_info.array_size != rhs_info.array_size) {
                throw compiler_exception(lhs_.tok(),
                                         "cannot compare arrays of different "
                                         "sizes. see `arrays_equal`");
            }

            rcx *= lhs_info.array_size;
        }

        if ((rcx % toc::size_qword) == 0) {
            rep_size = 'q';
            rcx /= toc::size_qword;
        } else if ((rcx % toc::size_dword) == 0) {
            rep_size = 'd';
            rcx /= toc::size_dword;
        } else if ((rcx % toc::size_word) == 0) {
            rep_size = 'w';
            rcx /= toc::size_word;
        }
        tc.asm_cmd(tok(), os, indent, "mov", "rcx", std::to_string(rcx));

        // copy
        toc::asm_repe_cmps(os, indent, rep_size);

        tc.free_named_register(tok(), os, indent, "rcx");
        tc.free_named_register(tok(), os, indent, "rdi");
        tc.free_named_register(tok(), os, indent, "rsi");

        // the labels
        const std::string lbl_if_equal{
            tc.create_unique_label(tok(), "cmps_eq")};
        const std::string lbl_end{tc.create_unique_label(tok(), "cmps_end")};

        toc::asm_jcc(os, indent, "e", lbl_if_equal); // je
        tc.asm_cmd(tok(), os, indent, "mov", dst, "false");
        toc::asm_jmp(os, indent, lbl_end);
        toc::asm_label(os, indent, lbl_if_equal);
        tc.asm_cmd(tok(), os, indent, "mov", dst, "true");
        toc::asm_label(os, indent, lbl_end);
    }
};
