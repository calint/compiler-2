#pragma once

#include <variant>

#include "bool_op.hpp"
#include "statement.hpp"

class bool_ops_list final : public statement {
    std::vector<std::variant<bool_op, bool_ops_list>> bools_{};
    std::vector<token> ops_{}; // 'and' or 'or'
    token not_token_{};        // not (a==b and c==d)
    bool enclosed_{};          // (a==b and c==d) vs a==b and c==d

  public:
    inline bool_ops_list(
        toc& tc, tokenizer& tz, const bool enclosed = false,
        token not_token = {}, const bool is_sub_expr = false,
        std::variant<bool_op, bool_ops_list> first_bool_op = {},
        token first_op = {})
        : // the token is used to give 'cmp' a unique label
          //   if first_bool_op is provided then use that token
          //   else the next white space token
          statement{first_op.is_name("") ? tz.next_whitespace_token()
                                         : token_from(first_bool_op)},
          not_token_{std::move(not_token)}, enclosed_{enclosed} {

        set_type(tc.get_type_bool());

        token prv_op{first_op};
        if (not first_op.is_empty()) {
            // sub-expression with first bool op provided
            bools_.emplace_back(std::move(first_bool_op));
            ops_.emplace_back(std::move(first_op));
        }
        while (true) {
            const token not_tk{tz.next_token()};
            if (not_tk.is_name("not")) {
                // not (a == 1 and b == 1)
                //  vs
                // not a == 1 and b == 1
                if (tz.is_next_char('(')) {
                    // not (a=1 and b=1)
                    bools_.emplace_back(bool_ops_list{tc, tz, true, not_tk});
                } else {
                    // not a=1 and b=1
                    tz.put_back_token(not_tk);
                    bools_.emplace_back(bool_op{tc, tz});
                }
            } else {
                // (a == 1 and b == 1)
                // a == 1 and b == 1
                tz.put_back_token(not_tk);
                if (tz.is_next_char('(')) {
                    // (a == 1 and b == 1)
                    bools_.emplace_back(bool_ops_list{tc, tz, true});
                } else {
                    // a == 1 and b == 1
                    bools_.emplace_back(bool_op{tc, tz});
                }
            }

            if (enclosed_ and tz.is_next_char(')')) {
                return;
            }

            token op_tk{tz.next_token()};
            if (not(op_tk.is_name("or") or op_tk.is_name("and"))) {
                tz.put_back_token(op_tk);
                break;
            }

            // if first op and is 'and' then create sub-expression
            if (prv_op.is_empty()) {
                prv_op = op_tk;
                if (op_tk.is_name("and")) {
                    // a and b or c -> (a and b) or c
                    // first op is 'and', make sub-expression (a and b) ...
                    bool_ops_list bol{tc,
                                      tz,
                                      false,
                                      {},
                                      true,
                                      std::move(bools_.back()),
                                      std::move(op_tk)};
                    bools_.pop_back();
                    bools_.emplace_back(std::move(bol));

                    if (enclosed_ and tz.is_next_char(')')) {
                        return;
                    }

                    token nxt_op_tk{tz.next_token()};
                    if (not(nxt_op_tk.is_name("or") or
                            nxt_op_tk.is_name("and"))) {
                        tz.put_back_token(nxt_op_tk);
                        break;
                    }

                    prv_op = nxt_op_tk;
                    ops_.emplace_back(std::move(nxt_op_tk));
                    continue;
                }
            }

            // if same op as previous continue
            if (op_tk.is_name(prv_op.name())) {
                ops_.emplace_back(std::move(op_tk));
                continue;
            }

            // previous op is not the same as next op

            // either a new sub-expression or exit current sub-expression
            // a or b  |or|       |c|          |and|  d or e
            //      |prv_op_tk| |ops.back()|  |op_tk|
            if (is_sub_expr) {
                // generated sub-expressions are 'and' ops and this is an 'or'
                // a or b and c  |or| d
                //      ------- |op_tk|
                tz.put_back_token(op_tk);
                return;
            }

            // this is an 'and' op after a 'or'
            // a    |or|      |b|     |and|   c or d
            //  |prv_op_tk| |back()| |op_tk|
            // create:
            // a    or   (b     and   c) or d
            bool_ops_list bol{
                tc, tz, false, {}, true, std::move(bools_.back()), op_tk};
            bools_.pop_back();
            bools_.emplace_back(std::move(bol));

            if (enclosed_ and tz.is_next_char(')')) {
                return;
            }

            prv_op = op_tk;
            op_tk = tz.next_token();
            if (not(op_tk.is_name("or") or op_tk.is_name("and"))) {
                tz.put_back_token(op_tk);
                break;
            }

            ops_.emplace_back(std::move(op_tk));
        }
        if (enclosed_) {
            throw compiler_exception(tok(), "expected ')' to close expression");
        }
    }

