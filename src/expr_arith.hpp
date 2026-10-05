#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "decouple.hpp"
#include "expression.hpp"
#include "toc.hpp"

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
// note: a bit quirky parsing but compilation is trivial and register efficient
//
class expr_arith final : public expression {
    std::vector<std::unique_ptr<statement>> exprs_; // expression list
    std::vector<char> ops_; // operators between elements in the vector
    std::vector<token> ws_before_ops_; // whitespace before each of 'ops_'
    unary_ops uops_;                   // unary ops for all result e.g. ~(a+b)
    token open_paren_tk_;              // when 'enclosed' the '(' token
    token close_paren_tk_;             // when 'enclosed' the ')' token
    bool enclosed_{};                  // (a + b)  vs  a + b

    // true when this list was created because of a higher-precedence operation
    //   1 + 2 * 3 + 4  => 1 + [2 * 3] + 4
    bool is_implied_subexpression_{};

    // an operation of the list being folded: an element, or merged constants
    // when 'element' is null
    struct step {
        char op{};
        const statement* element{};
        std::optional<int64_t> value; // set when the step is a constant
        std::string folded_source;    // e.g. '* 3 * 2' for the comment
    };

    static constexpr char precedence_additive{1};
    static constexpr char precedence_multiplicative{2};
    static constexpr char precedence_bitwise_or{3};
    static constexpr char precedence_bitwise_and{4};
    static constexpr char precedence_bitwise_xor{5};
    static constexpr char precedence_shift{6};

  public:
    expr_arith(toc& tc, tokenizer& tz, const bool in_args = false,
               const bool enclosed = false, const token open_paren_tk = {},
               const bool is_implied_subexpression = false, unary_ops uops = {},
               const uint8_t first_op_precedence = initial_precedence,
               std::unique_ptr<statement> first_expression = {})
        : expression{tz.cur_position_token()}, uops_{std::move(uops)},
          open_paren_tk_{open_paren_tk}, enclosed_{enclosed},
          is_implied_subexpression_{is_implied_subexpression} {

        // a recursive call might have supplied the first element it already
        // parsed
        exprs_.emplace_back(first_expression ? std::move(first_expression)
                                             : parse_element(tc, tz, in_args));

        // set the type of this list same as first element

        const statement& first_expr{*exprs_.front()};

        set_type(first_expr.is_identifier()
                     ? tc.make_ident_info(first_expr).type_ref()
                     : first_expr.get_type());

        // start the loop of arithmetic operator and element

        // start with provided precedence
        uint8_t precedence{first_op_precedence};

        while (true) {

            // a sub-expression is closed by its ')'
            if (is_closed(tz)) {
                validate_arithmetic_operands(tc);
                return;
            }

            // an operator may start the next line after the trailing
            // whitespace of the previous element
            const token ws_before_op_tk{tz.next_whitespace_token()};

            // no operator also ends a list in function arguments at ',' or ')'
            const std::optional<char> next_op{peek_operator(tz)};
            if (not next_op) {
                end_list(tc, tz, ws_before_op_tk);
                return;
            }

            ops_.emplace_back(*next_op);

            const uint8_t next_precedence{precedence_for_op(*next_op)};

            // a higher precedence groups the previous element with what
            // follows, so parse that as a sub-expression
            if (next_precedence > precedence) {
                open_implied_subexpression(tc, tz, in_args, next_precedence,
                                           ws_before_op_tk);
                continue;
            }

            // a lower precedence returns to the parent list
            if (is_end_of_implied_subexpression(precedence, next_precedence)) {
                ops_.pop_back();
                end_list(tc, tz, ws_before_op_tk);
                return;
            }

            // possible new lower or same precedence
            precedence = next_precedence;

            // consume the peeked operator
            ws_before_ops_.emplace_back(ws_before_op_tk);
            const char ch{tz.next_char()};

            // consume the second character of a previously recognized shift
            // operator
            if (ch == '<' or ch == '>') {
                tz.skip_char();
            }

            exprs_.emplace_back(parse_element(tc, tz, in_args));

            // continue to next arithmetic op + element
        }
    }

    expr_arith() = default;

