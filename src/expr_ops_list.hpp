#pragma once
// reviewed: 2025-09-28

#include <memory>
#include <ostream>
#include <span>
#include <sstream>
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
class expr_ops_list final : public expression {
    std::vector<std::unique_ptr<statement>> exprs_; // expression list
    std::vector<char> ops_; // operators between elements in the vector
    unary_ops uops_;        // unary ops for all result e.g. ~(a+b)
    token open_paren_tk_;   // when 'enclosed' the '(' token
    token close_paren_tk_;  // when 'enclosed' the ')' token
    bool enclosed_{};       // (a + b)  vs  a + b

    // true when this list was created because of a higher-precedence operation
    //   1 + 2 * 3 + 4  => 1 + [2 * 3] + 4
    bool is_implied_subexpression_{};

  public:
    expr_ops_list(toc& tc, tokenizer& tz, const bool in_args = false,
                  const bool enclosed = false, const token open_paren_tk = {},
                  const bool is_implied_subexpression = false,
                  unary_ops uops = {},
                  const uint8_t first_op_precedence = initial_precedence,
                  std::unique_ptr<statement> first_expression = {})
        : expression{tz.current_position_token()}, uops_{std::move(uops)},
          open_paren_tk_{open_paren_tk}, enclosed_{enclosed},
          is_implied_subexpression_{is_implied_subexpression} {

        // a recursive call might have supplied the first element it already
        // parsed

        if (first_expression) {
            // it did, add provided first expression as first element in the
            // list
            exprs_.emplace_back(std::move(first_expression));
        } else {
            // it did not, check if it is an element or the start of a new
            // sub-expression
            //   -(a + b)  vs  -a

            // read the unary ops before checking for open parenthesis
            unary_ops uo{tz};

            // parenthesized expressions become nested lists
            if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
                // move the unary ops to be applied on the whole sub-expression
                exprs_.emplace_back(std::make_unique<expr_ops_list>(
                    tc, tz, in_args, true, t, false, std::move(uo)));
            } else {
                // non-parenthesized unary ops belong to the next expression
                // push back for so the element attaches it as its own
                uo.put_back(tz);
                exprs_.emplace_back(create_statement_in_expr_ops_list(tc, tz));
            }
        }

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

            // is it parsed within a function argument?
            if (in_args) {
                // yes, exit when ',' or ')' is found
                if (tz.is_peek_char(',') or tz.is_peek_char(')')) {
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

                // move the last element out of the list
                std::unique_ptr<statement> last_elem_in_list{
                    std::move(exprs_.back())};

                exprs_.pop_back();

                // forward it to the sub-expression including its precedence
                exprs_.emplace_back(make_unique<expr_ops_list>(
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

                validate_arithmetic_operands(tc);

                return;
            }

            // possible new lower or same precedence
            precedence = next_precedence;

            // consume the peeked operator
            const char ch{tz.next_char()};

            // consume the second character of a previously recognized shift
            // operator
            if (ch == '<' or ch == '>') {
                (void)tz.next_char();
            }

            // check if the next statement is a sub-expression or an expression
            // element
            //   -(a + b)  vs  -a

            // read the unary ops before checking if parenthesis opens
            // sub-expression
            unary_ops uo{tz};

            // is it a sub-expression?
            if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
                // yes, recurse and forward the unary ops to be applied on the
                // whole sub-expression
                exprs_.emplace_back(std::make_unique<expr_ops_list>(
                    tc, tz, in_args, true, t, false, std::move(uo)));

                continue;
            }

            // not sub-expression, push back unary ops because those belong to
            // the next element
            //   [-a] + b
            uo.put_back(tz);

            // read the next element
            exprs_.emplace_back(create_statement_in_expr_ops_list(tc, tz));

            // continue to next arithmetic op + element
        }
    }

    expr_ops_list() = default;

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        if (enclosed_) {
            open_paren_tk_.source_to(os);
        }
        expression::source_to(os); // whitespace
        exprs_[0]->source_to(os);
        for (const auto [op, expr] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            std::print(os, "{}", op);
            if (op == '<' or op == '>') {
                // handle case << and >>
                std::print(os, "{}", op);
            }
            expr->source_to(os);
        }

