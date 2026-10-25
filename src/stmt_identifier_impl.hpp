#pragma once

// the members of 'stmt_identifier' that use its index expressions, an
// 'expr_any': 'stmt_identifier.hpp' only declares 'expr_any', so that the
// expressions, which 'expr_any.hpp' includes, can include it

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "access_path.hpp"
#include "compile_error.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

inline auto stmt_identifier::ident_elem::source_to(std::ostream& os) const
    -> void {

    name_tk.source_to(os);

    if (index) {
        index->open_bracket_tk.source_to(os);
        index->expr->source_to(os);
        index->close_bracket_tk.source_to(os);
    }
}

inline stmt_identifier::stmt_identifier(toc& tc, token tk, tokenizer& tz)
    : statement{tk}, dotted_path_{tk.text()} {

    // e.g. '"abc"' in 'array_copy("abc", b, 3)'
    if (tk.is_string()) {
        throw compile_error{tk, "expected an identifier"};
    }

    if (tk.is_none()) {
        throw compile_error{tz, "expected an identifier"};
    }

    // the path read so far, resolved at its last name
    std::optional<ident_info> prefix;

    // a path of elements, e.g. 'rooms[i].entities[j].name'
    while (true) {
        prefix.emplace(tc.resolve_declared(tk, dotted_path_));

        assert_indexable(*prefix, tz, tk);
        parse_element(tc, tz, tk);

        const path_continuation next{read_continuation(tc, tz, *prefix)};

        // the path so far is resolved as the receiver
        if (const method_tokens* const method{
                std::get_if<method_tokens>(&next),
            }) {

            method_ = *method;
            break;
        }

        const path_field* const field{std::get_if<path_field>(&next)};

        if (field == nullptr) {
            break;
        }

        assert_element_selected(*prefix, tk);
        assert_not_method_call(*prefix, tz, field->name_tk);

        elem_delims_tk_.emplace_back(field->dot_tk);
        tk = field->name_tk;
        dotted_path_.push_back('.');
        dotted_path_ += tk.text();
    }

    // note: a function name is rejected before a path is parsed
    //       ('parse_call_or_identifier', 'parse_array_argument'), the name of
    //       a generic function by the resolver here

    resolve_type(*prefix);

    resolve_accessed_bytes(tc);

    tc.record_use(ident_info::root_of(dotted_path_), access_path_,
                  use_kind::read);
}

inline auto stmt_identifier::path_source_to(std::ostream& os) const -> void {
    assert(not elems_.empty());

    source_delimited_to(os, elems_, elem_delims_tk_);
}

inline stmt_identifier::stmt_identifier() = default;

inline stmt_identifier::stmt_identifier(stmt_identifier&&) noexcept = default;

inline auto stmt_identifier::operator=(stmt_identifier&&) noexcept
    -> stmt_identifier& = default;

inline stmt_identifier::~stmt_identifier() = default;

inline auto stmt_identifier::visit_operands(const operand_visitor visitor) const
    -> void {

    for (const ident_elem& e : elems_) {
        if (e.index) {
            visitor({
                .node{&e.index_expr()},
                .written_place{},
            });
        }
    }
}

inline auto stmt_identifier::visit_index_reads(const read_filter var,
                                               const read_visitor reader) const
    -> void {

    statement::visit_reads(var, reader);
}

inline auto stmt_identifier::assert_count_in_bounds(const toc& tc,
                                                    const token& count_tk,
                                                    const int64_t count) const
    -> void {

    const ident_info array_info{
        tc.resolve_declared(first_token(), dotted_path_),
    };

    assert_constant_range(
        array_info,
        {
            .start{constant_start_index(tc)},
            .count{count},
            .start_tk{
                is_array_element()
                    ? std::optional{elems_.back().index_expr().tok()}
                    : std::nullopt,
            },
            .count_tk{count_tk},
            .count_text{std::format("count {}", count)},
            .array_name{dotted_path_},
        });
}

