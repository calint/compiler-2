#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <ostream>

#include "expr_ops_list.hpp"

class bool_op final : public statement {
    static constexpr std::string asm_je{"je"};
    static constexpr std::string asm_jne{"jne"};
    static constexpr std::string asm_jl{"jl"};
    static constexpr std::string asm_jle{"jle"};
    static constexpr std::string asm_jg{"jg"};
    static constexpr std::string asm_jge{"jge"};

    std::vector<token> nots_;
    expr_ops_list lhs_;
    std::string op_; // '<', '<=', '>', '>=', '==', '!='
    expr_ops_list rhs_;
    bool is_not_{};       // e.g. if not a == b ...
    bool is_shorthand_{}; // e.g. if a ...
    bool is_expression_{};

  public:
    bool_op(toc& tc, tokenizer& tz) : statement{tz.next_whitespace_token()} {

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
            nots_.emplace_back(std::move(tk));
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

    bool_op() = default;
    bool_op(const bool_op&) = default;
    bool_op(bool_op&&) = default;
    auto operator=(const bool_op&) -> bool_op& = default;
    auto operator=(bool_op&&) -> bool_op& = default;

    ~bool_op() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        for (const token& e : nots_) {
            e.source_to(os);
        }

        lhs_.source_to(os);

        if (not is_shorthand_) {
            os << op_;
            rhs_.source_to(os);
        }
    }

