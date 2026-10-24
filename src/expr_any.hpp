#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
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
#include "stmt_builtin_convert.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

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

    stmt_array_size();

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

    // a written size of at least 1, e.g. the '4' of 'i8[4]'
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
        token element_type_tk;
        token open_bracket_tk;
        stmt_array_size size;
        token close_bracket_tk;

        auto source_to(std::ostream& os) const -> void {
            if (open_bracket_tk.is_empty()) {
                return;
            }

            element_type_tk.source_to(os);
            open_bracket_tk.source_to(os);
            size.source_to(os);
            close_bracket_tk.source_to(os);
        }
    };

    // e.g. 'x + 1', 'a < b' or 'point{1, 2}'
    struct single_value {
        expr_variant value;
    };

    // e.g. '"hi"', which fills the start of an 'i8' array
    struct string_array {
        token string_tk;
    };

    // e.g. 'i8[]{1, 2}' or '{{1, 2}, {3, 4}}'
    struct element_list {
        array_prefix prefix;
        token open_brace_tk;
        std::vector<expr_variant> elements;
        std::vector<token> delims_tk;
        token close_brace_tk;
    };

    // e.g. 'i8[4]', every element is zero
    struct zero_array {
        array_prefix prefix;
    };

    // e.g. the 'a' of 'var b = a', copied whole as bytes
    struct array_copy {
        expr_type source;
    };

    std::variant<single_value, string_array, element_list, zero_array,
                 array_copy>
        form_;
    // what the destination gives, e.g. an array parameter 's[]' is an array
    // of unknown length, its argument gives the length when it is compiled
    array_shape destination_;
    // the size of an array form: the length of the destination, else the size
    // the literal gives, e.g. 3 of 'i8[]{1, 2, 3}'; empty until known
    std::optional<size_t> array_count_;

  public:
    // a single value, or an array when 'destination' is one; where 'typing' is
    // 'written' a type or array may be written without '{}', e.g. 'var p =
    // point' and 'var a = i8[4]'
    expr_any(toc& tc, tokenizer& tz, const type& tp,
             const literal_typing typing, const array_shape destination = {})
        : statement{tz.next_whitespace_token()}, destination_{destination},
          array_count_{destination.length()} {

        set_type(tp);

        // the basic case, e.g. 'x + 1' or 'point{1, 2}'
        if (not destination.is_array()) {
            form_ = single_value{
                .value{parse_variant(tc, tz, tp, typing)},
            };

            return;
        }

        // array, e.g. 'i8[]{1, 2}' or 'a' of 'var b = a'

        // e.g. "hello" fills the start of an 'i8' array
        if (tz.is_peek_char('"')) {
            const token string_tk{tz.next_token()};

            array_count_ = string_array_count(tc.builtins().i8(), string_tk, tp,
                                              array_count_);

            form_ = string_array{
                .string_tk{string_tk},
            };

            return;
        }

        // an array is initialized by a '{ ... }' or by the name of an array,
        // e.g. 'str.data'

        array_prefix prefix{parse_array_prefix(tc, tz, tp, typing)};

        const token open_brace_tk{tz.is_next_char_token('{')};

        if (open_brace_tk.is_empty() and
            not prefix.open_bracket_tk.is_empty()) {

            // e.g. 'var a = i8[4]' is 'i8[4]{}'
            if (not prefix.size.has_value()) {
                throw compile_error{
                    prefix.close_bracket_tk,
                    std::format("an array needs a constant size, e.g. "
                                "'{}[4]'",
                                prefix.element_type_tk.text())};
            }

            form_ = zero_array{
                .prefix{std::move(prefix)},
            };

            return;
        }

        if (open_brace_tk.is_empty()) {
            // 'expr_type' copies the whole array as bytes, 'expr_arith' would
            // read a single scalar
            // e.g. 'var b = a' copies the array 'a'
            form_ = array_copy{
                .source{
                    tc,
                    tz,
                    tp,
                    expr_type::destination_kind::whole_array,
                    typing,
                },
            };

            return;
        }

        // e.g. the '{1, 2, 3}' of 'var a = i8[]{1, 2, 3}'
        element_list list{
            .prefix{std::move(prefix)},
            .open_brace_tk{open_brace_tk},
            .elements{},
            .delims_tk{},
            .close_brace_tk{},
        };

        parse_braced_elements(tc, tz, tp, list);

        form_ = std::move(list);
    }

    // a method receiver, the first argument of the call
    expr_any(const token pos_tk, expr_type receiver) : statement{pos_tk} {
        set_type(receiver.get_type());

        form_ = single_value{
            .value{std::move(receiver)},
        };
    }

    expr_any() = default;

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

        list->prefix.source_to(os);
        list->open_brace_tk.source_to(os);

        const auto elements{
            std::views::iota(size_t{}, list->elements.size()) |
                std::views::transform(
                    [this](const size_t i) -> const statement& {
                        return element(i);
                    }),
        };

        source_delimited_to(os, elements, list->delims_tk);

        list->close_brace_tk.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // note: an unsized destination is the only array of unknown length

        assert(not is_array() or is_array_identifier() or
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

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        assert(not is_array());

        // note: an 'expr_type' parses no unary ops, its 'unary_ops' are empty
        return element(0).get_unary_ops();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        if (is_array()) {
            return true;
        }

        return element(0).is_expression();
    }

    // a single value or the copy of an array, e.g. the 'a' of 'var b = a'
    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override {
        if (const array_copy* const copy{std::get_if<array_copy>(&form_)}) {
            return copy->source.operand_path();
        }

        if (is_array()) {
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

    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        for (size_t i{}; i < element_count(); ++i) {
            element(i).visit_call_effects(tc, visitor);
        }
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        for (size_t i{}; i < element_count(); ++i) {
            element(i).visit_reads(var, reader);
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
            if (not prefix->element_type_tk.is_empty()) {
                return prefix->element_type_tk;
            }

            if (not prefix->open_bracket_tk.is_empty()) {
                return prefix->open_bracket_tk;
            }
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
        if (const array_copy* const copy{std::get_if<array_copy>(&form_)}) {
            assert(index == 0);

            return copy->source;
        }

        if (const single_value* const single{
                std::get_if<single_value>(&form_),
            }) {

            assert(index == 0);

            return as_statement(single->value);
        }

        return as_statement(std::get<element_list>(form_).elements.at(index));
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
        return not std::holds_alternative<single_value>(form_);
    }

    [[nodiscard]] auto is_array_identifier() const -> bool {
        return std::holds_alternative<array_copy>(form_);
    }

    [[nodiscard]] auto is_string() const -> bool {
        return std::holds_alternative<string_array>(form_);
    }

    // e.g. the '[' of 'i8[]{}', empty without an array prefix
    [[nodiscard]] auto open_bracket_token() const -> token {
        const array_prefix* const prefix{array_prefix_of()};
        return prefix != nullptr ? prefix->open_bracket_tk : token{};
    }

    // appends the steps that write the value at 'at'; 'unsized_count' is the
    // size of an unsized destination, e.g. a parameter 's[]', empty when it
    // is not known, then the elements after the literal are not written
    auto plan_writes(const toc& tc, const part_location& at,
                     const std::optional<size_t> unsized_count,
                     write_plan& plan) const -> void {

        if (not is_array() or is_array_identifier()) {
            plan_element(tc, 0, at, plan);
            return;
        }

        // empty when the size is not known
        const std::optional<size_t> array_count{
            destination_.is_length_unknown() ? unsized_count : array_count_,
        };

        if (const string_array* const str{std::get_if<string_array>(&form_)}) {
            plan_string(str->string_tk, at, array_count, plan);
            return;
        }

        const size_t count{element_count()};

        // the parser could not check the size of an unsized destination
        if (array_count and count > *array_count) {
            throw compile_error{
                element(*array_count).tok(),
                std::format("too many elements for array of size {}",
                            *array_count)};
        }

        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            plan_elements(tc, *list, at, plan);
        }

        if (not array_count) {
            return;
        }

        expr_type::plan_remaining_elements(tok(), at, get_type(), count,
                                           *array_count, plan);
    }

  private:
    // the prefix of an element list or a zero array, null for another form
    [[nodiscard]] auto array_prefix_of() const -> const array_prefix* {
        if (const element_list* const list{std::get_if<element_list>(&form_)}) {
            return &list->prefix;
        }

        if (const zero_array* const zero{std::get_if<zero_array>(&form_)}) {
            return &zero->prefix;
        }

        return nullptr;
    }

    // the remaining count would wrap around past the array size
    auto assert_room_for_element(const tokenizer& tz,
                                 const element_list& list) const -> void {

        if (not array_count_ or list.elements.size() != *array_count_) {
            return;
        }

        throw compile_error{tz, std::format("too many elements for array of "
                                            "size {}",
                                            *array_count_)};
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
            // a condition is compiled, not packed, e.g. '1 == 1'
            if (std::holds_alternative<expr_bool>(list.elements.at(index))) {
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
    // the default type; empty when the destination gives both
    [[nodiscard]] auto parse_array_prefix(toc& tc, tokenizer& tz,
                                          const type& tp,
                                          const literal_typing typing)
        -> array_prefix {

        array_prefix prefix;

        const token tk{tz.next_token()};

        const bool is_typed{
            classify_term(tc, tk, tz).is(term_kind::array_literal),
        };

        if (not is_typed) {
            tz.put_back_token(tk);
        }

        if (not is_typed and not is_default_array_literal(tz)) {
            return prefix;
        }

        prefix.open_bracket_tk = tz.is_next_char_token('[');

        const type& literal_type{
            is_typed ? tc.get_type_or_throw(tk) : tc.builtins().default_type(),
        };

        if (not literal_type.is_same(tp)) {
            throw compile_error{is_typed ? tk : prefix.open_bracket_tk,
                                std::format("expected type {}, got {}",
                                            in_quotes(tp.name()),
                                            in_quotes(literal_type.name()))};
        }

        if (is_typed) {
            prefix.element_type_tk = tk;
        }

        // e.g. the '4' of 'i8[4]', none for 'i8[]'
        prefix.size = {tc, tz};

        prefix.close_bracket_tk =
            tz.expect_char_token(']', "expected ']' after array size");

        // e.g. 'i8[]{1, 2}' for an 'i8' array field is '{1, 2}' and 'i8[4]'
        // is '{}'
        if (typing == literal_typing::from_destination) {
            throw_known_type(is_typed ? tk : prefix.open_bracket_tk,
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

        list.close_brace_tk = tz.is_next_char_token('}');

        while (list.close_brace_tk.is_empty()) {
            if (not list.elements.empty()) {
                parse_element_delimiter(tz, tp, list);

                // a trailing ',' lets each element end its line the same way
                list.close_brace_tk = tz.is_next_char_token('}');

                if (not list.close_brace_tk.is_empty()) {
                    break;
                }
            }

            assert_room_for_element(tz, list);

            // the array gives the type of its elements
            list.elements.emplace_back(
                parse_variant(tc, tz, tp, literal_typing::from_destination));

            list.close_brace_tk = tz.is_next_char_token('}');
        }

        if (not array_count_) {
            array_count_ = list.elements.size();
        }
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
            };
            bytes) {

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
                        .src_loc_tk{tok()},
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

    // the string is stored and the rest of the array is zeroed like unlisted
    // elements; 'array_count' is empty when the size is not known
    auto plan_string(const token& string_tk, const part_location& at,
                     const std::optional<size_t> array_count,
                     write_plan& plan) const -> void {

        std::string bytes{token::decode_string(string_tk.string_text())};

        const size_t size_bytes{bytes.size()};

        if (array_count and size_bytes > *array_count) {
            throw compile_error{
                string_tk, std::format("string size {} overflows array size {}",
                                       size_bytes, *array_count)};
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

        const size_t remaining_size_bytes{*array_count - size_bytes};

        if (remaining_size_bytes == 0) {
            return;
        }

        plan.push_back({
            .at{at},
            .action{
                zero_fill{
                    .src_loc_tk{string_tk},
                    .comment{
                        std::format("zero remaining elements: {} B",
                                    remaining_size_bytes),
                    },
                    .offset{size_bytes},
                    .value_type{&get_type()},
                    .size_bytes{remaining_size_bytes},
                    .alignment{get_type().alignment()},
                },
            },
            .covered{
                {at.elements_path(size_bytes, *array_count)},
            },
        });
    }

    // the element at 'index' when it is an 'expr_type', null otherwise
    [[nodiscard]] auto record_at(const size_t index) const -> const expr_type* {
        if (const array_copy* const copy{std::get_if<array_copy>(&form_)}) {
            return &copy->source;
        }

        if (const single_value* const single{
                std::get_if<single_value>(&form_),
            }) {

            return std::get_if<expr_type>(&single->value);
        }

        return std::get_if<expr_type>(
            &std::get<element_list>(form_).elements.at(index));
    }

    //
    // statics
    //

    // every kind of element is a 'statement'; the only 'visit' of the variant
    [[nodiscard]] static auto as_statement(const expr_variant& value)
        -> const statement& {

        const statement* const e{
            value.visit(
                [](const statement& v) -> const statement* { return &v; }),
        };

        return *e;
    }

    // a 'bool' is not an integer, 'int(b)' converts it; an arithmetic operand
    // and a call result are checked by rules of their own
    static auto assert_not_boolean_name(const toc& tc, const expr_arith& value,
                                        const type& tp) -> void {

        if (value.operand_path() == nullptr or not value.get_type().is_bool()) {
            return;
        }

        // a whole array of 'bool' is bytes to a built-in, e.g. 'flags' of
        // 'write(1, flags)'
        if (const std::optional<place> array{as_place(tc, value)};
            array and array->info.is_array()) {

            return;
        }

        throw compile_error{value.tok(),
                            std::format("expected type {}, got {}",
                                        in_quotes(tp.name()),
                                        in_quotes(value.get_type().name()))};
    }

    // a built-in value whose only element is parsed, e.g. the zero value of
    // 'i8', 'bool' or '{}'
    [[nodiscard]] static auto builtin_value(toc& tc, tokenizer& tz,
                                            std::unique_ptr<statement> first)
        -> expr_variant {

        return expr_arith{
            tc,
            tz,
            expr_arith::options{
                .uops{},
                .open_paren_tk{},
                .is_implied_subexpression{},
                .first_op_precedence{expr_arith::initial_precedence},
                .first_expression{std::move(first)},
                .destination_type{},
            },
        };
    }

    static auto parse_element_delimiter(tokenizer& tz, const type& tp,
                                        element_list& list) -> void {

        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by initializer "
                                 "for type {}",
                                 in_quotes(tp.name()))),
        };

        list.delims_tk.emplace_back(delimiter_tk);
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
        if (const token open_brace_tk{tz.is_next_char_token('{')};
            not open_brace_tk.is_empty()) {

            const token close_brace_tk{
                tz.expect_char_token(
                    '}', "expected '}', the zero value of a built-in is '{}'"),
            };

            // e.g. the '+' of 'n = {} + 1'
            if (continues_expression(tz)) {
                throw compile_error{
                    tz, "'{}' is a whole value, an operator cannot follow it"};
            }

            return builtin_value(
                tc, tz,
                std::make_unique<expr_zero>(tp, open_brace_tk, close_brace_tk));
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
                    throw compile_error{
                        tk, std::format("expected type {}, got {}",
                                        in_quotes(tp.name()),
                                        in_quotes(named_type.name()))};
                }

                throw_known_type(tk, tk.text(), "{}");
            }

            return builtin_value(
                tc, tz, std::make_unique<stmt_builtin_convert>(tc, tk));
        }

        tz.put_back_token(tk);

        // a generic call takes the type of a known destination for its
        // result, e.g. 'x = make()'
        if (not tp.is_bool() or (typing == literal_typing::from_destination and
                                 is_whole_generic_call(tc, tz))) {

            expr_arith value{
                tc,
                tz,
                expr_arith::options{
                    .uops{},
                    .open_paren_tk{},
                    .is_implied_subexpression{},
                    .first_op_precedence{expr_arith::initial_precedence},
                    .first_expression{},
                    .destination_type{
                        typing == literal_typing::from_destination ? &tp
                                                                   : nullptr,
                    },
                },
            };

            assert_not_boolean_name(tc, value, tp);

            // e.g. the '>' of 'int(a > 2)', which would end the number
            expr_arith::assert_no_condition_next(tz);

            return value;
        }

        expr_bool value{tc, tz.next_whitespace_token(), tz};

        value.assert_boolean_operands();

        // a name is read and passed where it is stored like a name of another
        // type, e.g. the 'x' of 'f(x)' that a 'mut bool' parameter writes
        if (std::optional<expr_arith> name{value.release_name_or_literal()};
            name) {

            return std::move(*name);
        }

        return value;
    }

    // the size of an 'i8' array that a string fills: 'array_count', or the
    // size of the string when the destination gives none
    [[nodiscard]] static auto
    string_array_count(const type& string_element_type, const token& string_tk,
                       const type& element_type,
                       const std::optional<size_t> array_count) -> size_t {

        const size_t size_bytes{string_tk.string_size_bytes()};

        if (not element_type.is_same(string_element_type)) {
            throw compile_error{
                string_tk, std::format("a string fills only an array of "
                                       "{}, not an array of {}",
                                       in_quotes(string_element_type.name()),
                                       in_quotes(element_type.name()))};
        }

        if (not array_count) {
            // arrays of zero length are not allowed
            if (size_bytes == 0) {
                throw compile_error{string_tk,
                                    "an empty string is not valid for an array "
                                    "with unspecified size"};
            }

            return size_bytes;
        }

        if (size_bytes > *array_count) {
            throw compile_error{
                string_tk, std::format("string size {} overflows array size {}",
                                       size_bytes, *array_count)};
        }

        return *array_count;
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

    folded_ = *folded;
}

inline stmt_array_size::stmt_array_size() = default;

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
    if (not value_) {
        throw compile_error{tok(),
                            "expected a constant array size greater than 0"};
    }

    if (folded_ < 1) {
        throw compile_error{value_->tok(),
                            "expected a constant array size greater than 0"};
    }

    return static_cast<size_t>(folded_);
}
