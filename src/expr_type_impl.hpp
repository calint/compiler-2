#pragma once

// the members of 'expr_type' that use 'expr_any', 'stmt_identifier' and
// 'stmt_call', whose headers include 'expr_type.hpp' through 'expr_any.hpp',
// see 'decouple.hpp'

#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "decouple_impl.hpp"
#include "expr_any.hpp"
#include "expr_type.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

inline expr_type::expr_type(toc& tc, tokenizer& tz, const type& tp,
                            const destination_kind kind,
                            const literal_typing typing)
    : statement{tz.next_token()},
      is_array_destination_{kind == destination_kind::whole_array} {

    const tokenizer::nesting_scope nesting{tz};

    set_type(tp);

    const bool is_bare{is_bare_record_type(tc, tok(), tz)};

    // e.g. 'point{x, y}' names the type that '{x, y}' takes from the
    // destination
    const bool is_typed_literal{is_record_literal(tc, tok(), tz) or is_bare};

    // an array is copied from an array, a value of its element type is not one
    if (is_array_destination_ and is_typed_literal) {
        throw compile_error{
            tok(), std::format("expected an array, got an instance of '{}'",
                               tok().text())};
    }

    // note: compared by type, a type parameter names the type of its argument
    if (is_typed_literal and
        not tc.get_type_or_throw(tok(), tok().text()).is_same(tp)) {

        throw compile_error{tok(), std::format("expected type '{}', got '{}'",
                                               tp.name(), tok().text())};
    }

    // e.g. 'point{1, 2}' for a field of type 'point' is '{1, 2}'
    if (is_typed_literal and typing == literal_typing::from_destination) {
        throw_known_type(tok(), tok().text(), is_bare ? "{}" : "{...}");
    }

    // e.g. 'var p = point', no fields are listed
    if (is_bare) {
        return;
    }

    // e.g. 'p = pt', the token is empty at '{x, y}'
    if (not tok().text().empty() and not is_typed_literal) {
        parse_copy_source(tc, tz, tp);
        return;
    }

    parse_fields(tc, tz, tp);
}

// the fields of 'tp' in braces, e.g. '{x, y}' of 'obj.pos = {x, y}'
inline auto expr_type::parse_fields(toc& tc, tokenizer& tz, const type& tp)
    -> void {

    open_brace_tk_ = tz.expect_char_token(
        '{',
        std::format("expected '{{' to begin a value of type '{}'", tp.name()));

    const std::span<const type_field> flds{tp.fields()};

    while (not at_closing_brace(tz)) {
        // each field adds one expression, e.g. the 'x' and the 'y' of '{x, y}'
        const size_t index{exprs_.size()};

        if (index == flds.size()) {
            // the error is at the field that is too much
            // e.g. the 'z' of '{x, y, z}' for a type with two fields
            std::ignore = tz.is_next_char_token(',');

            throw compile_error{
                tz, std::format("too many fields for type '{}'", tp.name())};
        }

        const type_field& tf{flds.at(index)};

        if (index > 0) {
            parse_field_delimiter(tz, tp, tf);
        }

        // the value is parsed as the type of the field, so a nested '{...}'
        // is another 'expr_type'
        const std::optional<size_t> array_count{
            tf.is_array() ? std::optional{tf.array_count} : std::nullopt,
        };

        exprs_.emplace_back(std::make_unique<expr_any>(
            tc, tz, tf.type(), literal_typing::from_destination, array_count));
    }
}

// the special members need the complete 'stmt_identifier' and 'stmt_call'
inline expr_type::expr_type() = default;
inline expr_type::expr_type(expr_type&&) noexcept = default;
inline auto expr_type::operator=(expr_type&&) noexcept -> expr_type& = default;
inline expr_type::~expr_type() = default;

inline expr_type::expr_type(std::unique_ptr<stmt_identifier> receiver)
    : statement{receiver->first_token()}, stmt_ident_{std::move(receiver)} {

    set_type(stmt_ident_->get_type());
}

