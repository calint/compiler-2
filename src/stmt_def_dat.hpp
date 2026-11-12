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
#include <string>
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

        // an 'i8' array filled with the bytes of a string, the rest zero;
        // 'src_loc_tk' is the string literal
        struct string {
            token src_loc_tk;
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
                [](const auto& f) -> const token& { return f.src_loc_tk; },
                form);
        }
    };

    token name_tk_;
    token equals_tk_;
    // the type and size the initializer gives, the name is not in scope
    initializer_declaration declaration_;
    expr_any initializer_;
    elem root_elem_{};

  public:
    stmt_def_dat(parse_toc& tc, const token& src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk},
          name_tk_{initializer_declaration::read_name(tc, tz)},
          equals_tk_{initializer_declaration::read_equals(tz, "dat", name_tk_)},
          declaration_{tc, tz, name_tk_, name_in_initializer::hidden},
          initializer_{declaration_.parse_value(tc, tz)},
          root_elem_{fold_data(tc, declaration_.type_ref(), initializer_)} {

        set_type(declaration_.type_ref());

        // register the data without emitting output so it is available
        // during subsequent parsing
        tc.add_dat(*this, declaration_.declared_var());
    }

    // 'data_table' keeps the address of the definition
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

    auto compile(compile_toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_source(tc);

        tc.add_var(declaration_.declared_var(), var_kind::dat);

        // note: the compile of a 'var' checks the elements of a condition
        //       after the one that decides it, so does this

        initializer_.check_folded_conditions(tc);
    }

    //
    // class methods
    //

    // the initial values, in the data section
    auto compile_data(compile_toc& tc) const -> void {
        machine& x{tc.machine()};

        x.comment({name_tk_, 0}, name_tk_.text());
        compile_data_elem(x, get_type(), root_elem_);
    }

  private:
    //
    // statics
    //

    // an element of any form at the type 'tp', a field or an element of it
    // in turn
    static auto compile_data_elem(machine& x, const type& tp, const elem& el)
        -> void {

        if (const elem::scalar* const value{
                std::get_if<elem::scalar>(&el.form),
            }) {

            x.comment({value->src_loc_tk, 0}, "{}", tp.name());

            x.emit_data_array(tp.size_bytes(),
                              std::array<int64_t, 1>{value->value});

            return;
        }

        if (const elem::fields* const record{
                std::get_if<elem::fields>(&el.form),
            }) {

            compile_data_record(x, tp, *record);
            return;
        }

        // only 'i8[]' can be initialized with a string
        if (const elem::string* const str{
                std::get_if<elem::string>(&el.form),
            }) {

            compile_data_string(x, tp, *str);
            return;
        }

        compile_data_list(x, tp, std::get<elem::elements>(el.form));
    }

    // the listed elements of a built-in type on one data line, e.g. 'dat
    // primes = []{2, 3, 5}'
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

    // the listed elements then zeros up to the length of the array
    static auto compile_data_list(machine& x, const type& tp,
                                  const elem::elements& list) -> void {

        x.comment({list.src_loc_tk, 0}, "{}[{}]", tp.name(), list.array_count);

        if (tp.is_builtin()) {
            compile_data_elements(x, tp, list);
        } else {
            for (const auto [i, e] : std::views::enumerate(list.items)) {
                x.comment({e.tok(), 0}, "[{}]", i);
                compile_data_record(x, tp, std::get<elem::fields>(e.form));
            }
        }

        compile_zero_elements(x, list.src_loc_tk, tp,
                              list.array_count - list.items.size());
    }

    static auto compile_data_record(machine& x, const type& tp,
                                    const elem::fields& record) -> void {

        // bytes of the instance emitted so far, fields in order then padding
        size_t written_bytes{};

        const std::span<const type_field> flds{tp.fields()};

        for (const auto [e, f] : std::views::zip(record.values, flds)) {
            const size_t padding_bytes{f.offset - written_bytes};

            if (padding_bytes != 0) {
                x.comment({e.tok(), 0}, "zero padding: {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            written_bytes = f.offset + f.size_bytes;

            compile_data_elem(x, f.type_ref(), e);
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

        x.comment({record.src_loc_tk, 0}, "zero {}: {} B", what,
                  remaining_bytes);

        x.emit_zero_data(remaining_bytes);
    }

    // the bytes of the string then zeros up to the length of the array
    static auto compile_data_string(machine& x, const type& tp,
                                    const elem::string& str) -> void {

        x.comment({str.src_loc_tk, 0}, "{}[{}]", tp.name(), str.array_count);

        const std::string bytes{str.src_loc_tk.string_bytes()};

        // note: the parse rejected a string longer than its array
        assert(bytes.size() <= str.array_count);

        x.emit_string_data(bytes);

        compile_zero_elements(x, str.src_loc_tk, tp,
                              str.array_count - bytes.size());
    }

    // the elements after the listed ones are zero, as in a 'var'
    static auto compile_zero_elements(machine& x, const token& src_loc_tk,
                                      const type& tp, const size_t count)
        -> void {

        if (count == 0) {
            return;
        }

        const size_t size_bytes{product_storage_size(tp.size_bytes(), count)};

        x.comment({src_loc_tk, 0}, "zero remaining elements: {} * {} B = {} B",
                  count, tp.size_bytes(), size_bytes);

        x.emit_zero_data(size_bytes);
    }

    // the constants of 'value' at the type 'tp', e.g. '{1, 2 * 3}' of 'point'
    [[nodiscard]] static auto fold_data(const toc& tc, const type& tp,
                                        const expr_any& value) -> elem {

        if (not value.is_array()) {
            return fold_data_value(tc, tp, value);
        }

        // e.g. 'dat b = a', a copy of the array 'a', which only the run time
        // makes
        if (value.is_whole_array_copy()) {
            // note: the parse rejected a source that is not an array
            //       ('expr_path::parse_whole_array'), a dat folds right after
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
                        .src_loc_tk{value.array_token()},
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

        for (const expr_any& listed : value.elements()) {
            list.items.emplace_back(fold_data(tc, tp, listed));
        }

        return {
            .form{std::move(list)},
        };
    }

    // the constants of the single value 'value' at the type 'tp'
    [[nodiscard]] static auto fold_data_value(const toc& tc, const type& tp,
                                              const expr_any& value) -> elem {

        const statement& element{value.single()};

        if (tp.is_builtin()) {
            const std::optional<folded_value> folded{
                element.fold_checked(tc, tc.use_at(tp)),
            };

            const std::optional<int64_t> constant{
                folded_value::value_of(folded),
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

        // e.g. 'dat p = q', a copy of the instance 'q', which only the run time
        // makes
        const expr_type* const record{value.instance_literal()};

        if (record == nullptr) {
            throw_not_constant(element);
        }

        elem::fields fields{
            .src_loc_tk{element.tok()},
            .values{},
        };

        const std::span<const type_field> flds{tp.fields()};

        for (const auto [f, field_value] :
             std::views::zip(flds, record->field_values())) {

            fields.values.emplace_back(
                fold_data(tc, f.type_ref(), field_value));
        }

        return {
            .form{std::move(fields)},
        };
    }

    [[noreturn]] static auto throw_not_constant(const statement& value)
        -> void {

        throw compile_error{
            value.tok(),
            std::format("{} is not a constant, a 'dat' holds constants",
                        in_quotes(trimmed_source(value)))};
    }
};
