#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "expr_type.hpp"
#include "expr_zero.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "token.hpp"
#include "ub_reads.hpp"

class expr_any;

// the constant size of an array, e.g. the '4' of 'i8[4]' or the 'n * 2' of a
// field 'i8[n * 2]': an expression at the default type, folded by 'fold' when
// parsed and never compiled; empty for 'i8[]'
// note: defined after 'expr_any', which holds one
class stmt_array_size final : public statement {
    std::unique_ptr<expr_any> value_;
    int64_t folded_{};

  public:
    stmt_array_size(toc& tc, tokenizer& tz);

    stmt_array_size(stmt_array_size&&) noexcept;

    stmt_array_size(const stmt_array_size&) = delete;
    auto operator=(const stmt_array_size&) -> stmt_array_size& = delete;

    ~stmt_array_size() override;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override;

    //
    // class methods
    //

    // the written size, at least 1 (checked when parsed), e.g. the '4' of
    // 'i8[4]'; asked only when 'has_value'
    [[nodiscard]] auto array_size() const -> size_t;

    [[nodiscard]] auto has_value() const -> bool { return value_ != nullptr; }

    auto operator=(stmt_array_size&&) noexcept -> stmt_array_size&;
};

// the value of an initializer, an argument or a field, parsed at the type of
// its destination; one of the forms below, an array form only where the
// destination is an array
class expr_any final : public statement {
    using expr_variant = std::variant<expr_arith, expr_bool, expr_type>;

    // e.g. 'i8[3]' of 'i8[3]{1, 2}' or '[3]' at the default type, empty for
    // '{1, 2}', whose type and size the destination gives
    struct array_prefix {
        // none for the default type, e.g. '[2]{1, 2}'
        std::optional<token> element_type_tk;
        token open_bracket_tk;
        stmt_array_size size;
        token close_bracket_tk;

        auto source_to(std::ostream& os) const -> void {
            token::source_to(os, element_type_tk);
            open_bracket_tk.source_to(os);
            size.source_to(os);
            close_bracket_tk.source_to(os);
        }
    };

    // e.g. 'x + 1', 'a < b' or 'point{1, 2}'; where the destination is an
    // array, the array copied whole as bytes, e.g. the 'a' of 'var b = a'
    struct single_value {
        expr_variant value;
    };

    // e.g. '"hi"', which fills the start of an 'i8' array
    struct string_array {
        token string_tk;
    };

    // an element of an 'element_list' after the whitespace before it, read
    // apart like that of a single value, a 'bool' operand the element unwraps
    // would drop it
    struct listed_element {
        token ws_tk;
        expr_variant value;

        auto source_to(std::ostream& os) const -> void {
            ws_tk.source_to(os);
            as_statement(value).source_to(os);
        }
    };

    // e.g. 'i8[]{1, 2}' or '{{1, 2}, {3, 4}}'
    struct element_list {
        // none when the destination gives the type and the size
        std::optional<array_prefix> prefix;
        token open_brace_tk;
        std::vector<listed_element> elements;
        std::vector<token> delims_tk;
        token close_brace_tk;
    };

    // e.g. 'i8[4]', every element is zero
    struct zero_array {
        array_prefix prefix;
    };

    using form_variant =
        std::variant<single_value, string_array, element_list, zero_array>;

    // what the destination gives, e.g. an array parameter 's[]' is an array
    // of unknown length, its argument gives the length when it is compiled
    array_shape destination_;
    // the size of an array form: the length of the destination, else the size
    // the literal gives, e.g. 3 of 'i8[]{1, 2, 3}'; empty until known
    std::optional<size_t> array_count_;
    form_variant form_;

  public:
    // a single value, or an array when 'destination' is one; where 'typing' is
    // 'written' a type or array may be written without '{}', e.g. 'var p =
    // point' and 'var a = i8[4]'
    expr_any(toc& tc, tokenizer& tz, const type& tp,
             const literal_typing typing, const array_shape destination = {})
        : statement{tz.next_whitespace_token()}, destination_{destination},
          array_count_{destination.length()},
          form_{parse_form(tc, tz, tp, typing)} {

        set_type(tp);

        // a copy checks its end itself
        if (is_array() and not is_whole_array_copy()) {
            expr_type::assert_value_ends(tz, array_token(), "an array", tp);
        }
    }

    // a path parsed apart, e.g. a method receiver or the array argument of a
    // built-in
    expr_any(const token pos_tk, expr_type path)
        : statement{pos_tk}, form_{single_value{
                                 .value{std::move(path)},
                             }} {

        set_type(as_expr_type().get_type());
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            str->string_tk.source_to(os);
            return;
        }

