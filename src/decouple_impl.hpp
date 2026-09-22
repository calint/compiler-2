#pragma once
// out-of-line definitions for functions/methods declared in 'decouple.hpp',
// 'expr_type_value.hpp' and 'unary_ops.hpp' to break circular includes
// between 'expr_any', 'expr_type_value', 'stmt_block', 'stmt_call',
// 'unary_ops' and 'toc'.

#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_type_value.hpp"
#include "stmt_builtin_address_of.hpp"
#include "stmt_builtin_array_size_of.hpp"
#include "stmt_builtin_arrays_equal.hpp"
#include "stmt_builtin_equal.hpp"
#include "stmt_builtin_exit.hpp"
#include "stmt_builtin_foo.hpp"
#include "stmt_builtin_io.hpp"
#include "stmt_builtin_mov.hpp"
#include "stmt_builtin_syscall.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "unary_ops.hpp"

// definitions are intentionally not 'inline': single translation unit build
// NOLINTBEGIN(misc-definitions-in-headers)

// declared in 'decouple.hpp'
auto operand::imm(std::string value, const type& value_type) -> operand {
    if (value.empty()) {
        throw std::invalid_argument{"operand text must not be empty"};
    }

    operand result;
    result.kind_ = kind::immediate;
    result.type_ptr_ = &value_type;
    result.immediate_ = std::move(value);

    return result;
}

// declared in 'decouple.hpp'
auto operand::reg(const std::string_view name, const type& value_type)
    -> operand {

    if (name.empty()) {
        throw std::invalid_argument{"operand text must not be empty"};
    }

    operand result;
    result.kind_ = kind::reg;
    result.type_ptr_ = &value_type;
    result.base_register_ = name;

    return result;
}

// declared in 'decouple.hpp'
auto operand::mem(const std::string_view base, const std::string_view index,
                  const uint8_t index_scale, const int64_t offset,
                  const type& value_type) -> operand {

    if (base.empty() and index.empty() and offset == 0) {
        throw std::invalid_argument{"operand address must not be empty"};
    }

    operand result;
    result.kind_ = kind::memory;
    result.type_ptr_ = &value_type;
    result.base_register_ = base;
    result.index_register_ = index;
    result.scale_ = index_scale;
    result.displacement_ = offset;

    return result;
}

// declared in 'decouple.hpp'
// called from 'stmt_block' to solve circular dependencies with 'loop',
// 'if', 'mov', 'syscall'
auto create_statement_in_stmt_block(toc& tc, tokenizer& tz, const token tk)
    -> std::unique_ptr<statement> {

    // note: no 'std:move' on 'tk' because it is trivially copyable
    if (tk.is_text("loop")) {
        return std::make_unique<stmt_loop>(tc, tk, tz);
    }
    if (tk.is_text("if")) {
        return std::make_unique<stmt_if>(tc, tk, tz);
    }
    if (tk.is_text("mov")) {
        return std::make_unique<stmt_builtin_mov>(tc, tk, tz);
    }
    if (tk.is_text("syscall")) {
        return std::make_unique<stmt_builtin_syscall>(tc, tk, tz);
    }
    if (tk.is_text("exit")) {
        return std::make_unique<stmt_builtin_exit>(tc, tk, tz);
    }
    if (tk.is_text("read") or tk.is_text("write")) {
        return std::make_unique<stmt_builtin_io>(tc, unary_ops{}, tk, tz);
    }
    if (tk.is_text("foo")) {
        return std::make_unique<stmt_builtin_foo>(tc, tk, tz);
    }

    std::unreachable();
}

// declared in 'decouple.hpp'
// called from 'stmt_block'
auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si,
                      const token open_paren_tk) -> std::unique_ptr<statement> {

    return std::make_unique<stmt_call>(tc, si.get_unary_ops(), si.first_token(),
                                       open_paren_tk, tz);
}