        if (enclosed_) {
            close_paren_tk_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        // is destination a register?
        if (dst_info.is_register()) {
            // yes, compile with the result placed in it
            do_compile(tc, indent, dst_info);

            return;
        }

        // compile with and without the scratch register to find the best
        // compilation

        // without scratch register
        std::stringstream ss1;

        machine& x{tc.machine()};

        std::ostream& prev1{x.use_stream(ss1)};
        do_compile(tc, indent, dst_info);
        x.use_stream(prev1);

        // with scratch register
        std::stringstream ss2;
        std::ostream& prev2{x.use_stream(ss2)};
        const operand reg{
            x.alloc_scratch_register(tok(), indent, dst_info.type_ref())};

        const ident_info dst_reg_info{toc::make_ident_info_from_register(reg)};

        do_compile(tc, indent, dst_reg_info);
        x.copy_value(tok(), indent, dst_info.operand, reg);
        x.free_scratch_register(tok(), indent, reg);
        x.use_stream(prev2);

        x.emit_most_efficient(tok(), indent, ss1.view(), ss2.view());
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

    [[nodiscard]] auto is_indexed() const -> bool override {
        assert(exprs_.size() == 1);

        return exprs_[0]->is_indexed();
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        for (const std::unique_ptr<statement>& e : exprs_) {
            e->assert_var_not_used(var);
        }
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return exprs_.size() == 1 and exprs_[0]->is_identifier();
    }

    [[nodiscard]] auto compile_lea(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, const operand& reg_size,
        const std::span<const operand> lea_path) const -> operand override {

        assert(exprs_.size() == 1);

        return exprs_[0]->compile_lea(tc, indent, src_loc_tk,
                                      allocated_registers, reg_size, lea_path);
    }

  private:
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

    auto do_compile(toc& tc, const size_t indent,
                    const ident_info& dst_info) const -> void {

        const statement& st0{*exprs_[0]};
        if (st0.is_identifier()) {
            st0.compile(tc, indent, dst_info);
        } else {
            // the first element is assigned to destination, operator '='
            asm_op(tc, indent, '=', dst_info, st0);
        }

        // remaining elements are +,-,*,/,%,|,&,^,<<,>>
        for (const auto [op, expr] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            const statement& st{*expr};
            asm_op(tc, indent, op, dst_info, st);
        }

        // apply unary expressions on destination
        uops_.compile(tc, indent, dst_info.operand);
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

        std::string op_str{op};
        if (op == '<') {
            op_str.push_back('<');
        } else if (op == '>') {
            op_str.push_back('>');
        }

        machine& x{tc.machine()};

        x.comment(src.tok(), indent,
                  statement::trimmed_source(src, dst.id, op_str));

        if (op == '=') {
            asm_op_mov(tc, indent, dst, src);

            return;
        }
        if (op == '+') {
            asm_op_add_sub(tc, indent, op, dst, src);

            return;
        }
        if (op == '-') {
            asm_op_add_sub(tc, indent, op, dst, src);

            return;
        }
        if (op == '*') {
            asm_op_mul(tc, indent, dst, src);

            return;
        }
        if (op == '/') {
            asm_op_div(tc, indent, op, dst, src);

            return;
        }
        if (op == '%') {
            asm_op_div(tc, indent, op, dst, src);

            return;
        }
        if (op == '&') {
            asm_op_bitwise(tc, indent, op, dst, src);

            return;
        }
        if (op == '|') {
            asm_op_bitwise(tc, indent, op, dst, src);

            return;
        }
        if (op == '^') {
            asm_op_bitwise(tc, indent, op, dst, src);

            return;
        }
        if (op == '<') {
            asm_op_shift(tc, indent, op, dst, src);

            return;
        }
        if (op == '>') {
            asm_op_shift(tc, indent, op, dst, src);

            return;
        }
    }

    static auto asm_op_mov(toc& tc, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        x.comment(src.tok(), indent, "{}",
                  src.is_expression() ? "= expression" : "");

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile with destination to 'dst'
            src.compile(tc, indent, dst_info);

            return;
        }

        std::unreachable();
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

    static auto asm_op_mul(toc& tc, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile it to a scratch register
            const operand reg{
                compile_to_scratch(tc, indent, src, dst_info.type_ref())};

            if (dst_info.is_register() and not dst_info.operand.is_memory()) {
                x.comment(src.tok(), indent, "imul: expr reg");
            } else {
                x.comment(src.tok(), indent, "imul: expr not reg");
            }
            x.multiply(src.tok(), indent, dst_info.operand, reg, true);
            x.free_scratch_register(src.tok(), indent, reg);

            return;
        }

        // not an expression, either a register or memory location, or constant

        const ident_info src_info{tc.make_ident_info(src)};

        const std::string_view dst_kind{dst_info.is_register() ? "reg"
                                                               : "not reg"};

        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "dst is {}, src is const", dst_kind);
            x.multiply(src.tok(), indent, dst_info.operand,
                       src.make_constant_operand(src_info));

