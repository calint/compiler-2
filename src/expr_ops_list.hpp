#pragma once
// reviewed: 2025-09-28

#include <iostream>
#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <sstream>
#include <utility>
#include <vector>

#include "decouple.hpp"
#include "expression.hpp"
#include "toc.hpp"

// a list of elements / lists connected by operators instead of tree
// note: quirky parsing but trivial compilation
class expr_ops_list final : public expression {
    std::vector<std::unique_ptr<statement>> exprs_; // expression list
    std::vector<char> ops_;     // operators between elements in the vector
    unary_ops uops_;            // unary ops for all result e.g. ~(a+b)
    token ws1_;                 // whitespace after parenthesis when enclosed
    bool enclosed_{};           //  (a+b) vs a+b
    bool is_base_expression_{}; // false when in implied sub-expressions

  public:
    expr_ops_list(toc& tc, tokenizer& tz, const bool in_args = false,
                  const bool enclosed = false,
                  const bool is_base_expression = true, unary_ops uops = {},
                  const uint8_t first_op_precedence = initial_precedence,
                  std::unique_ptr<statement> first_expression = {})
        : expression{tz.next_whitespace_token()}, uops_{std::move(uops)},
          enclosed_{enclosed}, is_base_expression_{is_base_expression} {

        // is this in a recursion?
        if (first_expression) {
            // yes, add provided first expression
            exprs_.emplace_back(std::move(first_expression));
        } else {
            // no, read the first expression or start a new recursion
            // e.g. =-(-(b+c)+d)
            unary_ops uo{tz};
            // is next a sub-expression?
            if (tz.is_next_char('(')) {
                // yes, recurse with unary ops
                exprs_.emplace_back(std::make_unique<expr_ops_list>(
                    tc, tz, in_args, true, true, std::move(uo)));
            } else {
                // no, push back the unary ops to be attached to the
                // statement
                uo.put_back(tz);
                exprs_.emplace_back(create_statement_in_expr_ops_list(tc, tz));
            }
        }

        // start with provided precedence
        uint8_t precedence{first_op_precedence};

        while (true) { // +a +3
            // if the end of sub-expression
            if (enclosed_ and tz.is_next_char(')')) {
                // return from recursion
                ws1_ = tz.next_whitespace_token();
                return;
            }

            // is it parsed within a function argument?
            if (in_args) {
                // yes, exit when ',' or ')' is found
                if (tz.is_peek_char(',') or tz.is_peek_char(')')) {
                    ws1_ = tz.next_whitespace_token();
                    return;
                }
            }

            // next operation
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
                return;
            }

            // is next operation precedence higher than current?
            // if so, the implied sub-expression is added to the list with
            // the last expression in this list being the first expression in
            // the sub-expression
            const uint8_t next_precedence{precedence_for_op(ops_.back())};
            if (next_precedence > precedence) {
                // e.g. =a+b*c+1 where the peeked char is '*'
                // next operation has higher precedence than the current
                // list is now =[(=a)(+b)]
                // move last expression (+b) to sub-expression
                //   =[(=a) +[(=b)(*c)(+1)]]
                ops_.pop_back();
                std::unique_ptr<statement> last_stmt_in_expr{
                    std::move(exprs_.back())};
                exprs_.pop_back();
                // start new recursion
                exprs_.emplace_back(make_unique<expr_ops_list>(
                    tc, tz, in_args, false, false, unary_ops{}, next_precedence,
                    std::move(last_stmt_in_expr)));
                // continue parsing expression starting with next operation
                continue;
            }
            // is this in an implied sub-expression and precedence has gone
            // lower?
            if (precedence != initial_precedence and
                next_precedence < precedence and not is_base_expression_) {
                // yes, return to the parent expression
                // e.g., a-b*c+3 => becomes a-(b*c+3) otherwise
                ops_.pop_back();
                return;
            }

            precedence = next_precedence;

            // read the peeked operator
            const char ch{tz.next_char()};

            // handle the 2 characters operator shift
            if (ch == '<') {
                if (tz.next_char() != '<') {
                    throw compiler_exception{tz, "expected operator '<<'"};
                }
            } else if (ch == '>') {
                if (tz.next_char() != '>') {
                    throw compiler_exception{tz, "expected operator '>>'"};
                }
            }

            // check if the next statement is a sub-expression or an expression
            // element
            // e.g. -(a + b)
            unary_ops uo{tz}; // read the unary ops, in this case '-'

            // is it a sub-expression?
            if (tz.is_next_char('(')) {
                // yes, recurse and forward the unary ops to be applied on the
                // whole sub-expression
                exprs_.emplace_back(std::make_unique<expr_ops_list>(
                    tc, tz, in_args, true, true, std::move(uo)));
                continue;
            }

            // not sub-expression, push back unary ops because those belong to
            // the next element
            // e.g. -a+b
            uo.put_back(tz);

            // read the next element
            exprs_.emplace_back(create_statement_in_expr_ops_list(tc, tz));

            // continue to next op + element or sub-expression
        }
    }

    ~expr_ops_list() override = default;

    expr_ops_list() = default;
    expr_ops_list(const expr_ops_list&) = default;
    expr_ops_list(expr_ops_list&&) = default;
    auto operator=(const expr_ops_list&) -> expr_ops_list& = default;
    auto operator=(expr_ops_list&&) -> expr_ops_list& = default;

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        if (enclosed_) {
            std::print(os, "(");
        }
        expression::source_to(os); // whitespace
        exprs_[0]->source_to(os);
        const size_t n{ops_.size()};
        for (size_t i{}; i < n; i++) {
            const char op{ops_[i]};
            std::print(os, "{}", op);
            if (op == '<' or op == '>') {
                // handle case << and >>
                std::print(os, "{}", op);
            }
            exprs_[i + 1]->source_to(os);
        }

        if (enclosed_) {
            std::print(os, ")");
            ws1_.source_to(os);
        }
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        // is destination a register?
        if (dst_info.is_register()) {
            // yes, compile with the result placed in it
            do_compile(tc, os, indent, dst_info);
            return;
        }

        // compile with and without the scratch register to find the best
        // compilation

        // without scratch register
        std::stringstream ss1;
        do_compile(tc, ss1, indent, dst_info);

        // with scratch register
        std::stringstream ss2;
        const std::string reg{tc.alloc_scratch_register(tok(), ss2, indent)};
        const std::string reg_sized{
            tc.get_sized_register_operand(reg, dst_info.type_ptr->size())};
        // note: sized register to propagate operation to destination size
        const ident_info dst_reg_info{
            tc.make_ident_info_for_register(reg_sized)};
        do_compile(tc, ss2, indent, dst_reg_info);
        tc.asm_cmd(tok(), ss2, indent, "mov", dst_info.operand.str(),
                   reg_sized);
        tc.free_scratch_register(tok(), ss2, indent, reg);

        // std::println(std::cerr,
        //              "---------\nwithout scratch register:\n{}\nwith scratch"
        //              " register:\n{}\n---------\n",
        //              ss1.str(), ss2.str());

        // compare instruction count
        const size_t ss1_count{count_instructions(tc, ss1)};
        const size_t ss2_count{count_instructions(tc, ss2)};

        // select the version with the fewest instructions
        if (ss1_count <= ss2_count) {
            std::print(os, "{}", ss1.str());
        } else {
            std::print(os, "{}", ss2.str());
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
    compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                size_t indent, std::vector<std::string>& allocated_registers,
                const std::string& reg_size,
                const std::span<const std::string> lea_path) const
        -> operand override {

        assert(exprs_.size() == 1);

        return exprs_[0]->compile_lea(src_loc_tk, tc, os, indent,
                                      allocated_registers, reg_size, lea_path);
    }

  private:
    auto do_compile(toc& tc, std::ostream& os, const size_t indent,
                    const ident_info& dst_info) const -> void {

        const statement& st0{*exprs_[0]};
        if (st0.is_identifier()) {
            st0.compile(tc, os, indent, dst_info);
        } else {
            // the first element is assigned to destination, operator '='
            asm_op(tc, os, indent, '=', dst_info, st0);
        }

        // remaining elements are +,-,*,/,%,|,&,^,<<,>>
        const size_t n{ops_.size()};
        for (size_t i{}; i < n; i++) {
            const statement& st{*exprs_[i + 1]};
            asm_op(tc, os, indent, ops_[i], dst_info, st);
        }

        // apply unary expressions on destination
        uops_.compile(tc, os, indent, dst_info.operand.str());
    }

    [[nodiscard]] static auto count_instructions(const toc& tc,
                                                 std::stringstream& ss)
        -> size_t {

        std::string line;
        size_t n{};
        while (getline(ss, line)) {
            if (std::regex_search(line, tc.regex_nasm_comment())) {
                continue;
            }
            n++;
        }
        return n;
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

    static auto asm_op(toc& tc, std::ostream& os, const size_t indent,
                       const char op, const ident_info& dst,
                       const statement& src) -> void {

        std::string op_str{op};
        if (op == '<') {
            op_str.push_back('<');
        } else if (op == '>') {
            op_str.push_back('>');
        }

        tc.comment_source(src, os, indent, dst.id, op_str);

        if (op == '=') {
            asm_op_mov(tc, os, indent, dst, src);
            return;
        }
        if (op == '+') {
            asm_op_add_sub(tc, os, indent, "add", "sub", dst, src);
            return;
        }
        if (op == '-') {
            asm_op_add_sub(tc, os, indent, "sub", "add", dst, src);
            return;
        }
        if (op == '*') {
            asm_op_mul(tc, os, indent, dst, src);
            return;
        }
        if (op == '/') {
            asm_op_div(tc, os, indent, "rax", dst, src);
            return;
        }
        if (op == '%') {
            asm_op_div(tc, os, indent, "rdx", dst, src);
            return;
        }
        if (op == '&') {
            asm_op_bitwise(tc, os, indent, "and", dst, src);
            return;
        }
        if (op == '|') {
            asm_op_bitwise(tc, os, indent, "or", dst, src);
            return;
        }
        if (op == '^') {
            asm_op_bitwise(tc, os, indent, "xor", dst, src);
            return;
        }
        if (op == '<') {
            asm_op_shift(tc, os, indent, "sal", dst, src);
            return;
        }
        if (op == '>') {
            asm_op_shift(tc, os, indent, "sar", dst, src);
            return;
        }
    }

    static auto asm_op_mov(toc& tc, std::ostream& os, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        tc.comment_start(src.tok(), os, indent);
        // does 'src' need to be compiled?
        if (src.is_expression()) {
            std::println(os, "= expression");
            // yes, compile with destination to 'dst'
            src.compile(tc, os, indent, dst_info);
            return;
        }

        std::unreachable();
    }

    static auto asm_op_mul(toc& tc, std::ostream& os, const size_t indent,
                           const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.type_ptr->size()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile it to a scratch register
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            // make register sized to destination
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};

            src.compile(tc, os, indent,
                        tc.make_ident_info_for_register(reg_sized));

            // note: 'imul' destination must be a register
            if (dst_info.is_register() and not dst_info.is_memory_operand()) {
                tc.comment_start(src.tok(), os, indent);
                std::println(os, "imul: expr reg");
                tc.asm_cmd(src.tok(), os, indent, "imul",
                           dst_info.operand.str(), reg_sized);
            } else {
                tc.comment_start(src.tok(), os, indent);
                std::println(os, "imul: expr not reg");
                // 'imul' destination is not a register
                tc.asm_cmd(src.tok(), os, indent, "imul", reg_sized,
                           dst_info.operand.str());
                tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                           reg_sized);
            }
            tc.free_scratch_register(src.tok(), os, indent, reg);
            return;
        }

        // not an expression, either a register or memory location, or constant

        const ident_info src_info{tc.make_ident_info(src)};

        // note: 'imul' destination operand must be register

        // note: special case for byte sized multiplication because x86_64 does
        //       not support constant as a second operand for byte size
        //       operation
        if (dst_size == 1) {
            std::vector<std::string> lea_registers;
            const operand src_operand{
                tc.get_lea_operand(os, indent, src, src_info, lea_registers)};
            const unary_ops& uops{src.get_unary_ops()};
            const std::string r1{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string r2{
                tc.alloc_scratch_register(src.tok(), os, indent)};

            tc.asm_cmd(src.tok(), os, indent, "mov", r1,
                       dst_info.operand.str());

            if (src_info.is_const()) {
                tc.comment_start(src.tok(), os, indent);
                std::println(os, "imul: byte const");
                tc.asm_cmd(src.tok(), os, indent, "mov", r2,
                           std::format("{}{}", uops.to_string(),
                                       src_info.const_value));
            } else {
                tc.comment_start(src.tok(), os, indent);
                std::println(os, "imul: byte not const");
                tc.asm_cmd(src.tok(), os, indent, "mov", r2, src_operand.str());
                uops.compile(tc, os, indent, r2);
            }

            tc.asm_cmd(src.tok(), os, indent, "imul", r1, r2);
            tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                       r1);

            tc.free_scratch_register(src.tok(), os, indent, r2);
            tc.free_scratch_register(src.tok(), os, indent, r1);
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        if (not dst_info.is_memory_operand()) {
            // destination is a register
            if (src_info.is_const()) {
                tc.comment_start(src.tok(), os, indent);
                std::println(os, "dst is reg, src is const");
                tc.asm_cmd(src.tok(), os, indent, "imul",
                           dst_info.operand.str(),
                           std::format("{}{}", src.get_unary_ops().to_string(),
                                       src_info.const_value));
                return;
            }

            std::vector<std::string> lea_registers;
            const operand src_operand{
                tc.get_lea_operand(os, indent, src, src_info, lea_registers)};

            const unary_ops& uops{src.get_unary_ops()};
            if (uops.is_empty()) {
                tc.comment_start(src.tok(), os, indent);
                std::println(os, "dst is reg, src is not const, no uops");
                tc.asm_cmd(src.tok(), os, indent, "imul",
                           dst_info.operand.str(), src_operand.str());
                free_registers(src, tc, os, indent, lea_registers);
                return;
            }

            tc.comment_start(src.tok(), os, indent);
            std::println(os, "dst is reg, src is not const, uops");
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};
            tc.asm_cmd(src.tok(), os, indent, "mov", reg_sized,
                       src_operand.str());
            uops.compile(tc, os, indent, reg_sized);
            tc.asm_cmd(src.tok(), os, indent, "imul", dst_info.operand.str(),
                       reg_sized);
            tc.free_scratch_register(src.tok(), os, indent, reg);
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // 'imul' destination is not a register

        if (src_info.is_const()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "dst is not reg, src is const");
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};
            tc.asm_cmd(src.tok(), os, indent, "mov", reg_sized,
                       dst_info.operand.str());
            tc.asm_cmd(src.tok(), os, indent, "imul", reg_sized,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.const_value));
            tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                       reg_sized);
            tc.free_scratch_register(src.tok(), os, indent, reg);
            return;
        }

        // source is not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(os, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "dst is not reg, src is not const, no uops");
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};
            tc.asm_cmd(src.tok(), os, indent, "mov", reg_sized,
                       dst_info.operand.str());
            tc.asm_cmd(src.tok(), os, indent, "imul", reg_sized,
                       src_operand.str());
            tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                       reg_sized);
            tc.free_scratch_register(src.tok(), os, indent, reg);
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // source is not a constant and unary ops need to be applied

        tc.comment_start(src.tok(), os, indent);
        std::println(os, "dst is not reg, src is not const, uops");
        const std::string reg{tc.alloc_scratch_register(src.tok(), os, indent)};
        const std::string reg_sized{
            tc.get_sized_register_operand(reg, dst_size)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg_sized, src_operand.str());
        uops.compile(tc, os, indent, reg_sized);
        tc.asm_cmd(src.tok(), os, indent, "imul", reg_sized,
                   dst_info.operand.str());
        tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                   reg_sized);
        tc.free_scratch_register(src.tok(), os, indent, reg);
        free_registers(src, tc, os, indent, lea_registers);
    }

    static auto asm_op_add_sub(toc& tc, std::ostream& os, const size_t indent,
                               const std::string_view op,
                               const std::string_view op_when_negated,
                               const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.type_ptr->size()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};
            src.compile(tc, os, indent,
                        tc.make_ident_info_for_register(reg_sized));
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       reg_sized);
            tc.free_scratch_register(src.tok(), os, indent, reg);
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.const_value));
            return;
        }

        // 'src' is not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(os, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       src_operand.str());
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // has unary ops

        if (uops.is_only_negated()) {
            // has unary ops
            tc.asm_cmd(src.tok(), os, indent, op_when_negated,
                       dst_info.operand.str(), src_operand.str());
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // multiple unary ops

        const std::string reg{tc.alloc_scratch_register(src.tok(), os, indent)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_operand.str());
        uops.compile(tc, os, indent, reg);
        tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(), reg);
        tc.free_scratch_register(src.tok(), os, indent, reg);
        free_registers(src, tc, os, indent, lea_registers);
    }

    static auto asm_op_bitwise(toc& tc, std::ostream& os, const size_t indent,
                               const std::string_view op,
                               const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.type_ptr->size()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};
            src.compile(tc, os, indent,
                        tc.make_ident_info_for_register(reg_sized));
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       reg_sized);
            tc.free_scratch_register(src.tok(), os, indent, reg);
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.const_value));
            return;
        }

        // 'src' is not an expression and not a constant, an identifier

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(os, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       src_operand.str());
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // 'src' is not an expression and not a constant and has unary ops

        const std::string reg{tc.alloc_scratch_register(src.tok(), os, indent)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_operand.str());
        uops.compile(tc, os, indent, reg);
        tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(), reg);
        tc.free_scratch_register(src.tok(), os, indent, reg);
        free_registers(src, tc, os, indent, lea_registers);
    }

    static auto asm_op_shift(toc& tc, std::ostream& os, const size_t indent,
                             const std::string_view op,
                             const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.type_ptr->size()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "shf: expr");
            // the operand must be stored in register 'CL'
            //? todo. BMI2 (Bit Manipulation Instruction Set 2)
            //        look at shlx/shrx/sarx which can use any register for the
            //        shift amount
            const bool rcx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rcx")};
            if (not rcx_allocated) {
                toc::asm_push(os, indent, "rcx");
            }
            const std::string rcx_sized{
                tc.get_sized_register_operand("rcx", dst_size)};
            // the number of bits to shift is an expression, compile it to 'rcx'
            src.compile(tc, os, indent,
                        tc.make_ident_info_for_register(rcx_sized));
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(), "cl");
            if (rcx_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rcx");
            } else {
                toc::asm_pop(os, indent, "rcx");
            }
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "shf: const");
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(),
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.const_value));
            return;
        }

        if (src_info.operand.str() == "rcx") {
            throw compiler_exception{
                src.tok(), "cannot use 'rcx' as operand in shift because "
                           "that registers is used"};
        }

        // 'src' is not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(os, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "shf: not const, no uops");
            // the operand must be stored in CL (see note above about BMI2)
            const bool rcx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rcx")};
            if (not rcx_allocated) {
                toc::asm_push(os, indent, "rcx");
            }
            const std::string rcx_sized{
                tc.get_sized_register_operand("rcx", dst_size)};
            tc.asm_cmd(src.tok(), os, indent, "mov", rcx_sized,
                       src_operand.str());
            tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(), "cl");
            if (rcx_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rcx");
            } else {
                toc::asm_pop(os, indent, "rcx");
            }
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // unary ops need to be applied on the argument src

        tc.comment_start(src.tok(), os, indent);
        std::println(os, "shf: not const, uops");

        const bool rcx_allocated{
            tc.alloc_named_register(src.tok(), os, indent, "rcx")};
        if (not rcx_allocated) {
            toc::asm_push(os, indent, "rcx");
        }
        const std::string rcx_sized{
            tc.get_sized_register_operand("rcx", dst_size)};
        tc.asm_cmd(src.tok(), os, indent, "mov", "rcx", src_operand.str());
        uops.compile(tc, os, indent, rcx_sized);
        tc.asm_cmd(src.tok(), os, indent, op, dst_info.operand.str(), "cl");
        if (rcx_allocated) {
            tc.free_named_register(src.tok(), os, indent, "rcx");
        } else {
            toc::asm_pop(os, indent, "rcx");
        }
        free_registers(src, tc, os, indent, lea_registers);
    }

    [[nodiscard]] static auto asm_op_div_reg_ext(const size_t op_size)
        -> std::string_view {
        switch (op_size) {
        case toc::size_qword:
            return "cqo";
        case toc::size_dword:
            return "cdq";
        case toc::size_word:
            return "cwde";
        case toc::size_byte:
            return "cbw";
        default:
            std::unreachable();
        }
    }
    // op is either 'rax' for the quotient or 'rdx' for the reminder to be moved
    // into 'dst'
    static auto asm_op_div(toc& tc, std::ostream& os, const size_t indent,
                           const std::string_view op,
                           const ident_info& dst_info, const statement& src)
        -> void {

        const size_t dst_size{dst_info.type_ptr->size()};

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "div expression");
            const std::string reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            const std::string reg_sized{
                tc.get_sized_register_operand(reg, dst_size)};
            src.compile(tc, os, indent,
                        tc.make_ident_info_for_register(reg_sized));
            const bool rax_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rax")};
            if (not rax_allocated) {
                toc::asm_push(os, indent, "rax");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov",
                       tc.get_sized_register_operand("rax", dst_size),
                       dst_info.operand.str());
            const bool rdx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rdx")};
            if (not rdx_allocated) {
                toc::asm_push(os, indent, "rdx");
            }
            toc::indent(os, indent, false);
            std::println(os, "{}", asm_op_div_reg_ext(dst_size));
            toc::indent(os, indent, false);
            std::println(os, "idiv {}", reg);
            tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                       op);
            if (rdx_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rdx");
            } else {
                toc::asm_pop(os, indent, "rdx");
            }
            if (rax_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rax");
            } else {
                toc::asm_pop(os, indent, "rax");
            }
            tc.free_scratch_register(src.tok(), os, indent, reg);
            return;
        }

        // 'src' is not an expression

        const ident_info src_info{tc.make_ident_info(src)};
        if (src_info.is_const()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "div const");
            const bool rax_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rax")};
            if (not rax_allocated) {
                toc::asm_push(os, indent, "rax");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov",
                       tc.get_sized_register_operand("rax", dst_size),
                       dst_info.operand.str());
            const bool rdx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rdx")};
            if (not rdx_allocated) {
                toc::asm_push(os, indent, "rdx");
            }
            toc::indent(os, indent, false);
            std::println(os, "{}", asm_op_div_reg_ext(dst_size));
            const std::string scratch_reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            tc.asm_cmd(src.tok(), os, indent, "mov", scratch_reg,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.const_value));
            toc::indent(os, indent, false);
            std::println(os, "idiv {}", scratch_reg);
            tc.free_scratch_register(src.tok(), os, indent, scratch_reg);
            tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                       op);
            if (rdx_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rdx");
            } else {
                toc::asm_pop(os, indent, "rdx");
            }
            if (rax_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rax");
            } else {
                toc::asm_pop(os, indent, "rax");
            }
            return;
        }

        if (src_info.operand.str() == "rdx" or
            src_info.operand.str() == "rax") {
            throw compiler_exception{
                src.tok(), "cannot use 'rdx' or 'rax' as operands in "
                           "division because those registers are used"};
        }

        // 'src' is not an expression and not a constant

        std::vector<std::string> lea_registers;
        const operand src_operand{
            tc.get_lea_operand(os, indent, src, src_info, lea_registers)};

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.comment_start(src.tok(), os, indent);
            std::println(os, "div not const, no uops");
            const bool rax_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rax")};
            if (not rax_allocated) {
                toc::asm_push(os, indent, "rax");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov",
                       tc.get_sized_register_operand("rax", dst_size),
                       dst_info.operand.str());
            const bool rdx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rdx")};
            if (not rdx_allocated) {
                toc::asm_push(os, indent, "rdx");
            }
            toc::indent(os, indent, false);
            std::println(os, "{}", asm_op_div_reg_ext(dst_size));
            toc::indent(os, indent, false);
            std::println(os, "idiv {}", src_operand.str());
            // op is either 'rax' for the quotient or 'rdx' for the reminder
            tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(),
                       op);
            if (rdx_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rdx");
            } else {
                toc::asm_pop(os, indent, "rdx");
            }
            if (rax_allocated) {
                tc.free_named_register(src.tok(), os, indent, "rax");
            } else {
                toc::asm_pop(os, indent, "rax");
            }
            free_registers(src, tc, os, indent, lea_registers);
            return;
        }

        // 'src' is not an expression and not a constant and has unary ops

        tc.comment_start(src.tok(), os, indent);
        std::println(os, "div not const, uops");
        const std::string reg{tc.alloc_scratch_register(src.tok(), os, indent)};
        const std::string reg_sized{
            tc.get_sized_register_operand(reg, dst_size)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg_sized, src_operand.str());
        uops.compile(tc, os, indent, reg_sized);
        const bool rax_allocated{
            tc.alloc_named_register(src.tok(), os, indent, "rax")};
        if (not rax_allocated) {
            toc::asm_push(os, indent, "rax");
        }
        tc.asm_cmd(src.tok(), os, indent, "mov",
                   tc.get_sized_register_operand("rax", dst_size),
                   dst_info.operand.str());
        const bool rdx_allocated{
            tc.alloc_named_register(src.tok(), os, indent, "rdx")};
        if (not rdx_allocated) {
            toc::asm_push(os, indent, "rdx");
        }
        toc::indent(os, indent, false);
        std::println(os, "{}", asm_op_div_reg_ext(dst_size));
        toc::indent(os, indent, false);
        std::println(os, "idiv {}", reg_sized);
        tc.asm_cmd(src.tok(), os, indent, "mov", dst_info.operand.str(), op);
        if (rdx_allocated) {
            tc.free_named_register(src.tok(), os, indent, "rdx");
        } else {
            toc::asm_pop(os, indent, "rdx");
        }
        if (rax_allocated) {
            tc.free_named_register(src.tok(), os, indent, "rax");
        } else {
            toc::asm_pop(os, indent, "rax");
        }
        tc.free_scratch_register(src.tok(), os, indent, reg);
        free_registers(src, tc, os, indent, lea_registers);
    }

    static auto free_registers(const statement& st, toc& tc, std::ostream& os,
                               const size_t indent,
                               const std::span<const std::string> registers)
        -> void {

        for (const std::string& reg : registers | std::views::reverse) {
            tc.free_scratch_register(st.tok(), os, indent, reg);
        }
    }
};
