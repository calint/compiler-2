#pragma once
// reviewed: 2025-09-28

#include <format>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_const.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "unary_ops.hpp"

class stmt_def_dat final : public statement {
    struct elem {
        unary_ops uops;
        token tk;
        int64_t value{};
        token open_brace_tk_;
        token close_brace_tk_;
        bool is_array{};
        size_t array_count{};
        std::vector<elem> elems;
        std::vector<token> elem_delims_tk_;

        auto source_to(std::ostream& os) const -> void {
            uops.source_to(os);
            tk.source_to(os);
        }
    };

    token name_tk_;
    token type_tk_;
    token open_bracket_tk_;
    stmt_const array_count_const_;
    token close_bracket_tk_;
    token equals_tk_;
    elem elroot_;
    bool has_init_{};

  public:
    stmt_def_dat(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (name_tk_.is_empty()) {
            throw compiler_exception{name_tk_, "expected name of data"};
        }

        bool is_array{};
        size_t array_count{};

        open_bracket_tk_ = tz.is_next_char_token('[');
        if (not open_bracket_tk_.is_empty()) {
            is_array = true;

            array_count_const_ = {tc, tz, 0};

            if (array_count_const_.has_value() and
                array_count_const_.value() <= 0) {

                throw compiler_exception{
                    array_count_const_.tok(),
                    "expected array size to be greater than 0"};
            }

            array_count = static_cast<size_t>(array_count_const_.value());

            close_bracket_tk_ = tz.is_next_char_token(']');
            if (close_bracket_tk_.is_empty()) {
                throw compiler_exception{open_bracket_tk_,
                                         "expected ']' after array size"};
            }
        }

        type_tk_ = tz.next_token();
        if (not tc.has_type(type_tk_.text())) {
            tz.put_back_token(type_tk_);
            type_tk_ = {};
        }

        // get type reference from the token
        const type& tp{type_tk_.text().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.text())};

        set_type(tp);

        // expect initialization
        equals_tk_ = tz.is_next_char_token('=');
        has_init_ = not equals_tk_.is_empty();