            return;
        }

        // source is not a constant

        std::vector<operand> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.comment(src.tok(), indent, "dst is {}, src is not const, no uops",
                      dst_kind);

            x.multiply(src.tok(), indent, dst_info.operand, src_operand);

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // source is not a constant and unary ops need to be applied

        x.comment(src.tok(), indent, "dst is {}, src is not const, uops",
                  dst_kind);

        const operand reg{compile_unary_to_scratch(tc, indent, src, src_operand,
                                                   dst_info.type_ref())};

        x.multiply(src.tok(), indent, dst_info.operand, reg, true);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }

    static auto asm_op_add_sub(toc& tc, const size_t indent, const char op,
                               const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            const operand reg{
                compile_to_scratch(tc, indent, src, dst_info.type_ref())};

            x.add_subtract(src.tok(), indent, op, dst_info.operand, reg);

            x.free_scratch_register(src.tok(), indent, reg);

            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            x.add_subtract(src.tok(), indent, op, dst_info.operand,
                           src.make_constant_operand(src_info));

            return;
        }

        // 'src' is not a constant

        std::vector<operand> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.add_subtract(src.tok(), indent, op, dst_info.operand,
                           src_operand);

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // has unary ops

        if (uops.is_only_negated()) {
            // has unary ops
            x.add_subtract(src.tok(), indent, op == '+' ? '-' : '+',
                           dst_info.operand, src_operand);

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // multiple unary ops

        const operand reg{compile_unary_to_scratch(tc, indent, src, src_operand,
                                                   tc.get_type_default())};

        x.add_subtract(src.tok(), indent, op, dst_info.operand, reg);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }

    static auto asm_op_bitwise(toc& tc, const size_t indent, const char op,
                               const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            const operand reg{
                compile_to_scratch(tc, indent, src, dst_info.type_ref())};

            x.bitwise(src.tok(), indent, op, dst_info.operand, reg);
            x.free_scratch_register(src.tok(), indent, reg);

            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            x.bitwise(src.tok(), indent, op, dst_info.operand,
                      src.make_constant_operand(src_info));

            return;
        }

        // 'src' is not an expression and not a constant, an identifier

        std::vector<operand> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.bitwise(src.tok(), indent, op, dst_info.operand, src_operand);

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // 'src' is not an expression and not a constant and has unary ops

        const operand reg{compile_unary_to_scratch(tc, indent, src, src_operand,
                                                   tc.get_type_default())};

        x.bitwise(src.tok(), indent, op, dst_info.operand, reg);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }

    static auto asm_op_shift(toc& tc, const size_t indent, const char op,
                             const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            x.comment(src.tok(), indent, "shf: expr");
            const operand count_register{
                compile_to_scratch(tc, indent, src, dst_info.type_ref())};

            x.shift(src.tok(), indent, op, dst_info.operand, count_register);
            x.free_scratch_register(src.tok(), indent, count_register);

            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "shf: const");
            x.shift(src.tok(), indent, op, dst_info.operand,
                    src.make_constant_operand(src_info));

            return;
        }

        x.validate_shift_operand(src.tok(), src_info.operand);

        // 'src' is not a constant

        std::vector<operand> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.comment(src.tok(), indent, "shf: not const, no uops");
            x.shift(src.tok(), indent, op, dst_info.operand, src_operand);

            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // unary ops need to be applied on the argument src

        x.comment(src.tok(), indent, "shf: not const, uops");

        const operand count_register{compile_unary_to_scratch(
            tc, indent, src, src_operand, dst_info.type_ref())};

        x.shift(src.tok(), indent, op, dst_info.operand, count_register);
        x.free_scratch_register(src.tok(), indent, count_register);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }

    static auto asm_op_div(toc& tc, const size_t indent, const char op,
                           const ident_info& dst_info, const statement& src)
        -> void {

        machine& x{tc.machine()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            x.comment(src.tok(), indent, "div expression");
            const operand reg{
                compile_to_scratch(tc, indent, src, dst_info.type_ref())};

            x.divide(src.tok(), indent, op, dst_info.operand, reg);
            x.free_scratch_register(src.tok(), indent, reg);

            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "div const");
            x.divide(src.tok(), indent, op, dst_info.operand,
                     src.make_constant_operand(src_info));

            return;
        }

        x.validate_division_operand(src.tok(), src_info.operand);

        // 'src' is not an expression and not a constant

        std::vector<operand> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.comment(src.tok(), indent, "div not const, no uops");
            x.divide(src.tok(), indent, op, dst_info.operand, src_operand);
            x.free_scratch_registers(src.tok(), indent, lea_registers);

            return;
        }

        // 'src' is not an expression and not a constant and has unary ops

        x.comment(src.tok(), indent, "div not const, uops");
        const operand reg{compile_unary_to_scratch(tc, indent, src, src_operand,
                                                   dst_info.type_ref())};

        x.divide(src.tok(), indent, op, dst_info.operand, reg);
        x.free_scratch_register(src.tok(), indent, reg);
        x.free_scratch_registers(src.tok(), indent, lea_registers);
    }
};
