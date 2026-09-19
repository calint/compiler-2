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
#include "x86.hpp"

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
                  const bool enclosed = false, token open_paren_tk = {},
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
                     ? tc.make_ident_info_parsing(first_expr).type_ref()
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

    auto compile(toc& tc, x86& x, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        // is destination a register?
        if (dst_info.is_register()) {
            // yes, compile with the result placed in it
            do_compile(tc, x, indent, dst_info);
            return;
        }

        // compile with and without the scratch register to find the best
        // compilation

        // without scratch register
        std::stringstream ss1;
        std::ostream& prev1{x.use_stream(ss1)};
        do_compile(tc, x, indent, dst_info);
        x.use_stream(prev1);

        // with scratch register
        std::stringstream ss2;
        std::ostream& prev2{x.use_stream(ss2)};
        const std::string reg{
            x.alloc_scratch_register(tok(), indent, tc.get_type_default())};

        const std::string reg_sized{
            x86::get_sized_register_operand(reg, dst_info.operand.size)};

        // note: sized register to propagate operation to destination size
        const ident_info dst_reg_info{
            toc::make_ident_info_from_register(x, reg_sized)};

        do_compile(tc, x, indent, dst_reg_info);
        x.mov(tok(), indent, dst_info.operand.str(), reg_sized);
        x.free_scratch_register(tok(), indent, reg);
        x.use_stream(prev2);

        // compare instruction count
        const size_t ss1_count{count_instructions(ss1)};
        const size_t ss2_count{count_instructions(ss2)};

        x.comment(tok(), indent,
                  "instructions without scratch register {}, with {}",
                  ss1_count, ss2_count);

        // select the version with the fewest instructions
        if (ss1_count <= ss2_count) {
            x.emit_buffer(ss1.str());
        } else {
            x.emit_buffer(ss2.str());
        }
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

    [[nodiscard]] auto
    compile_lea(toc& tc, x86& x, size_t indent, const token& src_loc_tk,
                std::vector<std::string>& allocated_registers,
                const std::string& reg_size,
                const std::span<const std::string> lea_path) const
        -> operand override {

        assert(exprs_.size() == 1);

        return exprs_[0]->compile_lea(tc, x, indent, src_loc_tk,
                                      allocated_registers, reg_size, lea_path);
    }

  private:
    auto validate_arithmetic_operands(const toc& tc) const -> void {
        if (ops_.empty()) {
            return;
        }

        for (const std::unique_ptr<statement>& expr : exprs_) {
            const type& expr_type{
                expr->is_identifier()
                    ? tc.make_ident_info_parsing(*expr).type_ref()
                    : expr->get_type()};

            if (expr_type.name() == tc.get_type_bool().name()) {
                throw compiler_exception{
                    expr->tok(),
                    "boolean values cannot be arithmetic operands"};
            }
        }
    }

    auto do_compile(toc& tc, x86& x, const size_t indent,
                    const ident_info& dst_info) const -> void {

        const statement& st0{*exprs_[0]};
        if (st0.is_identifier()) {
            st0.compile(tc, x, indent, dst_info);
        } else {
            // the first element is assigned to destination, operator '='
            asm_op(tc, x, indent, '=', dst_info, st0);
        }

        // remaining elements are +,-,*,/,%,|,&,^,<<,>>
        for (const auto [op, expr] :
             std::views::zip(ops_, exprs_ | std::views::drop(1))) {

            const statement& st{*expr};
            asm_op(tc, x, indent, op, dst_info, st);
        }

        // apply unary expressions on destination
        uops_.compile(tc, x, indent, dst_info.operand.str());
    }

    [[nodiscard]] static auto count_instructions(std::stringstream& ss)
        -> size_t {

        std::string line;
        size_t n{};
        while (getline(ss, line)) {
            if (is_nasm_comment_line(line)) {
                continue;
            }
            ++n;
        }
        return n;
    }

    [[nodiscard]] static auto is_nasm_comment_line(const std::string_view line)
        -> bool {

        const size_t first{line.find_first_not_of(" \t\n\r\f\v")};
        return first != std::string_view::npos && line[first] == ';';
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

    static auto asm_op(toc& tc, x86& x, const size_t indent, const char op,
                       const ident_info& dst, const statement& src) -> void {

        std::string op_str{op};
        if (op == '<') {
            op_str.push_back('<');
        } else if (op == '>') {
            op_str.push_back('>');
        }

        x.comment(src.tok(), indent,
                  statement::trimmed_source(src, dst.id, op_str));

        if (op == '=') {
            asm_op_mov(tc, x, indent, dst, src);
            return;
        }
        if (op == '+') {
            asm_op_add_sub(tc, x, indent, "add", "sub", dst, src);
            return;
        }
        if (op == '-') {
            asm_op_add_sub(tc, x, indent, "sub", "add", dst, src);
            return;
        }
        if (op == '*') {
            asm_op_mul(tc, x, indent, dst, src);
            return;
        }
        if (op == '/') {
            asm_op_div(tc, x, indent, "rax", dst, src);
            return;
        }
        if (op == '%') {
            asm_op_div(tc, x, indent, "rdx", dst, src);
            return;
        }
        if (op == '&') {
            asm_op_bitwise(tc, x, indent, "and", dst, src);
            return;
        }
        if (op == '|') {
            asm_op_bitwise(tc, x, indent, "or", dst, src);
            return;
        }
        if (op == '^') {
            asm_op_bitwise(tc, x, indent, "xor", dst, src);
            return;
        }
        if (op == '<') {
            asm_op_shift(tc, x, indent, "sal", dst, src);
            return;
        }
        if (op == '>') {
            asm_op_shift(tc, x, indent, "sar", dst, src);
            return;
        }
    }

    static auto asm_op_mov(toc& tc, x86& x, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        x.comment(src.tok(), indent, "{}",
                  src.is_expression() ? "= expression" : "");

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile with destination to 'dst'
            src.compile(tc, x, indent, dst_info);
            return;
        }

        std::unreachable();
    }

    static auto asm_op_mul(toc& tc, x86& x, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.operand.size};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile it to a scratch register
            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            // make register sized to destination
            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            src.compile(tc, x, indent,
                        toc::make_ident_info_from_register(x, reg_sized));

            // note: 'imul' destination must be a register
            if (dst_info.is_register() and not dst_info.operand.is_memory) {
                x.comment(src.tok(), indent, "imul: expr reg");
                x.op(src.tok(), indent, "imul", dst_info.operand.str(),
                     reg_sized);
            } else {
                x.comment(src.tok(), indent, "imul: expr not reg");
                // 'imul' destination is not a register
                x.op(src.tok(), indent, "imul", reg_sized,
                     dst_info.operand.str());

                x.mov(src.tok(), indent, dst_info.operand.str(), reg_sized);
            }
            x.free_scratch_register(src.tok(), indent, reg);
            return;
        }

        // not an expression, either a register or memory location, or constant

        const ident_info src_info{tc.make_ident_info(x, src)};

        // note: 'imul' destination operand must be register

        // note: special case for byte sized multiplication because x86_64 does
        //       not support constant as a second operand for byte size
        //       operation
        if (dst_size == 1) {
            std::vector<std::string> lea_registers;
            const operand src_operand{
                tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

            const unary_ops& uops{src.get_unary_ops()};
            const std::string r1{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string r2{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            x.mov(src.tok(), indent, r1, dst_info.operand.str());

            if (src_info.is_const()) {
                x.comment(src.tok(), indent, "imul: byte const");
                x.mov(src.tok(), indent, r2,
                      std::format("{}{}", uops.to_string(),
                                  src_info.const_value));
            } else {
                x.comment(src.tok(), indent, "imul: byte not const");
                x.mov(src.tok(), indent, r2, src_operand.str());
                uops.compile(tc, x, indent, r2);
            }

            x.imul(src.tok(), indent, r1, r2);
            x.mov(src.tok(), indent, dst_info.operand.str(), r1);

            x.free_scratch_register(src.tok(), indent, r2);
            x.free_scratch_register(src.tok(), indent, r1);
            free_registers(src, x, indent, lea_registers);
            return;
        }

        if (dst_info.is_register()) {
            // destination is a register
            if (src_info.is_const()) {
                x.comment(src.tok(), indent, "dst is reg, src is const");
                x.imul(src.tok(), indent, dst_info.operand.str(),
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.const_value));

                return;
            }

            std::vector<std::string> lea_registers;
            const operand src_operand{
                tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

            const unary_ops& uops{src.get_unary_ops()};
            if (uops.is_empty()) {
                x.comment(src.tok(), indent,
                          "dst is reg, src is not const, no uops");

                x.op(src.tok(), indent, "imul", dst_info.operand.str(),
                     src_operand.str());

                free_registers(src, x, indent, lea_registers);
                return;
            }

            x.comment(src.tok(), indent, "dst is reg, src is not const, uops");
            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            x.mov(src.tok(), indent, reg_sized, src_operand.str());
            uops.compile(tc, x, indent, reg_sized);
            x.imul(src.tok(), indent, dst_info.operand.str(), reg_sized);
            x.free_scratch_register(src.tok(), indent, reg);
            free_registers(src, x, indent, lea_registers);
            return;
        }

        // 'imul' destination is not a register

        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "dst is not reg, src is const");
            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            x.mov(src.tok(), indent, reg_sized, dst_info.operand.str());
            x.imul(src.tok(), indent, reg_sized,
                   std::format("{}{}", src.get_unary_ops().to_string(),
                               src_info.const_value));

            x.mov(src.tok(), indent, dst_info.operand.str(), reg_sized);
            x.free_scratch_register(src.tok(), indent, reg);
            return;
        }

        // source is not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.comment(src.tok(), indent,
                      "dst is not reg, src is not const, no uops");

            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            x.mov(src.tok(), indent, reg_sized, dst_info.operand.str());
            x.imul(src.tok(), indent, reg_sized, src_operand.str());
            x.mov(src.tok(), indent, dst_info.operand.str(), reg_sized);
            x.free_scratch_register(src.tok(), indent, reg);
            free_registers(src, x, indent, lea_registers);
            return;
        }

        // source is not a constant and unary ops need to be applied

        x.comment(src.tok(), indent, "dst is not reg, src is not const, uops");
        const std::string reg{
            x.alloc_scratch_register(src.tok(), indent, tc.get_type_default())};

        const std::string reg_sized{
            x86::get_sized_register_operand(reg, dst_size)};

        x.mov(src.tok(), indent, reg_sized, src_operand.str());
        uops.compile(tc, x, indent, reg_sized);
        x.imul(src.tok(), indent, reg_sized, dst_info.operand.str());
        x.mov(src.tok(), indent, dst_info.operand.str(), reg_sized);
        x.free_scratch_register(src.tok(), indent, reg);
        free_registers(src, x, indent, lea_registers);
    }

    static auto asm_op_add_sub(toc& tc, x86& x, const size_t indent,
                               const std::string_view op,
                               const std::string_view op_when_negated,
                               const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.operand.size};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            src.compile(tc, x, indent,
                        toc::make_ident_info_from_register(x, reg_sized));

            x.op(src.tok(), indent, op, dst_info.operand.str(), reg_sized);
            x.free_scratch_register(src.tok(), indent, reg);
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(x, src)};
        if (src_info.is_const()) {
            x.op(src.tok(), indent, op, dst_info.operand.str(),
                 std::format("{}{}", src.get_unary_ops().to_string(),
                             src_info.const_value));

            return;
        }

        // 'src' is not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.op(src.tok(), indent, op, dst_info.operand.str(),
                 src_operand.str());

            free_registers(src, x, indent, lea_registers);
            return;
        }

        // has unary ops

        if (uops.is_only_negated()) {
            // has unary ops
            x.op(src.tok(), indent, op_when_negated, dst_info.operand.str(),
                 src_operand.str());

            free_registers(src, x, indent, lea_registers);
            return;
        }

        // multiple unary ops

        const std::string reg{
            x.alloc_scratch_register(src.tok(), indent, tc.get_type_default())};

        x.mov(src.tok(), indent, reg, src_operand.str());
        uops.compile(tc, x, indent, reg);
        x.op(src.tok(), indent, op, dst_info.operand.str(), reg);
        x.free_scratch_register(src.tok(), indent, reg);
        free_registers(src, x, indent, lea_registers);
    }

    static auto asm_op_bitwise(toc& tc, x86& x, const size_t indent,
                               const std::string_view op,
                               const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.operand.size};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            src.compile(tc, x, indent,
                        toc::make_ident_info_from_register(x, reg_sized));

            x.op(src.tok(), indent, op, dst_info.operand.str(), reg_sized);
            x.free_scratch_register(src.tok(), indent, reg);
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(x, src)};
        if (src_info.is_const()) {
            x.op(src.tok(), indent, op, dst_info.operand.str(),
                 std::format("{}{}", src.get_unary_ops().to_string(),
                             src_info.const_value));

            return;
        }

        // 'src' is not an expression and not a constant, an identifier

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.op(src.tok(), indent, op, dst_info.operand.str(),
                 src_operand.str());

            free_registers(src, x, indent, lea_registers);
            return;
        }

        // 'src' is not an expression and not a constant and has unary ops

        const std::string reg{
            x.alloc_scratch_register(src.tok(), indent, tc.get_type_default())};

        x.mov(src.tok(), indent, reg, src_operand.str());
        uops.compile(tc, x, indent, reg);
        x.op(src.tok(), indent, op, dst_info.operand.str(), reg);
        x.free_scratch_register(src.tok(), indent, reg);
        free_registers(src, x, indent, lea_registers);
    }

    static auto asm_op_shift(toc& tc, x86& x, const size_t indent,
                             const std::string_view op,
                             const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.operand.size};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            x.comment(src.tok(), indent, "shf: expr");
            // the operand must be stored in register 'CL'
            x.alloc_named_register(src.tok(), indent, "rcx",
                                   tc.get_type_default());

            const std::string rcx_sized{
                x86::get_sized_register_operand("rcx", dst_size)};

            // the number of bits to shift is an expression, compile it to 'rcx'
            src.compile(tc, x, indent,
                        toc::make_ident_info_from_register(x, rcx_sized));

            x.op(src.tok(), indent, op, dst_info.operand.str(), "cl");
            x.free_named_register(src.tok(), indent, "rcx");
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(x, src)};
        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "shf: const");
            x.op(src.tok(), indent, op, dst_info.operand.str(),
                 std::format("{}{}", src.get_unary_ops().to_string(),
                             src_info.const_value));

            return;
        }

        if (src_info.operand.str() == "rcx") {
            throw compiler_exception{
                src.tok(), "cannot use 'rcx' as a shift operand; it is "
                           "reserved for the count"};
        }

        // 'src' is not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.comment(src.tok(), indent, "shf: not const, no uops");
            // the operand must be stored in CL (see note above about BMI2)
            x.alloc_named_register(src.tok(), indent, "rcx",
                                   tc.get_type_default());

            const std::string rcx_sized{
                x86::get_sized_register_operand("rcx", dst_size)};

            x.mov(src.tok(), indent, rcx_sized, src_operand.str());
            x.op(src.tok(), indent, op, dst_info.operand.str(), "cl");
            x.free_named_register(src.tok(), indent, "rcx");
            free_registers(src, x, indent, lea_registers);
            return;
        }

        // unary ops need to be applied on the argument src

        x.comment(src.tok(), indent, "shf: not const, uops");

        x.alloc_named_register(src.tok(), indent, "rcx", tc.get_type_default());
        const std::string rcx_sized{
            x86::get_sized_register_operand("rcx", dst_size)};

        x.mov(src.tok(), indent, "rcx", src_operand.str());
        uops.compile(tc, x, indent, rcx_sized);
        x.op(src.tok(), indent, op, dst_info.operand.str(), "cl");
        x.free_named_register(src.tok(), indent, "rcx");
        free_registers(src, x, indent, lea_registers);
    }

    // op is either 'rax' for the quotient or 'rdx' for the reminder to be moved
    // into 'dst'
    static auto asm_op_div(toc& tc, x86& x, const size_t indent,
                           const std::string_view op,
                           const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.operand.size};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            x.comment(src.tok(), indent, "div expression");
            const std::string reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            const std::string reg_sized{
                x86::get_sized_register_operand(reg, dst_size)};

            src.compile(tc, x, indent,
                        toc::make_ident_info_from_register(x, reg_sized));

            x.alloc_named_register(src.tok(), indent, "rax",
                                   tc.get_type_default());

            x.mov(src.tok(), indent,
                  x86::get_sized_register_operand("rax", dst_size),
                  dst_info.operand.str());

            x.alloc_named_register(src.tok(), indent, "rdx",
                                   tc.get_type_default());

            x.div_reg_ext(indent, dst_size);
            x.idiv(indent, reg);
            x.mov(src.tok(), indent, dst_info.operand.str(), op);
            x.free_named_register(src.tok(), indent, "rdx");
            x.free_named_register(src.tok(), indent, "rax");
            x.free_scratch_register(src.tok(), indent, reg);
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(x, src)};
        if (src_info.is_const()) {
            x.comment(src.tok(), indent, "div const");
            x.alloc_named_register(src.tok(), indent, "rax",
                                   tc.get_type_default());

            x.mov(src.tok(), indent,
                  x86::get_sized_register_operand("rax", dst_size),
                  dst_info.operand.str());

            x.alloc_named_register(src.tok(), indent, "rdx",
                                   tc.get_type_default());

            x.div_reg_ext(indent, dst_size);
            const std::string scratch_reg{x.alloc_scratch_register(
                src.tok(), indent, tc.get_type_default())};

            x.mov(src.tok(), indent, scratch_reg,
                  std::format("{}{}", src.get_unary_ops().to_string(),
                              src_info.const_value));

            x.idiv(indent, scratch_reg);
            x.free_scratch_register(src.tok(), indent, scratch_reg);
            x.mov(src.tok(), indent, dst_info.operand.str(), op);
            x.free_named_register(src.tok(), indent, "rdx");
            x.free_named_register(src.tok(), indent, "rax");
            return;
        }

        if (src_info.operand.str() == "rdx" or
            src_info.operand.str() == "rax") {

            throw compiler_exception{
                src.tok(), "cannot use 'rdx' or 'rax' for division; they are "
                           "reserved"};
        }

        // 'src' is not an expression and not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(x, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            x.comment(src.tok(), indent, "div not const, no uops");
            x.alloc_named_register(src.tok(), indent, "rax",
                                   tc.get_type_default());

            x.mov(src.tok(), indent,
                  x86::get_sized_register_operand("rax", dst_size),
                  dst_info.operand.str());

            x.alloc_named_register(src.tok(), indent, "rdx",
                                   tc.get_type_default());

            x.div_reg_ext(indent, dst_size);
            x.idiv(indent, src_operand.str());
            // op is either 'rax' for the quotient or 'rdx' for the reminder
            x.mov(src.tok(), indent, dst_info.operand.str(), op);
            x.free_named_register(src.tok(), indent, "rdx");
            x.free_named_register(src.tok(), indent, "rax");
            free_registers(src, x, indent, lea_registers);
            return;
        }

        // 'src' is not an expression and not a constant and has unary ops

        x.comment(src.tok(), indent, "div not const, uops");
        const std::string reg{
            x.alloc_scratch_register(src.tok(), indent, tc.get_type_default())};

        const std::string reg_sized{
            x86::get_sized_register_operand(reg, dst_size)};

        x.mov(src.tok(), indent, reg_sized, src_operand.str());
        uops.compile(tc, x, indent, reg_sized);
        x.alloc_named_register(src.tok(), indent, "rax", tc.get_type_default());
        x.mov(src.tok(), indent,
              x86::get_sized_register_operand("rax", dst_size),
              dst_info.operand.str());

        x.alloc_named_register(src.tok(), indent, "rdx", tc.get_type_default());
        x.div_reg_ext(indent, dst_size);
        x.idiv(indent, reg_sized);
        x.mov(src.tok(), indent, dst_info.operand.str(), op);
        x.free_named_register(src.tok(), indent, "rdx");
        x.free_named_register(src.tok(), indent, "rax");
        x.free_scratch_register(src.tok(), indent, reg);
        free_registers(src, x, indent, lea_registers);
    }

    static auto free_registers(const statement& st, x86& x, const size_t indent,
                               const std::span<const std::string> registers)
        -> void {

        for (const std::string& reg : registers | std::views::reverse) {
            x.free_scratch_register(st.tok(), indent, reg);
        }
    }
};