    // higher than the highest precedence
    static constexpr char initial_precedence{7};

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        if (enclosed_) {
            open_paren_tk_.source_to(os);
        }
        expression::source_to(os); // whitespace
        exprs_.at(0)->source_to(os);
        for (const auto [ws, o, e] : std::views::zip(
                 ws_before_ops_, ops_, exprs_ | std::views::drop(1))) {

            ws.source_to(os);
            std::print(os, "{}", o);
            if (o == '<' or o == '>') {
                // handle case << and >>
                std::print(os, "{}", o);
            }
            e->source_to(os);
        }

        if (enclosed_) {
            close_paren_tk_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        // a single constant is compiled as an identifier with its comments
        if (is_expression()) {
            const std::optional<int64_t> value{
                folded_constant(tc, dst_info.type_ref()),
            };

            if (value) {
                compile_constant(tc, indent, dst_info, *value,
                                 statement::trimmed_source(*this));

                return;
            }
        }

        // a register or a single plain element is compiled directly, without
        // trying with and without a scratch register
        if (dst_info.is_register() or is_single_plain_element()) {
            do_compile(tc, indent, dst_info);
            return;
        }

        machine& x{tc.machine()};

        // writing the destination early would change what later elements read
        if (reads_destination_early(dst_info)) {
            compile_through_scratch(tc, indent, dst_info);
            return;
        }

        // compile with and without the scratch register to find the best
        // compilation
        x.emit_most_efficient(
            tok(), indent, [&] -> void { do_compile(tc, indent, dst_info); },
            [&] -> void { compile_through_scratch(tc, indent, dst_info); });
    }

    [[nodiscard]] auto accessed_range() const
        -> std::optional<field_coverage::range> override {

        assert(exprs_.size() == 1);

        return exprs_.at(0)->accessed_range();
    }

    // each element is computed at the width of the destination
    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        exprs_.front()->assert_not_narrowed(tc, dst_type);

