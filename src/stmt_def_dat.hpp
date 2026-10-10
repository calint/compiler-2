#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <ostream>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_const.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "unary_ops.hpp"

class stmt_def_dat final : public statement {
    struct elem {
        unary_ops uops;
        token src_loc_tk;
        int64_t value{};
        token open_brace_tk;
        token close_brace_tk;
        bool is_array{};
        size_t array_count{};
        std::vector<elem> elems;
        std::vector<token> elem_delims_tk;

        auto source_to(std::ostream& os) const -> void {
            uops.source_to(os);
            src_loc_tk.source_to(os);
        }
    };

    token name_tk_;
    token equals_tk_;
    token type_tk_;
    token open_bracket_tk_;
    stmt_const array_count_const_;
    token close_bracket_tk_;
    token open_paren_tk_;
    token close_paren_tk_;
    elem elroot_;

  public:
    // e.g. 'dat x = i32(0)', the initializer gives the type
    stmt_def_dat(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, name_tk_{tz.next_token()} {

        if (name_tk_.text().empty()) {
            throw compiler_exception{name_tk_, "expected name of data"};
        }

        toc::assert_valid_name(name_tk_);

        equals_tk_ = parse_initializer_equals(tz, "dat");

        elroot_ = parse_initializer(tc, tz);

        // register the data without emitting output so it is available
        // during subsequent parsing
        tc.add_var(name_tk_, 0, make_var_info(), var_kind::dat);

        tc.add_dat(this);
    }

    stmt_def_dat() = default;
    // 'toc' keeps the address of the definition
    stmt_def_dat(const stmt_def_dat&) = delete;
    stmt_def_dat(stmt_def_dat&&) = delete;
    auto operator=(const stmt_def_dat&) -> stmt_def_dat& = delete;
    auto operator=(stmt_def_dat&&) -> stmt_def_dat& = delete;

    ~stmt_def_dat() override = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);
        type_tk_.source_to(os);

        if (not open_bracket_tk_.is_empty()) {
            open_bracket_tk_.source_to(os);
            array_count_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }

