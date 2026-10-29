#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "constant_arith.hpp"
#include "expr_arith.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_order.hpp"
#include "ub_reads.hpp"

//
// a condition, e.g. 'a < b and not c', is the types below, in reading order:
//
//   'condition_result', 'branch_targets'
//                            what a compiled condition found out and where it
//                            continues
//   'condition_nots'         the leading 'not's
//   'expr_bool_op'           one comparison or the shorthand, an operand alone
//   'expr_bool'              a list joined by 'and' or by 'or', its elements
//                            comparisons or parenthesized lists
//

// what compiling a condition found out about its value without running it
enum class condition_result : uint8_t { unknown, always_false, always_true };

// where a condition continues, e.g. the code of an 'if' when it holds and the
// next branch when it does not; the same label for both when the value is
// only stored, e.g. 'b = x < 3'
struct branch_targets {
    std::string_view if_false;
    std::string_view if_true;

    // both outcomes continue at the label that follows the condition, so its
    // last element branches nowhere
    [[nodiscard]] auto is_stored_only() const -> bool {
        return if_false == if_true;
    }
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

// the 'not's that lead a condition, e.g. both of 'not not a == b', like
// 'unary_ops' of a value; each is a token with the whitespace before it
class condition_nots final {
    std::vector<token> nots_;

  public:
    explicit condition_nots(tokenizer& tz) {
        while (true) {
            const token tk{tz.next_token()};

            if (not tk.is_text("not")) {
                tz.put_back_token(tk);
                return;
            }

            nots_.emplace_back(tk);
        }
    }

    condition_nots() = default;

    [[nodiscard]] auto is_empty() const -> bool { return nots_.empty(); }

    // an odd count negates
    [[nodiscard]] auto is_negated() const -> bool {
        return nots_.size() % 2 == 1;
    }

    auto source_to(std::ostream& os) const -> void {
        for (const token& not_tk : nots_) {
            not_tk.source_to(os);
        }
    }
};

// one operand of an 'expr_bool': a comparison 'a < b', a comparison of the
// bytes of two instances or arrays, or the shorthand, the operand alone as
// the condition, e.g. 'if a'
class expr_bool_op final : public statement {
    // the operator and the right side, e.g. '== b' of 'a == b'
    struct comparison {
        // the operator with the whitespace before it, 'op' is what it spells
        token op_tk;
        comparison_operator op{comparison_operator::equal};
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
    condition_nots nots_;
    expr_arith lhs_;
    // empty for a shorthand, the operand alone is the condition, e.g. 'if a'
    std::optional<comparison> comparison_;

  public:
    expr_bool_op(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()}, nots_{tz}, lhs_{tc, tz},
          comparison_{parse_comparison(tc, tz)} {

        set_type(tc.builtins().boolean());
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        nots_.source_to(os);
        lhs_.source_to(os);

        if (not comparison_) {
            return;
        }

        comparison_->op_tk.source_to(os);
        comparison_->rhs.source_to(os);
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        visitor({
            .node{&lhs_},
            .written_place{},
        });

        if (comparison_) {
            visitor({
                .node{&comparison_->rhs},
                .written_place{},
            });
        }
    }

    //
    // class methods
    //