        for (const auto [o, e] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            // a shift count is not stored in the destination
            if (o == '<' or o == '>') {
                continue;
            }

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
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
        -> operand override {

        assert(exprs_.size() == 1);

        return exprs_.at(0)->compile_lea(tc, indent, src_loc_tk,
                                         allocated_registers, reg_count,
                                         lea_path, address_register);
    }

    [[nodiscard]] auto folded_constant(const toc& tc,
                                       const type& width_type) const
        -> std::optional<int64_t> override {

        std::optional<int64_t> value{
            element_constant(tc, *exprs_.front(), width_type),
        };

        for (const auto [o, e] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            if (not value) {
                return std::nullopt;
            }

            const std::optional<int64_t> rhs{
                element_constant(tc, *e, width_type),
            };

            if (not rhs) {
                return std::nullopt;
            }

            value = apply_operation(*value, o, *rhs, width_type);
        }

        if (not value) {
            return std::nullopt;
        }

        return wrap_to_width(uops_.evaluate_constant(*value), width_type);
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        // note: the unary ops of a multi-element list are never asked for,
        //       they are compiled with the list
        assert(exprs_.size() == 1);

        return exprs_.at(0)->get_unary_ops();
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(exprs_.size() == 1);

        return exprs_.at(0)->identifier();
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        return exprs_.size() == 1 and exprs_.front()->is_array_element();
    }

    [[nodiscard]] auto is_expr_arith() const -> bool override { return true; }

    [[nodiscard]] auto is_expression() const -> bool override {
        // if unary operators on the list then it will need to compile the
        // expression
        if (not uops_.is_empty()) {
            return true;
        }

        // if only 1 element, then it decides if it is an expression
        if (exprs_.size() == 1) {
            return exprs_.at(0)->is_expression();
        }

        // more than 1 element, automatically an expression
        return true;
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return exprs_.size() == 1 and exprs_.at(0)->is_identifier();
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        assert(exprs_.size() == 1);

        return exprs_.at(0)->is_indexed();
    }

    // '/', '%' and '>>' read the high bits of their operands
    [[nodiscard]] auto keeps_low_bits_when_narrowed() const -> bool override {
        for (const char o : ops_) {
            if (o == '/' or o == '%' or o == '>') {
                return false;
            }
        }

        for (const std::unique_ptr<statement>& e : exprs_) {
            if (not e->keeps_low_bits_when_narrowed()) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return uops_.is_empty() and exprs_.size() == 1 and
               exprs_.front()->produces_boolean();
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

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
                                    const ident_info& dst_info) const
        -> std::optional<int64_t> {

        // note: only an index expression asks, it has no unary ops of its own
        assert(uops_.is_empty());

        const type& width_type{dst_info.type_ref()};

        const std::vector<step> steps{plan_steps(tc, width_type)};

        const step& last{steps.back()};

        if (steps.size() < 2 or last.element != nullptr or last.op != '+') {
            return std::nullopt;
        }

        // note: 2 steps are the least with something to add the constant to

        compile_first_step(tc, indent, dst_info, steps.front());

        const std::span<const step> middle_steps{
            std::span{steps}.subspan(1, steps.size() - 2),
        };

        // note: the 1 skips the first step compiled above and the 2 also
        //       leaves out the trailing constant

        for (const step& s : middle_steps) {
            compile_step(tc, indent, dst_info, s);
        }

        return last.value;
    }

    // e.g. 'flag', 'p', 'f(x)' or '-i32(x)', a parenthesized list is
    // arithmetic of its own
    [[nodiscard]] auto is_single_operand() const -> bool {
        if (exprs_.size() != 1) {
            return false;
        }

        return not exprs_.front()->is_expr_arith();
    }

    // the list type of a folded 'i8(-3)' is the default type of a constant
    [[nodiscard]] auto single_operand_type() const -> const type& {
        assert(is_single_operand());

        return exprs_.front()->get_type();
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
        src.get_unary_ops().compile(tc, indent, src.tok(), reg);

        return reg;
    }

  private:
    // e.g. 'b + 1 - 1' or 'b / 1 / 1' apply nothing
    auto apply_merged_constant(toc& tc, const size_t indent, const char op,
                               const ident_info& dst_info, const int64_t value,
                               const std::string_view folded_source) const
        -> void {

        if (value == identity_of(op)) {
            machine& x{tc.machine()};

            x.comment(tok(), indent,
                      "src: folded constant '{}' is {} and changes nothing",
                      folded_source, value);

            return;
        }

        asm_op_constant(tc, indent, op, dst_info, value, folded_source);
    }

    auto asm_op_constant(toc& tc, const size_t indent, const char op,
                         const ident_info& dst_info, const int64_t value,
                         const std::string_view folded_source) const -> void {

        // 'b - 3' rather than 'b + -3', the most negative value has no
        // positive counterpart in the width
        if (op == '+' and value < 0 and
            value != width_min(dst_info.type_ref())) {

            asm_op_constant(tc, indent, '-', dst_info, -value, folded_source);
            return;
        }

        machine& x{tc.machine()};

        x.comment(tok(), indent, "{} {} {}", dst_info.id, op, value);
        x.comment(tok(), indent, "src: folded constant '{}'", folded_source);

        const operand constant{constant_operand(tc, value)};

        switch (op) {
        case '+':
        case '-':
            x.add_subtract(tok(), indent, op, dst_info.operand, constant);
            return;

        case '*':
            x.multiply(tok(), indent, dst_info.operand, constant);
            return;

        case '/':
            x.divide(tok(), indent, op, dst_info.operand, constant);
            return;

        case '&':
        case '|':
        case '^':
            x.bitwise(tok(), indent, op, dst_info.operand, constant);
            return;

        default:
            std::unreachable();
        }
    }

    // a narrow memory destination truncates when stored, which gives the same
    // value when the low bits do not depend on the high bits
    [[nodiscard]] auto can_compute_wide(const toc& tc,
                                        const ident_info& dst_info) const
        -> bool {

        // note: a register destination is compiled directly by 'compile'
        assert(dst_info.operand.is_memory());

        if (dst_info.type_ref().size_bytes() >=
            tc.get_type_default().size_bytes()) {
            return false;
        }

        return keeps_low_bits_when_narrowed();
    }

    auto compile_constant(toc& tc, const size_t indent,
                          const ident_info& dst_info, const int64_t value,
                          const std::string_view folded_source) const -> void {

        machine& x{tc.machine()};

        x.comment(tok(), indent, "{} = {}", dst_info.id, value);
        x.comment(tok(), indent, "src: folded constant '{}'", folded_source);

        x.copy_value(tok(), indent, dst_info.operand,
                     constant_operand(tc, value));
    }

    // a nested list is folded when it is compiled as an element:
    //   '3 + a * 3 * 2 / 3 / 2 * 4 + 5'  =>  'a * 6 / 6 * 4 + 8'
    auto compile_elements(toc& tc, const size_t indent,
                          const ident_info& dst_info) const -> void {

        const type& width_type{dst_info.type_ref()};

        const std::vector<step> steps{plan_steps(tc, width_type)};

        compile_first_step(tc, indent, dst_info, steps.front());

        for (const step& s : std::span{steps}.subspan(1)) {
            // note: 1 because the first step is compiled above

            compile_step(tc, indent, dst_info, s);
        }
    }

    auto compile_first_step(toc& tc, const size_t indent,
                            const ident_info& dst_info, const step& first) const
        -> void {

        // e.g. '2 * 3 / b' or '3 - b'
        if (first.element == nullptr) {
            assert(first.value);

            compile_constant(tc, indent, dst_info, *first.value,
                             first.folded_source);

            return;
        }

        compile_first_element(tc, indent, dst_info, *first.element);

        // only a zero constant leaves a subtracted element first: '2 - b - 2'
        if (first.op == '-') {
            machine& x{tc.machine()};

            x.unary(tok(), indent, '-', dst_info.operand);
        }
    }

    auto compile_step(toc& tc, const size_t indent, const ident_info& dst_info,
                      const step& s) const -> void {

        if (s.element == nullptr and s.value) {
            apply_merged_constant(tc, indent, s.op, dst_info, *s.value,
                                  s.folded_source);

            return;
        }

        asm_op(tc, indent, s.op, dst_info, *s.element);
    }

    // the destination keeps its value until the whole expression is computed
    auto compile_through_scratch(toc& tc, const size_t indent,
                                 const ident_info& dst_info) const -> void {

        const type& dst_type{dst_info.type_ref()};

        if (not can_compute_wide(tc, dst_info)) {
            compile_through_scratch_as(tc, indent, dst_info, dst_type);
            return;
        }

        // a target without narrow register operations normalizes a narrow
        // register after each operation, a wide register needs only the store
        machine& x{tc.machine()};

        x.emit_most_efficient(
            tok(), indent,
            [&] -> void {
                compile_through_scratch_as(tc, indent, dst_info, dst_type);
            },
            [&] -> void {
                compile_through_scratch_as(tc, indent, dst_info,
                                           tc.get_type_default());
            });
    }

    auto compile_through_scratch_as(toc& tc, const size_t indent,
                                    const ident_info& dst_info,
                                    const type& scratch_type) const -> void {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(tok(), indent, scratch_type),
        };

        do_compile(tc, indent, toc::make_ident_info_from_register(reg));

        x.copy_value(tok(), indent, dst_info.operand, reg);

        x.free_scratch_register(tok(), indent, reg);
    }

    auto do_compile(toc& tc, const size_t indent,
                    const ident_info& dst_info) const -> void {

        compile_elements(tc, indent, dst_info);

        // apply unary expressions on destination
        uops_.compile(tc, indent, tok(), dst_info.operand);
    }

    // 'ws_before_op_tk' belongs to what follows the list
    auto end_list(const toc& tc, tokenizer& tz,
                  const token& ws_before_op_tk) const -> void {

        tz.put_back_token(ws_before_op_tk);
        validate_arithmetic_operands(tc);
    }

    // the first element is added in an additive list and a factor in a list
    // led by '*' or a bitwise operation, otherwise it is the dividend or the
    // shifted value
    [[nodiscard]] auto first_op() const -> char {
        if (ops_.empty()) {
            return '=';
        }

        const char first{ops_.front()};
        if (first == '-') {
            return '+';
        }

        return is_commutative(first) ? first : '=';
    }

    // a sub-expression is enclosed in parentheses
    [[nodiscard]] auto is_closed(tokenizer& tz) -> bool {
        if (not enclosed_) {
            return false;
        }

        close_paren_tk_ = tz.is_next_char_token(')');

        return not close_paren_tk_.is_empty();
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
        if (not e.is_identifier()) {
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
            const char op{i == 0 ? first_op() : ops_.at(i - 1)};
            // note: -1 because the first expression has no operator before it

            const statement& e{*exprs_.at(i)};

            steps.push_back({
                .op{op},
                .element{&e},
                .value{element_constant(tc, e, width_type)},
                .folded_source{
                    std::format("{} {}", op, statement::trimmed_source(e)),
                },
            });
        }

        return steps;
    }

    // moves the last element to the front of a sub-expression that takes the
    // operator with the higher precedence
    //   a + b * c  ->  [a] + [b * c]
    auto open_implied_subexpression(toc& tc, tokenizer& tz, const bool in_args,
                                    const uint8_t next_precedence,
                                    const token& ws_before_op_tk) -> void {

        // the operator is parsed by the sub-expression before its second
        // element
        ops_.pop_back();
        tz.put_back_token(ws_before_op_tk);

        std::unique_ptr<statement> last_elem_in_list{std::move(exprs_.back())};

        exprs_.pop_back();

        // the sub-expression continues with the precedence of the operator
        exprs_.emplace_back(make_unique<expr_arith>(
            tc, tz, in_args, false, token{}, true, unary_ops{}, next_precedence,
            std::move(last_elem_in_list)));
    }

    // the passes in order, the steps are ready to compile
    [[nodiscard]] auto plan_steps(const toc& tc, const type& width_type) const
        -> std::vector<step> {

        std::vector<step> steps{make_steps(tc, width_type)};
        steps = merge_commutative_constants(steps, width_type);
        steps = merge_divisors(steps, width_type);
        lead_with_constant(tc, steps);

        return steps;
    }

    // a plain first element is copied before anything writes the destination
    [[nodiscard]] auto reads_destination_early(const ident_info& dst_info) const
        -> bool {

        const std::string_view root{dst_info.root_id()};

        if (not exprs_.front()->is_identifier() and
            exprs_.front()->reads_var(root)) {

            return true;
        }

        return std::ranges::any_of(
            exprs_ | std::views::drop(1),
            [root](const std::unique_ptr<statement>& e) -> bool {
                return e->reads_var(root);
            });
    }

    auto validate_arithmetic_operands(const toc& tc) const -> void {
        if (ops_.empty()) {
            return;
        }

        for (const std::unique_ptr<statement>& expr : exprs_) {
            const type& expr_type{
                expr->is_identifier() ? tc.make_ident_info(*expr).type_ref()
                                      : expr->get_type(),
            };

            if (expr_type.is_bool()) {
                throw compiler_exception{
                    expr->tok(),
                    "boolean values cannot be arithmetic operands"};
            }
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

    // empty when the targets differ at run time: a zero divisor traps, the
    // most negative value divided by -1 traps or wraps and shift counts
    // outside the width are masked differently
    [[nodiscard]] static auto apply_operation(const int64_t lhs, const char op,
                                              const int64_t rhs,
                                              const type& width_type)
        -> std::optional<int64_t> {

        switch (op) {
        case '/':
        case '%':
            if (rhs == 0 or (rhs == -1 and lhs == width_min(width_type))) {
                return std::nullopt;
            }

            return wrap_to_width(op == '/' ? lhs / rhs : lhs % rhs, width_type);

        case '<':
        case '>':
            if (rhs < 0 or
                std::cmp_greater_equal(rhs, width_bits(width_type))) {
                return std::nullopt;
            }

            return shift_constant(lhs, op, static_cast<uint64_t>(rhs),
                                  width_type);

        default:
            return combine(lhs, op, rhs, width_type);
        }
    }

    static auto asm_op(toc& tc, const size_t indent, const char op,
                       const ident_info& dst, const statement& src) -> void {

        // shifts are stored as one character but written as two
        std::string op_str{op};
        if (op == '<' or op == '>') {
            op_str.push_back(op);
        }

        machine& x{tc.machine()};

        x.comment(src.tok(), indent,
                  statement::trimmed_source(src, dst.id, op_str));

        switch (op) {
        case '=':
            asm_op_mov(tc, indent, dst, src);
            return;

        case '+':
        case '-':
            asm_op_add_sub(tc, indent, op, dst, src);
            return;

        case '*':
            asm_op_mul(tc, indent, dst, src);
            return;

        case '/':
        case '%':
            asm_op_div(tc, indent, op, dst, src);
            return;

        case '&':
        case '|':
        case '^':
            asm_op_bitwise(tc, indent, op, dst, src);
            return;

        case '<':
        case '>':
            asm_op_shift(tc, indent, op, dst, src);
            return;

        default:
            std::unreachable();
        }
    }

    static auto asm_op_add_sub(toc& tc, const size_t indent, const char op,
                               const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        if (is_negated_operand(tc, src)) {
            x.comment(src.tok(), indent, "src: negated operand");

            const ident_info src_info{tc.make_scalar_ident_info(src)};
            std::vector<operand> lea_registers;
            const operand src_operand{
                tc.get_lea_operand(indent, src, src_info, lea_registers),
            };

            x.add_subtract(src.tok(), indent, op == '+' ? '-' : '+',
                           dst_info.operand, src_operand);

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        emit_with_source(
            tc, indent, src, dst_info.type_ref(), tc.get_type_default(),
            [&](const operand& term, const bool) -> void {
                x.add_subtract(src.tok(), indent, op, dst_info.operand, term);
            });
    }

    static auto asm_op_bitwise(toc& tc, const size_t indent, const char op,
                               const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        emit_with_source(
            tc, indent, src, dst_info.type_ref(), tc.get_type_default(),
            [&](const operand& value, const bool) -> void {
                x.bitwise(src.tok(), indent, op, dst_info.operand, value);
            });
    }

    static auto asm_op_div(toc& tc, const size_t indent, const char op,
                           const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // the backend reserves registers for division
        if (is_stored_value(tc, src)) {
            x.validate_division_operand(src.tok(),
                                        tc.make_ident_info(src).operand);
        }

        emit_with_source(
            tc, indent, src, dst_info.type_ref(), dst_info.type_ref(),
            [&](const operand& divisor, const bool) -> void {
                x.divide(src.tok(), indent, op, dst_info.operand, divisor);
            });
    }

    // 'compile_first_element' copies identifiers itself
    static auto asm_op_mov(toc& tc, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        assert(src.is_expression());

        machine& x{tc.machine()};

        x.comment(src.tok(), indent, "= expression");

        src.compile(tc, indent, dst_info);
    }

    static auto asm_op_mul(toc& tc, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // a scratch register factor can be overwritten by the backend
        emit_with_source(
            tc, indent, src, dst_info.type_ref(), dst_info.type_ref(),
            [&](const operand& factor, const bool is_scratch) -> void {
                x.multiply(src.tok(), indent, dst_info.operand, factor,
                           is_scratch);
            });
    }

    static auto asm_op_shift(toc& tc, const size_t indent, const char op,
                             const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // the backend reserves a register for the count
        if (is_stored_value(tc, src)) {
            x.validate_shift_operand(src.tok(),
                                     tc.make_ident_info(src).operand);
        }

        emit_with_source(
            tc, indent, src, dst_info.type_ref(), dst_info.type_ref(),
            [&](const operand& count, const bool) -> void {
                x.shift(src.tok(), indent, op, dst_info.operand, count);
            });
    }

    // operations whose low bits do not depend on higher bits
    [[nodiscard]] static auto combine(const int64_t lhs, const char op,
                                      const int64_t rhs, const type& width_type)
        -> int64_t {

        // unsigned arithmetic wraps like the registers
        const uint64_t l{static_cast<uint64_t>(lhs)};
        const uint64_t r{static_cast<uint64_t>(rhs)};

        switch (op) {
        case '+':
            return wrap_to_width(static_cast<int64_t>(l + r), width_type);

        case '-':
            return wrap_to_width(static_cast<int64_t>(l - r), width_type);

        case '*':
            return wrap_to_width(static_cast<int64_t>(l * r), width_type);

        case '&':
            return static_cast<int64_t>(l & r);

        case '|':
            return static_cast<int64_t>(l | r);

        case '^':
            return static_cast<int64_t>(l ^ r);

        default:
            std::unreachable();
        }
    }

    // an identifier copies itself, anything else is assigned with '='
    static auto compile_first_element(toc& tc, const size_t indent,
                                      const ident_info& dst_info,
                                      const statement& first) -> void {

        if (first.is_identifier()) {
            first.compile(tc, indent, dst_info);
            return;
        }

        asm_op(tc, indent, '=', dst_info, first);
    }

    [[nodiscard]] static auto compile_to_scratch(toc& tc, const size_t indent,
                                                 const statement& src,
                                                 const type& register_type)
        -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, register_type),
        };

        src.compile(tc, indent, toc::make_ident_info_from_register(reg));

        return reg;
    }

    // constants have the default type like literals
    [[nodiscard]] static auto constant_operand(const toc& tc,
                                               const int64_t value) -> operand {

        return operand::imm(std::format("{}", value), tc.get_type_default());
    }

    // a constant identifier or a list of constants
    [[nodiscard]] static auto
    element_constant(const toc& tc, const statement& e, const type& width_type)
        -> std::optional<int64_t> {

        if (e.is_expression()) {
            return e.folded_constant(tc, width_type);
        }

        const ident_info info{tc.make_ident_info(e)};
        if (not info.is_const()) {
            return std::nullopt;
        }

        return wrap_to_width(
            e.get_unary_ops().evaluate_constant(info.const_value), width_type);
    }

    // compiles 'src' into an operand the operation accepts, emits the
    // operation and frees the registers that producing the operand needed
    static auto emit_with_source(
        toc& tc, const size_t indent, const statement& src,
        const type& expression_type, const type& unary_type,
        const std::function_ref<void(const operand&, bool is_scratch)> emit)
        -> void {

        machine& x{tc.machine()};

        if (src.is_expression()) {
            // e.g. 'a / (1 + 1)'
            const std::optional<int64_t> value{
                src.folded_constant(tc, expression_type),
            };

            if (value) {
                x.comment(src.tok(), indent, "src: folded constant '{}'",
                          statement::trimmed_source(src));

                emit(constant_operand(tc, *value), false);

                return;
            }

            x.comment(src.tok(), indent, "src: expression");

            const operand reg{
                compile_to_scratch(tc, indent, src, expression_type),
            };

            emit(reg, true);
            x.free_scratch_register(src.tok(), indent, reg);

            return;
        }

        const ident_info src_info{tc.make_scalar_ident_info(src)};
        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "src: constant");
            emit(src.make_constant_operand(src_info), false);

            return;
        }

