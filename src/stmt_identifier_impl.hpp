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
#include <string_view>
#include <utility>
#include <variant>

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
    : stmt_identifier{tc, tk, tz, path_reach::element} {}

inline stmt_identifier::stmt_identifier(toc& tc, token tk, tokenizer& tz,
                                        const path_reach reach)
    : statement{tk}, dotted_path_{tk.text()} {

    // note: the callers read a name, a string or no token was rejected
    //       before, e.g. by 'parse_array_argument'

    assert(tk.is_word());

    // the path read so far, resolved at its last name
    ident_info prefix{tc.resolve_declared(tk, tk.text())};

    // a path of elements, e.g. 'rooms[i].entities[j].name'
    while (true) {
        assert_indexable(prefix, tz, tk);
        parse_element(tc, tz, tk);

        const path_continuation next{read_continuation(tc, tz, prefix)};

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

        assert_element_selected(prefix, tk);
        assert_not_method_call(prefix, tz, field->name_tk);

        elem_delims_tk_.emplace_back(field->dot_tk);
        tk = field->name_tk;
        dotted_path_.push_back('.');
        dotted_path_ += tk.text();
        prefix = step_field(prefix, tok().text(), tk);
    }

    // note: a function name is rejected before a path is parsed
    //       ('parse_call_or_identifier', 'parse_array_argument'), the name of
    //       a generic function by the resolver here

    resolve_type(prefix);

    resolve_accessed_bytes(tc, reach);

    tc.record_use(tok().text(), access_path_, use_kind::read);

    names_result_ = tc.scopes().names_result(tok().text());
}

inline auto stmt_identifier::path_source_to(std::ostream& os) const -> void {
    assert(not elems_.empty());

    source_delimited_to(os, elems_, elem_delims_tk_);
}

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

inline auto stmt_identifier::assert_constant_indexes(
    const toc& tc, const std::string_view what) const -> void {

    // note: the parse checked a constant index against its array
    //       ('resolve_accessed_bytes')

    for (const ident_elem& e : elems_) {
        if (not e.index) {
            continue;
        }

        const expr_any& index{e.index_expr()};

        if (index.constant_value(tc)) {
            continue;
        }

        throw compile_error{
            index.tok(),
            std::format("an index in the argument of {} must be a "
                        "constant, every element has the same length",
                        in_quotes(what))};
    }
}