        open_paren_tk_.source_to(os);
        print_source_elem(os, get_type(), elroot_);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        tc.add_var(name_tk_, indent, make_var_info(), var_kind::dat);
    }

    auto compile_data(toc& tc) const -> void override {
        machine& x{tc.machine()};

        x.comment(name_tk_, 0, name_tk_.text());
        compile_data_rec(tc, get_type(), elroot_);
    }

    [[nodiscard]] auto dat_size_bytes() const -> size_t override {
        return multiply_storage_size(name_tk_, get_type().size_bytes(),
                                     elroot_.is_array ? elroot_.array_count
                                                      : 1);
    }

  private:
    [[nodiscard]] auto make_var_info() const -> var_info {
        return {
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .src_loc_tk{name_tk_},
            .is_array{elroot_.is_array},
            .array_len{elroot_.array_count},
            .pointer_register{},
            .base_register{},
            .value_register{},
            .foo_array{},
        };
    }

    // e.g. 'i8[4]{1, 2}', '[]{1, 2}' or 'point[2]{{1, 2}}'
    [[nodiscard]] auto parse_array_literal(toc& tc, tokenizer& tz) -> elem {
        open_bracket_tk_ = tz.is_next_char_token('[');
        const size_t array_count{parse_array_size(tc, tz)};

        // the default type has no type token to locate the data comments
        const token src_loc_tk{type_tk_.is_empty() ? open_bracket_tk_
                                                   : type_tk_};

        // e.g. 'dat a = i8[4]' is 'i8[4]{}'
        const bool is_bare{tz.peek_char_after_whitespace() != '{'};

        elem el{
            is_bare ? make_empty_array(src_loc_tk, array_count)
                    : parse_array(tc, tz, src_loc_tk, get_type(), array_count),
        };

        if (el.array_count == 0) {
            throw compiler_exception{open_bracket_tk_,
                                     "empty arrays require a specified size"};
        }

        return el;
    }

    // e.g. '[4]', or '[]' when the initializer gives the size, which is 0
    [[nodiscard]] auto parse_array_size(toc& tc, tokenizer& tz) -> size_t {
        array_count_const_ = {tc, tz, 0};

        if (array_count_const_.has_value() and
            array_count_const_.value() <= 0) {

            throw compiler_exception{
                array_count_const_.tok(),
                "expected array size to be greater than 0"};
        }

        close_bracket_tk_ =
            tz.expect_char_token(']', "expected ']' after array size");

        return static_cast<size_t>(array_count_const_.value());
    }

    // e.g. 'i8(3)' or 'int(3)'
    [[nodiscard]] auto parse_conversion(const toc& tc, tokenizer& tz) -> elem {
        open_paren_tk_ = tz.is_next_char_token('(');

        elem el{parse_builtin(tc, tz, get_type())};

        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the argument");

        return el;
    }

    // e.g. 'dat s = "hi"' is an 'i8' array of 2, 'dat a = i8[4]{1, 2}' an 'i8'
    // array of 4, 'dat a = [4]{1, 2}' a default type array of 4, 'dat p =
    // point{1, 2}' a 'point', 'dat x = i8(3)' an 'i8', 'dat b = true' a 'bool'
    // and 'dat n = 3' has the default type
    [[nodiscard]] auto parse_initializer(toc& tc, tokenizer& tz) -> elem {
        // e.g. 'dat greeting = "hello"'
        if (tz.peek_char_after_whitespace() == '"') {
            const token string_tk{tz.next_token()};
            set_type(tc.get_type_i8());

            elem el{};
            el.is_array = true;
            el.src_loc_tk = string_tk;

            el.array_count =
                string_array_count(tc.get_type_i8(), string_tk, get_type(), 0);

            return el;
        }

        // e.g. 'dat primes = []{2, 3, 5}'
        if (is_default_array_literal(tz)) {
            set_type(tc.get_type_default());
            return parse_array_literal(tc, tz);
        }

        // an instance initializer cannot deduce its type, e.g. 'dat p = {1, 2}'
        if (const token brace_tk{tz.is_next_char_token('{')};
            not brace_tk.is_empty()) {

            throw compiler_exception{
                brace_tk, "expected type name before '{', e.g. 'point{1, 2}'"};
        }

        const token tk{tz.next_token()};

        // e.g. 'dat bytes = i8[]{1, 2}' or 'dat points = point[2]{{1, 2}, {3,
        // 4}}'
        if (is_array_literal(tc, tk, tz)) {
            set_named_type(tc, tk);
            return parse_array_literal(tc, tz);
        }

        if (is_bare_record_type(tc, tk, tz)) {
            set_named_type(tc, tk);

            // e.g. 'dat p = point' is 'point{}'
            elem el{};
            el.src_loc_tk = tk;

            return el;
        }

        // e.g. 'dat p = point{1, 2}'
        if (is_record_literal(tc, tk, tz)) {
            set_named_type(tc, tk);
            return parse_type(tc, tz, get_type());
        }

        // e.g. 'dat x = i8' is 'i8(0)', the token is printed as the value
        if (is_bare_builtin_type(tc, tk, tz)) {
            set_type(stmt_builtin_convert::conversion_type(tc, tk));

            elem el{};
            el.src_loc_tk = tk;

            return el;
        }

        // e.g. 'dat x = i8(3)'
        if (tc.is_integer_type_name(tk.text()) and
            tz.peek_char_after_whitespace() == '(') {

            type_tk_ = tk;
            set_type(stmt_builtin_convert::conversion_type(tc, tk));
            return parse_conversion(tc, tz);
        }

        // e.g. 'dat limit = 10' or 'dat enabled = true'
        const bool is_bool{
            tk.is_text(reserved_names::true_value) or
                tk.is_text(reserved_names::false_value),
        };

        set_type(is_bool ? tc.get_type_bool() : tc.get_type_default());

        // the constant may start with unary operations, e.g. '-1'
        tz.put_back_token(tk);

        return parse_builtin(tc, tz, get_type());
    }

    // the type 'tk' names, printed from 'tk' when the statement is reproduced
    auto set_named_type(const toc& tc, const token& tk) -> void {
        type_tk_ = tk;
        set_type(tc.get_type_or_throw(tk, tk.text()));
    }

    //
    // statics
    //

    // the '}' that ends the initializer, when it is the next character, e.g.
    // the '}' of '{1, 2}'
    [[nodiscard]] static auto at_closing_brace(tokenizer& tz, elem& el)
        -> bool {

        el.close_brace_tk = tz.is_next_char_token('}');
        return not el.close_brace_tk.is_empty();
    }

    static auto compile_data_builtin(toc& tc, const type& tp,
                                     const elem& elroot) -> void {

        machine& x{tc.machine()};

        const std::optional<std::string> size_error{
            x.data_element_size_error(tp.size_bytes()),
        };

        if (size_error) {
            throw compiler_exception{elroot.src_loc_tk, *size_error};
        }

        if (not elroot.is_array) {
            x.comment(elroot.src_loc_tk, 0, "{}", tp.name());

            x.emit_data(tp.size_bytes(), {
                                             .value{elroot.value},
                                             .uops{elroot.uops.to_string()},
                                         });

            return;
        }

        x.comment(elroot.src_loc_tk, 0, "{}[{}]", tp.name(),
                  elroot.array_count);

        // only 'i8[]' can be initialized with a string token
        if (elroot.src_loc_tk.is_string()) {
            compile_data_string(x, tp, elroot);
            return;
        }

        compile_data_elements(x, tp, elroot);
    }

    static auto compile_data_elem(toc& tc, const type& tp, const elem& elroot)
        -> void {

        if (tp.is_builtin()) {
            compile_data_builtin(tc, tp, elroot);
            return;
        }

        // user-defined type

        machine& x{tc.machine()};

        // bytes of the instance emitted so far, fields in order then padding
        size_t written_bytes{};

        const std::span<const type_field> flds{tp.fields()};
        for (const auto [e, f] : std::views::zip(elroot.elems, flds)) {
            const size_t padding_bytes{f.offset - written_bytes};

            if (padding_bytes != 0) {
                x.comment(e.src_loc_tk, 0, "padding {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            written_bytes = f.offset + f.size_bytes;

            if (f.type().is_builtin()) {
                compile_data_builtin(tc, f.type(), e);
                continue;
            }

            compile_data_rec(tc, f.type(), e);
        }

        // zero out remaining fields and the padding after the last field

        const size_t size_bytes{tp.size_bytes() - written_bytes};

        if (size_bytes == 0) {
            return;
        }

        const std::string_view what{elroot.elems.size() == flds.size()
                                        ? "padding"
                                        : "remaining fields"};

        x.comment(elroot.src_loc_tk, 0, "zero {}: {} B", what, size_bytes);
        x.emit_zero_data(size_bytes);
    }

    // e.g. 'dat primes = []{2, 3, 5}'
    static auto compile_data_elements(machine& x, const type& tp,
                                      const elem& elroot) -> void {

        const auto values{
            elroot.elems |
                std::views::transform(
                    [](const elem& element) -> machine::data_initializer {
                        return {
                            .value{element.value},
                            .uops{element.uops.to_string()},
                        };
                    }),
        };

        x.emit_data_array(tp.size_bytes(), values);

        // pad remaining array with 0
        if (elroot.array_count != elroot.elems.size()) {
            x.emit_repeated_data(tp.size_bytes(),
                                 elroot.array_count - elroot.elems.size(), {});
        }
    }

    static auto compile_data_rec(toc& tc, const type& tp, const elem& elroot)
        -> void {

        if (not elroot.is_array) {
            compile_data_elem(tc, tp, elroot);
            return;
        }

        // array

        // special case for a string
        // note: only i8[] can be initialized with a string token

        if (elroot.src_loc_tk.is_string()) {
            compile_data_builtin(tc, tp, elroot);
            return;
        }

        // regular arrays

        machine& x{tc.machine()};

        x.comment(elroot.src_loc_tk, 0, "{}[{}]", tp.name(),
                  elroot.array_count);

        for (const auto [i, e] : std::views::enumerate(elroot.elems)) {
            x.comment(e.src_loc_tk, 0, "[{}]", i);
            compile_data_elem(tc, tp, e);
        }

        // zero out the remaining array elements

        const size_t remaining_count{elroot.array_count - elroot.elems.size()};

        if (remaining_count == 0) {
            return;
        }

        x.comment(elroot.src_loc_tk, 0, "pad {} '{}' of size {}",
                  remaining_count, tp.name(), tp.size_bytes());

        x.emit_zero_data(multiply_storage_size(
            elroot.src_loc_tk, tp.size_bytes(), remaining_count));
    }

    // e.g. 'dat greeting = "hello"'
    static auto compile_data_string(machine& x, const type& tp,
                                    const elem& elroot) -> void {

        x.emit_string_data(elroot.src_loc_tk.string_text());
        const size_t size_bytes{elroot.src_loc_tk.string_size_bytes()};

        // pad remaining array with 0
        assert(elroot.array_count != 0);

        if (size_bytes < elroot.array_count) {
            x.comment(elroot.src_loc_tk, 0, "zero remaining array");

            x.emit_repeated_data(tp.size_bytes(),
                                 elroot.array_count - size_bytes, {});
        }
    }

    // an array without elements, the elements are parsed into it
    [[nodiscard]] static auto make_empty_array(const token src_loc_tk,
                                               const size_t array_count)
        -> elem {

        elem el{};
        el.is_array = true;
        el.array_count = array_count;

        el.src_loc_tk = src_loc_tk;

        return el;
    }

    // '{' elements '}', empty zeroes a sized array
    [[nodiscard]] static auto parse_array(const toc& tc, tokenizer& tz,
                                          const token src_loc_tk,
                                          const type& tp,
                                          const size_t array_count) -> elem {

        elem el{make_empty_array(src_loc_tk, array_count)};

        el.open_brace_tk = tz.expect_char_token(
            '{', std::format("expected '{{' to open array initializer for '{}'",
                             tp.name()));

        el.close_brace_tk = tz.is_next_char_token('}');

        if (el.close_brace_tk.is_empty()) {
            parse_array_elements(tc, tz, tp, el);
            el.close_brace_tk = tz.is_next_char_token('}');
        }

        if (el.close_brace_tk.is_empty()) {
            throw compiler_exception{
                tz,
                std::format("expected '}}' to close array initializer for '{}'",
                            tp.name())};
        }

        if (el.array_count == 0) {
            el.array_count = el.elems.size();
        }

        return el;
    }

    // a sized array rejects elements past its size
    static auto parse_array_elements(const toc& tc, tokenizer& tz,
                                     const type& tp, elem& el) -> void {

        while (true) {
            el.elems.emplace_back(tp.is_builtin() ? parse_builtin(tc, tz, tp)
                                                  : parse_type(tc, tz, tp));

            const token delim_tk{tz.is_next_char_token(',')};

            if (delim_tk.is_empty()) {
                return;
            }

            const size_t count{el.elems.size()};

            if (el.array_count != 0 and count == el.array_count) {
                throw compiler_exception{
                    delim_tk,
                    std::format("expected '}}' after {} element{} in array of "
                                "size {}",
                                count, count == 1 ? "" : "s", el.array_count)};
            }

            el.elem_delims_tk.emplace_back(delim_tk);
        }
    }

    [[nodiscard]] static auto parse_bool_value(const token& tk, const type& tp)
        -> int64_t {

        if (tk.is_text(reserved_names::true_value)) {
            return 1;
        }

        if (tk.is_text(reserved_names::false_value)) {
            return 0;
        }

        throw compiler_exception{
            tk, std::format("boolean field '{}' must be 'true' or 'false'",
                            tp.name())};
    }

    [[nodiscard]] static auto parse_builtin(const toc& tc, tokenizer& tz,
                                            const type& tp) -> elem {

        elem el{};
        el.uops = unary_ops{tz};
        el.src_loc_tk = tz.next_token();

        // e.g. '{ 1 }' or a trailing ',' where a single value is expected
        if (el.src_loc_tk.text().empty() or el.src_loc_tk.is_string()) {
            throw compiler_exception{
                el.src_loc_tk,
                std::format("expected a constant for '{}'", tp.name())};
        }

        if (tp.is_same(tc.get_type_bool())) {
            el.value = parse_bool_value(el.src_loc_tk, tp);
            return el;
        }

        const ident_info info{
            tc.make_ident_info(el.src_loc_tk, el.src_loc_tk.text()),
        };

        if (not info.is_const()) {
            throw compiler_exception{
                el.src_loc_tk,
                std::format("'{}' must be a constant", el.src_loc_tk.text())};
        }

        el.value = info.const_value;

        // the assembler would truncate the value or warn about it
        const int64_t value{el.uops.evaluate_constant(el.value)};

        if (fits_size_bytes(value, tp.size_bytes())) {
            return el;
        }

        throw compiler_exception{
            el.src_loc_tk,
            std::format("constant '{}{}' does not fit '{}'",
                        el.uops.to_string(), el.src_loc_tk.text(), tp.name())};
    }

    [[nodiscard]] static auto parse_elem(const toc& tc, tokenizer& tz,
                                         const token src_loc_tk, const type& tp,
                                         const bool is_array,
                                         const size_t array_count) -> elem {

        if (not is_array and tp.is_builtin()) {
            return parse_builtin(tc, tz, tp);
        }

        // user-defined type
        if (not is_array) {
            return parse_type(tc, tz, tp);
        }

        // array, a string is the special case
        if (not tp.is_builtin()) {
            return parse_array(tc, tz, src_loc_tk, tp, array_count);
        }

        const token tk{tz.next_token()};

        if (not tk.is_string()) {
            tz.put_back_token(tk);
            return parse_array(tc, tz, src_loc_tk, tp, array_count);
        }

        elem el{};
        el.is_array = true;
        el.src_loc_tk = tk;

        el.array_count =
            string_array_count(tc.get_type_i8(), tk, tp, array_count);

        return el;
    }

    // the ',' before the initializer of the field 'next', e.g. the ',' of
    // '{1, 2}' before the '2'
    static auto parse_field_delimiter(tokenizer& tz, const type& tp,
                                      const type_field& next, elem& el)
        -> void {

        const token tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by an initializer "
                                 "for field '{}' of type '{}{}' in type '{}'",
                                 next.name, next.type().name(),
                                 next.is_array ? "[]" : "", tp.name())),
        };

        el.elem_delims_tk.emplace_back(tk);
    }

    [[nodiscard]] static auto parse_type(const toc& tc, tokenizer& tz,
                                         const type& tp) -> elem {

        const tokenizer::nesting_scope nesting{tz};

        elem el{};
        el.src_loc_tk = tz.cur_position_token();

        // e.g. the '{1, 2}' of 'dat p = point{1, 2}'
        el.open_brace_tk = tz.expect_char_token(
            '{', std::format("expected '{{' to open type initializer for '{}'",
                             tp.name()));

        const std::span<const type_field> flds{tp.fields()};

        while (not at_closing_brace(tz, el)) {
            // each field adds one element, e.g. the '1' and the '2' of '{1, 2}'
            const size_t index{el.elems.size()};

            if (index == flds.size()) {
                // the error is at the initializer that is too much
                std::ignore = tz.is_next_char_token(',');

                throw compiler_exception{
                    tz, std::format("too many initializers for type '{}'",
                                    tp.name())};
            }

            const type_field& tf{flds.at(index)};

            if (index > 0) {
                parse_field_delimiter(tz, tp, tf, el);
            }

            el.elems.emplace_back(parse_elem(tc, tz, tz.cur_position_token(),
                                             tf.type(), tf.is_array,
                                             tf.array_count));
        }

        return el;
    }

    // '{' items separated by their delimiters '}'
    static auto print_source_braced(
        std::ostream& os, const elem& elroot,
        const std::function_ref<void(size_t, const elem&)> print_item) -> void {

        elroot.open_brace_tk.source_to(os);
        for (size_t i{}; i < elroot.elems.size(); ++i) {
            if (i != 0) {
                elroot.elem_delims_tk.at(i - 1).source_to(os);
                // note: -1 because there is one delimiter fewer than elements
            }

            print_item(i, elroot.elems.at(i));
        }

        elroot.close_brace_tk.source_to(os);
    }

    static auto print_source_elem(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not tp.is_builtin()) {
            print_source_type(os, tp, elroot);
            return;
        }

        if (not elroot.is_array) {
            elroot.source_to(os);
            return;
        }

        // special case for string
        if (elroot.src_loc_tk.is_string()) {
            elroot.src_loc_tk.source_to(os);
            return;
        }

        print_source_braced(
            os, elroot,
            [&os](const size_t, const elem& e) -> void { e.source_to(os); });
    }

    static auto print_source_type(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not elroot.is_array) {
            print_source_braced(
                os, elroot, [&os, &tp](const size_t i, const elem& e) -> void {
                    const type_field& tf{tp.fields().at(i)};
                    print_source_elem(os, tf.type(), e);
                });

            return;
        }

        print_source_braced(os, elroot,
                            [&os, &tp](const size_t, const elem& e) -> void {
                                print_source_elem(os, tp, e);
                            });
    }
};
