#pragma once
#include "bool_op.hpp"
#include "statement.hpp"

class bool_ops_list final : public statement {
public:
  inline bool_ops_list(toc &tc, tokenizer &t, const bool enclosed = false,
                       token not_token = {}, const bool is_sub_expr = false,
                       variant<bool_op, bool_ops_list> first_bool_op = {},
                       token first_op = {})
      : // the token is used to give the cmp a unique label
        //   if first_bool_op is provided then use that token
        //   else the next white space token
        statement{first_op.is_name("") ? t.next_whitespace_token()
                                       : token_from(first_bool_op)},
        not_token_{move(not_token)}, enclosed_{enclosed} {
    set_type(tc.get_type_bool());

    token prv_op{first_op};
    if (not first_op.is_empty()) {
      // sub-expression with first bool op provided
      bools_.push_back(move(first_bool_op));
      ops_.push_back(move(first_op));
    }
    while (true) {
      token tknot{t.next_token()};
      if (tknot.is_name("not")) {
        // not (a=1 and b=1)
        // not a=1 and b=1
        if (t.is_next_char('(')) {
          // not (a=1 and b=1)
          bools_.emplace_back(bool_ops_list{tc, t, true, move(tknot)});
        } else {
          // not a=1 and b=1
          t.put_back_token(tknot);
          bools_.emplace_back(bool_op{tc, t});
        }
      } else {
        // (a=1 and b=1)
        // a=1 and b=1
        t.put_back_token(tknot);
        if (t.is_next_char('(')) {
          // (a=1 and b=1)
          bools_.emplace_back(bool_ops_list{tc, t, true});
        } else {
          // a=1 and b=1
          bools_.emplace_back(bool_op{tc, t});
        }
      }

      if (enclosed_ and t.is_next_char(')')) {
        return;
      }

      token tk{t.next_token()};
      if (not(tk.is_name("or") or tk.is_name("and"))) {
        t.put_back_token(tk);
        break;
      }

      // if first op and is 'and' then create sub-expression
      if (prv_op.is_empty()) {
        prv_op = tk;
        if (tk.is_name("and")) {
          // a and b or c -> (a and b) or c
          // first op is 'and', make sub-expression (a and b) ...
          bool_ops_list bol{
              tc, t, false, {}, true, move(bools_.back()), move(tk)};
          bools_.pop_back();
          bools_.emplace_back(move(bol));

          if (enclosed_ and t.is_next_char(')')) {
            return;
          }

          token tk2 = t.next_token();
          if (not(tk2.is_name("or") or tk2.is_name("and"))) {
            t.put_back_token(tk2);
            break;
          }

          prv_op = tk2;
          ops_.push_back(move(tk2));
          continue;
        }
      }

      // if same op as previous continue
      if (tk.is_name(prv_op.name())) {
        ops_.push_back(move(tk));
        continue;
      }

      // previous op is not the same as next op
      //   either a new sub-expression or exit current sub-expression
      // a or b  |or|       |c|       |and|  d or e
      //       |prv_tk| |ops.back()|  |tk|
      if (is_sub_expr) {
        // generated sub-expressions are 'and' ops and this is an 'or'
        // a or b and c |or| d
        //      ------- |tk|
        t.put_back_token(tk);
        return;
      }

      // this is an 'and' op after a 'or'
      // a    |or|    |b|     |and|   c or d
      //    |prv_op| |back()| |tk|
      // create:
      // a    or   (b     and   c) or d
      bool_ops_list bol{tc, t, false, {}, true, move(bools_.back()), tk};
      bools_.pop_back();
      bools_.emplace_back(move(bol));

      if (enclosed_ and t.is_next_char(')')) {
        return;
      }

      prv_op = tk;
      tk = t.next_token();
      if (not(tk.is_name("or") or tk.is_name("and"))) {
        t.put_back_token(tk);
        break;
      }

      ops_.push_back(move(tk));
    }
    if (enclosed_) {
      throw compiler_exception(tok(), "expected ')' to close expression");
    }
  }

