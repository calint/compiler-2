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
#include <string_view>
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

    const toc::parse_nesting_scope nesting{tc, tz};

    set_type(tp);

    const bool is_bare{is_bare_record_type(tc, tok(), tz)};

    // e.g. 'point{x, y}' names the type that '{x, y}' takes from the
    // destination
    const bool is_typed_literal{is_record_literal(tc, tok(), tz) or is_bare};

    // an array is copied from an array, a value of its element type is not one
    if (is_array_destination_ and is_typed_literal) {
        throw compile_error{
            tok(), std::format("expected an array, got an instance of {}",
                               in_quotes(tok().text()))};
    }

    // note: compared by type, a type parameter names the type of its argument
    if (is_typed_literal and not tc.get_type_or_throw(tok()).is_same(tp)) {

        throw compile_error{tok(), std::format("expected type {}, got {}",
                                               in_quotes(tp.name()),
                                               in_quotes(tok().text()))};
    }

    // e.g. 'point{1, 2}' for a field of type 'point' is '{1, 2}'
    if (is_typed_literal and typing == literal_typing::from_destination) {
        throw_known_type(tok(), tok().text(), is_bare ? "{}" : "{...}");
    }

    // e.g. 'var p = point', no fields are listed
    if (is_bare) {
        // e.g. the '+' of 'point + 1', like 'p + 1' of a variable 'p'
        if (continues_expression(tz)) {
            throw compile_error{tok(),
                                std::format("an instance of {} is not a number",
                                            in_quotes(tok().text()))};
        }

        return;
    }

    // e.g. 'p = pt', the token is empty at '{x, y}'
    if (not tok().is_none() and not is_typed_literal) {
        parse_copy_source(tc, tz, tp);
        return;
    }

    // e.g. the '(' of 'q = (p)', parentheses group numbers
    if (tz.peek_char() == '(') {
        throw compile_error{
            tz, std::format("a value in parentheses is a number, not {} of "
                            "{}",
                            is_array_destination_ ? "an array" : "an instance",
                            in_quotes(tp.name()))};
    }

    // e.g. the '-' of 'q = -p'
    if (tz.peek_char() == '-' or tz.peek_char() == '~') {
        throw compile_error{
            tz, std::format("{} of {} is not a number",
                            is_array_destination_ ? "an array" : "an instance",
                            in_quotes(tp.name()))};
    }

    parse_fields(tc, tz, tp);
}

