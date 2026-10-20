#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
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
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "token.hpp"
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

class expr_bool_op final : public statement {
    // e.g. the 'not' of 'if not a == b ...'
    std::vector<token> nots_;
    expr_arith lhs_;
    token ws_pre_op_;
    machine::comparison_operator op_{machine::comparison_operator::equal};
    token ws_post_op_;
    expr_arith rhs_;
    bool is_shorthand_{}; // e.g. if a ...
    bool is_expression_{};

    // e.g. 'a == b' of two user type instances or two whole arrays, the bytes
    // are compared
    bool is_memory_comparison_{};

  public:
    expr_bool_op(toc& tc, tokenizer& tz,
                 std::unique_ptr<statement> first_expression = {})
        : statement{first_expression ? tz.cur_position_token()
                                     : tz.next_whitespace_token()} {

        set_type(tc.builtins().boolean());

        if (not first_expression) {
            nots_ = parse_not_tokens(tz);
        }

        lhs_ = {tc, tz,
                expr_arith::options{
                    .uops{},
                    .open_paren_tk{},
                    .is_implied_subexpression{},
                    .first_op_precedence{expr_arith::initial_precedence},
                    .first_expression{std::move(first_expression)},
                }};

        ws_pre_op_ = tz.next_whitespace_token();

        const std::optional<machine::comparison_operator> comparison{
            parse_comparison_operator(tz),
        };

        // without an operator the operand is the condition, e.g. 'if a {...}'
        if (not comparison) {
            // note: the whitespace stays with what follows, so the operand
            //       alone reproduces its source, see 'release_name_or_literal'
            tz.put_back_token(ws_pre_op_);
            ws_pre_op_ = {};
            is_shorthand_ = true;
            resolve_if_op_is_expression();
            return;
        }

        op_ = *comparison;

        ws_post_op_ = tz.next_whitespace_token();

        rhs_ = {tc, tz};

        classify_operands(tc);

        resolve_if_op_is_expression();
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
        ws_pre_op_.source_to(os);

        if (is_shorthand_) {
            return;
        }

        std::print(os, "{}", machine::source_text(op_));
        ws_post_op_.source_to(os);
        rhs_.source_to(os);
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return is_expression_;
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        assert(not is_expression_);

        return lhs_.name_or_literal();
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        lhs_.visit_reads(var, reader);
        rhs_.visit_reads(var, reader);
    }

    //
    // class methods
    //

    // e.g. 'a + 1', null when negated or compared
    [[nodiscard]] auto arithmetic() const -> const expr_arith* {
        if (not is_shorthand_ or not nots_.empty()) {
            return nullptr;
        }

        return &lhs_;
    }

    // a stored 'bool' is 0 or 1, an integer is compared first, a condition
    // such as 'if n' tests it without storing it
    auto assert_boolean_operands() const -> void {
        const type& lhs_type{lhs_.get_type()};

        // an instance is reported as a condition when it is compiled
        if (not is_shorthand_ or lhs_type.is_bool() or
            not lhs_type.is_builtin()) {

            return;
        }

        const std::string source{statement::trimmed_source(lhs_)};

        throw compile_error{
            lhs_.tok(),
            std::format("'{}' of type '{}' is not a 'bool', compare it, e.g. "
                        "'{} != 0'",
                        source, lhs_type.name(), source)};
    }

