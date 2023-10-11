#pragma once
#include "assign_type_value.hpp"
#include "bool_ops_list.hpp"
#include "expr_ops_list.hpp"

// holds 'expr_ops_list', 'bool_ops_list', 'assign_type_field'
class expr_any final : public statement {
  variant<expr_ops_list, bool_ops_list, assign_type_value> eols_{};

public:
  inline expr_any(toc &tc, tokenizer &tz, const type &tp, bool in_args)
      : statement{tz.next_whitespace_token()} {

    if (not tp.is_built_in()) {
      eols_ = assign_type_value{tc, tz, tp};
      return;
    }

    if (tp.name() == tc.get_type_bool().name()) {
      eols_ = bool_ops_list{tc, tz};
      set_type(tp);
      return;
    }

    expr_ops_list e{tc, tz, in_args};
    set_type(e.get_type()); //? const ref still valid after move?
    eols_ = move(e);
  }

  inline expr_any() = default;
  inline expr_any(const expr_any &) = default;
  inline expr_any(expr_any &&) = default;
  inline auto operator=(const expr_any &) -> expr_any & = default;
  inline auto operator=(expr_any &&) -> expr_any & = default;

  inline ~expr_any() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    switch (eols_.index()) {
    case 0:
      get<expr_ops_list>(eols_).source_to(os);
      return;
    case 1:
      get<bool_ops_list>(eols_).source_to(os);
      return;
    case 2:
      get<assign_type_value>(eols_).source_to(os);
      return;
    default:
      throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                            std::to_string(__LINE__));
    }
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      const string &dst = "") const override {

    tc.source_comment(*this, os, indent);

    if (eols_.index() == 0) {
      // number expression
      const expr_ops_list &eol{get<expr_ops_list>(eols_)};
      eol.compile(tc, os, indent, dst);
      return;
    }

    if (eols_.index() == 1) {
      // bool expression
      const ident_resolved &dst_resolved{
          tc.resolve_identifier(*this, dst, false)};
      const bool_ops_list &eol{get<bool_ops_list>(eols_)};
      if (not eol.is_expression()) {
        const ident_resolved &src_resolved{tc.resolve_identifier(eol, false)};
        tc.asm_cmd(*this, os, indent, "mov", dst_resolved.id_nasm,
                   src_resolved.id_nasm);
        return;
      }
      // expression
      const string &call_path{tc.get_inline_call_path(tok())};
      const string &src_loc{tc.source_location_for_use_in_label(tok())};
      const string &postfix{src_loc +
                            (call_path.empty() ? "" : ("_" + call_path))};
      const string &jmp_to_if_true{"true_" + postfix};
      const string &jmp_to_if_false{"false_" + postfix};
      const string &jmp_to_end{"end_" + postfix};
      optional<bool> const_eval{
          eol.compile(tc, os, indent, jmp_to_if_false, jmp_to_if_true, false)};
      if (const_eval) {
        if (*const_eval) {
          // constant evaluation to true
          toc::asm_label(*this, os, indent, jmp_to_if_true);
          tc.asm_cmd(*this, os, indent, "mov", dst_resolved.id_nasm, "1");
          return;
        }
        // constant evaluation to false
        toc::asm_label(*this, os, indent, jmp_to_if_false);
        tc.asm_cmd(*this, os, indent, "mov", dst_resolved.id_nasm, "0");
        return;
      }
      // not constant evaluation
      toc::asm_label(*this, os, indent, jmp_to_if_true);
      tc.asm_cmd(*this, os, indent, "mov", dst_resolved.id_nasm, "1");
      toc::asm_jmp(*this, os, indent, jmp_to_end);
      toc::asm_label(*this, os, indent, jmp_to_if_false);
      tc.asm_cmd(*this, os, indent, "mov", dst_resolved.id_nasm, "0");
      toc::asm_label(*this, os, indent, jmp_to_end);
      return;
    }

    if (eols_.index() == 2) {
      // assign type value
      const assign_type_value &atl{get<assign_type_value>(eols_)};
      atl.compile(tc, os, indent, dst);
      return;
    }
  }

  [[nodiscard]] inline auto is_bool() const -> bool {
    return eols_.index() == 1;
  }

  [[nodiscard]] inline auto is_expression() const -> bool override {
    if (eols_.index() == 0) {
      const expr_ops_list &eol{get<expr_ops_list>(eols_)};
      return eol.is_expression();
    }
    if (eols_.index() == 1) {
      // bool expression
      const bool_ops_list &eol{get<bool_ops_list>(eols_)};
      return eol.is_expression();
    }
    throw panic_exception("expr_any:1");
  }

  [[nodiscard]] inline auto identifier() const -> const string & override {
    if (eols_.index() == 0) {
      const expr_ops_list &eol{get<expr_ops_list>(eols_)};
      return eol.identifier();
    }
    if (eols_.index() == 1) {
      // bool expression
      const bool_ops_list &eol{get<bool_ops_list>(eols_)};
      return eol.identifier();
    }
    throw panic_exception("expr_any:2");
  }

  [[nodiscard]] inline auto get_unary_ops() const
      -> const unary_ops & override {
    if (eols_.index() == 0) {
      const expr_ops_list &eol{get<expr_ops_list>(eols_)};
      return eol.get_unary_ops();
    }
    if (eols_.index() == 1) {
      // bool expression
      const bool_ops_list &eol{get<bool_ops_list>(eols_)};
      return eol.get_unary_ops(); //? can there be unary ops on bool
    }
    throw panic_exception("expr_any:3");
  }

  [[nodiscard]] inline auto is_assign_type_value() const -> bool {
    return eols_.index() == 2;
  }

  [[nodiscard]] inline auto as_assign_type_value() const
      -> const assign_type_value & {
    return get<assign_type_value>(eols_);
  }
};