    // e.g. 'a + 1', null when negated or compared
    [[nodiscard]] auto arithmetic() const -> const expr_arith* {
        if (comparison_ or not nots_.is_empty()) {
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

        if (const stmt_identifier* const path{lhs_.operand_path()};
            path != nullptr and path->is_array()) {

            throw_unindexed_array(path->tok(), path->dotted_path());
        }

        if (lhs_type.is_bool()) {
            return;
        }

        // a condition tests a number, an instance would be tested as its
        // first field
        if (not lhs_type.is_builtin()) {
            throw compile_error{lhs_.tok(),
                                std::format("an instance of {} is not a 'bool'",
                                            in_quotes(lhs_type.name()))};
        }

        const std::string source{statement::trimmed_source(lhs_)};
        const std::string comparison{std::format("{} != 0", source)};

        throw compile_error{
            lhs_.tok(),
            std::format("{} of type {} is not a 'bool', compare it, e.g. {}",
                        in_quotes(source), in_quotes(lhs_type.name()),
                        in_quotes(comparison))};
    }

    // an element of a list: it branches to 'target' when its value is
    // 'branch_on_true', nowhere when 'target' is empty; an 'and' element and
    // the last element of any list branch when false, an 'or' element before
    // the last when true; 'inverted' is the inversion of the list, 'joined_by'
    // its connective after the inversion; the result is known when the
    // expression evaluated at compile time
    [[nodiscard]] auto
    compile_branch(toc& tc, const size_t indent, const std::string_view target,
                   const bool branch_on_true, const bool inverted,
                   const connective joined_by, const operand& dst) const
        -> condition_result {

        const std::string_view list_op{
            joined_by == connective::logical_or ? "or" : "and",
        };

        return compile_element(tc, indent, comment_label(list_op, inverted),
                               {
                                   .operation{operation()},
                                   .inverted{inverted != is_negated()},
                                   .destination{dst},
                                   .target{target},
                                   .branch_on_true{branch_on_true},
                               });
    }

    // the value 'compile' would find without emitting code, empty when
    // evaluated at run time
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<bool> {

        const comparison_side lhs{side_of(tc, lhs_)};

        if (comparison_ and not lhs.folded) {
            return std::nullopt;
        }

        const std::optional<bool> constant{
            comparison_ ? constant_comparison(lhs, comparison_->op,
                                              side_of(tc, comparison_->rhs))
                        : constant_shorthand(lhs),
        };

        if (not constant) {
            return std::nullopt;
        }

        return *constant != is_negated();
    }

    // takes the operand that 'arithmetic' points to
    [[nodiscard]] auto release_arithmetic() -> expr_arith {
        assert(arithmetic() != nullptr);

        return std::move(lhs_);
    }

  private:
    // e.g. 'a < b' of two 'bool's, which are not numbers
    auto assert_boolean_equality_only(const comparison_operator op) const
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

    // the sides are classified once
    [[nodiscard]] auto
    compile_comparison(toc& tc, const size_t indent, const comparison& cmp,
                       const machine::comparison_action& action) const
        -> condition_result {

        const comparison_side lhs{side_of(tc, lhs_)};
        const comparison_side rhs{side_of(tc, cmp.rhs)};

        if (const std::optional<bool> constant{
                constant_comparison(lhs, cmp.op, rhs),
            }) {

            return compile_constant(tc, indent, *constant != action.inverted,
                                    action);
        }

        // e.g. the 'i' of 'i == h(i)' with 'h' writing 'i'
        assert_not_written_later(tc, operands_of(*this), "operand");

        if (not cmp.is_memory) {
            assert_sides_fit(lhs, rhs, cmp.op);
        }

        // a constant 'lhs' becomes an immediate 'rhs' instead of a scratch
        // register copy: an untyped one fits the other side, a typed one when
        // its value does, which then compares as it would at its own width
        const bool is_mirrored{
            lhs.folded and (not lhs.is_typed_constant() or
                            constant_arith::fits_width(lhs.folded->value,
                                                       rhs.node->get_type())),
        };

        if (is_mirrored) {
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

        resolve_memory_cmp(tc, indent, cmp, action);

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

        if (value == action.branch_on_true and not action.target.empty()) {
            x.branch(indent, action.target);
        }

        return known_result(value);
    }

    // 'action' says where it branches, nowhere for an empty target, and
    // whether the comparison is inverted, by the list and by the element's
    // own 'not's
    [[nodiscard]] auto
    compile_element(toc& tc, const size_t indent, const std::string_view label,
                    const machine::comparison_action& action) const
        -> condition_result {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this, "?", label));

        x.label(indent, tc.create_cmp_label(tok()));

        if (not comparison_) {
            return compile_shorthand(tc, indent, action);
        }

        return compile_comparison(tc, indent, *comparison_, action);
    }

    // the operand alone, compared with 0
    [[nodiscard]] auto
    compile_shorthand(toc& tc, const size_t indent,
                      const machine::comparison_action& action) const
        -> condition_result {

        const comparison_side condition{side_of(tc, lhs_)};

        if (const std::optional<bool> constant{constant_shorthand(condition)}) {
            return compile_constant(tc, indent, *constant != action.inverted,
                                    action);
        }

        // note: the parse rejected an instance ('assert_boolean_operands')

        assert(lhs_.get_type().is_builtin());

        resolve_cmp_shorthand(tc, indent, condition, action);

        return condition_result::unknown;
    }

