#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "decouple2.hpp"
#include "expr_any.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"

class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si)
        : statement{si.tok()}, stmt_ident_{std::move(si)} {

        const ident_info& dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier(), false)};

        set_type(dst_info.type_ref);
        expr_ = {tc, tz, dst_info.type_ref, false};
    }

    stmt_assign_var() = default;
    stmt_assign_var(const stmt_assign_var&) = default;
    stmt_assign_var(stmt_assign_var&&) = default;
    auto operator=(const stmt_assign_var&) -> stmt_assign_var& = default;
    auto operator=(stmt_assign_var&&) -> stmt_assign_var& = default;

    ~stmt_assign_var() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        stmt_ident_.source_to(os);
        os << '=';
        expr_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier(), false)};
        if (dst_info.is_const()) {
            throw compiler_exception(tok(), "cannot assign to constant '" +
                                                dst_info.id + "'");
        }

        if (not stmt_ident_.is_expression()) {
            // no index calculations
            expr_.compile(tc, os, indent, dst_info.id);
            return;
        }

        // identifier contains index offsets

        if (stmt_ident_.elems().size() == 1) {
            const identifier_elem& ie{stmt_ident_.elems().at(0)};
            const ident_info ii{
                tc.make_ident_info(tok(), stmt_ident_.identifier(), false)};
            if (not ie.has_array_index_expr) {
                expr_.compile(tc, os, indent, ii.id);
                return;
            }

            const std::string& reg_accum{
                tc.alloc_scratch_register(tok(), os, indent)};
            tc.asm_cmd(tok(), os, indent, "lea", reg_accum,
                       "[rsp - " + std::to_string(-ii.stack_ix) + "]");
            const std::string& reg_index{
                tc.alloc_scratch_register(tok(), os, indent)};
            ie.array_index_expr->compile(tc, os, indent, reg_index);
            tc.asm_cmd(tok(), os, indent, "imul", reg_index,
                       std::to_string(ii.type_ref.size()));
            tc.asm_cmd(tok(), os, indent, "add", reg_accum, reg_index);
            tc.free_scratch_register(os, indent, reg_index);
            const std::string& memsize{type::get_memory_operand_for_size(
                tok(), dst_info.type_ref.size())};
            expr_.compile(tc, os, indent, memsize + " [" + reg_accum + "]");
            tc.free_scratch_register(os, indent, reg_accum);
            return;
        }
        const identifier_elem& ie_base{stmt_ident_.elems().at(0)};
        const std::string& reg_accum{
            tc.alloc_scratch_register(tok(), os, indent)};
        std::string path = ie_base.name_tk.name();
        const ident_info ii_base{tc.make_ident_info(tok(), path, false)};
        tc.asm_cmd(tok(), os, indent, "lea", reg_accum,
                   "[rsp - " + std::to_string(-ii_base.stack_ix) + "]");
        size_t i{0};
        size_t accum_offset{};
        while (true) {
            if (i + 1 == stmt_ident_.elems().size()) {
                break;
            }
            const identifier_elem& ie{stmt_ident_.elems().at(i)};
            const identifier_elem& ie_nxt{stmt_ident_.elems().at(i + 1)};
            const ident_info ii{tc.make_ident_info(tok(), path, false)};
            const std::string path_nxt{path + "." + ie_nxt.name_tk.name()};
            const ident_info ii_nxt{tc.make_ident_info(tok(), path_nxt, false)};
            if (not ie.has_array_index_expr) {
                accum_offset += toc::get_field_offset_in_type(
                    tok(), ii.type_ref, ie_nxt.name_tk.name());
                path = path_nxt;
                i++;
                continue;
            }
            const std::string& reg_index{
                tc.alloc_scratch_register(tok(), os, indent)};
            ie.array_index_expr->compile(tc, os, indent, reg_index);
            tc.asm_cmd(tok(), os, indent, "imul", reg_index,
                       std::to_string(ii.type_ref.size()));
            tc.asm_cmd(tok(), os, indent, "add", reg_accum, reg_index);
            tc.free_scratch_register(os, indent, reg_index);
            accum_offset += toc::get_field_offset_in_type(
                tok(), ii.type_ref, ie_nxt.name_tk.name());
            path = path_nxt;
            i++;
        }
        if (accum_offset != 0) {
            tc.asm_cmd(tok(), os, indent, "add", reg_accum,
                       std::to_string(accum_offset));
        }
        const std::string& memsize{
            type::get_memory_operand_for_size(tok(), dst_info.type_ref.size())};
        expr_.compile(tc, os, indent, memsize + " [" + reg_accum + "]");
        tc.free_scratch_register(os, indent, reg_accum);
    }

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }
};
