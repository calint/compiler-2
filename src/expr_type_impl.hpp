#pragma once

// the members of 'expr_type' that use 'expr_any' and 'stmt_call'; the headers
// of 'expr_any' and 'stmt_call' include 'expr_type.hpp', see 'decouple.hpp'

#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_arith.hpp"
#include "expr_type.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "parse_statement.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

inline expr_type::expr_type(toc& tc, tokenizer& tz, const type& tp,
                            const literal_typing typing)
    : statement{tz.next_token()} {

    const toc::parse_nesting_scope nesting{tc, tz};

    set_type(tp);

    const term_kind tm{classify_term(tc, tok(), tz)};
    const bool is_bare{tm == term_kind::record_type};

    // e.g. 'point{x, y}' names the type that '{x, y}' takes from the
    // destination
    const bool is_typed_literal{tm == term_kind::record_literal or is_bare};

    // e.g. 'p = i8' is the zero value of 'i8', as 'p = x' of an 'i8' 'x'
    if (is_bare_builtin_type(tc, tok(), tz)) {
        throw_type_mismatch(tok(), tp.name(),
                            tc.get_type_or_throw(tok()).name());
    }

    // e.g. 'p = i8[2]{1, 2}' or 'p = [2]{1, 2}', as for a number destination
    if (tm == term_kind::array_literal or
        (tok().is_none() and is_default_array_literal(tz))) {

        throw_array_literal_mismatch(tc, tok(), tp);
    }

    // note: compared by type, a type parameter names the type of its argument
    if (is_typed_literal) {
        const type& named_type{tc.get_type_or_throw(tok())};

        if (not named_type.is_same(tp)) {
            throw_type_mismatch(tok(), tp.name(), named_type.name());
        }
    }

    // e.g. 'point{1, 2}' for a field of type 'point' is '{1, 2}'
    if (is_typed_literal and typing == literal_typing::from_destination) {
        throw_known_literal_type(tz, tok(), "an instance", tp, tp.name());
    }

    // e.g. 'var p = point', no fields are listed
    if (is_bare) {
        assert_value_ends(tz, tok(), "an instance", tp);
        return;
    }

    // e.g. 'q = "a"' for an instance 'q'
    if (tok().is_string()) {
        throw compile_error{
            tok(),
            std::format("a string fills only an array of {}, not {} of {}",
                        in_quotes(tc.builtins().i8().name()), "an instance",
                        in_quotes(tp.name()))};
    }

    // e.g. the 'not' of 'q = not b', as a unary operator below
    if (tok().is_text("not")) {
        throw compile_error{tok(),
                            std::format("'not' makes a condition, not {} of {}",
                                        "an instance", in_quotes(tp.name()))};
    }

    // e.g. 'p = pt', the token is empty at '{x, y}'
    if (not tok().is_none() and not is_typed_literal) {
        parse_copy_source(tc, tz, tm);
        assert_value_ends(tz, tok(), "an instance", tp);
        return;
    }

    // e.g. the '(' of 'q = (p)', parentheses group numbers
    if (tz.is_peek_char('(')) {
        throw compile_error{
            tz, std::format("a value in parentheses is a number, not {} of "
                            "{}",
                            "an instance", in_quotes(tp.name()))};
    }

    // e.g. the '-' of 'q = -p' or of 'q = -1'
    if (tz.is_unary_operator_next()) {
        throw compile_error{
            tz, std::format("a unary operator makes a number, not {} of {}",
                            "an instance", in_quotes(tp.name()))};
    }

    parse_fields(tc, tz);
    assert_value_ends(tz, tok(), "an instance", tp);
}

// an equality gets the message of a number destination, it makes a condition
inline auto expr_type::assert_value_ends(const tokenizer& tz,
                                         const token& value_tk,
                                         const std::string_view kind,
                                         const type& tp) -> void {

    if (not continues_expression(tz)) {
        return;
    }

    const std::optional<comparison_operator> op{
        comparison_of(tz.peek_operator()),
    };

    // note: instances and arrays compare only by '==' and '!='
    if (op and is_equality(*op)) {
        expr_arith::assert_no_condition_next(tz);
    }

    throw compile_error{value_tk, std::format("{} of {} is not a number", kind,
                                              in_quotes(tp.name()))};
}

inline auto
expr_type::throw_known_literal_type(const tokenizer& tz, const token& type_tk,
                                    const std::string_view kind, const type& tp,
                                    const std::string_view type_name) -> void {

    tokenizer after_literal{tz};

    const bool has_values{after_literal.peek_char_after_whitespace() == '{'};

    // note: a literal that does not end is reported by its type
    if (not has_values or after_literal.skip_braced_block()) {
        assert_value_ends(after_literal, type_tk, kind, tp);
    }

    const bool is_zero{not has_values or is_zero_value(tz)};

    throw_known_type(type_tk, type_name, is_zero ? "{}" : "{...}");
}

