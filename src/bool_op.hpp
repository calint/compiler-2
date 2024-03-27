#pragma once
#include "expr_ops_list.hpp"

class bool_op final : public statement {
  vector<token> nots_{};
  expr_ops_list lhs_{};
  string op_{}; // '<', '<=', '>', '>=', '==', '!='
  expr_ops_list rhs_{};
  bool is_not_{};       // e.g. if not a == b ...
  bool is_shorthand_{}; // e.g. if a ...
  bool is_expression_{};

public:
  inline bool_op(toc &tc, tokenizer &tz)
      : statement{tz.next_whitespace_token()} {

    set_type(tc.get_type_bool());

    bool is_not{false};
    // if not a == 3 ...
    while (true) {
      token tk{tz.next_token()};
      if (not tk.is_name("not")) {
        tz.put_back_token(tk);
        break;
      }
      is_not = not is_not;
      nots_.emplace_back(move(tk));
    }
    is_not_ = is_not;

    lhs_ = {tc, tz, true};

    if (tz.is_next_char('=')) {
      if (not tz.is_next_char('=')) {
        throw compiler_exception(tz, "expected '=='");
      }
      op_ = "==";
    } else if (tz.is_next_char('!')) {
      if (not tz.is_next_char('=')) {
        throw compiler_exception(tz, "expected '!='");
      }
      op_ = "!=";
    } else if (tz.is_next_char('<')) {
      if (tz.is_next_char('=')) {
        op_ = "<=";
      } else {
        op_ = "<";
      }
    } else if (tz.is_next_char('>')) {
      if (tz.is_next_char('=')) {
        op_ = ">=";
      } else {
        op_ = ">";
      }
    } else {
      // if a ...
      is_shorthand_ = true;
      resolve_if_op_is_expression();
      return;
    }
    rhs_ = {tc, tz};
    resolve_if_op_is_expression();
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
    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
  }

