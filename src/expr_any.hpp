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
// its destination; one of five forms:
//   a single value       'x + 1', 'a < b' or 'point{1, 2}', one element
//   a string             '"hi"' fills the start of an 'i8' array
//   braced elements      'i8[]{1, 2}' or '{{1, 2}, {3, 4}}' of an array
//   a copy of an array   'a' of 'var b = a', one 'expr_type' element
//   a bare sized array   'i8[4]', no elements, every element is zero
// the form is told by 'is_array', 'is_string' and 'is_array_identifier', a
// bare sized array is the array form without elements
class expr_any final : public statement {
    using expr_variant = std::variant<expr_arith, expr_bool, expr_type>;

    std::vector<expr_variant> elements_;
    std::vector<token> element_delims_tk_;
    // e.g. 'i8' of 'i8[3]{1, 2}', empty for '[3]{1, 2}'
    token element_type_tk_;
    token open_bracket_tk_;
    stmt_array_size literal_size_;
    token close_bracket_tk_;
    token open_brace_tk_;
    token close_brace_tk_;
    token string_tk_;
    size_t array_count_{};
    bool is_array_{};
    bool is_identifier_{};
    // e.g. an array parameter 's[]' gets its size from the argument
    bool is_unsized_destination_{};

  public:
    // a single value, or with 'array_count' an array of that many elements,
    // see 'unsized_array_count'; where 'typing' is 'written' a
    // type or array may be written without '{}', e.g. 'var p = point' and
    // 'var a = i8[4]'
    expr_any(toc& tc, tokenizer& tz, const type& tp,
             const literal_typing typing,
             const std::optional<size_t> array_count = std::nullopt)
        : statement{tz.next_whitespace_token()},
          array_count_{array_count.value_or(0)},
          is_array_{array_count.has_value()} {

        set_type(tp);

        // the basic case, e.g. 'x + 1' or 'point{1, 2}'
        if (not is_array_) {
            elements_.emplace_back(parse_variant(tc, tz, tp, typing));
            return;
        }

        // array, e.g. 'i8[]{1, 2}' or 'a' of 'var b = a'

        is_unsized_destination_ = array_count_ == unsized_array_count;

        // e.g. "hello" fills the start of an 'i8' array
        if (tz.is_peek_char('"')) {
            string_tk_ = tz.next_token();

            array_count_ = string_array_count(tc.builtins().i8(), string_tk_,
                                              tp, array_count_);

            return;
        }

        // an array is initialized by a '{ ... }' or by the name of an array,
        // e.g. 'str.data'

        parse_element_type(tc, tz, tp, typing);

        open_brace_tk_ = tz.is_next_char_token('{');

        if (open_brace_tk_.is_empty() and not open_bracket_tk_.is_empty()) {
            // e.g. 'var a = i8[4]' is 'i8[4]{}'
            if (not literal_size_.has_value()) {
                throw compile_error{
                    close_bracket_tk_,
                    std::format("an array needs a constant size, e.g. "
                                "'{}[4]'",
                                element_type_tk_.text())};
            }

            return;
        }

        if (open_brace_tk_.is_empty()) {
            // 'expr_type' copies the whole array as bytes, 'expr_arith' would
            // read a single scalar
            // e.g. 'var b = a' copies the array 'a'
            elements_.emplace_back(expr_type{
                tc, tz, tp, expr_type::destination_kind::whole_array, typing});

            is_identifier_ = true;

            return;
        }

        // e.g. the '{1, 2, 3}' of 'var a = i8[]{1, 2, 3}'
        parse_braced_elements(tc, tz, tp);
    }

    // a method receiver, the first argument of the call
    expr_any(const token pos_tk, expr_type receiver) : statement{pos_tk} {
        set_type(receiver.get_type());
        elements_.emplace_back(std::move(receiver));
    }

    expr_any() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        string_tk_.source_to(os);

        if (not open_bracket_tk_.is_empty()) {
            element_type_tk_.source_to(os);
            open_bracket_tk_.source_to(os);
            literal_size_.source_to(os);
            close_bracket_tk_.source_to(os);
        }

