#pragma once
// reviewed: 2025-09-29

#include <cassert>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "decouple.hpp"
#include "expr_arith.hpp"
#include "statement.hpp"

class expr_bool_op final : public statement {
    std::vector<token> nots_;
    expr_arith lhs_;
    token ws_pre_op_;
    std::string op_; // '<', '<=', '>', '>=', '==', '!='
    token ws_post_op_;
    expr_arith rhs_;
    bool is_not_{};       // e.g. if not a == b ...
    bool is_shorthand_{}; // e.g. if a ...
    bool is_expression_{};

  public:
    expr_bool_op(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()} {

        set_type(tc.get_type_bool());

        bool is_not{};
        // e.g. if not a == 3 ...
        while (true) {
            const token t{tz.next_token()};
            if (not t.is_text("not")) {
                tz.put_back_token(t);
                break;
            }
            is_not = not is_not;
            nots_.emplace_back(t);
        }
        is_not_ = is_not;

        lhs_ = {tc, tz, true};

        ws_pre_op_ = tz.next_whitespace_token();

        if (tz.is_next_char('=')) {
            if (not tz.is_next_char('=')) {
                throw compiler_exception{tz, "expected '=='"};
            }
            op_ = "==";
        } else if (tz.is_next_char('!')) {
            if (not tz.is_next_char('=')) {
                throw compiler_exception{tz, "expected '!='"};
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
            // e.g. if a ...
            is_shorthand_ = true;
            resolve_if_op_is_expression();

            return;
        }

        ws_post_op_ = tz.next_whitespace_token();

        rhs_ = {tc, tz, true};
        resolve_if_op_is_expression();
    }

    expr_bool_op() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        for (const token& e : nots_) {
            e.source_to(os);
        }
        lhs_.source_to(os);
        ws_pre_op_.source_to(os);
        if (is_shorthand_) {
            return;
        }
        std::print(os, "{}", op_);
        ws_post_op_.source_to(os);
        rhs_.source_to(os);
    }

    // returns an optional bool, and if defined the expression evaluated to
    // the value of the optional
    [[nodiscard]] auto compile_or(toc& tc, const size_t indent,
                                  const std::string_view jmp_to_if_true,
                                  const bool inverted, const operand& dst) const
        -> std::optional<bool> {

        const bool invert{inverted ? not is_not_ : is_not_};

        machine& x{tc.machine()};

        x.comment(tok(), indent,
                  statement::trimmed_source(
                      *this, "?", inverted ? " 'or' inverted: " : " "));

        x.label(indent, create_cmp_bgn_label(tc));
        if (is_shorthand_) {
            // is 'lhs' a constant?
            if (not lhs_.is_expression()) {
                // yes, the left-hand-side is not an expression, either a
                // constant or an identifier
                const ident_info& lhs_info{tc.make_ident_info(lhs_)};
                if (lhs_info.is_const()) {
                    bool const_eval{lhs_.get_unary_ops().evaluate_constant(
                                        lhs_info.const_value) != 0};

                    if (invert) {
                        const_eval = not const_eval;
                    }
                    x.comment(lhs_.tok(), indent, "const eval to {}",
                              (const_eval ? "true" : "false"));

                    if (const_eval) {
                        // since it is an 'or' chain short-circuit
                        // expression and jump to label for true
                        x.branch(indent, jmp_to_if_true);
                    }

                    return const_eval;
                }
            }

            // 'lhs' is an expression
            resolve_cmp_shorthand(tc, indent, lhs_,
                                  {
                                      .operation{"!="},
                                      .inverted{invert},
                                      .destination{dst},
                                      .target{jmp_to_if_true},
                                      .branch_on_true{true},
                                  });

            return std::nullopt;
        }

        // not shorthand boolean expression

        // check case when both operands are constants
        if (not lhs_.is_expression() and not rhs_.is_expression()) {
            const ident_info& lhs_info{tc.make_ident_info(lhs_)};
            const ident_info& rhs_info{tc.make_ident_info(rhs_)};
            if (lhs_info.is_const() and rhs_info.is_const()) {
                bool const_eval{
                    eval_constant(lhs_.get_unary_ops().evaluate_constant(
                                      lhs_info.const_value),
                                  op_,
                                  rhs_.get_unary_ops().evaluate_constant(
                                      rhs_info.const_value))};

                if (invert) {
                    const_eval = not const_eval;
                }
                x.comment(lhs_.tok(), indent, "const eval to {}",
                          (const_eval ? "true" : "false"));

                if (const_eval) {
                    // expression evaluated at compile time and true so
                    // short-circuit and jump to true
                    x.branch(indent, jmp_to_if_true);
                }

                return const_eval;
            }
        }

        // left-hand-side or right-hand-side or both are expressions
        // note: if lhs is constant, then a scratch register is used, however,
        //       the if statement compile time evaluates constant expressions
        //       before reaching this
        resolve_cmp(tc, indent, lhs_, rhs_,
                    {
                        .operation{op_},
                        .inverted{invert},
                        .destination{dst},
                        .target{jmp_to_if_true},
                        .branch_on_true{true},
                    });

        return std::nullopt;
    }

