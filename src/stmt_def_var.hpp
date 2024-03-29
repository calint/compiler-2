#pragma once
#include "stmt_assign_var.hpp"

class null_stream : public ostream {
  class null_buffer : public streambuf {
  public:
    auto overflow(int c) -> int override { return c; }
  } nb_{};

public:
  null_stream() : ostream(&nb_) {}
};

class stmt_def_var final : public statement {
  token name_tk_{};
  token type_tk_{};
  stmt_assign_var initial_value_{};

public:
  inline stmt_def_var(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, name_tk_{tz.next_token()} {

    // check if type declared
    if (tz.is_next_char(':')) {
      type_tk_ = tz.next_token();
    }
    // expect initialization
    if (not tz.is_next_char('=')) {
      throw compiler_exception(name_tk_, "expected '=' and initializer");
    }
    // get type reference from token
    const type &tp{type_tk_.name().empty()
                       ? tc.get_type_default()
                       : tc.get_type_or_throw(type_tk_, type_tk_.name())};
    set_type(tp);

    // dry-run compilation to null stream to set up the program state in tocI
    null_stream null_strm{};
    tc.add_var(name_tk_, null_strm, 0, name_tk_.name(), tp, false);

    // initialization code
    initial_value_ = {tc, name_tk_, type_tk_, tz};
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

    tc.add_var(name_tk_, os, indent, name_tk_.name(), get_type(), false);
    tc.comment_source(*this, os, indent);
    initial_value_.compile(tc, os, indent, name_tk_.name());
  }
};