  inline auto compile_or(toc &tc, ostream &os, size_t indent,
                         const string &jmp_to_if_true,
                         const bool inverted) const -> optional<bool> {
    const bool invert{inverted ? not is_not_ : is_not_};
    toc::indent(os, indent, true);
    tc.comment_source(os, "?", inverted ? " 'or' inverted: " : " ", *this);
    toc::asm_label(tok(), os, indent, cmp_bgn_label(tc));
    if (is_shorthand_) {
      // check case when operand is constant
      if (not lhs_.is_expression()) {
        // left-hand-side is not a expression, either a constant or an
        // identifier
        const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
        if (lhs_resolved.is_const()) {
          bool const_eval{lhs_.get_unary_ops().evaluate_constant(
                              lhs_resolved.const_value) != 0};
          if (invert) {
            const_eval = not const_eval;
          }
          toc::indent(os, indent, true);
          os << "const eval to " << (const_eval ? "true" : "false") << endl;
          if (const_eval) {
            // since it is an 'or' chain short-circuit expression and jump to
            // label for true
            toc::indent(os, indent);
            os << "jmp " << jmp_to_if_true << endl;
          }
          return const_eval;
        }
      }
      resolve_cmp_shorthand(tc, os, indent, lhs_);
      toc::indent(os, indent);
      os << (not invert ? asm_jxx_for_op_inv("==") : asm_jxx_for_op("=="));
      os << " " << jmp_to_if_true << endl;
      return nullopt;
    }
    // check case when both operands are constants
    if (not lhs_.is_expression() and not rhs_.is_expression()) {
      const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
      const ident_resolved &rhs_resolved{tc.resolve_identifier(rhs_, false)};
      if (lhs_resolved.is_const() and rhs_resolved.is_const()) {
        bool const_eval{eval_constant(
            lhs_.get_unary_ops().evaluate_constant(lhs_resolved.const_value),
            op_,
            rhs_.get_unary_ops().evaluate_constant(rhs_resolved.const_value))};
        if (invert) {
          const_eval = not const_eval;
        }
        toc::indent(os, indent, true);
        os << "const eval to " << (const_eval ? "true" : "false") << endl;
        if (const_eval) {
          toc::asm_jmp(lhs_.tok(), os, indent, jmp_to_if_true);
        }
        return const_eval;
      }
    }
    // left-hand-side or right-hand-side or both are expressions
    //? todo. compilation error if lhs_ is constant
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
    tc.comment_source(os, "?", inverted ? " 'and' inverted: " : " ", *this);
    toc::asm_label(tok(), os, indent, cmp_bgn_label(tc));
    if (is_shorthand_) {
      // check case when operand is constant
      if (not lhs_.is_expression()) {
        const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
        if (lhs_resolved.is_const()) {
          bool const_eval{lhs_.get_unary_ops().evaluate_constant(
                              lhs_resolved.const_value) != 0};
          if (invert) {
            const_eval = not const_eval;
          }
          toc::indent(os, indent, true);
          os << "const eval to " << (const_eval ? "true" : "false") << endl;
          if (not const_eval) {
            // since it is an 'and' chain short-circuit expression and jump to
            // label for false
            toc::asm_jmp(lhs_.tok(), os, indent, jmp_to_if_false);
          }
          return const_eval;
        }
      }
      // left-hand-side is expression
      resolve_cmp_shorthand(tc, os, indent, lhs_);
      toc::indent(os, indent);
      os << (not invert ? asm_jxx_for_op("==") : asm_jxx_for_op_inv("=="));
      os << " " << jmp_to_if_false << endl;
      return nullopt;
    }
    // not shorthand expression
    // check the case when both operands are constants
    if (not lhs_.is_expression() and not rhs_.is_expression()) {
      const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
      const ident_resolved &rhs_resolved{tc.resolve_identifier(rhs_, false)};
      if (lhs_resolved.is_const() and rhs_resolved.is_const()) {
        bool const_eval{eval_constant(
            lhs_.get_unary_ops().evaluate_constant(lhs_resolved.const_value),
            op_,
            rhs_.get_unary_ops().evaluate_constant(rhs_resolved.const_value))};
        if (invert) {
          const_eval = not const_eval;
        }
        toc::indent(os, indent, true);
        os << "const eval to " << (const_eval ? "true" : "false") << endl;
        if (not const_eval) {
          // short circuit 'and' chain
          toc::asm_jmp(lhs_.tok(), os, indent, jmp_to_if_false);
        }
        return const_eval;
      }
    }

    // don't allow left-hand-side to be constant because generated assembler
    // does not compile
    if (not lhs_.is_expression()) {
      const ident_resolved &lhs_resolved{tc.resolve_identifier(lhs_, false)};
      if (lhs_resolved.is_const()) {
        throw compiler_exception(
            lhs_.tok(), "left hand side expression may not be a constant");
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
    return "cmp_" + tc.source_location_for_use_in_label(tok()) +
           (call_path.empty() ? "" : "_" + call_path);
  }

  [[nodiscard]] inline auto identifier() const -> const string & override {
    assert(not is_expression_);
    return lhs_.identifier();
  }

  [[nodiscard]] inline auto is_expression() const -> bool override {
    return is_expression_;
  }

private:
  inline void resolve_if_op_is_expression() {
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

    // if not expression then it is a single statement
    //   and identifier is valid
    const string &id{lhs_.identifier()};
    if (id == "true" or id == "false") {
      is_expression_ = false;
      return;
    }

    is_expression_ = true;
  }

  inline static auto eval_constant(const long lh, const string &op,
                                   const long rh) -> bool {
    if (op == "==") {
      return lh == rh;
    }
    if (op == "!=") {
      return lh != rh;
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
    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
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

    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
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
    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
  }

  inline void resolve_cmp(toc &tc, ostream &os, size_t indent,
                          const expr_ops_list &lhs,
                          const expr_ops_list &rhs) const {

    vector<string> allocated_registers{};

    const string &dst{resolve_expr(tc, os, indent, lhs, allocated_registers)};
    const string &src{resolve_expr(tc, os, indent, rhs, allocated_registers)};

    tc.asm_cmd(tok(), os, indent, "cmp", dst, src);

    // free allocated registers in reverse order
    for (auto it{allocated_registers.rbegin()};
         it != allocated_registers.rend(); ++it) {
      const string &reg{*it};
      tc.free_scratch_register(os, indent, reg);
    }
  }

  inline void resolve_cmp_shorthand(toc &tc, ostream &os, size_t indent,
                                    const expr_ops_list &lhs) const {
    vector<string> allocated_registers{};

    const string &dst{resolve_expr(tc, os, indent, lhs, allocated_registers)};

    tc.asm_cmd(tok(), os, indent, "cmp", dst, "0");

    // free allocated registers in reverse order
    for (auto it{allocated_registers.rbegin()};
         it != allocated_registers.rend(); ++it) {
      const string &reg{*it};
      tc.free_scratch_register(os, indent, reg);
    }
  }

  inline static auto resolve_expr(toc &tc, ostream &os, size_t indent,
                                  const expr_ops_list &expr,
                                  vector<string> &allocated_registers)
      -> string {

    if (expr.is_expression()) {
      const string &reg{tc.alloc_scratch_register(expr.tok(), os, indent)};
      allocated_registers.emplace_back(reg);
      expr.compile(tc, os, indent + 1, reg);
      return reg;
    }
    // expr is not an expression
    const ident_resolved &expr_resolved{tc.resolve_identifier(expr, true)};
    if (expr_resolved.is_const()) {
      return expr.get_unary_ops().to_string() + expr_resolved.id_nasm;
    }
    // expr not a constant, it is an identifier
    if (expr.get_unary_ops().is_empty()) {
      return expr_resolved.id_nasm;
    }
    // expr is not an expression and has unary ops
    const string &reg{tc.alloc_scratch_register(expr.tok(), os, indent)};
    allocated_registers.emplace_back(reg);
    tc.asm_cmd(expr.tok(), os, indent, "mov", reg, expr_resolved.id_nasm);
    expr.get_unary_ops().compile(tc, os, indent, reg);
    return reg;
  }
};
