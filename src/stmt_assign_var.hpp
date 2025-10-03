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
            expr_.compile(tc, os, indent, dst_info.id);
            return;
        }

        const std::string& reg_offset{
            tc.alloc_scratch_register(tok(), os, indent)};

        stmt_identifier::compile_address_calculation(
            tok(), tc, os, indent, stmt_ident_.elems(), reg_offset);

        const std::string& memsize{
            type::get_memory_operand_for_size(tok(), dst_info.type_ref.size())};
        expr_.compile(tc, os, indent, memsize + " [" + reg_offset + "]");

        tc.free_scratch_register(os, indent, reg_offset);
    }

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }
};