    [[nodiscard]] auto compile_and(toc& tc, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const bool inverted, const operand& dst,
                                   const bool branch_required = true) const
        -> std::optional<bool> {

        const bool invert{inverted ? not is_not_ : is_not_};

        machine& x{tc.machine()};

        x.comment(tok(), indent,
                  statement::trimmed_source(
                      *this, "?", inverted ? " 'and' inverted: " : " "));

        x.label(indent, create_cmp_bgn_label(tc));
        if (is_shorthand_) {
            // check case when operand is constant
            if (not lhs_.is_expression()) {
                const ident_info& lhs_info{tc.make_ident_info(lhs_)};
                if (lhs_info.is_const()) {
                    bool const_eval{lhs_.get_unary_ops().evaluate_constant(
                                        lhs_info.const_value) != 0};

                    if (invert) {
                        const_eval = not const_eval;
                    }
                    x.comment(lhs_.tok(), indent, "const eval to {}",
                              (const_eval ? "true" : "false"));

                    if (not const_eval) {
                        // since it is an 'and' chain short-circuit
                        // expression and jump to label for false
                        x.branch(indent, jmp_to_if_false);
                    }

                    return const_eval;
                }
            }

            // left-hand-side is expression
            resolve_cmp_shorthand(
                tc, indent, lhs_,
                {
                    .operation{"!="},
                    .inverted{invert},
                    .destination{dst},
                    .target{branch_required ? jmp_to_if_false
                                            : std::string_view{}},
                    .branch_on_true{},
                });

            return std::nullopt;
        }

        // not shorthand expression
        // check the case when both operands are constants
        if (not lhs_.is_expression() and not rhs_.is_expression()) {
            const ident_info& lhs_info{tc.make_ident_info(lhs_)};
            const ident_info& rhs_info{tc.make_ident_info(rhs_)};
            if (lhs_info.is_const() and rhs_info.is_const()) {
                bool const_eval{
                    eval_constant(lhs_.get_unary_ops().evaluate_constant(
                                      lhs_info.const_value),
                                  op_,
                                  rhs_.get_unary_ops().evaluate_constant(
                                      rhs_info.const_value))};

                if (invert) {
                    const_eval = not const_eval;
                }
                x.comment(lhs_.tok(), indent, "const eval to {}",
                          (const_eval ? "true" : "false"));

                if (not const_eval) {
                    // short circuit 'and' chain
                    x.branch(indent, jmp_to_if_false);
                }

                return const_eval;
            }
        }

        // don't allow left-hand-side to be constant because generated
        // assembler does not compile
        // if (not lhs_.is_expression()) {
        //     const ident_info& lhs_info{tc.make_ident_info(lhs_, false)};
        //     if (lhs_info.is_const()) {
        //         throw compiler_exception(
        //             lhs_.tok(),
        //             "left hand side expression may not be a constant");
        //     }
        // }

        resolve_cmp(tc, indent, lhs_, rhs_,
                    {
                        .operation{op_},
                        .inverted{invert},
                        .destination{dst},
                        .target{jmp_to_if_false},
                        .branch_on_true{},
                    });

        return std::nullopt;
    }