        open_brace_tk_.source_to(os);

        const auto elements{
            std::views::iota(size_t{}, elements_.size()) |
                std::views::transform(
                    [this](const size_t i) -> const statement& {
                        return element(i);
                    }),
        };

        source_delimited_to(os, elements, element_delims_tk_);

        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // note: an unsized destination is the only array of unknown length

        assert(not is_array_ or is_identifier_ or
               dst.is_length_unknown() ==
                   (is_unsized_destination_ and not dst.array_len()));

        write_plan plan;
        plan_writes(tc, part_location{}, dst.array_len(), plan);
        emit_write_plan(tc, indent, plan, dst);
    }

    [[nodiscard]] auto accessed_path() const -> access_path override {

        // note: only identifier arguments ask, and those are never arrays
        assert(not is_array_);
        assert(elements_.size() == 1);

        return element(0).accessed_path();
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        if (is_array_) {
            return;
        }

        element(0).assert_not_narrowed(tc, dst_type);
    }

    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
        -> operand override {

        return element(0).compile_lea(tc, indent, src_loc_tk,
                                      allocated_registers, request);
    }

    // a scalar folds as its value
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        assert(not is_array_);

        return fold_element(tc, 0, use);
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        assert(not is_array_);

        // note: an 'expr_type' parses no unary ops, its 'unary_ops' are empty
        return element(0).get_unary_ops();
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        if (is_array_ or elements_.size() != 1) {
            return false;
        }

        return element(0).is_array_element();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        if (is_array_) {
            return true;
        }

        return element(0).is_expression();
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        assert(not is_array_);

        return element(0).is_indexed();
    }

    [[nodiscard]] auto is_name_or_literal() const -> bool override {
        return element(0).is_name_or_literal();
    }

    // only string and '{}' initializers leave 'elements_' empty, callers ask
    // arrays only when 'is_array_identifier' and arguments are never arrays
    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        assert(not elements_.empty());

        return element(0).name_or_literal();
    }

    // a single value or the copy of an array, e.g. the 'a' of 'var b = a'
    [[nodiscard]] auto place_path() const -> const stmt_identifier* override {
        if (is_array_ and not is_identifier_) {
            return nullptr;
        }

        return element(0).place_path();
    }

    [[nodiscard]] auto tok() const -> const token& override {
        if (is_string()) {
            return string_tk_;
        }

        if (elements_.empty()) {
            return statement::tok();
        }

        return element(0).tok();
    }

    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        for (size_t i{}; i < elements_.size(); ++i) {
            element(i).visit_call_effects(tc, visitor);
        }
    }

    auto visit_index_reads(const read_filter var,
                           const read_visitor reader) const -> void override {

        assert(not is_array_);
        assert(elements_.size() == 1);

        element(0).visit_index_reads(var, reader);
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        // a non-array expression is the single element
        for (size_t i{}; i < elements_.size(); ++i) {
            element(i).visit_reads(var, reader);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }

    // where an array is written: its element type, e.g. 'i8' of 'i8[]{1}', its
    // '[' when the type is the default one, its '{' when the destination gives
    // the type, else its value, e.g. a string
    [[nodiscard]] auto array_token() const -> const token& {
        if (not element_type_tk_.is_empty()) {
            return element_type_tk_;
        }

        if (not open_bracket_tk_.is_empty()) {
            return open_bracket_tk_;
        }

        if (not open_brace_tk_.is_empty()) {
            return open_brace_tk_;
        }

        return tok();
    }

    [[nodiscard]] auto as_expr_type(const size_t index = 0) const
        -> const expr_type& {

        return std::get<expr_type>(elements_.at(index));
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

        assert(not is_array_);

        const expr_arith& arith{std::get<expr_arith>(elements_.front())};

        arith.assert_not_narrowed(tc, dst.type_ref());

        return arith.compile_without_trailing_addend(tc, indent, dst);
    }

    // e.g. '-2' or a named constant at the default type, empty when computed
    // at run time
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<int64_t> {

        return folded_value::value_of(
            fold(tc, tc.use_at(tc.builtins().default_type())));
    }

    // every kind of element is a 'statement', the forwarding methods call its
    // virtuals; the only 'visit' of the variant
    [[nodiscard]] auto element(const size_t index) const -> const statement& {
        const statement* const value{
            elements_.at(index).visit(
                [](const statement& e) -> const statement* { return &e; }),
        };

        return *value;
    }

    [[nodiscard]] auto element_count() const -> size_t {
        return elements_.size();
    }

    // the element 'index' folded and checked, empty when it is computed at
    // run time, e.g. 'maybe == 33' with 'maybe' a constant folds
    [[nodiscard]] auto fold_element(const toc& tc, const size_t index,
                                    const value_use& use) const
        -> std::optional<folded_value> {

        return element(index).fold_checked(tc, use);
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_array_identifier() const -> bool {
        return is_array_ and is_identifier_;
    }

    [[nodiscard]] auto is_string() const -> bool {
        return string_tk_.is_string();
    }

    [[nodiscard]] auto open_bracket_token() const -> const token& {
        return open_bracket_tk_;
    }

    // appends the steps that write the value at 'at'; 'unsized_count' is the
    // size of an unsized destination, e.g. a parameter 's[]', empty when it
    // is not known, then the elements after the literal are not written
    auto plan_writes(const toc& tc, const part_location& at,
                     const std::optional<size_t> unsized_count,
                     write_plan& plan) const -> void {

        if (is_identifier_ or not is_array_) {
            plan_element(tc, 0, at, plan);
            return;
        }

        // empty when the size is not known
        const std::optional<size_t> array_count{
            is_unsized_destination_ ? unsized_count : array_count_,
        };

        if (is_string()) {
            plan_string(at, array_count, plan);
            return;
        }

        // the parser could not check the size of an unsized destination
        if (array_count and elements_.size() > *array_count) {
            throw compile_error{
                element(*array_count).tok(),
                std::format("too many elements for array of size {}",
                            *array_count)};
        }

        plan_elements(tc, at, plan);

        if (not array_count) {
            return;
        }

        expr_type::plan_remaining_elements(
            tok(), at, get_type(), elements_.size(), *array_count, plan);
    }

  private:
    // the remaining count would wrap around past the array size
    auto assert_room_for_element(const tokenizer& tz) const -> void {
        if (array_count_ == unsized_array_count or
            elements_.size() != array_count_) {

            return;
        }

        throw compile_error{tz, std::format("too many elements for array of "
                                            "size {}",
                                            array_count_)};
    }

    // the little endian bytes of the elements, empty when an element is not a
    // constant or there are no elements to store
    [[nodiscard]] auto constant_bytes(const toc& tc,
                                      const type& element_type) const
        -> std::optional<std::string> {

        if (elements_.empty()) {
            return std::nullopt;
        }

        std::string bytes;
        for (size_t index{}; index < elements_.size(); ++index) {
            // a condition is compiled, not packed, e.g. '1 == 1'
            if (std::holds_alternative<expr_bool>(elements_.at(index))) {
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

    // the elements between the braces, e.g. '1, 2, 3' of '{1, 2, 3}'
    auto parse_braced_elements(toc& tc, tokenizer& tz, const type& tp) -> void {
        close_brace_tk_ = tz.is_next_char_token('}');

        while (close_brace_tk_.is_empty()) {
            if (not elements_.empty()) {
                parse_element_delimiter(tz, tp);

                // a trailing ',' lets each element end its line the same way
                close_brace_tk_ = tz.is_next_char_token('}');

                if (not close_brace_tk_.is_empty()) {
                    break;
                }
            }

            assert_room_for_element(tz);

            // the array gives the type of its elements
            elements_.emplace_back(
                parse_variant(tc, tz, tp, literal_typing::from_destination));

            close_brace_tk_ = tz.is_next_char_token('}');
        }

        if (array_count_ == unsized_array_count) {
            array_count_ = elements_.size();
        }
    }

    auto parse_element_delimiter(tokenizer& tz, const type& tp) -> void {
        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by initializer "
                                 "for type {}",
                                 in_quotes(tp.name()))),
        };

        element_delims_tk_.emplace_back(delimiter_tk);
    }

    // e.g. 'i8[3]' in 'i8[3]{1, 2}' names the element type that '{1, 2}' takes
    // from the destination, and the size unless it is 'i8[]'; '[3]{1, 2}' has
    // the default type
    auto parse_element_type(toc& tc, tokenizer& tz, const type& tp,
                            const literal_typing typing) -> void {

        const token tk{tz.next_token()};

        const bool is_typed{
            classify_term(tc, tk, tz).is(term_kind::array_literal),
        };

        if (not is_typed) {
            tz.put_back_token(tk);
        }

        if (not is_typed and not is_default_array_literal(tz)) {
            return;
        }

        open_bracket_tk_ = tz.is_next_char_token('[');

        const type& literal_type{
            is_typed ? tc.get_type_or_throw(tk) : tc.builtins().default_type(),
        };

        if (not literal_type.is_same(tp)) {
            throw compile_error{is_typed ? tk : open_bracket_tk_,
                                std::format("expected type {}, got {}",
                                            in_quotes(tp.name()),
                                            in_quotes(literal_type.name()))};
        }

        if (is_typed) {
            element_type_tk_ = tk;
        }

        // e.g. the '4' of 'i8[4]', none for 'i8[]'
        literal_size_ = {tc, tz};

        close_bracket_tk_ =
            tz.expect_char_token(']', "expected ']' after array size");

        // e.g. 'i8[]{1, 2}' for an 'i8' array field is '{1, 2}' and 'i8[4]'
        // is '{}'
        if (typing == literal_typing::from_destination) {
            throw_known_type(is_typed ? tk : open_bracket_tk_,
                             std::format("{}[]", literal_type.name()),
                             tz.peek_char_after_whitespace() == '{' ? "{...}"
                                                                    : "{}");
        }

        if (not literal_size_.has_value()) {
            return;
        }

        const size_t count{literal_size_.array_size()};

        // note: only a declaration names the array type, its size is the one
        //       of this literal

        assert(array_count_ == unsized_array_count or array_count_ == count);

        array_count_ = count;
    }

    // an instance literal is written field by field, any other value at once
    auto plan_element(const toc& tc, const size_t index,
                      const part_location& at, write_plan& plan) const -> void {

        if (const expr_type* const record{
                std::get_if<expr_type>(&elements_.at(index)),
            }) {

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
    auto plan_elements(const toc& tc, const part_location& at,
                       write_plan& plan) const -> void {

        if (std::optional<std::string> bytes{constant_bytes(tc, get_type())};
            bytes) {

            plan.push_back({
                .at{at},
                .action{
                    bytes_write{
                        .src_loc_tk{open_brace_tk_},
                        .bytes{std::move(*bytes)},
                        .is_string{},
                    },
                },
                .covered{
                    {at.elements_path(0, elements_.size())},
                },
            });

            return;
        }

        for (size_t i{}; i < elements_.size(); ++i) {
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
    auto plan_string(const part_location& at,
                     const std::optional<size_t> array_count,
                     write_plan& plan) const -> void {

        std::string bytes{token::decode_string(string_tk_.string_text())};

        const size_t size_bytes{bytes.size()};

        if (array_count and size_bytes > *array_count) {
            throw compile_error{
                string_tk_,
                std::format("string size {} overflows array size {}",
                            size_bytes, *array_count)};
        }

        // an empty string has nothing to store
        if (size_bytes != 0) {
            plan.push_back({
                .at{at},
                .action{
                    bytes_write{
                        .src_loc_tk{string_tk_},
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
                    .src_loc_tk{string_tk_},
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

    //
    // statics
    //

    // a 'bool' is not an integer, 'int(b)' converts it; an arithmetic operand
    // and a call result are checked by rules of their own
    static auto assert_not_boolean_name(const toc& tc, const expr_arith& value,
                                        const type& tp) -> void {

        if (not value.is_name_or_literal() or not value.get_type().is_bool()) {
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
            value_->fold(tc, tc.use_at(tc.builtins().i64()))),
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
