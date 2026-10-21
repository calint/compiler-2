#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "constant_arith.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

//
// a flat list of elements and nested lists instead of a binary tree
//
//   a + b * c - d
//
//   exprs: [a] [b * c] [d]
//   ops:      +       -
//
// same-precedence operations stay flat; higher-precedence operations become
// nested lists
//
// note: a list is computed left to right in its destination, only a nested
//       list needs a scratch register
//
class expr_arith final : public expression {
    using arithmetic_operator = machine::arithmetic_operator;

    std::vector<std::unique_ptr<statement>> exprs_; // expression list
    // operators between elements in the vector
    std::vector<arithmetic_operator> ops_;
    std::vector<token> ws_before_ops_; // whitespace before each of 'ops_'
    unary_ops uops_;                   // of the whole list, e.g. '~(a + b)'
    token open_paren_tk_;              // of '(a + b)', empty when not enclosed
    token close_paren_tk_;             // of '(a + b)', empty when not enclosed

    // true when this list was created because of a higher-precedence operation
    //   1 + 2 * 3 + 4  => 1 + [2 * 3] + 4
    bool is_implied_subexpression_{};

    // the first element is an enclosed list, e.g. the '(a + b)' of
    // '(a + b) * c'; a list given its first element by the caller has two or
    // more elements and keeps it false
    bool is_first_enclosed_{};

    // an operation of the list being folded: an element, or merged constants
    // when 'element' is null
    struct step {
        arithmetic_operator op{arithmetic_operator::assign};
        const statement* element{};
        std::optional<int64_t> value; // set when the step is a constant
        std::string folded_source;    // e.g. '* 3 * 2' for the comment
    };

    // an element of the list, 'is_enclosed' for a '(...)' list
    struct parsed_element {
        std::unique_ptr<statement> node;
        bool is_enclosed{};
    };

    static constexpr uint8_t precedence_additive_{1};
    static constexpr uint8_t precedence_multiplicative_{2};
    static constexpr uint8_t precedence_bitwise_or_{3};
    static constexpr uint8_t precedence_bitwise_and_{4};
    static constexpr uint8_t precedence_bitwise_xor_{5};
    static constexpr uint8_t precedence_shift_{6};

  public:
    // how a list is parsed when it is not a plain one
    struct options {
        // the unary ops of an enclosed list apply to all of it
        unary_ops uops;

        // the '(' of an enclosed list, which ends with its ')', empty when not
        // enclosed
        token open_paren_tk;

        // a list that was created for an operation of a higher precedence
        bool is_implied_subexpression{};

        uint8_t first_op_precedence{initial_precedence};

        // an element the caller has already parsed, e.g. the 'b' of 'b * c' in
        // 'a + b * c'
        std::unique_ptr<statement> first_expression;
    };

    expr_arith(toc& tc, tokenizer& tz)
        : expr_arith{tc, tz,
                     options{
                         .uops{},
                         .open_paren_tk{},
                         .is_implied_subexpression{},
                         .first_op_precedence{initial_precedence},
                         .first_expression{},
                     }} {}

    // the list is located where its first element starts
    expr_arith(toc& tc, tokenizer& tz, options opts)
        : expression{opts.first_expression ? position_of(*opts.first_expression)
                                           : tz.cur_position_token()},
          uops_{std::move(opts.uops)}, open_paren_tk_{opts.open_paren_tk},
          is_implied_subexpression_{opts.is_implied_subexpression} {

        const tokenizer::nesting_scope nesting{tz};

        // a recursive call might have supplied the first element it already
        // parsed
        // e.g. the 'b' of 'b * c' in 'a + b * c'
        if (opts.first_expression) {
            exprs_.emplace_back(std::move(opts.first_expression));
        } else {
            parsed_element first{parse_element(tc, tz)};
            is_first_enclosed_ = first.is_enclosed;
            exprs_.emplace_back(std::move(first.node));
        }

        // the first element decides the width the list is computed at
        // e.g. 'x + 1' has the type of the variable 'x'
        set_type(type_of_first_element(tc));

        // a sub-expression starts at the precedence of the operator that
        // opened it
        uint8_t precedence{opts.first_op_precedence};

        while (true) {
            // a sub-expression is closed by its ')'
            // e.g. '(a + b)'
            if (is_closed(tz)) {
                validate_arithmetic_operands(tc);
                return;
            }

            // an operator may start the next line after the trailing
            // whitespace of the previous element
            const token ws_before_op_tk{tz.next_whitespace_token()};

            // no operator also ends a list in function arguments at ',' or ')'
            // e.g. the ',' of 'f(a + b, c)'
            const std::optional<arithmetic_operator> next_op{peek_operator(tz)};

            if (not next_op) {
                end_list(tc, tz, ws_before_op_tk);
                return;
            }

            ops_.emplace_back(*next_op);

            const uint8_t next_precedence{precedence_for_op(*next_op)};

            // a higher precedence groups the previous element with what
            // follows, so parse that as a sub-expression
            // e.g. the '*' of 'a + b * c' groups 'b * c'
            if (next_precedence > precedence) {
                open_implied_subexpression(tc, tz, next_precedence,
                                           ws_before_op_tk);

                continue;
            }

            // a lower precedence returns to the parent list
            // e.g. the '-' of 'a + b * c - d' ends 'b * c'
            if (is_end_of_implied_subexpression(precedence, next_precedence)) {
                ops_.pop_back();
                end_list(tc, tz, ws_before_op_tk);
                return;
            }

            // an operator of the same precedence continues this list
            // e.g. the '-' of 'a + b - c'
            precedence = next_precedence;

            // the whitespace is kept to reproduce the source
            ws_before_ops_.emplace_back(ws_before_op_tk);
            consume_peeked_operator(tz);

            exprs_.emplace_back(parse_element(tc, tz).node);
        }
    }

    expr_arith() = default;

