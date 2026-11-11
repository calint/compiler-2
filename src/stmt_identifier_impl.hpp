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
    : statement{tk} {

    // note: the callers read a name, a string or no token was rejected
    //       before, e.g. by 'parse_array_argument'

    assert(tk.is_word());

    // the path read so far, resolved at its last name
    ident_info prefix{tc.resolve_declared(tk, tk.text())};

    // the root variable covers its whole storage
    accessed_bytes_ = {
        .offset{},
        .size_bytes{prefix.value_size_bytes(tk)},
    };

    // the field the element names, null for the root
    const type_field* fld{};

    // a path of elements, e.g. 'rooms[i].entities[j].name'
    while (true) {
        parse_element(tc, tz, tk, fld, prefix);

        const path_continuation next{read_continuation(tc, tz, prefix)};

        const path_field* const field{std::get_if<path_field>(&next)};

        // only the last element reaches to the end of its array
        narrow_to_element(tc, prefix,
                          field == nullptr ? reach : path_reach::element);

        // the path so far is resolved as the receiver
        if (const method_tokens* const method{
                std::get_if<method_tokens>(&next),
            }) {

            method_ = *method;
            break;
        }

        if (field == nullptr) {
            break;
        }

        assert_element_selected(prefix, tk, field->name_tk);
        assert_not_method_call(prefix, tz, field->name_tk);

        elem_delims_tk_.emplace_back(field->dot_tk);
        tk = field->name_tk;

        const type& parent_type{prefix.type_ref()};

        fld = &field_of(prefix, tok().text(), tk);
        prefix = prefix.field(*fld);
        narrow_to_field(parent_type, *fld);
    }

    // note: a function name is rejected before a path is parsed
    //       ('parse_call_or_identifier', 'parse_array_argument'), the name of
    //       a generic function by the resolver here

    resolve_type(prefix);

    // note: the parse records the uses, no name is bound to an argument

    tc.record_use(storage(tc), use_kind::read);

    names_result_ = tc.scopes().names_result(tok().text());
}

inline auto stmt_identifier::path_source_to(std::ostream& os) const -> void {
    assert(not elems_.empty());

    source_delimited_to(os, elems_, elem_delims_tk_);
}

inline stmt_identifier::stmt_identifier(stmt_identifier&&) noexcept = default;

inline stmt_identifier::~stmt_identifier() = default;

inline auto stmt_identifier::visit_operands(const operand_visitor visitor) const
    -> void {

    for (const ident_elem& e : elems_) {
        if (e.index) {
            visitor({
                .node{&e.index_expr()},
                .index_only_place{},
            });
        }
    }
}

