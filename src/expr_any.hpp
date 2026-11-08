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

// a value at the type of its destination, the types below in reading order:
//
//   'stmt_array_size'  the constant size of an array, an expression folded
//                      when parsed
//   'expr_any'         the value of an initializer, an argument or a field:
//                      a number, a condition, an instance or an array form

class expr_any;

// the constant size of an array, e.g. the '4' of 'i8[4]' or the 'n * 2' of a
// field 'i8[n * 2]': an expression at the default type, folded by 'fold' when
// parsed and never compiled; empty for 'i8[]'
// note: its members are defined after 'expr_any', which holds one, where a
//       'unique_ptr' of 'expr_any' is complete
class stmt_array_size final : public statement {
    std::unique_ptr<expr_any> value_;
    int64_t folded_{};

  public:
    stmt_array_size(toc& tc, tokenizer& tz);

    stmt_array_size(stmt_array_size&&) noexcept;

    stmt_array_size(const stmt_array_size&) = delete;
    auto operator=(const stmt_array_size&) -> stmt_array_size& = delete;
    auto operator=(stmt_array_size&&) -> stmt_array_size& = delete;

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

    // folds the size again where its value is written, whose checks see what
    // a call gives, e.g. the length of the unsized array parameter 'x' of
    // 'i8[array_length(x[5].a)]'
    auto check_where_written(const toc& tc) const -> void;

    [[nodiscard]] auto has_value() const -> bool { return value_ != nullptr; }

  private:
    //
    // statics
    //

    // a type alone is no size, e.g. the 'int' of 'i8[int]' or of
    // 'i8[int{2}]', not of the conversion 'i8[int(2)]'
    static auto assert_size_not_type(const toc& tc, const tokenizer& tz)
        -> void;
};

// the value of an initializer, an argument or a field, parsed at the type of
// its destination; one of the forms below, an array form only where the
// destination is an array
class expr_any final : public statement {
    using expr_variant =
        std::variant<expr_arith, expr_bool, expr_type, expr_path>;

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

    // a value or an instance literal; where the destination is an array, the
    // array copied whole as bytes, e.g. the 'a' of 'var b = a'
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

    // e.g. 'i8[]{1, 2}', '{{1, 2}, {3, 4}}' or 'i8[4]', which is 'i8[4]{}'
    struct element_list {
        // none when the destination gives the type and the size
        std::optional<array_prefix> prefix;
        // none without braces, then 'prefix' gives the size
        std::optional<token> open_brace_tk;
        std::vector<listed_element> elements;
        std::vector<token> delims_tk;
        std::optional<token> close_brace_tk;
    };

    using form_variant = std::variant<single_value, string_array, element_list>;

    // what the destination gives, e.g. an array parameter 's[]' is an array
    // of unknown length, its argument gives the length when it is compiled
    array_shape destination_;
    form_variant form_;

  public:
    // a single value, or an array when 'destination' is one; where 'typing' is
    // 'written' a type or array may be written without '{}', e.g. 'var p =
    // point' and 'var a = i8[4]'
    expr_any(toc& tc, tokenizer& tz, const type& tp,
             const literal_typing typing, const array_shape destination = {})
        : statement{tz.next_whitespace_token()}, destination_{destination},
          form_{parse_form(tc, tz, tp, typing)} {

        set_type(tp);

        // a copy checks its end itself
        if (is_array() and not is_whole_array_copy()) {
            assert_value_ends(tz, array_token(), value_kind::array, tp);
        }
    }

