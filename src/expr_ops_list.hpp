#pragma once
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "toc.hpp"

class expr_ops_list final : public expression {
public:
  inline expr_ops_list(
      toc &tc, tokenizer &tz, const bool in_args = false,
      const bool enclosed = false, const unary_ops &uops = {},
      const char first_op_precedence = initial_precedence,
      unique_ptr<statement> first_expression = unique_ptr<statement>())
      : expression{{}, tz.next_whitespace_token()}, enclosed_{enclosed},
        uops_{uops} {

    // read first expression i.e. =-a/-(b+1)
    if (first_expression) {
      // called in a recursion with first expression provided
      exps_.push_back(move(first_expression));
    } else {
      // check if new recursion is necessary e.g. =-a/-(-(b+c)+d), t at "-a/-("
      const unary_ops uo{tz};
      if (tz.is_next_char('(')) {
        // recursion
        exps_.emplace_back(
            make_unique<expr_ops_list>(tc, tz, in_args, true, uo));
      } else {
        // statement
        uo.put_back(tz);
        exps_.emplace_back(create_statement_from_tokenizer(tc, tz));
      }
    }

    char precedence{first_op_precedence};
    while (true) { // +a  +3
      // if end of subexpression
      if (enclosed_ and tz.is_next_char(')')) {
        break;
      }

      // if parsed in a function call argument list or in a boolean expression
      if (in_args) { //? rewrite is_in_bool_expr
        // if in boolean expression exit when an operation is found
        if (tz.is_peek_char('<') and not tz.is_peek_char2('<')) {
          break;
        }
        if (tz.is_peek_char('=')) {
          break;
        }
        if (tz.is_peek_char('>') and not tz.is_peek_char2('>')) {
          break;
        }
        // if in arguments exit when ',' or ')' is found
        if (tz.is_peek_char(',')) {
          break;
        }
        if (tz.is_peek_char(')')) {
          break;
        }
      }

      // next operation
      if (tz.is_peek_char('+')) {
        ops_.push_back('+');
      } else if (tz.is_peek_char('-')) {
        ops_.push_back('-');
      } else if (tz.is_peek_char('*')) {
        ops_.push_back('*');
      } else if (tz.is_peek_char('/')) {
        ops_.push_back('/');
      } else if (tz.is_peek_char('%')) {
        ops_.push_back('%');
      } else if (tz.is_peek_char('&')) {
        ops_.push_back('&');
      } else if (tz.is_peek_char('|')) {
        ops_.push_back('|');
      } else if (tz.is_peek_char('^')) {
        ops_.push_back('^');
      } else if (tz.is_peek_char('<') and tz.is_peek_char2('<')) {
        ops_.push_back('<');
      } else if (tz.is_peek_char('>') and tz.is_peek_char2('>')) {
        ops_.push_back('>');
      } else {
        // no more operations
        break;
      }

      // check if next operation precedence is same or lower
      // if not then a new subexpression is added to the list with the last
      // expression in this list as first expression
      const char next_precedence{precedence_for_op(ops_.back())};
      if (next_precedence > precedence) {
        // i.e. =a+b*c+1 where the peeked char is '*'
        // next operation has higher precedence than current
        // list is now =[(=a)(+b)]
        // move last expression (+b) to subexpression
        //   =[(=a) +[(=b)(*c)(+1)]]
        precedence = next_precedence;
        const char first_op_prec{precedence_for_op(ops_.back())};
        ops_.pop_back();
        unique_ptr<statement> prev{move(exps_.back())};
        exps_.pop_back();
        exps_.emplace_back(make_unique<expr_ops_list>(
            tc, tz, in_args, false, unary_ops{}, first_op_prec, move(prev)));
        continue;
      }

      precedence = next_precedence;
      const char ch{tz.next_char()}; // read the peeked operator
      if (ch == '<' or ch == '>') {
        tz.next_char(); // one more character for << and >>
      }

      // check if next statement is a subexpression
      const unary_ops uo{tz};
      if (tz.is_next_char('(')) {
        // subexpression, recurse
        exps_.emplace_back(
            make_unique<expr_ops_list>(tc, tz, in_args, true, uo));
        continue;
      }

      // not subexpression
      uo.put_back(tz);

      // read statement
      unique_ptr<statement> st{create_statement_from_tokenizer(tc, tz)};
      if (st->tok().is_empty()) {
        throw compiler_exception(st->tok(),
                                 "unexpected '" + string{tz.peek_char()} + "'");
      }
      // move statement to list
      exps_.push_back(move(st));
    }
  }