// declared in 'decouple.hpp'
// called from 'expr_ops_list' to solve circular dependencies with function
// calls
auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement> {

    // note: no 'std::move' on 'tk' because it is trivially copyable
    unary_ops uops{tz};
    const token tk{tz.next_token()};
    if (tk.text().empty()) {
        throw compiler_exception{
            tk, "expected constant, identifier, or function call"};
    }
    if (tk.text().starts_with("#")) {
        throw compiler_exception{tk, "unexpected comment in expression"};
    }
    if (tk.is_text("address_of")) {
        return std::make_unique<stmt_builtin_address_of>(tc, std::move(uops),
                                                         tk, tz);
    }
    if (tk.is_text("read") or tk.is_text("write")) {
        return std::make_unique<stmt_builtin_io>(tc, std::move(uops), tk, tz);
    }
    if (tk.is_text("array_size_of")) {
        return std::make_unique<stmt_builtin_array_size_of>(tc, std::move(uops),
                                                            tk, tz);
    }
    if (tk.is_text("arrays_equal")) {
        return std::make_unique<stmt_builtin_arrays_equal>(tc, std::move(uops),
                                                           tk, tz);
    }
    if (tk.is_text("equal")) {
        return std::make_unique<stmt_builtin_equal>(tc, std::move(uops), tk,
                                                    tz);
    }

    if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
        // e.g.  foo(...)
        return std::make_unique<stmt_call>(tc, std::move(uops), tk, t, tz);
    }

    // e.g. 0x80, rax, identifiers, constants
    return std::make_unique<stmt_identifier>(tc, std::move(uops), tk, tz);
}

