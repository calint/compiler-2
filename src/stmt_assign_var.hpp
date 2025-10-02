#pragma once
// reviewed: 2025-09-28

#include "expr_any.hpp"

class stmt_assign_var final : public statement {
    expr_any expr_;

  public:
    stmt_assign_var(toc& tc, token name, tokenizer& tz)
        : statement{std::move(name)} {

        const ident_info& dst_info{tc.make_ident_info(*this, false)};
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
        expr_.compile(tc, os, indent, dst_info.id);
        tc.set_var_is_initiated(dst_info.id);
    }

    auto expression() const -> const expr_any& { return expr_; }
};
