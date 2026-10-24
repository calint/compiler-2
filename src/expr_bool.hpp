#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "constant_arith.hpp"
#include "expr_arith.hpp"
#include "expression.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "ub_order.hpp"
#include "ub_reads.hpp"

// what compiling a condition found out about its value without running it
enum class condition_result : uint8_t { unknown, always_false, always_true };

// where a condition continues, e.g. the code of an 'if' when it holds and the
// next branch when it does not; the same label for both when the value is
// only stored, e.g. 'b = x < 3'
struct branch_targets {
    std::string_view if_false;
    std::string_view if_true;
};

[[nodiscard]] inline auto known_result(const bool value) -> condition_result {
    return value ? condition_result::always_true
                 : condition_result::always_false;
}

[[nodiscard]] inline auto is_known(const condition_result result) -> bool {
    return result != condition_result::unknown;
}

// only for a known result
[[nodiscard]] inline auto result_value(const condition_result result) -> bool {
    assert(is_known(result));

    return result == condition_result::always_true;
}

// the 'not's that lead a condition, e.g. both of 'not not a == b'
[[nodiscard]] inline auto parse_not_tokens(tokenizer& tz)
    -> std::vector<token> {

    std::vector<token> nots;

    while (true) {
        const token tk{tz.next_token()};

        if (not tk.is_text("not")) {
            tz.put_back_token(tk);
            return nots;
        }

        nots.emplace_back(tk);
    }
}

// true when the 'not's make an odd count
[[nodiscard]] inline auto is_negated_by(const std::span<const token> nots)
    -> bool {

    return nots.size() % 2 == 1;
}

// one operand of an 'expr_bool': a comparison 'a < b', a comparison of the
// bytes of two instances or arrays, or the shorthand, the operand alone as
// the condition, e.g. 'if a'
class expr_bool_op final : public expression {
    // the operator and the right side, e.g. '== b' of 'a == b'
    struct comparison {
        token ws_pre_op;
        machine::comparison_operator op{machine::comparison_operator::equal};
        token ws_post_op;
        expr_arith rhs;
        // e.g. 'a == b' of two user type instances or two whole arrays, the
        // bytes are compared
        bool is_memory{};
    };

    // a side with what it folds to, folded once where it is compiled: a typed
    // constant is compared as a value of its type, e.g. 'i8(3)' or a constant
    // argument of an inlined call, an untyped one as written, e.g. '3' or
    // 'K + 1', and must fit the other side
    struct comparison_side {
        const expr_arith* node{};
        // empty when computed or read at run time
        std::optional<folded_value> folded;

        [[nodiscard]] auto is_typed_constant() const -> bool {
            return folded and folded->is_typed();
        }
    };

    // e.g. the 'not' of 'if not a == b ...'
    std::vector<token> nots_;
    expr_arith lhs_;
    // empty for a shorthand, the operand alone is the condition, e.g. 'if a'
    std::optional<comparison> comparison_;

  public:
    expr_bool_op(toc& tc, tokenizer& tz)
        : expression{tz.next_whitespace_token()} {

        set_type(tc.builtins().boolean());

        nots_ = parse_not_tokens(tz);

        lhs_ = {tc, tz};

        const token ws_pre_op{tz.next_whitespace_token()};

        const std::optional<machine::comparison_operator> op{
            parse_comparison_operator(tz),
        };

        // without an operator the operand is the condition, e.g. 'if a {...}'
        if (not op) {
            // note: the whitespace stays with what follows, so the operand
            //       alone reproduces its source, see 'release_name_or_literal'

            tz.put_back_token(ws_pre_op);
            return;
        }

        comparison cmp{
            .ws_pre_op{ws_pre_op},
            .op{*op},
            .ws_post_op{tz.next_whitespace_token()},
            .rhs{},
            .is_memory{},
        };

        cmp.rhs = {tc, tz};
        cmp.is_memory = classify_operands(tc, cmp);

        comparison_ = std::move(cmp);
    }

    expr_bool_op() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        for (const token& e : nots_) {
            e.source_to(os);
        }

        lhs_.source_to(os);

        if (not comparison_) {
            return;
        }

