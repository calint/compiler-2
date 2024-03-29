#pragma once
#include "bool_ops_list.hpp"

class stmt_if_branch final : public statement {
  bool_ops_list bol_{};
  stmt_block code_{};

public:
  inline stmt_if_branch(toc &tc, tokenizer &tz)
      : statement{tz.next_whitespace_token()}, bol_{tc, tz}, code_{tc, tz} {

    set_type(tc.get_type_void());
  }

  inline stmt_if_branch() = default;
  inline stmt_if_branch(const stmt_if_branch &) = default;
  inline stmt_if_branch(stmt_if_branch &&) = default;
  inline auto operator=(const stmt_if_branch &) -> stmt_if_branch & = default;
  inline auto operator=(stmt_if_branch &&) -> stmt_if_branch & = default;

  inline ~stmt_if_branch() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    bol_.source_to(os);
    code_.source_to(os);
  }

  // returns the label where the if branch begins evaluating the boolean
  // expression
  [[nodiscard]] inline auto if_bgn_label(const toc &tc) const -> string {
    // construct a unique label considering in-lined functions
    const string &call_path{tc.get_inline_call_path(tok())};
    return "if_" + tc.source_location_for_use_in_label(tok()) +
           (call_path.empty() ? "" : "_" + call_path);
  }

  inline void compile([[maybe_unused]] toc &tc, [[maybe_unused]] ostream &os,
                      [[maybe_unused]] size_t indent,
                      [[maybe_unused]] const string &dst) const override {

    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
  }

  inline auto compile(toc &tc, ostream &os, size_t indent,
                      const string &jmp_to_if_false_label,
                      const string &jmp_to_after_code_label) const
      -> optional<bool> {

    const string &if_bgn_lbl{if_bgn_label(tc)};
    const string &jmp_to_if_true_lbl{if_bgn_lbl + "_code"};
    // the beginning of this branch
    toc::asm_label(tok(), os, indent, if_bgn_lbl);
    // compile boolean ops list
    optional<bool> const_eval{bol_.compile(
        tc, os, indent, jmp_to_if_false_label, jmp_to_if_true_lbl, false)};
    // if constant boolean expression
    if (const_eval) {
      // if evaluated to true then this branch code will execute
      if (*const_eval) {
        // the code of the branch
        code_.compile(tc, os, indent);
      }
      return *const_eval;
    }
    // the label where to jump if evaluation of condition is true
    toc::asm_label(tok(), os, indent, jmp_to_if_true_lbl);
    // the code of the branch
    code_.compile(tc, os, indent);
    // after the code of the branch is executed jump to the end of
    //   the 'if ... else if ... else ...' block
    //   if jump label not provided then there is no 'else' and this is the last
    //   'if' so just continue execution
    if (not jmp_to_after_code_label.empty()) {
      toc::asm_jmp(tok(), os, indent, jmp_to_after_code_label);
    }
    return nullopt;
  }
};