    [[nodiscard]] auto create_cmp_bgn_label(const toc& tc) const
        -> std::string {

        return tc.create_unique_label(tok(), "cmp");
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(not is_expression_);

        return lhs_.identifier();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return is_expression_;
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        lhs_.visit_reads(var, reader);
        rhs_.visit_reads(var, reader);
    }

  private:
    auto resolve_if_op_is_expression() -> void {
        // is it a negated expression?
        if (is_not_) {
            // yes, then it is an expression
            is_expression_ = true;

            return;
        }

        if (not is_shorthand_) {
            is_expression_ = true;

            return;
        }

        // shorthand expressions
        if (lhs_.is_expression()) {
            is_expression_ = true;

            return;
        }

        // if not expression, then it is a single statement and identifier is
        // valid
        const std::string_view id{lhs_.identifier()};
        // is it a boolean value?
        if (id == "true" or id == "false") {
            // yes, not an expression
            is_expression_ = false;

            return;
        }

        // not a boolean value thus a variable
        is_expression_ = true;
    }

    [[nodiscard]] static auto eval_constant(const int64_t lh,
                                            const std::string_view op,
                                            const int64_t rh) -> bool {

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
        std::unreachable();
    }

    auto resolve_cmp(toc& tc, const size_t indent, const expr_arith& lhs,
                     const expr_arith& rhs,
                     const machine::comparison_action& action) const -> void {

        std::vector<operand> allocated_registers;

        const operand dst{
            resolve_expr(tc, indent, lhs, true, allocated_registers)};

        const operand src{
            resolve_expr(tc, indent, rhs, false, allocated_registers)};

        assert_rhs_fits_lhs(tc, lhs, rhs, op_, dst, src);

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, dst, src, action,
                             allocated_registers);
    }

    // the backends compare at the width of 'lhs' which would truncate 'rhs'
    static auto assert_rhs_fits_lhs(toc& tc, const expr_arith& lhs,
                                    const expr_arith& rhs,
                                    const std::string_view op,
                                    const operand& lhs_op,
                                    const operand& rhs_op) -> void {

        const type& lhs_type{lhs_op.type_ref()};

        if (not rhs_op.is_immediate()) {
            const type& rhs_type{rhs_op.type_ref()};
            if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
                return;
            }

            throw compiler_exception{
                rhs.tok(),
                std::format(
                    "'{}' of type '{}' is wider than '{}' of type '{}', "
                    "swap the operands: '{} {} {}'",
                    trimmed_source(rhs), rhs_type.name(), trimmed_source(lhs),
                    lhs_type.name(), trimmed_source(rhs),
                    mirrored_operation(op), trimmed_source(lhs))};
        }

        const int64_t value{rhs.get_unary_ops().evaluate_constant(
            tc.make_ident_info(rhs).const_value)};

        if (fits_size_bytes(value, lhs_type.size_bytes())) {
            return;
        }

        throw compiler_exception{
            rhs.tok(),
            std::format("constant '{}' does not fit '{}' of type '{}'", value,
                        trimmed_source(lhs), lhs_type.name())};
    }

    // the operation that gives the same result with the operands swapped
    [[nodiscard]] static auto mirrored_operation(const std::string_view op)
        -> std::string_view {

        if (op == "<") {
            return ">";
        }
        if (op == "<=") {
            return ">=";
        }
        if (op == ">") {
            return "<";
        }
        if (op == ">=") {
            return "<=";
        }

        return op;
    }

    auto resolve_cmp_shorthand(toc& tc, const size_t indent,
                               const expr_arith& lhs,
                               const machine::comparison_action& action) const
        -> void {

        if (lhs.produces_boolean() and not action.destination.is_empty() and
            lhs.get_type().name() == action.destination.type_ref().name()) {

            lhs.compile_boolean(tc, indent + 1, action.destination,
                                action.inverted);

            machine& x{tc.machine()};

            if (not action.target.empty()) {
                machine::comparison_action branch_action{action};
                // branch on the stored result using 'action.branch_on_true'

                branch_action.destination = {};
                // 'compile_boolean' already stored the result and applied
                // inversion

                branch_action.inverted = false;

                x.compare_and_branch(tok(), indent, action.destination,
                                     operand::imm("0", tc.get_type_default()),
                                     branch_action, {});
            }

            return;
        }

        std::vector<operand> allocated_registers;

        operand dst;
        if (lhs.is_expression() and action.destination.is_register() and
            lhs.get_type().name() == action.destination.type_ref().name()) {
            // matching types avoid narrowing before the in-place truth test
            dst = action.destination;
            lhs.compile(tc, indent + 1,
                        toc::make_ident_info_from_register(dst));
        } else {
            dst = resolve_expr(tc, indent, lhs, true, allocated_registers);
        }

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, dst,
                             operand::imm("0", tc.get_type_default()), action,
                             allocated_registers);
    }

    [[nodiscard]] static auto
    compile_to_scratch(toc& tc, const size_t indent, const expr_arith& expr,
                       std::vector<operand>& allocated_registers) -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(expr.tok(), indent, expr.get_type())};

        allocated_registers.emplace_back(reg);
        expr.compile(tc, indent + 1, toc::make_ident_info_from_register(reg));

        return reg;
    }

    [[nodiscard]] static auto
    resolve_expr(toc& tc, const size_t indent, const expr_arith& expr,
                 const bool is_lhs, std::vector<operand>& allocated_registers)
        -> operand {

        if (not expr.is_expression() and
            (expr.is_indexed() or tc.has_lea(expr))) {

            const ident_info expr_info{tc.make_ident_info(expr)};
            return expr.compile_lea(tc, indent, expr.tok(), allocated_registers,
                                    {}, expr_info.lea_path, {});
        }

        if (expr.is_expression()) {
            return compile_to_scratch(tc, indent, expr, allocated_registers);
        }

        // 'expr' is not an expression
        const ident_info expr_info{tc.make_ident_info(expr)};
        if (expr_info.is_const()) {
            if (is_lhs) {
                return compile_to_scratch(tc, indent, expr,
                                          allocated_registers);
            }

            return expr.make_constant_operand(expr_info);
        }

        // 'expr' not a constant, it is an identifier
        if (expr.get_unary_ops().is_empty()) {
            return expr_info.operand;
        }

        // 'expr' is not an expression and has unary ops

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(expr.tok(), indent, expr_info.type_ref())};

        allocated_registers.emplace_back(reg);
        x.copy_value(expr.tok(), indent, reg, expr_info.operand);
        expr.get_unary_ops().compile(tc, indent, reg);

        return reg;
    }
};