// declared in 'expr_type_value.hpp'
// note: constructor is implemented here (rather than in the header) because
//       it needs the 'expr_any' definition, which would otherwise create a
//       circular include between 'expr_type_value.hpp' and 'expr_any.hpp'
expr_type_value::expr_type_value(toc& tc, tokenizer& tz, const type& tp)
    : statement{tz.next_token()} {

    set_type(tp);

    // is it an identifier or a function call?
    // note: token name would be empty at the '{x, y}' type of statement

    if (not tok().text().empty()) {
        // yes, e.g. obj.pos = p

        if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
            stmt_call_ =
                std::make_shared<stmt_call>(tc, unary_ops{}, tok(), t, tz);

            if (tp.name() != stmt_call_->get_type().name()) {
                throw compiler_exception{
                    tok(),
                    std::format("expected return type '{}', got '{}'",
                                tp.name(), stmt_call_->get_type().name())};
            }

            return;
        }

        stmt_ident_ =
            std::make_shared<stmt_identifier>(tc, unary_ops{}, tok(), tz);

        // check that an identifier type matches the expected type
        const ident_info src_info{tc.make_ident_info(*stmt_ident_)};

        if (tp.name() != src_info.type_ref().name()) {
            throw compiler_exception{
                tok(), std::format("expected type '{}', got '{}'", tp.name(),
                                   src_info.type_ref().name())};
        }

        return;
    }
    // e.g. obj.pos = {x, y}
    open_brace_tk_ = tz.is_next_char_token('{');
    if (open_brace_tk_.is_empty()) {
        throw compiler_exception{
            tz, std::format("expected '{{' to begin a value of type '{}'",
                            tp.name())};
    }

    const std::span<const type_field> flds{tp.fields()};
    const size_t field_count{flds.size()};
    size_t counter{};
    while (true) {
        close_brace_tk_ = tz.is_next_char_token('}');
        if (not close_brace_tk_.is_empty()) {
            break;
        }

        if (counter == field_count) {
            throw compiler_exception{
                tz, std::format("too many fields specified for type '{}'",
                                tp.name())};
        }
        const type_field& tf{flds[counter]};
        if (counter++) {
            const token t{tz.is_next_char_token(',')};
            if (t.is_empty()) {
                throw compiler_exception{
                    tz, std::format(
                            "expected ',' followed by a value for field '{}' "
                            "in type '{}'",
                            flds[counter - 1].name, tp.name())};
            }
            expr_delims_tk_.emplace_back(t);
        }
        // create an expression that assigns to field
        // might recurse creating 'expr_type_value'
        exprs_.emplace_back(std::make_unique<expr_any>(
            tc, tz, tf.type(), true, tf.is_array, tf.array_count));
    }
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::source_to(std::ostream& os) const -> void {
    if (stmt_call_) {
        stmt_call_->source_to(os);

        return;
    }

    // identifier case: base statement already emitted token text
    if (is_make_copy()) {
        stmt_ident_->source_to(os);

        return;
    }

    statement::source_to(os);

    // not an identifier
    open_brace_tk_.source_to(os);
    if (not exprs_.empty()) {
        exprs_.front()->source_to(os);
        for (const auto [d, e] :
             std::views::zip(expr_delims_tk_, exprs_ | std::views::drop(1))) {

            d.source_to(os);
            e->source_to(os);
        }
    }
    close_brace_tk_.source_to(os);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::is_array_element() const -> bool {
    return stmt_ident_ and stmt_ident_->is_array_element();
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::compile(toc& tc, const size_t indent,
                              const ident_info& dst_info) const -> void {

    if (stmt_call_) {
        stmt_call_->compile(tc, indent, dst_info);

        return;
    }

    const type& tp{dst_info.type_ref()};
    operand op{dst_info.operand};
    compile_assign(tc, indent, tp, dst_info, op);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::compile_assign(toc& tc, const size_t indent,
                                     const type& dst_type,
                                     const ident_info& dst_info,
                                     operand& dst_op) const -> void {

    // is it e.g. pt1 = pt2, or pt1 = f()?
    if (is_identifier()) {
        const ident_info src_info{tc.make_ident_info(*this)};

        // 'expr_type_value' validates the source type before entering here

        assert(dst_type.name() == src_info.type_ref().name());

        std::vector<operand> allocated_registers;
        const operand src_op{
            tc.get_lea_operand(indent, *this, src_info, allocated_registers)};

        const size_t size_bytes{multiply_storage_size(
            dst_type.size_bytes(), src_info.is_array ? src_info.array_len : 1)};

        machine& x{tc.machine()};

        x.copy(tok(), indent, src_op, dst_op, size_bytes);

        dst_op.increment_offset(address_offset(size_bytes));

        x.free_scratch_registers(tok(), indent, allocated_registers);

        return;
    }

    // initialize fields

    ident_info cur_dst_info{dst_info};
    cur_dst_info.operand = dst_op;
    cur_dst_info.use_operand = true;

    // note: keeping a current 'dst_info' accurate by adjusting 'cur_dst_info'
    // is not necessary but it looks nicer

    const std::span<const type_field> flds{dst_type.fields()};

    machine& x{tc.machine()};

    for (const auto [expr, field] : std::views::zip(exprs_, flds)) {
        x.comment(expr->tok(), indent, "copy field '{}'", field.name);

        cur_dst_info.push(field.name, field.type_ptr, {});

        if (not field.type().is_builtin()) {
            // the field has a user-defined type, so the expression is
            // 'expr_type_value'
            const expr_type_value& type_value{expr->as_expr_type_value()};
            type_value.compile_assign(tc, indent, field.type(), cur_dst_info,
                                      dst_op);
            // note: dst_op was mutated in the recursive call
            cur_dst_info.increment_offset(address_offset(field.size_bytes));
            cur_dst_info.pop();
            continue;
        }

        // built-in

        const expr_any& src{*expr};

        if (src.is_array() and src.is_empty()) {
            // special case when empty array
            // e.g.:
            //   type msgpoint {  msg : i8[128], pt : point }
            //   var mp : msgpoint[3] = { { {}, { x, y } } }
            x.comment(expr->tok(), indent, "zero empty field: {} * {} B = {} B",
                      field.array_count, field.type().size_bytes(),
                      field.size_bytes);

            x.zero(tok(), indent, dst_op, field.size_bytes);
            const int64_t size_bytes{address_offset(field.size_bytes)};
            dst_op.increment_offset(size_bytes);
            cur_dst_info.increment_offset(size_bytes);
            cur_dst_info.pop();
            continue;
        }

        if (field.is_array and src.is_array_identifier()) {
            validate_array_assignment(src.tok(), field,
                                      tc.make_ident_info(src));
        }

        const operand dst_operand{operand::mem(dst_op, field.type())};

        if (src.is_expression() or (src.is_identifier() and tc.has_lea(src))) {
            // built-in, expression
            cur_dst_info.operand = dst_operand;
            src.compile(tc, indent, cur_dst_info);
        } else {
            // built-in, not expression
            const ident_info src_info{tc.make_ident_info(src)};
            if (src_info.is_const()) {
                // built-in, not expression, constant
                x.copy_value(src.tok(), indent, dst_operand,
                             src.make_constant_operand(src_info));
            } else {
                // built-in, not expression, not constant
                if (field.is_array) {
                    // built-in, not expression, not constant, array

                    // note: never reached because when 'src' is an array,
                    //       'expr_any::is_expression()' returns true and takes
                    //       the expression path above

                    validate_array_assignment(src.tok(), field, src_info);
                    x.copy(src.tok(), indent, src_info.operand, dst_op,
                           field.size_bytes);
                } else {
                    // built-in, not expression, not constant, not array
                    x.copy_value(src.tok(), indent, dst_operand,
                                 src_info.operand);

                    src.get_unary_ops().compile(tc, indent, dst_operand);
                }
            }
        }
        const int64_t size_bytes{address_offset(field.size_bytes)};
        dst_op.increment_offset(size_bytes);
        cur_dst_info.increment_offset(size_bytes);
        cur_dst_info.pop();
    }

    // zero out the remaining fields

    if (exprs_.size() == flds.size()) {
        // all fields have been assigned
        return;
    }

    // calculate remaining bytes of the type to zero

    const size_t size_bytes{
        dst_type.remaining_fields_size_bytes(exprs_.size())};

    x.comment(tok(), indent, "zero remaining fields: {} B", size_bytes);
    x.zero(tok(), indent, dst_op, size_bytes);
    dst_op.increment_offset(address_offset(size_bytes));
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::validate_array_assignment(const token& src_loc_tk,
                                                const type_field& fld,
                                                const ident_info& src_info)
    -> void {

    if (not src_info.is_array) {
        throw compiler_exception{src_loc_tk, "source must be an array"};
    }

    // 'expr_any' validates the source element type before entering here

    assert(fld.type().name() == src_info.type_ref().name());

    if (fld.array_count != src_info.array_len) {
        throw compiler_exception{
            src_loc_tk, std::format("destination array size {} does not match "
                                    "source size {}",
                                    fld.array_count, src_info.array_len)};
    }
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::assert_var_not_used(const std::string_view var) const
    -> void {

    for (const std::unique_ptr<expr_any>& e : exprs_) {
        e->assert_var_not_used(var);
    }
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::compile_lea(toc& tc, const size_t indent,
                                  const token& src_loc_tk,
                                  std::vector<operand>& allocated_registers,
                                  const operand& reg_count,
                                  const std::span<const operand> lea_path) const
    -> operand {

    return stmt_ident_->compile_lea(tc, indent, src_loc_tk, allocated_registers,
                                    reg_count, lea_path);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::identifier() const -> std::string_view {
    if (stmt_ident_) {
        return stmt_ident_->identifier();
    }

    return statement::identifier();
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::is_indexed() const -> bool {
    return stmt_ident_ and stmt_ident_->is_indexed();
}

// declared in 'unary_ops.hpp'
// solves circular reference: unary_ops -> toc -> statement -> unary_ops
auto unary_ops::compile(toc& tc, const size_t indent,
                        const operand& dst_info) const -> void {

    machine& x{tc.machine()};

    for (const char o : ops_ | std::views::reverse) {
        x.unary(indent, o, dst_info);
    }
}

// NOLINTEND(misc-definitions-in-headers)
