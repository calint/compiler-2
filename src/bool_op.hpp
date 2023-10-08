#pragma once
#include "expr_ops_list.hpp"

class bool_op final : public statement {
public:
  inline bool_op(toc &tc, tokenizer &t) : statement{t.next_whitespace_token()} {
    set_type(tc.get_type_bool());

    bool is_not{false};
    // if not a=3
    while (true) {
      token tk{t.next_token()};
      if (not tk.is_name("not")) {
        t.put_back_token(tk);
        break;
      }
      is_not = not is_not;
      nots_.push_back(move(tk));
    }
    is_not_ = is_not;

    lhs_ = {tc, t, true};

    if (t.is_next_char('=')) {
      if (not t.is_next_char('=')) {
        throw compiler_exception(t, "expected '=='");
      }
      op_ = "==";
    } else if (t.is_next_char('!')) {
      if (not t.is_next_char('=')) {
        throw compiler_exception(t, "expected '!='");
      }
      op_ = "!=";
    } else if (t.is_next_char('<')) {
      if (t.is_next_char('=')) {
        op_ = "<=";
      } else {
        op_ = "<";
      }
    } else if (t.is_next_char('>')) {
      if (t.is_next_char('=')) {
        op_ = ">=";
      } else {
        op_ = ">";
      }
    } else {
      // if a ...
      is_shorthand_ = true;
      resolve_if_op_is_expression(tc);
      return;
      // throw compiler_error(*this,"expected boolean operation
      // '=','<','<=','>','>='");
    }
    rhs_ = {tc, t};
    resolve_if_op_is_expression(tc);
  }

  inline bool_op() = default;
  inline bool_op(const bool_op &) = default;
  inline bool_op(bool_op &&) = default;
  inline auto operator=(const bool_op &) -> bool_op & = default;
  inline auto operator=(bool_op &&) -> bool_op & = default;