    // a path parsed apart, e.g. a method receiver or the array argument of a
    // built-in
    expr_any(const token pos_tk, expr_path path)
        : statement{pos_tk}, form_{single_value{
                                 .value{std::move(path)},
                             }} {

        set_type(element(0).get_type());
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

        const element_list* const list{std::get_if<element_list>(&form_)};

        if (list == nullptr) {
            element(0).source_to(os);
            return;
        }

        if (list->prefix) {
            list->prefix->source_to(os);
        }

        token::source_to(os, list->open_brace_tk);

        source_delimited_to(os, list->elements, list->delims_tk);

        token::source_to(os, list->close_brace_tk);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // note: an array literal is written by its assignment or definition
        //       ('plan_writes'), the parse rejects one as an argument

        assert(not is_array() or is_whole_array_copy());

        write_plan plan;
        plan_writes(tc, part_location{}, dst.array_len(), plan);
        emit_write_plan(tc, indent, plan, dst);
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        // note: an array is written by its plan, which checks each element
        //       ('emit_write_plan')

        assert(not is_array());

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
        // note: an array literal is written by its elements, nothing asks it
        //       for a path

        assert(std::holds_alternative<single_value>(form_));

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
                .index_only_place{},
            });
        }
    }

    //
    // class methods
    //

    // the size of an array form: the length of the destination, else the size
    // the literal gives, e.g. 3 of 'i8[]{1, 2, 3}'; empty when neither gives
    // one, as for a copy to an unsized destination
    [[nodiscard]] auto array_count() const -> std::optional<size_t> {
        if (const std::optional<size_t> length{destination_.length()}) {
            return length;
        }

        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            return str->string_tk.string_bytes().size();
        }

        const element_list* const list{std::get_if<element_list>(&form_)};

        if (list == nullptr) {
            return std::nullopt;
        }

        // note: a list without a written size has braces
        return written_size(*list).value_or(list->elements.size());
    }

    // where an array is written: its element type, its '[' when the type is
    // the default one, its '{' when the destination gives the type, else its
    // value, e.g. a string
    [[nodiscard]] auto array_token() const -> const token& {
        if (const array_prefix* const prefix{array_prefix_of()}) {
            if (prefix->element_type_tk) {
                return *prefix->element_type_tk;
            }

            return prefix->open_bracket_tk;
        }

        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            // note: a list without a prefix has braces
            assert(list->open_brace_tk);

            return *list->open_brace_tk;
        }

        return tok();
    }

    // the element at 'index' of an instance value or of an array of
    // instances
    [[nodiscard]] auto as_expr_type(const size_t index) const
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

    // none for a string
    [[nodiscard]] auto element_count() const -> size_t {
        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            return list->elements.size();
        }

        if (std::holds_alternative<string_array>(form_)) {
            return 0;
        }

        return 1;
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
        const std::optional<size_t> dst_count{
            destination_.is_length_unknown() ? unsized_count : array_count(),
        };

        // the parse checked that the source of an array copy is an array,
        // its length is checked here
        if (std::holds_alternative<single_value>(form_)) {
            plan_element(tc, 0, at, plan);

            if (is_array()) {
                assert_whole_array_copy_fits(tc, dst_count);
            }

            return;
        }

        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            plan_string(str->string_tk, at, dst_count, plan);
            return;
        }

        if (const array_prefix* const prefix{array_prefix_of()}) {
            prefix->size.check_where_written(tc);
        }

        const size_t count{element_count()};

        // the parser could not check the size of an unsized destination
        if (dst_count and count > *dst_count) {
            throw compile_error{element(*dst_count).tok(),
                                too_many_elements(*dst_count)};
        }

        plan_elements(tc, std::get<element_list>(form_), at, plan);

        if (not dst_count) {
            return;
        }

        plan_remaining_elements(at, count, *dst_count, plan);
    }

    // where a literal written without a size gives one, e.g. the '[' of
    // 'i8[]{}' or the string '""'
    [[nodiscard]] auto size_token() const -> const token& {
        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            return str->string_tk;
        }

        const array_prefix* const prefix{array_prefix_of()};

        // note: only a declaration that sees a string or an array literal
        //       asks ('initializer_declaration::deduce_declaration')

        assert(prefix != nullptr);

        return prefix->open_bracket_tk;
    }

  private:
    // the prefix of an element list, null when it has none or for another
    // form
    [[nodiscard]] auto array_prefix_of() const -> const array_prefix* {
        const element_list* const list{std::get_if<element_list>(&form_)};

        if (list == nullptr or not list->prefix) {
            return nullptr;
        }

        return &*list->prefix;
    }

    // the copy size comes from the source, so its length is that of the
    // destination, 'array_count' when known
    auto assert_whole_array_copy_fits(
        const toc& tc, const std::optional<size_t> array_count) const -> void {

        const ident_info src_info{make_ident_info(tc, *operand_path())};

        // note: the parse checked that the source is an array
        //       ('expr_path::parse_whole_array')

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
                    element(index).fold_checked(tc, tc.use_at(element_type))),
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

        // the element type, or the '[' of the default type
        const token& literal_tk{is_typed ? tk : open_bracket_tk};

        if (not literal_type.is_same(tp)) {
            throw_type_mismatch(literal_tk, tp.name(), literal_type.name());
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
            const std::optional<size_t> length{destination_.length()};

            const std::string length_text{
                length ? std::format("{}", *length) : "",
            };

            throw_known_literal_type(
                tz, literal_tk, value_kind::array, tp,
                std::format("{}[{}]", literal_type.name(), length_text));
        }

        // note: only a declaration names the array type, its size is the one
        //       of this literal

        assert(not prefix.size.has_value() or not destination_.length() or
               *destination_.length() == prefix.size.array_size());

        return prefix;
    }

    // the form 'destination_' calls for
    // note: called by the constructor after 'destination_' is made
    [[nodiscard]] auto parse_form(toc& tc, tokenizer& tz, const type& tp,
                                  const literal_typing typing) -> form_variant {

        // the basic case, e.g. 'x + 1' or 'point{1, 2}'
        if (not destination_.is_array()) {
            return single_value{
                .value{parse_variant(tc, tz, tp, typing)},
            };
        }

        // array, e.g. 'i8[]{1, 2}' or 'a' of 'var b = a'

        // e.g. '"hello"' fills the start of an 'i8' array
        if (tz.peek_char_after_whitespace() == '"') {
            const token string_tk{tz.next_token()};

            assert_string_fits(tc.builtins().i8(), string_tk, tp,
                               destination_.length());

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

        // e.g. 'var a = i8[]', which lists nothing and gives no size
        if (not open_brace_tk and prefix and not prefix->size.has_value()) {
            throw compile_error{
                prefix->close_bracket_tk,
                std::format("an array needs a constant size, e.g. '{}[4]'",
                            prefix->element_type_tk
                                ? prefix->element_type_tk->text()
                                : "")};
        }

        if (not open_brace_tk and not prefix) {
            // 'expr_path' copies the whole array as bytes, 'expr_arith' would
            // read a single scalar
            // e.g. 'var b = a' copies the array 'a'
            return single_value{
                .value{expr_path::parse_whole_array(tc, tz, tp)},
            };
        }

        // e.g. the '{1, 2, 3}' of 'var a = i8[]{1, 2, 3}', or none of 'var a =
        // i8[4]'
        element_list list{
            .prefix{std::move(prefix)},
            .open_brace_tk{open_brace_tk},
            .elements{},
            .delims_tk{},
            .close_brace_tk{},
        };

        if (open_brace_tk) {
            const std::optional<size_t> length{
                destination_.length() ? destination_.length()
                                      : written_size(list),
            };

            parse_braced_elements(tc, tz, tp, length, list);
        }

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

            // note: only a list in braces has elements to pack
            assert(list.open_brace_tk);

            plan.push_back({
                .at{at},
                .action{
                    bytes_write{
                        .src_loc_tk{*list.open_brace_tk},
                        .bytes{std::move(*bytes)},
                        .constant_kind{"init"},
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
    auto plan_remaining_elements(const part_location& at,
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
                    .src_loc_tk{tok()},
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
                        .constant_kind{"str"},
                    },
                },
                .covered{at.path()},
            });
        }

        // the bytes after the string are not known
        if (not array_count) {
            return;
        }

        plan_remaining_elements(at, size_bytes, *array_count, plan);
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

    // a record or array literal where the built-in 'tp' is expected, also
    // one reached into, e.g. 'pt{1, 2}.x'
    static auto assert_not_aggregate(const toc& tc, const tokenizer& tz,
                                     const type& tp, const token& tk) -> void {

        // a user type or an instance literal, e.g. 'x = point' or
        // 'x = point{1, 2}' for an 'i8' 'x'; a condition may compare an
        // instance literal, which the comparison rejects
        const term_kind tm{classify_term(tc, tk, tz)};

        if (tm == term_kind::record_literal and is_literal_reached_into(tz)) {
            throw_literal_not_operand(
                tk,
                std::format("an instance literal of {}", in_quotes(tk.text())));
        }

        if (is_record_value(tm, tz) or
            (tm == term_kind::record_literal and not tp.is_bool())) {

            throw_type_mismatch(tk, tp.name(), tc.get_type_or_throw(tk).name());
        }

        // e.g. 'x = i8[2]{1, 2}' or 'x = [2]{1, 2}' for an 'i8' 'x', like an
        // instance literal
        assert_not_array_literal(tc, tz, tk, tm, tp);
    }

    // a number destination takes a number: not a 'bool', 'int(b)' converts
    // it, and not an instance, whatever the value, e.g. 'c', 'f()' or
    // 'arrays_equal(a, b, n)'; at the parse, the one point every value passes
    // (README 7.13); the width is checked where the value is compiled
    static auto assert_number_kind(const expr_arith& value, const type& tp)
        -> void {

        // a whole array is reported as one and not by its element type, e.g.
        // the 'cs' of 'x = cs' with 'cs' an array of instances
        assert_not_whole_array(value);

        const type& value_type{value.get_type()};

        if (value_type.is_builtin() and not value_type.is_bool()) {
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

    // a string fills only an 'i8' array, one of 'array_count' when known
    static auto assert_string_fits(const type& string_element_type,
                                   const token& string_tk,
                                   const type& element_type,
                                   const std::optional<size_t> array_count)
        -> void {

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

        if (not array_count or size_bytes <= *array_count) {
            return;
        }

        throw compile_error{string_tk,
                            string_overflows(size_bytes, *array_count)};
    }

    // e.g. the '.x' of 'i8{}.x', as of '1.x'; 'tz' is after the value
    static auto assert_zero_not_reached_into(const tokenizer& tz) -> void {
        if (is_reached_into(tz)) {
            throw compile_error{
                tz, "a zero value is a constant, it has no field or element"};
        }
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

            std::ignore = condition.compile_branches(tc, indent,
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

        if (instance == nullptr) {
            return;
        }

        // note: only a 'dat' folds an instance and it holds literals
        //       ('stmt_def_dat::fold_data')

        assert(instance->is_literal());

        for (const std::unique_ptr<expr_any>& field :
             instance->field_values()) {

            field->check_folded_conditions(tc, indent);
        }
    }

    // the elements between the braces, e.g. '1, 2, 3' of '{1, 2, 3}'; a list
    // of a known length ends when it is full
    static auto parse_braced_elements(toc& tc, tokenizer& tz, const type& tp,
                                      const std::optional<size_t> length,
                                      element_list& list) -> void {

        std::optional<full_list> full;

        if (length) {
            full = full_list{
                .capacity{*length},
                .too_many{too_many_elements(*length)},
                .unclosed{
                    std::format("expected '}}' to close the array of size {}",
                                *length),
                },
            };
        }

        list.close_brace_tk = parse_braced_items(
            tz, list.delims_tk, full,
            [&tp]([[maybe_unused]] const size_t index) -> std::string {
                return std::format(
                    "expected ',' followed by a value of type {}",
                    in_quotes(tp.name()));
            },
            [&]([[maybe_unused]] const size_t index) -> void {
                // note: the whitespace is read first, the initializers run in
                //       order

                // the array gives the type of its elements
                list.elements.push_back({
                    .ws_tk{tz.next_whitespace_token()},
                    .value{
                        parse_variant(tc, tz, tp,
                                      literal_typing::from_destination),
                    },
                });
            });
    }

    // e.g. 'var x = i8' is 'var x = i8(0)' and 'var b = bool' is
    // 'var b = false'
    [[nodiscard]] static auto
    parse_type_as_zero(toc& tc, tokenizer& tz, const type& tp,
                       const literal_typing typing, const token& tk)
        -> expr_variant {

        // e.g. the '.x' of 'i8{}.x'; 'classify_term' left no '.' or '['
        // after a bare type
        if (is_zero_value(tz)) {
            tokenizer after_value{tz};
            std::ignore = after_value.skip_braced_block();
            assert_zero_not_reached_into(after_value);
        }

        if (typing == literal_typing::from_destination) {
            assert_names_type(tc, tk, tp);
            throw_known_type(tk, tp.name(), "{}");
        }

        return builtin_value(
            tc, tz, std::make_unique<expr_zero>(tc.get_type_or_throw(tk), tk));
    }

    [[nodiscard]] static auto parse_variant(toc& tc, tokenizer& tz,
                                            const type& tp,
                                            const literal_typing typing)
        -> expr_variant {

        // an instance value, e.g. 'point{1, 2}' or 'p'
        if (not tp.is_builtin()) {
            return expr_type{tc, tz, tp, typing};
        }

        if (const std::optional<token> open_brace_tk{
                tz.is_next_char_token('{'),
            }) {

            return parse_zero_braces(tc, tz, tp, *open_brace_tk);
        }

        const token tk{tz.next_token()};

        if (is_bare_builtin_type(tc, tk, tz)) {
            return parse_type_as_zero(tc, tz, tp, typing, tk);
        }

        assert_not_aggregate(tc, tz, tp, tk);

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
                assert_number_kind(value, tp);
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

    // e.g. 'n = {}' is 'n = 0' and 'b = {}' is 'b = false'
    [[nodiscard]] static auto parse_zero_braces(toc& tc, tokenizer& tz,
                                                const type& tp,
                                                const token& open_brace_tk)
        -> expr_variant {

        const token close_brace_tk{
            tz.expect_char_token(
                '}', "expected '}', the zero value of a built-in is '{}'"),
        };

        assert_zero_not_reached_into(tz);

        // e.g. the '+' of 'n = {} + 1'
        if (continues_expression(tz)) {
            throw compile_error{
                tz, "'{}' is a whole value, an operator cannot follow it"};
        }

        return builtin_value(
            tc, tz,
            std::make_unique<expr_zero>(tp, open_brace_tk, close_brace_tk));
    }

    // the parse checks a string for a known length ('assert_string_fits'),
    // the plan one for an unsized destination ('plan_string')
    [[nodiscard]] static auto string_overflows(const size_t size_bytes,
                                               const size_t array_count)
        -> std::string {

        return std::format("string size {} overflows array size {}", size_bytes,
                           array_count);
    }

    // the parse checks a list of a known length when it is full
    // ('parse_braced_items'), the plan one of an unsized destination, whose
    // length the call gives
    [[nodiscard]] static auto too_many_elements(const size_t array_count)
        -> std::string {

        return std::format("too many elements for array of size {}",
                           array_count);
    }

    // the size the prefix of a list gives, e.g. the 4 of 'i8[4]{1, 2}', empty
    // for 'i8[]{1, 2}' and '{1, 2}'
    [[nodiscard]] static auto written_size(const element_list& list)
        -> std::optional<size_t> {

        if (not list.prefix or not list.prefix->size.has_value()) {
            return std::nullopt;
        }

        return list.prefix->size.array_size();
    }
};

// the size is a constant or none; it is parsed like an index, but it counts
// storage and is no value of the program, so it folds at 'i64' on every
// target, e.g. 'dat d = i8[3000000000]' on rv32i, which the address range
// limits
inline stmt_array_size::stmt_array_size(toc& tc, tokenizer& tz)
    : statement{tz.next_whitespace_token()} {

    set_type(tc.builtins().default_type());

    if (tz.peek_char_after_whitespace() == ']') {
        return;
    }

    assert_size_not_type(tc, tz);

    value_ = std::make_unique<expr_any>(tc, tz, tc.builtins().default_type(),
                                        literal_typing::from_destination);

    const std::optional<int64_t> folded{
        folded_value::value_of(
            value_->fold_checked(tc, tc.use_at(tc.builtins().i64()))),
    };

    if (not folded) {
        throw compile_error{value_->tok(),
                            std::format("{} is not a constant",
                                        in_quotes(trimmed_source(*value_)))};
    }

    if (*folded < 1) {
        throw compile_error{value_->tok(),
                            "expected a constant array size greater than 0"};
    }

    folded_ = *folded;
}

inline stmt_array_size::stmt_array_size(stmt_array_size&&) noexcept = default;

inline stmt_array_size::~stmt_array_size() = default;

inline auto stmt_array_size::source_to(std::ostream& os) const -> void {
    statement::source_to(os);

    if (value_) {
        value_->source_to(os);
    }
}

inline auto stmt_array_size::check_where_written(const toc& tc) const -> void {
    if (not value_) {
        return;
    }

    const std::optional<int64_t> folded{
        folded_value::value_of(
            value_->fold_checked(tc, tc.use_at(tc.builtins().i64()))),
    };

    // note: the names of a size are constants wherever it is written, and
    //       a length is the same in every call

    assert(folded == folded_);
}

inline auto stmt_array_size::array_size() const -> size_t {
    assert(value_);

    return static_cast<size_t>(folded_);
}

inline auto stmt_array_size::assert_size_not_type(const toc& tc,
                                                  const tokenizer& tz) -> void {

    tokenizer probe{tz};

    const token tk{probe.next_token()};

    if (not is_bare_builtin_type(tc, tk, probe)) {
        return;
    }

    throw compile_error{
        tk, std::format("{} is a type, an array size is a constant number",
                        in_quotes(tk.text()))};
}