    [[nodiscard]] auto is_negated() const -> bool { return nots_.is_negated(); }

    // a shorthand is a comparison with 0 by '!='
    [[nodiscard]] auto operation() const -> comparison_operator {
        return comparison_ ? comparison_->op : comparison_operator::not_equal;
    }

    // the operator and the right side after 'lhs_', none for a shorthand
    // note: called by the constructor after 'lhs_' is made
    [[nodiscard]] auto parse_comparison(toc& tc, tokenizer& tz) const
        -> std::optional<comparison> {

        const std::optional<comparison_operator> op{
            comparison_of(tz.peek_operator()),
        };

        // without an operator the operand is the condition, e.g. 'if a {...}'
        if (not op) {
            assert_no_lone_operator_char(tz);
            return std::nullopt;
        }

        const token op_tk{tz.seen_operator()};

        comparison cmp{
            .op_tk{op_tk},
            .op{*op},
            .rhs{tc, tz},
            .is_memory{},
        };

        cmp.is_memory = classify_operands(tc, cmp);

        return cmp;
    }

    auto resolve_cmp(toc& tc, const size_t indent, const comparison_side& lhs,
                     const comparison_side& rhs,
                     const machine::comparison_action& action) const -> void {

        machine& x{tc.machine()};

        machine::held_registers held{x, tok(), indent};

        const operand dst{left_operand(tc, indent, lhs, held)};
        const operand src{right_operand(tc, indent, rhs, held)};

        x.compare_and_branch(tok(), indent, dst, src, action);
    }

    auto resolve_cmp_shorthand(toc& tc, const size_t indent,
                               const comparison_side& condition,
                               const machine::comparison_action& action) const
        -> void {

        const expr_arith& lhs{*condition.node};

        machine& x{tc.machine()};

        machine::held_registers held{x, tok(), indent};

        // a plain 'bool' already holds 0 or 1, a copy is shorter than
        // evaluating the shorthand as '!= 0'
        if (is_bool_copy(lhs, action)) {
            const operand src{left_operand(tc, indent, condition, held)};

            x.copy_value(tok(), indent, action.destination, src);

            return;
        }

        const operand dst{
            truth_test_operand(tc, indent, condition, action, held),
        };

        x.compare_and_branch(tok(), indent, dst,
                             operand::imm(0, tc.builtins().default_type()),
                             action);
    }

    // the bytes are compared and the result is branched on as for a shorthand
    // condition
    auto resolve_memory_cmp(toc& tc, const size_t indent, const comparison& cmp,
                            const machine::comparison_action& action) const
        -> void {

        // note: both sides are places ('is_memory_operand'), so both have a
        //       path

        const stmt_identifier* const lhs_path{lhs_.operand_path()};
        const stmt_identifier* const rhs_path{cmp.rhs.operand_path()};

        assert(lhs_path != nullptr and rhs_path != nullptr);

        const stmt_identifier& lhs{*lhs_path};
        const stmt_identifier& rhs{*rhs_path};

        const bool is_not_equal{
            cmp.op == comparison_operator::not_equal,
        };

        // a stored result has the inversion applied, a branch applies it
        const bool is_stored{not action.destination.is_empty()};

        machine& x{tc.machine()};

        const operand result{
            is_stored ? action.destination
                      : x.alloc_scratch_register(tok(), indent,
                                                 tc.builtins().boolean()),
        };

        compile_memory_equality(tc, indent + 1, tok(), lhs, rhs, result,
                                (is_stored and action.inverted) !=
                                    is_not_equal);

        if (not is_stored or not action.target.empty()) {
            machine::comparison_action branch_action{action};
            branch_action.operation = comparison_operator::not_equal;
            branch_action.destination = {};
            branch_action.inverted = not is_stored and action.inverted;

            x.compare_and_branch(tok(), indent, result,
                                 operand::imm(0, tc.builtins().default_type()),
                                 branch_action);
        }

        if (not is_stored) {
            x.free_scratch_register(tok(), indent, result);
        }
    }

    //
    // statics
    //

