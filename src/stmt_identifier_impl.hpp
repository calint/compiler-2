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

inline stmt_identifier::stmt_identifier(parse_toc& tc, const token tk,
                                        tokenizer& tz)
    : stmt_identifier{tc, tk, tz, path_reach::element} {}

inline stmt_identifier::stmt_identifier(parse_toc& tc, token tk, tokenizer& tz,
                                        const path_reach reach)
    : statement{tk}, reach_{reach} {

    // note: the callers rejected a string or a missing token before, e.g.
    //       'parse_array_argument'

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

        narrow_to_element(tc, prefix, starts_range(field == nullptr));

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

    // note: the parse records the uses, where a parameter is not bound to an
    //       argument, so a use names the parameter ('body_uses')

    tc.record_use(storage(tc), use_kind::read);

    names_result_ = tc.scopes().names_result(tok().text());
}

inline auto stmt_identifier::source_path_to(std::ostream& os) const -> void {
    assert(not elems_.empty());

    source_delimited_to(os, elems_, elem_delims_tk_);
}

inline stmt_identifier::stmt_identifier(stmt_identifier&&) noexcept = default;

inline stmt_identifier::~stmt_identifier() = default;

inline auto stmt_identifier::visit_operands(const operand_visitor visitor) const
    -> void {

    for (const ident_elem& e : elems_) {
        if (e.index) {
            visitor(node_operand::read(e.index_expr()));
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
            checked_constant_index(tc, step.elem_pos, *step.info, false),
        };

        // note: 'assert_constant_indexes' rejected a run-time index
        assert(value);
    });
}

inline auto stmt_identifier::checked_constant_index(
    const toc& tc, const size_t elem_pos, const ident_info& array_info,
    const bool allow_end) const -> std::optional<int64_t> {

    const expr_any& index{elems_.at(elem_pos).index_expr()};
    const std::optional<int64_t> value{index.constant_value(tc)};

    if (not value) {
        return value;
    }

    const machine& x{tc.machine()};

    if (is_constant_index_in_bounds(array_info, *value, allow_end,
                                    x.address_space_bytes())) {

        return value;
    }

    // note: the text is built only for the error, each index of a long path
    //       would build a text as long as the path

    throw_constant_index_out_of_bounds(array_info, index.tok(),
                                       array_text(elem_pos), *value);
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
                          x.address_space_bytes());
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
                                               const bool is_range) -> void {

    if (not is_array_element()) {
        return;
    }

    // an index out of bounds is rejected here, before a check that uses the
    // accessed bytes reports it otherwise, e.g. as a read of an unset result
    const std::optional<int64_t> constant_index{
        checked_constant_index(tc, elems_.size() - 1, array_info, is_range),
    };

    const std::optional<size_t> array_len{array_info.array_len()};

    // note: 'checked_constant_index' rejected a negative index and one past the
    //       end; the end itself starts the empty range of a zero count, which
    //       'add_index_range' cannot hold, so it is taken as a run-time index

    if (not constant_index or
        (array_len and std::cmp_equal(*constant_index, *array_len))) {

        access_path_.add_run_time_index();
        return;
    }

    const size_t index{static_cast<size_t>(*constant_index)};

    // a range of an unsized array parameter reaches to the end its call gives
    const size_t range_end{
        is_range ? array_len.value_or(access_path::run_time_end) : index + 1,
    };

    access_path_.add_index_range(index, range_end);

    // the bytes of an unsized array parameter are not known; they matter only
    // for a result, which has a length
    if (not array_len) {
        return;
    }

    const size_t element_size_bytes{array_info.type_ref().size_bytes()};
    const size_t element_offset{index * element_size_bytes};

    // the last element keeps the padding after the array
    narrow_accessed_bytes(element_offset,
                          range_end == *array_len
                              ? accessed_bytes_.size_bytes - element_offset
                              : element_size_bytes);
}

inline auto stmt_identifier::parse_element(parse_toc& tc, tokenizer& tz,
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
    const std::optional<token> open_bracket_tk{tz.accept_char_token('[')};

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

inline auto stmt_identifier::path_address::indexed_element(
    const ident_elem& cur_elem, const ident_info& cur_info, const bool is_range,
    const std::optional<operand>& start_register, const operand& address)
    -> operand {

    // an operand holds one index, so an earlier one is folded first
    operand base{address};

    if (not address.index_register().empty()) {
        const operand folded_register{fold_indexed_address(address)};
        base = operand::mem(folded_register, cur_info.type_ref());
    }

    compile_toc& tc{tc_.get()};

    const token& index_tk{cur_elem.index_expr().tok()};

    // the register of the earlier index, unless the fold took it as the base,
    // is reused since the fold read its value
    if (not index_register_) {
        // index expressions are parsed with the default type
        index_register_ = held_.get().alloc_scratch_register(
            {index_tk, tc.indent()}, tc.builtins().default_type());
    }

    const operand& index_register{*index_register_};

    const int64_t addend_elements{
        compile_index(cur_elem.index_expr(), index_register, cur_info.shape(),
                      is_range),
    };

    machine& x{tc.machine()};

    if (start_register) {
        // note: a start is copied only under bounds checks, which compile the
        //       whole index

        assert(addend_elements == 0);

        x.copy_value({index_tk, tc.indent()}, *start_register, index_register);
    }

    const size_t element_size_bytes{cur_info.type_ref().size_bytes()};

    // note: the displacement wraps as the address does, the sum is right
    //       modulo the address width, e.g. 'a[i - 9223372036854775806]'

    if (addend_elements != 0) {
        x.comment({index_tk, tc.indent()},
                  "index constant {} goes into the displacement",
                  addend_elements);
    }

    // a scale the addressing mode cannot encode is applied to the index
    // register instead, leaving scale 1 in the operand
    const bool is_encodable_scale{x.can_lower_index_scale(element_size_bytes)};

    if (not is_encodable_scale) {
        x.scale_index({index_tk, tc.indent()}, index_register,
                      element_size_bytes);
    }

    const int64_t displacement{
        add_scaled_address_offset(base.displacement(), addend_elements,
                                  element_size_bytes),
    };

    return operand::mem(base.base_register(), index_register.register_name(),
                        is_encodable_scale ? element_size_bytes : 1,
                        displacement, cur_info.type_ref());
}

inline auto stmt_identifier::path_address::compile_index(
    const expr_any& index_expr, const operand& index_register,
    const array_shape& shape, const bool is_range) -> int64_t {

    compile_toc& tc{tc_.get()};

    machine& x{tc.machine()};

    x.comment({index_expr.tok(), tc.indent()}, "set array index");

    const destination index_dst{
        destination::of_register(index_register),
    };

    if (not tc.checks().is_bounds()) {
        const std::optional<int64_t> addend{
            index_expr.compile_without_trailing_addend(tc, index_dst),
        };

        if (addend) {
            return *addend;
        }
    }

    index_expr.compile(tc, index_dst);

    // note: an unknown length checks against 'discarded_array_len', its code
    //       is discarded

    const size_t array_count{shape.element_count()};

    const machine::bounds_range checked{
        is_range ? machine::bounds_range::up_to(index_register, array_count)
                 : machine::bounds_range::index(index_register, array_count),
    };

    x.check_bounds({index_expr.tok(), tc.indent()}, checked);

    return 0;
}