        if (const zero_array* const zero{std::get_if<zero_array>(&form_)}) {
            zero->prefix.source_to(os);
            return;
        }

        const element_list* const list{std::get_if<element_list>(&form_)};

        if (list == nullptr) {
            element(0).source_to(os);
            return;
        }

        if (list->prefix) {
            list->prefix->source_to(os);
        }

        list->open_brace_tk.source_to(os);

        source_delimited_to(os, list->elements, list->delims_tk);

        list->close_brace_tk.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // note: an unsized destination is the only array of unknown length

        assert(not is_array() or is_whole_array_copy() or
               dst.is_length_unknown() ==
                   (destination_.is_length_unknown() and not dst.array_len()));

        write_plan plan;
        plan_writes(tc, part_location{}, dst.array_len(), plan);
        emit_write_plan(tc, indent, plan, dst);
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        if (is_array()) {
            return;
        }

        element(0).assert_not_narrowed(tc, dst_type);
    }

    // a scalar folds as its value
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        assert(not is_array());

        return element(0).fold(tc, use);
    }

    // a single value or the copy of an array, e.g. the 'a' of 'var b = a'
    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override {
        if (not std::holds_alternative<single_value>(form_)) {
            return nullptr;
        }

        return element(0).operand_path();
    }

    [[nodiscard]] auto tok() const -> const token& override {
        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            return str->string_tk;
        }

        if (element_count() == 0) {
            return statement::tok();
        }

        return element(0).tok();
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        for (size_t i{}; i < element_count(); ++i) {
            visitor({
                .node{&element(i)},
                .written_place{},
            });
        }
    }

    //
    // class methods
    //

    // empty when neither the destination nor the literal gives the size
    [[nodiscard]] auto array_count() const -> std::optional<size_t> {
        return array_count_;
    }

    // where an array is written: its element type, e.g. 'i8' of 'i8[]{1}', its
    // '[' when the type is the default one, its '{' when the destination gives
    // the type, else its value, e.g. a string
    [[nodiscard]] auto array_token() const -> const token& {
        if (const array_prefix* const prefix{array_prefix_of()}) {
            if (prefix->element_type_tk) {
                return *prefix->element_type_tk;
            }

            return prefix->open_bracket_tk;
        }

        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            return list->open_brace_tk;
        }

        return tok();
    }

    // the element at 'index' of an instance value or of an array of
    // instances, or the source of an array copy
    [[nodiscard]] auto as_expr_type(const size_t index = 0) const
        -> const expr_type& {

        const expr_type* const record{record_at(index)};

        assert(record != nullptr);

        return *record;
    }

    // the fold of a condition stops at the element that decides it, so the
    // elements after it are checked like skipped code, e.g. the 'a / 0 == 1'
    // of 'true or a / 0 == 1'; every condition of the value is, also in the
    // elements of a list and in the fields of an instance literal
    auto check_folded_conditions(toc& tc, const size_t indent) const -> void {
        if (const single_value* const single{
                std::get_if<single_value>(&form_),
            }) {

            check_folded_conditions_of(tc, indent, single->value);
            return;
        }

        if (const element_list* const list{
                std::get_if<element_list>(&form_),
            }) {

            for (const listed_element& element : list->elements) {
                check_folded_conditions_of(tc, indent, element.value);
            }
        }
    }

    // compiles all but the trailing added constant of an arithmetic
    // expression and returns the constant, e.g. 'ix + 1' compiles 'ix' and
    // returns 1, empty when nothing was compiled
    [[nodiscard]] auto
    compile_without_trailing_addend(toc& tc, const size_t indent,
                                    const destination& dst) const
        -> std::optional<int64_t> {

        // note: only an index expression asks, it is never an array and has
        //       the default type

        const expr_arith& arith{
            std::get<expr_arith>(std::get<single_value>(form_).value),
        };

        arith.assert_not_narrowed(tc, dst.type_ref());

        return arith.compile_without_trailing_addend(tc, indent, dst);
    }

    // e.g. '-2' or a named constant at the default type, empty when computed
    // at run time
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<int64_t> {

        return folded_value::value_of(
            fold_checked(tc, tc.use_at(tc.builtins().default_type())));
    }

    // the value of a single value, an element of a list or the source of a
    // copy
    [[nodiscard]] auto element(const size_t index) const -> const statement& {
        return as_statement(value_at(index));
    }

    // none for a string or a zero array
    [[nodiscard]] auto element_count() const -> size_t {
        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            return list->elements.size();
        }

        if (std::holds_alternative<string_array>(form_) or
            std::holds_alternative<zero_array>(form_)) {

            return 0;
        }

        return 1;
    }

    // the element 'index' folded and checked, empty when it is computed at
    // run time, e.g. 'maybe == 33' with 'maybe' a constant folds
    [[nodiscard]] auto fold_element(const toc& tc, const size_t index,
                                    const value_use& use) const
        -> std::optional<folded_value> {

        return element(index).fold_checked(tc, use);
    }

    [[nodiscard]] auto is_array() const -> bool {
        return destination_.is_array();
    }

    [[nodiscard]] auto is_string() const -> bool {
        return std::holds_alternative<string_array>(form_);
    }

    [[nodiscard]] auto is_whole_array_copy() const -> bool {
        return is_array() and std::holds_alternative<single_value>(form_);
    }

    // appends the steps that write the value at 'at'; 'unsized_count' is the
    // size of an unsized destination, e.g. a parameter 's[]', empty when it
    // is not known, then the elements after the literal are not written
    auto plan_writes(const toc& tc, const part_location& at,
                     const std::optional<size_t> unsized_count,
                     write_plan& plan) const -> void {

        // empty when the size is not known
        const std::optional<size_t> array_count{
            destination_.is_length_unknown() ? unsized_count : array_count_,
        };

        // the source's shape of an array copy is checked by its plan, then
        // the lengths
        if (std::holds_alternative<single_value>(form_)) {
            plan_element(tc, 0, at, plan);

            if (is_array()) {
                assert_whole_array_copy_fits(tc, array_count);
            }

            return;
        }

        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            plan_string(str->string_tk, at, array_count, plan);
            return;
        }

        const size_t count{element_count()};

        // the parser could not check the size of an unsized destination
        if (array_count and count > *array_count) {
            throw compile_error{element(*array_count).tok(),
                                too_many_elements(*array_count)};
        }

        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            plan_elements(tc, *list, at, plan);
        }

        if (not array_count) {
            return;
        }

        plan_remaining_elements(tok(), at, count, *array_count, plan);
    }

    // where a literal written without a size gives one, e.g. the '[' of
    // 'i8[]{}' or the string '""'; none for another form
    [[nodiscard]] auto size_token() const -> std::optional<token> {
        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            return str->string_tk;
        }

        const array_prefix* const prefix{array_prefix_of()};

        if (prefix == nullptr) {
            return std::nullopt;
        }

        return prefix->open_bracket_tk;
    }

  private:
    // the prefix of an element list or a zero array, null for another form
    [[nodiscard]] auto array_prefix_of() const -> const array_prefix* {
        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            return list->prefix ? &*list->prefix : nullptr;
        }

        if (const zero_array* const zero{std::get_if<zero_array>(&form_)}) {
            return &zero->prefix;
        }

        return nullptr;
    }

    // the copy size comes from the source, so its length is that of the
    // destination, 'array_count' when known
    auto assert_whole_array_copy_fits(
        const toc& tc, const std::optional<size_t> array_count) const -> void {

        const ident_info src_info{make_ident_info(tc, *operand_path())};

        // note: the plan of the source checked that it is an array
        assert(src_info.is_array());

        const std::optional<size_t> src_len{src_info.array_len()};

        if (not array_count or not src_len or *array_count == *src_len) {
            return;
        }

        throw compile_error{
            tok(), std::format("destination array size {} does not match "
                               "source size {}",
                               *array_count, *src_len)};
    }

    // the little endian bytes of the elements, empty when an element is not a
    // constant or there are no elements to store
    [[nodiscard]] auto constant_bytes(const toc& tc, const element_list& list,
                                      const type& element_type) const
        -> std::optional<std::string> {

        if (list.elements.empty()) {
            return std::nullopt;
        }

        std::string bytes;
        for (size_t index{}; index < list.elements.size(); ++index) {
            // a condition is compiled, not packed, so the elements after the
            // one that decides it are checked, e.g. the 'a / 0 == 1' of
            // 'true or a / 0 == 1'
            if (std::holds_alternative<expr_bool>(
                    list.elements.at(index).value)) {

                return std::nullopt;
            }

            // e.g. '{300}' for 'i8' is rejected as when stored alone
            const std::optional<int64_t> value{
                folded_value::value_of(
                    fold_element(tc, index, tc.use_at(element_type))),
            };

            if (not value) {
                return std::nullopt;
            }

            const uint64_t bits{static_cast<uint64_t>(*value)};
            for (size_t i{}; i < element_type.size_bytes(); ++i) {
                bytes +=
                    static_cast<char>(bits >> (machine::bits_per_byte * i));
            }
        }

        return bytes;
    }

    // e.g. 'i8[3]' in 'i8[3]{1, 2}' names the element type that '{1, 2}' takes
    // from the destination, and the size unless it is 'i8[]'; '[3]{1, 2}' has
    // the default type; none when the destination gives both
    [[nodiscard]] auto parse_array_prefix(toc& tc, tokenizer& tz,
                                          const type& tp,
                                          const literal_typing typing)
        -> std::optional<array_prefix> {

        const token tk{tz.next_token()};

        const bool is_typed{
            classify_term(tc, tk, tz) == term_kind::array_literal,
        };

        if (not is_typed) {
            tz.put_back_token(tk);
        }

        if (not is_typed and not is_default_array_literal(tz)) {
            return std::nullopt;
        }

        // note: an array literal is a type name or nothing before a '['
        const token open_bracket_tk{tz.seen_char_token('[')};

        const type& literal_type{
            is_typed ? tc.get_type_or_throw(tk) : tc.builtins().default_type(),
        };

        if (not literal_type.is_same(tp)) {
            throw_type_mismatch(is_typed ? tk : open_bracket_tk, tp.name(),
                                literal_type.name());
        }

        // e.g. the '4' of 'i8[4]', none for 'i8[]'
        stmt_array_size size{tc, tz};

        const token close_bracket_tk{
            tz.expect_char_token(']', "expected ']' after array size"),
        };

        array_prefix prefix{
            .element_type_tk{
                is_typed ? std::optional<token>{tk} : std::nullopt,
            },
            .open_bracket_tk{open_bracket_tk},
            .size{std::move(size)},
            .close_bracket_tk{close_bracket_tk},
        };

        // e.g. 'i8[]{1, 2}' for an 'i8' array field is '{1, 2}' and 'i8[4]'
        // is '{}'
        if (typing == literal_typing::from_destination) {
            throw_known_type(is_typed ? tk : open_bracket_tk,
                             std::format("{}[]", literal_type.name()),
                             tz.peek_char_after_whitespace() == '{' ? "{...}"
                                                                    : "{}");
        }

        if (not prefix.size.has_value()) {
            return prefix;
        }

        const size_t count{prefix.size.array_size()};

        // note: only a declaration names the array type, its size is the one
        //       of this literal

        assert(not array_count_ or *array_count_ == count);

        array_count_ = count;

        return prefix;
    }

    // the elements between the braces, e.g. '1, 2, 3' of '{1, 2, 3}'
    auto parse_braced_elements(toc& tc, tokenizer& tz, const type& tp,
                               element_list& list) -> void {

        std::optional<token> close_brace_tk{tz.is_next_char_token('}')};

        while (not close_brace_tk) {
            // a list of a known length ends when it is full
            if (array_count_ and list.elements.size() == *array_count_) {
                close_brace_tk = read_full_list_end(
                    tz, list.delims_tk, too_many_elements(*array_count_),
                    std::format("expected '}}' to close the array of size {}",
                                *array_count_));

                break;
            }

            if (not list.elements.empty()) {
                read_element_delimiter(tz, tp, list);

                // a trailing ',' lets each element end its line the same way
                close_brace_tk = tz.is_next_char_token('}');

                if (close_brace_tk) {
                    break;
                }
            }

            // note: the whitespace is read first, the initializers run in
            //       order

            // the array gives the type of its elements
            list.elements.push_back({
                .ws_tk{tz.next_whitespace_token()},
                .value{
                    parse_variant(tc, tz, tp, literal_typing::from_destination),
                },
            });

            close_brace_tk = tz.is_next_char_token('}');
        }

        list.close_brace_tk = *close_brace_tk;

        if (not array_count_) {
            array_count_ = list.elements.size();
        }
    }

    // the form 'destination_' calls for; an array form sets 'array_count_'
    // note: called by the constructor after 'destination_' and
    //       'array_count_' are made
    [[nodiscard]] auto parse_form(toc& tc, tokenizer& tz, const type& tp,
                                  const literal_typing typing) -> form_variant {

        // the basic case, e.g. 'x + 1' or 'point{1, 2}'
        if (not destination_.is_array()) {
            return single_value{
                .value{parse_variant(tc, tz, tp, typing)},
            };
        }

        // array, e.g. 'i8[]{1, 2}' or 'a' of 'var b = a'

        // e.g. "hello" fills the start of an 'i8' array
        if (tz.is_peek_char('"')) {
            const token string_tk{tz.next_token()};

            array_count_ = string_array_count(tc.builtins().i8(), string_tk, tp,
                                              array_count_);

            return string_array{
                .string_tk{string_tk},
            };
        }

        // an array is initialized by a '{ ... }' or by the name of an array,
        // e.g. 'str.data'

        std::optional<array_prefix> prefix{
            parse_array_prefix(tc, tz, tp, typing),
        };

        const std::optional<token> open_brace_tk{tz.is_next_char_token('{')};

        if (not open_brace_tk and prefix) {
            // e.g. 'var a = i8[4]' is 'i8[4]{}'
            if (not prefix->size.has_value()) {
                throw compile_error{
                    prefix->close_bracket_tk,
                    std::format("an array needs a constant size, e.g. "
                                "'{}[4]'",
                                prefix->element_type_tk
                                    ? prefix->element_type_tk->text()
                                    : "")};
            }

            return zero_array{
                .prefix{std::move(*prefix)},
            };
        }

        if (not open_brace_tk) {
            // 'expr_type' copies the whole array as bytes, 'expr_arith' would
            // read a single scalar
            // e.g. 'var b = a' copies the array 'a'
            return single_value{
                .value{
                    expr_type{
                        tc,
                        tz,
                        tp,
                        expr_type::destination_kind::whole_array,
                        typing,
                    },
                },
            };
        }

        // e.g. the '{1, 2, 3}' of 'var a = i8[]{1, 2, 3}'
        element_list list{
            .prefix{std::move(prefix)},
            .open_brace_tk{*open_brace_tk},
            .elements{},
            .delims_tk{},
            .close_brace_tk{},
        };

        parse_braced_elements(tc, tz, tp, list);

        return list;
    }

    // an instance literal is written field by field, any other value at once
    auto plan_element(const toc& tc, const size_t index,
                      const part_location& at, write_plan& plan) const -> void {

        if (const expr_type* const record{record_at(index)}) {
            record->plan_writes(tc, at, plan);
            return;
        }

        plan.push_back({
            .at{at},
            .action{
                value_write{
                    .item{&element(index)},
                },
            },
            .covered{at.path()},
        });
    }

    // constant elements are stored like a string so the backend can pack them
    // into wider immediates or copy them from read-only data
    auto plan_elements(const toc& tc, const element_list& list,
                       const part_location& at, write_plan& plan) const
        -> void {

        if (std::optional<std::string> bytes{
                constant_bytes(tc, list, get_type()),
            }) {

            plan.push_back({
                .at{at},
                .action{
                    bytes_write{
                        .src_loc_tk{list.open_brace_tk},
                        .bytes{std::move(*bytes)},
                        .is_string{},
                    },
                },
                .covered{
                    {at.elements_path(0, list.elements.size())},
                },
            });

            return;
        }

        for (size_t i{}; i < list.elements.size(); ++i) {
            plan.push_back({
                .at{at},
                .action{
                    write_comment{
                        .src_loc_tk{element(i).tok()},
                        .text{std::format("[{}]", i)},
                    },
                },
                .covered{},
            });

            // a call element writes at the element, not at the array start
            // the identifier names, and an element is not an array
            plan_element(tc, i, at.element(i), plan);
        }
    }

    // unlisted elements are zero like unlisted fields, the first
    // 'listed_count' of the array at 'at' are written
    auto plan_remaining_elements(const token& src_loc_tk,
                                 const part_location& at,
                                 const size_t listed_count,
                                 const size_t array_count,
                                 write_plan& plan) const -> void {

        const type& element_type{get_type()};

        const size_t remaining_count{array_count - listed_count};

        if (remaining_count == 0) {
            return;
        }

        const size_t size_bytes{
            product_storage_size(element_type.size_bytes(), remaining_count),
        };

        plan.push_back({
            .at{at},
            .action{
                zero_fill{
                    .src_loc_tk{src_loc_tk},
                    .comment{
                        std::format("zero remaining elements: {} * {} B = "
                                    "{} B",
                                    remaining_count, element_type.size_bytes(),
                                    size_bytes),
                    },
                    .offset{listed_count * element_type.size_bytes()},
                    .value_type{&element_type},
                    .size_bytes{size_bytes},
                    .alignment{element_type.alignment()},
                },
            },
            .covered{
                {at.elements_path(listed_count, array_count)},
            },
        });
    }

    // the string is stored and the rest of the array is zeroed like unlisted
    // elements; 'array_count' is empty when the size is not known
    auto plan_string(const token& string_tk, const part_location& at,
                     const std::optional<size_t> array_count,
                     write_plan& plan) const -> void {

        std::string bytes{string_tk.string_bytes()};

        const size_t size_bytes{bytes.size()};

        // the parser could not check the size of an unsized destination
        if (array_count and size_bytes > *array_count) {
            throw compile_error{string_tk,
                                string_overflows(size_bytes, *array_count)};
        }

        // an empty string has nothing to store
        if (size_bytes != 0) {
            plan.push_back({
                .at{at},
                .action{
                    bytes_write{
                        .src_loc_tk{string_tk},
                        .bytes{std::move(bytes)},
                        .is_string{true},
                    },
                },
                .covered{at.path()},
            });
        }

        // the bytes after the string are not known
        if (not array_count) {
            return;
        }

        plan_remaining_elements(string_tk, at, size_bytes, *array_count, plan);
    }

    // the element at 'index' when it is an 'expr_type', null otherwise
    [[nodiscard]] auto record_at(const size_t index) const -> const expr_type* {
        return std::get_if<expr_type>(&value_at(index));
    }

    // the value of a single value or of an element of a list
    [[nodiscard]] auto value_at(const size_t index) const
        -> const expr_variant& {

        if (const single_value* const single{
                std::get_if<single_value>(&form_),
            }) {

            assert(index == 0);

            return single->value;
        }

        return std::get<element_list>(form_).elements.at(index).value;
    }

    //
    // statics
    //

    // every kind of element is a 'statement'; the only 'visit' of the variant
    [[nodiscard]] static auto as_statement(const expr_variant& value)
        -> const statement& {

        return value.visit(
            [](const auto& item) -> const statement& { return item; });
    }

    // a number destination takes a number: not a 'bool', 'int(b)' converts
    // it, and not an instance, whatever the value, e.g. 'c', 'f()' or
    // 'arrays_equal(a, b, n)'; at the parse, the one point every value passes
    // (README 7.13); the width is checked where the value is compiled
    static auto assert_number_kind(const toc& tc, const expr_arith& value,
                                   const type& tp) -> void {

        const type& value_type{value.get_type()};

        if (value_type.is_builtin() and not value_type.is_bool()) {
            return;
        }

        // a whole array is an argument of an array parameter, e.g. the 'a' of
        // 'f(a)'; its element type is checked where the call is compiled
        if (const std::optional<place> array{as_place(tc, value)};
            array and array->info.is_array()) {

            return;
        }

        // e.g. the name 'f' of the call 'f()'
        const token& value_tk{
            value.is_single_operand() ? value.single_operand().tok()
                                      : value.tok(),
        };

        if (not value_type.is_bool()) {
            throw_type_mismatch(value_tk, tp.name(), value_type.name());
        }

        throw compile_error{
            value_tk,
            std::format("expected type {}, got 'bool', use '{}(...)'",
                        in_quotes(tp.name()), tp.name()),
        };
    }

    // a built-in value whose only element is parsed, e.g. the zero value of
    // 'i8', 'bool' or '{}'
    [[nodiscard]] static auto builtin_value(toc& tc, tokenizer& tz,
                                            std::unique_ptr<statement> first)
        -> expr_variant {

        return expr_arith{tc, tz,
                          expr_arith::options::implied(list_start_precedence,
                                                       std::move(first),
                                                       nullptr)};
    }

    // compiled as a branch condition, which needs no register for its value
    static auto check_folded_condition(toc& tc, const size_t indent,
                                       const expr_bool& condition) -> void {

        tc.check_skipped([&] -> void {
            // note: two targets as an 'if', the same one for both would store
            //       the value

            const std::string true_label{
                tc.create_unique_label(condition.tok(), "bool"),
            };

            const std::string false_label{toc::end_label(true_label)};

            std::ignore = condition.compile(tc, indent,
                                            {
                                                .if_false{false_label},
                                                .if_true{true_label},
                                            },
                                            {});

            machine& x{tc.machine()};

            x.label(indent, true_label);
            x.label(indent, false_label);
        });
    }

    static auto check_folded_conditions_of(toc& tc, const size_t indent,
                                           const expr_variant& value) -> void {

        if (const expr_bool* const condition{std::get_if<expr_bool>(&value)}) {
            check_folded_condition(tc, indent, *condition);
            return;
        }

        const expr_type* const instance{std::get_if<expr_type>(&value)};

        if (instance == nullptr or not instance->is_literal()) {
            return;
        }

        for (const std::unique_ptr<expr_any>& field :
             instance->field_values()) {

            field->check_folded_conditions(tc, indent);
        }
    }

    // an array literal that no operator continues, e.g. 'i8[2]{1, 2}' or
    // '[2]', not '[2]{1, 2} == a'; 'tz' is at its '['
    [[nodiscard]] static auto is_whole_array_literal(const tokenizer& tz)
        -> bool {

        tokenizer after_literal{tz};

        // note: a literal the source ends in is rejected as an operand
        if (not after_literal.skip_enclosed('[', ']')) {
            return false;
        }

        if (after_literal.peek_char_after_whitespace() == '{' and
            not after_literal.skip_braced_block()) {

            return false;
        }

        return not continues_expression(after_literal);
    }

    // a user type alone or an instance literal that no operator continues,
    // e.g. 'point' or 'point{1, 2}', not 'point{1, 2} == p'
    [[nodiscard]] static auto is_whole_record(const term_kind tm,
                                              const tokenizer& tz) -> bool {

        if (tm == term_kind::record_type) {
            return not continues_expression(tz);
        }

        if (tm != term_kind::record_literal) {
            return false;
        }

        tokenizer after_literal{tz};

        // note: a literal the source ends in is rejected as an operand
        if (not after_literal.skip_braced_block()) {
            return false;
        }

        return not continues_expression(after_literal);
    }

    [[nodiscard]] static auto parse_variant(toc& tc, tokenizer& tz,
                                            const type& tp,
                                            const literal_typing typing)
        -> expr_variant {

        // an instance value, e.g. 'point{1, 2}' or 'p'
        if (not tp.is_builtin()) {
            return expr_type{tc, tz, tp, expr_type::destination_kind::instance,
                             typing};
        }

        // e.g. 'n = {}' is 'n = 0' and 'b = {}' is 'b = false'
        if (const std::optional<token> open_brace_tk{
                tz.is_next_char_token('{'),
            }) {

            const token close_brace_tk{
                tz.expect_char_token(
                    '}', "expected '}', the zero value of a built-in is '{}'"),
            };

            // e.g. the '+' of 'n = {} + 1'
            if (continues_expression(tz)) {
                throw compile_error{
                    tz, "'{}' is a whole value, an operator cannot follow it"};
            }

            return builtin_value(tc, tz,
                                 std::make_unique<expr_zero>(tp, *open_brace_tk,
                                                             close_brace_tk));
        }

        // e.g. 'var x = i8' is 'var x = i8(0)' and 'var b = bool' is
        // 'var b = false'
        const token tk{tz.next_token()};

        if (is_bare_builtin_type(tc, tk, tz)) {
            if (typing == literal_typing::from_destination) {
                // note: compared by type, a type parameter names the type of
                //       its argument

                const type& named_type{tc.get_type_or_throw(tk)};

                if (not named_type.is_same(tp)) {
                    throw_type_mismatch(tk, tp.name(), named_type.name());
                }

                throw_known_type(tk, tk.text(), "{}");
            }

            return builtin_value(
                tc, tz,
                std::make_unique<expr_zero>(tc.get_type_or_throw(tk), tk));
        }

        // e.g. 'x = point' or 'x = point{1, 2}' for an 'i8' 'x', as 'x = p' of
        // a 'point' 'p'; a condition may compare an instance literal, which the
        // comparison rejects, e.g. 'b = point{1, 2} == p'
        const term_kind tm{classify_term(tc, tk, tz)};

        if (is_whole_record(tm, tz) or
            (tm == term_kind::record_literal and not tp.is_bool())) {

            throw_type_mismatch(tk, tp.name(), tc.get_type_or_throw(tk).name());
        }

        // e.g. 'x = i8[2]{1, 2}' or 'x = [2]{1, 2}' for an 'i8' 'x', like an
        // instance literal; an operand or a compared literal is rejected as
        // an operand, e.g. 'b = [2]{1, 2} == a'
        const bool is_array_literal{
            tm == term_kind::array_literal or
                (tk.is_none() and is_default_array_literal(tz)),
        };

        if (is_array_literal and
            (not tp.is_bool() or is_whole_array_literal(tz))) {

            const type& element_type{
                tk.is_none() ? tc.builtins().default_type()
                             : tc.get_type_or_throw(tk),
            };

            throw_type_mismatch(tk, tp.name(),
                                std::format("{}[]", element_type.name()));
        }

        tz.put_back_token(tk);

        // a generic call takes the type of a known destination for its
        // result, e.g. 'x = make()'
        if (not tp.is_bool() or (typing == literal_typing::from_destination and
                                 is_whole_generic_call(tc, tz))) {

            const type* const destination_type{
                typing == literal_typing::from_destination ? &tp : nullptr,
            };

            expr_arith value{tc, tz,
                             expr_arith::options::plain(destination_type)};

            if (not tp.is_bool()) {
                assert_number_kind(tc, value, tp);
            }

            // e.g. the '>' of 'int(a > 2)', which would end the number
            expr_arith::assert_no_condition_next(tz);

            return value;
        }

        expr_bool value{tc, tz.next_whitespace_token(), tz};

        value.assert_boolean_operands();

        // one operand is compiled like a value of another type, a name is
        // passed where it is stored, e.g. the 'x' of 'f(x)' that a 'mut bool'
        // parameter writes, a call binds its result to the destination
        if (std::optional<expr_arith> operand{value.release_operand()}) {
            return std::move(*operand);
        }

        return value;
    }

    static auto read_element_delimiter(tokenizer& tz, const type& tp,
                                       element_list& list) -> void {

        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by a value of type {}",
                                 in_quotes(tp.name()))),
        };

        list.delims_tk.emplace_back(delimiter_tk);
    }

    // the size of an 'i8' array that a string fills: 'array_count', or the
    // size of the string when the destination gives none
    [[nodiscard]] static auto
    string_array_count(const type& string_element_type, const token& string_tk,
                       const type& element_type,
                       const std::optional<size_t> array_count) -> size_t {

        const size_t size_bytes{string_tk.string_bytes().size()};

        if (not element_type.is_same(string_element_type)) {
            throw compile_error{
                string_tk, std::format("a string fills only an array of "
                                       "{}, not an array of {}",
                                       in_quotes(string_element_type.name()),
                                       in_quotes(element_type.name()))};
        }

        // note: an empty string that would give a declaration its size is
        //       rejected there, e.g. 'var s = ""' ('initializer_declaration')

        if (not array_count) {
            return size_bytes;
        }

        if (size_bytes > *array_count) {
            throw compile_error{string_tk,
                                string_overflows(size_bytes, *array_count)};
        }

        return *array_count;
    }

    // the parse checks a string for a known length ('string_array_count'),
    // the plan one for an unsized destination ('plan_string')
    [[nodiscard]] static auto string_overflows(const size_t size_bytes,
                                               const size_t array_count)
        -> std::string {

        return std::format("string size {} overflows array size {}", size_bytes,
                           array_count);
    }

    // the parse checks a list of a known length when it is full
    // ('read_full_list_end'), the plan one of an unsized destination, whose
    // length the call gives
    [[nodiscard]] static auto too_many_elements(const size_t array_count)
        -> std::string {

        return std::format("too many elements for array of size {}",
                           array_count);
    }
};

