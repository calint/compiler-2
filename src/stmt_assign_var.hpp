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
    tc.source_comment(*this, os, indent);

    const ident_resolved &dst_resolved{tc.resolve_identifier(*this, false)};

    if (exp_.is_bool()) {
      exp_.compile(tc, os, indent, dst_resolved.id);
      tc.set_var_is_initiated(dst_resolved.id);
      return;
    }

    if (exp_.is_assign_type_value()) {
      exp_.compile(tc, os, indent, dst_resolved.id);
      tc.set_var_is_initiated(dst_resolved.id);
      return;
    }

    // number expression

    // compare generated instructions with and without scratch register
    // select the method that produces least instructions

    // try without scratch register
    stringstream ss1;
    exp_.compile(tc, ss1, indent, dst_resolved.id);

    // try with scratch register
    stringstream ss2;
    const string &reg{tc.alloc_scratch_register(*this, ss2, indent)};
    exp_.compile(tc, ss2, indent, reg);
    tc.asm_cmd(*this, ss2, indent, "mov", dst_resolved.id_nasm, reg);
    tc.free_scratch_register(ss2, indent, reg);

    // compare instruction count
    const size_t ss1_count{count_instructions(ss1)};
    const size_t ss2_count{count_instructions(ss2)};

    // select version with least instructions
    if (ss1_count <= ss2_count) {
      os << ss1.str();
    } else {
      os << ss2.str();
    }

    tc.set_var_is_initiated(dst_resolved.id);
  }

private:
  inline static auto count_instructions(stringstream &ss) -> size_t {
    const regex rxcomment{R"(^\s*;.*$)"};
    string line;
    size_t n{0};
    while (getline(ss, line)) {
      if (regex_search(line, rxcomment)) {
        continue;
      }
      n++;
    }
    return n;
  }
};