    inline bool_ops_list() = default;
    inline bool_ops_list(const bool_ops_list&) = default;
    inline bool_ops_list(bool_ops_list&&) = default;
    inline auto operator=(const bool_ops_list&) -> bool_ops_list& = default;
    inline auto operator=(bool_ops_list&&) -> bool_ops_list& = default;

    inline ~bool_ops_list() override = default;

    inline void source_to(std::ostream& os) const override {
        statement::source_to(os);
        not_token_.source_to(os);
        if (enclosed_) {
            os << "(";
        }
        const size_t n{bools_.size()};
        for (size_t i{0}; i < n; i++) {
            if (bools_.at(i).index() == 0) {
                get<bool_op>(bools_.at(i)).source_to(os);
            } else {
                get<bool_ops_list>(bools_.at(i)).source_to(os);
            }
            if (i < n - 1) {
                ops_.at(i).source_to(os);
            }
        }
        if (enclosed_) {
            os << ")";
        }
    }

    [[noreturn]] inline void
    compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
            [[maybe_unused]] size_t indent,
            [[maybe_unused]] const std::string& dst = "") const override {
        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    [[nodiscard]] inline auto cmp_bgn_label(const toc& tc) const
        -> std::string {
        const std::string& call_path{tc.get_inline_call_path(tok())};
        return "cmp_" + tc.source_location_for_use_in_label(tok()) +
               (call_path.empty() ? "" : "_" + call_path);
    }

    inline auto compile(toc& tc, std::ostream& os, const size_t indent,
                        const std::string& jmp_to_if_false,
                        const std::string& jmp_to_if_true,
                        const bool inverted) const -> std::optional<bool> {

        toc::indent(os, indent, true);
        tc.comment_source(os, "?", inverted ? " inverted: " : " ", *this);
        // invert according to De Morgan's laws
        const bool invert{inverted ? not not_token_.is_name("not")
                                   : not_token_.is_name("not")};
        const size_t n{bools_.size()};
        for (size_t i{0}; i < n; i++) {
            if (bools_.at(i).index() == 1) {
                //
                // bool_ops_list
                //
                const bool_ops_list& el{get<bool_ops_list>(bools_.at(i))};
                std::string jmp_false{jmp_to_if_false};
                std::string jmp_true{jmp_to_if_true};
                if (i < n - 1) {
                    if (!invert) {
                        // if not last element check if it is a 'or' or 'and'
                        // list
                        if (ops_.at(i).is_name("or")) {
                            // if evaluation is false and next op is "or" then
                            // jump_false is next bool eval
                            jmp_false = cmp_label_from(tc, bools_.at(i + 1));
                        } else if (ops_.at(i).is_name("and")) {
                            // if evaluation is true and next op is "and" then
                            // jump_true is next bool eval
                            jmp_true = cmp_label_from(tc, bools_.at(i + 1));
                        } else {
                            throw panic_exception("expected 'or' or 'and'" +
                                                  std::string{__FILE__} + ":" +
                                                  std::to_string(__LINE__));
                        }
                        std::optional<bool> const_eval{el.compile(
                            tc, os, indent, jmp_false, jmp_true, invert)};
                        if (const_eval) {
                            // expression evaluated to a constant

                            // if false and in an 'and' list short-circuit and
                            // return evaluation
                            if (not *const_eval and ops_.at(i).is_name("and")) {
                                return *const_eval;
                            }
                            // if true and in an 'or' list short-circuit and
                            // return result
                            if (*const_eval and ops_.at(i).is_name("or")) {
                                return *const_eval;
                            }
                        }
                    } else {
                        // invert according to De Morgan's laws
                        // if not last element check if it is a 'or' or 'and'
                        // list
                        if (ops_.at(i).is_name("and")) {
                            // if evaluation is false and next op is "or"
                            // (inverted) then jump_false is next bool eval
                            jmp_false = cmp_label_from(tc, bools_.at(i + 1));
                        } else if (ops_.at(i).is_name("or")) {
                            // if evaluation is true and next op is "and"
                            // (inverted) then jump_true is next bool eval
                            jmp_true = cmp_label_from(tc, bools_.at(i + 1));
                        } else {
                            throw panic_exception("expected 'or' or 'and'" +
                                                  std::string{__FILE__} + ":" +
                                                  std::to_string(__LINE__));
                        }
                        std::optional<bool> const_eval{el.compile(
                            tc, os, indent, jmp_false, jmp_true, invert)};
                        if (const_eval) {
                            // expression evaluated to a constant

                            // if false and in an 'and' (inverted 'or') list
                            // short-circuit and return evaluation
                            if (not *const_eval and ops_.at(i).is_name("or")) {
                                return *const_eval;
                            }
                            // if true and in an 'or' (inverted 'and') list
                            // short-circuit and return evaluation
                            if (*const_eval and ops_.at(i).is_name("and")) {
                                return *const_eval;
                            }
                        }
                    }
                } else {
                    // if last in list jmp_false is next bool eval
                    std::optional<bool> const_eval{el.compile(
                        tc, os, indent, jmp_false, jmp_true, invert)};
                    if (const_eval) {
                        // if evaluated to a constant then that is the final
                        // evaluation of this expression
                        return *const_eval;
                    }
                }
                continue;
            }

            //
            // bool_op
            //
            if (!invert) {
                // a == 1 and b == 2   vs   a == 1 or b == 2
                const bool_op& e{get<bool_op>(bools_.at(i))};
                if (i < n - 1) {
                    if (ops_.at(i).is_name("or")) {
                        std::optional<bool> const_eval{e.compile_or(
                            tc, os, indent, jmp_to_if_true, invert)};
                        if (const_eval and *const_eval) {
                            // constant evaluated to true, short-circuit
                            return true;
                        }
                    } else if (ops_.at(i).is_name("and")) {
                        std::optional<bool> const_eval{e.compile_and(
                            tc, os, indent, jmp_to_if_false, invert)};
                        if (const_eval and not *const_eval) {
                            // constant evaluated to false, short-circuit
                            return false;
                        }
                    } else {
                        throw panic_exception("expected 'or' or 'and'" +
                                              std::string{__FILE__} + ":" +
                                              std::to_string(__LINE__));
                    }
                } else {
                    std::optional<bool> const_eval{
                        e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
                    if (const_eval) { // constant evaluated
                        return *const_eval;
                    }
                    // if last element and not yet jumped to false then jump to
                    // true
                    toc::asm_jmp(tok(), os, indent, jmp_to_if_true);
                }
            } else {
                // inverted according to De Morgan's laws
                // a=1 and b=2   vs   a=1 or b=2
                const bool_op& e{get<bool_op>(bools_.at(i))};
                if (i < n - 1) {
                    if (ops_.at(i).is_name("and")) {
                        std::optional<bool> const_eval{e.compile_or(
                            tc, os, indent, jmp_to_if_true, invert)};
                        if (const_eval and *const_eval) {
                            // constant evaluated to true, short-circuit
                            return true;
                        }
                    } else if (ops_.at(i).is_name("or")) {
                        std::optional<bool> const_eval{e.compile_and(
                            tc, os, indent, jmp_to_if_false, invert)};
                        if (const_eval and not *const_eval) {
                            // constant evaluated to false, short-circuit
                            return false;
                        }
                    } else {
                        throw panic_exception("expected 'or' or 'and'" +
                                              std::string{__FILE__} + ":" +
                                              std::to_string(__LINE__));
                    }
                } else {
                    std::optional<bool> const_eval{
                        e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
                    if (const_eval) { // constant evaluated
                        return *const_eval;
                    }
                    // if last element and not yet jumped to false then jump to
                    // true
                    toc::asm_jmp(tok(), os, indent, jmp_to_if_true);
                }
            }
        }
        return std::nullopt;
    }

    //? assumes it is not an expression
    [[nodiscard]] inline auto is_expression() const -> bool override {
        if (bools_.size() > 1) {
            return true;
        }

        assert(!bools_.empty());

        if (bools_.at(0).index() == 0) {
            return get<bool_op>(bools_.at(0)).is_expression();
        }

        return get<bool_ops_list>(bools_.at(0)).is_expression();
    }

    [[nodiscard]] inline auto identifier() const
        -> const std::string& override {
        if (bools_.size() > 1) {
            throw panic_exception("unexpected code path " +
                                  std::string{__FILE__} + ":" +
                                  std::to_string(__LINE__));
        }

        assert(!bools_.empty());

        if (bools_.at(0).index() == 0) {
            return get<bool_op>(bools_.at(0)).identifier();
        }

        return get<bool_ops_list>(bools_.at(0)).identifier();
    }

  private:
    inline static auto
    cmp_label_from(const toc& tc,
                   const std::variant<bool_op, bool_ops_list>& var)
        -> std::string {

        if (var.index() == 1) {
            return get<bool_ops_list>(var).cmp_bgn_label(tc);
        }

        return get<bool_op>(var).cmp_bgn_label(tc);
    }

    inline static auto
    token_from(const std::variant<bool_op, bool_ops_list>& var) -> token {

        if (var.index() == 0) {
            return get<bool_op>(var).tok();
        }

        return get<bool_ops_list>(var).tok();
    }
};