  inline expr_ops_list() = default;
  inline expr_ops_list(const expr_ops_list &) = default;
  inline expr_ops_list(expr_ops_list &&) = default;
  inline auto operator=(const expr_ops_list &) -> expr_ops_list & = default;
  inline auto operator=(expr_ops_list &&) -> expr_ops_list & = default;

  inline ~expr_ops_list() override = default;

  inline void source_to(ostream &os) const override {
    expression::source_to(os); // whitespace
    uops_.source_to(os);

    if (enclosed_) {
      os << "(";
    }

    exps_[0]->source_to(os);
    const size_t n{ops_.size()};
    for (size_t i{0}; i < n; i++) {
      const char op{ops_[i]};
      os << op;
      if (op == '<' or op == '>') {
        // << or >>
        os << op;
      }
      exps_[i + 1]->source_to(os);
    }

    if (enclosed_) {
      os << ")";
    }
  }

  inline void compile(toc &tc, ostream &os, const size_t indent,
                      const string &dst) const override {
    tc.source_comment(*this, os, indent);

    // first element is assigned to destination
    const ident_resolved &dst_resolved{
        tc.resolve_identifier(*this, dst, false)};
    asm_op(tc, os, indent, '=', dst_resolved, *exps_[0]);

    // remaining elements are +,-,*,/,%,|,&,^
    const size_t n{ops_.size()};
    for (size_t i{0}; i < n; i++) {
      asm_op(tc, os, indent, ops_[i], dst_resolved, *exps_[i + 1]);
    }

    // apply unary expressions on destination
    uops_.compile(tc, os, indent, dst_resolved.id_nasm);
  }

  [[nodiscard]] inline auto is_expression() const -> bool override {
    if (not uops_.is_empty()) {
      return true;
    }

    if (exps_.size() == 1) {
      return exps_[0]->is_expression();
    }

    return true;
  }

  [[nodiscard]] inline auto identifier() const -> const string & override {
    assert(exps_.size() == 1);

    return exps_[0]->identifier();
  }

  [[nodiscard]] inline auto get_unary_ops() const
      -> const unary_ops & override {
    if (exps_.size() == 1) {
      return exps_[0]->get_unary_ops();
    }

    return uops_;
  }

  [[nodiscard]] inline auto get_type() const -> const type & override {
    assert(not exps_.empty());
    //? hack  find the size of the largest integral element
    return exps_[0]->get_type();
  }

private:
  // higher than the highest precedence
  static constexpr char initial_precedence{7};