    // 'constant' is compared at the width of 'typed', e.g. 1000 with an 'i8'
    static auto assert_constant_fits(const expr_arith& typed,
                                     const comparison_side& constant) -> void {

        assert(constant.folded);

        const type& typed_type{typed.get_type()};

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

    // a '=' or '!' that is not part of a comparison, e.g. 'if a = b', is an
    // error at it
    static auto assert_no_lone_operator_char(const tokenizer& tz) -> void {
        const char next{tz.peek_char_after_whitespace()};

        if (next == '=') {
            throw compile_error{tz, "expected '=='"};
        }

        if (next == '!') {
            throw compile_error{tz, "expected '!='"};
        }
    }

    static auto assert_not_record(const expr_arith& side,
                                  const bool is_equality) -> void {

        if (side.get_type().is_builtin()) {
            return;
        }

        const std::string_view type_name{side.get_type().name()};

        // e.g. the '-' of '-p == p' applies to a number
        const stmt_identifier* const ident{side.operand_path()};

        const bool has_unary_ops{
            ident != nullptr and not ident->get_unary_ops().is_empty(),
        };

        if (is_equality and not has_unary_ops) {
            throw compile_error{
                side.tok(),
                std::format("an instance of {} is compared only with an "
                            "instance of the same type",
                            in_quotes(type_name))};
        }

        throw_not_a_number(side.tok(), type_name);
    }

    // the backends compare at the width of 'lhs': an untyped constant side
    // fits the type of the other, else 'rhs' is not wider
    static auto assert_sides_fit(const comparison_side& lhs,
                                 const comparison_side& rhs,
                                 const comparison_operator op) -> void {

        const bool is_lhs_untyped{lhs.folded and not lhs.is_typed_constant()};
        const bool is_rhs_untyped{rhs.folded and not rhs.is_typed_constant()};

        if (is_lhs_untyped and is_rhs_untyped) {
            return;
        }

        if (is_rhs_untyped) {
            assert_constant_fits(*lhs.node, rhs);
            return;
        }

        if (is_lhs_untyped) {
            assert_constant_fits(*rhs.node, lhs);
            return;
        }

        const type& lhs_type{lhs.node->get_type()};
        const type& rhs_type{rhs.node->get_type()};

        if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
            return;
        }

        throw_rhs_wider(*lhs.node, *rhs.node, op);
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

        // note: the size of the array was accepted where it is declared

        return product_storage_size(element_size_bytes, *lhs_len);
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

        x.memory_equal(src_loc_tk, indent, *size_bytes,
                       {
                           .alignment{lhs_info.type_ref().alignment()},
                           .lhs{lhs.address_emitter_of(tc, indent, src_loc_tk)},
                           .rhs{rhs.address_emitter_of(tc, indent, src_loc_tk)},
                           .dst{dst},
                           .inverted{inverted},
                       });
    }

    [[nodiscard]] static auto compile_to_scratch(toc& tc, const size_t indent,
                                                 const expr_arith& expr,
                                                 machine::held_registers& held)
        -> operand {

        // the operand is computed at the width of its first element
        expr.assert_not_narrowed(tc, expr.get_type());

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(expr.tok(), indent, expr.get_type()),
        };

        held.add(reg);
        expr.compile(tc, indent + 1, destination::of_register(reg));

        return reg;
    }

    // the value of a comparison whose sides fold, without the element's
    // 'not's, empty otherwise; the one fold of the parse, a 'let' and the
    // compile, so its sides are checked like those of a compiled comparison
    [[nodiscard]] static auto constant_comparison(const comparison_side& lhs,
                                                  const comparison_operator op,
                                                  const comparison_side& rhs)
        -> std::optional<bool> {

        if (not lhs.folded or not rhs.folded) {
            return std::nullopt;
        }

        assert_sides_fit(lhs, rhs, op);

        return constant_arith::eval_constant(lhs.folded->value, op,
                                             rhs.folded->value);
    }

    // the value of a shorthand whose operand folds, without the element's
    // 'not's, empty otherwise
    [[nodiscard]] static auto
    constant_shorthand(const comparison_side& condition)
        -> std::optional<bool> {

        if (not condition.folded) {
            return std::nullopt;
        }

        return condition.folded->value != 0;
    }

    // a side that folded as an immediate: a typed constant at its own type,
    // e.g. 'i16(300) + 1', an untyped one at 'untyped_type', the type of the
    // constant name or the default type for a list
    [[nodiscard]] static auto immediate_of(const comparison_side& side,
                                           const type& untyped_type)
        -> operand {

        assert(side.folded);

        if (side.folded->is_typed()) {
            return typed_constant(side);
        }

        return operand::imm(side.folded->value, untyped_type);
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

        // note: a value that is not computed has a path ('is_computed')
        return not lhs.is_computed() and
               lhs.operand_path()->get_unary_ops().is_empty() and
               lhs.get_type().is_bool() and not action.inverted and
               action.target.empty();
    }