// list of boolean expressions / lists instead of tree
// note: quirky parsing and compiling but supports short-circuiting
class expr_bool final : public statement {
    using element = std::variant<expr_bool_op, expr_bool>;

    std::vector<element> bools_;
    std::vector<token> ops_; // 'and' or 'or' ops between element in 'bools_'
    token not_tk_;           // e.g. not (a==b and c==d)
    token open_paren_tk_;
    token close_paren_tk_;
    bool enclosed_{}; // e.g. (a==b and c==d) vs a==b and c==d

  public:
    expr_bool(toc& tc, const token tk, tokenizer& tz,
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
                // yes, try as 'expr_bool' but it might not be that
                // e.g.: (t1 + t2) > 3 is not but will compile so further checks
                // are necessary after the parsing
                expr_bool bol{tc, pos_tk, tz, true, maybe_not_tk, t};
                // check if 'expr_bool' parsed an expression,
                // wrongfully, as the shorthand boolean expression
                //   e.g., not ((t1 + t2) > 2)
                //         where (t1 + t2) is a valid 'expr_bool' of 1
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

    expr_bool() = default;

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

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        for (const element& e : bools_) {
            e.visit([&var, &reader](const auto& item) -> void {
                item.visit_reads(var, reader);
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

        if (std::holds_alternative<expr_bool>(bools_[expr_index])) {
            const expr_bool& nested_expr{
                std::get<expr_bool>(bools_[expr_index])};

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

        if (std::holds_alternative<expr_bool>(bools_.back())) {
            const expr_bool& nested_expr{std::get<expr_bool>(bools_.back())};

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

        return tc.create_unique_label(tok(), "cmp");
    }

    [[nodiscard]] static auto create_cmp_label_from(const toc& tc,
                                                    const element& var)
        -> std::string {

        return var.visit([&tc](const auto& e) -> std::string {
            return e.create_cmp_bgn_label(tc);
        });
    }
};