  inline static auto precedence_for_op(const char ch) -> char {
    switch (ch) {
    case '|':
      return 1;
    case '^':
      return 2;
    case '&':
      return 3;
    case '<': // shift left (note. consider having different precedence than C)
    case '>': // shift right(      with << and >> being atleast * / %         )
      return 4;
    case '+':
    case '-':
      return 5;
    case '*':
    case '/':
    case '%':
      return 6;
    default:
      throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                            to_string(__LINE__));
    }
  }

  inline static void asm_op(toc &tc, ostream &os, const size_t indent,
                            const char op, const ident_resolved &dst,
                            const statement &src) {
    toc::indent(os, indent, true);
    tc.source_comment(os, dst.id, {op}, src);
    if (op == '=') {
      asm_op_mov(tc, os, indent, dst, src);
      return;
    }
    if (op == '+') {
      asm_op_add_sub(tc, os, indent, "add", "sub", dst, src);
      return;
    }
    if (op == '-') {
      asm_op_add_sub(tc, os, indent, "sub", "add", dst, src);
      return;
    }
    if (op == '*') {
      asm_op_mul(tc, os, indent, dst, src);
      return;
    }
    if (op == '/') {
      asm_op_div(tc, os, indent, "rax", dst, src);
      return;
    }
    if (op == '%') {
      asm_op_div(tc, os, indent, "rdx", dst, src);
      return;
    }
    if (op == '&') {
      asm_op_bitwise(tc, os, indent, "and", dst, src);
      return;
    }
    if (op == '|') {
      asm_op_bitwise(tc, os, indent, "or", dst, src);
      return;
    }
    if (op == '^') {
      asm_op_bitwise(tc, os, indent, "xor", dst, src);
      return;
    }
    if (op == '<') {
      asm_op_shift(tc, os, indent, "sal", dst, src);
      return;
    }
    if (op == '>') {
      asm_op_shift(tc, os, indent, "sar", dst, src);
      return;
    }
  }

  inline static void asm_op_mov(toc &tc, ostream &os, const size_t indent,
                                const ident_resolved &dst,
                                const statement &src) {
    if (src.is_expression()) {
      src.compile(tc, os, indent, dst.id);
      return;
    }
    const ident_resolved &src_resolved{tc.resolve_identifier(src, true)};
    if (src_resolved.is_const()) {
      tc.asm_cmd(src, os, indent, "mov", dst.id_nasm,
                 src.get_unary_ops().to_string() + src_resolved.id_nasm);
      return;
    }
    tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, src_resolved.id_nasm);
    src.get_unary_ops().compile(tc, os, indent, dst.id_nasm);
  }

  inline static void asm_op_mul(toc &tc, ostream &os, const size_t indent,
                                const ident_resolved &dst,
                                const statement &src) {
    if (src.is_expression()) {
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      src.compile(tc, os, indent, reg);
      // imul destination must be a register
      if (dst.is_register()) {
        tc.asm_cmd(src, os, indent, "imul", dst.id_nasm, reg);
      } else {
        // imul destination is not a register
        tc.asm_cmd(src, os, indent, "imul", reg, dst.id_nasm);
        tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, reg);
      }
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    // not an expression, either a register or memory location
    const ident_resolved &src_resolved{tc.resolve_identifier(src, true)};
    // imul destination operand must be register
    if (dst.is_register()) {
      if (src_resolved.is_const()) {
        tc.asm_cmd(src, os, indent, "imul", dst.id_nasm,
                   src.get_unary_ops().to_string() + src_resolved.id_nasm);
        return;
      }
      const unary_ops &uops{src.get_unary_ops()};
      if (uops.is_empty()) {
        tc.asm_cmd(src, os, indent, "imul", dst.id_nasm, src_resolved.id_nasm);
        return;
      }
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      tc.asm_cmd(src, os, indent, "mov", reg, src_resolved.id_nasm);
      uops.compile(tc, os, indent, reg);
      tc.asm_cmd(src, os, indent, "imul", dst.id_nasm, reg);
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    // imul destination is not a register
    if (src_resolved.is_const()) {
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      tc.asm_cmd(src, os, indent, "mov", reg, dst.id_nasm);
      tc.asm_cmd(src, os, indent, "imul", reg,
                 src.get_unary_ops().to_string() + src_resolved.id_nasm);
      tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, reg);
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    const unary_ops &uops{src.get_unary_ops()};
    if (uops.is_empty()) {
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      tc.asm_cmd(src, os, indent, "mov", reg, dst.id_nasm);
      tc.asm_cmd(src, os, indent, "imul", reg, src_resolved.id_nasm);
      tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, reg);
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    const string &reg{tc.alloc_scratch_register(src, os, indent)};
    tc.asm_cmd(src, os, indent, "mov", reg, src_resolved.id_nasm);
    uops.compile(tc, os, indent, reg);
    tc.asm_cmd(src, os, indent, "imul", reg, dst.id_nasm);
    tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, reg);
    tc.free_scratch_register(os, indent, reg);
  }

  inline static void asm_op_add_sub(toc &tc, ostream &os, const size_t indent,
                                    const string &op, const string &op_inv,
                                    const ident_resolved &dst,
                                    const statement &src) {
    if (src.is_expression()) {
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      src.compile(tc, os, indent, reg);
      tc.asm_cmd(src, os, indent, op, dst.id_nasm, reg);
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    const ident_resolved &src_resolved{tc.resolve_identifier(src, true)};
    if (src_resolved.is_const()) {
      tc.asm_cmd(src, os, indent, op, dst.id_nasm,
                 src.get_unary_ops().to_string() + src_resolved.id_nasm);
      return;
    }
    const unary_ops &uops{src.get_unary_ops()};
    if (uops.is_empty()) {
      tc.asm_cmd(src, os, indent, op, dst.id_nasm, src_resolved.id_nasm);
      return;
    }
    if (uops.is_only_negated()) {
      tc.asm_cmd(src, os, indent, op_inv, dst.id_nasm, src_resolved.id_nasm);
      return;
    }
    const string &reg{tc.alloc_scratch_register(src, os, indent)};
    tc.asm_cmd(src, os, indent, "mov", reg, src_resolved.id_nasm);
    uops.compile(tc, os, indent, reg);
    tc.asm_cmd(src, os, indent, op, dst.id_nasm, reg);
    tc.free_scratch_register(os, indent, reg);
  }

  inline static void asm_op_bitwise(toc &tc, ostream &os, const size_t indent,
                                    const string &op, const ident_resolved &dst,
                                    const statement &src) {
    if (src.is_expression()) {
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      src.compile(tc, os, indent, reg);
      tc.asm_cmd(src, os, indent, op, dst.id_nasm, reg);
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    const ident_resolved &src_resolved{tc.resolve_identifier(src, true)};
    if (src_resolved.is_const()) {
      tc.asm_cmd(src, os, indent, op, dst.id_nasm,
                 src.get_unary_ops().to_string() + src_resolved.id_nasm);
      return;
    }
    const unary_ops &uops{src.get_unary_ops()};
    if (uops.is_empty()) {
      tc.asm_cmd(src, os, indent, op, dst.id_nasm, src_resolved.id_nasm);
      return;
    }
    const string &reg{tc.alloc_scratch_register(src, os, indent)};
    tc.asm_cmd(src, os, indent, "mov", reg, src_resolved.id_nasm);
    uops.compile(tc, os, indent, reg);
    tc.asm_cmd(src, os, indent, op, dst.id_nasm, reg);
    tc.free_scratch_register(os, indent, reg);
  }

  inline static void asm_op_shift(toc &tc, ostream &os, const size_t indent,
                                  const string &op, const ident_resolved &dst,
                                  const statement &src) {
    if (src.is_expression()) {
      // the operand must be stored in CL
      const bool rcx_allocated{tc.alloc_named_register(src, os, indent, "rcx")};
      if (not rcx_allocated) {
        toc::asm_push(src, os, indent, "rcx");
      }
      src.compile(tc, os, indent, "rcx");
      tc.asm_cmd(src, os, indent, op, dst.id_nasm, "cl");
      if (rcx_allocated) {
        tc.free_named_register(os, indent, "rcx");
      } else {
        toc::asm_pop(src, os, indent, "rcx");
      }
      return;
    }
    const ident_resolved &src_resolved{tc.resolve_identifier(src, true)};
    if (src_resolved.is_const()) {
      tc.asm_cmd(src, os, indent, op, dst.id_nasm,
                 src.get_unary_ops().to_string() + src_resolved.id_nasm);
      return;
    }
    if (src_resolved.id_nasm == "rcx") {
      throw compiler_exception(src.tok(),
                               "cannot use 'rcx' as operand in shift because "
                               "that registers is used");
    }

    const unary_ops &uops{src.get_unary_ops()};
    if (uops.is_empty()) {
      // the operand must be stored in CL
      const bool rcx_allocated{tc.alloc_named_register(src, os, indent, "rcx")};
      if (not rcx_allocated) {
        toc::asm_push(src, os, indent, "rcx");
      }
      tc.asm_cmd(src, os, indent, "mov", "rcx", src_resolved.id_nasm);
      tc.asm_cmd(src, os, indent, op, dst.id_nasm, "cl");
      if (rcx_allocated) {
        tc.free_named_register(os, indent, "rcx");
      } else {
        toc::asm_pop(src, os, indent, "rcx");
      }
      return;
    }
    const bool rcx_allocated{tc.alloc_named_register(src, os, indent, "rcx")};
    if (not rcx_allocated) {
      toc::asm_push(src, os, indent, "rcx");
    }
    tc.asm_cmd(src, os, indent, "mov", "rcx", src_resolved.id_nasm);
    uops.compile(tc, os, indent, "rcx");
    tc.asm_cmd(src, os, indent, op, dst.id_nasm, "cl");
    if (rcx_allocated) {
      tc.free_named_register(os, indent, "rcx");
    } else {
      toc::asm_pop(src, os, indent, "rcx");
    }
  }

  // op is either "rax" for the quotient or "rdx" for the reminder
  inline static void asm_op_div(toc &tc, ostream &os, const size_t indent,
                                const string &op, const ident_resolved &dst,
                                const statement &src) {
    if (src.is_expression()) {
      const string &reg{tc.alloc_scratch_register(src, os, indent)};
      src.compile(tc, os, indent, reg);
      const bool rax_allocated{tc.alloc_named_register(src, os, indent, "rax")};
      if (not rax_allocated) {
        toc::asm_push(src, os, indent, "rax");
      }
      tc.asm_cmd(src, os, indent, "mov", "rax", dst.id_nasm);
      const bool rdx_allocated{tc.alloc_named_register(src, os, indent, "rdx")};
      if (not rdx_allocated) {
        toc::asm_push(src, os, indent, "rdx");
      }
      toc::indent(os, indent, false);
      os << "cqo" << endl;
      toc::indent(os, indent, false);
      os << "idiv " << reg << endl;
      tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, op);
      if (rdx_allocated) {
        tc.free_named_register(os, indent, "rdx");
      } else {
        toc::asm_pop(src, os, indent, "rdx");
      }
      if (rax_allocated) {
        tc.free_named_register(os, indent, "rax");
      } else {
        toc::asm_pop(src, os, indent, "rax");
      }
      tc.free_scratch_register(os, indent, reg);
      return;
    }
    const ident_resolved &src_resolved{tc.resolve_identifier(src, true)};
    if (src_resolved.is_const()) {
      const bool rax_allocated{tc.alloc_named_register(src, os, indent, "rax")};
      if (not rax_allocated) {
        toc::asm_push(src, os, indent, "rax");
      }
      tc.asm_cmd(src, os, indent, "mov", "rax", dst.id_nasm);
      const bool rdx_allocated{tc.alloc_named_register(src, os, indent, "rdx")};
      if (not rdx_allocated) {
        toc::asm_push(src, os, indent, "rdx");
      }
      toc::indent(os, indent, false);
      os << "cqo" << endl;
      const string &r{tc.alloc_scratch_register(src, os, indent)};
      tc.asm_cmd(src, os, indent, "mov", r,
                 src.get_unary_ops().to_string() + src_resolved.id_nasm);
      toc::indent(os, indent, false);
      os << "idiv " << r << endl;
      tc.free_scratch_register(os, indent, r);
      tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, op);
      if (rdx_allocated) {
        tc.free_named_register(os, indent, "rdx");
      } else {
        toc::asm_pop(src, os, indent, "rdx");
      }
      if (rax_allocated) {
        tc.free_named_register(os, indent, "rax");
      } else {
        toc::asm_pop(src, os, indent, "rax");
      }
      return;
    }
    if (src_resolved.id_nasm == "rdx" or src_resolved.id_nasm == "rax") {
      throw compiler_exception(src.tok(),
                               "cannot use 'rdx' or 'rax' as operands in "
                               "division because those registers are used");
    }

    const unary_ops &uops{src.get_unary_ops()};
    if (uops.is_empty()) {
      const bool rax_allocated{tc.alloc_named_register(src, os, indent, "rax")};
      if (not rax_allocated) {
        toc::asm_push(src, os, indent, "rax");
      }
      tc.asm_cmd(src, os, indent, "mov", "rax", dst.id_nasm);
      const bool rdx_allocated{tc.alloc_named_register(src, os, indent, "rdx")};
      if (not rdx_allocated) {
        toc::asm_push(src, os, indent, "rdx");
      }
      toc::indent(os, indent, false);
      os << "cqo" << endl;
      toc::indent(os, indent, false);
      os << "idiv " << src_resolved.id_nasm << endl;
      // op is either "rax" for the quotient or "rdx" for the reminder
      tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, op);
      if (rdx_allocated) {
        tc.free_named_register(os, indent, "rdx");
      } else {
        toc::asm_pop(src, os, indent, "rdx");
      }
      if (rax_allocated) {
        tc.free_named_register(os, indent, "rax");
      } else {
        toc::asm_pop(src, os, indent, "rax");
      }
      return;
    }
    const string &reg{tc.alloc_scratch_register(src, os, indent)};
    tc.asm_cmd(src, os, indent, "mov", reg, src_resolved.id_nasm);
    uops.compile(tc, os, indent, reg);
    const bool rax_allocated{tc.alloc_named_register(src, os, indent, "rax")};
    if (not rax_allocated) {
      toc::asm_push(src, os, indent, "rax");
    }
    tc.asm_cmd(src, os, indent, "mov", "rax", dst.id_nasm);
    const bool rdx_allocated{tc.alloc_named_register(src, os, indent, "rdx")};
    if (not rdx_allocated) {
      toc::asm_push(src, os, indent, "rdx");
    }
    toc::indent(os, indent, false);
    os << "cqo" << endl;
    toc::indent(os, indent, false);
    os << "idiv " << reg << endl;
    tc.asm_cmd(src, os, indent, "mov", dst.id_nasm, op);
    if (rdx_allocated) {
      tc.free_named_register(os, indent, "rdx");
    } else {
      toc::asm_pop(src, os, indent, "rdx");
    }
    if (rax_allocated) {
      tc.free_named_register(os, indent, "rax");
    } else {
      toc::asm_pop(src, os, indent, "rax");
    }
    tc.free_scratch_register(os, indent, reg);
  }

  bool enclosed_{}; //  (a+b) vs a+b
  vector<unique_ptr<statement>> exps_{};
  vector<char> ops_{};
  unary_ops uops_{};
};