// e.g. 'obj.pos = p', 'obj.pos = f()', 'o.pos = lst.first()' or
// 'o.pos = point.at(1, 2)'
inline auto expr_type::parse_copy_source(toc& tc, tokenizer& tz, const type& tp)
    -> void {

    // a built-in is not a function, its value is a scalar of a built-in type
    if (const type* const value_type{builtin_value_type(tc, tok(), tz)};
        value_type != nullptr) {

        if (is_array_destination_) {
            throw compile_error{tok(), "source must be an array"};
        }

        throw compile_error{tok(), std::format("expected type '{}', got '{}'",
                                               tp.name(), value_type->name())};
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tok(), unary_ops{}, &tp),
    };

    if (parsed.call) {
        stmt_call_ = std::move(parsed.call);

        assert_call_type(tp);

        return;
    }

    stmt_ident_ = std::move(parsed.identifier);

    const ident_info src_info{tc.make_ident_info(*stmt_ident_)};

    if (not tp.is_same(src_info.type_ref())) {
        throw compile_error{
            stmt_ident_->last_token(),
            std::format("expected type '{}', got '{}'", tp.name(),
                        src_info.type_ref().name()),
        };
    }
}

inline auto expr_type::builtin_value_type(const toc& tc, const token& tk,
                                          tokenizer& tz) -> const type* {

    if (tk.is_text("array_length")) {
        return &tc.builtins().default_type();
    }

    if (tk.is_text("arrays_equal")) {
        return &tc.builtins().boolean();
    }

    if (tc.builtins().is_integer_name(tk.text()) and
        tz.peek_char_after_whitespace() == '(') {

        return &stmt_builtin_convert::conversion_type(tc, tk);
    }

    return nullptr;
}

inline auto expr_type::assert_call_type(const type& tp) const -> void {
    // a function returns one value, not an array
    if (is_array_destination_) {
        throw compile_error{tok(),
                            "an array cannot be assigned the result of a call"};
    }

    if (tp.is_same(stmt_call_->get_type())) {
        return;
    }

    throw compile_error{tok(),
                        std::format("expected return type '{}', got '{}'",
                                    tp.name(), stmt_call_->get_type().name())};
}

inline auto expr_type::source_to(std::ostream& os) const -> void {
    if (stmt_call_) {
        stmt_call_->source_to(os);
        return;
    }

    // the token of the base statement is the first one of the identifier
    if (stmt_ident_) {
        stmt_ident_->source_to(os);
        return;
    }

    statement::source_to(os);

    open_brace_tk_.source_to(os);

    const auto exprs{
        exprs_ | std::views::transform([](const std::unique_ptr<expr_any>& e)
                                           -> const expr_any& { return *e; }),
    };

    source_delimited_to(os, exprs, expr_delims_tk_);

    close_brace_tk_.source_to(os);
}

inline auto expr_type::is_array_element() const -> bool {
    return stmt_ident_ and stmt_ident_->is_array_element();
}

inline auto expr_type::compile(toc& tc, const size_t indent,
                               const destination& dst) const -> void {

    if (stmt_call_) {
        zero_padding(tc, indent, dst.type_ref(), dst.operand());
        stmt_call_->compile(tc, indent, dst);
        return;
    }

    const type& tp{dst.type_ref()};
    operand op{dst.operand()};
    compile_assign(tc, indent, tp, dst, op);
}

inline auto
expr_type::assert_items_not_reading(const constructor_target& dst,
                                    const size_t record_offset) const -> void {

    for (const auto [expr, field] :
         std::views::zip(exprs_, get_type().fields())) {

        const size_t field_offset{record_offset + field.offset};

        if (not field.type().is_builtin() and not expr->is_array_identifier()) {
            assert_record_field_not_reading(*expr, field, dst, field_offset);
            continue;
        }

        // a copy of an array field reads the same bytes or other ones, e.g.
        // 'v = {v.a, 4}'
        if (not field.is_array() or expr->is_array_identifier()) {
            assert_item_not_reading(*expr, dst, field_offset);
            continue;
        }

        // braced elements are written one by one, each after it is read
        const size_t element_size_bytes{field.type().size_bytes()};

        for (size_t i{}; i < expr->element_count(); ++i) {
            const size_t element_offset{
                field_offset +
                    multiply_storage_size(expr->tok(), element_size_bytes, i),
            };

            assert_item_not_reading(expr->element(i), dst, element_offset);
        }
    }
}

// each element of an instance array field is an item of its own, a plain
// instance field is the single element
inline auto expr_type::assert_record_field_not_reading(
    const expr_any& src, const type_field& field, const constructor_target& dst,
    const size_t field_offset) -> void {

    const size_t element_size_bytes{field.type().size_bytes()};

    for (size_t i{}; i < src.element_count(); ++i) {
        const expr_type& element{src.as_expr_type(i)};

        const size_t element_offset{
            field_offset +
                multiply_storage_size(src.tok(), element_size_bytes, i),
        };

        if (element.is_name_or_literal()) {
            assert_item_not_reading(element, dst, element_offset);
            continue;
        }

        element.assert_items_not_reading(dst, element_offset);
    }
}