inline auto stmt_identifier::constant_start_index(const toc& tc) const
    -> std::optional<int64_t> {

    if (not is_array_element()) {
        return 0;
    }

    return elems_.back().index_expr().constant_value(tc);
}

inline auto stmt_identifier::narrow_to_element(toc& tc,
                                               const expr_any& index_expr,
                                               const ident_info& array_info)
    -> void {

    const std::optional<size_t> index{
        in_range_constant_index(tc, index_expr, array_info),
    };

    const size_t element_size_bytes{array_info.type_ref().size_bytes()};

    if (not index) {
        access_path_.add_run_time_index();
        return;
    }

    const size_t element_offset{*index * element_size_bytes};

    // the last element keeps the padding after the array
    // note: an index in range has a known length
    const bool is_last_element{array_info.array_len() == *index + 1};

    access_path_.add_fixed_index(*index);

    narrow_accessed_bytes(element_offset,
                          is_last_element
                              ? accessed_bytes_.size_bytes - element_offset
                              : element_size_bytes);
}

inline auto stmt_identifier::parse_element(toc& tc, tokenizer& tz,
                                           const token& tk) -> void {

    // e.g. the '[i]' of 'rooms[i]', none for 'name'
    const std::optional<token> open_bracket_tk{tz.is_next_char_token('[')};

    if (not open_bracket_tk) {
        elems_.push_back({
            .name_tk{tk},
            .index{},
        });

        return;
    }

    std::unique_ptr<expr_any> index_expr{
        std::make_unique<expr_any>(tc, tz, tc.builtins().default_type(),
                                   literal_typing::from_destination),
    };

    const token close_bracket_tk{
        tz.expect_char_token(']',
                             "expected ']' to close array index expression"),
    };

    elems_.push_back({
        .name_tk{tk},
        .index{
            index_part{
                .open_bracket_tk{*open_bracket_tk},
                .expr{std::move(index_expr)},
                .close_bracket_tk{close_bracket_tk},
            },
        },
    });
}

inline auto stmt_identifier::resolve_accessed_bytes(toc& tc) -> void {
    for_each_prefix(tc, std::nullopt, [&](const prefix_step& step) -> void {
        const ident_elem& elem{*step.elem};

        if (step.parent_type == nullptr) {
            accessed_bytes_ = root_range(*step.info);
        } else {
            // a field covers its trailing padding so that assigning every
            // field assigns the whole instance
            access_path_.add_field(step.field_offset);

            const size_t extent_bytes{
                step.parent_type->field_extent_bytes(elem.name_tk,
                                                     elem.name_tk.text()),
            };

            // the last field keeps the padding after the instance that
            // holds it, like the last element
            const bool is_last_field{
                step.field_offset + extent_bytes ==
                    step.parent_type->size_bytes(),
            };

            narrow_accessed_bytes(step.field_offset,
                                  is_last_field ? accessed_bytes_.size_bytes -
                                                      step.field_offset
                                                : extent_bytes);
        }

        if (elem.index) {
            narrow_to_element(tc, elem.index_expr(), *step.info);
        }
    });
}

