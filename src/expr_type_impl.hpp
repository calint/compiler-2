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

    const term tm{classify_term(tc, tok(), tz)};
    const bool is_bare{tm.is(term_kind::record_type)};

    // e.g. 'point{x, y}' names the type that '{x, y}' takes from the
    // destination
    const bool is_typed_literal{tm.is(term_kind::record_literal) or is_bare};

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
        parse_copy_source(tc, tz, tp, tm);
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

    field_list& list{std::get<field_list>(value_)};

    list.open_brace_tk = tz.expect_char_token(
        '{', std::format("expected '{{' to begin a value of type {}",
                         in_quotes(tp.name())));

    const std::span<const type_field> flds{tp.fields()};

    while (not at_closing_brace(tz)) {
        // each field adds one expression, e.g. the 'x' and the 'y' of '{x, y}'
        const size_t index{list.exprs.size()};

        if (index == flds.size()) {
            const token delimiter_tk{tz.is_next_char_token(',')};

            // a trailing ',' lets each field end its line the same way
            if (not delimiter_tk.is_empty() and at_closing_brace(tz)) {
                list.delims_tk.emplace_back(delimiter_tk);
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
        const array_shape shape{
            tf.is_array() ? array_shape::array(tf.array_count) : array_shape{},
        };

        list.exprs.emplace_back(std::make_unique<expr_any>(
            tc, tz, tf.type(), literal_typing::from_destination, shape));
    }
}

// the special members need the complete 'stmt_identifier' and 'stmt_call'
inline expr_type::expr_type() = default;
inline expr_type::expr_type(expr_type&&) noexcept = default;
inline auto expr_type::operator=(expr_type&&) noexcept -> expr_type& = default;
inline expr_type::~expr_type() = default;

inline expr_type::expr_type(std::unique_ptr<stmt_identifier> receiver)
    : statement{receiver->first_token()}, value_{std::move(receiver)} {

    set_type(operand_path()->get_type());
}

// e.g. 'obj.pos = p', 'obj.pos = f()', 'o.pos = lst.first()' or
// 'o.pos = point.at(1, 2)'
inline auto expr_type::parse_copy_source(toc& tc, tokenizer& tz, const type& tp,
                                         const term& tm) -> void {

    unary_ops no_uops;

    // a built-in is not a function, its value is a scalar of a built-in type
    if (const std::unique_ptr<statement> builtin{
            parse_builtin_value(tc, tz, tok(), tm, no_uops),
        }) {

        if (is_array_destination_) {
            throw compile_error{tok(), "source must be an array"};
        }

        throw compile_error{
            tok(), std::format("expected type {}, got {}", in_quotes(tp.name()),
                               in_quotes(builtin->get_type().name()))};
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tok(), tm, std::move(no_uops), &tp),
    };

    if (parsed.call) {
        value_ = std::move(parsed.call);

        assert_call_type(tp);

        return;
    }

    const stmt_identifier& source{*parsed.identifier};

    value_ = std::move(parsed.identifier);

    const ident_info src_info{make_ident_info(tc, source)};

    if (not tp.is_same(src_info.type_ref())) {
        throw compile_error{
            source.last_token(),
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

    const type& result_type{call_source()->get_type()};

    if (tp.is_same(result_type)) {
        return;
    }

    throw compile_error{tok(), std::format("expected return type {}, got {}",
                                           in_quotes(tp.name()),
                                           in_quotes(result_type.name()))};
}

inline auto expr_type::source_to(std::ostream& os) const -> void {
    if (const stmt_call* const call{call_source()}) {
        call->source_to(os);
        return;
    }

    // the token of the base statement is the first one of the identifier
    if (const stmt_identifier* const source{operand_path()}) {
        source->source_to(os);
        return;
    }

    statement::source_to(os);

    const field_list& list{std::get<field_list>(value_)};

    list.open_brace_tk.source_to(os);

    const auto exprs{
        list.exprs |
            std::views::transform([](const std::unique_ptr<expr_any>& e)
                                      -> const expr_any& { return *e; }),
    };

    source_delimited_to(os, exprs, list.delims_tk);

    list.close_brace_tk.source_to(os);
}

// a copy is of its source, e.g. the 'p' of 'q = p'
inline auto expr_type::operand_path() const -> const stmt_identifier* {
    const std::unique_ptr<stmt_identifier>* const source{
        std::get_if<std::unique_ptr<stmt_identifier>>(&value_),
    };

    return source != nullptr ? source->get() : nullptr;
}

inline auto expr_type::compile(toc& tc, const size_t indent,
                               const destination& dst) const -> void {

    if (call_source() != nullptr) {
        compile_call_field(tc, indent, dst);
        return;
    }

    // note: a literal is written in parts by the 'write_plan' of the value
    //       that holds it, see 'expr_any::plan_writes'

    assert(operand_path() != nullptr);

    compile_identifier_copy(tc, indent, dst);
}

// e.g. 'f()' in '{f(), 3}' writes the field in place
inline auto expr_type::compile_call_field(toc& tc, const size_t indent,
                                          const destination& dst) const
    -> void {

    zero_padding(tc, indent, dst.type_ref(), dst.operand());
    call_source()->compile(tc, indent, dst);
}

// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
inline auto expr_type::plan_writes(const toc& tc, const part_location& at,
                                   write_plan& plan) const -> void {

    // a call or a copy writes the value at once
    if (not is_literal()) {
        plan.push_back({
            .at{at},
            .action{
                value_write{
                    .item{this},
                },
            },
            .covered{at.path()},
        });

        return;
    }

    const std::span<const type_field> flds{get_type().fields()};

    // bytes of the instance written so far, fields in order then padding
    size_t written_bytes{};

    const field_list& list{std::get<field_list>(value_)};

    for (const auto [expr, field] : std::views::zip(list.exprs, flds)) {
        plan_unwritten(at, "padding", written_bytes,
                       field.offset - written_bytes, {}, plan);

        written_bytes = field.offset + field.size_bytes;

        plan.push_back({
            .at{at},
            .action{
                write_comment{
                    .src_loc_tk{expr->tok()},
                    .text{std::format("copy field {}", in_quotes(field.name))},
                },
            },
            .covered{},
        });

        plan_field(tc, *expr, field, at.field(field), plan);
    }

    std::vector<access_path> unlisted;

    for (const type_field& field : flds.subspan(list.exprs.size())) {
        unlisted.push_back(at.field(field).path());
    }

    // zero out the remaining fields and the padding after the written ones
    plan_unwritten(at, unlisted.empty() ? "padding" : "remaining fields",
                   written_bytes, get_type().size_bytes() - written_bytes,
                   std::move(unlisted), plan);
}

// e.g. 'pt1 = pt2'
inline auto expr_type::compile_identifier_copy(toc& tc, const size_t indent,
                                               const destination& dst) const
    -> void {

    const type& dst_type{dst.type_ref()};

    const stmt_identifier& source{*operand_path()};

    const ident_info src_info{make_ident_info(tc, source)};

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
        source.get_lea_operand(tc, indent, src_info, allocated_registers),
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

// a field of an instance literal, its array size checked against a copy
inline auto expr_type::plan_field(const toc& tc, const expr_any& src,
                                  const type_field& field,
                                  const part_location& at, write_plan& plan)
    -> void {

    const std::optional<size_t> field_count{field.array_count};

    // the copy size comes from the source so it must match the field
    if (field_count and src.is_array_copy()) {
        validate_array_assignment(src.tok(), field.type(), *field_count,
                                  make_ident_info(tc, *src.operand_path()));
    }

    // like any value: an instance by its fields, an array element by element
    // and the rest zeroed, e.g. 'msg' of 'type msgpoint { msg i8[128], pt
    // point }' given '{}', a copy or a call at once
    src.plan_writes(tc, at, std::nullopt, plan);
}

inline auto expr_type::visit_reads(const read_filter var,
                                   const read_visitor reader) const -> void {

    // a copy or a call reads its source instead of the '{...}' items
    if (const stmt_identifier* const source{operand_path()}) {
        source->visit_reads(var, reader);
        return;
    }

    if (const stmt_call* const call{call_source()}) {
        call->visit_reads(var, reader);
        return;
    }

    for (const std::unique_ptr<expr_any>& e : field_values()) {
        e->visit_reads(var, reader);
    }
}

inline auto
expr_type::visit_call_effects(const toc& tc,
                              const call_effect_visitor visitor) const -> void {

    if (const stmt_identifier* const source{operand_path()}) {
        source->visit_call_effects(tc, visitor);
        return;
    }

    if (const stmt_call* const call{call_source()}) {
        call->visit_call_effects(tc, visitor);
        return;
    }

    for (const std::unique_ptr<expr_any>& e : field_values()) {
        e->visit_call_effects(tc, visitor);
    }
}