inline auto expr_type::compile_assign(toc& tc, const size_t indent,
                                      const type& dst_type,
                                      const destination& dst,
                                      operand& dst_op) const -> void {

    if (stmt_call_) {
        compile_call_field(tc, indent, dst_type, dst, dst_op);
        return;
    }

    if (is_name_or_literal()) {
        compile_identifier_copy(tc, indent, dst_type, dst_op);
        return;
    }

    compile_field_list(tc, indent, dst_type, dst, dst_op);
}

// e.g. 'f()' in '{f(), 3}' writes the field in place
inline auto expr_type::compile_call_field(toc& tc, const size_t indent,
                                          const type& dst_type,
                                          const destination& dst,
                                          operand& dst_op) const -> void {

    const destination call_dst_info{
        dst.at_address(operand::mem(dst_op, dst_type)),
    };

    zero_padding(tc, indent, dst_type, dst_op);
    stmt_call_->compile(tc, indent, call_dst_info);

    dst_op.increment_offset(address_offset(dst_type.size_bytes()));
}

// advances 'dst_op' past the field, an instance field does it in the recursion
inline auto expr_type::compile_field(toc& tc, const size_t indent,
                                     const expr_any& src,
                                     const type_field& field,
                                     const destination& dst,
                                     operand& dst_op) const -> void {

    if (not field.type().is_builtin()) {
        compile_record_field(tc, indent, src, field, dst, dst_op);
        return;
    }

    write_builtin_field(tc, indent, src, field, dst, dst_op);

    dst_op.increment_offset(address_offset(field.size_bytes));
}

// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
inline auto expr_type::compile_field_list(toc& tc, const size_t indent,
                                          const type& dst_type,
                                          const destination& dst,
                                          operand& dst_op) const -> void {

    // each field is written at 'dst_op', which advances past it
    const destination record{dst.at_address(dst_op)};

    const std::span<const type_field> flds{dst_type.fields()};

    machine& x{tc.machine()};

    // bytes of the instance written so far, fields in order then padding
    size_t written_bytes{};

    for (const auto [expr, field] : std::views::zip(exprs_, flds)) {
        const size_t padding_bytes{field.offset - written_bytes};

        zero_unwritten(tc, indent, "padding", padding_bytes,
                       offset_alignment(written_bytes, dst_type.alignment()),
                       dst_op);

        written_bytes = field.offset + field.size_bytes;

        x.comment(expr->tok(), indent, "copy field '{}'", field.name);

        compile_field(tc, indent, *expr, field,
                      record.field(field.name, field.type()), dst_op);
    }

    // zero out the remaining fields and the padding after the written ones
    const size_t remaining_bytes{dst_type.size_bytes() - written_bytes};

    zero_unwritten(
        tc, indent,
        exprs_.size() == flds.size() ? "padding" : "remaining fields",
        remaining_bytes, offset_alignment(written_bytes, dst_type.alignment()),
        dst_op);
}

// e.g. 'pt1 = pt2'
inline auto expr_type::compile_identifier_copy(toc& tc, const size_t indent,
                                               const type& dst_type,
                                               operand& dst_op) const -> void {

    const ident_info src_info{tc.make_ident_info(*this)};

    // 'expr_type' validates the source type before entering here
    assert(dst_type.is_same(src_info.type_ref()));

    // the copy size comes from the source, a whole array would overflow
    if (src_info.is_array() and not is_array_destination_) {
        throw compile_error{tok(), "source must not be an array"};
    }

    if (is_array_destination_ and not src_info.is_array()) {
        throw compile_error{tok(), "source must be an array"};
    }

    std::vector<operand> allocated_registers;

    const operand src_op{
        get_lea_operand(tc, indent, src_info, allocated_registers),
    };

    const size_t size_bytes{
        multiply_storage_size(tok(), dst_type.size_bytes(),
                              src_info.element_count()),
    };

    machine& x{tc.machine()};

    x.copy(tok(), indent, dst_op, src_op, size_bytes, dst_type.alignment());

    dst_op.increment_offset(address_offset(size_bytes));

    x.free_scratch_registers(tok(), indent, allocated_registers);
}

