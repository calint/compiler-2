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
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_type.hpp"
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

    // e.g. 'p = i8[2]{1, 2}', as for a number destination
    assert_not_array_literal(tc, tz, tok(), tm, tp);

    if (is_typed_literal) {
        assert_names_type(tc, tok(), tp);
    }

    // e.g. 'point{1, 2}' for a field of type 'point' is '{1, 2}'
    if (is_typed_literal and typing == literal_typing::from_destination) {
        throw_known_literal_type(tz, tok(), value_kind::instance, tp,
                                 tp.name());
    }

    // e.g. 'var p = point', no fields are listed
    if (is_bare) {
        assert_value_ends(tz, tok(), value_kind::instance, tp);
        return;
    }

    // e.g. "an instance of 'point'" in the messages below
    const std::string instance_text{
        std::format("an instance of {}", in_quotes(tp.name())),
    };

    // e.g. 'q = "a"' for an instance 'q'
    if (tok().is_string()) {
        throw compile_error{
            tok(),
            std::format("a string fills only an array of {}, not {}",
                        in_quotes(tc.builtins().i8().name()), instance_text)};
    }

    // e.g. the 'not' of 'q = not b', as a unary operator below
    if (tok().is_text("not")) {
        throw compile_error{
            tok(),
            std::format("'not' makes a condition, not {}", instance_text)};
    }

    // e.g. 'p = pt', the token is empty at '{x, y}'
    if (not tok().is_none() and not is_typed_literal) {
        parse_copy_source(tc, tz, tm);
        assert_value_ends(tz, tok(), value_kind::instance, tp);
        return;
    }

    // e.g. the '(' of 'q = (p)' or the '-' of 'q = -p'
    assert_no_value_operator(tz, instance_text);

    parse_fields(tc, tz);
    assert_value_ends(tz, tok(), value_kind::instance, tp);
}

// the fields of 'tp' in braces, e.g. '{x, y}' of 'obj.pos = {x, y}'
inline auto expr_type::parse_fields(toc& tc, tokenizer& tz) -> void {
    const type& tp{get_type()};

    field_list& list{std::get<field_list>(value_)};

    list.open_brace_tk = tz.expect_char_token(
        '{', std::format("expected '{{' to begin a value of type {}",
                         in_quotes(tp.name())));

    const std::span<const type_field> flds{tp.fields()};

    const full_list full{
        .capacity{flds.size()},
        .too_many{
            std::format("too many fields for type {}", in_quotes(tp.name())),
        },
        .unclosed{
            std::format("expected '}}' to close the value of type {}",
                        in_quotes(tp.name())),
        },
    };

    list.close_brace_tk = parse_braced_items(
        tz, list.delims_tk, full,
        [&](const size_t index) -> std::string {
            return std::format("expected ',' followed by a value for field {} "
                               "in type {}",
                               in_quotes(flds.at(index).name),
                               in_quotes(tp.name()));
        },
        [&](const size_t index) -> void {
            const type_field& tf{flds.at(index)};

            // the value is parsed as the type of the field, so a nested
            // '{...}' is another 'expr_type'
            list.exprs.emplace_back(std::make_unique<expr_any>(
                tc, tz, tf.type_ref(), literal_typing::from_destination,
                tf.shape));
        });
}

// the special members need the complete 'expr_any'
inline expr_type::expr_type(expr_type&&) noexcept = default;
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

        const stmt_call& source{**call};

        // e.g. 'p = f()' with 'func f()', the message of a number destination
        if (not source.callee().has_result()) {
            throw_no_value(source.callee_name_token(), source.callee().name());
        }

        if (not tp.is_same(source.get_type())) {
            throw_type_mismatch(tok(), tp.name(), source.get_type().name());
        }

        value_ = std::move(*call);

        return;
    }

    value_ = std::make_unique<expr_path>(
        std::move(*std::get<std::unique_ptr<stmt_identifier>>(parsed)));

    const stmt_identifier& ident{*operand_path()};

    // a whole array is reported as one, not by its element type, as for a
    // number
    assert_not_whole_array(ident);

    if (not tp.is_same(ident.get_type())) {
        throw_type_mismatch(ident.last_token(), tp.name(),
                            ident.get_type().name());
    }
}

inline auto expr_type::copy_or_call() const -> const statement* {
    const std::unique_ptr<statement>* const value{
        std::get_if<std::unique_ptr<statement>>(&value_),
    };

    return value != nullptr ? value->get() : nullptr;
}

inline auto expr_type::source_to(std::ostream& os) const -> void {
    if (const statement* const value{copy_or_call()}) {
        value->source_to(os);
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
    const statement* const value{copy_or_call()};
    return value != nullptr ? value->operand_path() : nullptr;
}

// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
inline auto expr_type::plan_writes(const toc& tc, const part_location& at,
                                   write_plan& plan) const -> void {

    // a call or a copy writes the value at once; the call zeroes the
    // padding of the result, see 'call_site::zero_result_padding'
    if (const statement* const value{copy_or_call()}) {
        plan.push_back({
            .at{at},
            .action{
                value_write{
                    .item{value},
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

inline auto expr_type::visit_operands(const operand_visitor visitor) const
    -> void {

    if (const statement* const value{copy_or_call()}) {
        visitor({
            .node{value},
            .index_only_place{},
        });

        return;
    }

    for (const std::unique_ptr<expr_any>& e : field_values()) {
        visitor({
            .node{e.get()},
            .index_only_place{},
        });
    }
}
