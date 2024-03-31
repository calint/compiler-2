#pragma once
#include "expr_any.hpp"

class stmt_assign_var final : public statement {
  token type_tk_{};
  expr_any expr_{};

public:
  inline stmt_assign_var(toc &tc, token name, token type, tokenizer &tz)
      : statement{move(name)}, type_tk_{move(type)} {

    const ident_resolved &dst_resolved{tc.resolve_identifier(*this, false)};
    set_type(dst_resolved.type_ref);
    expr_ = {tc, tz, dst_resolved.type_ref, false};
  }

  inline stmt_assign_var() = default;
  inline stmt_assign_var(const stmt_assign_var &) = default;
  inline stmt_assign_var(stmt_assign_var &&) = default;
  inline auto operator=(const stmt_assign_var &) -> stmt_assign_var & = default;
  inline auto operator=(stmt_assign_var &&) -> stmt_assign_var & = default;

  inline ~stmt_assign_var() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    if (not type_tk_.is_empty()) {
      os << ':';
      type_tk_.source_to(os);
    }
    os << "=";
    expr_.source_to(os);
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {

    tc.comment_source(*this, os, indent);

    const ident_resolved &dst_resolved{tc.resolve_identifier(*this, false)};

    if (dst_resolved.is_const()) {
      throw compiler_exception(tok(), "cannot assign to constant '" +
                                          dst_resolved.id + "'");
    }

    expr_.compile(tc, os, indent, dst_resolved.id);

    tc.set_var_is_initiated(dst_resolved.id);
  }
};