    [[noreturn]] auto
    compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
            [[maybe_unused]] size_t indent,
            [[maybe_unused]] const std::string& dst = "") const
        -> void override {
        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    auto compile_or(toc& tc, std::ostream& os, size_t indent,
                    const std::string& jmp_to_if_true,
                    const bool inverted) const -> std::optional<bool> {
        const bool invert{inverted ? not is_not_ : is_not_};
        toc::indent(os, indent, true);
        tc.comment_source(os, "?", inverted ? " 'or' inverted: " : " ", *this);
        toc::asm_label(tok(), os, indent, cmp_bgn_label(tc));
        if (is_shorthand_) {
            // check case when operand is constant
            if (not lhs_.is_expression()) {
                // left-hand-side is not a expression, either a constant or an
                // identifier
                const ident_resolved& lhs_resolved{
                    tc.resolve_identifier(lhs_, false)};
                if (lhs_resolved.is_const()) {
                    bool const_eval{lhs_.get_unary_ops().evaluate_constant(
                                        lhs_resolved.const_value) != 0};
                    if (invert) {
                        const_eval = not const_eval;
                    }
                    toc::indent(os, indent, true);
                    os << "const eval to " << (const_eval ? "true" : "false")
                       << '\n';
                    if (const_eval) {
                        // since it is an 'or' chain short-circuit expression
                        // and jump to label for true
                        toc::indent(os, indent);
                        os << "jmp " << jmp_to_if_true << '\n';
                    }
                    return const_eval;
                }
            }
            resolve_cmp_shorthand(tc, os, indent, lhs_);
            toc::indent(os, indent);
            os << (not invert ? asm_jxx_for_op_inv("==")
                              : asm_jxx_for_op("=="));
            os << " " << jmp_to_if_true << '\n';
            return std::nullopt;
        }
        // check case when both operands are constants
        if (not lhs_.is_expression() and not rhs_.is_expression()) {
            const ident_resolved& lhs_resolved{
                tc.resolve_identifier(lhs_, false)};
            const ident_resolved& rhs_resolved{
                tc.resolve_identifier(rhs_, false)};
            if (lhs_resolved.is_const() and rhs_resolved.is_const()) {
                bool const_eval{
                    eval_constant(lhs_.get_unary_ops().evaluate_constant(
                                      lhs_resolved.const_value),
                                  op_,
                                  rhs_.get_unary_ops().evaluate_constant(
                                      rhs_resolved.const_value))};
                if (invert) {
                    const_eval = not const_eval;
                }
                toc::indent(os, indent, true);
                os << "const eval to " << (const_eval ? "true" : "false")
                   << '\n';
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
        os << " " << jmp_to_if_true << '\n';
        return std::nullopt;
    }

    auto compile_and(toc& tc, std::ostream& os, size_t indent,
                     const std::string& jmp_to_if_false,
                     const bool inverted) const -> std::optional<bool> {

        const bool invert{inverted ? not is_not_ : is_not_};
        toc::indent(os, indent, true);
        tc.comment_source(os, "?", inverted ? " 'and' inverted: " : " ", *this);
        toc::asm_label(tok(), os, indent, cmp_bgn_label(tc));
        if (is_shorthand_) {
            // check case when operand is constant
            if (not lhs_.is_expression()) {
                const ident_resolved& lhs_resolved{
                    tc.resolve_identifier(lhs_, false)};
                if (lhs_resolved.is_const()) {
                    bool const_eval{lhs_.get_unary_ops().evaluate_constant(
                                        lhs_resolved.const_value) != 0};
                    if (invert) {
                        const_eval = not const_eval;
                    }
                    toc::indent(os, indent, true);
                    os << "const eval to " << (const_eval ? "true" : "false")
                       << '\n';
                    if (not const_eval) {
                        // since it is an 'and' chain short-circuit expression
                        // and jump to label for false
                        toc::asm_jmp(lhs_.tok(), os, indent, jmp_to_if_false);
                    }
                    return const_eval;
                }
            }
            // left-hand-side is expression
            resolve_cmp_shorthand(tc, os, indent, lhs_);
            toc::indent(os, indent);
            os << (not invert ? asm_jxx_for_op("==")
                              : asm_jxx_for_op_inv("=="));
            os << " " << jmp_to_if_false << '\n';
            return std::nullopt;
        }
        // not shorthand expression
        // check the case when both operands are constants
        if (not lhs_.is_expression() and not rhs_.is_expression()) {
            const ident_resolved& lhs_resolved{
                tc.resolve_identifier(lhs_, false)};
            const ident_resolved& rhs_resolved{
                tc.resolve_identifier(rhs_, false)};
            if (lhs_resolved.is_const() and rhs_resolved.is_const()) {
                bool const_eval{
                    eval_constant(lhs_.get_unary_ops().evaluate_constant(
                                      lhs_resolved.const_value),
                                  op_,
                                  rhs_.get_unary_ops().evaluate_constant(
                                      rhs_resolved.const_value))};
                if (invert) {
                    const_eval = not const_eval;
                }
                toc::indent(os, indent, true);
                os << "const eval to " << (const_eval ? "true" : "false")
                   << '\n';
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
            const ident_resolved& lhs_resolved{
                tc.resolve_identifier(lhs_, false)};
            if (lhs_resolved.is_const()) {
                throw compiler_exception(
                    lhs_.tok(),
                    "left hand side expression may not be a constant");
            }
        }

        resolve_cmp(tc, os, indent, lhs_, rhs_);
        toc::indent(os, indent);
        os << (invert ? asm_jxx_for_op(op_) : asm_jxx_for_op_inv(op_));
        os << " " << jmp_to_if_false << '\n';
        return std::nullopt;
    }

    [[nodiscard]] auto cmp_bgn_label(const toc& tc) const -> std::string {
        const std::string& call_path{tc.get_inline_call_path(tok())};
        return "cmp_" + tc.source_location_for_use_in_label(tok()) +
               (call_path.empty() ? "" : "_" + call_path);
    }

    [[nodiscard]] auto identifier() const -> const std::string& override {
        assert(not is_expression_);
        return lhs_.identifier();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return is_expression_;
    }

  private:
    auto resolve_if_op_is_expression() -> void {
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
        const std::string& id{lhs_.identifier()};
        if (id == "true" or id == "false") {
            is_expression_ = false;
            return;
        }

        is_expression_ = true;
    }

    static auto eval_constant(const int64_t lh, const std::string& op,
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
        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    static auto asm_jxx_for_op(const std::string& op) -> const std::string& {
        if (op == "==") {
            return asm_je;
        }
        if (op == "!=") {
            return asm_jne;
        }
        if (op == "<") {
            return asm_jl;
        }
        if (op == "<=") {
            return asm_jle;
        }
        if (op == ">") {
            return asm_jg;
        }
        if (op == ">=") {
            return asm_jge;
        }

        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    static auto asm_jxx_for_op_inv(const std::string& op)
        -> const std::string& {
        if (op == "==") {
            return asm_jne;
        }
        if (op == "!=") {
            return asm_je;
        }
        if (op == "<") {
            return asm_jge;
        }
        if (op == "<=") {
            return asm_jg;
        }
        if (op == ">") {
            return asm_jle;
        }
        if (op == ">=") {
            return asm_jl;
        }
        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    auto resolve_cmp(toc& tc, std::ostream& os, size_t indent,
                     const expr_ops_list& lhs, const expr_ops_list& rhs) const
        -> void {

        std::vector<std::string> allocated_registers;

        const std::string& dst{
            resolve_expr(tc, os, indent, lhs, allocated_registers)};
        const std::string& src{
            resolve_expr(tc, os, indent, rhs, allocated_registers)};

        tc.asm_cmd(tok(), os, indent, "cmp", dst, src);

        // free allocated registers in reverse order
        for (auto it{allocated_registers.rbegin()};
             it != allocated_registers.rend(); ++it) {
            const std::string& reg{*it};
            tc.free_scratch_register(os, indent, reg);
        }
    }

    auto resolve_cmp_shorthand(toc& tc, std::ostream& os, size_t indent,
                               const expr_ops_list& lhs) const -> void {
        std::vector<std::string> allocated_registers;

        const std::string& dst{
            resolve_expr(tc, os, indent, lhs, allocated_registers)};

        tc.asm_cmd(tok(), os, indent, "cmp", dst, "0");

        // free allocated registers in reverse order
        for (auto it{allocated_registers.rbegin()};
             it != allocated_registers.rend(); ++it) {
            const std::string& reg{*it};
            tc.free_scratch_register(os, indent, reg);
        }
    }

    static auto resolve_expr(toc& tc, std::ostream& os, size_t indent,
                             const expr_ops_list& expr,
                             std::vector<std::string>& allocated_registers)
        -> std::string {

        if (expr.is_expression()) {
            const std::string& reg{
                tc.alloc_scratch_register(expr.tok(), os, indent)};
            allocated_registers.emplace_back(reg);
            expr.compile(tc, os, indent + 1, reg);
            return reg;
        }
        // expr is not an expression
        const ident_resolved& expr_resolved{tc.resolve_identifier(expr, true)};
        if (expr_resolved.is_const()) {
            return expr.get_unary_ops().to_string() + expr_resolved.id_nasm;
        }
        // expr not a constant, it is an identifier
        if (expr.get_unary_ops().is_empty()) {
            return expr_resolved.id_nasm;
        }
        // expr is not an expression and has unary ops
        const std::string& reg{
            tc.alloc_scratch_register(expr.tok(), os, indent)};
        allocated_registers.emplace_back(reg);
        tc.asm_cmd(expr.tok(), os, indent, "mov", reg, expr_resolved.id_nasm);
        expr.get_unary_ops().compile(tc, os, indent, reg);
        return reg;
    }
};
