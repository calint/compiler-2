#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
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
// rejected; it is folded at the parse into its own 'elem' tree rather than
// laid out by the 'write_plan' of a 'var', which is made at compile time and
// packs constant elements into bytes, where the data keeps typed lines, e.g.
// 'dd 2, 3, 5, 7'
class stmt_def_dat final : public statement {
    // the folded initializer at the type it fills, e.g. '3', '"hi"',
    // '{1, 2}' of an array or '{1, 2}' of an instance
    struct elem {
        // a value of a built-in type
        struct scalar {
            token src_loc_tk;
            int64_t value{};
        };

        // an 'i8' array filled with the bytes of a string, the rest zero
        struct string {
            token string_tk;
            size_t array_count{};
        };

        // the listed elements of an array, the rest zero
        struct elements {
            token src_loc_tk;
            size_t array_count{};
            std::vector<elem> items;
        };

        // the listed fields of an instance, the rest zero
        struct fields {
            token src_loc_tk;
            std::vector<elem> values;
        };

        // note: a scalar until the definition folds its initializer
        std::variant<scalar, string, elements, fields> form{scalar{}};

        [[nodiscard]] auto tok() const -> const token& {
            return std::visit(
                [](const auto& f) -> const token& {
                    if constexpr (requires { f.string_tk; }) {
                        return f.string_tk;
                    } else {
                        return f.src_loc_tk;
                    }
                },
                form);
        }
    };

    token name_tk_;
    token equals_tk_;
    expr_any initializer_;
    elem root_elem_{};

  public:
    stmt_def_dat(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, name_tk_{read_name(tc, tz)},
          equals_tk_{initializer_declaration::read_equals(tz, "dat")},
          initializer_{parse_initializer(tc, tz, name_tk_)},
          root_elem_{fold(tc, initializer_.get_type(), initializer_)} {

        set_type(initializer_.get_type());

        // register the data without emitting output so it is available
        // during subsequent parsing
        tc.add_dat(*this, tok(), make_var_info());
    }

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
        const array_shape shape{
            initializer_.is_array()
                ? array_shape::array(initializer_.array_count())
                : array_shape{},
        };

        return initializer_declaration::make_var_info(name_tk_, get_type(),
                                                      shape);
    }

    //
    // statics
    //

    // e.g. 'dat primes = []{2, 3, 5}'
    static auto compile_data_elements(machine& x, const type& tp,
                                      const elem::elements& list) -> void {

        std::vector<int64_t> values;
        values.reserve(list.items.size());

        for (const elem& item : list.items) {
            values.push_back(std::get<elem::scalar>(item.form).value);
        }

        // e.g. 'dat a = i8[4]' lists no element
        if (not values.empty()) {
            x.emit_data_array(tp.size_bytes(), values);
        }
    }

    static auto compile_data_rec(toc& tc, const type& tp, const elem& el)
        -> void {

        machine& x{tc.machine()};

        if (const elem::scalar* const value{
                std::get_if<elem::scalar>(&el.form),
            }) {

            x.comment(value->src_loc_tk, 0, "{}", tp.name());

            x.emit_data_array(tp.size_bytes(),
                              std::array<int64_t, 1>{value->value});

            return;
        }

        if (const elem::fields* const record{
                std::get_if<elem::fields>(&el.form),
            }) {

            compile_data_record(tc, tp, *record);
            return;
        }

        // only 'i8[]' can be initialized with a string
        if (const elem::string* const str{
                std::get_if<elem::string>(&el.form),
            }) {

            x.comment(str->string_tk, 0, "{}[{}]", tp.name(), str->array_count);
            compile_data_string(x, tp, *str);
            return;
        }

        const elem::elements& list{std::get<elem::elements>(el.form)};

        x.comment(list.src_loc_tk, 0, "{}[{}]", tp.name(), list.array_count);

        if (tp.is_builtin()) {
            compile_data_elements(x, tp, list);
        } else {
            for (const auto [i, e] : std::views::enumerate(list.items)) {
                x.comment(e.tok(), 0, "[{}]", i);
                compile_data_record(tc, tp, std::get<elem::fields>(e.form));
            }
        }

        compile_zero_elements(x, list.src_loc_tk, tp,
                              list.array_count - list.items.size());
    }