        std::vector<operand> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(indent, src, src_info, lea_registers),
        };

        if (src.get_unary_ops().is_empty()) {
            x.comment(src.tok(), indent, "src: operand");
            emit(src_operand, false);
            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        x.comment(src.tok(), indent, "src: operand with unary ops");

        const operand reg{
            compile_unary_to_scratch(tc, indent, src, src_operand, unary_type),
        };

        emit(reg, true);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }

    // the constant that leaves a value unchanged
    [[nodiscard]] static auto identity_of(const char op) -> int64_t {
        switch (op) {
        case '+':
        case '|':
        case '^':
            return 0;

        case '*':
        case '/':
            return 1;

        case '&':
            return -1;

        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto is_commutative(const char op) -> bool {
        return op == '+' or op == '-' or op == '*' or op == '&' or op == '|' or
               op == '^';
    }

    // a lone negation folds into add/sub: 'a - -b' is 'a + b'
    [[nodiscard]] static auto is_negated_operand(const toc& tc,
                                                 const statement& src) -> bool {

        return is_stored_value(tc, src) and
               src.get_unary_ops().is_only_negated();
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
        if (last.element != nullptr or last.op != '+' or last.value == 0) {
            return;
        }

        const bool is_negated{
            first.op == '+' and first.element != nullptr and
                is_negated_operand(tc, *first.element),
        };

        if (first.op != '-' and not is_negated) {
            return;
        }

        std::ranges::rotate(steps, steps.end() - 1);
        // note: -1 moves the last step to the front
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
    merge_commutative_constants(const std::span<const step> steps,
                                const type& width_type) -> std::vector<step> {

        // a dividend or a divisor is a run of its own
        const auto same_run{
            [](const step& a, const step& b) -> bool {
                return is_commutative(a.op) and is_commutative(b.op) and
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

    static auto merge_run(std::vector<step>& merged,
                          const std::span<const step> run,
                          const type& width_type) -> void {

        // subtracted constants are subtracted from an added constant
        const char op{run.front().op == '-' ? '+' : run.front().op};

        if (not is_commutative(op)) {
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

            constant.value =
                combine(*constant.value, s.op, *s.value, width_type);

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

        if (s.op != '/' or not s.value or *s.value == 0 or *s.value == -1) {
            return std::nullopt;
        }

        return s.value;
    }

    // '(b / m) / n' is 'b / (m * n)' while the product fits the width
    [[nodiscard]] static auto merged_divisor(const step& divisor,
                                             const step& next,
                                             const type& width_type)
        -> std::optional<int64_t> {

        const std::optional<int64_t> m{mergeable_divisor(divisor)};
        const std::optional<int64_t> n{mergeable_divisor(next)};

        if (not m or not n) {
            return std::nullopt;
        }

        const int64_t product{combine(*m, '*', *n, width_type)};

        // a wrapped product divided back differs from the divisor
        if (product / *n != *m) {
            return std::nullopt;
        }

        return product;
    }

    [[nodiscard]] static auto parse_element(toc& tc, tokenizer& tz,
                                            const bool in_args)
        -> std::unique_ptr<statement> {

        // read the unary ops before checking for open parenthesis
        unary_ops uo{tz};

        // the unary ops apply to the whole sub-expression
        if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
            return std::make_unique<expr_arith>(tc, tz, in_args, true, t, false,
                                                std::move(uo));
        }

        // the element reads its own unary ops: '[-a] + b'
        uo.put_back(tz);

        return create_statement_in_expr_arith(tc, tz);
    }

    // an element or a parenthesized sub-expression: '-a' vs '-(a + b)'
    // the operator at the next character, not consumed
    [[nodiscard]] static auto peek_operator(tokenizer& tz)
        -> std::optional<char> {

        constexpr std::string_view single_char_operators{"+-*/%&|^"};

        for (const char op : single_char_operators) {
            if (tz.is_peek_char(op)) {
                return op;
            }
        }

        // shifts are two characters stored as one
        if (tz.is_peek_char('<') and tz.is_peek_char2('<')) {
            return '<';
        }

        if (tz.is_peek_char('>') and tz.is_peek_char2('>')) {
            return '>';
        }

        return std::nullopt;
    }

    // higher value higher precedence
    [[nodiscard]] static auto precedence_for_op(const char ch) -> uint8_t {
        switch (ch) {
        case '+':
        case '-':
            return precedence_additive;

        case '*':
        case '/':
        case '%':
            return precedence_multiplicative;

        case '|':
            return precedence_bitwise_or;

        case '&':
            return precedence_bitwise_and;

        case '^':
            return precedence_bitwise_xor;

        case '<': // shift left
        case '>': // shift right
            return precedence_shift;

        default:
            std::unreachable();
        }
    }

    // a count within the width shifts the same on both targets
    [[nodiscard]] static auto shift_constant(const int64_t lhs, const char op,
                                             const uint64_t count,
                                             const type& width_type)
        -> int64_t {

        const uint64_t bits{static_cast<uint64_t>(lhs)};

        if (op == '<') {
            return wrap_to_width(static_cast<int64_t>(bits << count),
                                 width_type);
        }

        // 'lhs' is sign extended so shifting in its sign bit keeps the width
        if (lhs < 0) {
            return static_cast<int64_t>(~(~bits >> count));
        }

        return static_cast<int64_t>(bits >> count);
    }

    [[nodiscard]] static auto width_bits(const type& width_type) -> size_t {
        constexpr size_t byte_bits{8};
        return width_type.size_bytes() * byte_bits;
    }

    // e.g. -128 for 'i8'
    [[nodiscard]] static auto width_min(const type& width_type) -> int64_t {
        return static_cast<int64_t>(~uint64_t{}
                                    << (width_bits(width_type) - 1));
        // note: -1 because the sign bit is the highest bit
    }

    // the value a register of 'width_type' holds, sign extended
    [[nodiscard]] static auto wrap_to_width(const int64_t value,
                                            const type& width_type) -> int64_t {

        const size_t bits{width_bits(width_type)};
        if (bits >= std::numeric_limits<uint64_t>::digits) {
            return value;
        }

        const uint64_t sign_bit{uint64_t{1} << (bits - 1)};
        // note: -1 because the sign bit is the highest bit

        const uint64_t low{
            static_cast<uint64_t>(value) & ((sign_bit << 1U) - 1U),
        };

        // flipping and subtracting the sign bit extends it
        return static_cast<int64_t>((low ^ sign_bit) - sign_bit);
    }
};
