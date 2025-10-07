#pragma once
// reviewed: 2025-09-28

#include <sstream>
#include <vector>

#include "decouple.hpp"
#include "expression.hpp"
#include "toc.hpp"

// a list of elements / lists connected by operators instead of tree
// note: quirky parsing but trivial compilation
class expr_ops_list final : public expression {
    std::vector<std::unique_ptr<statement>> exprs_; // expressions list
    std::vector<char> ops_;     // operators between elements in the vector
    unary_ops uops_;            // unary ops for all result e.g. ~(a+b)
    token ws1_;                 // whitespace after parenthesis when enclosed
    bool enclosed_{};           //  (a+b) vs a+b
    bool is_base_expression_{}; // false when in implied sub-expressions

  public:
    expr_ops_list(toc& tc, tokenizer& tz, const bool in_args = false,
                  const bool enclosed = false,
                  const bool is_base_expression = true, unary_ops uops = {},
                  const char first_op_precedence = initial_precedence,
                  std::unique_ptr<statement> first_expression = {})
        : expression{tz.next_whitespace_token()}, uops_{std::move(uops)},
          enclosed_{enclosed}, is_base_expression_{is_base_expression} {

        // is this in a recursion?
        if (first_expression) {
            // yes, add provided first expression
            exprs_.emplace_back(std::move(first_expression));
        } else {
            // no, read first expression or start a new recursion
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
        char precedence{first_op_precedence};

        while (true) { // +a  +3
            // if end of sub-expression
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
            // the last expression in this list being first expression in the
            // sub-expression
            const char next_precedence{precedence_for_op(ops_.back())};
            if (next_precedence > precedence) {
                // e.g. =a+b*c+1 where the peeked char is '*'
                // next operation has higher precedence than current
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
                // yes, return to parent expression
                // e.g. a-b*c+3 => becomes a-(b*c+3) otherwise
                ops_.pop_back();
                return;
            }

            precedence = next_precedence;

            // read the peeked operator
            const char ch{tz.next_char()};

            // handle the two character operator shift
            if (ch == '<') {
                if (tz.next_char() != '<') {
                    throw compiler_exception{tz, "expected operator '<<'"};
                }
            } else if (ch == '>') {
                if (tz.next_char() != '>') {
                    throw compiler_exception{tz, "expected operator '>>'"};
                }
            }

            // check if next statement is a sub-expression or an expression
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

            // read next element
            exprs_.emplace_back(create_statement_in_expr_ops_list(tc, tz));

            // continue to next op + element or sub-expression
        }
    }

    expr_ops_list() = default;
    expr_ops_list(const expr_ops_list&) = default;
    expr_ops_list(expr_ops_list&&) = default;
    auto operator=(const expr_ops_list&) -> expr_ops_list& = default;
    auto operator=(expr_ops_list&&) -> expr_ops_list& = default;

    ~expr_ops_list() override = default;

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        if (enclosed_) {
            os << "(";
        }
        expression::source_to(os); // whitespace
        exprs_.at(0)->source_to(os);
        const size_t n{ops_.size()};
        for (size_t i{}; i < n; i++) {
            const char op{ops_.at(i)};
            os << op;
            if (op == '<' or op == '>') {
                // handle case << and >>
                os << op;
            }
            exprs_.at(i + 1)->source_to(os);
        }

        if (enclosed_) {
            os << ")";
            ws1_.source_to(os);
        }
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 std::string_view dst) const -> void override {

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        // is destination a register?
        if (dst_info.is_register()) {
            // yes, compile with result placed in it
            do_compile(tc, os, indent, dst_info);
            return;
        }

        // compile with and without scratch register to find best compilation

        // without scratch register
        std::stringstream ss1;
        do_compile(tc, ss1, indent, dst_info);

        // with scratch register
        std::stringstream ss2;
        const std::string& reg{tc.alloc_scratch_register(tok(), ss2, indent)};
        const ident_info dest_reg{tc.make_ident_info(tok(), reg, false)};
        do_compile(tc, ss2, indent, dst_info);
        tc.asm_cmd(tok(), ss2, indent, "mov", dst_info.id_nasm, reg);
        tc.free_scratch_register(ss2, indent, reg);

        // compare instruction count
        const size_t ss1_count{count_instructions(ss1)};
        const size_t ss2_count{count_instructions(ss2)};

        // select version with least instructions
        if (ss1_count <= ss2_count) {
            os << ss1.str();
        } else {
            os << ss2.str();
        }
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(exprs_.size() == 1);

        return exprs_.at(0)->identifier();
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        // is this list one element?
        if (exprs_.size() == 1) {
            // then the unary ops are on the first element
            return exprs_.at(0)->get_unary_ops();
        }
        // in multi-element list, unary ops for all are on the current list
        // element
        return uops_;
    }

