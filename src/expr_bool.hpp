#pragma once
// reviewed: 2025-09-29

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "decouple.hpp"
#include "expr_arith.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "ub_unset_var.hpp"

class expr_bool_op final : public statement {
    std::vector<token> nots_;
    expr_arith lhs_;
    token ws_pre_op_;
    machine::comparison_operator op_{machine::comparison_operator::equal};
    token ws_post_op_;
    expr_arith rhs_;
    bool is_not_{};       // e.g. if not a == b ...
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

        set_type(tc.get_type_bool());

        bool is_not{};
        // e.g. if not a == 3 ...
        while (not first_expression) {
            const token t{tz.next_token()};

            if (not t.is_text("not")) {
                tz.put_back_token(t);
                break;
            }

            is_not = not is_not;
            nots_.emplace_back(t);
        }

        is_not_ = is_not;

        lhs_ = {tc,
                tz,
                true,
                false,
                {},
                false,
                {},
                expr_arith::initial_precedence,
                std::move(first_expression)};

        ws_pre_op_ = tz.next_whitespace_token();

        const std::optional<machine::comparison_operator> comparison{
            parse_comparison_operator(tz),
        };

        // e.g. if a ...
        if (not comparison) {
            is_shorthand_ = true;
            resolve_if_op_is_expression();

            return;
        }

        op_ = *comparison;

        ws_post_op_ = tz.next_whitespace_token();

        rhs_ = {tc, tz, true};

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

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(not is_expression_);

        return lhs_.identifier();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return is_expression_;
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

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

    [[nodiscard]] auto compile_and(toc& tc, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const bool inverted, const operand& dst,
                                   const bool branch_required = true) const
        -> std::optional<bool> {

        return compile_element(tc, indent, "and", inverted,
                               {
                                   .operation{op_},
                                   .inverted{inverted != is_not_},
                                   .destination{dst},
                                   .target{jmp_to_if_false},
                                   .branch_on_true{},
                               },
                               branch_required);
    }

