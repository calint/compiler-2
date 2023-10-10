#pragma once
#include "compiler_exception.hpp"
#include "toc.hpp"

class stmt_def_field final : public statement {
  token name_{};
  unary_ops uops_{};
  token initial_value_{};

public:
  inline stmt_def_field(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, name_{tz.next_token()} {

    if (name_.is_empty()) {
      throw compiler_exception(name_, "expected field name");
    }

    if (not tz.is_next_char('=')) {
      throw compiler_exception(tz, "expected '=' and initial value");
    }

    uops_ = unary_ops{tz};
    initial_value_ = tz.next_token();

    if (initial_value_.is_string()) {
      set_type(tc.get_type_void()); // ! not implemented
    } else {
      set_type(tc.get_type_default());
    }
    tc.add_field(*this, name_.name(), this, initial_value_.is_string());
  }

  inline stmt_def_field() = default;
  inline stmt_def_field(const stmt_def_field &) = default;
  inline stmt_def_field(stmt_def_field &&) = default;
  inline auto operator=(const stmt_def_field &) -> stmt_def_field & = default;
  inline auto operator=(stmt_def_field &&) -> stmt_def_field & = default;

  inline ~stmt_def_field() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    name_.source_to(os);
    os << "=";
    uops_.source_to(os);
    initial_value_.source_to(os);
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.source_comment(*this, os, indent);
    os << name_.name() << ':';
    if (initial_value_.is_string()) {
      os << " db '";
      initial_value_.compile_to(os);
      os << "'\n";
      toc::indent(os, indent);
      os << name_.name() << ".len equ $-" << name_.name() << "\n";
      return;
    }
    os << " dq ";
    os << uops_.to_string();
    initial_value_.compile_to(os);
    os << endl;
  }

  [[nodiscard]] inline auto is_in_data_section() const -> bool override {
    return true;
  }

  [[nodiscard]] inline auto is_string_field() const -> bool {
    return initial_value_.is_string();
  }
};
