#pragma once

// the parse of an instance value, which makes an 'expr_type', an 'expr_path'
// or an 'expr_call_result', and the members of 'expr_type' that use
// 'expr_any'; the headers of 'expr_any' and 'stmt_call' include
// 'expr_type.hpp', see 'decouple.hpp'

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

inline auto expr_any::parse_instance(parse_toc& tc, tokenizer& tz,
                                     const type& tp,
                                     const literal_typing typing)
    -> expr_variant {

    const token tk{tz.next_token()};

    const parse_toc::parse_nesting_scope nesting{tc, tz};

    const term_kind tm{classify_term(tc, tk, tz)};
    const bool is_bare{tm == term_kind::record_type};

    // e.g. 'point{x, y}' names the type that '{x, y}' takes from the
    // destination
    const bool is_typed_literal{tm == term_kind::record_literal or is_bare};

    // e.g. 'p = i8' is the zero value of 'i8', as 'p = x' of an 'i8' 'x'
    if (is_bare_builtin_type(tc, tk, tz)) {
        throw_type_mismatch(tk, tp.name(), tc.get_type_or_throw(tk).name());
    }

    // e.g. 'p = i8[2]{1, 2}', as for a number destination
    assert_not_array_literal(tc, tz, tk, tm, tp);

    if (is_typed_literal) {
        assert_names_type(tc, tk, tp);
    }

    // e.g. 'point{1, 2}' for a field of type 'point' is '{1, 2}'
    if (is_typed_literal and typing == literal_typing::from_destination) {
        throw_known_literal_type(tz, tk, value_kind::instance, tp, tp.name());
    }

    // e.g. 'var p = point', no fields are listed
    if (is_bare) {
        assert_value_ends(tz, tk, value_kind::instance, tp);
        return expr_type{tk, tp};
    }

    // e.g. "an instance of 'point'" in the messages below
    const std::string instance_text{
        std::format("an instance of {}", in_quotes(tp.name())),
    };

    // e.g. 'q = "a"' for an instance 'q'
    if (tk.is_string()) {
        throw compile_error{
            tk,
            std::format("a string fills only an array of {}, not {}",
                        in_quotes(tc.builtins().i8().name()), instance_text)};
    }

    // e.g. the 'not' of 'q = not b', as a unary operator below
    if (tk.is_text("not")) {
        throw_not_makes_condition(tk, instance_text);
    }

    // e.g. 'p = pt', the token is empty at '{x, y}'
    if (not tk.is_none() and not is_typed_literal) {
        expr_variant source{parse_instance_source(tc, tz, tk, tm, tp)};
        assert_value_ends(tz, tk, value_kind::instance, tp);
        return source;
    }

    // e.g. the '(' of 'q = (p)' or the '-' of 'q = -p'
    assert_no_value_operator(tz, instance_text);

    expr_type literal{tc, tz, tk, tp};
    assert_value_ends(tz, tk, value_kind::instance, tp);

    return literal;
}

// e.g. 'obj.pos = p', 'obj.pos = f()', 'o.pos = lst.first()' or
// 'o.pos = point.at(1, 2)'
inline auto expr_any::parse_instance_source(parse_toc& tc, tokenizer& tz,
                                            const token& tk, const term_kind tm,
                                            const type& tp) -> expr_variant {

    // a built-in is not a function, its value is a scalar of a built-in type
    if (is_builtin_term(tm)) {
        const std::unique_ptr<statement> builtin{
            parse_builtin(tc, tz, tk, tm, unary_ops{}, builtin_place::value),
        };

        throw_type_mismatch(tk, tp.name(), builtin->get_type().name());
    }

    call_or_identifier parsed{
        parse_call_or_identifier(tc, tz, tk, tm, unary_ops{}, &tp),
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
            throw_type_mismatch(tk, tp.name(), source.get_type().name());
        }

        return expr_call_result{tk, std::move(*call)};
    }

    stmt_identifier& ident{*std::get<std::unique_ptr<stmt_identifier>>(parsed)};

    // a whole array is reported as one, not by its element type, as for a
    // number
    assert_not_whole_array(ident);

    if (not tp.is_same(ident.get_type())) {
        throw_type_mismatch(ident.last_token(), tp.name(),
                            ident.get_type().name());
    }

    return expr_path{std::move(ident)};
}

inline expr_type::expr_type(const token& type_tk, const type& tp)
    : statement{type_tk} {

    set_type(tp);
}

// the fields of 'tp' in braces, e.g. '{x, y}' of 'obj.pos = {x, y}'
inline expr_type::expr_type(parse_toc& tc, tokenizer& tz, const token& first_tk,
                            const type& tp)
    : statement{first_tk} {

    set_type(tp);

    open_brace_tk_ = tz.expect_char_token(
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

    close_brace_tk_ = parse_braced_items(
        tz, delims_tk_, full,
        [&](const size_t index) -> std::string {
            return std::format("expected ',' followed by a value for field {} "
                               "in type {}",
                               in_quotes(flds.at(index).name),
                               in_quotes(tp.name()));
        },
        [&](const size_t index) -> void {
            assert_field_unnamed(tz);

            const type_field& tf{flds.at(index)};

            // the value is parsed as the type of the field, so a nested
            // '{...}' is another 'expr_type'
            exprs_.emplace_back(std::make_unique<expr_any>(
                tc, tz, tf.type_ref(), literal_typing::from_destination,
                tf.shape));
        });
}

// the special members need the complete 'expr_any'
inline expr_type::expr_type(expr_type&&) noexcept = default;
inline expr_type::~expr_type() = default;

inline auto expr_type::source_to(std::ostream& os) const -> void {
    statement::source_to(os);

    open_brace_tk_.source_to(os);

    const auto exprs{
        exprs_ | std::views::transform([](const std::unique_ptr<expr_any>& e)
                                           -> const expr_any& { return *e; }),
    };

    source_delimited_to(os, exprs, delims_tk_);

    close_brace_tk_.source_to(os);
}

// e.g. '{1, 2}' writes the fields in order, then zeroes the unlisted fields
// and the padding
inline auto expr_type::plan_writes(const toc& tc, const part_location& at,
                                   write_plan& plan) const -> void {

    const std::span<const type_field> flds{get_type().fields()};

    // bytes of the instance written so far, fields in order then padding
    size_t written_bytes{};

    for (const auto [expr, field] : std::views::zip(exprs_, flds)) {
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

    for (const type_field& field : flds.subspan(exprs_.size())) {
        unlisted.push_back(at.field(field).path());
    }

    // zero out the remaining fields and the padding after the written ones
    plan_unwritten(at, unlisted.empty() ? "padding" : "remaining fields",
                   written_bytes, get_type().size_bytes() - written_bytes,
                   std::move(unlisted), plan);
}

inline auto expr_type::visit_operands(const operand_visitor visitor) const
    -> void {

    for (const std::unique_ptr<expr_any>& e : exprs_) {
        visitor({
            .node{e.get()},
            .index_only_place{},
        });
    }
}