    [[nodiscard]] static auto is_equality(const comparison_operator op)
        -> bool {

        return op == comparison_operator::equal or
               op == comparison_operator::not_equal;
    }

    // a user type instance or a whole array in storage, compared in place
    [[nodiscard]] static auto is_memory_operand(const toc& tc,
                                                const expr_arith& side)
        -> bool {

        const std::optional<place> stored{as_place(tc, side)};

        return stored and (stored->info.is_array() or
                           not stored->info.type_ref().is_builtin());
    }

    // the comparison needs its left operand in a register; a constant left
    // side was mirrored onto the right unless it is typed and does not fit it
    [[nodiscard]] static auto left_operand(toc& tc, const size_t indent,
                                           const comparison_side& side,
                                           machine::held_registers& held)
        -> operand {

        const expr_arith& expr{*side.node};

        // a computed value, the zero value '{}', which has no name, or a
        // constant, which 'compile' folds into one copy of its type, e.g. the
        // 'i16(300)' of 'i16(300) > b' with 'b' an 'i8'
        const stmt_identifier* const ident{expr.operand_path()};

        if (ident == nullptr or side.folded) {
            return compile_to_scratch(tc, indent, expr, held);
        }

        const ident_info expr_info{make_scalar_ident_info(tc, *ident)};

        assert(not expr_info.is_const());

        // e.g. 'a', '-a[i]'
        return ident->compile_value_operand(
            tc, indent, expr.tok(), expr_info, expr_info.type_ref(),
            tc.checks().overflow_options(), held);
    }

    // memory is compared, so a constant has nothing to compare
    [[nodiscard]] static auto
    make_memory_operand_info(const toc& tc, const stmt_identifier& identifier)
        -> ident_info {

        ident_info info{make_ident_info(tc, identifier)};

        // note: 'is_memory_operand' rejected constants
        assert(not info.is_const());

        return info;
    }

