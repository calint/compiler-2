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

#include "compiler_exception.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "expr_type.hpp"
#include "expr_zero.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_const.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

class expr_any final : public statement {
    using expr_variant = std::variant<expr_arith, expr_bool, expr_type>;

    std::vector<expr_variant> elements_;
    std::vector<token> element_delims_tk_;
    // e.g. 'i8[3]' in 'i8[3]{1, 2}'
    token element_type_tk_;
    token open_bracket_tk_;
    stmt_const literal_count_const_;
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
    // a single value, or with 'array_count' an array of that many elements, 0
    // when the initializer gives the count; where 'typing' is 'written' a
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

        is_unsized_destination_ = array_count_ == 0;

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
            if (not literal_count_const_.has_value()) {
                throw compiler_exception{
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
            literal_count_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }

        open_brace_tk_.source_to(os);

        if (not elements_.empty()) {
            element(0).source_to(os);
            for (size_t i{1}; i < elements_.size(); ++i) {
                element_delims_tk_.at(i - 1).source_to(os);
                // note: -1 because there is one delimiter fewer than elements

                element(i).source_to(os);
            }
        }

        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        if (is_array_identifier()) {
            const ident_info src_info{tc.make_ident_info(*this)};

            if (not src_info.is_array()) {
                throw compiler_exception{tok(), "source must be an array"};
            }
        }

        // the base case
        if (is_identifier_ or not is_array_) {
            compile_element(tc, indent, element(0), dst);
            return;
        }

        if (is_string()) {
            compile_string(tc, indent, dst);
            return;
        }

        const size_t array_count{destination_array_count(dst)};

        // the parser could not check the size of an unsized destination
        if (elements_.size() > array_count) {
            throw compiler_exception{
                element(array_count).tok(),
                std::format("too many elements for array of size {}",
                            array_count)};
        }

        // a call element writes at the element, not at the array start the
        // identifier names, and an element is not an array
        const destination first{dst.as_element(dst.operand())};

        const operand after_elements{compile_elements(tc, indent, first)};

        expr_type::zero_remaining_elements(tc, indent, tok(), after_elements,
                                           first.type_ref(),
                                           array_count - elements_.size());
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

    [[nodiscard]] auto tok() const -> const token& override {
        if (is_string()) {
            return string_tk_;
        }

        if (elements_.empty()) {
            return statement::tok();
        }

        return element(0).tok();
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

    [[nodiscard]] auto as_expr_type(const size_t index = 0) const
        -> const expr_type& {

        return std::get<expr_type>(elements_.at(index));
    }

    auto assert_record_value_not_reading(
        const expr_type::constructor_target& dst) const -> void {

        if (is_array_ or
            not std::holds_alternative<expr_type>(elements_.at(0))) {

            return;
        }

        as_expr_type().assert_not_reading(dst);
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

    // e.g. '-2' or a named constant, empty when computed at run time
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<int64_t> {

        return constant_value(tc, tc.builtins().default_type());
    }

    // folded at the width of 'width_type'
    [[nodiscard]] auto constant_value(const toc& tc,
                                      const type& width_type) const
        -> std::optional<int64_t> {

        assert(not is_array_);

        const expr_variant& e{elements_.front()};

        // e.g. 'maybe == 33' with 'maybe' a constant
        const expr_bool* const bol{std::get_if<expr_bool>(&e)};

        if (bol != nullptr) {
            const std::optional<bool> value{bol->constant_value(tc)};

            if (not value) {
                return std::nullopt;
            }

            return *value ? 1 : 0;
        }

        return constant_element_value(tc, e, width_type);
    }

    [[nodiscard]] auto element_count() const -> size_t {
        return elements_.size();
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_array_identifier() const -> bool {
        return is_array_ and is_identifier_;
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return elements_.empty() and not is_string();
    }

    [[nodiscard]] auto is_string() const -> bool {
        return string_tk_.is_string();
    }

    [[nodiscard]] auto open_bracket_token() const -> const token& {
        return open_bracket_tk_;
    }

  private:
    // the remaining count would wrap around past the array size
    auto assert_room_for_element(const tokenizer& tz) const -> void {
        if (array_count_ == 0 or elements_.size() != array_count_) {
            return;
        }

        throw compiler_exception{tz,
                                 std::format("too many elements for array of "
                                             "size {}",
                                             array_count_)};
    }

    // constant elements are stored like a string so the backend can pack them
    // into wider immediates or copy them from read-only data, 'first' is the
    // first element, returns the operand after the listed elements
    [[nodiscard]] auto compile_elements(toc& tc, const size_t indent,
                                        const destination& first) const
        -> operand {

        machine& x{tc.machine()};

        operand cursor{first.operand()};

        const std::optional<std::string> bytes{
            constant_bytes(tc, first.type_ref()),
        };

        if (bytes) {
            x.copy_bytes(open_brace_tk_, indent, *bytes, cursor,
                         first.type_ref().alignment(), [&] -> std::string {
                             return tc.add_bytes_constant(open_brace_tk_,
                                                          *bytes);
                         });

            cursor.increment_offset(address_offset(bytes->size()));

            return cursor;
        }

        for (size_t i{}; i < elements_.size(); ++i) {
            x.comment(tok(), indent, "[{}]", i);

            compile_element(tc, indent, element(i), first.with_operand(cursor));

            cursor.increment_offset(
                address_offset(first.type_ref().size_bytes()));
        }

        return cursor;
    }

    // the string is stored and the rest of the array is zeroed like unlisted
    // elements
    auto compile_string(toc& tc, const size_t indent,
                        const destination& dst) const -> void {

        machine& x{tc.machine()};

        const std::string bytes{token::decode_string(string_tk_.string_text())};

        const size_t size_bytes{bytes.size()};

        const size_t array_count{destination_array_count(dst)};

        if (size_bytes > array_count) {
            throw compiler_exception{
                string_tk_,
                std::format("string size {} overflows array size {}",
                            size_bytes, array_count)};
        }

        operand cursor{dst.operand()};

        // an empty string has nothing to store
        if (size_bytes != 0) {
            x.copy_bytes(string_tk_, indent, bytes, cursor,
                         dst.type_ref().alignment(), [&] -> std::string {
                             return tc.add_string_constant(string_tk_);
                         });

            cursor.increment_offset(address_offset(size_bytes));
        }

        const size_t remaining_size_bytes{array_count - size_bytes};

        if (remaining_size_bytes == 0) {
            return;
        }

        x.comment(string_tk_, indent, "zero remaining elements: {} B",
                  remaining_size_bytes);

        x.zero(string_tk_, indent, cursor, remaining_size_bytes,
               dst.type_ref().alignment());
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
        for (const expr_variant& e : elements_) {
            const std::optional<int64_t> value{
                constant_element_value(tc, e, tc.builtins().default_type()),
            };

            if (not value) {
                return std::nullopt;
            }

            // e.g. '{300}' for 'i8' is rejected as when stored alone
            std::get<expr_arith>(e).assert_not_narrowed(tc, element_type);

            const uint64_t bits{static_cast<uint64_t>(*value)};
            for (size_t i{}; i < element_type.size_bytes(); ++i) {
                bytes +=
                    static_cast<char>(bits >> (machine::bits_per_byte * i));
            }
        }

        return bytes;
    }

    // an unsized destination, e.g. a parameter 's[]', has the size of the
    // argument, which is known only at compile time
    [[nodiscard]] auto destination_array_count(const destination& dst) const
        -> size_t {

        if (is_unsized_destination_) {
            return dst.array_len();
        }

        return array_count_;
    }

    // every kind of element is a 'statement', the forwarding methods call its
    // virtuals; the only 'visit' of the variant
    [[nodiscard]] auto element(const size_t index) const -> const statement& {
        const statement* const kind{
            elements_.at(index).visit(
                [](const statement& e) -> const statement* { return &e; }),
        };

        return *kind;
    }

    // the elements between the braces, e.g. '1, 2, 3' of '{1, 2, 3}'
    auto parse_braced_elements(toc& tc, tokenizer& tz, const type& tp) -> void {

        close_brace_tk_ = tz.is_next_char_token('}');

        while (close_brace_tk_.is_empty()) {
            if (not elements_.empty()) {
                parse_element_delimiter(tz, tp);
            }

            assert_room_for_element(tz);

            // the array gives the type of its elements
            elements_.emplace_back(
                parse_variant(tc, tz, tp, literal_typing::from_destination));

            close_brace_tk_ = tz.is_next_char_token('}');
        }

        if (array_count_ == 0) {
            array_count_ = elements_.size();
        }
    }

    auto parse_element_delimiter(tokenizer& tz, const type& tp) -> void {
        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by initializer "
                                 "for type '{}'",
                                 tp.name())),
        };

        element_delims_tk_.emplace_back(delimiter_tk);
    }

    // e.g. 'i8[3]' in 'i8[3]{1, 2}' names the element type that '{1, 2}' takes
    // from the destination, and the size unless it is 'i8[]'; '[3]{1, 2}' has
    // the default type
    auto parse_element_type(toc& tc, tokenizer& tz, const type& tp,
                            const literal_typing typing) -> void {

        const token tk{tz.next_token()};
        const bool is_typed{is_array_literal(tc, tk, tz)};

        if (not is_typed) {
            tz.put_back_token(tk);
        }

        if (not is_typed and not is_default_array_literal(tz)) {
            return;
        }

        open_bracket_tk_ = tz.is_next_char_token('[');

        const type& literal_type{
            is_typed ? tc.get_type_or_throw(tk, tk.text())
                     : tc.builtins().default_type(),
        };

        if (not literal_type.is_same(tp)) {
            throw compiler_exception{is_typed ? tk : open_bracket_tk_,
                                     std::format("expected type '{}', got '{}'",
                                                 tp.name(),
                                                 literal_type.name())};
        }

        if (is_typed) {
            element_type_tk_ = tk;
        }

        // e.g. the '4' of 'i8[4]', none for 'i8[]'
        literal_count_const_ = {tc, tz, 0};

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

        if (not literal_count_const_.has_value()) {
            return;
        }

        if (literal_count_const_.value() <= 0) {
            throw compiler_exception{
                literal_count_const_.tok(),
                "expected a constant array size greater than 0"};
        }

        const size_t count{static_cast<size_t>(literal_count_const_.value())};

        // note: only a declaration names the array type, its size is the one
        //       of this literal
        assert(array_count_ == 0 or array_count_ == count);

        array_count_ = count;
    }

    //
    // statics
    //

    // a built-in value whose first element is parsed, e.g. the zero value of
    // 'i8' or '{}', 'pos_tk' is where the value starts
    [[nodiscard]] static auto builtin_value(toc& tc, tokenizer& tz,
                                            const token& pos_tk, const type& tp,
                                            std::unique_ptr<statement> first)
        -> expr_variant {

        if (tp.is_bool()) {
            return expr_bool{tc, pos_tk, tz, std::move(first)};
        }

        return expr_arith{
            tc,
            tz,
            expr_arith::options{
                .uops{},
                .open_paren_tk{},
                .is_implied_subexpression{},
                .first_op_precedence{expr_arith::initial_precedence},
                .first_expression{std::move(first)},
            },
        };
    }

    // the value boundary is where a wider source loses bits, an instance or a
    // 'bool' has none to lose
    static auto compile_element(toc& tc, const size_t indent,
                                const statement& value, const destination& dst)
        -> void {

        value.assert_not_narrowed(tc, dst.type_ref());
        value.compile(tc, indent, dst);
    }

    [[nodiscard]] static auto constant_arith_value(const toc& tc,
                                                   const expr_arith& arith,
                                                   const type& width_type)
        -> std::optional<int64_t> {

        // e.g. 'array_length(a)'
        if (arith.is_expression()) {
            return arith.folded_constant(tc, width_type);
        }

        const ident_info info{tc.make_ident_info(arith)};

        if (not info.is_const()) {
            return std::nullopt;
        }

        return arith.get_unary_ops().evaluate_constant(info.const_value());
    }

    // 'bool' and instance elements are not packed
    [[nodiscard]] static auto constant_element_value(const toc& tc,
                                                     const expr_variant& e,
                                                     const type& width_type)
        -> std::optional<int64_t> {

        const expr_arith* const arith{std::get_if<expr_arith>(&e)};

        if (arith == nullptr) {
            return std::nullopt;
        }

        const std::optional<int64_t> value{
            constant_arith_value(tc, *arith, width_type),
        };

        // the fold keeps the low bits of an operand that does not fit, which
        // only a conversion asks for
        if (value) {
            arith->assert_not_narrowed(tc, width_type);
        }

        return value;
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

        const token pos_tk{tz.cur_position_token()};

        // e.g. 'n = {}' is 'n = 0' and 'b = {}' is 'b = false'
        if (const token open_brace_tk{tz.is_next_char_token('{')};
            not open_brace_tk.is_empty()) {

            const token close_brace_tk{
                tz.expect_char_token(
                    '}', "expected '}', the zero value of a built-in is '{}'"),
            };

            return builtin_value(
                tc, tz, pos_tk, tp,
                std::make_unique<expr_zero>(tp, open_brace_tk, close_brace_tk));
        }

        // e.g. 'var x = i8' is 'var x = i8(0)' and 'var b = bool' is
        // 'var b = false'
        const token tk{tz.next_token()};

        if (is_bare_builtin_type(tc, tk, tz)) {
            if (typing == literal_typing::from_destination) {
                throw_known_type(tk, tk.text(), "{}");
            }

            return builtin_value(
                tc, tz, pos_tk, tp,
                std::make_unique<stmt_builtin_convert>(tc, tk));
        }

        // note: rewound by position, a string token cannot be put back
        tz.rewind_to_position(pos_tk);

        if (not tp.is_bool()) {
            return expr_arith{tc, tz};
        }

        expr_bool value{tc, tz.next_whitespace_token(), tz};

        value.assert_boolean_operands();

        return value;
    }
};