    // returns an optional bool, and if defined the expression evaluated to
    // the value of the optional
    [[nodiscard]] auto compile_or(toc& tc, const size_t indent,
                                  const std::string_view jmp_to_if_true,
                                  const bool inverted, const operand& dst) const
        -> std::optional<bool> {

        return compile_element(tc, indent, "or", inverted,
                               {
                                   .operation{op_},
                                   .inverted{inverted != is_not_},
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
            return (*lhs_value != 0) != is_not_;
        }

        const std::optional<int64_t> rhs_value{side_constant(tc, rhs_)};

        if (not rhs_value) {
            return std::nullopt;
        }

        return eval_constant(*lhs_value, op_, *rhs_value) != is_not_;
    }

    [[nodiscard]] auto create_cmp_bgn_label(const toc& tc) const
        -> std::string {

        return tc.create_unique_label(tok(), "cmp");
    }

  private:
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
        -> bool {

        machine& x{tc.machine()};

        x.comment(lhs_.tok(), indent, "const eval to {}",
                  (value ? "true" : "false"));

        if (value == action.branch_on_true) {
            x.branch(indent, action.target);
        }

        return value;
    }

    // an 'or' element branches when true and an 'and' element when false
    [[nodiscard]] auto compile_element(toc& tc, const size_t indent,
                                       const std::string_view list_op,
                                       const bool inverted,
                                       const machine::comparison_action& action,
                                       const bool branch_required) const
        -> std::optional<bool> {

        machine& x{tc.machine()};

        x.comment(tok(), indent,
                  statement::trimmed_source(*this, "?",
                                            comment_label(list_op, inverted)));

        x.label(indent, create_cmp_bgn_label(tc));

        const std::optional<bool> constant{constant_value(tc)};

        if (constant) {
            return compile_constant(tc, indent, *constant != inverted, action);
        }

        // a constant 'lhs' becomes an immediate 'rhs' instead of a scratch
        // register copy, the width check then applies to the swapped sides
        if (not is_shorthand_ and side_constant(tc, lhs_)) {
            machine::comparison_action mirrored_action{action};
            mirrored_action.operation = machine::mirrored(op_);

            resolve_cmp(tc, indent, rhs_, lhs_, mirrored_action);

            return std::nullopt;
        }

        if (is_memory_comparison_) {
            resolve_memory_cmp(tc, indent, action);
            return std::nullopt;
        }

        if (not is_shorthand_) {
            resolve_cmp(tc, indent, lhs_, rhs_, action);
            return std::nullopt;
        }

        assert_not_instance_condition(lhs_);

        machine::comparison_action shorthand_action{action};
        shorthand_action.operation = machine::comparison_operator::not_equal;

        // only a shorthand omits its branch, a comparison keeps it
        if (not branch_required) {
            shorthand_action.target = {};
        }

        resolve_cmp_shorthand(tc, indent, lhs_, shorthand_action);

        return std::nullopt;
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

            machine& x{tc.machine()};

            if (not action.target.empty()) {
                machine::comparison_action branch_action{action};
                // branch on the stored result using 'action.branch_on_true'

                branch_action.destination = {};
                // 'compile_boolean' already stored the result and applied
                // inversion

                branch_action.inverted = false;

                x.compare_and_branch(tok(), indent, action.destination,
                                     operand::imm("0", tc.get_type_default()),
                                     branch_action, {});
            }

            return;
        }

        std::vector<operand> allocated_registers;

        // optimization: copy a plain 'bool' instead of evaluating the
        // shorthand as '!= 0'
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
                             operand::imm("0", tc.get_type_default()), action,
                             allocated_registers);
    }

    auto resolve_if_op_is_expression() -> void {
        // a negated expression is an expression
        if (is_not_) {
            is_expression_ = true;
            return;
        }

        if (not is_shorthand_) {
            is_expression_ = true;
            return;
        }

        // a shorthand condition on an expression
        if (lhs_.is_expression()) {
            is_expression_ = true;
            return;
        }

        // if not expression, then it is a single statement and identifier is
        // valid
        const std::string_view id{lhs_.identifier()};

        // a boolean value is not an expression
        if (id == "true" or id == "false") {
            is_expression_ = false;
            return;
        }

        // not a boolean value thus a variable
        is_expression_ = true;
    }

    // the bytes are compared and the result is branched on as for a shorthand
    // condition
    auto resolve_memory_cmp(toc& tc, const size_t indent,
                            const machine::comparison_action& action) const
        -> void {

        const statement& lhs{lhs_.identifier_statement()};
        const statement& rhs{rhs_.identifier_statement()};

        const bool is_not_equal{op_ == machine::comparison_operator::not_equal};
        machine& x{tc.machine()};

        if (not action.destination.is_empty()) {
            compile_memory_equality(tc, indent + 1, tok(), lhs, rhs,
                                    action.destination,
                                    action.inverted != is_not_equal);

            if (action.target.empty()) {
                return;
            }

            machine::comparison_action branch_action{action};
            // the stored result has the inversion applied

            branch_action.destination = {};
            branch_action.inverted = false;

            x.compare_and_branch(tok(), indent, action.destination,
                                 operand::imm("0", tc.get_type_default()),
                                 branch_action, {});

            return;
        }

        const operand result{
            x.alloc_scratch_register(tok(), indent, tc.get_type_bool()),
        };

        compile_memory_equality(tc, indent + 1, tok(), lhs, rhs, result,
                                is_not_equal);

        machine::comparison_action branch_action{action};
        branch_action.operation = machine::comparison_operator::not_equal;

        const std::vector<operand> allocated_registers{result};

        x.compare_and_branch(tok(), indent, result,
                             operand::imm("0", tc.get_type_default()),
                             branch_action, allocated_registers);
    }

    //
    // statics
    //

    // emits the address of 'side' for the comparison of memory
    [[nodiscard]] static auto
    address_emitter_of(toc& tc, const size_t indent, const token& src_loc_tk,
                       const statement& side, const ident_info& info)
        -> machine::address_emitter {

        return [&tc, indent, &src_loc_tk, &side, &info](
                   const operand& reg_count, const operand& address_register,
                   const machine::address_use use) -> void {
            side.compile_address(tc, indent, src_loc_tk,
                                 {
                                     .reg_count{reg_count},
                                     .lea_path{info.lea_path},
                                     .address_register{address_register},
                                 },
                                 use);
        };
    }

    // a condition tests a number, an instance would be tested as its first
    // field; checked when compiling since the initializer of a 'var' parses
    // its expression to learn the type, e.g. 'var b = a' copies the instance
    static auto assert_not_instance_condition(const expr_arith& side) -> void {
        if (side.get_type().is_builtin()) {
            return;
        }

        throw compiler_exception{
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
            throw compiler_exception{
                side.tok(),
                std::format("an instance of '{}' is compared only with an "
                            "instance of the same type",
                            type_name)};
        }

        throw compiler_exception{
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

        if (not rhs_op.is_immediate()) {
            const type& rhs_type{rhs_op.type_ref()};

            if (rhs_type.size_bytes() <= lhs_type.size_bytes()) {
                return;
            }

            throw compiler_exception{
                rhs.tok(),
                std::format(
                    "'{}' of type '{}' is wider than '{}' of type '{}', "
                    "swap the operands: '{} {} {}'",
                    trimmed_source(rhs), rhs_type.name(), trimmed_source(lhs),
                    lhs_type.name(), trimmed_source(rhs),
                    machine::source_text(machine::mirrored(op)),
                    trimmed_source(lhs))};
        }

        const std::optional<int64_t> constant{side_constant(tc, rhs)};

        assert(constant);

        const int64_t value{*constant};

        if (fits_size_bytes(value, lhs_type.size_bytes())) {
            return;
        }

        throw compiler_exception{
            rhs.tok(),
            std::format("constant '{}' does not fit '{}' of type '{}'", value,
                        trimmed_source(lhs), lhs_type.name())};
    }

    // the bytes that both sides hold, they hold the same type and, as arrays,
    // the same number of elements
    [[nodiscard]] static auto
    compared_size_bytes(const statement& lhs, const statement& rhs,
                        const ident_info& lhs_info, const ident_info& rhs_info)
        -> size_t {

        if (not lhs_info.type_ref().is_same(rhs_info.type_ref())) {
            throw compiler_exception{
                rhs.tok(),
                std::format("source and compare types are not the "
                            "same. source is '{}' and compare is '{}'",
                            lhs_info.type_ref().name(),
                            rhs_info.type_ref().name())};
        }

        // a whole array is not compared as its first element
        if (lhs_info.is_array != rhs_info.is_array) {
            toc::assert_not_whole_array(lhs, lhs_info);
            toc::assert_not_whole_array(rhs, rhs_info);
        }

        const size_t element_size_bytes{lhs_info.type_ref().size_bytes()};

        if (not lhs_info.is_array) {
            return element_size_bytes;
        }

        // check comparing 2 arrays of the same size without indexing
        // note: a whole array on one side only was rejected above

        if (lhs_info.array_len != rhs_info.array_len) {
            throw compiler_exception{lhs.tok(),
                                     "cannot compare arrays of different "
                                     "sizes"};
        }

        return multiply_storage_size(lhs.tok(), element_size_bytes,
                                     lhs_info.array_len);
    }

    // compares the bytes of two user type instances or of two whole arrays,
    // 1 is put into 'dst' when they are equal, 0 when 'inverted'
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

        machine& x{tc.machine()};

        x.memory_equal(
            src_loc_tk, indent, size_bytes,
            {
                .alignment{lhs_info.type_ref().alignment()},
                .lhs{address_emitter_of(tc, indent, src_loc_tk, lhs, lhs_info)},
                .rhs{address_emitter_of(tc, indent, src_loc_tk, rhs, rhs_info)},
                .dst{dst},
                .inverted{inverted},
            });
    }

    [[nodiscard]] static auto
    compile_to_scratch(toc& tc, const size_t indent, const expr_arith& expr,
                       std::vector<operand>& allocated_registers) -> operand {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(expr.tok(), indent, expr.get_type()),
        };

        allocated_registers.emplace_back(reg);
        expr.compile(tc, indent + 1, toc::make_ident_info_from_register(reg));

        return reg;
    }

    [[nodiscard]] static auto
    eval_constant(const int64_t lh, const machine::comparison_operator op,
                  const int64_t rh) -> bool {

        if (op == machine::comparison_operator::equal) {
            return lh == rh;
        }

        if (op == machine::comparison_operator::not_equal) {
            return lh != rh;
        }

        if (op == machine::comparison_operator::less) {
            return lh < rh;
        }

        if (op == machine::comparison_operator::less_equal) {
            return lh <= rh;
        }

        if (op == machine::comparison_operator::greater) {
            return lh > rh;
        }

        assert(op == machine::comparison_operator::greater_equal);

        return lh >= rh;
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

        if (not side.is_identifier() or not side.get_unary_ops().is_empty()) {
            return false;
        }

        const ident_info info{tc.make_ident_info(side.identifier_statement())};

        return not info.is_const() and
               (info.is_array or not info.type_ref().is_builtin());
    }

    // memory is compared, so a constant has nothing to compare
    [[nodiscard]] static auto
    make_memory_operand_info(const toc& tc, const statement& identifier)
        -> ident_info {

        ident_info info{tc.make_ident_info(identifier)};

        if (info.is_const()) {
            throw compiler_exception{identifier.tok(),
                                     "constant not supported"};
        }

        return info;
    }

    // the comparison at the next characters, none for a shorthand condition
    [[nodiscard]] static auto parse_comparison_operator(tokenizer& tz)
        -> std::optional<machine::comparison_operator> {

        if (tz.is_next_char('=')) {
            if (not tz.is_next_char('=')) {
                // the error is at the '=' that is alone
                tz.put_back_char('=');

                throw compiler_exception{tz, "expected '=='"};
            }

            return machine::comparison_operator::equal;
        }

        if (tz.is_next_char('!')) {
            if (not tz.is_next_char('=')) {
                // the error is at the '!' that is alone
                tz.put_back_char('!');

                throw compiler_exception{tz, "expected '!='"};
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

            return operand::imm(std::format("{}", *value),
                                tc.get_type_default());
        }

        const ident_info expr_info{tc.make_scalar_ident_info(expr)};

        if (expr.is_indexed() or tc.has_lea(expr)) {
            return expr.compile_lea(tc, indent, expr.tok(), allocated_registers,
                                    {
                                        .reg_count{},
                                        .lea_path{expr_info.lea_path},
                                        .address_register{},
                                    });
        }

        // a constant left side was mirrored onto the right
        assert(not expr_info.is_const() or not is_lhs);

        if (expr_info.is_const()) {
            return expr.make_constant_operand(expr_info);
        }

        if (expr.get_unary_ops().is_empty()) {
            return expr_info.operand;
        }

        const operand reg{
            expr_arith::compile_unary_to_scratch(
                tc, indent, expr, expr_info.operand, expr_info.type_ref()),
        };

        allocated_registers.emplace_back(reg);

        return reg;
    }

    // a side computed at run time in a register of its own type, so a
    // constant list folds at that width
    [[nodiscard]] static auto side_constant(const toc& tc,
                                            const expr_arith& side)
        -> std::optional<int64_t> {

        if (side.is_expression()) {
            return side.folded_constant(tc, side.get_type());
        }

        const ident_info info{tc.make_ident_info(side)};

        if (not info.is_const()) {
            return std::nullopt;
        }

        return side.get_unary_ops().evaluate_constant(info.const_value);
    }

    // the value compared with 0, matching types avoid narrowing before the
    // in-place truth test
    [[nodiscard]] static auto
    truth_test_operand(toc& tc, const size_t indent, const expr_arith& lhs,
                       const machine::comparison_action& action,
                       std::vector<operand>& allocated_registers) -> operand {

        if (lhs.is_expression() and action.destination.is_register() and
            lhs.get_type().is_same(action.destination.type_ref())) {

            lhs.compile(tc, indent + 1,
                        toc::make_ident_info_from_register(action.destination));

            return action.destination;
        }

        return resolve_expr(tc, indent, lhs, true, allocated_registers);
    }
};