    [[nodiscard]] auto get_type() const -> const type& override {
        assert(not exprs_.empty());
        // return size of first element
        //? todo.  find the size of the largest integral element
        return exprs_.at(0)->get_type();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        // if unary operators on list then it will need to compile expression
        if (not uops_.is_empty()) {
            return true;
        }

        // if only 1 element then it decides if it is an expression
        if (exprs_.size() == 1) {
            return exprs_.at(0)->is_expression();
        }

        // more than 1 element, automatically an expression
        return true;
    }

  private:
    auto do_compile(toc& tc, std::ostream& os, const size_t indent,
                    const ident_info& dst_info) const -> void {

        tc.comment_source(*this, os, indent);

        // first element is assigned to destination, operator '='
        asm_op(tc, os, indent, '=', dst_info, *exprs_.at(0));

        // remaining elements are +,-,*,/,%,|,&,^,<<,>>
        const size_t n{ops_.size()};
        for (size_t i{}; i < n; i++) {
            asm_op(tc, os, indent, ops_.at(i), dst_info, *exprs_.at(i + 1));
        }

        // apply unary expressions on destination
        uops_.compile(tc, os, indent, dst_info.id_nasm);
    }

    static auto count_instructions(std::stringstream& ss) -> size_t {
        const std::regex rxcomment{R"(^\s*;.*$)"};
        std::string line;
        size_t n{};
        while (getline(ss, line)) {
            if (std::regex_search(line, rxcomment)) {
                continue;
            }
            n++;
        }
        return n;
    }

