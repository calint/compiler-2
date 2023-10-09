#pragma once
#include "stmt_assign_var.hpp"

class null_buffer : public streambuf {
public:
  auto overflow(int c) -> int override { return c; }
};

class null_stream : public ostream {
  null_buffer nb_{};

public:
  null_stream() : ostream(&nb_) {}
};

class stmt_def_var final : public statement {
  token name_{};
  token type_{};
  stmt_assign_var initial_value_{};

public:
  inline stmt_def_var(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, name_{tz.next_token()} {
    if (tz.is_next_char(':')) {
      type_ = tz.next_token();
    }
    if (not tz.is_next_char('=')) {
      throw compiler_exception(name_, "expected '=' and initializer");
    }

    const type &tp{type_.name().empty()
                       ? tc.get_type_default()
                       : tc.get_type_or_throw(*this, type_.name())};
    set_type(tp);
    null_stream ns{};
    tc.add_var(*this, ns, 0, name_.name(), tp, false);
    initial_value_ = {tc, name_, type_, tz};
  }

  inline stmt_def_var() = default;
  inline stmt_def_var(const stmt_def_var &) = default;
  inline stmt_def_var(stmt_def_var &&) = default;
  inline auto operator=(const stmt_def_var &) -> stmt_def_var & = default;
  inline auto operator=(stmt_def_var &&) -> stmt_def_var & = default;

  inline ~stmt_def_var() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    initial_value_.source_to(os);
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.add_var(*this, os, indent, name_.name(), get_type(), false);
    tc.source_comment(*this, os, indent);
    initial_value_.compile(tc, os, indent, name_.name());
  }
};