    // the right operand may be an immediate; an untyped constant is compared
    // as written, the caller checks that it fits the left side
    [[nodiscard]] static auto right_operand(toc& tc, const size_t indent,
                                            const comparison_side& side,
                                            machine::held_registers& held)
        -> operand {

        const expr_arith& expr{*side.node};

        // a computed value, or the zero value '{}', which has no name
        const stmt_identifier* const ident{expr.operand_path()};

        if (ident == nullptr) {
            if (not side.folded) {
                return compile_to_scratch(tc, indent, expr, held);
            }

            machine& x{tc.machine()};

            x.comment(expr.tok(), indent, "src: folded constant {}",
                      in_quotes(statement::trimmed_source(expr)));

            return immediate_of(side, tc.builtins().default_type());
        }

        const ident_info expr_info{make_scalar_ident_info(tc, *ident)};

        if (side.is_typed_constant() or expr_info.is_const()) {
            // note: a constant name or literal folds
            assert(side.folded);

            return immediate_of(side, expr_info.type_ref());
        }

        // e.g. 'a', '-a[i]'
        return ident->compile_value_operand(
            tc, indent, expr.tok(), expr_info, expr_info.type_ref(),
            tc.checks().overflow_options(), held);
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

    [[noreturn]] static auto throw_rhs_wider(const expr_arith& lhs,
                                             const expr_arith& rhs,
                                             const comparison_operator op)
        -> void {

        const type& lhs_type{lhs.get_type()};
        const type& rhs_type{rhs.get_type()};

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
                       machine::held_registers& held) -> operand {

        const expr_arith& lhs{*condition.node};

        if (lhs.is_computed() and action.destination.is_register() and
            lhs.get_type().is_same(action.destination.type_ref())) {

            lhs.compile_checked(tc, indent + 1,
                                destination::of_register(action.destination));

            return action.destination;
        }

        return left_operand(tc, indent, condition, held);
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
class expr_bool final : public statement {
    using element = std::variant<expr_bool_op, expr_bool>;

    std::vector<element> bools_;
    // the 'and' or 'or' between the elements of 'bools_'
    std::vector<spelled_operator<connective>> ops_;
    // the 'not's before the '(' of a parenthesized list, e.g. both of
    // 'not not (a == b and c == d)'
    condition_nots nots_;
    // none unless the list is parenthesized
    std::optional<token> open_paren_tk_;
    std::optional<token> close_paren_tk_;

  public:
    expr_bool(toc& tc, const token tk, tokenizer& tz) : statement{tk} {
        parse_list(tc, tz);
    }

    // e.g. '(a == b and c == d)' after its 'not's, the '(' has been read
    expr_bool(toc& tc, const token tk, tokenizer& tz, condition_nots nots,
              const token open_paren_tk)
        : statement{tk}, nots_{std::move(nots)}, open_paren_tk_{open_paren_tk} {

        parse_list(tc, tz);
        assert_parentheses_group();
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        nots_.source_to(os);
        token::source_to(os, open_paren_tk_);

        as_statement(bools_.front()).source_to(os);

        for (const auto [o, e] :
             std::views::zip(ops_, bools_ | std::views::drop(1))) {

            o.tk.source_to(os);
            as_statement(e).source_to(os);
        }

        token::source_to(os, close_paren_tk_);
    }

    // the value of the list stored in 'dst', e.g. 'b = x < 3 and y'
    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // stored comparisons would change what later elements read; a
        // register that the list reads is the result of an inlined function
        if (not reaches_destination(tc, *this, dst)) {
            compile_value(tc, indent, dst.operand());
            return;
        }

        machine& x{tc.machine()};

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

    auto visit_operands(const operand_visitor visitor) const -> void override {
        for (const element& e : bools_) {
            visitor({
                .node{&as_statement(e)},
                .written_place{},
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

        const bool inverted{is_negated()};

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
                return *value != inverted;
            }
        }

        // note: no element decided, so each is 'false' in an 'or' list and
        //       'true' in an 'and' list

        return (not is_or_list()) != inverted;
    }

    // the operand when the whole expression is one 'bool' operand, e.g. 'x'
    // or 'f(x)' but not 'not x', else none and the expression is unchanged
    [[nodiscard]] auto release_operand() -> std::optional<expr_arith> {
        if (bools_.size() != 1 or has_not()) {
            return std::nullopt;
        }

        expr_bool_op* const op{std::get_if<expr_bool_op>(&bools_.front())};

        if (op == nullptr) {
            return std::nullopt;
        }

        const expr_arith* const arith{op->arithmetic()};

        if (arith == nullptr) {
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

        // note: only a parenthesized list asks, see its constructor
        assert(open_paren_tk_);

        throw_parentheses_group_nothing(
            *open_paren_tk_, statement::trimmed_source(as_statement(only)));
    }

    // e.g. 'a or b and c' is rejected, '(a or b) and c' is not
    auto assert_same_connective(const spelled_operator<connective>& next) const
        -> void {

        if (ops_.empty() or ops_.front().op == next.op) {
            return;
        }

        throw compile_error{next.tk,
                            "mixing 'and' and 'or' without parentheses"};
    }

    // the elements from 'first'; a constant that decides the list ends it,
    // the elements after it never run and hold the compile rules too, see
    // 'toc::check_skipped'
    [[nodiscard]] auto
    compile_elements_from(toc& tc, const size_t indent,
                          const branch_targets& targets, const bool inverted,
                          const operand& dst, const size_t first) const
        -> condition_result {

        bool has_runtime_element{};

        // an element decided at run time may end the list, so the code after
        // the list is reached when the code after that element is, e.g. the
        // 'x > 0' of 'x > 0 or die(3) > 0' with 'die' an inlined 'exit'
        toc::path_merge paths{tc};

        const size_t last_index{bools_.size() - 1};

        for (size_t expr_index{first}; expr_index < last_index; ++expr_index) {
            const condition_result const_eval{
                compile_inner_element(tc, indent, expr_index, targets, inverted,
                                      dst),
            };

            if (not is_known(const_eval)) {
                has_runtime_element = true;
                paths.join_path();
                continue;
            }

            if (is_short_circuit(result_value(const_eval), inverted)) {
                tc.check_skipped([&] -> void {
                    std::ignore = compile_elements_from(
                        tc, indent, targets, inverted, dst, expr_index + 1);
                });

                paths.join_path();

                return const_eval;
            }
        }

        const condition_result last{
            compile_last_element(tc, indent, targets, inverted, dst,
                                 has_runtime_element),
        };

        paths.join_path();

        return last;
    }

    // an element that does not decide the list continues at the next element
    [[nodiscard]] auto
    compile_inner_element(toc& tc, const size_t indent, const size_t expr_index,
                          const branch_targets& targets, const bool inverted,
                          const operand& dst) const -> condition_result {

        const bool is_or{is_or_list() != inverted};

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
                                                  inverted, dst);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.at(expr_index))};

        return expr.compile_branch(tc, indent,
                                   is_or ? targets.if_true : targets.if_false,
                                   is_or, inverted, joined_by(inverted), dst);
    }

    [[nodiscard]] auto compile_last_element(
        toc& tc, const size_t indent, const branch_targets& targets,
        const bool inverted, const operand& dst,
        const bool has_runtime_element) const -> condition_result {

        // a stored value branches nowhere at its end, the label of both
        // outcomes follows; an empty target is no branch ('machine')
        const bool is_stored_only{targets.is_stored_only()};

        const std::string_view if_false{
            is_stored_only ? std::string_view{} : targets.if_false,
        };

        const std::string_view if_true{
            is_stored_only ? std::string_view{} : targets.if_true,
        };

        if (std::holds_alternative<expr_bool>(bools_.back())) {
            const expr_bool& nested_expr{std::get<expr_bool>(bools_.back())};

            const condition_result const_eval{
                nested_expr.compile_with_label(tc, indent, targets, inverted,
                                               dst),
            };

            // the nested list already emitted its final branch
            if (not is_known(const_eval)) {
                return condition_result::unknown;
            }

            return resolve_last_constant(tc, indent, result_value(const_eval),
                                         if_true, inverted,
                                         has_runtime_element);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.back())};

        const condition_result const_eval{
            expr.compile_branch(tc, indent, if_false, false, inverted,
                                joined_by(inverted), dst),
        };

        if (is_known(const_eval)) {
            return resolve_last_constant(tc, indent, result_value(const_eval),
                                         if_true, inverted,
                                         has_runtime_element);
        }

        if (is_stored_only) {
            return condition_result::unknown;
        }

        machine& x{tc.machine()};

        // every element that failed has branched to false
        x.branch(indent, if_true);

        return condition_result::unknown;
    }

    [[nodiscard]] auto
    compile_rec(toc& tc, const size_t indent, const branch_targets& targets,
                const bool outer_inverted, const operand& dst) const
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
                          *this, "?", outer_inverted ? " inverted: " : " "));
        }

        // inverted, according to De Morgan's laws
        const bool inverted{outer_inverted != is_negated()};

        return compile_elements_from(tc, indent, targets, inverted, dst, 0);
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
                                          const bool outer_inverted,
                                          const operand& dst) const
        -> condition_result {

        machine& x{tc.machine()};

        x.label(indent, tc.create_cmp_label(tok()));

        return compile_rec(tc, indent, targets, outer_inverted, dst);
    }

    // true when a 'not' leads the list
    [[nodiscard]] auto has_not() const -> bool { return not nots_.is_empty(); }

    [[nodiscard]] auto is_enclosed() const -> bool {
        return open_paren_tk_.has_value();
    }

    [[nodiscard]] auto is_negated() const -> bool { return nots_.is_negated(); }

    // note: every operator of a list is the same, mixing them is rejected
    [[nodiscard]] auto is_or_list() const -> bool {
        return not ops_.empty() and ops_.front().op == connective::logical_or;
    }

    // a constant false ends an 'and' list and a constant true ends an 'or'
    // list; 'inverted' turns one into the other, see De Morgan's laws
    [[nodiscard]] auto is_short_circuit(const bool const_eval,
                                        const bool inverted) const -> bool {

        return const_eval == (is_or_list() != inverted);
    }

    // the connective of the list after 'inverted', see De Morgan's laws
    [[nodiscard]] auto joined_by(const bool inverted) const -> connective {
        return is_or_list() != inverted ? connective::logical_or
                                        : connective::logical_and;
    }

    // a comparison or a parenthesized list, either may follow 'not'
    auto parse_element(toc& tc, tokenizer& tz) -> void {
        // a comparison reads its 'not's itself, e.g. '(t1 + t2) > 3' is a
        // comparison with a parenthesized operand
        if (not is_list_next(tz)) {
            bools_.emplace_back(std::in_place_type<expr_bool_op>, tc, tz);
            return;
        }

        // the 'not's before a parenthesis, e.g. 'not not (a == b)'
        condition_nots nots{tz};

        // the list is located at its '(', which keeps the whitespace before it
        const token pos_tk{tz.next_token_position()};

        const std::optional<token> open_paren_tk{tz.is_next_char_token('(')};

        // note: 'is_list_next' saw the '('
        assert(open_paren_tk);

        bools_.emplace_back(std::in_place_type<expr_bool>, tc, pos_tk, tz,
                            std::move(nots), *open_paren_tk);
    }

    // the operands are joined by 'and' or by 'or', not by both, e.g.
    // 'a == 1 and b == 2 and c == 3'
    auto parse_list(toc& tc, tokenizer& tz) -> void {
        const toc::parse_nesting_scope nesting{tc, tz};

        set_type(tc.builtins().boolean());

        parse_element(tc, tz);

        while (not read_close_paren(tz)) {
            const std::optional<spelled_operator<connective>> next{
                read_connective(tz),
            };

            if (not next) {
                return;
            }

            assert_same_connective(*next);

            ops_.push_back(*next);

            parse_element(tc, tz);
        }
    }

    // the ')' of an enclosed list is next: read it, e.g. '(a == 1 or b == 2)'
    [[nodiscard]] auto read_close_paren(tokenizer& tz) -> bool {
        if (not is_enclosed()) {
            return false;
        }

        close_paren_tk_ = tz.is_next_char_token(')');

        return close_paren_tk_.has_value();
    }

    // the 'and' or 'or' after an operand, none when the expression ends; an
    // enclosed expression first reads its ')' ('read_close_paren')
    [[nodiscard]] auto read_connective(tokenizer& tz)
        -> std::optional<spelled_operator<connective>> {

        // the connective after an operand, e.g. the 'and' of
        // 'a == 1 and b == 2'
        const token op_tk{tz.next_token()};

        if (const std::optional<connective> op{connective_of(op_tk)}) {
            return spelled_operator<connective>{
                .tk{op_tk},
                .op{*op},
            };
        }

        // anything else ends the expression, an enclosed one needs its ')'
        // first
        // e.g. the 'exit' of 'if a == 1 exit(1)', or of '(a == 1 exit(1)'
        // which lacks its ')'
        if (is_enclosed()) {
            throw_unclosed_list(op_tk);
        }

        tz.put_back_token(op_tk);

        return std::nullopt;
    }

    // a constant last element decides the list only if all earlier elements
    // were constants or it short-circuits the list
    [[nodiscard]] auto resolve_last_constant(
        toc& tc, const size_t indent, const bool const_eval,
        const std::string_view jmp_to_if_true, const bool inverted,
        const bool has_runtime_element) const -> condition_result {

        if (not has_runtime_element) {
            return known_result(const_eval);
        }

        if (is_short_circuit(const_eval, inverted)) {
            return known_result(const_eval);
        }

        // a constant false has already branched to false, a stored value has
        // no branch
        if (not const_eval or jmp_to_if_true.empty()) {
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

    // an element as the node it is, a comparison or a parenthesized list
    [[nodiscard]] static auto as_statement(const element& e)
        -> const statement& {

        return e.visit(
            [](const auto& item) -> const statement& { return item; });
    }

    // a parenthesized list follows its 'not's, decided on a copy
    [[nodiscard]] static auto is_list_next(const tokenizer& tz) -> bool {
        tokenizer probe{tz};

        std::ignore = condition_nots{probe};

        return probe.is_next_char_token('(') and
               not is_parenthesized_operand(probe);
    }

    // a binary operator or a comparison after the ')' that closes the '('
    // just read makes the parentheses part of an operand, decided on a copy
    // before anything is parsed; a lone '=' or '!' too, so the comparison
    // reports it ('assert_no_lone_operator_char')
    [[nodiscard]] static auto is_parenthesized_operand(const tokenizer& tz)
        -> bool {

        tokenizer after_paren{tz};
        after_paren.skip_to_close_paren();

        if (after_paren.is_binary_operator_next()) {
            return true;
        }

        const char next{after_paren.peek_char_after_whitespace()};

        return next == '=' or next == '!';
    }
};
