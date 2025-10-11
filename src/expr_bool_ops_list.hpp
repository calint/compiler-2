#pragma once
// reviewed: 2025-09-29

#include <variant>

#include "expr_bool_op.hpp"
#include "statement.hpp"

// list of boolean expressions / lists instead of tree
// note: quirky parsing and compiling but supports short-circuiting
class expr_bool_ops_list final : public statement {
    std::vector<std::variant<expr_bool_op, expr_bool_ops_list>> bools_;
    std::vector<token> ops_; // 'and' or 'or' ops between element in 'bools_'
    bool enclosed_{};        // e.g. (a==b and c==d) vs a==b and c==d
    token not_token_;        // e.g. not (a==b and c==d)
    token ws1_;              // whitespace after parenthesis

  public:
    expr_bool_ops_list(toc& tc, token tk, tokenizer& tz,
                       const bool enclosed = false, token not_token = {})
        : statement{tk}, enclosed_{enclosed}, not_token_{not_token} {

        set_type(tc.get_type_bool());

        token prv_op{};

        // parse
        while (true) {
            // place a marker at this location to be able to rewind if
            // speculative parsing failed
            const token rewind_pos_tk{tz.current_position_token()};
            // this token may be "not"
            token maybe_not_tk{tz.next_token()};
            // is it "not"?
            if (not maybe_not_tk.is_text("not")) {
                // no, put the token back and make it into the whitespace
                tz.put_back_token(maybe_not_tk);
                maybe_not_tk = tz.next_whitespace_token();
            }
            // place the position at the beginning of the parenthesis or start
            // of expression
            const token pos_tk{tz.current_position_token()};
            // is it start of new sub-expression?
            if (tz.is_next_char('(')) {
                // yes, try as 'expr_bool_ops_list' but it might not be that
                // e.g.: (t1 + t2) > 3 is not but will compile so further checks
                // are necessary after the parsing
                expr_bool_ops_list bol{tc, pos_tk, tz, true, maybe_not_tk};
                // check if 'expr_bool_ops_list' parsed an expression,
                // wrongfully, as the shorthand boolean expression
                //   e.g. not ((t1 + t2) > 2)
                //        where (t1 + t2) is a valid 'expr_bool_ops_list' of 1
                //        element with the expression 't1 + t2'
                if (std::string_view{"<>=!+-*/%&|^"}.contains(tz.peek_char())) {
                    // it is a 'bool_op', reposition the tokenizer and parse it
                    tz.rewind_to_position(rewind_pos_tk);
                    bools_.emplace_back(std::in_place_type<expr_bool_op>, tc,
                                        tz);
                } else {
                    // is not an 'expr_bool_op'
                    bools_.emplace_back(std::move(bol));
                }
            } else {
                // put back the token in the tokenizer for the 'expr_bool_op' to
                // parse it
                tz.put_back_token(maybe_not_tk);
                bools_.emplace_back(std::in_place_type<expr_bool_op>, tc, tz);
            }

            // end of '(...)' enclosed expression?
            if (enclosed_ and tz.is_next_char(')')) {
                // yes, done
                ws1_ = tz.next_whitespace_token();
                return;
            }

            // read 'and' or 'or'
            const token op_tk{tz.next_token()};
            if (not op_tk.is_text("or") and not op_tk.is_text("and")) {
                // not expected keyword, end of expression, put token back and
                // return

                // is the expression enclosed and no closing ')' found?
                if (enclosed_) {
                    // yes, fail
                    throw compiler_exception{
                        tok(), "expected ')' to close expression"};
                }

                // success
                tz.put_back_token(op_tk);
                return;
            }

            // get the 'and' or 'or' mode of this expression
            if (prv_op.is_empty()) {
                prv_op = op_tk;
            }

            // is it mixing 'and's and 'or's?
            if (not prv_op.is_text(op_tk.text())) {
                // yes, not allowed
                throw compiler_exception{
                    op_tk, "mixing 'and' and 'or' without parenthesis"};
            }

            // add the list of ops
            ops_.emplace_back(op_tk);
        }
    }

    ~expr_bool_ops_list() override = default;

