#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "expr_any.hpp"

class stmt_assign_var final : public statement {
    expr_any expr_;
    expr_any array_index_expr_;
    token ws1_;
    bool has_array_index_{};

  public:
    stmt_assign_var(toc& tc, token name, tokenizer& tz,
                    bool has_array_index = false)
        : statement{std::move(name)}, has_array_index_{has_array_index} {
        const ident_info& dst_info{tc.make_ident_info(*this, false)};
        if (has_array_index) {
            array_index_expr_ = {tc, tz, dst_info.type_ref, false};
            if (not tz.is_next_char(']')) {
                throw compiler_exception{
                    tz, "expected ']' after array index expression"};
            }
            ws1_ = tz.next_whitespace_token();
            if (not tz.is_next_char('=')) {
                throw compiler_exception{tz, "expected '=' and expression"};
            }
        }
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
        if (has_array_index_) {
            os << '[';
            array_index_expr_.source_to(os);
            os << ']';
            ws1_.source_to(os);
        }
        os << "=";
        expr_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info& dst_info{tc.make_ident_info(*this, false)};
        if (dst_info.is_const()) {
            throw compiler_exception(tok(), "cannot assign to constant '" +
                                                dst_info.id + "'");
        }
        // is it index into array?
        if (has_array_index_) {
            // indexing into an array
            const std::string& reg{
                tc.alloc_scratch_register(array_index_expr_.tok(), os, indent)};
            array_index_expr_.compile(tc, os, indent, reg);
            const size_t dst_type_size{dst_info.type_ref.size()};
            std::string reg_scaled;
            switch (dst_type_size) {
            case 1:
                reg_scaled = reg;
                break;
            case 2:
                reg_scaled = reg + "*2";
                break;
            case 4:
                reg_scaled = reg + "*4";
                break;
            case 8:
                reg_scaled = reg + "*8";
                break;
            default:
                tc.asm_cmd(array_index_expr_.tok(), os, indent, "imul", reg,
                           std::to_string(dst_type_size));
                reg_scaled = reg;
                break;
            }
            tc.asm_cmd(array_index_expr_.tok(), os, indent, "lea", reg,
                       "[rsp+" + reg_scaled +
                           std::to_string(dst_info.stack_ix_rel_rsp) + "]");
            const std::string& memsize{type::get_memory_operand_for_size(
                array_index_expr_.tok(), dst_info.type_ref.size())};
            expr_.compile(tc, os, indent, memsize + "[" + reg + "]");
            tc.free_scratch_register(os, indent, reg);
            return;
        }
        expr_.compile(tc, os, indent, dst_info.id);
        tc.set_var_is_initiated(dst_info.id);
    }

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }
};