// the fields of 'tp' in braces, e.g. '{x, y}' of 'obj.pos = {x, y}'
inline auto expr_type::parse_fields(toc& tc, tokenizer& tz, const type& tp)
    -> void {

    open_brace_tk_ = tz.expect_char_token(
        '{', std::format("expected '{{' to begin a value of type {}",
                         in_quotes(tp.name())));

    const std::span<const type_field> flds{tp.fields()};

    while (not at_closing_brace(tz)) {
        // each field adds one expression, e.g. the 'x' and the 'y' of '{x, y}'
        const size_t index{exprs_.size()};

        if (index == flds.size()) {
            const token delimiter_tk{tz.is_next_char_token(',')};

            // a trailing ',' lets each field end its line the same way
            if (not delimiter_tk.is_empty() and at_closing_brace(tz)) {
                expr_delims_tk_.emplace_back(delimiter_tk);
                break;
            }

            // the error is at the field that is too much
            // e.g. the 'z' of '{x, y, z}' for a type with two fields
            throw compile_error{tz, std::format("too many fields for type {}",
                                                in_quotes(tp.name()))};
        }

        const type_field& tf{flds.at(index)};

        if (index > 0) {
            parse_field_delimiter(tz, tp, tf);

            // a trailing ',' lets each field end its line the same way
            if (at_closing_brace(tz)) {
                break;
            }
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

    assert_builtin_without_type_args(tok(), tz);

    assert_builtin_has_value(tok(), tz);

    // a built-in is not a function, its value is a scalar of a built-in type
    if (const type* const value_type{builtin_value_type(tc, tok(), tz)};
        value_type != nullptr) {

        if (is_array_destination_) {
            throw compile_error{tok(), "source must be an array"};
        }

        throw compile_error{tok(), std::format("expected type {}, got {}",
                                               in_quotes(tp.name()),
                                               in_quotes(value_type->name()))};
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

    const ident_info src_info{make_ident_info(tc, *stmt_ident_)};

    if (not tp.is_same(src_info.type_ref())) {
        throw compile_error{
            stmt_ident_->last_token(),
            std::format("expected type {}, got {}", in_quotes(tp.name()),
                        in_quotes(src_info.type_ref().name())),
        };
    }
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
                        std::format("expected return type {}, got {}",
                                    in_quotes(tp.name()),
                                    in_quotes(stmt_call_->get_type().name()))};
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
        compile_call_field(tc, indent, dst);
        return;
    }

    if (is_name_or_literal()) {
        compile_identifier_copy(tc, indent, dst);
        return;
    }

    compile_field_list(tc, indent, dst);
}

// e.g. 'f()' in '{f(), 3}' writes the field in place
inline auto expr_type::compile_call_field(toc& tc, const size_t indent,
                                          const destination& dst) const
    -> void {

    zero_padding(tc, indent, dst.type_ref(), dst.operand());
    stmt_call_->compile(tc, indent, dst);
}

inline auto expr_type::compile_field(toc& tc, const size_t indent,
                                     const expr_any& src,
                                     const type_field& field,
                                     const destination& dst) -> void {

    // the copy size comes from the source so it must match the field
    if (field.is_array() and src.is_array_identifier()) {
        validate_array_assignment(src.tok(), field, make_ident_info(tc, src));
    }

    if (not field.type().is_builtin()) {
        compile_record_field(tc, indent, src, field, dst);
        return;
    }

    // note: like any value, an empty array field is zeroed, e.g. 'msg' of
    //       'type msgpoint { msg i8[128], pt point }' given '{}'

    src.compile(tc, indent, dst);
}

// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
inline auto expr_type::compile_field_list(toc& tc, const size_t indent,
                                          const destination& dst) const
    -> void {

    const type& dst_type{dst.type_ref()};

    const std::span<const type_field> flds{dst_type.fields()};

    machine& x{tc.machine()};

    // bytes of the instance written so far, fields in order then padding
    size_t written_bytes{};

    for (const auto [expr, field] : std::views::zip(exprs_, flds)) {
        const size_t padding_bytes{field.offset - written_bytes};

        zero_unwritten(tc, indent, "padding", padding_bytes,
                       offset_alignment(written_bytes, dst_type.alignment()),
                       dst.operand_at(written_bytes, dst_type));

        written_bytes = field.offset + field.size_bytes;

        x.comment(expr->tok(), indent, "copy field {}", in_quotes(field.name));

        compile_field(tc, indent, *expr, field, dst.field(field));
    }

    // zero out the remaining fields and the padding after the written ones
    const size_t remaining_bytes{dst_type.size_bytes() - written_bytes};

    zero_unwritten(
        tc, indent,
        exprs_.size() == flds.size() ? "padding" : "remaining fields",
        remaining_bytes, offset_alignment(written_bytes, dst_type.alignment()),
        dst.operand_at(written_bytes, dst_type));
}

// e.g. 'pt1 = pt2'
inline auto expr_type::compile_identifier_copy(toc& tc, const size_t indent,
                                               const destination& dst) const
    -> void {

    const type& dst_type{dst.type_ref()};

    const ident_info src_info{make_ident_info(tc, *this)};

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

    x.copy(tok(), indent, dst.operand(), src_op, size_bytes,
           dst_type.alignment());

    x.free_scratch_registers(tok(), indent, allocated_registers);
}

// an instance field, or each element of an instance array field in turn
inline auto expr_type::compile_record_field(toc& tc, const size_t indent,
                                            const expr_any& src,
                                            const type_field& field,
                                            const destination& dst) -> void {

    // one assignment fills an instance or copies a whole array identifier
    if (not field.is_array() or src.is_array_identifier()) {
        src.as_expr_type().compile(tc, indent, dst);
        return;
    }

    const size_t count{src.element_count()};

    for (size_t i{}; i < count; ++i) {
        src.as_expr_type(i).compile(tc, indent, dst.element(i));
    }

    zero_remaining_elements(
        tc, indent, src.tok(),
        dst.operand_at(count * field.type().size_bytes(), field.type()),
        field.type(), field.array_count - count);
}

inline auto
expr_type::list_written_parts(const access_path& at,
                              std::vector<written_part>& parts) const -> void {

    // a call or a copy writes the value at once
    if (stmt_call_ or stmt_ident_) {
        parts.push_back({
            .item{this},
            .suffix{at},
        });

        return;
    }

    for (const auto [expr, field] :
         std::views::zip(exprs_, get_type().fields())) {

        access_path field_at{at};
        field_at.add_field(field.offset);
        list_field_parts(*expr, field, field_at, parts);
    }
}

inline auto expr_type::list_field_parts(const expr_any& src,
                                        const type_field& field,
                                        const access_path& at,
                                        std::vector<written_part>& parts)
    -> void {

    if (field.type().is_builtin()) {
        src.list_written_parts(at, parts);
        return;
    }

    if (not field.is_array() or src.is_array_identifier()) {
        src.as_expr_type().list_written_parts(at, parts);
        return;
    }

    for (size_t i{}; i < src.element_count(); ++i) {
        access_path element_at{at};
        element_at.add_fixed_index(i);
        src.as_expr_type(i).list_written_parts(element_at, parts);
    }
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