  inline ~bool_op() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);

    for (const token &e : nots_) {
      e.source_to(os);
    }

    lhs_.source_to(os);

    if (not is_shorthand_) {
      os << op_;
      rhs_.source_to(os);
    }
  }

  inline void compile([[maybe_unused]] toc &tc, [[maybe_unused]] ostream &os,
                      [[maybe_unused]] size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    throw compiler_exception(
        tok(), "this code should not be reached: " + string{__FILE__} + ":" +
                   to_string(__LINE__));
  }

  inline auto compile_or(toc &tc, ostream &os, size_t indent,
                         const string &jmp_to_if_true,
                         const bool inverted) const -> optional<bool> {
    const bool invert{inverted ? not is_not_ : is_not_};
    toc::indent(os, indent, true);
    tc.source_comment(os, "?", inverted ? " 'or' inverted: " : " ", *this);
    toc::asm_label(*this, os, indent, cmp_bgn_label(tc));
    if (is_shorthand_) {
      // check case when operand is constant
      if (not lhs_.is_expression()) {
        const ident_resolved &ir{tc.resolve_identifier(lhs_, false)};
        if (ir.is_const()) {
          bool b{lhs_.get_unary_ops().evaluate_constant(ir.const_value) != 0};
          if (invert) {
            b = !b;
          }
          toc::indent(os, indent, true);
          os << "const eval to " << (b ? "true" : "false") << endl;
          if (b) {
            toc::indent(os, indent);
            os << "jmp " << jmp_to_if_true << endl;
          }
          return b;
        }
      }
      resolve_cmp_shorthand(tc, os, indent, lhs_);
      toc::indent(os, indent);
      os << (not invert ? asm_jxx_for_op_inv("==") : asm_jxx_for_op("=="));
      os << " " << jmp_to_if_true << endl;
      return nullopt;
    }
    // check the case when both operands are constants
    if (not lhs_.is_expression() and not rhs_.is_expression()) {
      const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
      const ident_resolved &rhs_resolved{tc.resolve_identifier(rhs_, false)};
      if (lhs_resolved.is_const() and rhs_resolved.is_const()) {
        bool b{eval_constant(
            lhs_.get_unary_ops().evaluate_constant(lhs_resolved.const_value),
            op_,
            rhs_.get_unary_ops().evaluate_constant(rhs_resolved.const_value))};
        if (invert) {
          b = !b;
        }
        toc::indent(os, indent, true);
        os << "const eval to " << (b ? "true" : "false") << endl;
        if (b) {
          toc::indent(os, indent);
          os << "jmp " << jmp_to_if_true << endl;
        }
        return b;
      }
    }
    resolve_cmp(tc, os, indent, lhs_, rhs_);
    toc::indent(os, indent);
    os << (invert ? asm_jxx_for_op_inv(op_) : asm_jxx_for_op(op_));
    os << " " << jmp_to_if_true << endl;
    return nullopt;
  }

  inline auto compile_and(toc &tc, ostream &os, size_t indent,
                          const string &jmp_to_if_false,
                          const bool inverted) const -> optional<bool> {
    const bool invert{inverted ? not is_not_ : is_not_};
    toc::indent(os, indent, true);
    tc.source_comment(os, "?", inverted ? " 'and' inverted: " : " ", *this);
    toc::asm_label(*this, os, indent, cmp_bgn_label(tc));
    if (is_shorthand_) {
      // check case when operand is constant
      if (not lhs_.is_expression()) {
        const ident_resolved &ir{tc.resolve_identifier(lhs_, false)};
        if (ir.is_const()) {
          bool b{lhs_.get_unary_ops().evaluate_constant(ir.const_value) != 0};
          if (invert) {
            b = !b;
          }
          toc::indent(os, indent, true);
          os << "const eval to " << (b ? "true" : "false") << endl;
          if (not b) {
            toc::indent(os, indent);
            os << "jmp " << jmp_to_if_false << endl;
          }
          return b;
        }
      }
      resolve_cmp_shorthand(tc, os, indent, lhs_);
      toc::indent(os, indent);
      os << (not invert ? asm_jxx_for_op("==") : asm_jxx_for_op_inv("=="));
      os << " " << jmp_to_if_false << endl;
      return nullopt;
    }
    // check the case when both operands are constants
    if (not lhs_.is_expression() and not rhs_.is_expression()) {
      const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
      const ident_resolved &rhs_resolved{tc.resolve_identifier(rhs_, false)};
      if (lhs_resolved.is_const() and rhs_resolved.is_const()) {
        bool b{eval_constant(
            lhs_.get_unary_ops().evaluate_constant(lhs_resolved.const_value),
            op_,
            rhs_.get_unary_ops().evaluate_constant(rhs_resolved.const_value))};
        if (invert) {
          b = !b;
        }
        toc::indent(os, indent, true);
        os << "const eval to " << (b ? "true" : "false") << endl;
        if (not b) {
          toc::indent(os, indent);
          os << "jmp " << jmp_to_if_false << endl;
        }
        return b;
      }
    }
    resolve_cmp(tc, os, indent, lhs_, rhs_);
    toc::indent(os, indent);
    os << (invert ? asm_jxx_for_op(op_) : asm_jxx_for_op_inv(op_));
    os << " " << jmp_to_if_false << endl;
    return nullopt;
  }

  [[nodiscard]] inline auto cmp_bgn_label(const toc &tc) const -> string {
    const string &call_path{tc.get_inline_call_path(tok())};
    return "cmp_" + tc.source_location_for_label(tok()) +
           (call_path.empty() ? "" : "_" + call_path);
  }

  [[nodiscard]] inline auto identifier() const -> const string & override {
    assert(not is_expression());

    return lhs_.identifier();

    throw panic_exception("unexpected code path " + string{__FILE__} +
                               ":" + to_string(__LINE__));
  }

  [[nodiscard]] inline auto is_expression() const -> bool override {
    return is_expression_;
  }