    expr_bool_ops_list() = default;
    expr_bool_ops_list(const expr_bool_ops_list&) = default;
    expr_bool_ops_list(expr_bool_ops_list&&) = default;
    auto operator=(const expr_bool_ops_list&) -> expr_bool_ops_list& = default;
    auto operator=(expr_bool_ops_list&&) -> expr_bool_ops_list& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        not_token_.source_to(os);
        if (enclosed_) {
            std::print(os, "(");
        }
        const size_t n{bools_.size()};
        for (size_t i{}; i < n; i++) {
            if (bools_.at(i).index() == 0) {
                get<expr_bool_op>(bools_.at(i)).source_to(os);
            } else {
                get<expr_bool_ops_list>(bools_.at(i)).source_to(os);
            }
            if (i < n - 1) {
                ops_.at(i).source_to(os);
            }
        }
        if (enclosed_) {
            std::print(os, ")");
            ws1_.source_to(os);
        }
    }

    [[noreturn]] auto compile([[maybe_unused]] toc& tc,
                              [[maybe_unused]] std::ostream& os,
                              [[maybe_unused]] size_t indent,
                              [[maybe_unused]] std::string_view dst) const
        -> void override {

        throw panic_exception("unexpected code path expr_bool_ops_list:1");
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 std::string_view jmp_to_if_false,
                 std::string_view jmp_to_if_true, bool inverted) const
        -> std::optional<bool> {

        tc.comment_source(os, indent, "?", inverted ? " inverted: " : " ",
                          *this);
        // invert, according to De Morgan's laws
        const bool invert{inverted ? not not_token_.is_text("not")
                                   : not_token_.is_text("not")};
        const size_t n{bools_.size()};
        for (size_t i{}; i < n; i++) {
            if (bools_.at(i).index() == 1) {
                //
                // expr_bool_ops_list
                //
                const expr_bool_ops_list& el{
                    get<expr_bool_ops_list>(bools_.at(i))};
                toc::asm_label(tok(), os, indent, el.create_cmp_bgn_label(tc));
                std::string jmp_false{jmp_to_if_false};
                std::string jmp_true{jmp_to_if_true};
                if (i < n - 1) {
                    if (not invert) {
                        // if not last element check if it is an 'or' or
                        // 'and' list
                        if (ops_.at(i).is_text("or")) {
                            // if evaluation is false and the next op is 'or'
                            // then 'jump_false' goes to the next bool op in
                            // the list
                            jmp_false =
                                create_cmp_label_from(tc, bools_.at(i + 1));
                        } else if (ops_.at(i).is_text("and")) {
                            // if evaluation is true and next op is 'and'
                            // then 'jump_true' goes to the next bool op in the
                            // list
                            jmp_true =
                                create_cmp_label_from(tc, bools_.at(i + 1));
                        } else {
                            throw panic_exception("expected 'or' or 'and' 1");
                        }
                        std::optional<bool> const_eval{el.compile(
                            tc, os, indent, jmp_false, jmp_true, invert)};
                        if (const_eval) {
                            // expression evaluated to a constant

                            // if false and in an 'and' list short-circuit
                            // and return evaluation
                            if (not *const_eval and ops_.at(i).is_text("and")) {
                                return *const_eval;
                            }
                            // if true and in an 'or' list short-circuit and
                            // return result
                            if (*const_eval and ops_.at(i).is_text("or")) {
                                return *const_eval;
                            }
                        }
                    } else {
                        // invert, according to De Morgan's laws
                        // if not, the last element check if it is an 'or' or
                        // 'and' list
                        if (ops_.at(i).is_text("and")) {
                            // 'and' list inverted
                            // if evaluation is false and the next op is 'or'
                            // (inverted from 'and') then 'jump_false' is
                            // the next bool op in the list
                            jmp_false =
                                create_cmp_label_from(tc, bools_.at(i + 1));
                        } else if (ops_.at(i).is_text("or")) {
                            // 'or' list inverted
                            // if evaluation is true and the next op is 'and'
                            // (inverted from 'or') then 'jump_true' is the next
                            // bool op in the list
                            jmp_true =
                                create_cmp_label_from(tc, bools_.at(i + 1));
                        } else {
                            throw panic_exception("expected 'or' or 'and' 2");
                        }

                        std::optional<bool> const_eval{el.compile(
                            tc, os, indent, jmp_false, jmp_true, invert)};
                        // did expression evaluate to a constant?
                        if (const_eval) {
                            // yes, short-circuit

                            // if 'false' and in an 'and' (inverted 'or')
                            // list short-circuit and return evaluation
                            if (not *const_eval and ops_.at(i).is_text("or")) {
                                return *const_eval;
                            }
                            // if 'true' and in an 'or' (inverted 'and')
                            // list short-circuit and return evaluation
                            if (*const_eval and ops_.at(i).is_text("and")) {
                                return *const_eval;
                            }
                        }
                    }
                } else {
                    // the last bool op in the list
                    // 'jmp_false' is the next bool ops list
                    std::optional<bool> const_eval{el.compile(
                        tc, os, indent, jmp_false, jmp_true, invert)};
                    // did expression evaluate to a constant?
                    if (const_eval) {
                        // yes, return it
                        return *const_eval;
                    }
                }
                continue;
            }

            //
            // expr_bool_op
            //
            if (not invert) {
                // a == 1 and b == 2   vs   a == 1 or b == 2
                const expr_bool_op& e{get<expr_bool_op>(bools_.at(i))};
                if (i < n - 1) {
                    // not last element
                    if (ops_.at(i).is_text("or")) {
                        std::optional<bool> const_eval{e.compile_or(
                            tc, os, indent, jmp_to_if_true, invert)};
                        if (const_eval and *const_eval) {
                            // constant evaluated to true, short-circuit
                            return true;
                        }
                    } else if (ops_.at(i).is_text("and")) {
                        std::optional<bool> const_eval{e.compile_and(
                            tc, os, indent, jmp_to_if_false, invert)};
                        if (const_eval and not *const_eval) {
                            // constant evaluated to false, short-circuit
                            return false;
                        }
                    } else {
                        throw panic_exception("expected 'or' or 'and' 3");
                    }
                } else {
                    // last element
                    std::optional<bool> const_eval{
                        e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
                    if (const_eval) {
                        return *const_eval;
                    }
                    // if not yet jumped to false then jump to true
                    toc::asm_jmp(tok(), os, indent, jmp_to_if_true);
                }
            } else {
                // inverted according to De Morgan's laws
                // a=1 and b=2   vs   a=1 or b=2
                const expr_bool_op& e{get<expr_bool_op>(bools_.at(i))};
                if (i < n - 1) {
                    // not last element
                    if (ops_.at(i).is_text("and")) {
                        std::optional<bool> const_eval{e.compile_or(
                            tc, os, indent, jmp_to_if_true, invert)};
                        if (const_eval and *const_eval) {
                            // constant evaluated to true, short-circuit
                            return true;
                        }
                    } else if (ops_.at(i).is_text("or")) {
                        std::optional<bool> const_eval{e.compile_and(
                            tc, os, indent, jmp_to_if_false, invert)};
                        if (const_eval and not *const_eval) {
                            // constant evaluated to false, short-circuit
                            return false;
                        }
                    } else {
                        throw panic_exception("expected 'or' or 'and' 4");
                    }
                } else {
                    // last element
                    std::optional<bool> const_eval{
                        e.compile_and(tc, os, indent, jmp_to_if_false, invert)};
                    if (const_eval) {
                        return *const_eval;
                    }
                    // if not yet jumped to false then jump to true
                    toc::asm_jmp(tok(), os, indent, jmp_to_if_true);
                }
            }
        }
        return std::nullopt;
    }

    //? assumes it is not an expression
    [[nodiscard]] auto is_expression() const -> bool override {
        // is there more than 1 bool in the list?
        if (bools_.size() > 1) {
            // yes, it is an expression
            return true;
        }

        assert(not bools_.empty());

        // 1 expression in the list

        // is it an 'expr_bool_ops'?
        if (bools_.at(0).index() == 0) {
            // yes, call its 'is_expression'
            return get<expr_bool_op>(bools_.at(0)).is_expression();
        }

        // it is an 'expr_bool_ops_list'

        return get<expr_bool_ops_list>(bools_.at(0)).is_expression();
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        if (bools_.size() > 1) {
            throw panic_exception("unexpected code path expr_bool_ops_list:5");
        }

        assert(not bools_.empty());

        // is it an 'expr_bool_ops'?
        if (bools_.at(0).index() == 0) {
            // yes, call its 'identifier'
            return get<expr_bool_op>(bools_.at(0)).identifier();
        }

        // it is an 'expr_bool_ops_list'

        return get<expr_bool_ops_list>(bools_.at(0)).identifier();
    }

  private:
    [[nodiscard]] auto create_cmp_bgn_label(const toc& tc) const
        -> std::string {

        const std::string_view call_path{tc.get_call_path(tok())};
        return std::format("cmp_{}{}",
                           tc.source_location_for_use_in_label(tok()),
                           (call_path.empty() ? std::string{}
                                              : std::format("_{}", call_path)));
    }

    static auto create_cmp_label_from(
        const toc& tc,
        const std::variant<expr_bool_op, expr_bool_ops_list>& var)
        -> std::string {

        if (var.index() == 1) {
            return get<expr_bool_ops_list>(var).create_cmp_bgn_label(tc);
        }

        return get<expr_bool_op>(var).create_cmp_bgn_label(tc);
    }
};