// the caller advances 'dst_op' past the field
inline auto expr_type::write_builtin_field(
    toc& tc, const size_t indent, const expr_any& src, const type_field& field,
    const destination& dst, const operand& dst_op) const -> void {

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

    if (field.is_array() and src.is_array_identifier()) {
        validate_array_assignment(src.tok(), field, tc.make_ident_info(src));
    }

    const operand dst_operand{operand::mem(dst_op, field.type())};

    // the paths below that copy directly skip the check in 'expr_any'
    src.assert_not_narrowed(tc, field.type());

    // an indexed name has the address of its base, 'compile' adds the index
    if (src.is_expression() or src.is_indexed() or
        (src.is_name_or_literal() and
         tc.scopes().has_computed_address(src.name_or_literal()))) {

        src.compile(tc, indent, dst.with_operand(dst_operand));
        return;
    }

    // an array field is parsed as an array 'expr_any' which is an expression
    assert(not field.is_array());

    const ident_info src_info{tc.make_scalar_ident_info(src)};

    if (src_info.is_const()) {
        x.copy_value(src.tok(), indent, dst_operand,
                     src.make_constant_operand(src_info));

        return;
    }

    compile_builtin_field(tc, indent, src, src_info.operand(), dst_operand);
}

// unary ops on a memory field are a load, modify and store each on a
// load/store machine, a scratch register can be shorter
inline auto expr_type::compile_builtin_field(toc& tc, const size_t indent,
                                             const expr_any& src,
                                             const operand& src_op,
                                             const operand& dst) -> void {

    machine& x{tc.machine()};

    const unary_ops& uops{src.get_unary_ops()};

    if (uops.is_empty()) {
        x.copy_value(src.tok(), indent, dst, src_op);
        return;
    }

    const auto compile_in_field{
        [&] -> void {
            x.copy_value(src.tok(), indent, dst, src_op);

            uops.compile(x, indent, src.tok(), dst,
                         tc.checks().overflow_options());
        },
    };

    tc.emit_most_efficient(src.tok(), indent, compile_in_field, [&] -> void {
        const operand reg{
            x.alloc_scratch_register(src.tok(), indent, dst.type_ref()),
        };

        x.copy_value(src.tok(), indent, reg, src_op);
        uops.compile(x, indent, src.tok(), reg, tc.checks().overflow_options());
        x.copy_value(src.tok(), indent, dst, reg);
        x.free_scratch_register(src.tok(), indent, reg);
    });
}

// an instance field, or each element of an instance array field in turn
inline auto expr_type::compile_record_field(toc& tc, const size_t indent,
                                            const expr_any& src,
                                            const type_field& field,
                                            const destination& dst,
                                            operand& dst_op) -> void {

    // the copy size comes from the source so it must match the field
    if (field.is_array() and src.is_array_identifier()) {
        validate_array_assignment(src.tok(), field, tc.make_ident_info(src));
    }

    // one assignment fills an instance or copies a whole array identifier
    if (not field.is_array() or src.is_array_identifier()) {
        src.as_expr_type().compile_assign(tc, indent, field.type(), dst,
                                          dst_op);

        return;
    }

    for (size_t i{}; i < src.element_count(); ++i) {
        src.as_expr_type(i).compile_assign(tc, indent, field.type(), dst,
                                           dst_op);
    }

    const size_t zeroed_size_bytes{
        zero_remaining_elements(tc, indent, src.tok(), dst_op, field.type(),
                                field.array_count - src.element_count()),
    };

    dst_op.increment_offset(address_offset(zeroed_size_bytes));
}

inline auto expr_type::visit_reads(const read_filter var,
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

inline auto expr_type::compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
    -> operand {

    return stmt_ident_->compile_lea(tc, indent, src_loc_tk, allocated_registers,
                                    request);
}

inline auto expr_type::name_or_literal() const -> std::string_view {
    // note: only a copy is asked for its variable, a '{...}' value is not
    assert(stmt_ident_);

    return stmt_ident_->name_or_literal();
}

inline auto expr_type::accessed_path() const -> access_path {
    // note: only a copy has an accessed path
    assert(stmt_ident_);

    return stmt_ident_->accessed_path();
}

inline auto expr_type::is_indexed() const -> bool {
    return stmt_ident_ and stmt_ident_->is_indexed();
}