    [[nodiscard]] auto compile_and(toc& tc, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const bool inverted, const operand& dst,
                                   const bool branch_required) const
        -> condition_result {

        return compile_element(tc, indent, "and", inverted,
                               {
                                   .operation{op_},
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
                                   .operation{op_},
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

        const std::optional<int64_t> lhs_value{side_constant(tc, lhs_)};

        if (not lhs_value) {
            return std::nullopt;
        }

        if (is_shorthand_) {
            return (*lhs_value != 0) != is_negated();
        }

        const std::optional<int64_t> rhs_value{side_constant(tc, rhs_)};

        if (not rhs_value) {
            return std::nullopt;
        }

        return constant_arith::eval_constant(*lhs_value, op_, *rhs_value) !=
               is_negated();
    }

    // takes the operand that 'arithmetic' points to
    [[nodiscard]] auto release_arithmetic() -> expr_arith {
        assert(arithmetic() != nullptr);

        return std::move(lhs_);
    }

  private:
    // a comparison that folds is checked like one that is compiled
    auto assert_typed_constant_fits_lhs(const toc& tc) const -> void {
        if (is_shorthand_) {
            return;
        }

        const bool is_lhs_typed{is_typed_constant(tc, lhs_)};
        const bool is_rhs_typed{is_typed_constant(tc, rhs_)};

        if (is_lhs_typed and not is_rhs_typed) {
            assert_constant_fits(tc, lhs_, lhs_.get_type(), rhs_);
            return;
        }

        if (is_rhs_typed and not is_lhs_typed) {
            assert_constant_fits(tc, rhs_, rhs_.get_type(), lhs_);
            return;
        }

        if (not is_rhs_typed) {
            return;
        }

        const type& lhs_type{lhs_.get_type()};
        const type& rhs_type{rhs_.get_type()};

        if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
            return;
        }

        throw_rhs_wider(lhs_, rhs_, op_, lhs_type, rhs_type);
    }

    // the result that 'compile_boolean' or 'resolve_memory_cmp' stored in
    // 'action.destination' is branched on using 'action.branch_on_true'
    auto branch_on_stored_result(toc& tc, const size_t indent,
                                 const machine::comparison_action& action) const
        -> void {

        machine::comparison_action branch_action{action};

        // the stored result has the inversion applied
        branch_action.destination = {};
        branch_action.inverted = false;

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, action.destination,
                             operand::imm(0, tc.builtins().default_type()),
                             branch_action, {});
    }

    // two instances or arrays are compared as memory, anything else as numbers
    // e.g. 'p1 == p2' of instances, but 'p1 < p2' or 'p1 == 1' is rejected
    auto classify_operands(const toc& tc) -> void {
        const bool is_equality{
            op_ == machine::comparison_operator::equal or
                op_ == machine::comparison_operator::not_equal,
        };

        is_memory_comparison_ = is_equality and is_memory_operand(tc, lhs_) and
                                is_memory_operand(tc, rhs_);

        if (not is_memory_comparison_) {
            assert_not_record(lhs_, is_equality);
            assert_not_record(rhs_, is_equality);
        }
    }

    // a shorthand is named since its source shows no comparison with 0
    [[nodiscard]] auto comment_label(const std::string_view list_op,
                                     const bool inverted) const -> std::string {

        if (inverted and is_shorthand_) {
            return std::format(" '{}' inverted shorthand: ", list_op);
        }

        if (inverted) {
            return std::format(" '{}' inverted: ", list_op);
        }

        if (is_shorthand_) {
            return " shorthand: ";
        }

        return " ";
    }

    // a constant emits no comparison, only the short-circuit branch
    auto compile_constant(toc& tc, const size_t indent, const bool value,
                          const machine::comparison_action& action) const
        -> condition_result {

        machine& x{tc.machine()};

        x.comment(lhs_.tok(), indent, "const eval to {}",
                  value ? "true" : "false");

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

        const std::optional<bool> constant{constant_value(tc)};

        if (constant) {
            assert_typed_constant_fits_lhs(tc);
            return compile_constant(tc, indent, *constant != inverted, action);
        }

        // a constant 'lhs' becomes an immediate 'rhs' instead of a scratch
        // register copy, the width check then applies to the swapped sides
        if (not is_shorthand_ and side_constant(tc, lhs_) and
            not is_typed_constant(tc, lhs_)) {

            machine::comparison_action mirrored_action{action};
            mirrored_action.operation = machine::mirrored(op_);

            resolve_cmp(tc, indent, rhs_, lhs_, mirrored_action);

            return condition_result::unknown;
        }

        if (not is_shorthand_ and not is_memory_comparison_) {
            resolve_cmp(tc, indent, lhs_, rhs_, action);
            return condition_result::unknown;
        }

        // a stored result needs no branch to the label that follows it
        machine::comparison_action stored_action{action};

        if (not branch_required) {
            stored_action.target = {};
        }

        if (is_memory_comparison_) {
            resolve_memory_cmp(tc, indent, stored_action);
            return condition_result::unknown;
        }

        assert_not_instance_condition(lhs_);

        machine::comparison_action shorthand_action{stored_action};
        shorthand_action.operation = machine::comparison_operator::not_equal;

        resolve_cmp_shorthand(tc, indent, lhs_, shorthand_action);

        return condition_result::unknown;
    }

    [[nodiscard]] auto is_negated() const -> bool {
        return is_negated_by(nots_);
    }