    // higher than the highest precedence
    static constexpr uint8_t initial_precedence{precedence_shift_ + 1};

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);

        if (is_enclosed()) {
            open_paren_tk_.source_to(os);
        }

        expression::source_to(os); // whitespace
        exprs_.at(0)->source_to(os);
        for (const auto [ws, o, e] : std::views::zip(
                 ws_before_ops_, ops_, exprs_ | std::views::drop(1))) {

            ws.source_to(os);
            std::print(os, "{}", machine::source_text(o));

            e->source_to(os);
        }

        if (is_enclosed()) {
            close_paren_tk_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // a list that folds is one copy of its value
        if (is_expression()) {
            const std::optional<folded_value> folded{
                fold(tc, value_use::wrapped_at(dst.type_ref())),
            };

            if (folded) {
                compile_constant(tc, indent, dst, folded->value,
                                 statement::trimmed_source(*this));

                return;
            }
        }

        // a single plain element in memory is compiled directly, without
        // trying with and without a scratch register
        if (not dst.is_register() and is_single_plain_element()) {
            do_compile(tc, indent, dst);
            return;
        }

        // writing the destination early would change what later elements
        // read; a register that the expression reads is the result of an
        // inlined function
        // e.g. 'r = a + r * 2' would copy 'a' into 'r' before 'r * 2' reads it
        if (destination_is_clobbered_before_read(dst)) {
            if (dst.is_register()) {
                compile_through_scratch_as(tc, indent, dst, dst.type_ref());
                return;
            }

            compile_through_scratch(tc, indent, dst);

            return;
        }

        // a register destination needs no scratch register
        if (dst.is_register()) {
            do_compile(tc, indent, dst);
            return;
        }

        // a scratch register saves loads and stores of a memory destination
        // or costs a copy, which depends on the elements, so both are measured
        tc.emit_most_efficient(
            tok(), indent, [&] -> void { do_compile(tc, indent, dst); },
            [&] -> void { compile_through_scratch(tc, indent, dst); });
    }

    [[nodiscard]] auto accessed_path() const -> access_path override {

        assert(exprs_.size() == 1);

        return exprs_.at(0)->accessed_path();
    }

    // each element is computed at the width of the destination
    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        for (const std::unique_ptr<statement>& e : exprs_) {
            e->assert_not_narrowed(tc, dst_type);
        }
    }

    auto compile_boolean(toc& tc, const size_t indent, const operand& dst,
                         const bool inverted) const -> void override {

        assert(produces_boolean());

        exprs_.front()->compile_boolean(tc, indent, dst, inverted);
    }

    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
        -> operand override {

        assert(exprs_.size() == 1);

        return exprs_.at(0)->compile_lea(tc, indent, src_loc_tk,
                                         allocated_registers, request);
    }

    // an element wraps at the width as its register holds it; the first
    // element gives the list its type, e.g. 'i16(300) + 1' is typed
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        // e.g. 'k' or 'i8(1)'
        if (not is_expression()) {
            return exprs_.front()->fold(tc, use);
        }

        const type& width_type{use.width_ref()};
        const value_use element_use{value_use::wrapped_at(width_type)};

        const std::optional<folded_value> first{
            exprs_.front()->fold(tc, element_use),
        };

        if (not first) {
            return std::nullopt;
        }

        std::optional<int64_t> value{first->value};

        for (const auto [o, e] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            if (not value) {
                return std::nullopt;
            }

            const std::optional<int64_t> rhs{
                folded_value::value_of(e->fold(tc, element_use)),
            };

            if (not rhs) {
                return std::nullopt;
            }

            if (tc.checks().overflow and
                constant_arith::overflows_width(*value, o, *rhs, width_type)) {

                throw compile_error{e->tok(),
                                    "constant expression overflows the type"};
            }

            value = fold_operation(tc, *value, o, *e, *rhs, width_type);
        }

        if (not value) {
            return std::nullopt;
        }

        const int64_t result{
            uops_.evaluate_at(*value, width_type, tc.checks().overflow),
        };

        return folded_value{
            .value{constant_arith::wrap_to_width(result, width_type)},
            .own_type{first->own_type},
        };
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        // note: the unary ops of a multi-element list are never asked for,
        //       they are compiled with the list

        assert(exprs_.size() == 1);

        return exprs_.at(0)->get_unary_ops();
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        return exprs_.size() == 1 and exprs_.front()->is_array_element();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        // unary operators apply to the computed list, e.g. '-(a)'
        if (not uops_.is_empty()) {
            return true;
        }

        // a single element is read in place unless it is computed itself
        if (exprs_.size() == 1) {
            return exprs_.at(0)->is_expression();
        }

        // several elements are combined by instructions
        return true;
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        assert(exprs_.size() == 1);

        return exprs_.at(0)->is_indexed();
    }

    [[nodiscard]] auto is_name_or_literal() const -> bool override {
        return exprs_.size() == 1 and exprs_.at(0)->is_name_or_literal();
    }

    // '/', '%' and '>>' read the high bits of their operands. narrowed, a
    // '<<' by a count not below the narrower width is defined only at the
    // wider one; widened, a '<<' count has the narrower type, so where it is
    // defined it is below that width
    [[nodiscard]] auto keeps_low_bits(const width_change change) const
        -> bool override {

        for (const arithmetic_operator o : ops_) {
            if (o == arithmetic_operator::divide or
                o == arithmetic_operator::remainder or
                o == arithmetic_operator::shift_right) {

                return false;
            }

            if (o == arithmetic_operator::shift_left and
                change == width_change::narrowed) {

                return false;
            }
        }

        for (const std::unique_ptr<statement>& e : exprs_) {
            if (not e->keeps_low_bits(change)) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        assert(exprs_.size() == 1);

        return exprs_.at(0)->name_or_literal();
    }

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return uops_.is_empty() and exprs_.size() == 1 and
               exprs_.front()->produces_boolean();
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        for (const std::unique_ptr<statement>& e : exprs_) {
            e->visit_reads(var, reader);
        }
    }

    //
    // class methods
    //

    // compiles the list without its trailing added constant and returns that
    // constant, empty when the list is compiled whole by 'compile'
    //   'ix + 1'  =>  compiles 'ix', returns 1
    //   'ix - 2'  =>  compiles 'ix', returns -2
    [[nodiscard]] auto
    compile_without_trailing_addend(toc& tc, const size_t indent,
                                    const destination& dst) const
        -> std::optional<int64_t> {

        // note: only an index expression asks, it has no unary ops of its own
        assert(uops_.is_empty());

        const type& width_type{dst.type_ref()};

        const std::vector<step> steps{plan_steps(tc, width_type)};

        const step& last{steps.back()};

        if (steps.size() < 2 or last.element != nullptr or
            last.op != arithmetic_operator::add) {

            return std::nullopt;
        }

        // note: 2 steps are the least with something to add the constant to

        compile_first_step(tc, indent, dst, steps.front());

        const std::span<const step> middle_steps{
            std::span{steps}.subspan(1, steps.size() - 2),
        };

        // note: the 1 skips the first step compiled above and the 2 also
        //       leaves out the trailing constant

        for (const step& s : middle_steps) {
            compile_step(tc, indent, dst, s);
        }

        return last.value;
    }

    // the statement of an arithmetic that is only an identifier
    [[nodiscard]] auto identifier_statement() const -> const statement& {
        assert(is_name_or_literal());

        return *exprs_.front();
    }

    // e.g. 'flag', 'p', 'f(x)' or '-i32(x)', a parenthesized list is
    // arithmetic of its own
    [[nodiscard]] auto is_single_operand() const -> bool {
        if (exprs_.size() != 1) {
            return false;
        }

        return not is_first_enclosed_;
    }

    //
    // statics
    //

    // the caller frees the returned register
    [[nodiscard]] static auto
    compile_unary_to_scratch(toc& tc, const size_t indent, const statement& src,
                             const operand& src_operand,
                             const type& register_type) -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, register_type),
        };

        x.copy_value(src.tok(), indent, reg, src_operand);

        src.get_unary_ops().compile(x, indent, src.tok(), reg,
                                    tc.checks().overflow_options());

        return reg;
    }

  private:
    // e.g. 'b + 1 - 1' or 'b / 1 / 1' apply nothing
    auto apply_merged_constant(toc& tc, const size_t indent,
                               const arithmetic_operator op,
                               const destination& dst, const int64_t value,
                               const std::string_view folded_source) const
        -> void {

        // subtracting zero changes nothing like adding it
        const arithmetic_operator identity_op{
            op == arithmetic_operator::subtract ? arithmetic_operator::add : op,
        };

        if (value == identity_of(identity_op)) {
            machine& x{tc.machine()};

            x.comment(tok(), indent,
                      "src: folded constant '{}' is {} and changes nothing",
                      folded_source, value);

            return;
        }

        asm_op_constant(tc, indent, op, dst, value, folded_source);
    }

    auto asm_op_constant(toc& tc, const size_t indent,
                         const arithmetic_operator op, const destination& dst,
                         const int64_t value,
                         const std::string_view folded_source) const -> void {

        // 'b - 3' rather than 'b + -3', the most negative value has no
        // positive counterpart in the width
        if (op == arithmetic_operator::add and value < 0 and
            value !=
                constant_arith::signed_minimum(dst.type_ref().size_bits())) {

            asm_op_constant(tc, indent, arithmetic_operator::subtract, dst,
                            -value, folded_source);

            return;
        }

        machine& x{tc.machine()};

        x.comment(tok(), indent, "{} {} {}", dst.name(),
                  machine::source_text(op), value);

        x.comment(tok(), indent, "src: folded constant '{}'", folded_source);

        const operand constant{constant_operand(tc, value)};

        if (op == arithmetic_operator::add or
            op == arithmetic_operator::subtract) {

            x.add_subtract(tok(), indent, op, dst.operand(), constant,
                           tc.checks().overflow_options());

            return;
        }

        if (op == arithmetic_operator::multiply) {
            x.multiply(tok(), indent, dst.operand(), constant,
                       machine::factor_use::read_only,
                       tc.checks().overflow_options());

            return;
        }

        if (op == arithmetic_operator::divide) {
            x.divide(tok(), indent, op, dst.operand(), constant,
                     tc.checks().division_options());

            return;
        }

        x.bitwise(tok(), indent, op, dst.operand(), constant);
    }

    // a narrow memory destination truncates when stored, which gives the same
    // value when the low bits do not depend on the high bits
    [[nodiscard]] auto can_compute_wide(const toc& tc,
                                        const destination& dst) const -> bool {

        // note: a register destination is compiled directly by 'compile'
        assert(dst.operand().is_memory());

        if (dst.type_ref().size_bytes() >=
            tc.builtins().default_type().size_bytes()) {

            return false;
        }

        // the wide result must be checked against the narrow type, and a
        // shift count against the width of the narrow type
        if (tc.checks().overflow or tc.checks().shift) {
            return false;
        }

        return keeps_low_bits(width_change::widened);
    }

    auto compile_constant(toc& tc, const size_t indent, const destination& dst,
                          const int64_t value,
                          const std::string_view folded_source) const -> void {

        machine& x{tc.machine()};

        x.comment(tok(), indent, "{} = {}", dst.name(), value);
        x.comment(tok(), indent, "src: folded constant '{}'", folded_source);

        x.copy_value(tok(), indent, dst.operand(),
                     folded_value::untyped(value).operand_at(dst.type_ref()));
    }

    // a nested list is folded when it is compiled as an element:
    //   '3 + a * 3 * 2 / 3 / 2 * 4 + 5'  =>  'a * 6 / 6 * 4 + 8'
    auto compile_elements(toc& tc, const size_t indent,
                          const destination& dst) const -> void {

        const type& width_type{dst.type_ref()};

        const std::vector<step> steps{plan_steps(tc, width_type)};

        compile_first_step(tc, indent, dst, steps.front());

        for (const step& s : std::span{steps}.subspan(1)) {
            // note: 1 because the first step is compiled above

            compile_step(tc, indent, dst, s);
        }
    }

    auto compile_first_step(toc& tc, const size_t indent,
                            const destination& dst, const step& first) const
        -> void {

        // e.g. '2 * 3 / b' or '3 - b'
        // under the overflow check the constants stay elements, a first
        // constant is copied
        const bool is_checked_constant{
            tc.checks().overflow and first.value.has_value() and
                first.op == arithmetic_operator::add,
        };

        if (first.element == nullptr or is_checked_constant) {
            assert(first.value);

            compile_constant(tc, indent, dst, *first.value,
                             first.folded_source);

            return;
        }

        // e.g. the 'g()' of 'g() + 1' but not of '(g())'
        const destination first_dst{ops_.empty() ? dst : dst.as_operand()};

        compile_first_element(tc, indent, first_dst, *first.element);

        // only a zero constant leaves a subtracted element first: '2 - b - 2'
        if (first.op == arithmetic_operator::subtract) {
            machine& x{tc.machine()};

            x.unary(tok(), indent, machine::arithmetic_operator::negate,
                    dst.operand(), tc.checks().overflow_options());
        }
    }

    auto compile_step(toc& tc, const size_t indent, const destination& dst,
                      const step& s) const -> void {

        // the merging passes are off under the overflow check, a constant
        // that changes nothing must still not reach the backend
        const bool is_checked_constant{
            tc.checks().overflow and s.value.has_value() and
                is_regroupable(s.op),
        };

        if (s.value and (s.element == nullptr or is_checked_constant)) {
            apply_merged_constant(tc, indent, s.op, dst, *s.value,
                                  s.folded_source);

            return;
        }

        asm_op(tc, indent, s.op, dst, *s.element);
    }

    // the destination keeps its value until the whole expression is computed
    auto compile_through_scratch(toc& tc, const size_t indent,
                                 const destination& dst) const -> void {

        const type& dst_type{dst.type_ref()};

        if (not can_compute_wide(tc, dst)) {
            compile_through_scratch_as(tc, indent, dst, dst_type);
            return;
        }

        // a target without narrow register operations normalizes a narrow
        // register after each operation, a wide register needs only the store
        tc.emit_most_efficient(
            tok(), indent,
            [&] -> void {
                compile_through_scratch_as(tc, indent, dst, dst_type);
            },
            [&] -> void {
                compile_through_scratch_as(tc, indent, dst,
                                           tc.builtins().default_type());
            });
    }

    auto compile_through_scratch_as(toc& tc, const size_t indent,
                                    const destination& dst,
                                    const type& scratch_type) const -> void {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(tok(), indent, scratch_type),
        };

        do_compile(tc, indent, destination::of_register(reg));

        x.copy_value(tok(), indent, dst.operand(), reg);

        x.free_scratch_register(tok(), indent, reg);
    }

    // a plain first element is copied before anything writes the destination;
    // a negated one may move behind a leading constant: 'n = -n + 1'
    [[nodiscard]] auto
    destination_is_clobbered_before_read(const destination& dst) const -> bool {

        const std::string_view root{dst.root_id()};

        const statement& first{*exprs_.front()};

        const bool is_plain{
            first.is_name_or_literal() and first.get_unary_ops().is_empty(),
        };

        if (not is_plain and first.reads_var(root)) {
            return true;
        }

        return std::ranges::any_of(
            exprs_ | std::views::drop(1),
            [root](const std::unique_ptr<statement>& e) -> bool {
                return e->reads_var(root);
            });
    }

    auto do_compile(toc& tc, const size_t indent, const destination& dst) const
        -> void {

        compile_elements(tc, indent, dst);

        machine& x{tc.machine()};

        // the unary ops of the list apply to its result, e.g. '-(a + b)'
        uops_.compile(x, indent, tok(), dst.operand(),
                      tc.checks().overflow_options());
    }

    // 'ws_before_op_tk' belongs to what follows the list
    auto end_list(const toc& tc, tokenizer& tz,
                  const token& ws_before_op_tk) const -> void {

        tz.put_back_token(ws_before_op_tk);

        // e.g. the '==' of '(x == 1) == b' or of 'x = 1 + (x < 1)', which
        // takes the condition for an arithmetic operand
        if (is_enclosed() and is_comparison_next(tz)) {
            throw compile_error{tz.next_token(),
                                "a comparison makes a condition, which is not "
                                "an operand here, store it in a 'bool' first"};
        }

        // an enclosed list ends with its ')'
        // e.g. the 'exit' of 'x = (1 + 2 exit(x)'
        if (is_enclosed()) {
            throw compile_error{tz.next_token(),
                                "expected ')' to close expression"};
        }

        validate_arithmetic_operands(tc);
    }

    // the first element is added in an additive list and a factor in a list
    // led by '*' or a bitwise operation, otherwise it is the dividend or the
    // shifted value
    [[nodiscard]] auto first_op() const -> arithmetic_operator {
        if (ops_.empty()) {
            return arithmetic_operator::assign;
        }

        const arithmetic_operator first{ops_.front()};

        if (first == arithmetic_operator::subtract) {
            return arithmetic_operator::add;
        }

        return is_regroupable(first) ? first : arithmetic_operator::assign;
    }

    // a sub-expression is enclosed in parentheses
    [[nodiscard]] auto is_closed(tokenizer& tz) -> bool {
        if (not is_enclosed()) {
            return false;
        }

        close_paren_tk_ = tz.is_next_char_token(')');

        return not close_paren_tk_.is_empty();
    }

    // e.g. '(a + b)' vs 'a + b'
    [[nodiscard]] auto is_enclosed() const -> bool {
        return not open_paren_tk_.is_empty();
    }

    // a sub-expression ends when the operator has a lower precedence than its
    // own list
    //   want:  a - b * c + 3  ->  [a] - [b * c] + [3]
    //   if not returning then becomes: a - [b * c + 3]
    [[nodiscard]] auto is_end_of_implied_subexpression(
        const uint8_t precedence, const uint8_t next_precedence) const -> bool {

        // note: a sub-expression starts at an operator, so it has a precedence
        assert(not is_implied_subexpression_ or
               precedence != initial_precedence);

        return is_implied_subexpression_ and next_precedence < precedence;
    }

    // unary ops on a memory destination are a load, modify and store on a
    // load/store machine where a scratch register can be shorter, other single
    // elements such as calls are not compiled twice
    [[nodiscard]] auto is_single_plain_element() const -> bool {
        if (exprs_.size() != 1) {
            return false;
        }

        const statement& e{*exprs_.front()};

        if (not e.is_name_or_literal()) {
            return true;
        }

        return uops_.is_empty() and e.get_unary_ops().is_empty();
    }

    // pass 0: one step per element with the constant value of the element
    //   'a * 3 * 2 / 3 / 2 * 4'  =>  '* a' '* 3' '* 2' '/ 3' '/ 2' '* 4'
    [[nodiscard]] auto make_steps(const toc& tc, const type& width_type) const
        -> std::vector<step> {

        std::vector<step> steps;

        for (size_t i{}; i < exprs_.size(); ++i) {
            const arithmetic_operator op{i == 0 ? first_op() : ops_.at(i - 1)};
            // note: -1 because the first expression has no operator before it

            const statement& e{*exprs_.at(i)};

            steps.push_back({
                .op{op},
                .element{&e},
                .value{
                    folded_value::value_of(
                        e.fold(tc, value_use::wrapped_at(width_type))),
                },
                .folded_source{
                    std::format("{} {}", machine::source_text(op),
                                statement::trimmed_source(e)),
                },
            });
        }

        return steps;
    }

    // moves the last element to the front of a sub-expression that takes the
    // operator with the higher precedence
    //   a + b * c  ->  [a] + [b * c]
    auto open_implied_subexpression(toc& tc, tokenizer& tz,
                                    const uint8_t next_precedence,
                                    const token& ws_before_op_tk) -> void {

        // the operator is parsed by the sub-expression before its second
        // element
        ops_.pop_back();
        tz.put_back_token(ws_before_op_tk);

        std::unique_ptr<statement> last_elem_in_list{std::move(exprs_.back())};

        exprs_.pop_back();

        // the sub-expression continues with the precedence of the operator
        exprs_.emplace_back(std::make_unique<expr_arith>(
            tc, tz,
            options{
                .uops{},
                .open_paren_tk{},
                .is_implied_subexpression{true},
                .first_op_precedence{next_precedence},
                .first_expression{std::move(last_elem_in_list)},
            }));
    }

    // the passes in order, the steps are ready to compile
    [[nodiscard]] auto plan_steps(const toc& tc, const type& width_type) const
        -> std::vector<step> {

        std::vector<step> steps{make_steps(tc, width_type)};

        // moving or combining constants changes which partial results
        // overflow
        if (tc.checks().overflow) {
            return steps;
        }

        steps = merge_regroupable_constants(steps, width_type);
        steps = merge_divisors(steps, width_type);
        lead_with_constant(tc, steps);

        return steps;
    }

    // a function name has no type, its lookup rejects it as a value, e.g. the
    // 'make' of 'var x = make'
    [[nodiscard]] auto type_of_first_element(const toc& tc) const
        -> const type& {

        const statement& first_expr{*exprs_.front()};

        if (first_expr.is_name_or_literal()) {
            std::ignore = tc.make_ident_info(first_expr);
        }

        return first_expr.get_type();
    }

    // the elements are operands of the operators of the list or of its unary
    // ops, e.g. '-(a)'
    auto validate_arithmetic_operands(const toc& tc) const -> void {
        if (ops_.empty() and uops_.is_empty()) {
            return;
        }

        for (const std::unique_ptr<statement>& expr : exprs_) {
            assert_not_boolean(tc, *expr);
        }
    }

    //
    // statics
    //

    static auto append_source(step& merged, const step& s) -> void {
        if (not merged.folded_source.empty()) {
            merged.folded_source += ' ';
        }

        merged.folded_source += s.folded_source;
    }

    static auto asm_op(toc& tc, const size_t indent,
                       const arithmetic_operator op, const destination& dst,
                       const statement& src) -> void {

        machine& x{tc.machine()};

        x.comment(src.tok(), indent,
                  statement::trimmed_source(src, dst.name(),
                                            machine::source_text(op)));

        if (op == arithmetic_operator::assign) {
            asm_op_mov(tc, indent, dst, src);
            return;
        }

        if (op == arithmetic_operator::add or
            op == arithmetic_operator::subtract) {

            asm_op_add_sub(tc, indent, op, dst, src);
            return;
        }

        if (op == arithmetic_operator::multiply) {
            asm_op_mul(tc, indent, dst, src);
            return;
        }

        if (op == arithmetic_operator::divide or
            op == arithmetic_operator::remainder) {

            asm_op_div(tc, indent, op, dst, src);
            return;
        }

        if (op == arithmetic_operator::shift_left or
            op == arithmetic_operator::shift_right) {

            asm_op_shift(tc, indent, op, dst, src);
            return;
        }

        asm_op_bitwise(tc, indent, op, dst, src);
    }

    static auto asm_op_add_sub(toc& tc, const size_t indent,
                               const arithmetic_operator op,
                               const destination& dst, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // note: the rewrite hides an overflow of the negation, the check
        //       computes it

        if (not tc.checks().overflow and is_negated_operand(tc, src)) {
            x.comment(src.tok(), indent, "src: negated operand");

            const ident_info src_info{tc.make_scalar_ident_info(src)};
            std::vector<operand> lea_registers;

            const operand src_operand{
                src.get_lea_operand(tc, indent, src_info, lea_registers),
            };

            x.add_subtract(
                src.tok(), indent,
                op == arithmetic_operator::add ? arithmetic_operator::subtract
                                               : arithmetic_operator::add,
                dst.operand(), src_operand, tc.checks().overflow_options());

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // a negated operand is computed at the width of the destination so
        // the overflow check sees it
        emit_with_source(tc, indent, src, dst.type_ref(),
                         [&](const operand& term, const bool) -> void {
                             x.add_subtract(src.tok(), indent, op,
                                            dst.operand(), term,
                                            tc.checks().overflow_options());
                         });
    }

    static auto asm_op_bitwise(toc& tc, const size_t indent,
                               const arithmetic_operator op,
                               const destination& dst, const statement& src)
        -> void {

        machine& x{tc.machine()};

        emit_with_source(tc, indent, src, dst.type_ref(),
                         [&](const operand& value, const bool) -> void {
                             x.bitwise(src.tok(), indent, op, dst.operand(),
                                       value);
                         });
    }

    static auto asm_op_div(toc& tc, const size_t indent,
                           const arithmetic_operator op, const destination& dst,
                           const statement& src) -> void {

        machine& x{tc.machine()};

        emit_with_source(tc, indent, src, dst.type_ref(),
                         [&](const operand& divisor, const bool) -> void {
                             if (divisor.is_immediate()) {
                                 assert_divisor_not_zero(src.tok(),
                                                         divisor.immediate());
                             }

                             x.divide(src.tok(), indent, op, dst.operand(),
                                      divisor, tc.checks().division_options());
                         });
    }

    // 'compile_first_element' copies identifiers itself
    static auto asm_op_mov(toc& tc, const size_t indent, const destination& dst,
                           const statement& src) -> void {

        assert(src.is_expression());

        machine& x{tc.machine()};

        x.comment(src.tok(), indent, "= expression");

        src.compile(tc, indent, dst);
    }

    static auto asm_op_mul(toc& tc, const size_t indent, const destination& dst,
                           const statement& src) -> void {

        machine& x{tc.machine()};

        // a scratch register factor can be overwritten by the backend
        emit_with_source(
            tc, indent, src, dst.type_ref(),
            [&](const operand& factor, const bool is_scratch) -> void {
                const machine::factor_use use{
                    is_scratch ? machine::factor_use::may_overwrite
                               : machine::factor_use::read_only,
                };

                x.multiply(src.tok(), indent, dst.operand(), factor, use,
                           tc.checks().overflow_options());
            });
    }

    static auto asm_op_shift(toc& tc, const size_t indent,
                             const arithmetic_operator op,
                             const destination& dst, const statement& src)
        -> void {

        // note: a count wider than the value is rejected as narrowed also in
        //       the argument of a conversion, which narrows the other elements
        //       on purpose

        src.assert_not_narrowed(tc, dst.type_ref());

        machine& x{tc.machine()};

        emit_with_source(tc, indent, src, dst.type_ref(),
                         [&](const operand& count, const bool) -> void {
                             if (count.is_immediate()) {
                                 assert_shift_count_fits(src.tok(),
                                                         count.immediate(),
                                                         dst.type_ref());
                             }

                             x.shift(src.tok(), indent, op, dst.operand(),
                                     count, tc.checks().shift_options());
                         });
    }

    // a zero divisor traps on every target, so a constant one is an error like
    // a constant index out of bounds, e.g. 'x / 0' or 'x % (2 - 2)'
    static auto assert_divisor_not_zero(const token& src_loc_tk,
                                        const int64_t divisor) -> void {

        if (divisor != 0) {
            return;
        }

        throw compile_error{src_loc_tk, "divisor must not be zero"};
    }

    // a stored 'bool' is 0 or 1, arithmetic would make other values
    static auto assert_not_boolean(const toc& tc, const statement& term)
        -> void {

        const type& operand_type{
            term.is_name_or_literal() ? tc.make_ident_info(term).type_ref()
                                      : term.get_type(),
        };

        if (not operand_type.is_bool()) {
            return;
        }

        // the conversion goes inside the unary operators, e.g. '-int(t)'; an
        // indexed name is not the whole operand, e.g. 'bs' of 'bs[1]'
        const bool is_named{
            term.is_name_or_literal() and not term.is_indexed(),
        };

        const std::string_view converted{
            is_named ? term.name_or_literal() : "...",
        };

        // note: the list may be narrower than 'int', e.g. 'y + b' of an 'i8'
        //       'y' takes 'i8(b)'

        throw compile_error{
            term.tok(),
            std::format("a 'bool' is not an arithmetic operand, convert it, "
                        "e.g. 'int({})'",
                        converted)};
    }

    // the targets shift differently by a count outside the width, so a
    // constant one is an error, e.g. 'x << 8' of an 'i8'
    static auto assert_shift_count_fits(const token& src_loc_tk,
                                        const int64_t count,
                                        const type& width_type) -> void {

        const size_t width_bits{width_type.size_bits()};

        if (count >= 0 and std::cmp_less(count, width_bits)) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("shift count must be 0 to {} for {}-bit values",
                        width_bits - 1, width_bits)};
    }

    // an identifier copies itself, anything else is assigned with '='
    static auto compile_first_element(toc& tc, const size_t indent,
                                      const destination& dst,
                                      const statement& first) -> void {

        if (first.is_name_or_literal()) {
            first.compile(tc, indent, dst);
            return;
        }

        asm_op(tc, indent, arithmetic_operator::assign, dst, first);
    }

    [[nodiscard]] static auto compile_to_scratch(toc& tc, const size_t indent,
                                                 const statement& src,
                                                 const type& register_type)
        -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, register_type),
        };

        // note: the callers are the operators after the first element
        src.compile(tc, indent, destination::of_register(reg).as_operand());

        return reg;
    }

    // constants have the default type like literals
    [[nodiscard]] static auto constant_operand(const toc& tc,
                                               const int64_t value) -> operand {

        return operand::imm(value, tc.builtins().default_type());
    }

    static auto consume_peeked_operator(tokenizer& tz) -> void {
        const char ch{tz.next_char()};

        // 'peek_operator' recognized a shift by both of its characters
        // e.g. the second '<' of 'a << 2'
        if (ch == '<' or ch == '>') {
            tz.skip_char();
        }
    }

    // compiles 'src' into an operand the operation accepts, emits the
    // operation and frees the registers that producing the operand needed
    static auto emit_with_source(
        toc& tc, const size_t indent, const statement& src,
        const type& operand_type,
        const std::function_ref<void(const operand&, bool is_scratch)> emit)
        -> void {

        machine& x{tc.machine()};

        if (src.is_expression()) {
            // e.g. 'a / (1 + 1)'
            const std::optional<folded_value> folded{
                src.fold(tc, value_use::wrapped_at(operand_type)),
            };

            if (folded) {
                x.comment(src.tok(), indent, "src: folded constant '{}'",
                          statement::trimmed_source(src));

                emit(folded->operand_at(operand_type), false);

                return;
            }

            x.comment(src.tok(), indent, "src: expression");

            const operand reg{
                compile_to_scratch(tc, indent, src, operand_type),
            };

            emit(reg, true);
            x.free_scratch_register(src.tok(), indent, reg);

            return;
        }

        const ident_info src_info{tc.make_scalar_ident_info(src)};

        if (src_info.is_const()) {
            const std::optional<folded_value> folded{
                src.fold(tc, value_use::wrapped_at(operand_type)),
            };

            // note: a constant name or literal folds
            assert(folded);

            x.comment(src.tok(), indent, "src: constant");
            emit(folded->operand_at(operand_type), false);

            return;
        }

        std::vector<operand> lea_registers;

        const operand src_operand{
            src.get_lea_operand(tc, indent, src_info, lea_registers),
        };

        if (src.get_unary_ops().is_empty()) {
            x.comment(src.tok(), indent, "src: operand");
            emit(src_operand, false);
            x.free_scratch_registers(src.tok(), indent, lea_registers);
            return;
        }

        x.comment(src.tok(), indent, "src: operand with unary ops");

        const operand reg{
            compile_unary_to_scratch(tc, indent, src, src_operand,
                                     operand_type),
        };

        emit(reg, true);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }

    // rejects a constant divisor or shift count like 'compile'; none for a
    // quotient that the division check reports at run time
    [[nodiscard]] static auto
    fold_operation(const toc& tc, const int64_t lhs,
                   const arithmetic_operator op, const statement& rhs_element,
                   const int64_t rhs, const type& width_type)
        -> std::optional<int64_t> {

        if (op == arithmetic_operator::divide or
            op == arithmetic_operator::remainder) {

            assert_divisor_not_zero(rhs_element.tok(), rhs);

            if (tc.checks().division and
                constant_arith::division_overflows(lhs, rhs, width_type)) {

                return std::nullopt;
            }
        }

        if (op == arithmetic_operator::shift_left or
            op == arithmetic_operator::shift_right) {

            assert_shift_count_fits(rhs_element.tok(), rhs, width_type);
        }

        return constant_arith::apply_operation(lhs, op, rhs, width_type);
    }

    // the constant that leaves a value unchanged
    [[nodiscard]] static auto identity_of(const arithmetic_operator op)
        -> int64_t {

        if (op == arithmetic_operator::add or
            op == arithmetic_operator::bit_or or
            op == arithmetic_operator::bit_xor) {

            return 0;
        }

        if (op == arithmetic_operator::multiply or
            op == arithmetic_operator::divide) {

            return 1;
        }

        assert(op == arithmetic_operator::bit_and);

        return -1;
    }

    // a comparison operator, '<' and '>' are not shifts where an arithmetic
    // list ends
    [[nodiscard]] static auto is_comparison_next(const tokenizer& tz) -> bool {
        tokenizer probe{tz};
        std::ignore = probe.next_whitespace_token();

        if (probe.is_next_char('<') or probe.is_next_char('>')) {
            return true;
        }

        if (not probe.is_next_char('=') and not probe.is_next_char('!')) {
            return false;
        }

        return probe.is_next_char('=');
    }

    // a lone negation folds into add/sub: 'a - -b' is 'a + b'
    [[nodiscard]] static auto is_negated_operand(const toc& tc,
                                                 const statement& src) -> bool {

        return is_stored_value(tc, src) and
               src.get_unary_ops().is_only_negated();
    }

    // the constants of a run of the operator can be merged and moved to the
    // end of the run, '-' is the add of a negated value; not '/', '%' and
    // shifts, whose operands have fixed roles
    [[nodiscard]] static auto is_regroupable(const arithmetic_operator op)
        -> bool {

        return op == arithmetic_operator::add or
               op == arithmetic_operator::subtract or
               op == arithmetic_operator::multiply or
               op == arithmetic_operator::bit_and or
               op == arithmetic_operator::bit_or or
               op == arithmetic_operator::bit_xor;
    }

    // a register or memory location, neither computed nor constant
    [[nodiscard]] static auto is_stored_value(const toc& tc,
                                              const statement& src) -> bool {

        return not src.is_expression() and
               not tc.make_ident_info(src).is_const();
    }

    // pass 3: a subtracted or negated first element needs a negation so the
    // merged constant of an additive list leads instead, a zero constant adds
    // nothing so negating in place is shorter
    //   '- b + 3'  =>  '3 - b'   from '1 - b + 2'
    //   '-b + 23'  =>  '23 - b'
    //   '- b + 0'  =>  '- b + 0' from '2 - b - 2'
    static auto lead_with_constant(const toc& tc, std::vector<step>& steps)
        -> void {

        const step& first{steps.front()};
        const step& last{steps.back()};

        // only a merged constant has no element
        if (last.element != nullptr or last.op != arithmetic_operator::add or
            last.value == 0) {

            return;
        }

        const bool is_negated{
            first.op == arithmetic_operator::add and
                first.element != nullptr and
                is_negated_operand(tc, *first.element),
        };

        if (first.op != arithmetic_operator::subtract and not is_negated) {
            return;
        }

        std::ranges::rotate(steps, steps.end() - 1);
        // note: -1 moves the last step to the front
    }

    // pass 2: consecutive constant divisors merge into their product
    //   'a * 6 / 3 / 2 * 4'  =>  'a * 6 / 6 * 4'
    //   'b / 2 / c / 3'      =>  'b / 2 / c / 3'
    [[nodiscard]] static auto merge_divisors(const std::span<const step> steps,
                                             const type& width_type)
        -> std::vector<step> {

        std::vector<step> merged;

        for (const step& s : steps) {
            const std::optional<int64_t> product{
                merged.empty() ? std::nullopt
                               : merged_divisor(merged.back(), s, width_type),
            };

            if (not product) {
                merged.push_back(s);
                continue;
            }

            step& divisor{merged.back()};
            divisor.element = nullptr;
            divisor.value = product;
            append_source(divisor, s);
        }

        return merged;
    }

    // pass 1: the constants of a run of '+' and '-', of '*' or of one bitwise
    // operation merge into one constant at the end of the run, '/' and '%'
    // end a run since moving a constant across them changes the result, a
    // lower precedence operation continues the list left to right and starts
    // a new run
    //   '3 + a * 3 * 2 / 3 / 2 * 4 + 5'  =>  'a * 3 * 2 / 3 / 2 * 4 + 8'
    //   'a * 3 * 2 / 3 / 2 * 4'          =>  'a * 6 / 3 / 2 * 4'
    //   '2 * b * 3 / c'                  =>  'b * 6 / c'
    //   'b & 7 & 13'                     =>  'b & 5'
    //   'b & 1 | 2 | 4'                  =>  'b & 1 | 6'
    //   'b * 2 - 3 + c + 4'              =>  'b * 2 + c + 1'
    [[nodiscard]] static auto
    merge_regroupable_constants(const std::span<const step> steps,
                                const type& width_type) -> std::vector<step> {

        // a dividend or a divisor is a run of its own
        const auto same_run{
            [](const step& a, const step& b) -> bool {
                return is_regroupable(a.op) and is_regroupable(b.op) and
                       precedence_for_op(a.op) == precedence_for_op(b.op);
            },
        };

        std::vector<step> merged;

        for (const std::span<const step> run :
             steps | std::views::chunk_by(same_run)) {

            merge_run(merged, run, width_type);
        }

        return merged;
    }

    static auto merge_run(std::vector<step>& merged,
                          const std::span<const step> run,
                          const type& width_type) -> void {

        // subtracted constants are subtracted from an added constant
        const arithmetic_operator op{
            run.front().op == arithmetic_operator::subtract
                ? arithmetic_operator::add
                : run.front().op,
        };

        if (not is_regroupable(op)) {
            merged.append_range(run);
            return;
        }

        step constant{
            .op{op},
            .element{},
            .value{identity_of(op)},
            .folded_source{},
        };

        bool has_constant{};

        for (const step& s : run) {
            if (not s.value) {
                merged.push_back(s);
                continue;
            }

            constant.value = constant_arith::combine(*constant.value, s.op,
                                                     *s.value, width_type);

            append_source(constant, s);
            has_constant = true;
        }

        if (has_constant) {
            merged.push_back(std::move(constant));
        }
    }

    // a divisor of 0 or -1 is left to run time where it traps or wraps
    [[nodiscard]] static auto mergeable_divisor(const step& s)
        -> std::optional<int64_t> {

        if (s.op != arithmetic_operator::divide or not s.value or
            *s.value == 0 or *s.value == -1) {

            return std::nullopt;
        }

        return s.value;
    }

    // '(b / m) / n' is 'b / (m * n)' while the product fits the width
    [[nodiscard]] static auto merged_divisor(const step& divisor,
                                             const step& next,
                                             const type& width_type)
        -> std::optional<int64_t> {

        const std::optional<int64_t> current_divisor{
            mergeable_divisor(divisor),
        };

        const std::optional<int64_t> next_divisor{mergeable_divisor(next)};

        if (not current_divisor or not next_divisor) {
            return std::nullopt;
        }

        const int64_t product{
            constant_arith::combine(*current_divisor,
                                    arithmetic_operator::multiply,
                                    *next_divisor, width_type),
        };

        // a wrapped product divided back differs from the divisor
        if (product / *next_divisor != *current_divisor) {
            return std::nullopt;
        }

        return product;
    }

    // an element or a parenthesized sub-expression: '-a' vs '-(a + b)'
    [[nodiscard]] static auto parse_element(toc& tc, tokenizer& tz)
        -> parsed_element {

        // '-(a + b)' negates the list, so its unary ops come before the '('
        unary_ops uo{tz};

        // the unary ops apply to the whole sub-expression
        if (const token open_paren_tk{tz.is_next_char_token('(')};
            not open_paren_tk.is_empty()) {

            std::unique_ptr<statement> enclosed{
                std::make_unique<expr_arith>(
                    tc, tz,
                    options{
                        .uops{std::move(uo)},
                        .open_paren_tk{open_paren_tk},
                        .is_implied_subexpression{},
                        .first_op_precedence{initial_precedence},
                        .first_expression{},
                    }),
            };

            return {
                .node{std::move(enclosed)},
                .is_enclosed{true},
            };
        }

        // the element reads its own unary ops: '[-a] + b'
        uo.put_back(tz);

        std::unique_ptr<statement> element{
            create_statement_in_expr_arith(tc, tz),
        };

        if (not uo.is_empty()) {
            assert_not_boolean(tc, *element);
        }

        return {
            .node{std::move(element)},
            .is_enclosed{},
        };
    }

    // the operator at the next character, not consumed
    [[nodiscard]] static auto peek_operator(tokenizer& tz)
        -> std::optional<arithmetic_operator> {

        if (tz.is_peek_char('+')) {
            return arithmetic_operator::add;
        }

        if (tz.is_peek_char('-')) {
            return arithmetic_operator::subtract;
        }

        if (tz.is_peek_char('*')) {
            return arithmetic_operator::multiply;
        }

        if (tz.is_peek_char('/')) {
            return arithmetic_operator::divide;
        }

        if (tz.is_peek_char('%')) {
            return arithmetic_operator::remainder;
        }

        if (tz.is_peek_char('&')) {
            return arithmetic_operator::bit_and;
        }

        if (tz.is_peek_char('|')) {
            return arithmetic_operator::bit_or;
        }

        if (tz.is_peek_char('^')) {
            return arithmetic_operator::bit_xor;
        }

        // shifts are two characters stored as one
        if (tz.is_peek_char('<') and tz.is_peek_char2('<')) {
            return arithmetic_operator::shift_left;
        }

        if (tz.is_peek_char('>') and tz.is_peek_char2('>')) {
            return arithmetic_operator::shift_right;
        }

        return std::nullopt;
    }

    // a marker at the start of 'element', without its whitespace
    [[nodiscard]] static auto position_of(const statement& element) -> token {
        const token& tk{element.tok()};
        return token::position(tk.file_index(), tk.start_index(), tk.at_line());
    }

    // higher value higher precedence
    [[nodiscard]] static auto precedence_for_op(const arithmetic_operator op)
        -> uint8_t {

        if (op == arithmetic_operator::add or
            op == arithmetic_operator::subtract) {

            return precedence_additive_;
        }

        if (op == arithmetic_operator::multiply or
            op == arithmetic_operator::divide or
            op == arithmetic_operator::remainder) {

            return precedence_multiplicative_;
        }

        if (op == arithmetic_operator::bit_or) {
            return precedence_bitwise_or_;
        }

        if (op == arithmetic_operator::bit_and) {
            return precedence_bitwise_and_;
        }

        if (op == arithmetic_operator::bit_xor) {
            return precedence_bitwise_xor_;
        }

        assert(op == arithmetic_operator::shift_left or
               op == arithmetic_operator::shift_right);

        return precedence_shift_;
    }
};