        comparison_->ws_pre_op.source_to(os);
        std::print(os, "{}", source_text(comparison_->op));
        comparison_->ws_post_op.source_to(os);
        comparison_->rhs.source_to(os);
    }

    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        lhs_.visit_call_effects(tc, visitor);

        if (comparison_) {
            comparison_->rhs.visit_call_effects(tc, visitor);
        }
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        lhs_.visit_reads(var, reader);

        if (comparison_) {
            comparison_->rhs.visit_reads(var, reader);
        }
    }

    //
    // class methods
    //

    // e.g. 'a + 1', null when negated or compared
    [[nodiscard]] auto arithmetic() const -> const expr_arith* {
        if (comparison_ or not nots_.empty()) {
            return nullptr;
        }

        return &lhs_;
    }

    // a stored 'bool' is 0 or 1 and a condition is a 'bool', an integer is
    // compared first, e.g. 'n != 0', and not with a 'bool', e.g. 'b == 1'
    auto assert_boolean_operands() const -> void {
        if (comparison_) {
            assert_not_boolean_with_integer(*comparison_);
            assert_boolean_equality_only(comparison_->op);
            return;
        }

        const type& lhs_type{lhs_.get_type()};

        // an instance is reported as a condition when it is compiled
        if (lhs_type.is_bool() or not lhs_type.is_builtin()) {
            return;
        }

        const std::string source{statement::trimmed_source(lhs_)};
        const std::string comparison{std::format("{} != 0", source)};

        throw compile_error{
            lhs_.tok(),
            std::format("{} of type {} is not a 'bool', compare it, e.g. {}",
                        in_quotes(source), in_quotes(lhs_type.name()),
                        in_quotes(comparison))};
    }

    [[nodiscard]] auto compile_and(toc& tc, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const bool inverted, const operand& dst,
                                   const bool branch_required) const
        -> condition_result {

        return compile_element(tc, indent, "and", inverted,
                               {
                                   .operation{operation()},
                                   .inverted{inverted != is_negated()},
                                   .destination{dst},
                                   .target{jmp_to_if_false},
                                   .branch_on_true{},
                               },
                               branch_required);
    }

    // the result is known when the expression evaluated at compile time
    [[nodiscard]] auto compile_or(toc& tc, const size_t indent,
                                  const std::string_view jmp_to_if_true,
                                  const bool inverted, const operand& dst) const
        -> condition_result {

        return compile_element(tc, indent, "or", inverted,
                               {
                                   .operation{operation()},
                                   .inverted{inverted != is_negated()},
                                   .destination{dst},
                                   .target{jmp_to_if_true},
                                   .branch_on_true{true},
                               },
                               true);
    }

    // the value 'compile' would find without emitting code, empty when
    // evaluated at run time
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<bool> {

        const comparison_side lhs{side_of(tc, lhs_)};

        if (not comparison_) {
            return constant_shorthand(lhs);
        }

        if (not lhs.folded) {
            return std::nullopt;
        }

        return constant_comparison(lhs, comparison_->op,
                                   side_of(tc, comparison_->rhs));
    }

    // takes the operand that 'arithmetic' points to
    [[nodiscard]] auto release_arithmetic() -> expr_arith {
        assert(arithmetic() != nullptr);

        return std::move(lhs_);
    }

  private:
    // e.g. 'a < b' of two 'bool's, which are not numbers
    auto
    assert_boolean_equality_only(const machine::comparison_operator op) const
        -> void {

        if (not lhs_.get_type().is_bool() or is_equality(op)) {
            return;
        }

        throw compile_error{lhs_.tok(),
                            "a 'bool' is compared only with '==' or '!='"};
    }

    // e.g. 'b == 1' or '2 == b'
    auto assert_not_boolean_with_integer(const comparison& cmp) const -> void {
        if (cmp.is_memory) {
            return;
        }

        const bool is_lhs_bool{lhs_.get_type().is_bool()};

        if (is_lhs_bool == cmp.rhs.get_type().is_bool()) {
            return;
        }

        const expr_arith& boolean{is_lhs_bool ? lhs_ : cmp.rhs};

        throw compile_error{boolean.tok(),
                            "a 'bool' is compared only with a 'bool'"};
    }

    // the result that 'compile_arrays_equal' or 'resolve_memory_cmp' stored in
    // 'action.destination' is branched on using 'action.branch_on_true'
    auto branch_on_stored_result(toc& tc, const size_t indent,
                                 const machine::comparison_action& action) const
        -> void {

        machine::comparison_action branch_action{action};

        // the stored result has the inversion applied
        branch_action.operation = machine::comparison_operator::not_equal;
        branch_action.destination = {};
        branch_action.inverted = false;

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, action.destination,
                             operand::imm(0, tc.builtins().default_type()),
                             branch_action);
    }

    // two instances or arrays are compared as memory, anything else as
    // numbers, e.g. 'p1 == p2' of instances, but 'p1 < p2' or 'p1 == 1' is
    // rejected; true for memory
    [[nodiscard]] auto classify_operands(const toc& tc,
                                         const comparison& cmp) const -> bool {

        const bool equality{is_equality(cmp.op)};

        const bool is_memory{
            equality and is_memory_operand(tc, lhs_) and
                is_memory_operand(tc, cmp.rhs),
        };

        if (equality) {
            assert_instance_stored(tc, lhs_);
            assert_instance_stored(tc, cmp.rhs);
        }

        if (not is_memory) {
            assert_not_record(lhs_, equality);
            assert_not_record(cmp.rhs, equality);
        }

        return is_memory;
    }

    // a shorthand is named since its source shows no comparison with 0
    [[nodiscard]] auto comment_label(const std::string_view list_op,
                                     const bool inverted) const -> std::string {

        if (inverted and not comparison_) {
            return std::format(" {} inverted shorthand: ", in_quotes(list_op));
        }

        if (inverted) {
            return std::format(" {} inverted: ", in_quotes(list_op));
        }

        if (not comparison_) {
            return " shorthand: ";
        }

        return " ";
    }

    // the sides are classified once, a stored result needs no branch to the
    // label that follows it ('stored_action')
    [[nodiscard]] auto
    compile_comparison(toc& tc, const size_t indent, const bool inverted,
                       const comparison& cmp,
                       const machine::comparison_action& action,
                       const machine::comparison_action& stored_action) const
        -> condition_result {

        const comparison_side lhs{side_of(tc, lhs_)};
        const comparison_side rhs{side_of(tc, cmp.rhs)};

        if (const std::optional<bool> constant{
                constant_comparison(lhs, cmp.op, rhs),
            }) {

            assert_folded_sides_fit(lhs, rhs, cmp.op);
            return compile_constant(tc, indent, *constant != inverted, action);
        }

        // e.g. the 'i' of 'i == h(i)' with 'h' writing 'i'
        const std::array<list_operand, 2> sides{
            list_operand{
                .node{&lhs_},
                .written_place{},
            },
            list_operand{
                .node{&cmp.rhs},
                .written_place{},
            },
        };

        assert_not_written_later(tc, sides, "operand");

        // a constant 'lhs' becomes an immediate 'rhs' instead of a scratch
        // register copy, the width check then applies to the swapped sides
        if (lhs.folded and not lhs.is_typed_constant()) {
            machine::comparison_action mirrored_action{action};
            mirrored_action.operation = mirrored(cmp.op);

            const comparison_side& mirrored_lhs{rhs};
            const comparison_side& mirrored_rhs{lhs};

            resolve_cmp(tc, indent, mirrored_lhs, mirrored_rhs,
                        mirrored_action);
            return condition_result::unknown;
        }

        if (not cmp.is_memory) {
            resolve_cmp(tc, indent, lhs, rhs, action);
            return condition_result::unknown;
        }

        resolve_memory_cmp(tc, indent, cmp, stored_action);
        return condition_result::unknown;
    }

    // a constant emits no comparison, only the short-circuit branch
    auto compile_constant(toc& tc, const size_t indent, const bool value,
                          const machine::comparison_action& action) const
        -> condition_result {

        machine& x{tc.machine()};

        x.comment(lhs_.tok(), indent, "const eval to {}",
                  value ? "true" : "false");

        // note: a run-time element before it may have stored the other value
        if (not action.destination.is_empty()) {
            x.store_boolean(tok(), indent, action.destination, value);
        }

        if (value == action.branch_on_true) {
            x.branch(indent, action.target);
        }

        return known_result(value);
    }

    // an 'or' element branches when true and an 'and' element when false
    [[nodiscard]] auto compile_element(toc& tc, const size_t indent,
                                       const std::string_view list_op,
                                       const bool inverted,
                                       const machine::comparison_action& action,
                                       const bool branch_required) const
        -> condition_result {

        machine& x{tc.machine()};

        x.comment(tok(), indent,
                  statement::trimmed_source(*this, "?",
                                            comment_label(list_op, inverted)));

        x.label(indent, tc.create_cmp_label(tok()));

        // a stored result needs no branch to the label that follows it
        machine::comparison_action stored_action{action};

        if (not branch_required) {
            stored_action.target = {};
        }

        if (not comparison_) {
            return compile_shorthand(tc, indent, inverted, action,
                                     stored_action);
        }

        return compile_comparison(tc, indent, inverted, *comparison_, action,
                                  stored_action);
    }

    // the operand alone, compared with 0
    [[nodiscard]] auto
    compile_shorthand(toc& tc, const size_t indent, const bool inverted,
                      const machine::comparison_action& action,
                      const machine::comparison_action& stored_action) const
        -> condition_result {

        const comparison_side condition{side_of(tc, lhs_)};

        if (const std::optional<bool> constant{constant_shorthand(condition)}) {
            return compile_constant(tc, indent, *constant != inverted, action);
        }

        assert_not_instance_condition(lhs_);
        resolve_cmp_shorthand(tc, indent, condition, stored_action);

        return condition_result::unknown;
    }

    // the value of a comparison whose sides fold, empty otherwise
    [[nodiscard]] auto constant_comparison(
        const comparison_side& lhs, const machine::comparison_operator op,
        const comparison_side& rhs) const -> std::optional<bool> {

        if (not lhs.folded or not rhs.folded) {
            return std::nullopt;
        }

        return constant_arith::eval_constant(lhs.folded->value, op,
                                             rhs.folded->value) != is_negated();
    }

    // the value of a shorthand whose operand folds, empty otherwise
    [[nodiscard]] auto
    constant_shorthand(const comparison_side& condition) const
        -> std::optional<bool> {

        if (not condition.folded) {
            return std::nullopt;
        }

        return (condition.folded->value != 0) != is_negated();
    }

    [[nodiscard]] auto is_negated() const -> bool {
        return is_negated_by(nots_);
    }

    // a shorthand is a comparison with 0 by '!='
    [[nodiscard]] auto operation() const -> machine::comparison_operator {
        return comparison_ ? comparison_->op
                           : machine::comparison_operator::not_equal;
    }

    auto resolve_cmp(toc& tc, const size_t indent, const comparison_side& lhs,
                     const comparison_side& rhs,
                     const machine::comparison_action& action) const -> void {

        std::vector<operand> allocated_registers;

        const operand dst{
            resolve_expr(tc, indent, lhs, true, allocated_registers),
        };

        const operand src{
            resolve_expr(tc, indent, rhs, false, allocated_registers),
        };

        assert_rhs_fits_lhs(lhs, rhs, action.operation, dst, src);

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, dst, src, action);
        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    auto resolve_cmp_shorthand(toc& tc, const size_t indent,
                               const comparison_side& condition,
                               const machine::comparison_action& action) const
        -> void {

        const expr_arith& lhs{*condition.node};

        if (lhs.is_arrays_equal_call() and not action.destination.is_empty()) {
            // note: only a 'bool' is assigned a boolean expression
            assert(lhs.get_type().is_same(action.destination.type_ref()));

            lhs.compile_arrays_equal(tc, indent + 1, action.destination,
                                     action.inverted);

            if (not action.target.empty()) {
                branch_on_stored_result(tc, indent, action);
            }

            return;
        }

        std::vector<operand> allocated_registers;

        // a plain 'bool' already holds 0 or 1, a copy is shorter than
        // evaluating the shorthand as '!= 0'
        if (is_bool_copy(lhs, action)) {
            machine& x{tc.machine()};

            const operand src{
                resolve_expr(tc, indent, condition, true, allocated_registers),
            };

            x.copy_value(tok(), indent, action.destination, src);
            x.free_scratch_registers(tok(), indent, allocated_registers);

            return;
        }

        const operand dst{
            truth_test_operand(tc, indent, condition, action,
                               allocated_registers),
        };

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, dst,
                             operand::imm(0, tc.builtins().default_type()),
                             action);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    // the bytes are compared and the result is branched on as for a shorthand
    // condition
    auto resolve_memory_cmp(toc& tc, const size_t indent, const comparison& cmp,
                            const machine::comparison_action& action) const
        -> void {

        const stmt_identifier& lhs{lhs_.identifier()};
        const stmt_identifier& rhs{cmp.rhs.identifier()};

        const bool is_not_equal{
            cmp.op == machine::comparison_operator::not_equal,
        };

        if (not action.destination.is_empty()) {
            compile_memory_equality(tc, indent + 1, tok(), lhs, rhs,
                                    action.destination,
                                    action.inverted != is_not_equal);

            if (not action.target.empty()) {
                branch_on_stored_result(tc, indent, action);
            }

            return;
        }

        machine& x{tc.machine()};

        const operand result{
            x.alloc_scratch_register(tok(), indent, tc.builtins().boolean()),
        };

        compile_memory_equality(tc, indent + 1, tok(), lhs, rhs, result,
                                is_not_equal);

        machine::comparison_action branch_action{action};
        branch_action.operation = machine::comparison_operator::not_equal;

        x.compare_and_branch(tok(), indent, result,
                             operand::imm(0, tc.builtins().default_type()),
                             branch_action);

        x.free_scratch_register(tok(), indent, result);
    }

    //
    // statics
    //

    // 'constant' is compared at the width of 'typed', e.g. 1000 with an 'i8'
    static auto assert_constant_fits(const expr_arith& typed,
                                     const type& typed_type,
                                     const comparison_side& constant) -> void {

        assert(constant.folded);

        const int64_t value{constant.folded->value};

        if (constant_arith::fits_width(value, typed_type)) {
            return;
        }

        throw compile_error{
            constant.node->tok(),
            std::format("constant {} does not fit {} of type {}",
                        in_quotes(std::format("{}", value)),
                        in_quotes(statement::trimmed_source(typed)),
                        in_quotes(typed_type.name()))};
    }

    // a comparison that folds is checked like one that is compiled
    static auto assert_folded_sides_fit(const comparison_side& lhs,
                                        const comparison_side& rhs,
                                        const machine::comparison_operator op)
        -> void {

        const bool is_lhs_typed{lhs.is_typed_constant()};
        const bool is_rhs_typed{rhs.is_typed_constant()};

        if (is_lhs_typed and not is_rhs_typed) {
            assert_constant_fits(*lhs.node, lhs.node->get_type(), rhs);
            return;
        }

        if (is_rhs_typed and not is_lhs_typed) {
            assert_constant_fits(*rhs.node, rhs.node->get_type(), lhs);
            return;
        }

        if (not is_rhs_typed) {
            return;
        }

        const type& lhs_type{lhs.node->get_type()};
        const type& rhs_type{rhs.node->get_type()};

        if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
            return;
        }

        throw_rhs_wider(*lhs.node, *rhs.node, op, lhs_type, rhs_type);
    }

    // an operator would compare a user type instance as its first field only
    // e.g. the 'mk()' of 'p == mk()', an instance is compared as memory,
    // which only a variable has
    static auto assert_instance_stored(const toc& tc, const expr_arith& side)
        -> void {

        if (side.get_type().is_builtin() or side.operand_path() != nullptr or
            is_memory_operand(tc, side)) {

            return;
        }

        throw compile_error{
            side.tok(),
            std::format("store {} in a variable before comparing "
                        "it",
                        in_quotes(statement::trimmed_source(side)))};
    }

    // a condition tests a number, an instance would be tested as its first
    // field; checked when compiling since the initializer of a 'var' parses
    // its expression to learn the type, e.g. 'var b = a' copies the instance
    static auto assert_not_instance_condition(const expr_arith& side) -> void {
        if (side.get_type().is_builtin()) {
            return;
        }

        throw compile_error{side.tok(),
                            std::format("an instance of {} is not a condition",
                                        in_quotes(side.get_type().name()))};
    }

    static auto assert_not_record(const expr_arith& side,
                                  const bool is_equality) -> void {

        if (side.get_type().is_builtin()) {
            return;
        }

        const std::string_view type_name{side.get_type().name()};

        // e.g. the '-' of '-p == p' applies to a number
        const stmt_identifier* const path{side.operand_path()};

        const bool has_unary_ops{
            path != nullptr and not path->get_unary_ops().is_empty(),
        };

        if (is_equality and not has_unary_ops) {
            throw compile_error{
                side.tok(),
                std::format("an instance of {} is compared only with an "
                            "instance of the same type",
                            in_quotes(type_name))};
        }

        throw compile_error{side.tok(),
                            std::format("an instance of {} is not a number",
                                        in_quotes(type_name))};
    }

    // the backends compare at the width of 'lhs' which would truncate 'rhs'
    static auto assert_rhs_fits_lhs(const comparison_side& lhs,
                                    const comparison_side& rhs,
                                    const machine::comparison_operator op,
                                    const operand& lhs_op,
                                    const operand& rhs_op) -> void {

        const type& lhs_type{lhs_op.type_ref()};

        // a typed constant, e.g. an argument of an inlined call, is compared
        // as a variable of its type
        if (not rhs_op.is_immediate() or rhs.is_typed_constant()) {
            const type& rhs_type{rhs_op.type_ref()};

            if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
                return;
            }

            throw_rhs_wider(*lhs.node, *rhs.node, op, lhs_type, rhs_type);
        }

        assert_constant_fits(*lhs.node, lhs_type, rhs);
    }

    // the bytes that both sides hold, they hold the same type and, as arrays,
    // the same number of elements; empty when an array length is not known
    [[nodiscard]] static auto
    compared_size_bytes(const stmt_identifier& lhs, const stmt_identifier& rhs,
                        const ident_info& lhs_info, const ident_info& rhs_info)
        -> std::optional<size_t> {

        if (not lhs_info.type_ref().is_same(rhs_info.type_ref())) {
            throw compile_error{
                rhs.tok(), std::format("cannot compare type {} with type {}",
                                       in_quotes(lhs_info.type_ref().name()),
                                       in_quotes(rhs_info.type_ref().name()))};
        }

        // a whole array is not compared as its first element
        if (lhs_info.is_array() != rhs_info.is_array()) {
            assert_not_whole_array(lhs, lhs_info);
            assert_not_whole_array(rhs, rhs_info);
        }

        const size_t element_size_bytes{lhs_info.type_ref().size_bytes()};

        if (not lhs_info.is_array()) {
            return element_size_bytes;
        }

        const std::optional<size_t> lhs_len{lhs_info.array_len()};
        const std::optional<size_t> rhs_len{rhs_info.array_len()};

        if (not lhs_len or not rhs_len) {
            return std::nullopt;
        }

        // arrays are compared element by element, so their lengths must agree
        // note: a whole array on one side only was rejected above
        if (*lhs_len != *rhs_len) {
            throw compile_error{
                rhs.tok(),
                std::format("cannot compare arrays of sizes {} and {}",
                            *lhs_len, *rhs_len)};
        }

        return multiply_storage_size(lhs.tok(), element_size_bytes, *lhs_len);
    }

    // compares the bytes of two user type instances or of two whole arrays,
    // 'dst' is 1 when they are equal, negated when 'inverted'
    static auto compile_memory_equality(toc& tc, const size_t indent,
                                        const token& src_loc_tk,
                                        const stmt_identifier& lhs,
                                        const stmt_identifier& rhs,
                                        const operand& dst, const bool inverted)
        -> void {

        const ident_info lhs_info{make_memory_operand_info(tc, lhs)};
        const ident_info rhs_info{make_memory_operand_info(tc, rhs)};

        const std::optional<size_t> size_bytes{
            compared_size_bytes(lhs, rhs, lhs_info, rhs_info),
        };

        // nothing to compare, the code of a function checked without a call
        // is discarded
        if (not size_bytes) {
            return;
        }

        machine& x{tc.machine()};

        x.memory_equal(
            src_loc_tk, indent, *size_bytes,
            {
                .alignment{lhs_info.type_ref().alignment()},
                .lhs{lhs.address_emitter_of(tc, indent, src_loc_tk, lhs_info)},
                .rhs{rhs.address_emitter_of(tc, indent, src_loc_tk, rhs_info)},
                .dst{dst},
                .inverted{inverted},
            });
    }

    [[nodiscard]] static auto
    compile_to_scratch(toc& tc, const size_t indent, const expr_arith& expr,
                       std::vector<operand>& allocated_registers) -> operand {

        machine& x{tc.machine()};

        // the operand is computed at the width of its first element
        expr.assert_not_narrowed(tc, expr.get_type());

        const operand reg{
            x.alloc_scratch_register(expr.tok(), indent, expr.get_type()),
        };

        allocated_registers.emplace_back(reg);
        expr.compile(tc, indent + 1, destination::of_register(reg));

        return reg;
    }

    // a stored 'bool' is 0 or 1 so a plain one needs no comparison with 0,
    // inversion and branches keep the comparison which is shorter on x86
    [[nodiscard]] static auto
    is_bool_copy(const expr_arith& lhs,
                 const machine::comparison_action& action) -> bool {

        // note: a shorthand without a branch target stores into a 'bool'
        assert(not action.target.empty() or
               (not action.destination.is_empty() and
                action.destination.type_ref().is_bool()));

        // note: a value that is no expression is one name or literal
        return not lhs.is_expression() and
               lhs.operand_path()->get_unary_ops().is_empty() and
               lhs.get_type().is_bool() and not action.inverted and
               action.target.empty();
    }

    // a user type instance or a whole array in storage, compared in place
    [[nodiscard]] static auto is_equality(const machine::comparison_operator op)
        -> bool {

        return op == machine::comparison_operator::equal or
               op == machine::comparison_operator::not_equal;
    }

    [[nodiscard]] static auto is_memory_operand(const toc& tc,
                                                const expr_arith& side)
        -> bool {

        const std::optional<place> stored{as_place(tc, side)};

        return stored and (stored->info.is_array() or
                           not stored->info.type_ref().is_builtin());
    }

    // a typed constant side is compared as a variable of its type
    // memory is compared, so a constant has nothing to compare
    [[nodiscard]] static auto
    make_memory_operand_info(const toc& tc, const stmt_identifier& identifier)
        -> ident_info {

        ident_info info{make_ident_info(tc, identifier)};

        // note: 'is_memory_operand' rejected constants
        assert(not info.is_const());

        return info;
    }

    // the comparison at the next characters, none for a shorthand condition
    [[nodiscard]] static auto parse_comparison_operator(tokenizer& tz)
        -> std::optional<machine::comparison_operator> {

        if (tz.is_next_char('=')) {
            if (not tz.is_next_char('=')) {
                // the error is at the '=' that is alone
                tz.put_back_char('=');

                throw compile_error{tz, "expected '=='"};
            }

            return machine::comparison_operator::equal;
        }

        if (tz.is_next_char('!')) {
            if (not tz.is_next_char('=')) {
                // the error is at the '!' that is alone
                tz.put_back_char('!');

                throw compile_error{tz, "expected '!='"};
            }

            return machine::comparison_operator::not_equal;
        }

        if (tz.is_next_char('<')) {
            return tz.is_next_char('=')
                       ? machine::comparison_operator::less_equal
                       : machine::comparison_operator::less;
        }

        if (tz.is_next_char('>')) {
            return tz.is_next_char('=')
                       ? machine::comparison_operator::greater_equal
                       : machine::comparison_operator::greater;
        }

        return std::nullopt;
    }

    [[nodiscard]] static auto
    resolve_expr(toc& tc, const size_t indent, const comparison_side& side,
                 const bool is_lhs, std::vector<operand>& allocated_registers)
        -> operand {

        const expr_arith& expr{*side.node};

        // a computed value, or the zero value '{}', which has no name
        const stmt_identifier* const path{expr.operand_path()};

        // the comparison needs its left operand in a register, 'compile'
        // folds a constant list into one copy
        if (path == nullptr and is_lhs) {
            return compile_to_scratch(tc, indent, expr, allocated_registers);
        }

        if (path == nullptr) {
            const std::optional<folded_value>& folded{side.folded};

            if (not folded) {
                return compile_to_scratch(tc, indent, expr,
                                          allocated_registers);
            }

            machine& x{tc.machine()};

            x.comment(expr.tok(), indent, "src: folded constant {}",
                      in_quotes(statement::trimmed_source(expr)));

            // e.g. 'i16(300) + 1' is a value of type 'i16'
            if (folded->is_typed()) {
                return operand::imm(folded->value, *folded->own_type);
            }

            return operand::imm(folded->value, tc.builtins().default_type());
        }

        const ident_info expr_info{make_scalar_ident_info(tc, *path)};

        // a typed constant left side is a value of its type in a register
        if (is_lhs and side.is_typed_constant()) {
            machine& x{tc.machine()};

            const operand reg{
                x.alloc_scratch_register(expr.tok(), indent, expr.get_type()),
            };

            allocated_registers.emplace_back(reg);

            x.copy_value(expr.tok(), indent, reg, typed_constant(side));

            return reg;
        }

        // a constant left side was mirrored onto the right
        assert(not expr_info.is_const() or not is_lhs);

        if (side.is_typed_constant()) {
            return typed_constant(side);
        }

        // an untyped constant is compared as written, the caller checks that
        // it fits the left side
        if (expr_info.is_const()) {
            // note: a constant name or literal folds
            assert(side.folded);

            return operand::imm(side.folded->value, expr_info.type_ref());
        }

        // e.g. 'a', '-a[i]'
        return path->compile_value_operand(
            tc, indent, expr.tok(), expr_info, expr_info.type_ref(),
            tc.checks().overflow_options(), allocated_registers);
    }

    // a side computed at run time in a register of its own type, so a
    // constant side folds at that width and is checked there, e.g. the '300'
    // of 'e < i8(1) + 300' does not fit 'i8'
    [[nodiscard]] static auto side_of(const toc& tc, const expr_arith& node)
        -> comparison_side {

        return {
            .node{&node},
            .folded{node.fold_checked(tc, tc.use_at(node.get_type()))},
        };
    }

    [[noreturn]] static auto
    throw_rhs_wider(const expr_arith& lhs, const expr_arith& rhs,
                    const machine::comparison_operator op, const type& lhs_type,
                    const type& rhs_type) -> void {

        throw compile_error{
            rhs.tok(), std::format("{} of type {} is wider than {} of type {}, "
                                   "swap the operands: '{} {} {}'",
                                   in_quotes(statement::trimmed_source(rhs)),
                                   in_quotes(rhs_type.name()),
                                   in_quotes(statement::trimmed_source(lhs)),
                                   in_quotes(lhs_type.name()),
                                   statement::trimmed_source(rhs),
                                   source_text(mirrored(op)),
                                   statement::trimmed_source(lhs))};
    }

    // the value compared with 0, matching types avoid narrowing before the
    // in-place truth test
    [[nodiscard]] static auto
    truth_test_operand(toc& tc, const size_t indent,
                       const comparison_side& condition,
                       const machine::comparison_action& action,
                       std::vector<operand>& allocated_registers) -> operand {

        const expr_arith& lhs{*condition.node};

        if (lhs.is_expression() and action.destination.is_register() and
            lhs.get_type().is_same(action.destination.type_ref())) {

            lhs.compile_checked(tc, indent + 1,
                                destination::of_register(action.destination));

            return action.destination;
        }

        return resolve_expr(tc, indent, condition, true, allocated_registers);
    }

    // the value at the width of its type, e.g. -128 for '-i8(-128)'
    [[nodiscard]] static auto typed_constant(const comparison_side& side)
        -> operand {

        assert(side.folded and side.folded->is_typed());

        return operand::imm(side.folded->value, *side.folded->own_type);
    }
};

