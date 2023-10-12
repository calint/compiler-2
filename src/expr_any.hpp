#pragma once
#include "bool_ops_list.hpp"
#include "expr_ops_list.hpp"
#include "expr_type_value.hpp"

class expr_any final : public statement {
  variant<expr_ops_list, bool_ops_list, expr_type_value> var_{};

public:
  inline expr_any(toc &tc, tokenizer &tz, const type &tp, bool in_args)
      : statement{tz.next_whitespace_token()} {

    set_type(tp);

    if (not tp.is_built_in()) {
      // destination is not built-in (register) value
      // assume assign type value
      var_ = expr_type_value{tc, tz, tp};
      return;
    }

    if (tp.name() == tc.get_type_bool().name()) {
      // destination is boolean
      var_ = bool_ops_list{tc, tz};
      return;
    }

    // destination is built-in (register) value
    var_ = expr_ops_list{tc, tz, in_args};
  }

  inline expr_any() = default;
  inline expr_any(const expr_any &) = default;
  inline expr_any(expr_any &&) = default;
  inline auto operator=(const expr_any &) -> expr_any & = default;
  inline auto operator=(expr_any &&) -> expr_any & = default;

  inline ~expr_any() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    switch (var_.index()) {
    case 0:
      get<expr_ops_list>(var_).source_to(os);
      return;
    case 1:
      get<bool_ops_list>(var_).source_to(os);
      return;
    case 2:
      get<expr_type_value>(var_).source_to(os);
      return;
    default:
      throw panic_exception("expr_any:1");
    }
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      const string &dst = "") const override {

    tc.source_comment(*this, os, indent);

    switch (var_.index()) {
    default:
      throw panic_exception("expr_any:2");
    case 0:
      // number expression
      get<expr_ops_list>(var_).compile(tc, os, indent, dst);
      return;
    case 2:
      // assign type value
      get<expr_type_value>(var_).compile(tc, os, indent, dst);
      return;
    case 1:
      // bool expression
      const ident_resolved &dst_resolved{
          tc.resolve_identifier(tok(), dst, false)};
      const bool_ops_list &bol{get<bool_ops_list>(var_)};
      if (not bol.is_expression()) {
        const ident_resolved &src_resolved{tc.resolve_identifier(bol, false)};
        tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm,
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
          bol.compile(tc, os, indent, jmp_to_if_false, jmp_to_if_true, false)};
      if (const_eval) {
        if (*const_eval) {
          // constant evaluation to true
          toc::asm_label(tok(), os, indent, jmp_to_if_true);
          tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm, "1");
          return;
        }
        // constant evaluation to false
        toc::asm_label(tok(), os, indent, jmp_to_if_false);
        tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm, "0");
        return;
      }
      // not constant evaluation
      toc::asm_label(tok(), os, indent, jmp_to_if_true);
      tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm, "1");
      toc::asm_jmp(tok(), os, indent, jmp_to_end);
      toc::asm_label(tok(), os, indent, jmp_to_if_false);
      tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm, "0");
      toc::asm_label(tok(), os, indent, jmp_to_end);
      return;
    }
  }

  [[nodiscard]] inline auto is_bool() const -> bool {
    return var_.index() == 1;
  }

  [[nodiscard]] inline auto is_expression() const -> bool override {
    switch (var_.index()) {
    case 0:
      // number expression
      return get<expr_ops_list>(var_).is_expression();
    case 1:
      // bool expression
      return get<bool_ops_list>(var_).is_expression();
    case 2:
      // assign value
      return get<expr_type_value>(var_).is_expression();
    default:
      throw panic_exception("expr_any:3");
    }
    // note. 'expr_type_value' cannot be expression
  }

  [[nodiscard]] inline auto identifier() const -> const string & override {
    switch (var_.index()) {
    case 0:
      return get<expr_ops_list>(var_).identifier();
    case 1:
      return get<bool_ops_list>(var_).identifier();
    case 2:
      return get<expr_type_value>(var_).identifier();
    default:
      throw panic_exception("expr_any:4");
    }
  }

  [[nodiscard]] inline auto get_unary_ops() const
      -> const unary_ops & override {

    switch (var_.index()) {
    case 0:
      return get<expr_ops_list>(var_).get_unary_ops();
    case 1:
      return get<bool_ops_list>(var_).get_unary_ops();
    case 2:
      return get<expr_type_value>(var_).get_unary_ops();
    default:
      throw panic_exception("expr_any:5");
    }
    // note. 'expr_type_value' does not have 'unary_ops' and cannot be
    // argument in call
  }

  [[nodiscard]] inline auto is_assign_type_value() const -> bool {
    return var_.index() == 2;
  }

  [[nodiscard]] inline auto as_assign_type_value() const
      -> const expr_type_value & {
    return get<expr_type_value>(var_);
  }
};
