#pragma once
// reviewed: 2025-09-29

#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "expr_bool_op.hpp"
#include "statement.hpp"

// list of boolean expressions / lists instead of tree
// note: quirky parsing and compiling but supports short-circuiting
class expr_bool_ops_list final : public statement {
    std::vector<std::variant<expr_bool_op, expr_bool_ops_list>> bools_;
    std::vector<token> ops_; // 'and' or 'or' ops between element in 'bools_'
    token not_tk_;           // e.g. not (a==b and c==d)
    token open_paren_tk_;
    token close_paren_tk_;
    bool enclosed_{}; // e.g. (a==b and c==d) vs a==b and c==d

  public:
    expr_bool_ops_list(toc& tc, const token tk, tokenizer& tz,
                       const bool enclosed = false, const token not_tk = {},
                       const token open_paren_tk = {})
        : statement{tk}, not_tk_{not_tk}, open_paren_tk_{open_paren_tk},
          enclosed_{enclosed} {

        set_type(tc.get_type_bool());

        token prv_op{};

        // parse
        while (true) {
            // place a marker at this location to be able to rewind if
            // speculative parsing failed
            const token rewind_pos_tk{tz.cur_position_token()};
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
            const token pos_tk{tz.cur_position_token()};
            // is it start of new sub-expression?
            if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
                // yes, try as 'expr_bool_ops_list' but it might not be that
                // e.g.: (t1 + t2) > 3 is not but will compile so further checks
                // are necessary after the parsing
                expr_bool_ops_list bol{tc, pos_tk, tz, true, maybe_not_tk, t};
                // check if 'expr_bool_ops_list' parsed an expression,
                // wrongfully, as the shorthand boolean expression
                //   e.g., not ((t1 + t2) > 2)
                //         where (t1 + t2) is a valid 'expr_bool_ops_list' of 1
                //         element with the expression 't1 + t2'
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
            if (enclosed_) {
                close_paren_tk_ = tz.is_next_char_token(')');
                if (not close_paren_tk_.is_empty()) {
                    // yes, done
                    return;
                }
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

    expr_bool_ops_list() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        not_tk_.source_to(os);
        if (enclosed_) {
            open_paren_tk_.source_to(os);
        }
        const size_t n{bools_.size()};
        for (size_t i{}; i < n; ++i) {
            bools_[i].visit([&os](const auto& e) -> void { e.source_to(os); });
            if (i < n - 1) {
                ops_[i].source_to(os);
            }
        }
        if (enclosed_) {
            close_paren_tk_.source_to(os);
        }
    }

    [[noreturn]] auto compile([[maybe_unused]] toc& tc,
                              [[maybe_unused]] const size_t indent,
                              [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        std::unreachable();
    }

    [[nodiscard]] auto compile(toc& tc, const size_t indent,
                               const std::string_view jmp_to_if_false,
                               const std::string_view jmp_to_if_true,
                               const operand& dst) const
        -> std::optional<bool> {

        return compile_rec(tc, indent, jmp_to_if_false, jmp_to_if_true, false,
                           dst);
    }

    // assumes callers only query this when expression status is relevant
    [[nodiscard]] auto is_expression() const -> bool override {
        // is there more than 1 bool in the list?
        if (bools_.size() > 1) {
            // yes, it is an expression
            return true;
        }

        // 'identifier' cannot carry the negation
        if (not_tk_.is_text("not")) {
            return true;
        }

        assert(not bools_.empty());

        // 1 expression in the list

        return bools_[0].visit(
            [](const auto& e) -> bool { return e.is_expression(); });
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(bools_.size() == 1);

        return bools_[0].visit(
            [](const auto& e) -> std::string_view { return e.identifier(); });
    }

    auto assert_var_not_used(const std::string_view var,
                             const field_coverage& assigned) const
        -> void override {

        for (const auto& e : bools_) {
            e.visit([&var, &assigned](const auto& item) -> void {
                item.assert_var_not_used(var, assigned);
            });
        }
    }

  private:
    [[nodiscard]] auto compile_rec(toc& tc, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const std::string_view jmp_to_if_true,
                                   const bool inverted,
                                   const operand& dst) const
        -> std::optional<bool> {

        machine& x{tc.machine()};

        x.comment(tok(), indent,
                  statement::trimmed_source(*this, "?",
                                            inverted ? " inverted: " : " "));

        // invert, according to De Morgan's laws
        const bool invert{inverted ? not not_tk_.is_text("not")
                                   : not_tk_.is_text("not")};

        bool has_runtime_element{};

        const size_t last_index{bools_.size() - 1};
        for (size_t expr_index{}; expr_index < last_index; ++expr_index) {
            const std::optional<bool> const_eval{
                compile_inner_element(tc, indent, expr_index, jmp_to_if_false,
                                      jmp_to_if_true, invert, dst)};

            if (not const_eval) {
                has_runtime_element = true;
                continue;
            }

            if (is_short_circuit(*const_eval, expr_index, invert)) {
                return *const_eval;
            }
        }

        return compile_last_element(tc, indent, jmp_to_if_false, jmp_to_if_true,
                                    invert, dst, has_runtime_element);
    }

    // an element that does not decide the list continues at the next element
    [[nodiscard]] auto
    compile_inner_element(toc& tc, const size_t indent, const size_t expr_index,
                          const std::string_view jmp_to_if_false,
                          const std::string_view jmp_to_if_true,
                          const bool invert, const operand& dst) const
        -> std::optional<bool> {

        const bool is_or{is_effective_or(expr_index, invert)};

        if (std::holds_alternative<expr_bool_ops_list>(bools_[expr_index])) {
            const expr_bool_ops_list& nested_expr{
                std::get<expr_bool_ops_list>(bools_[expr_index])};

            const std::string next_label{
                create_cmp_label_from(tc, bools_[expr_index + 1])};

            // an 'or' continues when false and an 'and' continues when true
            const std::string_view jmp_false{
                is_or ? std::string_view{next_label} : jmp_to_if_false};
            const std::string_view jmp_true{
                is_or ? jmp_to_if_true : std::string_view{next_label}};

            return nested_expr.compile_with_label(tc, indent, jmp_false,
                                                  jmp_true, invert, dst);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_[expr_index])};

        if (is_or) {
            return expr.compile_or(tc, indent, jmp_to_if_true, invert, dst);
        }

        return expr.compile_and(tc, indent, jmp_to_if_false, invert, dst);
    }

    [[nodiscard]] auto compile_last_element(
        toc& tc, const size_t indent, const std::string_view jmp_to_if_false,
        const std::string_view jmp_to_if_true, const bool invert,
        const operand& dst, const bool has_runtime_element) const
        -> std::optional<bool> {

        if (std::holds_alternative<expr_bool_ops_list>(bools_.back())) {
            const expr_bool_ops_list& nested_expr{
                std::get<expr_bool_ops_list>(bools_.back())};

            const std::optional<bool> const_eval{nested_expr.compile_with_label(
                tc, indent, jmp_to_if_false, jmp_to_if_true, invert, dst)};

            // the nested list already emitted its final branch
            if (not const_eval) {
                return std::nullopt;
            }

            return resolve_last_constant(tc, indent, *const_eval,
                                         jmp_to_if_true, invert,
                                         has_runtime_element);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.back())};

        const std::optional<bool> const_eval{
            expr.compile_and(tc, indent, jmp_to_if_false, invert, dst,
                             jmp_to_if_false != jmp_to_if_true)};

        if (const_eval) {
            return resolve_last_constant(tc, indent, *const_eval,
                                         jmp_to_if_true, invert,
                                         has_runtime_element);
        }

        machine& x{tc.machine()};

        // if not yet jumped to false, then jump to true
        x.branch(indent, jmp_to_if_true);

        return std::nullopt;
    }

    // a constant last element decides the list only if all earlier elements
    // were constants or it short-circuits the list
    [[nodiscard]] auto resolve_last_constant(
        toc& tc, const size_t indent, const bool const_eval,
        const std::string_view jmp_to_if_true, const bool invert,
        const bool has_runtime_element) const -> std::optional<bool> {

        if (not has_runtime_element) {
            return const_eval;
        }

        if (is_short_circuit(const_eval, ops_.size() - 1, invert)) {
            return const_eval;
        }

        // a constant false has already branched to false
        if (not const_eval) {
            return std::nullopt;
        }

        machine& x{tc.machine()};

        // a constant true emits no branch of its own
        x.branch(indent, jmp_to_if_true);

        return std::nullopt;
    }

    // earlier elements jump to the label that begins a nested list
    [[nodiscard]] auto compile_with_label(
        toc& tc, const size_t indent, const std::string_view jmp_to_if_false,
        const std::string_view jmp_to_if_true, const bool inverted,
        const operand& dst) const -> std::optional<bool> {

        machine& x{tc.machine()};

        x.label(indent, create_cmp_bgn_label(tc));

        return compile_rec(tc, indent, jmp_to_if_false, jmp_to_if_true,
                           inverted, dst);
    }

    // inversion swaps 'and' and 'or' according to De Morgan's laws
    [[nodiscard]] auto is_effective_or(const size_t op_index,
                                       const bool invert) const -> bool {

        return ops_[op_index].is_text("or") != invert;
    }

    // a constant false ends an 'and' list and a constant true ends an 'or' list
    [[nodiscard]] auto is_short_circuit(const bool const_eval,
                                        const size_t op_index,
                                        const bool invert) const -> bool {

        return const_eval == is_effective_or(op_index, invert);
    }

    [[nodiscard]] auto create_cmp_bgn_label(const toc& tc) const
        -> std::string {

        const std::string_view call_path{tc.get_call_path()};

        return std::format(
            "cmp_{}{}", tc.source_location_for_use_in_label(tok()),
            (call_path.empty() ? "" : std::format("_{}", call_path)));
    }

    [[nodiscard]] static auto create_cmp_label_from(
        const toc& tc,
        const std::variant<expr_bool_op, expr_bool_ops_list>& var)
        -> std::string {

        return var.visit([&tc](const auto& e) -> std::string {
            return e.create_cmp_bgn_label(tc);
        });
    }
};