inline auto stmt_identifier::path_address::add_index(const ident_elem& cur_elem,
                                                     const ident_info& cur_info,
                                                     const operand& reg_count,
                                                     const operand& address)
    -> operand {

    toc& tc{tc_.get()};
    const size_t indent{indent_};

    machine& x{tc.machine()};

    const token& index_tk{cur_elem.index_expr().tok()};

    // a register kept by an earlier fold is reused since the fold already
    // consumed its value
    if (index_register_.is_empty()) {
        // index expressions are parsed with the default type
        index_register_ = x.alloc_scratch_register(
            index_tk, indent, tc.builtins().default_type());

        held_.get().add(index_register_);
    }

    const operand& index_register{index_register_};

    int64_t addend_elements{
        compile_checked_index(tc, indent, cur_elem.index_expr(), index_register,
                              cur_info.element_count(), reg_count),
    };

    const size_t type_size{cur_info.type_ref().size_bytes()};

    // a constant too far for the displacement is added to the index, the
    // sum can still be an element, e.g. 'a[i - 9223372036854775806]'
    if (not fits_address_offset(address.displacement(), addend_elements,
                                type_size)) {

        x.comment(index_tk, indent, "index constant {} is added",
                  addend_elements);

        x.add_subtract(
            index_tk, indent, arithmetic_operator::add, index_register,
            operand::imm(addend_elements, tc.builtins().default_type()));

        addend_elements = 0;
    } else if (addend_elements != 0) {
        x.comment(index_tk, indent,
                  "index constant {} goes into the displacement",
                  addend_elements);
    }

    // a scale the addressing mode cannot encode is applied to the index
    // register instead, leaving scale 1 in the operand
    const bool is_encodable_scale{x.can_lower_index_scale(type_size)};

    if (not is_encodable_scale) {
        x.scale_index(index_tk, indent, index_register, type_size);
    }

    return operand::mem(
        address.base_register(), index_register.register_name(),
        is_encodable_scale ? type_size : 1,
        add_address_offset(address.displacement(),
                           scaled_address_offset(addend_elements, type_size)),
        cur_info.type_ref());
}

inline auto stmt_identifier::compile_checked_index(
    toc& tc, const size_t indent, const expr_any& index_expr,
    const operand& index_register, const size_t array_length,
    const operand& range_count) -> int64_t {

    machine& x{tc.machine()};

    x.comment(index_expr.tok(), indent, "set array index");

    const destination index_dst{
        destination::of_register(index_register),
    };

    const machine::bounds_check_options checks{
        tc.checks().bounds_options(),
    };

    if (not checks.upper and not checks.lower) {
        const std::optional<int64_t> addend{
            index_expr.compile_without_trailing_addend(tc, indent, index_dst),
        };

        if (addend) {
            return *addend;
        }
    }

    index_expr.compile(tc, indent, index_dst);

    const machine::bounds_range checked{
        range_count.is_empty()
            ? machine::bounds_range::index(index_register, array_length)
            : machine::bounds_range::range(index_register, range_count,
                                           array_length),
    };

    check_array_bounds(tc, indent, index_expr.tok(), checked);

    return 0;
}

inline auto stmt_identifier::path_address::indexed_element(
    const ident_elem& cur_elem, const std::string_view path,
    const ident_info& cur_info, const operand& range_count,
    const operand& address) -> operand {

    const std::optional<int64_t> constant_index{
        cur_elem.index_expr().constant_value(tc_),
    };

    if (constant_index) {
        assert_constant_index(cur_info, cur_elem.index_expr().tok(), path,
                              *constant_index, not range_count.is_empty());
    }

    // with a range count the run-time check covers 'index + count'
    if (constant_index and range_count.is_empty()) {
        operand folded{address};

        folded.increment_offset(
            address_offset(static_cast<size_t>(*constant_index) *
                           cur_info.type_ref().size_bytes()));

        return folded;
    }

    // an operand holds one index, so an earlier one is folded first
    operand base{address};

    if (not address.index_register().empty()) {
        base = operand::mem(fold_indexed_address(address), cur_info.type_ref());
    }

    return add_index(cur_elem, cur_info, range_count, base);
}

inline auto stmt_identifier::in_range_constant_index(
    const toc& tc, const expr_any& index_expr, const ident_info& array_info)
    -> std::optional<size_t> {

    const std::optional<int64_t> index{index_expr.constant_value(tc)};

    const std::optional<size_t> array_len{array_info.array_len()};

    if (not index or *index < 0 or not array_len or
        std::cmp_greater_equal(*index, *array_len)) {

        return std::nullopt;
    }

    return static_cast<size_t>(*index);
}