    static auto compile_data_record(toc& tc, const type& tp,
                                    const elem::fields& record) -> void {

        machine& x{tc.machine()};

        // bytes of the instance emitted so far, fields in order then padding
        size_t written_bytes{};

        const std::span<const type_field> flds{tp.fields()};
        for (const auto [e, f] : std::views::zip(record.values, flds)) {
            const size_t padding_bytes{f.offset - written_bytes};

            if (padding_bytes != 0) {
                x.comment(e.tok(), 0, "padding {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            written_bytes = f.offset + f.size_bytes;

            compile_data_rec(tc, f.type_ref(), e);
        }

        // zero out remaining fields and the padding after the last field
        const size_t remaining_bytes{tp.size_bytes() - written_bytes};

        if (remaining_bytes == 0) {
            return;
        }

        const std::string_view what{
            record.values.size() == flds.size() ? "padding"
                                                : "remaining fields",
        };

        x.comment(record.src_loc_tk, 0, "zero {}: {} B", what, remaining_bytes);
        x.emit_zero_data(remaining_bytes);
    }

    // e.g. 'dat greeting = "hello"'
    static auto compile_data_string(machine& x, const type& tp,
                                    const elem::string& str) -> void {

        x.emit_string_data(str.string_tk.string_bytes());
        const size_t size_bytes{str.string_tk.string_size_bytes()};

        // note: the parse rejected a string longer than its array
        assert(size_bytes <= str.array_count);

        compile_zero_elements(x, str.string_tk, tp,
                              str.array_count - size_bytes);
    }

    // the elements after the listed ones are zero, as in a 'var'
    static auto compile_zero_elements(machine& x, const token& src_loc_tk,
                                      const type& tp, const size_t count)
        -> void {

        if (count == 0) {
            return;
        }

        x.comment(src_loc_tk, 0, "pad {} {} of size {}", count,
                  in_quotes(tp.name()), tp.size_bytes());

        x.emit_zero_data(product_storage_size(tp.size_bytes(), count));
    }

    // the constants of 'value' at the type 'tp', e.g. '{1, 2 * 3}' of 'point'
    [[nodiscard]] static auto fold(const toc& tc, const type& tp,
                                   const expr_any& value) -> elem {

        if (not value.is_array()) {
            return fold_element(tc, tp, value, 0);
        }

        // e.g. 'dat b = a' copies the array 'a' at run time
        if (value.is_whole_array_copy()) {
            // note: the parse rejected a source that is not an array
            //       ('expr_type::parse_copy_source'), a dat folds right after
            //       its parse with the same names

            assert(make_ident_info(tc, *value.operand_path()).is_array());

            throw_not_constant(value);
        }

        // note: the declaration or the type of the field gave the size
        assert(value.array_count());

        // a string is emitted as it is written
        if (value.is_string()) {
            return {
                .form{
                    elem::string{
                        .string_tk{value.array_token()},
                        .array_count{*value.array_count()},
                    },
                },
            };
        }

        elem::elements list{
            .src_loc_tk{value.array_token()},
            .array_count{*value.array_count()},
            .items{},
        };

        for (size_t i{}; i < value.element_count(); ++i) {
            list.items.emplace_back(fold_element(tc, tp, value, i));
        }

        return {
            .form{std::move(list)},
        };
    }

    [[nodiscard]] static auto fold_element(const toc& tc, const type& tp,
                                           const expr_any& value,
                                           const size_t index) -> elem {

        const statement& element{value.element(index)};

        if (tp.is_builtin()) {
            const std::optional<int64_t> constant{
                folded_value::value_of(
                    value.fold_element(tc, index, tc.use_at(tp))),
            };

            if (not constant) {
                throw_not_constant(element);
            }

            return {
                .form{
                    elem::scalar{
                        .src_loc_tk{element.tok()},
                        .value{*constant},
                    },
                },
            };
        }

        // e.g. 'dat p = q' copies the instance 'q' at run time
        const expr_type& record{value.as_expr_type(index)};

        if (not record.is_literal()) {
            throw_not_constant(record);
        }

        elem::fields fields{
            .src_loc_tk{element.tok()},
            .values{},
        };

        const std::span<const type_field> flds{tp.fields()};

        for (const auto [f, field_value] :
             std::views::zip(flds, record.field_values())) {

            fields.values.emplace_back(fold(tc, f.type_ref(), *field_value));
        }

        return {
            .form{std::move(fields)},
        };
    }

    // the value at the type and size it deduces, the name is not in scope
    [[nodiscard]] static auto parse_initializer(toc& tc, tokenizer& tz,
                                                const token& name_tk)
        -> expr_any {

        const initializer_declaration declaration{
            tc,
            tz,
            name_tk,
            name_in_initializer::hidden,
        };

        return declaration.parse_value(tc, tz);
    }

    [[nodiscard]] static auto read_name(const toc& tc, tokenizer& tz) -> token {
        const token name_tk{tz.next_token()};

        tc.assert_valid_unqualified_name(name_tk);

        return name_tk;
    }

    [[noreturn]] static auto throw_not_constant(const statement& value)
        -> void {

        throw compile_error{
            value.tok(),
            std::format("{} is not a constant, a 'dat' holds constants",
                        in_quotes(statement::trimmed_source(value)))};
    }
};
