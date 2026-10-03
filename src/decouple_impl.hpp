#pragma once
// out-of-line definitions that need classes whose headers include the
// declaring header:
//   - statement factories declared in 'decouple.hpp' create every statement
//     class, and every statement header includes 'decouple.hpp'
//   - 'expr_type' members use 'expr_any', 'stmt_identifier' and 'stmt_call',
//     whose headers include 'expr_type.hpp' through 'expr_any.hpp'
//   - 'unary_ops::compile' uses 'toc', which includes 'unary_ops.hpp' through
//     'statement.hpp'

#include <cassert>
#include <format>
#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_type.hpp"
#include "stmt_builtin_array_length.hpp"
#include "stmt_builtin_arrays_equal.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_builtin_equal.hpp"
#include "stmt_builtin_exit.hpp"
#include "stmt_builtin_foo.hpp"
#include "stmt_builtin_io.hpp"
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
// called from 'stmt_block'
auto create_stmt_constructor_call(toc& tc, tokenizer& tz, const token type_tk)
    -> std::unique_ptr<statement> {

    return std::make_unique<stmt_call>(tc, unary_ops{}, type_tk, tz);
}

// declared in 'decouple.hpp'
// called from 'stmt_block'
auto create_stmt_method_call(toc& tc, tokenizer& tz, stmt_identifier receiver)
    -> std::unique_ptr<statement> {

    return std::make_unique<stmt_call>(tc, unary_ops{}, std::move(receiver),
                                       tz);
}

// a type name hidden by a var or alias is an identifier instead
static auto is_type_name_followed_by(const toc& tc, const token& tk,
                                     tokenizer& tz, const char next) -> bool {

    return tc.has_type(tk.text()) and not tc.is_var_or_alias(tk.text()) and
           tz.peek_char_after_whitespace() == next;
}

// declared in 'decouple.hpp'
// e.g. 'i8[]{1, 2}' or 'point[]{{1, 2}}', a type name followed by '[' has no
// other meaning
auto is_array_literal(const toc& tc, const token& tk, tokenizer& tz) -> bool {
    return is_type_name_followed_by(tc, tk, tz, '[');
}

// declared in 'decouple.hpp'
// e.g. '[]{1, 2}' has the default type, no expression starts with '['
auto is_default_array_literal(tokenizer& tz) -> bool {
    return tz.peek_char_after_whitespace() == '[';
}

// declared in 'decouple.hpp'
// e.g. 'point.at(1, 2)', a type name followed by '.' has no other meaning
auto is_constructor_call(const toc& tc, const token& tk, tokenizer& tz)
    -> bool {

    return is_type_name_followed_by(tc, tk, tz, '.');
}

// declared in 'decouple.hpp'
// e.g. 'point{1, 2}', a record type name followed by '{' has no other meaning
auto is_record_literal(const toc& tc, const token& tk, tokenizer& tz) -> bool {
    if (not is_type_name_followed_by(tc, tk, tz, '{')) {
        return false;
    }

    return not tc.get_type_or_throw(tk, tk.text()).is_builtin();
}