inline auto stmt_identifier::assert_constant_indexes(
    const toc& tc, const std::string_view what) const -> void {

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

inline auto
stmt_identifier::assert_constant_indexes_in_bounds(const toc& tc) const
    -> void {

    for_each_prefix(tc, [&](const prefix_step& step) -> void {
        if (not step.elem->index) {
            return;
        }

        const std::optional<int64_t> value{
            checked_constant_index(tc, step.elem_index, *step.info, false),
        };

        // note: 'assert_constant_indexes' rejected a run-time index
        assert(value);
    });
}

inline auto stmt_identifier::checked_constant_index(
    const toc& tc, const size_t elem_index, const ident_info& array_info,
    const bool allow_end) const -> std::optional<int64_t> {

    const expr_any& index{elems_.at(elem_index).index_expr()};
    const std::optional<int64_t> value{index.constant_value(tc)};

    if (value) {
        const machine& x{tc.machine()};

        assert_constant_index(array_info, index.tok(), array_text(elem_index),
                              *value, allow_end, x.max_storage_bytes());
    }

    return value;
}

inline auto stmt_identifier::assert_count_in_bounds(const toc& tc,
                                                    const token& count_tk,
                                                    const int64_t count) const
    -> void {

    const ident_info array_info{resolve_path(tc)};

    // the written start, none for a whole array
    std::optional<token> start_tk;

    if (is_array_element()) {
        start_tk = elems_.back().index_expr().tok();
    }

    const machine& x{tc.machine()};

    assert_constant_range(array_info,
                          {
                              .start{constant_start_index(tc)},
                              .count{count},
                              .start_tk{start_tk},
                              .count_tk{count_tk},
                              .count_text{std::format("count {}", count)},
                              .array_name{array_text(elems_.size() - 1)},
                          },
                          x.max_storage_bytes());
}

inline auto stmt_identifier::constant_start_index(const toc& tc) const
    -> std::optional<int64_t> {

    if (not is_array_element()) {
        return 0;
    }

    return elems_.back().index_expr().constant_value(tc);
}

inline auto stmt_identifier::narrow_to_element(const toc& tc,
                                               const ident_info& array_info,
                                               const path_reach reach) -> void {

    if (not is_array_element()) {
        return;
    }

    // the checks that use the accessed bytes would report an unknown
    // element first, e.g. a result that may be unset
    const std::optional<int64_t> constant_index{
        checked_constant_index(tc, elems_.size() - 1, array_info,
                               reach == path_reach::to_end),
    };

    const std::optional<size_t> array_len{array_info.array_len()};

    // note: 'assert_constant_index' rejected a negative index and one past the
    //       end, the end itself starts the empty range of a zero count

    // an unsized array parameter: a constant element is known, its bytes
    // are not; they matter only for a result, which has a length
    if (constant_index and not array_len and reach == path_reach::element) {
        access_path_.add_fixed_index(static_cast<size_t>(*constant_index));
        return;
    }

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
                                           const token& tk,
                                           const type_field* const fld,
                                           const ident_info& prefix) -> void {

    // note: the element is added before its index so that the messages name
    //       the path up to it, e.g. 'hs[1].x' of 'hs[1].x[0]'

    elems_.push_back({
        .name_tk{tk},
        .field{fld},
        .index{},
    });

    // e.g. the '[i]' of 'rooms[i]', none for 'name'
    const std::optional<token> open_bracket_tk{tz.is_next_char_token('[')};

    if (not open_bracket_tk) {
        return;
    }

    // the '[' of an element needs an array, e.g. the 'point[2]' of a value
    // 'point' that hides the type
    if (not prefix.is_array()) {
        const std::string path{path_text()};

        if (tc.types().has(path)) {
            throw_value_hides_type(tk, path);
        }

        throw_not_indexable(tk, path);
    }

    std::unique_ptr<expr_any> index_expr{
        std::make_unique<expr_any>(tc, tz, tc.builtins().default_type(),
                                   literal_typing::from_destination),
    };

    const token close_bracket_tk{
        tz.expect_char_token(']',
                             "expected ']' to close array index expression"),
    };

    elems_.back().index = index_part{
        .open_bracket_tk{*open_bracket_tk},
        .expr{std::move(index_expr)},
        .close_bracket_tk{close_bracket_tk},
    };

    // e.g. the second '[' of 'a[1][2]', an element is not an array
    if (tz.peek_char_after_whitespace() == '[') {
        throw_not_indexable(tk, path_text());
    }
}

inline auto stmt_identifier::path_address::add_index(const ident_elem& cur_elem,
                                                     const ident_info& cur_info,
                                                     const lea_request& request,
                                                     const operand& address)
    -> operand {

    toc& tc{tc_.get()};

    const token& index_tk{cur_elem.index_expr().tok()};

    // a register kept by an earlier fold is reused since the fold already
    // consumed its value
    if (not index_register_) {
        // index expressions are parsed with the default type
        index_register_ = held_.get().alloc_scratch_register(
            index_tk, indent_, tc.builtins().default_type());
    }

    const operand& index_register{*index_register_};

    const int64_t addend_elements{
        compile_checked_index(tc, indent_, cur_elem.index_expr(),
                              index_register, cur_info.shape(),
                              request.is_range),
    };

    machine& x{tc.machine()};

    if (request.start_register) {
        // note: a start is copied only under bounds checks, which compile the
        //       whole index

        assert(addend_elements == 0);

        x.copy_value(index_tk, indent_, *request.start_register,
                     index_register);
    }

    const size_t element_size_bytes{cur_info.type_ref().size_bytes()};

    // note: the displacement wraps as the address does, the sum is right
    //       modulo the address width, e.g. 'a[i - 9223372036854775806]'

    if (addend_elements != 0) {
        x.comment(index_tk, indent_,
                  "index constant {} goes into the displacement",
                  addend_elements);
    }

    // a scale the addressing mode cannot encode is applied to the index
    // register instead, leaving scale 1 in the operand
    const bool is_encodable_scale{x.can_lower_index_scale(element_size_bytes)};

    if (not is_encodable_scale) {
        x.scale_index(index_tk, indent_, index_register, element_size_bytes);
    }

    const int64_t displacement{
        add_scaled_address_offset(address.displacement(), addend_elements,
                                  element_size_bytes),
    };

    return operand::mem(address.base_register(), index_register.register_name(),
                        is_encodable_scale ? element_size_bytes : 1,
                        displacement, cur_info.type_ref());
}

inline auto stmt_identifier::compile_checked_index(
    toc& tc, const size_t indent, const expr_any& index_expr,
    const operand& index_register, const array_shape& shape,
    const bool is_range) -> int64_t {

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

    // note: an unknown length checks the index alone, its code is discarded

    const machine::bounds_range checked{
        is_range and array_len
            ? machine::bounds_range::up_to(index_register, *array_len)
            : machine::bounds_range::index(index_register,
                                           shape.element_count()),
    };

    x.check_bounds(index_expr.tok(), indent, checked, checks);

    return 0;
}

inline auto stmt_identifier::path_address::indexed_element(
    const ident_elem& cur_elem, const std::optional<int64_t> constant_index,
    const ident_info& cur_info, const lea_request& request,
    const operand& address) -> operand {

    if (constant_index) {
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
        const operand folded_register{fold_indexed_address(address)};
        base = operand::mem(folded_register, cur_info.type_ref());
    }

    return add_index(cur_elem, cur_info, request, base);
}