// a list of conditions and nested lists joined by 'and' or by 'or', instead
// of a tree
// note: a list branches at the first operand that decides it, which is the
//       short-circuit of 'and' and 'or'
class expr_bool final : public expression {
    using element = std::variant<expr_bool_op, expr_bool>;

    std::vector<element> bools_;
    // the 'and' or 'or' between the elements of 'bools_'
    std::vector<token> ops_;
    // the 'not's before the '(' of a parenthesized list, e.g. both of
    // 'not not (a == b and c == d)'
    std::vector<token> nots_;
    // empty unless the list is parenthesized
    token open_paren_tk_;
    token close_paren_tk_;

  public:
    expr_bool(toc& tc, const token tk, tokenizer& tz) : expression{tk} {
        parse_list(tc, tz);
    }

    // e.g. '(a == b and c == d)' after its 'not's, the '(' has been read
    expr_bool(toc& tc, const token tk, tokenizer& tz, std::vector<token> nots,
              const token open_paren_tk)
        : expression{tk}, nots_{std::move(nots)},
          open_paren_tk_{open_paren_tk} {

        assert(not open_paren_tk_.is_empty());

        parse_list(tc, tz);
        assert_parentheses_group();
    }

    expr_bool() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        for (const token& not_tk : nots_) {
            not_tk.source_to(os);
        }