// declared in 'decouple.hpp'
// called from 'expr_arith' to solve circular dependencies with function
// calls
auto create_statement_in_expr_arith(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement> {

    // note: no 'std::move' on 'tk' because it is trivially copyable
    unary_ops uops{tz};
    const token tk{tz.next_token()};
    // array destinations parse strings in 'expr_any'
    if (tk.is_string()) {
        throw compiler_exception{
            tk, "a string can only be assigned to an array of type 'i8'"};
    }
    if (tk.text().empty()) {
        throw compiler_exception{
            tk, "expected constant, identifier, or function call"};
    }
    if (tk.is_text("read") or tk.is_text("write")) {
        return std::make_unique<stmt_builtin_io>(tc, std::move(uops), tk, tz);
    }
    if (tk.is_text("array_length")) {
        return std::make_unique<stmt_builtin_array_length>(tc, std::move(uops),
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
    if (stmt_builtin_convert::is_builtin_name(tk.text()) and
        tz.peek_char_after_whitespace() == '(') {

        return std::make_unique<stmt_builtin_convert>(tc, std::move(uops), tk,
                                                      tz);
    }

    if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
        // e.g.  foo(...)
        return std::make_unique<stmt_call>(tc, std::move(uops), tk, t, tz);
    }

    if (is_constructor_call(tc, tk, tz)) {
        return std::make_unique<stmt_call>(tc, std::move(uops), tk, tz);
    }

    // e.g. 0x80, rax, identifiers, constants
    stmt_identifier si{tc, {}, tk, tz};

    // e.g. '-lst.size()' negates the result, not the receiver
    if (si.is_method_receiver()) {
        return std::make_unique<stmt_call>(tc, std::move(uops), std::move(si),
                                           tz);
    }

    si.set_unary_ops(std::move(uops));

    return std::make_unique<stmt_identifier>(std::move(si));
}

// declared in 'expr_type.hpp'
// note: constructor is implemented here (rather than in the header) because
//       it needs the 'expr_any' definition, which would otherwise create a
//       circular include between 'expr_type.hpp' and 'expr_any.hpp'
expr_type::expr_type(toc& tc, tokenizer& tz, const type& tp,
                     const bool is_array_destination)
    : statement{tz.next_token()}, is_array_destination_{is_array_destination} {

    set_type(tp);

    // e.g. 'point{x, y}' names the type that '{x, y}' takes from the
    // destination
    const bool is_typed_literal{is_record_literal(tc, tok(), tz)};
    if (is_typed_literal and not tok().is_text(tp.name())) {
        throw compiler_exception{tok(),
                                 std::format("expected type '{}', got '{}'",
                                             tp.name(), tok().text())};
    }

    // e.g. 'p = pt', the token is empty at '{x, y}'
    if (not tok().text().empty() and not is_typed_literal) {
        parse_copy_source(tc, tz, tp);
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
        const type_field& tf{flds.at(counter)};
        if (counter++) {
            const token t{tz.is_next_char_token(',')};
            if (t.is_empty()) {
                throw compiler_exception{
                    tz, std::format(
                            "expected ',' followed by a value for field '{}' "
                            "in type '{}'",
                            flds.at(counter - 1).name, tp.name())};
            }
            expr_delims_tk_.emplace_back(t);
        }
        // create an expression that assigns to field
        // might recurse creating 'expr_type'
        exprs_.emplace_back(std::make_unique<expr_any>(
            tc, tz, tf.type(), true, tf.is_array, tf.array_count));
    }
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
expr_type::expr_type(std::shared_ptr<stmt_identifier> receiver)
    : statement{receiver->first_token()}, stmt_ident_{std::move(receiver)} {

    set_type(stmt_ident_->get_type());
}

// declared in 'expr_type.hpp'
// e.g. 'obj.pos = p', 'obj.pos = f()', 'o.pos = lst.first()' or
// 'o.pos = point.at(1, 2)'
auto expr_type::parse_copy_source(toc& tc, tokenizer& tz, const type& tp)
    -> void {

    if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
        stmt_call_ = std::make_shared<stmt_call>(tc, unary_ops{}, tok(), t, tz);

        assert_call_type(tp);

        return;
    }

    if (is_constructor_call(tc, tok(), tz)) {
        stmt_call_ = std::make_shared<stmt_call>(tc, unary_ops{}, tok(), tz);

        assert_call_type(tp);

        return;
    }

    stmt_identifier si{tc, unary_ops{}, tok(), tz};

    if (si.is_method_receiver()) {
        stmt_call_ =
            std::make_shared<stmt_call>(tc, unary_ops{}, std::move(si), tz);

        assert_call_type(tp);

        return;
    }

    stmt_ident_ = std::make_shared<stmt_identifier>(std::move(si));

    const ident_info src_info{tc.make_ident_info(*stmt_ident_)};

    if (tp.name() != src_info.type_ref().name()) {
        throw compiler_exception{
            tok(), std::format("expected type '{}', got '{}'", tp.name(),
                               src_info.type_ref().name())};
    }
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::assert_call_type(const type& tp) const -> void {
    if (tp.name() == stmt_call_->get_type().name()) {
        return;
    }

    throw compiler_exception{
        tok(), std::format("expected return type '{}', got '{}'", tp.name(),
                           stmt_call_->get_type().name())};
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::source_to(std::ostream& os) const -> void {
    if (stmt_call_) {
        stmt_call_->source_to(os);
        return;
    }

    // identifier case: base statement already emitted token text
    if (stmt_ident_) {
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

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::is_array_element() const -> bool {
    return stmt_ident_ and stmt_ident_->is_array_element();
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::compile(toc& tc, const size_t indent,
                        const ident_info& dst_info) const -> void {

    if (stmt_call_) {
        zero_padding(tc, indent, dst_info.type_ref(), dst_info.operand);
        stmt_call_->compile(tc, indent, dst_info);
        return;
    }

    const type& tp{dst_info.type_ref()};
    operand op{dst_info.operand};
    compile_assign(tc, indent, tp, dst_info, op);
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::assert_items_not_reading(const record_destination& dst,
                                         const size_t record_offset) const
    -> void {

    for (const auto [expr, field] :
         std::views::zip(exprs_, get_type().fields())) {

        const size_t field_offset{record_offset + field.offset};

        if (not field.type().is_builtin() and not expr->is_array_identifier()) {
            assert_record_field_not_reading(*expr, field, dst, field_offset);
            continue;
        }

        // array items are written element by element
        const size_t written_size_bytes{
            field.is_array ? field_offset + field.size_bytes : field_offset,
        };

        assert_item_not_reading(*expr, dst, written_size_bytes);
    }
}

// declared in 'expr_type.hpp'
// each element of a record array field is an item of its own, a plain record
// field is the single element
auto expr_type::assert_record_field_not_reading(const expr_any& src,
                                                const type_field& field,
                                                const record_destination& dst,
                                                const size_t field_offset)
    -> void {

    const size_t element_size_bytes{field.type().size_bytes()};

    for (size_t i{}; i < src.element_count(); ++i) {
        const expr_type& element{src.as_expr_type(i)};
        const size_t element_offset{
            field_offset +
                multiply_storage_size(src.tok(), element_size_bytes, i),
        };

        if (element.is_identifier()) {
            assert_item_not_reading(element, dst, element_offset);
            continue;
        }

        element.assert_items_not_reading(dst, element_offset);
    }
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::compile_assign(toc& tc, const size_t indent,
                               const type& dst_type, const ident_info& dst_info,
                               operand& dst_op) const -> void {

    if (stmt_call_) {
        compile_call_field(tc, indent, dst_type, dst_info, dst_op);
        return;
    }

    if (is_identifier()) {
        compile_identifier_copy(tc, indent, dst_type, dst_op);
        return;
    }

    compile_field_list(tc, indent, dst_type, dst_info, dst_op);
}

// declared in 'expr_type.hpp'
// e.g. 'f()' in '{f(), 3}' writes the field in place
auto expr_type::compile_call_field(toc& tc, const size_t indent,
                                   const type& dst_type,
                                   const ident_info& dst_info,
                                   operand& dst_op) const -> void {

    ident_info call_dst_info{dst_info};
    call_dst_info.operand = operand::mem(dst_op, dst_type);
    call_dst_info.use_operand = true;

    zero_padding(tc, indent, dst_type, dst_op);
    stmt_call_->compile(tc, indent, call_dst_info);

    dst_op.increment_offset(address_offset(dst_type.size_bytes()));
}

// declared in 'expr_type.hpp'
// advances 'dst_op' past the field, a record field does it in the recursion
auto expr_type::compile_field(toc& tc, const size_t indent, const expr_any& src,
                              const type_field& field, ident_info& dst_info,
                              operand& dst_op) const -> void {

    if (not field.type().is_builtin()) {
        compile_record_field(tc, indent, src, field, dst_info, dst_op);
        return;
    }

    write_builtin_field(tc, indent, src, field, dst_info, dst_op);

    dst_op.increment_offset(address_offset(field.size_bytes));
}

// declared in 'expr_type.hpp'
// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
auto expr_type::compile_field_list(toc& tc, const size_t indent,
                                   const type& dst_type,
                                   const ident_info& dst_info,
                                   operand& dst_op) const -> void {

    ident_info cur_dst_info{dst_info};
    cur_dst_info.operand = dst_op;
    cur_dst_info.use_operand = true;

    // note: keeping a current 'dst_info' accurate by adjusting 'cur_dst_info'
    // is not necessary but it looks nicer

    const std::span<const type_field> flds{dst_type.fields()};

    machine& x{tc.machine()};

    // bytes of the record written so far, fields in order then padding
    size_t written_bytes{};

    for (const auto [expr, field] : std::views::zip(exprs_, flds)) {
        const size_t padding_bytes{field.offset - written_bytes};

        zero_unwritten(tc, indent, "padding", padding_bytes,
                       offset_alignment(written_bytes, dst_type.alignment()),
                       dst_op);

        cur_dst_info.increment_offset(address_offset(padding_bytes));
        written_bytes = field.offset + field.size_bytes;

        x.comment(expr->tok(), indent, "copy field '{}'", field.name);

        cur_dst_info.push(field.name, field.type_ptr, {});

        compile_field(tc, indent, *expr, field, cur_dst_info, dst_op);

        cur_dst_info.increment_offset(address_offset(field.size_bytes));
        cur_dst_info.pop();
    }

    // zero out the remaining fields and the padding after the written ones

    const size_t remaining_bytes{dst_type.size_bytes() - written_bytes};

    zero_unwritten(
        tc, indent,
        exprs_.size() == flds.size() ? "padding" : "remaining fields",
        remaining_bytes, offset_alignment(written_bytes, dst_type.alignment()),
        dst_op);
}

// declared in 'expr_type.hpp'
// e.g. 'pt1 = pt2'
auto expr_type::compile_identifier_copy(toc& tc, const size_t indent,
                                        const type& dst_type,
                                        operand& dst_op) const -> void {

    const ident_info src_info{tc.make_ident_info(*this)};

    // 'expr_type' validates the source type before entering here

    assert(dst_type.name() == src_info.type_ref().name());

    // the copy size comes from the source, a whole array would overflow
    if (src_info.is_array and not is_array_destination_) {
        throw compiler_exception{tok(), "source must not be an array"};
    }

    if (is_array_destination_ and not src_info.is_array) {
        throw compiler_exception{tok(), "source must be an array"};
    }

    std::vector<operand> allocated_registers;
    const operand src_op{
        tc.get_lea_operand(indent, *this, src_info, allocated_registers),
    };

    const size_t size_bytes{
        multiply_storage_size(tok(), dst_type.size_bytes(),
                              src_info.is_array ? src_info.array_len : 1),
    };

    machine& x{tc.machine()};

    x.copy(tok(), indent, src_op, dst_op, size_bytes, dst_type.alignment());

    dst_op.increment_offset(address_offset(size_bytes));

    x.free_scratch_registers(tok(), indent, allocated_registers);
}

// declared in 'expr_type.hpp'
// the caller advances 'dst_op' past the field
auto expr_type::write_builtin_field(toc& tc, const size_t indent,
                                    const expr_any& src,
                                    const type_field& field,
                                    ident_info& dst_info,
                                    const operand& dst_op) const -> void {

    machine& x{tc.machine()};

    // e.g. 'msg' in 'type msgpoint { msg i8[128], pt point }' given '{}'
    if (src.is_array() and src.is_empty()) {
        x.comment(src.tok(), indent, "zero empty field: {} * {} B = {} B",
                  field.array_count, field.type().size_bytes(),
                  field.size_bytes);

        x.zero(tok(), indent, dst_op, field.size_bytes,
               field.type().alignment());

        return;
    }

    if (field.is_array and src.is_array_identifier()) {
        validate_array_assignment(src.tok(), field, tc.make_ident_info(src));
    }

    const operand dst_operand{operand::mem(dst_op, field.type())};

    // the paths below that copy directly skip the check in 'expr_any'
    src.assert_not_narrowed(tc, field.type());

    if (src.is_expression() or (src.is_identifier() and tc.has_lea(src))) {
        dst_info.operand = dst_operand;
        src.compile(tc, indent, dst_info);

        return;
    }

    // an array field is parsed as an array 'expr_any' which is an expression
    assert(not field.is_array);

    const ident_info src_info{tc.make_scalar_ident_info(src)};

    if (src_info.is_const()) {
        x.copy_value(src.tok(), indent, dst_operand,
                     src.make_constant_operand(src_info));

        return;
    }

    compile_builtin_field(tc, indent, src, src_info.operand, dst_operand);
}

// declared in 'expr_type.hpp'
// unary ops on a memory field are a load, modify and store each on a
// load/store machine, a scratch register can be shorter
auto expr_type::compile_builtin_field(toc& tc, const size_t indent,
                                      const expr_any& src,
                                      const operand& src_op, const operand& dst)
    -> void {

    machine& x{tc.machine()};

    const unary_ops& uops{src.get_unary_ops()};

    if (uops.is_empty()) {
        x.copy_value(src.tok(), indent, dst, src_op);
        return;
    }

    const auto compile_in_field{
        [&] -> void {
            x.copy_value(src.tok(), indent, dst, src_op);
            uops.compile(tc, indent, dst);
        },
    };

    x.emit_most_efficient(src.tok(), indent, compile_in_field, [&] -> void {
        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, dst.type_ref()),
        };

        x.copy_value(src.tok(), indent, reg, src_op);
        uops.compile(tc, indent, reg);
        x.copy_value(src.tok(), indent, dst, reg);
        x.free_scratch_register(src.tok(), indent, reg);
    });
}

// declared in 'expr_type.hpp'
// a record field, or each element of a record array field in turn
auto expr_type::compile_record_field(toc& tc, const size_t indent,
                                     const expr_any& src,
                                     const type_field& field,
                                     const ident_info& dst_info,
                                     operand& dst_op) -> void {

    // the copy size comes from the source so it must match the field
    if (field.is_array and src.is_array_identifier()) {
        validate_array_assignment(src.tok(), field, tc.make_ident_info(src));
    }

    // one assignment fills a record or copies a whole array identifier
    if (not field.is_array or src.is_array_identifier()) {
        src.as_expr_type().compile_assign(tc, indent, field.type(), dst_info,
                                          dst_op);

        return;
    }

    for (size_t i{}; i < src.element_count(); ++i) {
        src.as_expr_type(i).compile_assign(tc, indent, field.type(), dst_info,
                                           dst_op);
    }

    const size_t zeroed_size_bytes{
        zero_remaining_elements(tc, indent, src.tok(), dst_op, field.type(),
                                field.array_count - src.element_count()),
    };

    dst_op.increment_offset(address_offset(zeroed_size_bytes));
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::visit_reads(const std::string_view var,
                            const read_visitor reader) const -> void {

    // a copy or a call reads its source instead of the '{...}' items
    if (stmt_ident_) {
        stmt_ident_->visit_reads(var, reader);
    }

    if (stmt_call_) {
        stmt_call_->visit_reads(var, reader);
    }

    for (const std::unique_ptr<expr_any>& e : exprs_) {
        e->visit_reads(var, reader);
    }
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::compile_lea(toc& tc, const size_t indent,
                            const token& src_loc_tk,
                            std::vector<operand>& allocated_registers,
                            const operand& reg_count,
                            const std::span<const operand> lea_path,
                            const operand& address_register) const -> operand {

    return stmt_ident_->compile_lea(tc, indent, src_loc_tk, allocated_registers,
                                    reg_count, lea_path, address_register);
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::identifier() const -> std::string_view {
    // note: only a copy is asked for its variable, a '{...}' value is not
    assert(stmt_ident_);

    return stmt_ident_->identifier();
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::accessed_range() const -> std::optional<field_coverage::range> {
    // note: only a copy has an accessed range
    assert(stmt_ident_);

    return stmt_ident_->accessed_range();
}

// declared in 'expr_type.hpp'
// solves circular reference: expr_type -> expr_any -> expr_type
auto expr_type::is_indexed() const -> bool {
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
