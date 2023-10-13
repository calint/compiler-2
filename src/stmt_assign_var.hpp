#pragma once
#include "expr_any.hpp"

class stmt_assign_var final : public statement {
  token type_{};
  expr_any exp_{};

public:
  inline stmt_assign_var(toc &tc, token name, token type, tokenizer &tz)
      : statement{move(name)}, type_{move(type)} {

    const ident_resolved &dst_resolved{tc.resolve_identifier(*this, false)};
    exp_ = {tc, tz, dst_resolved.tp, false};
    set_type(dst_resolved.tp);
  }

  inline stmt_assign_var() = default;
  inline stmt_assign_var(const stmt_assign_var &) = default;
  inline stmt_assign_var(stmt_assign_var &&) = default;
  inline auto operator=(const stmt_assign_var &) -> stmt_assign_var & = default;
  inline auto operator=(stmt_assign_var &&) -> stmt_assign_var & = default;

  inline ~stmt_assign_var() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    if (not type_.is_empty()) {
      os << ':';
      type_.source_to(os);
    }
    os << "=";
    exp_.source_to(os);
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.comment_source(*this, os, indent);

    const ident_resolved &dst_resolved{tc.resolve_identifier(*this, false)};

    if (exp_.is_bool() or exp_.is_assign_type_value()) {
      // boolean expresion
      exp_.compile(tc, os, indent, dst_resolved.id);
      tc.set_var_is_initiated(dst_resolved.id);
      return;
    }

    // number (register) expression
    exp_.compile(tc, os, indent, dst_resolved.id);
    tc.set_var_is_initiated(dst_resolved.id);
  }
};