// list of boolean expressions / lists instead of tree
// note: quirky parsing and compiling but supports short-circuiting
class expr_bool final : public statement {
    using element = std::variant<expr_bool_op, expr_bool>;

    std::vector<element> bools_;
    std::vector<token> ops_; // 'and' or 'or' ops between element in 'bools_'
    token not_tk_;           // e.g. not (a==b and c==d)
    token open_paren_tk_;
    token close_paren_tk_;
    bool enclosed_{}; // e.g. (a==b and c==d) vs a==b and c==d

  public:
    expr_bool(toc& tc, const token tk, tokenizer& tz,
              const bool enclosed = false, const token not_tk = {},
              const token open_paren_tk = {},
              std::unique_ptr<statement> first_expression = {})
        : statement{tk}, not_tk_{not_tk}, open_paren_tk_{open_paren_tk},
          enclosed_{enclosed} {

        set_type(tc.get_type_bool());

        // a caller might have supplied the first operand it already parsed
        // e.g. the 'bool' of 'var b = bool'
        parse_operand(tc, tz, std::move(first_expression));

        // the operands are joined by 'and' or by 'or', not by both
        // e.g. 'a == 1 and b == 2 and c == 3'
        while (const std::optional<token> op_tk{read_connective(tz)}) {
            assert_same_connective(*op_tk);

            ops_.emplace_back(*op_tk);

            parse_operand(tc, tz, {});
        }
    }

