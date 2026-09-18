#pragma once
// reviewed: 2025-09-29

#include <cassert>
#include <optional>
#include <ostream>
#include <utility>

#include "decouple.hpp"
#include "expr_ops_list.hpp"

class expr_bool_op final : public statement {
    std::vector<token> nots_;
    expr_ops_list lhs_;
    token ws_pre_op_;
    std::string op_; // '<', '<=', '>', '>=', '==', '!='
    token ws_post_op_;
    expr_ops_list rhs_;
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
    [[nodiscard]] auto compile_or(toc& tc, x86& x, const size_t indent,
                                  const std::string_view jmp_to_if_true,
                                  const bool inverted,
                                  const std::string_view dst) const
        -> std::optional<bool> {

        const bool invert{inverted ? not is_not_ : is_not_};

        x.comment(tok(), indent,
                  statement::trimmed_source(
                      *this, "?", inverted ? " 'or' inverted: " : " "));

        x.label(indent, create_cmp_bgn_label(tc));
        if (is_shorthand_) {
            // is 'lhs' a constant?
            if (not lhs_.is_expression()) {
                // yes, the left-hand-side is not an expression, either a
                // constant or an identifier
                const ident_info& lhs_info{tc.make_ident_info(x, lhs_)};
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
                        x.jmp(indent, jmp_to_if_true);
                    }
                    return const_eval;
                }
            }

            // 'lhs' is an expression
            resolve_cmp_shorthand(tc, x, indent, lhs_);
            // note: compares with 0

            if (not dst.empty()) {
                x.setcc(indent, asm_cc_for_op("!=", invert), dst);
            }
            x.jcc(indent, asm_cc_for_op("!=", invert), jmp_to_if_true);

