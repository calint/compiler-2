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
#include "null_stream.hpp"
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
        size_t array_size{};
        std::vector<elem> elems;
        std::vector<token> elems_delim_tk_;

        auto source_to(std::ostream& os) const -> void {
            uops.source_to(os);
            tk.source_to(os);
        }

        auto compile(x86& x) const -> void {
            x.dat_value(std::format("{}{}", uops.to_string(), value));
        }
    };

    token name_tk_;
    token type_delim_tk_;
    token type_tk_;
    token open_bracket_tk_;
    stmt_const array_size_const_;
    token close_bracket_tk_;
    token equals_tk_;
    elem elroot_;
    bool has_init_{};

  public:
    stmt_def_dat(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()},
          type_delim_tk_{tz.is_next_char_token(':')} {

        if (name_tk_.is_empty()) {
            throw compiler_exception(name_tk_, "expected name of data");
        }

        bool is_array{};
        size_t array_size{};

        // check whether a type is declared
        if (not type_delim_tk_.is_empty()) {
            type_tk_ = tz.next_token();

            open_bracket_tk_ = tz.is_next_char_token('[');
            if (not open_bracket_tk_.is_empty()) {
                is_array = true;

                array_size_const_ = {tc, tz, 0};

                if (array_size_const_.has_value() and
                    array_size_const_.value() <= 0) {

                    throw compiler_exception{
                        array_size_const_.tok(),
                        "expected array size to be greater than 0"};
                }

                array_size = static_cast<size_t>(array_size_const_.value());

                close_bracket_tk_ = tz.is_next_char_token(']');
                if (close_bracket_tk_.is_empty()) {
                    throw compiler_exception{type_tk_,
                                             "expected ']' after array size"};
                }
            }
        }

        // get type reference from the token
        const type& tp{type_tk_.text().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.text())};

        set_type(tp);

        // expect initialization
        equals_tk_ = tz.is_next_char_token('=');
        has_init_ = not equals_tk_.is_empty();

        // add var to toc without emitting output so the further parsing has the
        // variable declared

        null_stream null_strm;
        x86 x{null_strm, tc.source()};

        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&tp},
            .declared_at_tk{name_tk_},
            .is_array{is_array},
            .array_size{array_size},
            .reg{},
        };

        tc.add_var(x, name_tk_, 0, var, true);

        if (has_init_) {
            elroot_ = parse_elem(tc, tz, type_tk_, tp, is_array, array_size);
            if (elroot_.is_array and elroot_.array_size == 0 and
                not elroot_.tk.is_string() and elroot_.elems.empty()) {

                throw compiler_exception{name_tk_,
                                         "empty arrays require a specified "
                                         "size"};
            }
        } else {
            elroot_.is_array = is_array;
            elroot_.array_size = array_size;
        }

        tc.add_dat(this);
    }

    stmt_def_dat() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (not type_tk_.is_empty()) {
            type_delim_tk_.source_to(os);
            type_tk_.source_to(os);
            if (elroot_.is_array) {
                open_bracket_tk_.source_to(os);
                array_size_const_.source_to(os);
                close_bracket_tk_.source_to(os);
            }
        }

        // special case for string

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

    auto compile(toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .declared_at_tk{name_tk_},
            .is_array{elroot_.is_array},
            .array_size{elroot_.array_size},
            .reg{},
        };

        tc.add_var(x, name_tk_, indent, var, true);
    }

    auto compile_data(const toc& tc, x86& x) const -> void override {
        x.comment(name_tk_, 0, name_tk_.text());
        compile_data_rec(tc, x, get_type(), elroot_);
    }

    [[nodiscard]] auto dat_size_bytes() const -> size_t override {
        return get_type().size() * (elroot_.is_array ? elroot_.array_size : 1);
    }

  private:
    static auto compile_data_rec(const toc& tc, x86& x, const type& tp,
                                 const elem& elroot) -> void {

        if (not elroot.is_array) {
            compile_data_elem(tc, x, tp, elroot);
            return;
        }

        // array

        // special case for string
        // note: only i8[] can be initialized with string token

        if (elroot.tk.is_string()) {
            compile_data_builtin(x, tp, elroot);
            return;
        }

        // regular arrays

        x.comment(elroot.tk, 0, "{}[{}]", tp.name(), elroot.array_size);

        size_t counter{};
        for (const elem& el : elroot.elems) {
            x.comment(el.tk, 0, "[{}]", counter);
            compile_data_elem(tc, x, tp, el);
            ++counter;
        }

        // zero out remaining array

        const size_t diff{elroot.array_size - counter};

        if (diff == 0) {
            return;
        }

        x.comment(elroot.tk, 0, "pad {} '{}' of size {}", diff, tp.name(),
                  tp.size());

        x.times(diff * tp.size(), "db", "0");
    }

    static auto compile_data_elem(const toc& tc, x86& x, const type& tp,
                                  const elem& elroot) -> void {

        if (tp.is_built_in()) {
            compile_data_builtin(x, tp, elroot);
            return;
        }

        // user type

        const std::span<const type_field>& flds{tp.fields()};
        size_t counter{};
        for (const elem& el : elroot.elems) {
            const type_field& tf{flds[counter]};
            if (tf.type().is_built_in()) {
                compile_data_builtin(x, tf.type(), el);
            } else {
                compile_data_rec(tc, x, tf.type(), el);
            }
            ++counter;
        }

        // zero out remaining fields, if any

        const size_t n{flds.size()};
        const size_t diff{n - counter};
        if (diff == 0) {
            return;
        }
        size_t nbytes{};
        for (size_t i{counter}; i < n; ++i) {
            nbytes += flds[i].size;
        }
        x.comment(elroot.tk, 0, "zero remaining fields");
        x.times(nbytes, "db", "0");
    }

    static auto compile_data_builtin(x86& x, const type& tp, const elem& elroot)
        -> void {

        // NASM data directive for this element size
        const std::string_view dd{x86::get_data_def(tp.size())};
        if (not elroot.is_array) {
            x.comment(elroot.tk, 0, "{}", tp.name());
            if (elroot.tk.text().empty()) {
                x.dat_begin(tp.size());
                x.dat_value("0");
                x.dat_end();
                return;
            }

            x.dat_begin(tp.size());
            x.dat_value(
                std::format("{}{}", elroot.uops.to_string(), elroot.value));

            x.dat_end();
            return;
        }

        // array of built-ins

        x.comment(elroot.tk, 0, "{}[{}]", tp.name(), elroot.array_size);

        // special case for string
        // note: only i8[] can be initialized with string token

        if (elroot.tk.is_string()) {
            x.str_begin();
            x.str_value(elroot.tk.text());
            x.str_end();
            const size_t sz{elroot.tk.string_size_bytes()};
            // pad remaining array with 0
            if (elroot.array_size != 0 and sz < elroot.array_size) {
                x.comment(elroot.tk, 0, "zero remaining array");
                x.times(elroot.array_size - sz, dd, "0");
            }
            return;
        }

        // normal case

        // initializer
        x.dat_begin(tp.size());
        if (not elroot.elems.empty()) {
            elroot.elems.front().compile(x);
            for (const elem& e : elroot.elems | std::views::drop(1)) {
                x.dat_separator();
                e.compile(x);
            }
        }
        x.dat_end();

        // pad remaining array with 0
        if (elroot.array_size != elroot.elems.size()) {
            x.times(elroot.array_size - elroot.elems.size(), dd, "0");
        }
    }

    static auto parse_elem(const toc& tc, tokenizer& tz, const token src_loc_tk,
                           const type& tp, const bool is_array,
                           const size_t array_size) -> elem {

        if (not is_array) {
            if (tp.is_built_in()) {
                return parse_builtin(tc, tz, tp);
            }

            // user type

            return parse_type(tc, tz, tp);
        }

        // array

        elem el{};
        el.is_array = is_array;
        el.array_size = array_size;

        if (tp.is_built_in()) {
            // special case for string
            el.tk = tz.next_token();
            if (el.tk.is_string()) {
                const size_t strsz{el.tk.string_size_bytes()};
                if (strsz == 0 and el.array_size == 0) {
                    throw compiler_exception{
                        el.tk, "an empty string is not valid for an array "
                               "with unspecified size"};
                }
                if (el.array_size == 0) {
                    el.array_size = strsz;
                } else {
                    if (strsz > el.array_size) {
                        throw compiler_exception(
                            el.tk, std::format(
                                       "string size {} overflows array size {}",
                                       strsz, el.array_size));
                    }
                }
                if (tp.name() == "i8") {
                    return el;
                }
                throw compiler_exception(el.tk,
                                         "only arrays of type 'i8' can be "
                                         "initialized with strings");
            }
            tz.put_back_token(el.tk);

            // normal case

            el.tk = src_loc_tk;
            // note: 'el.tk' is not part of data but is used for source location
            //       at compile

            el.open_brace_tk_ = tz.is_next_char_token('{');
            if (el.open_brace_tk_.is_empty()) {
                throw compiler_exception(
                    tz, std::format(
                            "expected '{{' to open array initializer for '{}'",
                            tp.name()));
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
                    if (el.array_size != 0 and counter == el.array_size) {
                        throw compiler_exception{
                            t,
                            std::format("expected '}}' after {} element{} in "
                                        "array of size {}",
                                        counter, counter == 1 ? "" : "s",
                                        el.array_size)};
                    }
                    el.elems_delim_tk_.emplace_back(t);
                }
            }

            if (el.close_brace_tk_.is_empty()) {
                throw compiler_exception(
                    tz, std::format(
                            "expected '}}' to close array initializer for '{}'",
                            tp.name()));
            }

            if (el.array_size == 0) {
                el.array_size = counter;
            }

            return el;
        }

        // user type array

        el.tk = src_loc_tk;
        // note: 'el.tk' is not part of data but is used for source location
        //       at compile

        el.open_brace_tk_ = tz.is_next_char_token('{');
        if (el.open_brace_tk_.is_empty()) {
            throw compiler_exception(
                tz,
                std::format("expected '{{' to open array initializer for '{}'",
                            tp.name()));
        }

        size_t counter{};
        while (true) {
            el.elems.emplace_back(parse_type(tc, tz, tp));
            ++counter;
            const token tk{tz.is_next_char_token(',')};
            if (tk.is_empty()) {
                break;
            }
            if (array_size != 0 and counter == array_size) {
                throw compiler_exception{
                    tk,
                    std::format("expected '}}' after {} element{} in array of "
                                "size {}",
                                counter, counter == 1 ? "" : "s", array_size)};
            }
            el.elems_delim_tk_.emplace_back(tk);
        }

        el.close_brace_tk_ = tz.is_next_char_token('}');
        if (el.close_brace_tk_.is_empty()) {
            throw compiler_exception(
                tz,
                std::format("expected '}}' to close array initializer for '{}'",
                            tp.name()));
        }

        if (array_size == 0) {
            el.array_size = counter;
        }

        return el;
    }

    static auto parse_builtin(const toc& tc, tokenizer& tz, const type& tp)
        -> elem {

        elem el{};
        el.uops = unary_ops{tz};
        el.tk = tz.next_token();
        if (&tp == &tc.get_type_bool()) {
            if (el.tk.is_text("true")) {
                el.value = 1;
            } else if (el.tk.is_text("false")) {
                el.value = 0;
            } else {
                throw compiler_exception(
                    el.tk,
                    std::format("boolean field '{}' must be 'true' or 'false'",
                                tp.name()));
            }
            return el;
        }

        const ident_info ii{tc.make_ident_info_parsing(el.tk, el.tk.text())};
        if (not ii.is_const()) {
            throw compiler_exception(
                el.tk, std::format("'{}' must be a constant", el.tk.text()));
        }
        el.value = ii.const_value;
        return el;
    }

    static auto parse_type(const toc& tc, tokenizer& tz, const type& tp)
        -> elem {

        elem el{};
        el.tk = tz.current_position_token();
        el.open_brace_tk_ = tz.is_next_char_token('{');
        if (el.open_brace_tk_.is_empty()) {
            throw compiler_exception(
                tz,
                std::format("expected '{{' to open type initializer for '{}'",
                            tp.name()));
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
                    throw compiler_exception(
                        tz, std::format(
                                "expected ',' followed by an initializer "
                                "for field '{}' of type '{}{}' in type '{}'",
                                tf.name, tf.type().name(),
                                tf.is_array ? "[]" : "", tp.name()));
                }
                el.elems_delim_tk_.emplace_back(tk);
            }

            el.elems.emplace_back(
                parse_elem(tc, tz, tz.current_position_token(), tf.type(),
                           tf.is_array, tf.array_size));
        }

        return el;
    }

    static auto print_source_elem(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not elroot.is_array) {
            if (tp.is_built_in()) {
                elroot.uops.source_to(os);
                elroot.tk.source_to(os);
                return;
            }

            // user type

            print_source_type(os, tp, elroot);
            return;
        }

        // array

        if (tp.is_built_in()) {
            elroot.open_brace_tk_.source_to(os);
            if (not elroot.elems.empty()) {
                elroot.elems.front().source_to(os);
                for (const auto [d, e] :
                     std::views::zip(elroot.elems_delim_tk_,
                                     elroot.elems | std::views::drop(1))) {

                    d.source_to(os);
                    e.source_to(os);
                }
            }
            elroot.close_brace_tk_.source_to(os);
            return;
        }

        // user type array

        print_source_type(os, tp, elroot);
    }

    static auto print_source_type(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not elroot.is_array) {
            elroot.open_brace_tk_.source_to(os);
            if (not elroot.elems.empty()) {
                print_source_field(os, tp.fields().front(),
                                   elroot.elems.front());

                for (const auto [d, e, f] :
                     std::views::zip(elroot.elems_delim_tk_,
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
                 std::views::zip(elroot.elems_delim_tk_,
                                 elroot.elems | std::views::drop(1))) {

                d.source_to(os);
                print_source_elem(os, tp, e);
            }
        }
        elroot.close_brace_tk_.source_to(os);
    }

    static auto print_source_field(std::ostream& os, const type_field& tf,
                                   const elem& elroot) -> void {

        if (tf.type().is_built_in()) {
            if (not tf.is_array) {
                elroot.uops.source_to(os);
                elroot.tk.source_to(os);
                return;
            }

            // array

            // special case for string

            if (elroot.tk.is_string()) {
                elroot.tk.source_to(os);
                return;
            }

            // normal case

            elroot.open_brace_tk_.source_to(os);
            if (not elroot.elems.empty()) {
                elroot.elems.front().source_to(os);
                for (const auto [d, e] :
                     std::views::zip(elroot.elems_delim_tk_,
                                     elroot.elems | std::views::drop(1))) {

                    d.source_to(os);
                    e.source_to(os);
                }
            }
            elroot.close_brace_tk_.source_to(os);
            return;
        }

        // user type

        print_source_type(os, tf.type(), elroot);
    }
};