private:
  inline void resolve_if_op_is_expression(toc &tc) {
    if (is_not_) {
      is_expression_ = true;
      return;
    }

    if (not is_shorthand_) {
      is_expression_ = true;
      return;
    }

    // short-hand expressions
    if (lhs_.is_expression()) {
      is_expression_ = true;
      return;
    }

    // check if lhs is a bool
    if (lhs_.get_type().name() == tc.get_type_bool().name()) {
      is_expression_ = false;
      return;
    }

    // if not expression then it is a single statement
    //   and identifier is valid
    const string &id{lhs_.identifier()};
    if (id == "true" or id == "false") {
      is_expression_ = false;
      return;
    }

    is_expression_ = true;
  }

  inline static auto eval_constant(const long int lh, const string &op,
                                   const long int rh) -> bool {
    if (op == "==") {
      return lh == rh;
    }
    if (op == "<") {
      return lh < rh;
    }
    if (op == "<=") {
      return lh <= rh;
    }
    if (op == ">") {
      return lh > rh;
    }
    if (op == ">=") {
      return lh >= rh;
    }
    throw panic_exception("unexpected code path " + string{__FILE__} +
                               ":" + to_string(__LINE__));
  }

  inline static auto asm_jxx_for_op(const string &op) -> string {
    if (op == "==") {
      return "je";
    }
    if (op == "!=") {
      return "jne";
    }
    if (op == "<") {
      return "jl";
    }
    if (op == "<=") {
      return "jle";
    }
    if (op == ">") {
      return "jg";
    }
    if (op == ">=") {
      return "jge";
    }

    throw panic_exception("unexpected code path " + string{__FILE__} +
                               ":" + to_string(__LINE__));
  }

  inline static auto asm_jxx_for_op_inv(const string &op) -> string {
    if (op == "==") {
      return "jne";
    }
    if (op == "!=") {
      return "je";
    }
    if (op == "<") {
      return "jge";
    }
    if (op == "<=") {
      return "jg";
    }
    if (op == ">") {
      return "jle";
    }
    if (op == ">=") {
      return "jl";
    }
    throw panic_exception("unexpected code path " + string{__FILE__} +
                               ":" + to_string(__LINE__));
  }

  inline void resolve_cmp(toc &tc, ostream &os, size_t indent,
                          const expr_ops_list &lh,
                          const expr_ops_list &rh) const {
    vector<string> allocated_registers;

    const string &dst{resolve_expr(tc, os, indent, lh, allocated_registers)};
    const string &src{resolve_expr(tc, os, indent, rh, allocated_registers)};

    tc.asm_cmd(*this, os, indent, "cmp", dst, src);

    // free allocated registers in reverse order
    for (auto it{allocated_registers.rbegin()};
         it != allocated_registers.rend(); ++it) {
      const string &reg{*it};
      tc.free_scratch_register(os, indent, reg);
    }
  }

  inline void resolve_cmp_shorthand(toc &tc, ostream &os, size_t indent,
                                    const expr_ops_list &lh) const {
    vector<string> allocated_registers;

    const string &dst{resolve_expr(tc, os, indent, lh, allocated_registers)};

    tc.asm_cmd(*this, os, indent, "cmp", dst, "0");

    // free allocated registers in reverse order
    for (auto it{allocated_registers.rbegin()};
         it != allocated_registers.rend(); ++it) {
      const string &reg{*it};
      tc.free_scratch_register(os, indent, reg);
    }
  }

  inline static auto resolve_expr(toc &tc, ostream &os, size_t indent,
                                  const expr_ops_list &exp,
                                  vector<string> &allocated_registers)
      -> string {
    if (exp.is_expression()) {
      const string &reg{tc.alloc_scratch_register(exp, os, indent)};
      allocated_registers.push_back(reg);
      exp.compile(tc, os, indent + 1, reg);
      return reg;
    }

    const ident_resolved &id_r{tc.resolve_identifier(exp, true)};
    if (id_r.is_const()) {
      return exp.get_unary_ops().as_string() + id_r.id_nasm;
    }

    if (exp.get_unary_ops().is_empty()) {
      return id_r.id_nasm;
    }

    const string &reg = tc.alloc_scratch_register(exp, os, indent);
    allocated_registers.push_back(reg);
    tc.asm_cmd(exp, os, indent, "mov", reg, id_r.id_nasm);
    exp.get_unary_ops().compile(tc, os, indent, reg);
    return reg;
  }

  vector<token> nots_{};
  expr_ops_list lhs_{};
  string op_{}; // '<', '<=', '>', '>=', '='
  expr_ops_list rhs_{};
  bool is_not_{};       // if not a=b
  bool is_shorthand_{}; // if a ...
  bool is_expression_{};
};