        open_paren_tk_.source_to(os);

        const size_t element_count{bools_.size()};
        for (size_t i{}; i < element_count; ++i) {
            bools_.at(i).visit(
                [&os](const auto& e) -> void { e.source_to(os); });

            if (i < element_count - 1) {
                // note: -1 because there is one operator fewer than elements

                ops_.at(i).source_to(os);
            }
        }

        close_paren_tk_.source_to(os);
    }

    // the value of the list stored in 'dst', e.g. 'b = x < 3 and y'
    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        // stored comparisons would change what later elements read; a
        // register that the list reads is the result of an inlined function
        if (not reaches_destination(tc, *this, dst)) {
            compile_value(tc, indent, dst.operand());
            return;
        }

        const operand reg{
            x.alloc_scratch_register(tok(), indent, dst.type_ref()),
        };

        compile_value(tc, indent, reg);
        x.copy_value(tok(), indent, dst.operand(), reg);
        x.free_scratch_register(tok(), indent, reg);
    }

    [[nodiscard]] auto compile(toc& tc, const size_t indent,
                               const branch_targets& targets,
                               const operand& dst) const -> condition_result {

        return compile_rec(tc, indent, targets, false, dst);
    }

    // a condition known at compile time is 1 or 0
    [[nodiscard]] auto fold(const toc& tc,
                            [[maybe_unused]] const value_use& use) const
        -> std::optional<folded_value> override {

        const std::optional<bool> value{constant_value(tc)};

        if (not value) {
            return std::nullopt;
        }

        return folded_value{
            .value{*value ? 1 : 0},
            .own_type{},
        };
    }

    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        for (const element& e : bools_) {
            e.visit([&tc, &visitor](const auto& item) -> void {
                item.visit_call_effects(tc, visitor);
            });
        }
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        for (const element& e : bools_) {
            e.visit([&var, &reader](const auto& item) -> void {
                item.visit_reads(var, reader);
            });
        }
    }

    //
    // class methods
    //

    // e.g. 'a + 1' or '(a + 1)', null when the list negates, compares, or
    // combines with 'and' or 'or'
    [[nodiscard]] auto arithmetic() const -> const expr_arith* {
        if (bools_.size() != 1 or has_not()) {
            return nullptr;
        }

        return bools_.front().visit(
            [](const auto& e) -> const expr_arith* { return e.arithmetic(); });
    }

    // the operands of a condition or of a stored 'bool', see the elements
    auto assert_boolean_operands() const -> void {
        for (const element& e : bools_) {
            e.visit([](const auto& item) -> void {
                item.assert_boolean_operands();
            });
        }
    }

    // the value the parse can tell, the part of what 'compile' finds that
    // needs no code: constants up to the one that decides the list; after a
    // run-time element 'compile' may still decide it, see
    // 'resolve_last_constant'
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<bool> {

        const bool invert{is_negated()};

        for (const element& e : bools_) {
            const std::optional<bool> value{
                e.visit([&tc](const auto& item) -> std::optional<bool> {
                    return item.constant_value(tc);
                }),
            };

            if (not value) {
                return std::nullopt;
            }

            if (is_short_circuit(*value, false)) {
                return *value != invert;
            }
        }

        // note: no element decided, so each is 'false' in an 'or' list and
        //       'true' in an 'and' list

        return (not is_or_list()) != invert;
    }

    // the operand when the whole expression is a name or a literal, e.g. 'x'
    // but not 'not x', else none and the expression is unchanged
    [[nodiscard]] auto release_name_or_literal() -> std::optional<expr_arith> {
        if (bools_.size() != 1 or has_not()) {
            return std::nullopt;
        }

        expr_bool_op* const op{std::get_if<expr_bool_op>(&bools_.front())};

        if (op == nullptr) {
            return std::nullopt;
        }

        const expr_arith* const arith{op->arithmetic()};

        if (arith == nullptr or arith->operand_path() == nullptr) {

            return std::nullopt;
        }

        return op->release_arithmetic();
    }

  private:
    // parentheses group a connective, a comparison or a 'not', around one
    // plain condition they mean nothing, e.g. '(flag)' is 'flag' and
    // '((a or b))' is '(a or b)'
    auto assert_parentheses_group() const -> void {
        if (bools_.size() != 1) {
            return;
        }

        const element& only{bools_.front()};

        if (const expr_bool_op* const op{std::get_if<expr_bool_op>(&only)}) {
            const expr_arith* const arith{op->arithmetic()};

            if (arith == nullptr or not arith->is_one_plain_element()) {
                return;
            }
        } else if (std::get<expr_bool>(only).has_not()) {
            return;
        }

        const statement& value{
            *only.visit(
                [](const statement& e) -> const statement* { return &e; }),
        };

        throw compile_error{
            open_paren_tk_,
            std::format("parentheses around a single value group nothing, "
                        "write {}",
                        in_quotes(statement::trimmed_source(value)))};
    }

    // e.g. 'a or b and c' is rejected, '(a or b) and c' is not
    auto assert_same_connective(const token& op_tk) const -> void {
        if (ops_.empty() or ops_.front().is_text(op_tk.text())) {
            return;
        }

        throw compile_error{op_tk, "mixing 'and' and 'or' without parentheses"};
    }

    // an element that does not decide the list continues at the next element
    [[nodiscard]] auto
    compile_inner_element(toc& tc, const size_t indent, const size_t expr_index,
                          const branch_targets& targets, const bool invert,
                          const operand& dst) const -> condition_result {

        const bool is_or{is_or_list() != invert};

        if (std::holds_alternative<expr_bool>(bools_.at(expr_index))) {
            const expr_bool& nested_expr{
                std::get<expr_bool>(bools_.at(expr_index)),
            };

            const std::string next_label{
                bools_.at(expr_index + 1)
                    .visit([&tc](const auto& next) -> std::string {
                        return tc.create_cmp_label(next.tok());
                    }),
            };

            // an 'or' continues when false and an 'and' continues when true
            const branch_targets nested_targets{
                .if_false{
                    is_or ? std::string_view{next_label} : targets.if_false,
                },
                .if_true{
                    is_or ? targets.if_true : std::string_view{next_label},
                },
            };

            return nested_expr.compile_with_label(tc, indent, nested_targets,
                                                  invert, dst);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.at(expr_index))};

        if (is_or) {
            return expr.compile_or(tc, indent, targets.if_true, invert, dst);
        }

        return expr.compile_and(tc, indent, targets.if_false, invert, dst,
                                true);
    }

    [[nodiscard]] auto compile_last_element(
        toc& tc, const size_t indent, const branch_targets& targets,
        const bool invert, const operand& dst,
        const bool has_runtime_element) const -> condition_result {

        if (std::holds_alternative<expr_bool>(bools_.back())) {
            const expr_bool& nested_expr{std::get<expr_bool>(bools_.back())};

            const condition_result const_eval{
                nested_expr.compile_with_label(tc, indent, targets, invert,
                                               dst),
            };

            // the nested list already emitted its final branch
            if (not is_known(const_eval)) {
                return condition_result::unknown;
            }

            return resolve_last_constant(tc, indent, result_value(const_eval),
                                         targets.if_true, invert,
                                         has_runtime_element);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.back())};

        const condition_result const_eval{
            expr.compile_and(tc, indent, targets.if_false, invert, dst,
                             targets.if_false != targets.if_true),
        };

        if (is_known(const_eval)) {
            return resolve_last_constant(tc, indent, result_value(const_eval),
                                         targets.if_true, invert,
                                         has_runtime_element);
        }

        machine& x{tc.machine()};

        // every element that failed has branched to false
        x.branch(indent, targets.if_true);

        return condition_result::unknown;
    }

    [[nodiscard]] auto
    compile_rec(toc& tc, const size_t indent, const branch_targets& targets,
                const bool inverted, const operand& dst) const
        -> condition_result {

        // the element of a list that is only that element comments the same
        // source and also how it is joined and inverted
        const bool is_only_element{
            bools_.size() == 1 and not has_not() and not is_enclosed(),
        };

        if (not is_only_element) {
            machine& x{tc.machine()};

            x.comment(tok(), indent,
                      statement::trimmed_source(
                          *this, "?", inverted ? " inverted: " : " "));
        }

        // invert, according to De Morgan's laws
        const bool invert{inverted != is_negated()};

        bool has_runtime_element{};

        const size_t last_index{bools_.size() - 1};

        for (size_t expr_index{}; expr_index < last_index; ++expr_index) {
            const condition_result const_eval{
                compile_inner_element(tc, indent, expr_index, targets, invert,
                                      dst),
            };

            if (not is_known(const_eval)) {
                has_runtime_element = true;
                continue;
            }

            if (is_short_circuit(result_value(const_eval), invert)) {

                return const_eval;
            }
        }

        return compile_last_element(tc, indent, targets, invert, dst,
                                    has_runtime_element);
    }

    // the condition stores its value in 'dst', so both outcomes continue at
    // the same label
    // note: the label is named after the token of this list so that two
    //       conditions of one initializer get distinct labels
    auto compile_value(toc& tc, const size_t indent, const operand& dst) const
        -> void {

        const std::string jmp_to_end{
            toc::end_label(tc.create_unique_label(tok(), "bool")),
        };

        // note: every element stores its value, a known result included
        std::ignore = compile(tc, indent,
                              {
                                  .if_false{jmp_to_end},
                                  .if_true{jmp_to_end},
                              },
                              dst);

        machine& x{tc.machine()};

        x.label(indent, jmp_to_end);
    }

    // earlier elements jump to the label that begins a nested list
    [[nodiscard]] auto compile_with_label(toc& tc, const size_t indent,
                                          const branch_targets& targets,
                                          const bool inverted,
                                          const operand& dst) const
        -> condition_result {

        machine& x{tc.machine()};

        x.label(indent, tc.create_cmp_label(tok()));

        return compile_rec(tc, indent, targets, inverted, dst);
    }

    // true when a 'not' leads the list
    [[nodiscard]] auto has_not() const -> bool { return not nots_.empty(); }

    [[nodiscard]] auto is_enclosed() const -> bool {
        return not open_paren_tk_.is_empty();
    }

    [[nodiscard]] auto is_negated() const -> bool {
        return is_negated_by(nots_);
    }

    // inversion swaps 'and' and 'or' according to De Morgan's laws
    // every operator of a list is the same, mixing them is rejected
    [[nodiscard]] auto is_or_list() const -> bool {
        return not ops_.empty() and ops_.front().is_text("or");
    }

    // a constant false ends an 'and' list and a constant true ends an 'or'
    // list; 'invert' turns one into the other, see De Morgan's laws
    [[nodiscard]] auto is_short_circuit(const bool const_eval,
                                        const bool invert) const -> bool {

        return const_eval == (is_or_list() != invert);
    }

    // a comparison or a parenthesized list, either may follow 'not'
    auto parse_element(toc& tc, tokenizer& tz) -> void {
        // a comparison is parsed from before the 'not's, it reads them itself
        const token rewind_pos_tk{tz.cur_position_token()};

        // the 'not's before a parenthesis, e.g. 'not not (a == b)'
        std::vector<token> not_tks{parse_not_tokens(tz)};

        // the list is located at its '(', which keeps the whitespace before it
        const token ws_tk{tz.next_whitespace_token()};
        const token pos_tk{tz.cur_position_token()};
        tz.put_back_token(ws_tk);

        const token open_paren_tk{tz.is_next_char_token('(')};

        // e.g. '(t1 + t2) > 3' is a comparison with a parenthesized operand
        if (open_paren_tk.is_empty() or is_parenthesized_operand(tz)) {
            tz.rewind_to_position(rewind_pos_tk);
            bools_.emplace_back(std::in_place_type<expr_bool_op>, tc, tz);
            return;
        }

        bools_.emplace_back(std::in_place_type<expr_bool>, tc, pos_tk, tz,
                            std::move(not_tks), open_paren_tk);
    }

    // the operands are joined by 'and' or by 'or', not by both, e.g.
    // 'a == 1 and b == 2 and c == 3'
    auto parse_list(toc& tc, tokenizer& tz) -> void {
        const toc::parse_nesting_scope nesting{tc, tz};

        set_type(tc.builtins().boolean());

        parse_element(tc, tz);

        while (const std::optional<token> op_tk{read_connective(tz)}) {
            assert_same_connective(*op_tk);

            ops_.emplace_back(*op_tk);

            parse_element(tc, tz);
        }
    }

    // the 'and' or 'or' after an operand, none when the expression ends: an
    // enclosed expression ends with its ')'
    [[nodiscard]] auto read_connective(tokenizer& tz) -> std::optional<token> {
        // e.g. '(a == 1 or b == 2)'
        if (is_enclosed()) {
            close_paren_tk_ = tz.is_next_char_token(')');

            if (not close_paren_tk_.is_empty()) {
                return std::nullopt;
            }
        }

        // the connective after an operand, e.g. the 'and' of
        // 'a == 1 and b == 2'
        const token op_tk{tz.next_token()};

        if (op_tk.is_text("or") or op_tk.is_text("and")) {
            return op_tk;
        }

        // anything else ends the expression, an enclosed one needs its ')'
        // first
        // e.g. the 'exit' of 'if a == 1 exit(1)', or of '(a == 1 exit(1)'
        // which lacks its ')'
        if (is_enclosed()) {
            throw compile_error{op_tk, "expected ')' to close expression"};
        }

        tz.put_back_token(op_tk);

        return std::nullopt;
    }

    // a constant last element decides the list only if all earlier elements
    // were constants or it short-circuits the list
    [[nodiscard]] auto resolve_last_constant(
        toc& tc, const size_t indent, const bool const_eval,
        const std::string_view jmp_to_if_true, const bool invert,
        const bool has_runtime_element) const -> condition_result {

        if (not has_runtime_element) {
            return known_result(const_eval);
        }

        if (is_short_circuit(const_eval, invert)) {
            return known_result(const_eval);
        }

        // a constant false has already branched to false
        if (not const_eval) {
            return condition_result::unknown;
        }

        machine& x{tc.machine()};

        // a constant true emits no branch of its own
        x.branch(indent, jmp_to_if_true);

        return condition_result::unknown;
    }

    //
    // statics
    //

    // an operator after the ')' that closes the '(' just read makes the
    // parentheses part of an operand, decided on a copy before anything is
    // parsed
    [[nodiscard]] static auto is_parenthesized_operand(const tokenizer& tz)
        -> bool {

        tokenizer after_paren{tz};
        after_paren.skip_to_close_paren();

        return std::string_view{"<>=!+-*/%&|^"}.contains(
            after_paren.peek_char_after_whitespace());
    }
};
