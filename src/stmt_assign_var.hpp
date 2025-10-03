#pragma once
// reviewed: 2025-09-28

#include "decouple2.hpp"
#include "expr_any.hpp"
#include "stmt_identifier.hpp"

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

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info& dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier(), false)};
        if (dst_info.is_const()) {
            throw compiler_exception(tok(), "cannot assign to constant '" +
                                                dst_info.id + "'");
        }

        const identifier_elem& e{stmt_ident_.elems().at(0)};
        // is it index into array?
        if (e.has_array_index_expr) {
            // indexing into an array
            const std::string& reg{
                tc.alloc_scratch_register(tok(), os, indent)};
            e.array_index_expr->compile(tc, os, indent, reg);
            const size_t dst_type_size{dst_info.type_ref.size()};
            std::string reg_scaled;
            switch (dst_type_size) {
            case 1:
                reg_scaled = reg;
                break;
            case 2:
                reg_scaled = reg + " * 2";
                break;
            case 4:
                reg_scaled = reg + " * 4";
                break;
            case 8:
                reg_scaled = reg + " * 8";
                break;
            default:
                tc.asm_cmd(tok(), os, indent, "imul", reg,
                           std::to_string(dst_type_size));
                reg_scaled = reg;
                break;
            }
            tc.asm_cmd(tok(), os, indent, "lea", reg,
                       "[rsp + " + reg_scaled + " - " +
                           std::to_string(-dst_info.stack_ix) + "]");
            // note: -dst_info.stack_ix for nicer source formatting
            const std::string& memsize{type::get_memory_operand_for_size(
                tok(), dst_info.type_ref.size())};
            expr_.compile(tc, os, indent, memsize + " [" + reg + "]");
            tc.free_scratch_register(os, indent, reg);
            return;
        }
        // not an array index
        expr_.compile(tc, os, indent, dst_info.id);
        tc.set_var_is_initiated(dst_info.id);
    }

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }
};
