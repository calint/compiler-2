#pragma once
// reviewed: 2025-09-28

#include <format>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
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

        // DEBUG
        // std::println(std::cerr, "[{}] identifier path: {}", tok().at_line(),
        //              dst_info.id);
        // for (size_t i = 0; i < dst_info.elem_path.size(); i++) {
        //     std::println(std::cerr, "  {} ; {} ; {}", dst_info.elem_path[i],
        //                  dst_info.type_path[i]->name(),
        //                  dst_info.lea_path[i]);
        // }

        // find the first element from top that has a "lea" and get accessor
        // relative to that
        size_t i{dst_info.elem_path.size()};
        std::string lea;
        while (i) {
            i--;
            if (not dst_info.lea_path[i].empty()) {
                lea = dst_info.lea_path[i];
                break;
            }
        }

        if (dst_info.type_ptr->is_built_in()) {
            std::string dst_accessor{dst_info.id};
            if (not lea.empty()) {
                const std::string_view size_specifier{
                    toc::get_size_specifier(dst_info.type_ptr->size())};

                const size_t offset{dst_info.type_path[i]->field_offset(
                    tok(), std::span{dst_info.elem_path}.subspan(i))};

                if (offset != 0) {
                    operand operand{lea};
                    operand.displacement += static_cast<int>(offset);
                    dst_accessor = std::format("{} [{}]", size_specifier,
                                               operand.to_string());
                } else {
                    dst_accessor = std::format("{} [{}]", size_specifier,
                                               dst_info.lea_path[i]);
                }
            }

            // does the identifier contain array indexing?
            if (not stmt_ident_.is_indexed()) {
                // no, compile to 'dst_info'
                expr_.compile(tc, os, indent, dst_accessor);
                return;
            }

            // identifier contains array indexing
            // calculate effective address to the built-in type

            std::vector<std::string> allocated_registers;

            const std::string effective_address =
                stmt_identifier::compile_effective_address(
                    tok(), tc, os, indent, stmt_ident_.elems(),
                    allocated_registers, "", dst_info.lea_path);

            const std::string_view size_specifier{
                toc::get_size_specifier(dst_info.type_ptr->size())};

            expr_.compile(
                tc, os, indent,
                std::format("{} [{}]", size_specifier, effective_address));

            for (const std::string& reg :
                 allocated_registers | std::views::reverse) {
                tc.free_scratch_register(tok(), os, indent, reg);
            }
            return;
        }

        // not-builtin type

        operand dst_op;
        std::vector<std::string> allocated_registers;

        if (stmt_ident_.is_indexed() or dst_info.has_lea()) {
            const std::string& dst_lea{stmt_ident_.compile_lea(
                tok(), tc, os, indent, allocated_registers, "",
                dst_info.lea_path)};

            dst_op = operand{dst_lea};
        } else {
            if (lea.empty()) {
                dst_op =
                    operand{toc::get_operand_address_str(dst_info.operand)};
            } else {
                dst_op = operand{lea};
            }
        }

        expr_.as_expr_type_value().compile_assign(tc, os, indent,
                                                  *dst_info.type_ptr, dst_op);

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