// the fields of 'tp' in braces, e.g. '{x, y}' of 'obj.pos = {x, y}'
inline auto expr_type::parse_fields(toc& tc, tokenizer& tz) -> void {
    const type& tp{get_type()};

    field_list& list{std::get<field_list>(value_)};

    list.open_brace_tk = tz.expect_char_token(
        '{', std::format("expected '{{' to begin a value of type {}",
                         in_quotes(tp.name())));

    const std::span<const type_field> flds{tp.fields()};

    while (not read_close_brace(tz)) {
        // each field adds one expression, e.g. the 'x' and the 'y' of '{x, y}'
        const size_t index{list.exprs.size()};

        if (index == flds.size()) {
            list.close_brace_tk = read_full_list_end(
                tz, list.delims_tk,
                std::format("too many fields for type {}",
                            in_quotes(tp.name())),
                std::format("expected '}}' to close the value of type {}",
                            in_quotes(tp.name())));

            break;
        }

        const type_field& tf{flds.at(index)};

        if (index > 0) {
            read_field_delimiter(tz, tf);

            // a trailing ',' lets each field end its line the same way
            if (read_close_brace(tz)) {
                break;
            }
        }

        // the value is parsed as the type of the field, so a nested '{...}'
        // is another 'expr_type'
        list.exprs.emplace_back(std::make_unique<expr_any>(
            tc, tz, tf.type_ref(), literal_typing::from_destination, tf.shape));
    }
}

// the special members need the complete 'stmt_call'
inline expr_type::expr_type(expr_type&&) noexcept = default;
inline auto expr_type::operator=(expr_type&&) noexcept -> expr_type& = default;
inline expr_type::~expr_type() = default;

// e.g. 'obj.pos = p', 'obj.pos = f()', 'o.pos = lst.first()' or
// 'o.pos = point.at(1, 2)'
inline auto expr_type::parse_copy_source(toc& tc, tokenizer& tz,
                                         const term_kind tm) -> void {

    const type& tp{get_type()};

    unary_ops no_uops;

    // a built-in is not a function, its value is a scalar of a built-in type
    if (const std::unique_ptr<statement> builtin{
            parse_builtin(tc, tz, tok(), tm, no_uops, builtin_place::value),
        }) {

        throw_type_mismatch(tok(), tp.name(), builtin->get_type().name());
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tok(), tm, std::move(no_uops), &tp),
    };

    if (std::unique_ptr<stmt_call>* const call{
            std::get_if<std::unique_ptr<stmt_call>>(&parsed),
        }) {

        value_ = std::move(*call);

        assert_call_type();

        return;
    }

    value_ = expr_path{
        std::move(*std::get<std::unique_ptr<stmt_identifier>>(parsed)),
    };

    const stmt_identifier& ident{*operand_path()};

    const ident_info src_info{make_ident_info(tc, ident)};

    // note: a whole array for an instance is rejected before its type, as
    //       for a number: by the plan of an assignment
    //       ('assert_copy_source_shape') or by the call it is an argument of,
    //       which checks the type of an array parameter

    if (src_info.is_array()) {
        return;
    }

    if (not tp.is_same(src_info.type_ref())) {
        throw_type_mismatch(ident.last_token(), tp.name(),
                            src_info.type_ref().name());
    }
}

inline auto expr_type::assert_call_type() const -> void {
    const type& tp{get_type()};

    const stmt_call& call{*call_source()};

    // e.g. 'p = f()' with 'func f()', the message of a number destination
    if (not call.callee().has_result()) {
        throw_no_value(call.callee_name_token(), call.callee().name());
    }

    const type& result_type{call.get_type()};

    if (tp.is_same(result_type)) {
        return;
    }

    throw_type_mismatch(tok(), tp.name(), result_type.name());
}

inline auto expr_type::source_to(std::ostream& os) const -> void {
    if (const stmt_call* const call{call_source()}) {
        call->source_to(os);
        return;
    }

    if (const expr_path* const path{std::get_if<expr_path>(&value_)}) {
        path->source_to(os);
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
    const expr_path* const path{std::get_if<expr_path>(&value_)};
    return path != nullptr ? path->operand_path() : nullptr;
}

inline auto expr_type::compile(toc& tc, const size_t indent,
                               const destination& dst) const -> void {

    // note: the call zeroes the padding of the result, see
    //       'call_site::zero_result_padding'

    if (const stmt_call* const call{call_source()}) {
        call->compile(tc, indent, dst);
        return;
    }

    // note: a literal is written in parts by the 'write_plan' of the value
    //       that holds it, see 'expr_any::plan_writes'

    // note: 'plan_writes' checked the shape, the plan of every value is made
    //       before it is compiled ('assert_copy_source_shape')

    assert(not make_ident_info(tc, *operand_path()).is_array());

    std::get<expr_path>(value_).compile(tc, indent, dst);
}

// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
inline auto expr_type::plan_writes(const toc& tc, const part_location& at,
                                   write_plan& plan) const -> void {

    // a call or a copy writes the value at once
    if (not is_literal()) {
        assert_copy_source_shape(tc);

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

        // like any value: an instance by its fields, an array element by
        // element and the rest zeroed, a copy or a call at once; an array
        // copy is checked against the field
        expr->plan_writes(tc, at.field(field), std::nullopt, plan);
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

// the copy size comes from the source: an instance is copied only from one;
// checked when the plan is made, at compile time, where a parameter is
// resolved to what its call passed
inline auto expr_type::assert_copy_source_shape(const toc& tc) const -> void {
    const stmt_identifier* const ident{operand_path()};

    if (ident == nullptr or not make_ident_info(tc, *ident).is_array()) {
        return;
    }

    throw_unindexed_array(tok(), ident->dotted_path());
}

inline auto expr_type::visit_operands(const operand_visitor visitor) const
    -> void {

    if (const stmt_identifier* const ident{operand_path()}) {
        visitor({
            .node{ident},
            .written_place{},
        });

        return;
    }

    if (const stmt_call* const call{call_source()}) {
        visitor({
            .node{call},
            .written_place{},
        });

        return;
    }

    for (const std::unique_ptr<expr_any>& e : field_values()) {
        visitor({
            .node{e.get()},
            .written_place{},
        });
    }
}
