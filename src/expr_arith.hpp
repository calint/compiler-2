#pragma once
// reviewed: 2025-09-28

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

            if (enclosed_) {
                // this is a sub-expression, check if it is closed
                close_paren_tk_ = tz.is_next_char_token(')');
                if (not close_paren_tk_.is_empty()) {
                    // it is closed
                    // validate that it is arithmetic containing no bools
                    validate_arithmetic_operands(tc);

                    // return from recursion
                    return;
                }
            }

            // an operator may start the next line after the trailing
            // whitespace of the previous element
            const token ws_before_op_tk{tz.next_whitespace_token()};

            // is it parsed within a function argument?
            if (in_args) {
                // yes, exit when ',' or ')' is found
                if (tz.is_peek_char(',') or tz.is_peek_char(')')) {
                    tz.put_back_token(ws_before_op_tk);
                    validate_arithmetic_operands(tc);

                    return;
                }
            }

            // next arithmetic operation

            if (tz.is_peek_char('+')) {
                ops_.emplace_back('+');
            } else if (tz.is_peek_char('-')) {
                ops_.emplace_back('-');
            } else if (tz.is_peek_char('*')) {
                ops_.emplace_back('*');
            } else if (tz.is_peek_char('/')) {
                ops_.emplace_back('/');
            } else if (tz.is_peek_char('%')) {
                ops_.emplace_back('%');
            } else if (tz.is_peek_char('&')) {
                ops_.emplace_back('&');
            } else if (tz.is_peek_char('|')) {
                ops_.emplace_back('|');
            } else if (tz.is_peek_char('^')) {
                ops_.emplace_back('^');
            } else if (tz.is_peek_char('<') and tz.is_peek_char2('<')) {
                ops_.emplace_back('<');
            } else if (tz.is_peek_char('>') and tz.is_peek_char2('>')) {
                ops_.emplace_back('>');
            } else {
                // no more operations, return
                tz.put_back_token(ws_before_op_tk);
                validate_arithmetic_operands(tc);

                return;
            }

            // check if precedence increased in which case open an implied
            // sub-expression
            //   a + b * c  ->  [a] + [b * c]

            const uint8_t next_precedence{precedence_for_op(ops_.back())};
            if (next_precedence > precedence) {
                // last read operator has higher precedence than previous
                // create an implied sub-expression
                // move the last element to be the first element of the
                // sub-expression

                // remove the operator which has higher precedence, it will be
                // parsed in the sub-expression before the second element
                ops_.pop_back();
                tz.put_back_token(ws_before_op_tk);

                // move the last element out of the list
                std::unique_ptr<statement> last_elem_in_list{
                    std::move(exprs_.back())};

                exprs_.pop_back();

                // forward it to the sub-expression including its precedence
                exprs_.emplace_back(make_unique<expr_arith>(
                    tc, tz, in_args, false, token{}, true, unary_ops{},
                    next_precedence, std::move(last_elem_in_list)));

                // continue parsing when the precedence has been lowered
                continue;
            }

            // is this in an implied sub-expression and precedence has gone
            // lower?

            if (precedence != initial_precedence and
                next_precedence < precedence and is_implied_subexpression_) {

                // lower precedence returns to the parent list
                //   want:  a - b * c + 3  ->  [a] - [b * c] + [3]
                //   if not returning then becomes: a - [b * c + 3]

                // remove the operator that has lower precedence, it will be
                // parsed by the parent expression
                ops_.pop_back();
                tz.put_back_token(ws_before_op_tk);

                validate_arithmetic_operands(tc);

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
                (void)tz.next_char();
            }

            exprs_.emplace_back(parse_element(tc, tz, in_args));

            // continue to next arithmetic op + element
        }
    }

    expr_arith() = default;

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        if (enclosed_) {
            open_paren_tk_.source_to(os);
        }
        expression::source_to(os); // whitespace
        exprs_[0]->source_to(os);
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
                folded_constant(tc, dst_info.type_ref())};

            if (value) {
                compile_constant(tc, indent, dst_info, *value,
                                 statement::trimmed_source(*this));

                return;
            }
        }

        // is destination a register or a single element without unary ops?
        if (dst_info.is_register() or is_single_plain_element()) {
            // yes, compile without trying with and without scratch register
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

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return uops_.is_empty() and exprs_.size() == 1 and
               exprs_.front()->produces_boolean();
    }

    auto compile_boolean(toc& tc, const size_t indent, const operand& dst,
                         const bool inverted) const -> void override {

        assert(produces_boolean());

        exprs_.front()->compile_boolean(tc, indent, dst, inverted);
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        return exprs_.size() == 1 and exprs_.front()->is_array_element();
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(exprs_.size() == 1);

        return exprs_[0]->identifier();
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        // is this list one element?
        if (exprs_.size() == 1) {
            // then the unary ops are on the first element
            return exprs_[0]->get_unary_ops();
        }

        // in the multi-element list, unary ops for all are on the current list
        // element
        return uops_;
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        // if unary operators on the list then it will need to compile the
        // expression
        if (not uops_.is_empty()) {
            return true;
        }

        // if only 1 element, then it decides if it is an expression
        if (exprs_.size() == 1) {
            return exprs_[0]->is_expression();
        }

        // more than 1 element, automatically an expression
        return true;
    }

    [[nodiscard]] auto folded_constant(const toc& tc,
                                       const type& width_type) const
        -> std::optional<int64_t> override {

        std::optional<int64_t> value{
            element_constant(tc, *exprs_.front(), width_type)};

        for (const auto [o, e] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            if (not value) {
                return std::nullopt;
            }

            const std::optional<int64_t> rhs{
                element_constant(tc, *e, width_type)};

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

    [[nodiscard]] auto is_indexed() const -> bool override {
        assert(exprs_.size() == 1);

        return exprs_[0]->is_indexed();
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        for (const std::unique_ptr<statement>& e : exprs_) {
            e->visit_reads(var, reader);
        }
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return exprs_.size() == 1 and exprs_[0]->is_identifier();
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

    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
        -> operand override {

        assert(exprs_.size() == 1);

        return exprs_[0]->compile_lea(tc, indent, src_loc_tk,
                                      allocated_registers, reg_count, lea_path,
                                      address_register);
    }

    // the caller frees the returned register
    [[nodiscard]] static auto
    compile_unary_to_scratch(toc& tc, const size_t indent, const statement& src,
                             const operand& src_operand,
                             const type& register_type) -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, register_type)};

        x.copy_value(src.tok(), indent, reg, src_operand);
        src.get_unary_ops().compile(tc, indent, reg);

        return reg;
    }

  private:
    // an element or a parenthesized sub-expression: '-a' vs '-(a + b)'
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
            x.alloc_scratch_register(tok(), indent, scratch_type)};

        do_compile(tc, indent, toc::make_ident_info_from_register(reg));

        x.copy_value(tok(), indent, dst_info.operand, reg);

        x.free_scratch_register(tok(), indent, reg);
    }

    // a narrow memory destination truncates when stored, which gives the same
    // value when the low bits do not depend on the high bits
    [[nodiscard]] auto can_compute_wide(const toc& tc,
                                        const ident_info& dst_info) const
        -> bool {

        if (not dst_info.operand.is_memory()) {
            return false;
        }

        if (dst_info.type_ref().size_bytes() >=
            tc.get_type_default().size_bytes()) {
            return false;
        }

        return keeps_low_bits_when_narrowed();
    }

    auto validate_arithmetic_operands(const toc& tc) const -> void {
        if (ops_.empty()) {
            return;
        }

        for (const std::unique_ptr<statement>& expr : exprs_) {
            const type& expr_type{expr->is_identifier()
                                      ? tc.make_ident_info(*expr).type_ref()
                                      : expr->get_type()};

            if (expr_type.name() == tc.get_type_bool().name()) {
                throw compiler_exception{
                    expr->tok(),
                    "boolean values cannot be arithmetic operands"};
            }
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

    auto do_compile(toc& tc, const size_t indent,
                    const ident_info& dst_info) const -> void {

        compile_elements(tc, indent, dst_info);

        // apply unary expressions on destination
        uops_.compile(tc, indent, dst_info.operand);
    }

    auto compile_elements(toc& tc, const size_t indent,
                          const ident_info& dst_info) const -> void {

        const std::optional<merged_constants> merged{
            merge_constants(tc, dst_info.type_ref())};

        if (merged) {
            compile_merged(tc, indent, dst_info, *merged);
            return;
        }

        compile_first_element(tc, indent, dst_info, *exprs_[0]);

        // remaining elements are +,-,*,/,%,|,&,^,<<,>>
        for (const auto [o, e] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            const statement& s{*e};
            asm_op(tc, indent, o, dst_info, s);
        }
    }

    // an element computed at run time and the operation that applies it
    struct runtime_element {
        char op{};
        const statement* element{};
    };

    // the constant elements combined into one value applied with 'op'
    struct merged_constants {
        char op{};
        int64_t value{};
        std::vector<runtime_element> runtime_elements;
        std::string folded_source; // e.g. '- 3 + c * 2' for the comment
    };

    // empty when the order of the operations matters or there is no constant
    // to merge
    [[nodiscard]] auto merge_constants(const toc& tc,
                                       const type& width_type) const
        -> std::optional<merged_constants> {

        if (not is_mergeable()) {
            return std::nullopt;
        }

        // the first element is added in an additive list
        const char list_op{ops_.front() == '-' ? '+' : ops_.front()};

        merged_constants merged{
            .op{list_op},
            .value{identity_of(list_op)},
            .runtime_elements{},
            .folded_source{},
        };

        bool has_constant{};

        for (size_t i{}; i < exprs_.size(); ++i) {
            const char op{i == 0 ? list_op : ops_[i - 1]};
            const statement& e{*exprs_[i]};

            const std::optional<int64_t> value{
                element_constant(tc, e, width_type)};

            if (not value) {
                merged.runtime_elements.push_back({.op{op}, .element{&e}});
                continue;
            }

            merged.value = combine(merged.value, op, *value, width_type);
            has_constant = true;

            if (not merged.folded_source.empty()) {
                merged.folded_source += ' ';
            }

            merged.folded_source +=
                std::format("{} {}", op, statement::trimmed_source(e));
        }

        if (not has_constant) {
            return std::nullopt;
        }

        // 'compile' folds a list of only constants into one immediate
        assert(not merged.runtime_elements.empty());

        return merged;
    }

    // a subtracted element cannot lead without a negation so the constant
    // leads instead: '1 - b + 2' is '3 - b', a negated element is then
    // subtracted: '-b + 23' is '23 - b'
    [[nodiscard]] static auto
    leads_with_constant(const toc& tc, const merged_constants& merged) -> bool {

        const runtime_element& first{merged.runtime_elements.front()};

        // a zero constant adds nothing so negating in place is shorter
        if (merged.value == 0) {
            return false;
        }

        if (first.op == '-') {
            return true;
        }

        if (first.op != '+') {
            return false;
        }

        return is_negated_operand(tc, *first.element);
    }

    auto compile_merged(toc& tc, const size_t indent,
                        const ident_info& dst_info,
                        const merged_constants& merged) const -> void {

        const std::span<const runtime_element> elements{
            merged.runtime_elements};

        if (leads_with_constant(tc, merged)) {
            compile_constant(tc, indent, dst_info, merged.value,
                             merged.folded_source);

            for (const runtime_element& e : elements) {
                asm_op(tc, indent, e.op, dst_info, *e.element);
            }

            return;
        }

        compile_first_element(tc, indent, dst_info, *elements.front().element);

        machine& x{tc.machine()};

        // only a zero constant leaves a subtracted element first: '2 - b - 2'
        if (elements.front().op == '-') {
            x.unary(indent, '-', dst_info.operand);
        }

        for (const runtime_element& e : elements.subspan(1)) {
            asm_op(tc, indent, e.op, dst_info, *e.element);
        }

        // e.g. 'b + 1 - 1'
        if (merged.value == identity_of(merged.op)) {
            x.comment(tok(), indent,
                      "src: folded constant '{}' is {} and changes nothing",
                      merged.folded_source, merged.value);

            return;
        }

        asm_op_constant(tc, indent, merged.op, dst_info, merged.value,
                        merged.folded_source);
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
            x.multiply(tok(), indent, dst_info.operand, constant, false);
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

    // constants have the default type like literals
    [[nodiscard]] static auto constant_operand(const toc& tc,
                                               const int64_t value) -> operand {

        return operand::imm(std::format("{}", value), tc.get_type_default());
    }

    // constants can move to one end when the order of the operations does not
    // matter: '+' and '-', only '*' or one bitwise operation
    [[nodiscard]] auto is_mergeable() const -> bool {
        if (ops_.empty()) {
            return false;
        }

        const char first{ops_.front()};
        if (first == '*') {
            return std::ranges::all_of(
                ops_, [](const char o) -> bool { return o == '*'; });
        }

        return first == '+' or first == '-' or first == '&' or first == '|' or
               first == '^';
    }

    // the constant that leaves a value unchanged
    [[nodiscard]] static auto identity_of(const char op) -> int64_t {
        switch (op) {
        case '+':
        case '|':
        case '^':
            return 0;

        case '*':
            return 1;

        case '&':
            return -1;

        default:
            std::unreachable();
        }
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

    [[nodiscard]] static auto width_bits(const type& width_type) -> size_t {
        constexpr size_t byte_bits{8};
        return width_type.size_bytes() * byte_bits;
    }

    // e.g. -128 for 'i8'
    [[nodiscard]] static auto width_min(const type& width_type) -> int64_t {
        return static_cast<int64_t>(~uint64_t{}
                                    << (width_bits(width_type) - 1));
    }

    // the value a register of 'width_type' holds, sign extended
    [[nodiscard]] static auto wrap_to_width(const int64_t value,
                                            const type& width_type) -> int64_t {

        const size_t bits{width_bits(width_type)};
        if (bits >= std::numeric_limits<uint64_t>::digits) {
            return value;
        }

        const uint64_t sign_bit{uint64_t{1} << (bits - 1)};
        const uint64_t low{static_cast<uint64_t>(value) &
                           ((sign_bit << 1U) - 1U)};

        // flipping and subtracting the sign bit extends it
        return static_cast<int64_t>((low ^ sign_bit) - sign_bit);
    }

    static constexpr char precedence_additive{1};
    static constexpr char precedence_multiplicative{2};
    static constexpr char precedence_bitwise_or{3};
    static constexpr char precedence_bitwise_and{4};
    static constexpr char precedence_bitwise_xor{5};
    static constexpr char precedence_shift{6};

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

    // higher than the highest precedence
    static constexpr char initial_precedence{7};

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

    // 'compile_first_element' copies identifiers itself
    static auto asm_op_mov(toc& tc, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        assert(src.is_expression());

        machine& x{tc.machine()};

        x.comment(src.tok(), indent, "= expression");

        src.compile(tc, indent, dst_info);
    }

    [[nodiscard]] static auto compile_to_scratch(toc& tc, const size_t indent,
                                                 const statement& src,
                                                 const type& register_type)
        -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, register_type)};

        src.compile(tc, indent, toc::make_ident_info_from_register(reg));

        return reg;
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
                src.folded_constant(tc, expression_type)};

            if (value) {
                x.comment(src.tok(), indent, "src: folded constant '{}'",
                          statement::trimmed_source(src));

                emit(constant_operand(tc, *value), false);

                return;
            }

            x.comment(src.tok(), indent, "src: expression");

            const operand reg{
                compile_to_scratch(tc, indent, src, expression_type)};

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
            tc.get_lea_operand(indent, src, src_info, lea_registers)};

        if (src.get_unary_ops().is_empty()) {
            x.comment(src.tok(), indent, "src: operand");
            emit(src_operand, false);
            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        x.comment(src.tok(), indent, "src: operand with unary ops");

        const operand reg{
            compile_unary_to_scratch(tc, indent, src, src_operand, unary_type)};

        emit(reg, true);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
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

    // a register or memory location, neither computed nor constant
    [[nodiscard]] static auto is_stored_value(const toc& tc,
                                              const statement& src) -> bool {

        return not src.is_expression() and
               not tc.make_ident_info(src).is_const();
    }

    // a lone negation folds into add/sub: 'a - -b' is 'a + b'
    [[nodiscard]] static auto is_negated_operand(const toc& tc,
                                                 const statement& src) -> bool {

        return is_stored_value(tc, src) and
               src.get_unary_ops().is_only_negated();
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
                tc.get_lea_operand(indent, src, src_info, lea_registers)};

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
};