    // higher value higher precedence
    static auto precedence_for_op(const char ch) -> char {
        switch (ch) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
        case '%':
            return 2;
        case '|':
            return 3;
        case '&':
            return 4;
        case '^':
            return 5;
        case '<': // shift left
        case '>': // shift right
            return 6;
        default:
            throw panic_exception("unexpected code path expr_ops_list:1");
        }
    }

    // higher than the highest precedence
    static constexpr char initial_precedence{7};

    static auto asm_op(toc& tc, std::ostream& os, const size_t indent,
                       const char op, const ident_info& dst,
                       const statement& src) -> void {
        toc::indent(os, indent, true);
        std::string op_str{op};
        if (op == '<') {
            op_str.push_back('<');
        } else if (op == '>') {
            op_str.push_back('>');
        }

        tc.comment_source(os, dst.id, op_str, src);

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
                           const ident_info& dst, const statement& src)
        -> void {

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile with destination to 'dst'
            src.compile(tc, os, indent, dst.id);
            return;
        }

        const ident_info& src_info{tc.make_ident_info(src, true)};
        // is 'src' a constant?
        if (src_info.is_const()) {
            // move it to 'dst' with unary ops
            tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.id_nasm));
            return;
        }

        // move 'dst' to 'src' and compile the unary ops since 'src' is not a
        // constant
        tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, src_info.id_nasm);
        src.get_unary_ops().compile(tc, os, indent, dst.id_nasm);
    }

    static auto asm_op_mul(toc& tc, std::ostream& os, const size_t indent,
                           const ident_info& dst, const statement& src)
        -> void {

        // does 'src' need to be compiled?
        if (src.is_expression()) {
            // yes, compile it to a scratch register
            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            src.compile(tc, os, indent, reg);
            // note 'imul' destination must be a register
            if (dst.is_register()) {
                tc.asm_cmd(src.tok(), os, indent, "imul", dst.id_nasm, reg);
            } else {
                // 'imul' destination is not a register
                tc.asm_cmd(src.tok(), os, indent, "imul", reg, dst.id_nasm);
                tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, reg);
            }
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // not an expression, either a register or memory location
        const ident_info& src_info{tc.make_ident_info(src, true)};
        // 'imul' destination operand must be register
        if (dst.is_register()) {
            if (src_info.is_const()) {
                tc.asm_cmd(src.tok(), os, indent, "imul", dst.id_nasm,
                           std::format("{}{}", src.get_unary_ops().to_string(),
                                       src_info.id_nasm));
                return;
            }

            const unary_ops& uops{src.get_unary_ops()};
            if (uops.is_empty()) {
                tc.asm_cmd(src.tok(), os, indent, "imul", dst.id_nasm,
                           src_info.id_nasm);
                return;
            }

            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_info.id_nasm);
            uops.compile(tc, os, indent, reg);
            tc.asm_cmd(src.tok(), os, indent, "imul", dst.id_nasm, reg);
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // 'imul' destination is not a register
        if (src_info.is_const()) {
            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            tc.asm_cmd(src.tok(), os, indent, "mov", reg, dst.id_nasm);
            tc.asm_cmd(src.tok(), os, indent, "imul", reg,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.id_nasm));
            tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, reg);
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // source is not a constant
        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            tc.asm_cmd(src.tok(), os, indent, "mov", reg, dst.id_nasm);
            tc.asm_cmd(src.tok(), os, indent, "imul", reg, src_info.id_nasm);
            tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, reg);
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // source is not a constant and unary ops need to be applied
        const std::string& reg{
            tc.alloc_scratch_register(src.tok(), os, indent)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_info.id_nasm);
        uops.compile(tc, os, indent, reg);
        tc.asm_cmd(src.tok(), os, indent, "imul", reg, dst.id_nasm);
        tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, reg);
        tc.free_scratch_register(os, indent, reg);
    }

    static auto asm_op_add_sub(toc& tc, std::ostream& os, const size_t indent,
                               const std::string& op,
                               const std::string& op_when_negated,
                               const ident_info& dst, const statement& src)
        -> void {

        if (src.is_expression()) {
            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            src.compile(tc, os, indent, reg);
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, reg);
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // 'src' is not an expression
        const ident_info& src_info{tc.make_ident_info(src, true)};
        if (src_info.is_const()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.id_nasm));
            return;
        }

        // 'src' is not a constant
        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm,
                       src_info.id_nasm);
            return;
        }

        // has unary ops
        if (uops.is_only_negated()) {
            tc.asm_cmd(src.tok(), os, indent, op_when_negated, dst.id_nasm,
                       src_info.id_nasm);
            return;
        }

        // multiple unary ops
        const std::string& reg{
            tc.alloc_scratch_register(src.tok(), os, indent)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_info.id_nasm);
        uops.compile(tc, os, indent, reg);
        tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, reg);
        tc.free_scratch_register(os, indent, reg);
    }

    static auto asm_op_bitwise(toc& tc, std::ostream& os, const size_t indent,
                               const std::string& op, const ident_info& dst,
                               const statement& src) -> void {

        if (src.is_expression()) {
            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            src.compile(tc, os, indent, reg);
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, reg);
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // 'src' is not an expression
        const ident_info& src_info{tc.make_ident_info(src, true)};
        if (src_info.is_const()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.id_nasm));
            return;
        }

        // 'src' is not an expression and not a constant
        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm,
                       src_info.id_nasm);
            return;
        }

        // 'src' is not an expression and not a constant and has unary ops
        const std::string& reg{
            tc.alloc_scratch_register(src.tok(), os, indent)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_info.id_nasm);
        uops.compile(tc, os, indent, reg);
        tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, reg);
        tc.free_scratch_register(os, indent, reg);
    }

    static auto asm_op_shift(toc& tc, std::ostream& os, const size_t indent,
                             const std::string& op, const ident_info& dst,
                             const statement& src) -> void {

        if (src.is_expression()) {
            // the operand must be stored in register 'CL'
            //? todo. BMI2 (Bit Manipulation Instruction Set 2)
            //        look at shlx/shrx/sarx which can use any register for the
            //        shift amount
            const bool rcx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rcx")};
            if (not rcx_allocated) {
                toc::asm_push(src.tok(), os, indent, "rcx");
            }
            // the number of bits to shift is an expression, compile it to 'rcx'
            src.compile(tc, os, indent, "rcx");
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, "cl");
            if (rcx_allocated) {
                tc.free_named_register(os, indent, "rcx");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rcx");
            }
            return;
        }

        // 'src' is not an expression
        const ident_info& src_info{tc.make_ident_info(src, true)};
        if (src_info.is_const()) {
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.id_nasm));
            return;
        }

        if (src_info.id_nasm == "rcx") {
            throw compiler_exception{
                src.tok(), "cannot use 'rcx' as operand in shift because "
                           "that registers is used"};
        }

        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            // the operand must be stored in CL (see note above about BMI2)
            const bool rcx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rcx")};
            if (not rcx_allocated) {
                toc::asm_push(src.tok(), os, indent, "rcx");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov", "rcx", src_info.id_nasm);
            tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, "cl");
            if (rcx_allocated) {
                tc.free_named_register(os, indent, "rcx");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rcx");
            }
            return;
        }

        // unary ops need to be applied on the argument src
        const bool rcx_allocated{
            tc.alloc_named_register(src.tok(), os, indent, "rcx")};
        if (not rcx_allocated) {
            toc::asm_push(src.tok(), os, indent, "rcx");
        }
        tc.asm_cmd(src.tok(), os, indent, "mov", "rcx", src_info.id_nasm);
        uops.compile(tc, os, indent, "rcx");
        tc.asm_cmd(src.tok(), os, indent, op, dst.id_nasm, "cl");
        if (rcx_allocated) {
            tc.free_named_register(os, indent, "rcx");
        } else {
            toc::asm_pop(src.tok(), os, indent, "rcx");
        }
    }

    // op is either 'rax' for the quotient or 'rdx' for the reminder to be moved
    // into 'dst'
    static auto asm_op_div(toc& tc, std::ostream& os, const size_t indent,
                           const std::string& op, const ident_info& dst,
                           const statement& src) -> void {

        if (src.is_expression()) {
            const std::string& reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            src.compile(tc, os, indent, reg);
            const bool rax_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rax")};
            if (not rax_allocated) {
                toc::asm_push(src.tok(), os, indent, "rax");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov", "rax", dst.id_nasm);
            const bool rdx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rdx")};
            if (not rdx_allocated) {
                toc::asm_push(src.tok(), os, indent, "rdx");
            }
            toc::indent(os, indent, false);
            os << "cqo\n";
            toc::indent(os, indent, false);
            os << "idiv " << reg << '\n';
            tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, op);
            if (rdx_allocated) {
                tc.free_named_register(os, indent, "rdx");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rdx");
            }
            if (rax_allocated) {
                tc.free_named_register(os, indent, "rax");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rax");
            }
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // 'src' is not an expression
        const ident_info& src_info{tc.make_ident_info(src, true)};
        if (src_info.is_const()) {
            const bool rax_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rax")};
            if (not rax_allocated) {
                toc::asm_push(src.tok(), os, indent, "rax");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov", "rax", dst.id_nasm);
            const bool rdx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rdx")};
            if (not rdx_allocated) {
                toc::asm_push(src.tok(), os, indent, "rdx");
            }
            toc::indent(os, indent, false);
            os << "cqo\n";
            const std::string& scratch_reg{
                tc.alloc_scratch_register(src.tok(), os, indent)};
            tc.asm_cmd(src.tok(), os, indent, "mov", scratch_reg,
                       std::format("{}{}", src.get_unary_ops().to_string(),
                                   src_info.id_nasm));
            toc::indent(os, indent, false);
            os << "idiv " << scratch_reg << '\n';
            tc.free_scratch_register(os, indent, scratch_reg);
            tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, op);
            if (rdx_allocated) {
                tc.free_named_register(os, indent, "rdx");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rdx");
            }
            if (rax_allocated) {
                tc.free_named_register(os, indent, "rax");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rax");
            }
            return;
        }

        if (src_info.id_nasm == "rdx" or src_info.id_nasm == "rax") {
            throw compiler_exception{
                src.tok(), "cannot use 'rdx' or 'rax' as operands in "
                           "division because those registers are used"};
        }

        // 'src' is not an expression and not a constant
        const unary_ops& uops{src.get_unary_ops()};
        if (uops.is_empty()) {
            const bool rax_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rax")};
            if (not rax_allocated) {
                toc::asm_push(src.tok(), os, indent, "rax");
            }
            tc.asm_cmd(src.tok(), os, indent, "mov", "rax", dst.id_nasm);
            const bool rdx_allocated{
                tc.alloc_named_register(src.tok(), os, indent, "rdx")};
            if (not rdx_allocated) {
                toc::asm_push(src.tok(), os, indent, "rdx");
            }
            toc::indent(os, indent, false);
            os << "cqo\n";
            toc::indent(os, indent, false);
            os << "idiv " << src_info.id_nasm << '\n';
            // op is either 'rax' for the quotient or 'rdx' for the reminder
            tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, op);
            if (rdx_allocated) {
                tc.free_named_register(os, indent, "rdx");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rdx");
            }
            if (rax_allocated) {
                tc.free_named_register(os, indent, "rax");
            } else {
                toc::asm_pop(src.tok(), os, indent, "rax");
            }
            return;
        }

        // 'src' is not an expression and not a constant and has unary ops
        const std::string& reg{
            tc.alloc_scratch_register(src.tok(), os, indent)};
        tc.asm_cmd(src.tok(), os, indent, "mov", reg, src_info.id_nasm);
        uops.compile(tc, os, indent, reg);
        const bool rax_allocated{
            tc.alloc_named_register(src.tok(), os, indent, "rax")};
        if (not rax_allocated) {
            toc::asm_push(src.tok(), os, indent, "rax");
        }
        tc.asm_cmd(src.tok(), os, indent, "mov", "rax", dst.id_nasm);
        const bool rdx_allocated{
            tc.alloc_named_register(src.tok(), os, indent, "rdx")};
        if (not rdx_allocated) {
            toc::asm_push(src.tok(), os, indent, "rdx");
        }
        toc::indent(os, indent, false);
        os << "cqo\n";
        toc::indent(os, indent, false);
        os << "idiv " << reg << '\n';
        tc.asm_cmd(src.tok(), os, indent, "mov", dst.id_nasm, op);
        if (rdx_allocated) {
            tc.free_named_register(os, indent, "rdx");
        } else {
            toc::asm_pop(src.tok(), os, indent, "rdx");
        }
        if (rax_allocated) {
            tc.free_named_register(os, indent, "rax");
        } else {
            toc::asm_pop(src.tok(), os, indent, "rax");
        }
        tc.free_scratch_register(os, indent, reg);
    }
};