// e.g. 'i8[-]' is not an expression and 'i8[p]' with 'p' a parameter is not a
// constant; the size is parsed like an index, but it counts storage and is no
// value of the program, so it folds at 'i64' on every target, e.g. 'dat d =
// i8[3000000000]' on rv32i, which the address range limits
inline stmt_array_size::stmt_array_size(toc& tc, tokenizer& tz)
    : statement{tz.next_whitespace_token()} {

    set_type(tc.builtins().default_type());

    if (tz.is_peek_char(']')) {
        return;
    }

    value_ = std::make_unique<expr_any>(tc, tz, tc.builtins().default_type(),
                                        literal_typing::from_destination);

    const std::optional<int64_t> folded{
        folded_value::value_of(
            value_->fold_checked(tc, tc.use_at(tc.builtins().i64()))),
    };

    if (not folded) {
        throw compile_error{
            value_->tok(),
            std::format("{} is not a constant",
                        in_quotes(statement::trimmed_source(*value_)))};
    }

    if (*folded < 1) {
        throw compile_error{value_->tok(),
                            "expected a constant array size greater than 0"};
    }

    folded_ = *folded;
}

inline stmt_array_size::stmt_array_size(stmt_array_size&&) noexcept = default;

inline auto stmt_array_size::operator=(stmt_array_size&&) noexcept
    -> stmt_array_size& = default;

inline stmt_array_size::~stmt_array_size() = default;

inline auto stmt_array_size::source_to(std::ostream& os) const -> void {
    statement::source_to(os);

    if (value_) {
        value_->source_to(os);
    }
}

inline auto stmt_array_size::array_size() const -> size_t {
    assert(value_);

    return static_cast<size_t>(folded_);
}
