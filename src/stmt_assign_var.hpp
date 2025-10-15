#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include <cinttypes>
#include <ranges>

class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si)
        : statement{si.tok()}, stmt_ident_{std::move(si)} {

        const ident_info& dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier())};

        set_type(dst_info.type_ref);
        expr_ = {tc, tz, dst_info.type_ref, false};
    }

    ~stmt_assign_var() override = default;

    stmt_assign_var() = default;
    stmt_assign_var(const stmt_assign_var&) = default;
    stmt_assign_var(stmt_assign_var&&) = default;
    auto operator=(const stmt_assign_var&) -> stmt_assign_var& = default;
    auto operator=(stmt_assign_var&&) -> stmt_assign_var& = default;

    auto source_to(std::ostream& os) const -> void override {
        // note: all the source info is in `stmt_ident_`
        // statement::source_to(os);
        stmt_ident_.source_to(os);
        std::print(os, "=");
        expr_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get information about the destination of the compilation
        const ident_info dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier())};

        if (dst_info.is_const()) {
            throw compiler_exception{
                tok(),
                std::format("cannot assign to constant '{}'", dst_info.id)};
        }

        // is it a copy type instance expression?
        if (expr_.is_expr_type_value() and
            expr_.as_expr_type_value().is_make_copy()) {
            // yes, calculate the destination address to register 'rdi'

            // ; Copy RCX bytes from RSI to RDI
            // mov rsi, source_addr    ; source pointer
            // mov rdi, dest_addr      ; destination pointer
            // mov rcx, byte_count     ; number of bytes
            // rep movsb               ; repeat: copy byte [RSI++] to [RDI++]

            tc.alloc_named_register_or_throw(tok(), os, indent, "rsi");
            tc.alloc_named_register_or_throw(tok(), os, indent, "rdi");
            tc.alloc_named_register_or_throw(tok(), os, indent, "rcx");
            std::vector<std::string> allocated_registers;

            const std::string offset{stmt_identifier::compile_effective_address(
                tok(), tc, os, indent, stmt_ident_.elems(), allocated_registers,
                "", {})};

            tc.asm_cmd(tok(), os, indent, "lea", "rdi",
                       std::format("[{}]", offset));

            expr_.compile(tc, os, indent, "rsi");

            const ident_info ii{
                tc.make_ident_info(tok(), stmt_ident_.identifier())};

            tc.asm_cmd(tok(), os, indent, "mov", "rcx",
                       std::format("{}", ii.type_ref.size()));

            toc::asm_rep_movs(tok(), os, indent, 'b');

            for (const std::string& reg :
                 allocated_registers | std::views::reverse) {
                tc.free_scratch_register(tok(), os, indent, reg);
            }
            tc.free_named_register(tok(), os, indent, "rcx");
            tc.free_named_register(tok(), os, indent, "rdi");
            tc.free_named_register(tok(), os, indent, "rsi");

            return;
        }

        // DEBUG
        std::println(std::cerr, "[{}] identifier path: {}", tok().at_line(),
                     dst_info.id);
        for (size_t i = 0; i < dst_info.elem_path.size(); i++) {
            std::println(std::cerr, "  {} ; {} ; {}", dst_info.elem_path[i],
                         dst_info.type_path[i]->name(), dst_info.lea_path[i]);
        }

        // find the first element that has a "lea" and get accessor relative to
        // that "lea"
        size_t i{dst_info.elem_path.size()};
        bool found{};
        while (i) {
            i--;
            if (dst_info.lea_path[i] != "") {
                found = true;
                break;
            }
        }

        std::string dst_accessor{dst_info.id};
        if (found) {
            size_t offset = dst_info.type_path[i - 1]->field_offset(
                tok(), dst_info.elem_path, i);
            const std::string_view size_specifier{
                type::get_size_specifier(tok(), dst_info.type_ref.size())};
            if (offset == 0) {
                dst_accessor = std::format("{} [{}]", size_specifier,
                                           dst_info.lea_path[i]);
            } else {
                dst_accessor = std::format("{} [{} + {}]", size_specifier,
                                           dst_info.lea_path[i], offset);
            }
        }

        // does the identifier contain array indexing?
        if (not stmt_ident_.is_expression()) {
            // no, compile to 'dst_info'
            expr_.compile(tc, os, indent, dst_accessor);
            return;
        }

        // identifier contains array indexing
        // calculate effective address to the built-in type

        std::vector<std::string> allocated_registers;

        const std::string effective_address =
            stmt_identifier::compile_effective_address(
                tok(), tc, os, indent, stmt_ident_.elems(), allocated_registers,
                "", dst_info.lea_path);

        const std::string_view size_specifier{
            type::get_size_specifier(tok(), dst_info.type_ref.size())};

        expr_.compile(
            tc, os, indent,
            std::format("{} [{}]", size_specifier, effective_address));

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }
    }

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }

    [[nodiscard]] auto is_var_set(const std::string_view var) const
        -> bool override {
        return identifier() == var;
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        expr_.assert_var_not_used(var);
    }
};