            return std::nullopt;
        }

        // not shorthand boolean expression

        // check case when both operands are constants
        if (not lhs_.is_expression() and not rhs_.is_expression()) {
            const ident_info& lhs_info{tc.make_ident_info(x, lhs_)};
            const ident_info& rhs_info{tc.make_ident_info(x, rhs_)};
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
                    x.jmp(indent, jmp_to_if_true);
                }
                return const_eval;
            }
        }

        // left-hand-side or right-hand-side or both are expressions
        // note: if lhs is constant, then a scratch register is used, however,
        //       the if statement compile time evaluates constant expressions
        //       before reaching this
        resolve_cmp(tc, x, indent, lhs_, rhs_);

        if (not dst.empty()) {
            x.setcc(indent, asm_cc_for_op(op_, invert), dst);
        }
        x.jcc(indent, asm_cc_for_op(op_, invert), jmp_to_if_true);

        return std::nullopt;
    }

    [[nodiscard]] auto compile_and(toc& tc, x86& x, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const bool inverted,
                                   const std::string_view dst) const
        -> std::optional<bool> {

        const bool invert{inverted ? not is_not_ : is_not_};

        x.comment(tok(), indent,
                  statement::trimmed_source(
                      *this, "?", inverted ? " 'and' inverted: " : " "));

        x.label(indent, create_cmp_bgn_label(tc));
        if (is_shorthand_) {
            // check case when operand is constant
            if (not lhs_.is_expression()) {
                const ident_info& lhs_info{tc.make_ident_info(x, lhs_)};
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
                        x.jmp(indent, jmp_to_if_false);
                    }
                    return const_eval;
                }
            }

            // left-hand-side is expression
            resolve_cmp_shorthand(tc, x, indent, lhs_);
            // note: compares with 0

            if (not dst.empty()) {
                x.setcc(indent, asm_cc_for_op("!=", invert), dst);
            }
            x.jcc(indent, asm_cc_for_op("==", invert), jmp_to_if_false);
            // note: '==' because it jumps to 'if false' label

            return std::nullopt;
        }

        // not shorthand expression
        // check the case when both operands are constants
        if (not lhs_.is_expression() and not rhs_.is_expression()) {
            const ident_info& lhs_info{tc.make_ident_info(x, lhs_)};
            const ident_info& rhs_info{tc.make_ident_info(x, rhs_)};
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
                    x.jmp(indent, jmp_to_if_false);
                }
                return const_eval;
            }
        }

        // don't allow left-hand-side to be constant because generated
        // assembler does not compile
        // if (not lhs_.is_expression()) {
        //     const ident_info& lhs_info{tc.make_ident_info(x, lhs_, false)};
        //     if (lhs_info.is_const()) {
        //         throw compiler_exception(
        //             lhs_.tok(),
        //             "left hand side expression may not be a constant");
        //     }
        // }

        resolve_cmp(tc, x, indent, lhs_, rhs_);
        if (not dst.empty()) {
            x.setcc(indent, asm_cc_for_op(op_, invert), dst);
        }
        x.jcc(indent, asm_cc_for_op(op_, not invert), jmp_to_if_false);
        // note: 'not invert' because it jumps to 'if false' label

        return std::nullopt;
    }

    [[nodiscard]] auto create_cmp_bgn_label(const toc& tc) const
        -> std::string {

        const std::string_view call_path{tc.get_call_path()};
        return std::format("cmp_{}{}",
                           tc.source_location_for_use_in_label(tok()),
                           (call_path.empty() ? std::string{}
                                              : std::format("_{}", call_path)));
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(not is_expression_);
        return lhs_.identifier();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return is_expression_;
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        lhs_.assert_var_not_used(var);
        rhs_.assert_var_not_used(var);
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

    [[nodiscard]] static auto asm_cc_for_op(const std::string_view op,
                                            const bool inverted)
        -> std::string_view {

        if (op == "==") {
            return inverted ? "ne" : "e";
        }
        if (op == "!=") {
            return inverted ? "e" : "ne";
        }
        if (op == "<") {
            return inverted ? "ge" : "l";
        }
        if (op == "<=") {
            return inverted ? "g" : "le";
        }
        if (op == ">") {
            return inverted ? "le" : "g";
        }
        if (op == ">=") {
            return inverted ? "l" : "ge";
        }
        std::unreachable();
    }

    auto resolve_cmp(toc& tc, x86& x, const size_t indent,
                     const expr_ops_list& lhs, const expr_ops_list& rhs) const
        -> void {

        std::vector<std::string> allocated_registers;

        const std::string dst{
            resolve_expr(tc, x, indent, lhs, true, allocated_registers)};

        const std::string src{
            resolve_expr(tc, x, indent, rhs, false, allocated_registers)};

        x.cmp(tok(), indent, dst, src);

        // free allocated registers in reverse order
        for (const std::string& reg :
             allocated_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }
    }

    auto resolve_cmp_shorthand(toc& tc, x86& x, const size_t indent,
                               const expr_ops_list& lhs) const -> void {

        std::vector<std::string> allocated_registers;

        const std::string dst{
            resolve_expr(tc, x, indent, lhs, true, allocated_registers)};

        x.cmp(tok(), indent, dst, "0");

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }
    }

    [[nodiscard]] static auto
    resolve_expr(toc& tc, x86& x, const size_t indent,
                 const expr_ops_list& expr, const bool is_lhs,
                 std::vector<std::string>& allocated_registers) -> std::string {

        if (not expr.is_expression() and
            (expr.is_indexed() or tc.has_lea(expr))) {
            const ident_info expr_info{tc.make_ident_info(x, expr)};
            const operand op{expr.compile_lea(tc, x, indent, expr.tok(),
                                              allocated_registers, "",
                                              expr_info.lea_path)};
            return op.str(expr_info.type_ref().size());
        }

        if (expr.is_expression()) {
            const std::string reg{
                x.alloc_scratch_register(expr.tok(), indent, expr.get_type())};
            allocated_registers.emplace_back(reg);
            expr.compile(tc, x, indent + 1,
                         toc::make_ident_info_from_register(x, reg));
            return x86::get_sized_register_operand(reg, expr.get_type().size());
        }

        // 'expr' is not an expression
        const ident_info expr_info{tc.make_ident_info(x, expr)};
        if (expr_info.is_const()) {
            if (is_lhs) {
                const std::string reg{x.alloc_scratch_register(
                    expr.tok(), indent, tc.get_type_default())};
                allocated_registers.emplace_back(reg);
                expr.compile(tc, x, indent + 1,
                             toc::make_ident_info_from_register(x, reg));
                return x86::get_sized_register_operand(reg,
                                                       expr.get_type().size());
            }
            return std::format("{}{}", expr.get_unary_ops().to_string(),
                               expr_info.const_value);
        }

        // 'expr' not a constant, it is an identifier
        if (expr.get_unary_ops().is_empty()) {
            return expr_info.operand.str();
        }

        // 'expr' is not an expression and has unary ops
        const std::string reg{x.alloc_scratch_register(expr.tok(), indent,
                                                       tc.get_type_default())};
        allocated_registers.emplace_back(reg);
        x.mov(expr.tok(), indent, reg, expr_info.operand.str());
        expr.get_unary_ops().compile(tc, x, indent, reg);
        return x86::get_sized_register_operand(reg,
                                               expr_info.type_ref().size());
    }
};
