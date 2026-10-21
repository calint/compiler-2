#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "expr_type.hpp"
#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_def_var.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

// e.g. 'dat x = i32(0)': the initializer is parsed like the one of a 'var' and
// folded to the constants of the data section, a value that is not constant is
// rejected
class stmt_def_dat final : public statement {
    // the folded initializer: a built-in value, the elements of an array, the
    // fields of an instance or a string
    struct elem {
        token src_loc_tk;
        int64_t value{};
        bool is_array{};
        bool is_string{};
        size_t array_count{};
        std::vector<elem> elems;
    };

    token name_tk_;
    token equals_tk_;
    expr_any initializer_;
    elem root_elem_;

  public:
    stmt_def_dat(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, name_tk_{tz.next_token()} {

        toc::assert_valid_unqualified_name(name_tk_);

        equals_tk_ = initializer_declaration::parse_equals(tz, "dat");

        const initializer_declaration declaration{
            tc,
            tz,
            name_tk_,
            name_in_initializer::hidden,
        };

        set_type(declaration.type_ref());

        initializer_ = declaration.parse_value(tc, tz);
        root_elem_ = fold(tc, get_type(), initializer_);

        // register the data without emitting output so it is available
        // during subsequent parsing
        tc.add_dat(*this, tok(), make_var_info());
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
        initializer_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        tc.add_var(name_tk_, indent, make_var_info(), var_kind::dat);
    }

    //
    // class methods
    //

    // the initial values, in the data section
    auto compile_data(toc& tc) const -> void {
        machine& x{tc.machine()};

        x.comment(name_tk_, 0, name_tk_.text());
        compile_data_rec(tc, get_type(), root_elem_);
    }

  private:
    [[nodiscard]] auto make_var_info() const -> var_info {
        const std::optional<size_t> array_count{
            root_elem_.is_array ? std::optional{root_elem_.array_count}
                                : std::nullopt,
        };

        return initializer_declaration::make_var_info(name_tk_, get_type(),
                                                      array_count);
    }

    //
    // statics
    //

    static auto compile_data_builtin(toc& tc, const type& tp, const elem& el)
        -> void {

        machine& x{tc.machine()};

        const std::optional<std::string> size_error{
            x.data_element_size_error(tp.size_bytes()),
        };

        if (size_error) {
            throw compile_error{el.src_loc_tk, *size_error};
        }

        if (not el.is_array) {
            x.comment(el.src_loc_tk, 0, "{}", tp.name());

            x.emit_data(tp.size_bytes(), el.value);

            return;
        }

        x.comment(el.src_loc_tk, 0, "{}[{}]", tp.name(), el.array_count);

        // only 'i8[]' can be initialized with a string
        if (el.is_string) {
            compile_data_string(x, tp, el);
            return;
        }

        compile_data_elements(x, tp, el);
    }

    // e.g. 'dat primes = []{2, 3, 5}'
    static auto compile_data_elements(machine& x, const type& tp,
                                      const elem& el) -> void {

        const auto values{
            el.elems |
                std::views::transform([](const elem& element) -> int64_t {
                    return element.value;
                }),
        };

        // e.g. 'dat a = i8[4]' lists no element
        if (not el.elems.empty()) {
            x.emit_data_array(tp.size_bytes(), values);
        }

        // the elements not listed are zero, as in a 'var'
        if (el.array_count != el.elems.size()) {
            x.emit_repeated_data(tp.size_bytes(),
                                 el.array_count - el.elems.size(), 0);
        }
    }

    static auto compile_data_rec(toc& tc, const type& tp, const elem& el)
        -> void {

        // a built-in value, or the values of a built-in array
        if (tp.is_builtin()) {
            compile_data_builtin(tc, tp, el);
            return;
        }

        if (not el.is_array) {
            compile_data_record(tc, tp, el);
            return;
        }

        machine& x{tc.machine()};

        x.comment(el.src_loc_tk, 0, "{}[{}]", tp.name(), el.array_count);

        for (const auto [i, e] : std::views::enumerate(el.elems)) {
            x.comment(e.src_loc_tk, 0, "[{}]", i);
            compile_data_record(tc, tp, e);
        }

        // zero out the remaining array elements
        const size_t remaining_count{el.array_count - el.elems.size()};

        if (remaining_count == 0) {
            return;
        }

        x.comment(el.src_loc_tk, 0, "pad {} '{}' of size {}", remaining_count,
                  tp.name(), tp.size_bytes());

        x.emit_zero_data(multiply_storage_size(el.src_loc_tk, tp.size_bytes(),
                                               remaining_count));
    }

    static auto compile_data_record(toc& tc, const type& tp, const elem& el)
        -> void {

        machine& x{tc.machine()};

        // bytes of the instance emitted so far, fields in order then padding
        size_t written_bytes{};

        const std::span<const type_field> flds{tp.fields()};
        for (const auto [e, f] : std::views::zip(el.elems, flds)) {
            const size_t padding_bytes{f.offset - written_bytes};

            if (padding_bytes != 0) {
                x.comment(e.src_loc_tk, 0, "padding {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            written_bytes = f.offset + f.size_bytes;

            compile_data_rec(tc, f.type(), e);
        }

        // zero out remaining fields and the padding after the last field
        const size_t size_bytes{tp.size_bytes() - written_bytes};

        if (size_bytes == 0) {
            return;
        }

        const std::string_view what{
            el.elems.size() == flds.size() ? "padding" : "remaining fields",
        };

        x.comment(el.src_loc_tk, 0, "zero {}: {} B", what, size_bytes);
        x.emit_zero_data(size_bytes);
    }

    // e.g. 'dat greeting = "hello"'
    static auto compile_data_string(machine& x, const type& tp, const elem& el)
        -> void {

        x.emit_string_data(el.src_loc_tk.string_text());
        const size_t size_bytes{el.src_loc_tk.string_size_bytes()};

        // the bytes after the text are zero, as in a 'var'
        if (size_bytes < el.array_count) {
            x.comment(el.src_loc_tk, 0, "zero remaining array");

            x.emit_repeated_data(tp.size_bytes(), el.array_count - size_bytes,
                                 0);
        }
    }

    // the constants of 'value' at the type 'tp', e.g. '{1, 2 * 3}' of 'point'
    [[nodiscard]] static auto fold(const toc& tc, const type& tp,
                                   const expr_any& value) -> elem {

        if (not value.is_array()) {
            return fold_element(tc, tp, value, 0);
        }

        // e.g. 'dat b = a' copies the array 'a' at run time
        if (value.is_array_identifier()) {
            const ident_info source{tc.make_ident_info(value.element(0))};

            // e.g. the '5' of 't{5, 1}' for an array field
            if (not source.is_array()) {
                throw compile_error{value.tok(), "source must be an array"};
            }

            throw_not_constant(value);
        }

        elem el{
            .src_loc_tk{value.array_token()},
            .value{},
            .is_array{true},
            .is_string{value.is_string()},
            .array_count{value.array_count()},
            .elems{},
        };

        // a string is emitted as it is written
        if (el.is_string) {
            return el;
        }

        for (size_t i{}; i < value.element_count(); ++i) {
            el.elems.emplace_back(fold_element(tc, tp, value, i));
        }

        return el;
    }

    [[nodiscard]] static auto fold_element(const toc& tc, const type& tp,
                                           const expr_any& value,
                                           const size_t index) -> elem {

        const statement& element{value.element(index)};

        elem el{
            .src_loc_tk{element.tok()},
            .value{},
            .is_array{},
            .is_string{},
            .array_count{},
            .elems{},
        };

        if (tp.is_builtin()) {
            const std::optional<int64_t> constant{
                value.element_constant(tc, index, tp),
            };

            if (not constant) {
                throw_not_constant(element);
            }

            el.value = *constant;

            return el;
        }

        // e.g. 'dat p = q' copies the instance 'q' at run time
        const expr_type& record{value.as_expr_type(index)};

        if (not record.is_literal()) {
            throw_not_constant(record);
        }

        const std::span<const type_field> flds{tp.fields()};

        for (const auto [f, field_value] :
             std::views::zip(flds, record.field_values())) {

            el.elems.emplace_back(fold(tc, f.type(), *field_value));
        }

        return el;
    }

    [[noreturn]] static auto throw_not_constant(const statement& value)
        -> void {

        throw compile_error{
            value.tok(),
            std::format("'{}' is not a constant, a 'dat' holds constants",
                        statement::trimmed_source(value))};
    }
};