  inline bool_ops_list() = default;
  inline bool_ops_list(const bool_ops_list &) = default;
  inline bool_ops_list(bool_ops_list &&) = default;
  inline auto operator=(const bool_ops_list &) -> bool_ops_list & = default;
  inline auto operator=(bool_ops_list &&) -> bool_ops_list & = default;

  inline ~bool_ops_list() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    not_token_.source_to(os);
    if (enclosed_) {
      os << "(";
    }
    const size_t n{bools_.size()};
    for (size_t i{0}; i < n; i++) {
      if (bools_[i].index() == 0) {
        get<bool_op>(bools_[i]).source_to(os);
      } else {
        get<bool_ops_list>(bools_[i]).source_to(os);
      }
      if (i < n - 1) {
        const token &t{ops_[i]};
        t.source_to(os);
      }
    }
    if (enclosed_) {
      os << ")";
    }
  }
  inline void compile([[maybe_unused]] toc &tc, [[maybe_unused]] ostream &os,
                      [[maybe_unused]] size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    throw compiler_exception(
        tok(), "this code should not be reached: " + string{__FILE__} + ":" +
                   to_string(__LINE__));
  }

  [[nodiscard]] inline auto cmp_bgn_label(const toc &tc) const -> string {
    const string &call_path{tc.get_inline_call_path(tok())};
    return "cmp_" + tc.source_location_for_label(tok()) +
           (call_path.empty() ? "" : "_" + call_path);
  }

  inline auto compile(toc &tc, ostream &os, const size_t indent,
                      const string &jmp_to_if_false,
                      const string &jmp_to_if_true, const bool inverted) const
      -> optional<bool> {
    toc::indent(os, indent, true);
    tc.source_comment(os, "?", inverted ? " inverted: " : " ", *this);
    // invert according to De Morgan's laws
    const bool invert{inverted ? not not_token_.is_name("not")
                               : not_token_.is_name("not")};
    const size_t n{bools_.size()};
    for (size_t i = 0; i < n; i++) {
      if (bools_[i].index() == 1) {
        //
        // stmt_if_bool_ops_list
        //
        const bool_ops_list &el{get<bool_ops_list>(bools_[i])};
        string jmp_false{jmp_to_if_false};
        string jmp_true{jmp_to_if_true};
        if (i < n - 1) {
          if (!invert) {
            // if not last element check if it is a 'or' or 'and' list
            if (ops_[i].is_name("or")) {
              // if evaluation is false and next op is "or" then jump_false is
              // next bool eval
              jmp_false = cmp_label_from(tc, bools_[i + 1]);
            } else if (ops_[i].is_name("and")) {
              // if evaluation is true and next op is "and" then jump_true is
              // next bool eval
              jmp_true = cmp_label_from(tc, bools_[i + 1]);
            } else {
              throw panic_exception("expected 'or' or 'and'");
            }
            optional<bool> const_eval{
                el.compile(tc, os, indent, jmp_false, jmp_true, invert)};
            if (const_eval) {
              return *const_eval;
            }
          } else {
            // invert according to De Morgan's laws
            // if not last element check if it is a 'or' or 'and' list
            if (ops_[i].is_name("and")) {
              // if evaluation is false and next op is "or" (inverted) then
              // jump_false is next bool eval
              jmp_false = cmp_label_from(tc, bools_[i + 1]);
            } else if (ops_[i].is_name("or")) {
              // if evaluation is true and next op is "and" (inverted) then
              // jump_true is next bool eval
              jmp_true = cmp_label_from(tc, bools_[i + 1]);
            } else {
              throw panic_exception("expected 'or' or 'and'");
            }
            optional<bool> const_eval{
                el.compile(tc, os, indent, jmp_false, jmp_true, invert)};
            if (const_eval) {
              return *const_eval;
            }
          }
        } else {
          // if last in list jmp_false is next bool eval
          optional<bool> const_eval{
              el.compile(tc, os, indent, jmp_false, jmp_true, invert)};
          if (const_eval) {
            return *const_eval;
          }
        }
        continue;
      }

      //
      // stmt_if_bool_op
      //
      if (!invert) {
        // a=1 and b=2   vs   a=1 or b=2
        const bool_op &e{get<bool_op>(bools_[i])};
        if (i < n - 1) {
          if (ops_[i].is_name("or")) {
            optional<bool> const_eval{
                e.compile_or(tc, os, indent, jmp_to_if_true, invert)};
            if (const_eval and *const_eval) {
              // constant evaluated to true, short-circuit
              return true;
            }
          } else if (ops_[i].is_name("and")) {
            optional<bool> const_eval{
                e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
            if (const_eval and not *const_eval) {
              // constant evaluated to false, short-circuit
              return *const_eval;
            }
          } else {
            throw panic_exception("expected 'or' or 'and'");
          }
        } else {
          optional<bool> const_eval{
              e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
          if (const_eval) { // constant evaluated
            return *const_eval;
          }
          // if last element and not yet jumped to false then jump to true
          toc::asm_jmp(*this, os, indent, jmp_to_if_true);
        }
      } else {
        // inverted according to De Morgan's laws
        // a=1 and b=2   vs   a=1 or b=2
        const bool_op &e{get<bool_op>(bools_[i])};
        if (i < n - 1) {
          if (ops_[i].is_name("and")) {
            optional<bool> const_eval{
                e.compile_or(tc, os, indent, jmp_to_if_true, invert)};
            if (const_eval and *const_eval) {
              // constant evaluated to true, short-circuit
              return true;
            }
          } else if (ops_[i].is_name("or")) {
            optional<bool> const_eval{
                e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
            if (const_eval and not *const_eval) {
              // constant evaluated to false, short-circuit
              return false;
            }
          } else {
            throw panic_exception("expected 'or' or 'and'");
          }
        } else {
          optional<bool> const_eval{
              e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
          if (const_eval) { // constant evaluated
            return *const_eval;
          }
          // if last element and not yet jumped to false then jump to true
          toc::asm_jmp(*this, os, indent, jmp_to_if_true);
        }
      }
    }
    return nullopt;
  }

  [[nodiscard]] inline auto is_expression() const
      -> bool override { // ? assumes it is not an expression
    if (bools_.size() > 1) {
      return true;
    }

    assert(!bools_.empty());

    if (bools_[0].index() == 0) {
      return get<bool_op>(bools_[0]).is_expression();
    }

    return get<bool_ops_list>(bools_[0]).is_expression();
  }

  [[nodiscard]] inline auto identifier() const -> const string & override {
    if (bools_.size() > 1) {
      throw panic_exception("unexpected code path " + string{__FILE__} +
                                 ":" + to_string(__LINE__));
    }

    assert(!bools_.empty());

    if (bools_[0].index() == 0) {
      return get<bool_op>(bools_[0]).identifier();
    }

    return get<bool_ops_list>(bools_[0]).identifier();
  }

private:
  inline static auto cmp_label_from(const toc &tc,
                                    const variant<bool_op, bool_ops_list> &v)
      -> string {
    if (v.index() == 1) {
      return get<bool_ops_list>(v).cmp_bgn_label(tc);
    }

    return get<bool_op>(v).cmp_bgn_label(tc);
  }

  inline static auto token_from(const variant<bool_op, bool_ops_list> &bop)
      -> token {
    if (bop.index() == 0) {
      return get<bool_op>(bop).tok();
    }

    return get<bool_ops_list>(bop).tok();
  }

  vector<variant<bool_op, bool_ops_list>> bools_{};
  vector<token> ops_{}; // 'and' or 'or'
  token not_token_{};
  bool enclosed_{}; // (a=b and c=d) vs a=b and c=d
};
