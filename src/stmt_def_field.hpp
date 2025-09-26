#pragma once

#include "statement.hpp"
#include "toc.hpp"

class stmt_def_field final : public statement {
  token name_tk_{};
  unary_ops uops_{};
  token initial_value_{};

public:
  inline stmt_def_field(toc &tc, token tk, tokenizer &tz)
      : statement{std::move(tk)}, name_tk_{tz.next_token()} {

    if (name_tk_.is_empty()) {
      throw compiler_exception(name_tk_, "expected field name");
    }

    if (not tz.is_next_char('=')) {
      throw compiler_exception(tz, "expected '=' and initial value");
    }

    uops_ = unary_ops{tz};

    initial_value_ = tz.next_token();

    if (initial_value_.is_string()) {
      set_type(tc.get_type_void()); //? not implemented
    } else {
      set_type(tc.get_type_default()); //? only default type supported
    }

    tc.add_field(tok(), name_tk_.name(), this, initial_value_.is_string());
  }

  inline stmt_def_field() = default;
  inline stmt_def_field(const stmt_def_field &) = default;
  inline stmt_def_field(stmt_def_field &&) = default;
  inline auto operator=(const stmt_def_field &) -> stmt_def_field & = default;
  inline auto operator=(stmt_def_field &&) -> stmt_def_field & = default;

  inline ~stmt_def_field() override = default;

  inline void source_to(std::ostream &os) const override {
    statement::source_to(os);
    name_tk_.source_to(os);
    os << "=";
    uops_.source_to(os);
    initial_value_.source_to(os);
  }

  inline void
  compile(toc &tc, std::ostream &os, size_t indent,
          [[maybe_unused]] const std::string &dst = "") const override {

    tc.comment_source(*this, os, indent);
    os << name_tk_.name() << ':';
    if (initial_value_.is_string()) {
      os << " db '";
      initial_value_.compile_to(os);
      os << "'\n";
      toc::indent(os, indent);
      // the length of th string
      os << name_tk_.name() << ".len equ $-" << name_tk_.name() << "\n";
      return;
    }
    // default type is i64
    os << " dq ";
    os << uops_.to_string();
    initial_value_.compile_to(os);
    os << std::endl;
  }

  [[nodiscard]] inline auto is_in_data_section() const -> bool override {
    return true;
  }

  [[nodiscard]] inline auto is_string_field() const -> bool {
    return initial_value_.is_string();
  }
};