    expr_bool() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        not_tk_.source_to(os);

        if (enclosed_) {
            open_paren_tk_.source_to(os);
        }

        const size_t n{bools_.size()};
        for (size_t i{}; i < n; ++i) {
            bools_.at(i).visit(
                [&os](const auto& e) -> void { e.source_to(os); });

            if (i < n - 1) {
                // note: -1 because there is one operator fewer than elements

                ops_.at(i).source_to(os);
            }
        }

        if (enclosed_) {
            close_paren_tk_.source_to(os);
        }
    }

    [[noreturn]] auto compile([[maybe_unused]] toc& tc,
                              [[maybe_unused]] const size_t indent,
                              [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        std::unreachable();
    }

    [[nodiscard]] auto compile(toc& tc, const size_t indent,
                               const std::string_view jmp_to_if_false,
                               const std::string_view jmp_to_if_true,
                               const operand& dst) const
        -> std::optional<bool> {

        return compile_rec(tc, indent, jmp_to_if_false, jmp_to_if_true, false,
                           dst);
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(bools_.size() == 1);

        return bools_.at(0).visit(
            [](const auto& e) -> std::string_view { return e.identifier(); });
    }

    // assumes callers only query this when expression status is relevant
    [[nodiscard]] auto is_expression() const -> bool override {
        // more than one bool in the list is an expression
        if (bools_.size() > 1) {
            return true;
        }

        // 'identifier' cannot carry the negation
        if (not_tk_.is_text("not")) {
            return true;
        }

        assert(not bools_.empty());

        // 1 expression in the list

        return bools_.at(0).visit(
            [](const auto& e) -> bool { return e.is_expression(); });
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

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
        if (bools_.size() != 1 or not_tk_.is_text("not")) {
            return nullptr;
        }

        return bools_.front().visit(
            [](const auto& e) -> const expr_arith* { return e.arithmetic(); });
    }

    // decided at compile time only by constants before any run-time element,
    // a later short-circuit still needs the earlier elements evaluated
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<bool> {

        const bool is_or{not ops_.empty() and ops_.front().is_text("or")};
        const bool invert{not_tk_.is_text("not")};

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

  private:
    // e.g. 'a or b and c' is rejected, '(a or b) and c' is not
    auto assert_same_connective(const token& op_tk) const -> void {
        if (ops_.empty() or ops_.front().is_text(op_tk.text())) {
            return;
        }

        throw compiler_exception{op_tk,
                                 "mixing 'and' and 'or' without parenthesis"};
    }

    // an element that does not decide the list continues at the next element
    [[nodiscard]] auto
    compile_inner_element(toc& tc, const size_t indent, const size_t expr_index,
                          const std::string_view jmp_to_if_false,
                          const std::string_view jmp_to_if_true,
                          const bool invert, const operand& dst) const
        -> std::optional<bool> {

        const bool is_or{is_effective_or(expr_index, invert)};

        if (std::holds_alternative<expr_bool>(bools_.at(expr_index))) {
            const expr_bool& nested_expr{
                std::get<expr_bool>(bools_.at(expr_index)),
            };

            const std::string next_label{
                create_cmp_label_from(tc, bools_.at(expr_index + 1)),
            };
            // note: +1 because the next element is the continuation

            // an 'or' continues when false and an 'and' continues when true
            const std::string_view jmp_false{
                is_or ? std::string_view{next_label} : jmp_to_if_false,
            };

            const std::string_view jmp_true{
                is_or ? jmp_to_if_true : std::string_view{next_label},
            };

            return nested_expr.compile_with_label(tc, indent, jmp_false,
                                                  jmp_true, invert, dst);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.at(expr_index))};

        if (is_or) {
            return expr.compile_or(tc, indent, jmp_to_if_true, invert, dst);
        }

        return expr.compile_and(tc, indent, jmp_to_if_false, invert, dst);
    }

    [[nodiscard]] auto compile_last_element(
        toc& tc, const size_t indent, const std::string_view jmp_to_if_false,
        const std::string_view jmp_to_if_true, const bool invert,
        const operand& dst, const bool has_runtime_element) const
        -> std::optional<bool> {

        if (std::holds_alternative<expr_bool>(bools_.back())) {
            const expr_bool& nested_expr{std::get<expr_bool>(bools_.back())};

            const std::optional<bool> const_eval{
                nested_expr.compile_with_label(tc, indent, jmp_to_if_false,
                                               jmp_to_if_true, invert, dst),
            };

            // the nested list already emitted its final branch
            if (not const_eval) {
                return std::nullopt;
            }

            return resolve_last_constant(tc, indent, *const_eval,
                                         jmp_to_if_true, invert,
                                         has_runtime_element);
        }

        const expr_bool_op& expr{std::get<expr_bool_op>(bools_.back())};

        const std::optional<bool> const_eval{
            expr.compile_and(tc, indent, jmp_to_if_false, invert, dst,
                             jmp_to_if_false != jmp_to_if_true),
        };

        if (const_eval) {
            return resolve_last_constant(tc, indent, *const_eval,
                                         jmp_to_if_true, invert,
                                         has_runtime_element);
        }

        machine& x{tc.machine()};

        // if not yet jumped to false, then jump to true
        x.branch(indent, jmp_to_if_true);

        return std::nullopt;
    }

    [[nodiscard]] auto compile_rec(toc& tc, const size_t indent,
                                   const std::string_view jmp_to_if_false,
                                   const std::string_view jmp_to_if_true,
                                   const bool inverted,
                                   const operand& dst) const
        -> std::optional<bool> {

        machine& x{tc.machine()};

        x.comment(tok(), indent,
                  statement::trimmed_source(*this, "?",
                                            inverted ? " inverted: " : " "));

        // invert, according to De Morgan's laws
        const bool invert{
            inverted ? not not_tk_.is_text("not") : not_tk_.is_text("not"),
        };

        bool has_runtime_element{};

        const size_t last_index{bools_.size() - 1};
        // note: -1 is the index of the last element

        for (size_t expr_index{}; expr_index < last_index; ++expr_index) {
            const std::optional<bool> const_eval{
                compile_inner_element(tc, indent, expr_index, jmp_to_if_false,
                                      jmp_to_if_true, invert, dst),
            };

            if (not const_eval) {
                has_runtime_element = true;
                continue;
            }

            if (is_short_circuit(*const_eval, expr_index, invert)) {
                return *const_eval;
            }
        }

        return compile_last_element(tc, indent, jmp_to_if_false, jmp_to_if_true,
                                    invert, dst, has_runtime_element);
    }

    // earlier elements jump to the label that begins a nested list
    [[nodiscard]] auto compile_with_label(
        toc& tc, const size_t indent, const std::string_view jmp_to_if_false,
        const std::string_view jmp_to_if_true, const bool inverted,
        const operand& dst) const -> std::optional<bool> {

        machine& x{tc.machine()};

        x.label(indent, create_cmp_bgn_label(tc));

        return compile_rec(tc, indent, jmp_to_if_false, jmp_to_if_true,
                           inverted, dst);
    }

    [[nodiscard]] auto create_cmp_bgn_label(const toc& tc) const
        -> std::string {

        return tc.create_unique_label(tok(), "cmp");
    }

    // inversion swaps 'and' and 'or' according to De Morgan's laws
    [[nodiscard]] auto is_effective_or(const size_t op_index,
                                       const bool invert) const -> bool {

        return ops_.at(op_index).is_text("or") != invert;
    }

    // a constant false ends an 'and' list and a constant true ends an 'or' list
    [[nodiscard]] auto is_short_circuit(const bool const_eval,
                                        const size_t op_index,
                                        const bool invert) const -> bool {

        return const_eval == is_effective_or(op_index, invert);
    }

    // a comparison or a parenthesized list, either may follow 'not'
    auto parse_element(toc& tc, tokenizer& tz) -> void {
        // a speculative parse may need to start over from here
        const token rewind_pos_tk{tz.cur_position_token()};

        // a token that is not 'not' is put back and becomes the whitespace
        token maybe_not_tk{tz.next_token()};

        if (not maybe_not_tk.is_text("not")) {
            tz.put_back_token(maybe_not_tk);
            maybe_not_tk = tz.next_whitespace_token();
        }

        const token pos_tk{tz.cur_position_token()};
        const token open_paren_tk{tz.is_next_char_token('(')};

        // 'expr_bool_op' parses the 'not' itself
        if (open_paren_tk.is_empty()) {
            tz.put_back_token(maybe_not_tk);
            bools_.emplace_back(std::in_place_type<expr_bool_op>, tc, tz);

            return;
        }

        // '(t1 + t2) > 3' parses as a list but is a comparison
        expr_bool nested{tc, pos_tk, tz, true, maybe_not_tk, open_paren_tk};

        // an operator after ')' means the parentheses belonged to an operand
        if (std::string_view{"<>=!+-*/%&|^"}.contains(
                tz.peek_char_after_whitespace())) {

            tz.rewind_to_position(rewind_pos_tk);
            bools_.emplace_back(std::in_place_type<expr_bool_op>, tc, tz);

            return;
        }

        bools_.emplace_back(std::move(nested));
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
        // the ')' ends an enclosed expression
        // e.g. '(a == 1 or b == 2)'
        if (enclosed_) {
            close_paren_tk_ = tz.is_next_char_token(')');

            if (not close_paren_tk_.is_empty()) {
                return std::nullopt;
            }
        }

        // read 'and' or 'or'
        // e.g. the 'and' of 'a == 1 and b == 2'
        const token op_tk{tz.next_token()};

        if (op_tk.is_text("or") or op_tk.is_text("and")) {
            return op_tk;
        }

        // anything else ends the expression, an enclosed one needs its ')'
        // first
        // e.g. the 'exit' of 'if a == 1 exit(1)', or of '(a == 1 exit(1)'
        // which lacks its ')'
        if (enclosed_) {
            throw compiler_exception{op_tk, "expected ')' to close expression"};
        }

        tz.put_back_token(op_tk);

        return std::nullopt;
    }

    // a constant last element decides the list only if all earlier elements
    // were constants or it short-circuits the list
    [[nodiscard]] auto resolve_last_constant(
        toc& tc, const size_t indent, const bool const_eval,
        const std::string_view jmp_to_if_true, const bool invert,
        const bool has_runtime_element) const -> std::optional<bool> {

        if (not has_runtime_element) {
            return const_eval;
        }

        if (is_short_circuit(const_eval, ops_.size() - 1, invert)) {
            return const_eval;
        }

        // a constant false has already branched to false
        if (not const_eval) {
            return std::nullopt;
        }

        machine& x{tc.machine()};

        // a constant true emits no branch of its own
        x.branch(indent, jmp_to_if_true);

        return std::nullopt;
    }

    //
    // statics
    //

    [[nodiscard]] static auto create_cmp_label_from(const toc& tc,
                                                    const element& var)
        -> std::string {

        return var.visit([&tc](const auto& e) -> std::string {
            return e.create_cmp_bgn_label(tc);
        });
    }
};