inline auto stmt_identifier::assert_count_in_bounds(const toc& tc,
                                                    const token& count_tk,
                                                    const int64_t count) const
    -> void {

    const ident_info array_info{resolve_path(tc)};

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

inline auto
stmt_identifier::narrow_to_element(const std::optional<int64_t> constant_index,
                                   const ident_info& array_info,
                                   const path_reach reach) -> void {

    const std::optional<size_t> array_len{array_info.array_len()};

    // note: 'assert_constant_index' rejected a negative index and one past the
    //       end, the end itself starts the empty range of a zero count

    if (not constant_index or not array_len or
        std::cmp_equal(*constant_index, *array_len)) {

        access_path_.add_run_time_index();
        return;
    }

    const size_t index{static_cast<size_t>(*constant_index)};
    const size_t element_size_bytes{array_info.type_ref().size_bytes()};
    const size_t element_offset{index * element_size_bytes};

    if (reach == path_reach::to_end) {
        access_path_.add_index_range(index, *array_len);
    } else {
        access_path_.add_fixed_index(index);
    }

    // the last element keeps the padding after the array
    const bool is_to_end{
        reach == path_reach::to_end or *array_len == index + 1,
    };

    narrow_accessed_bytes(
        element_offset, is_to_end ? accessed_bytes_.size_bytes - element_offset
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

    // e.g. the second '[' of 'a[1][2]', an element is not an array
    if (tz.peek_char_after_whitespace() == '[') {
        throw_not_indexable(tk, path_text());
    }
}

inline auto stmt_identifier::resolve_accessed_bytes(toc& tc,
                                                    const path_reach reach)
    -> void {

    for_each_prefix(tc, [&](const prefix_step& step) -> void {
        const ident_elem& elem{*step.elem};

        if (step.fld == nullptr) {
            // the root variable covers its whole storage
            accessed_bytes_ = {
                .offset{},
                .size_bytes{step.info->value_size_bytes(tok())},
            };
        } else {
            const size_t field_offset{step.fld->offset};

            access_path_.add_field(field_offset);

            // a field covers its trailing padding so that assigning every
            // field assigns the whole instance; the last field keeps the
            // padding after the instance that holds it, like the last element
            const size_t field_end{
                step.parent_type
                    ->next_field_offset(elem.name_tk, elem.name_tk.text())
                    .value_or(accessed_bytes_.size_bytes),
            };

            narrow_accessed_bytes(field_offset, field_end - field_offset);
        }

        if (not elem.index) {
            return;
        }

        const path_reach elem_reach{
            step.is_last ? reach : path_reach::element,
        };

        const std::optional<int64_t> constant_index{
            elem.index_expr().constant_value(tc),
        };

        // the checks that use the accessed bytes would report an unknown
        // element first, e.g. a result that may be unset
        if (constant_index) {
            assert_constant_index(*step.info, elem.index_expr().tok(),
                                  step.path, *constant_index,
                                  elem_reach == path_reach::to_end);
        }

        narrow_to_element(constant_index, *step.info, elem_reach);
    });
}

inline auto stmt_identifier::path_address::add_index(const ident_elem& cur_elem,
                                                     const ident_info& cur_info,
                                                     const lea_request& request,
                                                     const operand& address)
    -> operand {

    toc& tc{tc_.get()};
    const size_t indent{indent_};

    const token& index_tk{cur_elem.index_expr().tok()};

    machine& x{tc.machine()};

    // a register kept by an earlier fold is reused since the fold already
    // consumed its value
    if (not index_register_) {
        // index expressions are parsed with the default type
        index_register_ = x.alloc_scratch_register(
            index_tk, indent, tc.builtins().default_type());

        held_.get().add(*index_register_);
    }

    const operand& index_register{*index_register_};

    int64_t addend_elements{
        compile_checked_index(tc, indent, cur_elem.index_expr(), index_register,
                              cur_info.shape(), request.range_count),
    };

    if (request.start_register) {
        // note: a start is copied only under bounds checks, which compile the
        //       whole index

        assert(addend_elements == 0);

        x.copy_value(index_tk, indent, *request.start_register, index_register);
    }

    const size_t element_size_bytes{cur_info.type_ref().size_bytes()};

    // a constant too far for the displacement is added to the index, the
    // sum can still be an element, e.g. 'a[i - 9223372036854775806]'
    if (not fits_address_offset(address.displacement(), addend_elements,
                                element_size_bytes)) {

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
    const bool is_encodable_scale{x.can_lower_index_scale(element_size_bytes)};

    if (not is_encodable_scale) {
        x.scale_index(index_tk, indent, index_register, element_size_bytes);
    }

    return operand::mem(
        address.base_register(), index_register.register_name(),
        is_encodable_scale ? element_size_bytes : 1,
        add_address_offset(
            address.displacement(),
            scaled_address_offset(addend_elements, element_size_bytes)),
        cur_info.type_ref());
}

inline auto stmt_identifier::compile_checked_index(
    toc& tc, const size_t indent, const expr_any& index_expr,
    const operand& index_register, const array_shape& shape,
    const std::optional<size_t>& range_count) -> int64_t {

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

    const std::optional<size_t> array_len{shape.length()};

    // note: a constant count past the end was rejected at compile time; an
    //       unknown length checks the index alone, its code is discarded

    assert(not range_count or not array_len or *range_count <= *array_len);

    const machine::bounds_range checked{
        range_count and array_len
            ? machine::bounds_range::up_to(index_register,
                                           *array_len - *range_count)
            : machine::bounds_range::index(index_register,
                                           shape.element_count()),
    };

    x.check_bounds(index_expr.tok(), indent, checked, checks);

    return 0;
}

inline auto stmt_identifier::path_address::indexed_element(
    const ident_elem& cur_elem, const std::string_view path,
    const ident_info& cur_info, const lea_request& request,
    const operand& address) -> operand {

    const std::optional<int64_t> constant_index{
        cur_elem.index_expr().constant_value(tc_),
    };

    if (constant_index) {
        assert_constant_index(cur_info, cur_elem.index_expr().tok(), path,
                              *constant_index, request.range_count.has_value());

        // note: the caller checks the range from a constant start
        assert(not request.start_register);

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

    return add_index(cur_elem, cur_info, request, base);
}