        // register the variable without emitting output so it is available
        // during subsequent parsing

        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&tp},
            .src_loc_tk{name_tk_},
            .is_array{is_array},
            .array_len{array_count},
            .reg{},
            .base_register{},
        };

        tc.add_var(name_tk_, 0, var, true);

        if (has_init_) {
            elroot_ = parse_elem(tc, tz, type_tk_, tp, is_array, array_count);
            if (elroot_.is_array and elroot_.array_count == 0 and
                not elroot_.tk.is_string() and elroot_.elems.empty()) {

                throw compiler_exception{name_tk_,
                                         "empty arrays require a specified "
                                         "size"};
            }
        } else {
            elroot_.is_array = is_array;
            elroot_.array_count = array_count;
        }

        tc.add_dat(this);
    }

    stmt_def_dat() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (elroot_.is_array) {
            open_bracket_tk_.source_to(os);
            array_count_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
        if (not type_tk_.is_empty()) {
            type_tk_.source_to(os);
        }

        // special case for a string

        if (elroot_.tk.is_string()) {
            equals_tk_.source_to(os);
            elroot_.tk.source_to(os);

            return;
        }

        // normal case

        if (not has_init_) {
            return;
        }

        equals_tk_.source_to(os);

        const type& tp{get_type()};
        print_source_elem(os, tp, elroot_);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .src_loc_tk{name_tk_},
            .is_array{elroot_.is_array},
            .array_len{elroot_.array_count},
            .reg{},
            .base_register{},
        };

        tc.add_var(name_tk_, indent, var, true);
    }

    auto compile_data(toc& tc) const -> void override {
        machine& x{tc.machine()};

        x.comment(name_tk_, 0, name_tk_.text());
        compile_data_rec(tc, get_type(), elroot_);
    }

    [[nodiscard]] auto dat_size_bytes() const -> size_t override {
        return multiply_storage_size(get_type().size_bytes(),
                                     elroot_.is_array ? elroot_.array_count
                                                      : 1);
    }

  private:
    static auto compile_data_rec(toc& tc, const type& tp, const elem& elroot)
        -> void {

        if (not elroot.is_array) {
            compile_data_elem(tc, tp, elroot);

            return;
        }

        // array

        // special case for a string
        // note: only i8[] can be initialized with a string token

        if (elroot.tk.is_string()) {
            compile_data_builtin(tc, tp, elroot);

            return;
        }

        // regular arrays

        machine& x{tc.machine()};

        x.comment(elroot.tk, 0, "{}[{}]", tp.name(), elroot.array_count);

        for (const auto [i, e] : std::views::enumerate(elroot.elems)) {
            x.comment(e.tk, 0, "[{}]", i);
            compile_data_elem(tc, tp, e);
        }

        // zero out the remaining array elements

        const size_t remaining_count{elroot.array_count - elroot.elems.size()};

        if (remaining_count == 0) {
            return;
        }

        x.comment(elroot.tk, 0, "pad {} '{}' of size {}", remaining_count,
                  tp.name(), tp.size_bytes());

        x.emit_zero_data(
            multiply_storage_size(tp.size_bytes(), remaining_count));
    }

    static auto compile_data_elem(toc& tc, const type& tp, const elem& elroot)
        -> void {

        if (tp.is_builtin()) {
            compile_data_builtin(tc, tp, elroot);

            return;
        }

        // user-defined type

        machine& x{tc.machine()};

        // bytes of the record emitted so far, fields in order then padding
        size_t written_bytes{};

        const std::span<const type_field> flds{tp.fields()};
        for (const auto [e, f] : std::views::zip(elroot.elems, flds)) {
            const size_t padding_bytes{f.offset - written_bytes};
            if (padding_bytes != 0) {
                x.comment(e.tk, 0, "padding {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            written_bytes = f.offset + f.size_bytes;

            if (f.type().is_builtin()) {
                compile_data_builtin(tc, f.type(), e);
            } else {
                compile_data_rec(tc, f.type(), e);
            }
        }

        // zero out remaining fields and the padding after the last field

        const size_t size_bytes{tp.size_bytes() - written_bytes};
        if (size_bytes == 0) {
            return;
        }

        const std::string_view what{elroot.elems.size() == flds.size()
                                        ? "padding"
                                        : "remaining fields"};

        x.comment(elroot.tk, 0, "zero {}: {} B", what, size_bytes);
        x.emit_zero_data(size_bytes);
    }

    static auto compile_data_builtin(toc& tc, const type& tp,
                                     const elem& elroot) -> void {

        machine& x{tc.machine()};

        if (not elroot.is_array) {
            x.comment(elroot.tk, 0, "{}", tp.name());
            if (elroot.tk.text().empty()) {
                x.emit_data(tp.size_bytes(), {});

                return;
            }

            x.emit_data(tp.size_bytes(), {
                                             .value{elroot.value},
                                             .uops{elroot.uops.to_string()},
                                         });

            return;
        }

        // array of built-ins

        x.comment(elroot.tk, 0, "{}[{}]", tp.name(), elroot.array_count);

        // special case for string
        // note: only i8[] can be initialized with string token

        if (elroot.tk.is_string()) {
            x.emit_string_data(elroot.tk.text());
            const size_t size_bytes{elroot.tk.string_size_bytes()};
            // pad remaining array with 0
            if (elroot.array_count != 0 and size_bytes < elroot.array_count) {
                x.comment(elroot.tk, 0, "zero remaining array");
                x.emit_repeated_data(tp.size_bytes(),
                                     elroot.array_count - size_bytes, {});
            }

            return;
        }

        // normal case

        // initializer
        const auto values{
            elroot.elems |
            std::views::transform(
                [](const elem& element) -> machine::data_initializer {
                    return {
                        .value{element.value},
                        .uops{element.uops.to_string()},
                    };
                })};

        x.emit_data_array(tp.size_bytes(), values);

        // pad remaining array with 0
        if (elroot.array_count != elroot.elems.size()) {
            x.emit_repeated_data(tp.size_bytes(),
                                 elroot.array_count - elroot.elems.size(), {});
        }
    }

    [[nodiscard]] static auto parse_elem(const toc& tc, tokenizer& tz,
                                         const token src_loc_tk, const type& tp,
                                         const bool is_array,
                                         const size_t array_count) -> elem {

        if (not is_array) {
            if (tp.is_builtin()) {
                return parse_builtin(tc, tz, tp);
            }

            // user-defined type

            return parse_type(tc, tz, tp);
        }

        // array

        elem el{};
        el.is_array = is_array;
        el.array_count = array_count;

        if (tp.is_builtin()) {
            // special case for string
            el.tk = tz.next_token();
            if (el.tk.is_string()) {
                el.array_count = string_array_count(el.tk, tp, el.array_count);

                return el;
            }
            tz.put_back_token(el.tk);

            // normal case

            el.tk = src_loc_tk;
            // note: 'el.tk' is not part of data but is used for source location
            //       at compile

            el.open_brace_tk_ = tz.is_next_char_token('{');
            if (el.open_brace_tk_.is_empty()) {
                throw compiler_exception{
                    tz, std::format(
                            "expected '{{' to open array initializer for '{}'",
                            tp.name())};
            }

            size_t counter{};
            el.close_brace_tk_ = tz.is_next_char_token('}');
            if (el.close_brace_tk_.is_empty()) {
                while (true) {
                    el.elems.emplace_back(parse_builtin(tc, tz, tp));
                    ++counter;
                    const token t{tz.is_next_char_token(',')};
                    if (t.is_empty()) {
                        el.close_brace_tk_ = tz.is_next_char_token('}');
                        break;
                    }
                    if (el.array_count != 0 and counter == el.array_count) {
                        throw compiler_exception{
                            t,
                            std::format("expected '}}' after {} element{} in "
                                        "array of size {}",
                                        counter, counter == 1 ? "" : "s",
                                        el.array_count)};
                    }
                    el.elem_delims_tk_.emplace_back(t);
                }
            }

            if (el.close_brace_tk_.is_empty()) {
                throw compiler_exception{
                    tz, std::format(
                            "expected '}}' to close array initializer for '{}'",
                            tp.name())};
            }

            if (el.array_count == 0) {
                el.array_count = counter;
            }

            return el;
        }

        // array of a user-defined type

        el.tk = src_loc_tk;
        // note: 'el.tk' is not part of data but is used for source location
        //       at compile

        el.open_brace_tk_ = tz.is_next_char_token('{');
        if (el.open_brace_tk_.is_empty()) {
            throw compiler_exception{
                tz,
                std::format("expected '{{' to open array initializer for '{}'",
                            tp.name())};
        }

        size_t counter{};
        while (true) {
            el.elems.emplace_back(parse_type(tc, tz, tp));
            ++counter;
            const token tk{tz.is_next_char_token(',')};
            if (tk.is_empty()) {
                break;
            }
            if (array_count != 0 and counter == array_count) {
                throw compiler_exception{
                    tk,
                    std::format("expected '}}' after {} element{} in array of "
                                "size {}",
                                counter, counter == 1 ? "" : "s", array_count)};
            }
            el.elem_delims_tk_.emplace_back(tk);
        }

        el.close_brace_tk_ = tz.is_next_char_token('}');
        if (el.close_brace_tk_.is_empty()) {
            throw compiler_exception{
                tz,
                std::format("expected '}}' to close array initializer for '{}'",
                            tp.name())};
        }

        if (array_count == 0) {
            el.array_count = counter;
        }

        return el;
    }

    [[nodiscard]] static auto parse_builtin(const toc& tc, tokenizer& tz,
                                            const type& tp) -> elem {

        elem el{};
        el.uops = unary_ops{tz};
        el.tk = tz.next_token();
        if (&tp == &tc.get_type_bool()) {
            if (el.tk.is_text("true")) {
                el.value = 1;
            } else if (el.tk.is_text("false")) {
                el.value = 0;
            } else {
                throw compiler_exception{
                    el.tk,
                    std::format("boolean field '{}' must be 'true' or 'false'",
                                tp.name())};
            }

            return el;
        }

        const ident_info ii{tc.make_ident_info(el.tk, el.tk.text())};
        if (not ii.is_const()) {
            throw compiler_exception{
                el.tk, std::format("'{}' must be a constant", el.tk.text())};
        }
        el.value = ii.const_value;

        return el;
    }

    [[nodiscard]] static auto parse_type(const toc& tc, tokenizer& tz,
                                         const type& tp) -> elem {

        elem el{};
        el.tk = tz.cur_position_token();
        el.open_brace_tk_ = tz.is_next_char_token('{');
        if (el.open_brace_tk_.is_empty()) {
            throw compiler_exception{
                tz,
                std::format("expected '{{' to open type initializer for '{}'",
                            tp.name())};
        }

        const std::span<const type_field> flds{tp.fields()};
        size_t counter{};
        while (true) {
            el.close_brace_tk_ = tz.is_next_char_token('}');
            if (not el.close_brace_tk_.is_empty()) {
                break;
            }

            if (counter == flds.size()) {
                throw compiler_exception{
                    tz, std::format("too many initializers for type '{}'",
                                    tp.name())};
            }

            const type_field& tf{flds[counter]};

            if (counter++) {
                const token tk{tz.is_next_char_token(',')};
                if (tk.is_empty()) {
                    throw compiler_exception{
                        tz, std::format(
                                "expected ',' followed by an initializer "
                                "for field '{}' of type '{}{}' in type '{}'",
                                tf.name, tf.type().name(),
                                tf.is_array ? "[]" : "", tp.name())};
                }
                el.elem_delims_tk_.emplace_back(tk);
            }

            el.elems.emplace_back(parse_elem(tc, tz, tz.cur_position_token(),
                                             tf.type(), tf.is_array,
                                             tf.array_count));
        }

        return el;
    }

    static auto print_source_elem(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not elroot.is_array) {
            if (tp.is_builtin()) {
                elroot.source_to(os);

                return;
            }

            // user-defined type

            print_source_type(os, tp, elroot);

            return;
        }

        // array

        if (tp.is_builtin()) {
            print_source_builtin_array(os, elroot);

            return;
        }

        // array of a user-defined type

        print_source_type(os, tp, elroot);
    }

    static auto print_source_builtin_array(std::ostream& os, const elem& elroot)
        -> void {

        elroot.open_brace_tk_.source_to(os);
        if (not elroot.elems.empty()) {
            elroot.elems.front().source_to(os);
            for (const auto [d, e] :
                 std::views::zip(elroot.elem_delims_tk_,
                                 elroot.elems | std::views::drop(1))) {

                d.source_to(os);
                e.source_to(os);
            }
        }
        elroot.close_brace_tk_.source_to(os);
    }

    static auto print_source_type(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not elroot.is_array) {
            elroot.open_brace_tk_.source_to(os);
            if (not elroot.elems.empty()) {
                print_source_field(os, tp.fields().front(),
                                   elroot.elems.front());

                for (const auto [d, e, f] :
                     std::views::zip(elroot.elem_delims_tk_,
                                     elroot.elems | std::views::drop(1),
                                     tp.fields() | std::views::drop(1))) {

                    d.source_to(os);
                    print_source_field(os, f, e);
                }
            }
            elroot.close_brace_tk_.source_to(os);

            return;
        }

        // array

        elroot.open_brace_tk_.source_to(os);
        if (not elroot.elems.empty()) {
            print_source_elem(os, tp, elroot.elems.front());
            for (const auto [d, e] :
                 std::views::zip(elroot.elem_delims_tk_,
                                 elroot.elems | std::views::drop(1))) {

                d.source_to(os);
                print_source_elem(os, tp, e);
            }
        }
        elroot.close_brace_tk_.source_to(os);
    }

    static auto print_source_field(std::ostream& os, const type_field& tf,
                                   const elem& elroot) -> void {

        if (tf.type().is_builtin()) {
            if (not tf.is_array) {
                elroot.source_to(os);

                return;
            }

            // array

            // special case for string

            if (elroot.tk.is_string()) {
                elroot.tk.source_to(os);

                return;
            }

            // normal case

            print_source_builtin_array(os, elroot);

            return;
        }

        // user-defined type

        print_source_type(os, tf.type(), elroot);
    }
};
