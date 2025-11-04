#pragma once
// reviewed: 2025-09-28

#include <format>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"

class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;
    token ws1_;

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si, token ws1 = {})
        : statement{si.tok()}, stmt_ident_{std::move(si)}, ws1_{ws1} {

        // note: ws1 is forwarded by `stmt_def_var` to make the `source_to`
        //       accurate when `stmt_assign_var` is created within the context
        //       of `stmt_def_var`

        const ident_info& dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier())};

        set_type(*dst_info.type_ptr);
        expr_ = {tc, tz, *dst_info.type_ptr, false};
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
        ws1_.source_to(os);
        expr_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get information about the destination of the compilation
        ident_info dst_info{
            tc.make_ident_info(tok(), stmt_ident_.identifier())};

        if (dst_info.is_const()) {
            throw compiler_exception{
                tok(), std::format("cannot assign to constant '{}'",
                                   dst_info.const_value)};
        }

        if (dst_info.type_ptr->is_built_in()) {
            std::vector<std::string> lea_registers;
            dst_info.operand = tc.get_lea_operand(os, indent, stmt_ident_,
                                                  dst_info, lea_registers);
            expr_.compile(tc, os, indent, dst_info);
            for (const std::string& reg : lea_registers | std::views::reverse) {
                tc.free_scratch_register(tok(), os, indent, reg);
            }
            return;
        }

        // not-builtin type

        operand dst_op;
        std::vector<std::string> lea_registers;

        if (stmt_ident_.is_indexed() or dst_info.has_lea()) {
            const std::string& dst_lea{stmt_ident_.compile_lea(
                tok(), tc, os, indent, lea_registers, "", dst_info.lea_path)};

            dst_op = operand{dst_lea};
        } else {
            if (dst_info.lea.empty()) {
                dst_op = operand{dst_info.operand.address_str()};
            } else {
                dst_op = operand{dst_info.lea};
            }
        }

        expr_.as_expr_type_value().compile_assign(tc, os, indent,
                                                  *dst_info.type_ptr, dst_op);

        for (const std::string& reg : lea_registers | std::views::reverse) {
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