    auto resolve_cmp(toc& tc, const size_t indent, const expr_arith& lhs,
                     const expr_arith& rhs,
                     const machine::comparison_action& action) const -> void {

        std::vector<operand> allocated_registers;

        const operand dst{
            resolve_expr(tc, indent, lhs, true, allocated_registers),
        };

        const operand src{
            resolve_expr(tc, indent, rhs, false, allocated_registers),
        };

        assert_rhs_fits_lhs(tc, lhs, rhs, action.operation, dst, src);

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, dst, src, action,
                             allocated_registers);
    }

    auto resolve_cmp_shorthand(toc& tc, const size_t indent,
                               const expr_arith& lhs,
                               const machine::comparison_action& action) const
        -> void {

        if (lhs.produces_boolean() and not action.destination.is_empty()) {
            // note: only a 'bool' is assigned a boolean expression
            assert(lhs.get_type().is_same(action.destination.type_ref()));

            lhs.compile_boolean(tc, indent + 1, action.destination,
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
                resolve_expr(tc, indent, lhs, true, allocated_registers),
            };

            x.copy_value(tok(), indent, action.destination, src);
            x.free_scratch_registers(tok(), indent, allocated_registers);

            return;
        }

        const operand dst{
            truth_test_operand(tc, indent, lhs, action, allocated_registers),
        };

        machine& x{tc.machine()};

        x.compare_and_branch(tok(), indent, dst,
                             operand::imm(0, tc.builtins().default_type()),
                             action, allocated_registers);
    }

    auto resolve_if_op_is_expression() -> void {
        // a negated expression, a comparison and a shorthand condition on an
        // expression are expressions
        if (is_negated() or not is_shorthand_ or lhs_.is_expression()) {
            is_expression_ = true;
            return;
        }

        // a boolean value is read in place, a variable is tested
        const std::string_view id{lhs_.name_or_literal()};

        is_expression_ = id != reserved_names::true_value and
                         id != reserved_names::false_value;
    }

    // the bytes are compared and the result is branched on as for a shorthand
    // condition
    auto resolve_memory_cmp(toc& tc, const size_t indent,
                            const machine::comparison_action& action) const
        -> void {

        const statement& lhs{lhs_.identifier_statement()};
        const statement& rhs{rhs_.identifier_statement()};

        const bool is_not_equal{op_ == machine::comparison_operator::not_equal};

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

        const std::vector<operand> allocated_registers{result};

        x.compare_and_branch(tok(), indent, result,
                             operand::imm(0, tc.builtins().default_type()),
                             branch_action, allocated_registers);
    }

    //
    // statics
    //

    // 'constant' is compared at the width of 'typed', e.g. 1000 with an 'i8'
    static auto assert_constant_fits(const toc& tc, const expr_arith& typed,
                                     const type& typed_type,
                                     const expr_arith& constant) -> void {

        const std::optional<int64_t> value{side_constant(tc, constant)};

        assert(value);

        if (constant_arith::fits_size_bytes(*value, typed_type.size_bytes())) {
            return;
        }

        throw compile_error{
            constant.tok(),
            std::format("constant '{}' does not fit '{}' of type '{}'", *value,
                        statement::trimmed_source(typed), typed_type.name())};
    }

    // a condition tests a number, an instance would be tested as its first
    // field; checked when compiling since the initializer of a 'var' parses
    // its expression to learn the type, e.g. 'var b = a' copies the instance
    static auto assert_not_instance_condition(const expr_arith& side) -> void {
        if (side.get_type().is_builtin()) {
            return;
        }

        throw compile_error{
            side.tok(), std::format("an instance of '{}' is not a condition",
                                    side.get_type().name())};
    }

    // an operator would compare a user type instance as its first field only
    static auto assert_not_record(const expr_arith& side,
                                  const bool is_equality) -> void {

        if (side.get_type().is_builtin()) {
            return;
        }

        const std::string_view type_name{side.get_type().name()};

        if (is_equality) {
            throw compile_error{
                side.tok(),
                std::format("an instance of '{}' is compared only with an "
                            "instance of the same type",
                            type_name)};
        }

        throw compile_error{
            side.tok(),
            std::format("an instance of '{}' is not a number", type_name)};
    }

    // the backends compare at the width of 'lhs' which would truncate 'rhs'
    static auto assert_rhs_fits_lhs(toc& tc, const expr_arith& lhs,
                                    const expr_arith& rhs,
                                    const machine::comparison_operator op,
                                    const operand& lhs_op,
                                    const operand& rhs_op) -> void {

        const type& lhs_type{lhs_op.type_ref()};

        // a typed constant, e.g. an argument of an inlined call, is compared
        // as a variable of its type
        if (not rhs_op.is_immediate() or is_typed_constant(tc, rhs)) {
            const type& rhs_type{rhs_op.type_ref()};

            if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
                return;
            }

            throw_rhs_wider(lhs, rhs, op, lhs_type, rhs_type);
        }

        assert_constant_fits(tc, lhs, lhs_type, rhs);
    }

    // the bytes that both sides hold, they hold the same type and, as arrays,
    // the same number of elements
    [[nodiscard]] static auto
    compared_size_bytes(const statement& lhs, const statement& rhs,
                        const ident_info& lhs_info, const ident_info& rhs_info)
        -> size_t {

        if (not lhs_info.type_ref().is_same(rhs_info.type_ref())) {
            throw compile_error{
                rhs.tok(),
                std::format("cannot compare type '{}' with type '{}'",
                            lhs_info.type_ref().name(),
                            rhs_info.type_ref().name())};
        }

        // a whole array is not compared as its first element
        if (lhs_info.is_array() != rhs_info.is_array()) {
            toc::assert_not_whole_array(lhs, lhs_info);
            toc::assert_not_whole_array(rhs, rhs_info);
        }

        const size_t element_size_bytes{lhs_info.type_ref().size_bytes()};

        if (not lhs_info.is_array()) {
            return element_size_bytes;
        }

        const bool is_length_unknown{
            lhs_info.is_length_unknown() or rhs_info.is_length_unknown(),
        };

        // arrays are compared element by element, so their lengths must agree
        // note: a whole array on one side only was rejected above
        if (not is_length_unknown and
            lhs_info.array_len() != rhs_info.array_len()) {

            throw compile_error{
                rhs.tok(),
                std::format("cannot compare arrays of sizes {} and {}",
                            lhs_info.array_len(), rhs_info.array_len())};
        }

        return multiply_storage_size(lhs.tok(), element_size_bytes,
                                     lhs_info.array_len());
    }

    // compares the bytes of two user type instances or of two whole arrays,
    // 'dst' is 1 when they are equal, negated when 'inverted'
    static auto compile_memory_equality(toc& tc, const size_t indent,
                                        const token& src_loc_tk,
                                        const statement& lhs,
                                        const statement& rhs,
                                        const operand& dst, const bool inverted)
        -> void {

        const ident_info lhs_info{make_memory_operand_info(tc, lhs)};
        const ident_info rhs_info{make_memory_operand_info(tc, rhs)};

        const size_t size_bytes{
            compared_size_bytes(lhs, rhs, lhs_info, rhs_info),
        };

        // nothing to compare, the code of a function checked without a call
        // is discarded
        if (lhs_info.is_length_unknown() or rhs_info.is_length_unknown()) {
            return;
        }

        machine& x{tc.machine()};

        x.memory_equal(
            src_loc_tk, indent, size_bytes,
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

        return not lhs.is_expression() and lhs.get_unary_ops().is_empty() and
               lhs.get_type().is_bool() and not action.inverted and
               action.target.empty();
    }

    // a user type instance or a whole array, not unary operated or computed
    [[nodiscard]] static auto is_memory_operand(const toc& tc,
                                                const expr_arith& side)
        -> bool {

        if (not side.is_name_or_literal() or
            not side.get_unary_ops().is_empty()) {

            return false;
        }

        const ident_info info{tc.make_ident_info(side.identifier_statement())};

        return not info.is_const() and
               (info.is_array() or not info.type_ref().is_builtin());
    }

    // the value of an argument of an inlined call, it has the rules of a
    // variable of the type of its parameter
    [[nodiscard]] static auto is_typed_constant(const toc& tc,
                                                const expr_arith& side)
        -> bool {

        return not side.is_expression() and
               tc.make_ident_info(side).is_typed_const();
    }

    // memory is compared, so a constant has nothing to compare
    [[nodiscard]] static auto
    make_memory_operand_info(const toc& tc, const statement& identifier)
        -> ident_info {

        ident_info info{tc.make_ident_info(identifier)};

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
    resolve_expr(toc& tc, const size_t indent, const expr_arith& expr,
                 const bool is_lhs, std::vector<operand>& allocated_registers)
        -> operand {

        // the comparison needs its left operand in a register, 'compile'
        // folds a constant list into one copy
        if (expr.is_expression() and is_lhs) {
            return compile_to_scratch(tc, indent, expr, allocated_registers);
        }

        if (expr.is_expression()) {
            const std::optional<int64_t> value{side_constant(tc, expr)};

            if (not value) {
                return compile_to_scratch(tc, indent, expr,
                                          allocated_registers);
            }

            machine& x{tc.machine()};

            x.comment(expr.tok(), indent, "src: folded constant '{}'",
                      statement::trimmed_source(expr));

            return operand::imm(*value, tc.builtins().default_type());
        }

        const ident_info expr_info{tc.make_scalar_ident_info(expr)};

        if (expr.is_indexed() or
            (expr.is_name_or_literal() and
             tc.scopes().has_computed_address(expr.name_or_literal()))) {

            return expr.compile_lea(tc, indent, expr.tok(), allocated_registers,
                                    {
                                        .reg_count{},
                                        .known{expr_info.known()},
                                        .address_register{},
                                    });
        }

        // a typed constant left side is a value of its type in a register
        if (is_lhs and expr_info.is_typed_const()) {
            machine& x{tc.machine()};

            const operand reg{
                x.alloc_scratch_register(expr.tok(), indent,
                                         expr_info.type_ref()),
            };

            allocated_registers.emplace_back(reg);

            x.copy_value(expr.tok(), indent, reg,
                         expr.make_constant_operand(expr_info));

            return reg;
        }

        // a constant left side was mirrored onto the right
        assert(not expr_info.is_const() or not is_lhs);

        if (expr_info.is_const()) {
            return expr.make_constant_operand(expr_info);
        }

        if (expr.get_unary_ops().is_empty()) {
            return expr_info.operand();
        }

        const operand reg{
            expr_arith::compile_unary_to_scratch(
                tc, indent, expr, expr_info.operand(), expr_info.type_ref()),
        };

        allocated_registers.emplace_back(reg);

        return reg;
    }

    // a side computed at run time in a register of its own type, so a
    // constant list folds at that width
    [[nodiscard]] static auto side_constant(const toc& tc,
                                            const expr_arith& side)
        -> std::optional<int64_t> {

        return expr_arith::constant_value_of(
            tc, side, side.get_type(), expr_arith::constant_wrap::as_written);
    }

    [[noreturn]] static auto
    throw_rhs_wider(const expr_arith& lhs, const expr_arith& rhs,
                    const machine::comparison_operator op, const type& lhs_type,
                    const type& rhs_type) -> void {

        throw compile_error{
            rhs.tok(),
            std::format("'{}' of type '{}' is wider than '{}' of type '{}', "
                        "swap the operands: '{} {} {}'",
                        statement::trimmed_source(rhs), rhs_type.name(),
                        statement::trimmed_source(lhs), lhs_type.name(),
                        statement::trimmed_source(rhs),
                        machine::source_text(machine::mirrored(op)),
                        statement::trimmed_source(lhs))};
    }

    // the value compared with 0, matching types avoid narrowing before the
    // in-place truth test
    [[nodiscard]] static auto
    truth_test_operand(toc& tc, const size_t indent, const expr_arith& lhs,
                       const machine::comparison_action& action,
                       std::vector<operand>& allocated_registers) -> operand {

        if (lhs.is_expression() and action.destination.is_register() and
            lhs.get_type().is_same(action.destination.type_ref())) {

            lhs.assert_not_narrowed(tc, lhs.get_type());

            lhs.compile(tc, indent + 1,
                        destination::of_register(action.destination));

            return action.destination;
        }

        return resolve_expr(tc, indent, lhs, true, allocated_registers);
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
    std::vector<token> ops_;
    // the 'not's before the '(' of a parenthesized list, e.g. both of
    // 'not not (a == b and c == d)'
    std::vector<token> nots_;
    // empty unless the list is parenthesized
    token open_paren_tk_;
    token close_paren_tk_;

  public:
    // a caller might have supplied the first operand it already parsed, e.g.
    // the 'bool' of 'var b = bool'
    expr_bool(toc& tc, const token tk, tokenizer& tz,
              std::unique_ptr<statement> first_expression = {})
        : statement{tk} {

        parse_list(tc, tz, std::move(first_expression));
    }

    // e.g. '(a == b and c == d)' after its 'not's, the '(' has been read
    expr_bool(toc& tc, const token tk, tokenizer& tz, std::vector<token> nots,
              const token open_paren_tk)
        : statement{tk}, nots_{std::move(nots)}, open_paren_tk_{open_paren_tk} {

        assert(not open_paren_tk_.is_empty());

        parse_list(tc, tz, {});
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

        // e.g. 'true' or a named constant
        if (not is_expression()) {
            const ident_info src_info{tc.make_ident_info(*this)};

            assert(src_info.is_const());

            x.copy_value(
                tok(), indent, dst.operand(),
                operand::imm(src_info.const_value(), src_info.type_ref()));

            return;
        }

        // stored comparisons would change what later elements read
        if (dst.is_register() or not reads_var(dst.root_id())) {
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

    [[nodiscard]] auto is_expression() const -> bool override {
        if (bools_.size() > 1) {
            return true;
        }

        // 'name_or_literal' cannot carry the negation
        if (has_not()) {
            return true;
        }

        assert(not bools_.empty());

        return bools_.at(0).visit(
            [](const auto& e) -> bool { return e.is_expression(); });
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        assert(bools_.size() == 1);

        return bools_.at(0).visit([](const auto& e) -> std::string_view {
            return e.name_or_literal();
        });
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

    // every operand of a stored 'bool' is a boolean
    auto assert_boolean_operands() const -> void {
        for (const element& e : bools_) {
            e.visit([](const auto& item) -> void {
                item.assert_boolean_operands();
            });
        }
    }

    // decided at compile time only by constants before any run-time element,
    // a later short-circuit still needs the earlier elements evaluated
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<bool> {

        const bool is_or{not ops_.empty() and ops_.front().is_text("or")};
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

            // 'true' ends an 'or' list and 'false' ends an 'and' list
            if (*value == is_or) {
                return *value != invert;
            }
        }

        return (not is_or) != invert;
    }

    // the operand when the whole expression is a name or a literal, e.g. 'x'
    // but not '(x)' or 'not x', else none and the expression is unchanged
    [[nodiscard]] auto release_name_or_literal() -> std::optional<expr_arith> {
        if (bools_.size() != 1 or has_not()) {
            return std::nullopt;
        }

        expr_bool_op* const op{std::get_if<expr_bool_op>(&bools_.front())};

        if (op == nullptr) {
            return std::nullopt;
        }

        const expr_arith* const arith{op->arithmetic()};

        if (arith == nullptr or not arith->is_name_or_literal() or
            arith->is_expression()) {

            return std::nullopt;
        }

        return op->release_arithmetic();
    }

  private:
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

        const bool is_or{is_effective_or(expr_index, invert)};

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

            if (is_short_circuit(result_value(const_eval), expr_index,
                                 invert)) {

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

        const condition_result const_eval{
            compile(tc, indent,
                    {
                        .if_false{jmp_to_end},
                        .if_true{jmp_to_end},
                    },
                    dst),
        };

        machine& x{tc.machine()};

        x.label(indent, jmp_to_end);

        // a constant evaluation stores its value
        if (is_known(const_eval)) {
            x.store_boolean(tok(), indent, dst, result_value(const_eval));
        }
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

    // inversion swaps 'and' and 'or' according to De Morgan's laws
    [[nodiscard]] auto is_effective_or(const size_t op_index,
                                       const bool invert) const -> bool {

        return ops_.at(op_index).is_text("or") != invert;
    }

    [[nodiscard]] auto is_enclosed() const -> bool {
        return not open_paren_tk_.is_empty();
    }

    [[nodiscard]] auto is_negated() const -> bool {
        return is_negated_by(nots_);
    }

    // a constant false ends an 'and' list and a constant true ends an 'or' list
    [[nodiscard]] auto is_short_circuit(const bool const_eval,
                                        const size_t op_index,
                                        const bool invert) const -> bool {

        return const_eval == is_effective_or(op_index, invert);
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
    auto parse_list(toc& tc, tokenizer& tz,
                    std::unique_ptr<statement> first_expression) -> void {

        const tokenizer::nesting_scope nesting{tz};

        set_type(tc.builtins().boolean());

        parse_operand(tc, tz, std::move(first_expression));

        while (const std::optional<token> op_tk{read_connective(tz)}) {
            assert_same_connective(*op_tk);

            ops_.emplace_back(*op_tk);

            parse_operand(tc, tz, {});
        }
    }

    auto parse_operand(toc& tc, tokenizer& tz,
                       std::unique_ptr<statement> already_parsed) -> void {

        if (already_parsed) {
            bools_.emplace_back(std::in_place_type<expr_bool_op>, tc, tz,
                                std::move(already_parsed));

            return;
        }

        parse_element(tc, tz);
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
        const token pos_tk{tz.cur_position_token()};
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

        // note: rewound by position, a string token cannot be put back
        tz.rewind_to_position(pos_tk);

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

        if (is_short_circuit(const_eval, ops_.size() - 1, invert)) {
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
