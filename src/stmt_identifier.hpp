#pragma once

#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "unary_ops.hpp"

class stmt_identifier : public statement {
    expr_any array_index_expr_;
    bool has_array_index_expr_{};

  public:
    stmt_identifier(toc& tc, tokenizer& tz, token tk, unary_ops uops = {})
        : statement{std::move(tk), std::move(uops)} {
        if (not tz.is_next_char('[')) {
            return;
        }
        has_array_index_expr_ = true;
        const ident_info& ii{tc.make_ident_info(*this, false)};
        array_index_expr_ = {tc, tz, ii.type_ref, false};
        if (not tz.is_next_char(']')) {
            throw compiler_exception{
                tz, "expected '[' to close array index expression"};
        }
    }

    stmt_identifier() = default;
    stmt_identifier(const stmt_identifier&) = default;
    stmt_identifier(stmt_identifier&&) = default;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = default;
    auto operator=(stmt_identifier&&) -> stmt_identifier& = default;

    ~stmt_identifier() override = default;

    [[nodiscard]] auto is_expression() const -> bool override {
        return has_array_index_expr_;
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        if (dst.empty()) {
            return;
        }
        const ident_info& dst_info{tc.make_ident_info(*this, false)};
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
            array_index_expr_.tok(), dst_type_size)};
        const std::string src_nasm{memsize + "[" + reg + "]"};
        tc.asm_cmd(tok(), os, indent, "mov", dst, src_nasm);
        get_unary_ops().compile(tc, os, indent, dst);
        tc.free_scratch_register(os, indent, reg);
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (not has_array_index_expr_) {
            return;
        }
        os << '[';
        array_index_expr_.source_to(os);
        os << ']';
    }
};
