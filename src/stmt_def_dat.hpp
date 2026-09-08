#pragma once
// reviewed: 2025-09-28

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "null_stream.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "unary_ops.hpp"

class stmt_def_dat final : public statement {
    class elem {
      public:
        unary_ops uops;
        token tk;
        int64_t value{};
        token ws1; // whitespace before '{'
        token ws2; // whitespace after '{'
        token ws3; // whitespace before '}'
        token ws4; // whitespace after '}'
        bool is_array{};
        size_t array_size{};
        std::vector<elem> elems;
    };

    token name_tk_;
    token type_tk_;
    token array_size_tk_;
    token ws1_; // whitespace after ']'
    token ws2;  // whitespace after '='
    elem elroot_;
    bool has_init_{};

  public:
    stmt_def_dat(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (name_tk_.is_empty()) {
            throw compiler_exception(name_tk_, "expected name of data");
        }

        bool is_array{};
        size_t array_size{};

        // check if type declared
        if (tz.is_next_char(':')) {
            type_tk_ = tz.next_token();
            if (tz.is_next_char('[')) {
                is_array = true;
                array_size_tk_ = tz.next_token();
                if (const std::optional<int64_t> value{toc::parse_to_constant(
                        array_size_tk_, array_size_tk_.text())};
                    value) {
                    array_size = static_cast<size_t>(*value);
                }
                if (not tz.is_next_char(']')) {
                    throw compiler_exception{type_tk_,
                                             "expected array size and ']'"};
                }
                ws1_ = tz.next_whitespace_token();
            }
        }

        // get type reference from the token
        const type& tp{type_tk_.text().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.text())};
        set_type(tp);

        // expect initialization
        has_init_ = {tz.is_next_char('=')};

        ws2 = tz.next_whitespace_token();

        // add var to toc without causing output by passing a null stream
        null_stream null_strm;
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&tp},
            .declared_at_tk{name_tk_},
            .is_array{is_array},
            .array_size{array_size},
        };
        tc.add_var(name_tk_, null_strm, 0, var, true);

        if (has_init_) {
            elroot_ = parse_elem(tc, tz, tp, is_array, array_size);
        } else {
            elroot_.is_array = is_array;
            elroot_.array_size = array_size;
        }

        tc.add_dat(this);
    }

    ~stmt_def_dat() override = default;

    stmt_def_dat() = default;
    stmt_def_dat(const stmt_def_dat&) = delete;
    stmt_def_dat(stmt_def_dat&&) = default;
    auto operator=(const stmt_def_dat&) -> stmt_def_dat& = delete;
    auto operator=(stmt_def_dat&&) -> stmt_def_dat& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (not type_tk_.is_empty()) {
            std::print(os, ":");
            type_tk_.source_to(os);
            if (elroot_.is_array) {
                std::print(os, "[");
                array_size_tk_.source_to(os);
                std::print(os, "]");
                ws1_.source_to(os);
            }
        }

        // special case for string

        if (elroot_.tk.is_string()) {
            std::print(os, "=");
            ws2.source_to(os);
            elroot_.tk.source_to(os);
            return;
        }

        // normal case

        if (not has_init_) {
            return;
        }

        const type& tp{get_type()};
        std::print(os, "=");
        ws2.source_to(os);

        print_source_elem(os, tp, elroot_);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        tc.comment_source(*this, os, indent);
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .declared_at_tk{name_tk_},
            .is_array{elroot_.is_array},
            .array_size{elroot_.array_size},
        };
        tc.add_var(name_tk_, os, indent, var, true);
    }

    auto compile_data(const toc& tc, std::ostream& os) const -> void override {
        compile_data_rec(tc, os, name_tk_.text(), get_type(), elroot_);
    }

  private:
    static auto compile_data_rec(const toc& tc, std::ostream& os,
                                 const std::string_view nm, const type& tp,
                                 const elem& elroot) -> void {

        if (not elroot.is_array) {
            compile_data_elem(tc, os, nm, tp, elroot);
            return;
        }

        // array

        // special case for string

        if (tp.name() == "i8" and elroot.tk.is_string()) {
            compile_data_builtin(os, nm, tp, elroot);
            return;
        }

        // regular arrays

        std::println(os, "; {}: {}[{}]", nm, tp.name(), elroot.array_size);
        size_t counter{};
        for (const elem& el : elroot.elems) {
            std::println(os, "; [{}]", counter);
            compile_data_elem(tc, os, nm, tp, el);
            ++counter;
        }

        // zero out remaining array

        const size_t diff{elroot.array_size - counter};

        if (diff == 0) {
            return;
        }

        std::println(os, "; pad {} '{}' of size {}", diff, tp.name(),
                     tp.size());
        std::println(os, "times {} db 0", diff * tp.size());
    }

    static auto compile_data_elem(const toc& tc, std::ostream& os,
                                  const std::string_view& nm, const type& tp,
                                  const elem& elroot) -> void {

        if (tp.is_built_in()) {
            compile_data_builtin(os, nm, tp, elroot);
            return;
        }

        // user type

        const std::span<const type_field>& flds{tp.fields()};
        size_t counter{};
        for (const elem& el : elroot.elems) {
            const type_field& tf{flds[counter]};
            if (tf.type_ptr->is_built_in()) {
                compile_data_builtin(os, tf.name, *tf.type_ptr, el);
            } else {
                compile_data_rec(tc, os, tf.name, *tf.type_ptr, el);
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
        std::println(os, "; zero remaining fields");
        std::println(os, "times {} db 0", nbytes);
    }

    static auto compile_data_builtin(std::ostream& os,
                                     const std::string_view fldnm,
                                     const type& tp, const elem& elroot)
        -> void {

        // nasm define data token
        std::string_view dd{toc::get_data_def(tp.size())};
        if (not elroot.is_array) {
            std::println(os, "; {}: {}", fldnm, tp.name());
            if (elroot.tk.text().empty()) {
                std::println(os, "{} 0", dd);
                return;
            }

            std::print(os, "{} ", dd);
            elroot.uops.source_to_without_whitespace(os);
            std::println(os, "{}", elroot.value);
            return;
        }

        // array of built-ins

        std::println(os, "; {}: {}[{}]", fldnm, tp.name(), elroot.array_size);

        // special case for string

        if (tp.name() == "i8" and elroot.tk.is_string()) {
            std::print(os, "{} `", dd);
            elroot.tk.compile_to(os);
            std::println(os, "`");
            const size_t sz{elroot.tk.string_size_bytes()};
            // pad remaining array with 0
            if (elroot.array_size != 0 and sz < elroot.array_size) {
                std::println(os, "; zero remaining array");
                std::println(os, "times {} {} 0", elroot.array_size - sz, dd);
            }
            return;
        }

        // normal case

        // initializer
        std::print(os, "{} ", dd);
        int32_t counter{};
        for (const elem& e : elroot.elems) {
            if (counter++) {
                std::print(os, ", ");
            }
            e.uops.source_to_without_whitespace(os);
            std::print(os, "{}", e.value);
        }

        std::println(os);

        // pad remaining array with 0
        if (elroot.array_size != elroot.elems.size()) {
            std::println(os, "times {} {} 0",
                         elroot.array_size - elroot.elems.size(), dd);
        }
    }

    static auto parse_elem(const toc& tc, tokenizer& tz, const type& tp,
                           const bool is_array, const size_t array_size)
        -> elem {

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
                                         "only 'i8' arrays can be strings");
            }
            tz.put_back_token(el.tk);

            // normal case

            el.ws1 = tz.next_whitespace_token();
            if (not tz.is_next_char('{')) {
                throw compiler_exception(
                    tz, std::format(
                            "expected '{{' to open array initializer for '{}'",
                            tp.name()));
            }
            el.ws2 = tz.next_whitespace_token();

            size_t counter{};
            while (true) {
                el.elems.emplace_back(parse_builtin(tc, tz, tp));
                ++counter;
                if (not tz.is_next_char(',')) {
                    break;
                }
            }

            el.ws3 = tz.next_whitespace_token();
            if (not tz.is_next_char('}')) {
                throw compiler_exception(
                    tz, std::format(
                            "expected '}}' to close array initializer for '{}'",
                            tp.name()));
            }
            el.ws4 = tz.next_whitespace_token();

            if (el.array_size != 0 and el.elems.size() > el.array_size) {
                throw compiler_exception(
                    tz,
                    std::format("array size is {} but contains {} initializers",
                                el.array_size, el.elems.size()));
            }

            if (el.array_size == 0) {
                el.array_size = counter;
            }

            return el;
        }

        // user type array

        el.ws1 = tz.next_whitespace_token();
        if (not tz.is_next_char('{')) {
            throw compiler_exception(
                tz,
                std::format("expected '{{' to open array initializer for '{}'",
                            tp.name()));
        }
        el.ws2 = tz.next_whitespace_token();

        size_t counter{};
        while (true) {
            el.elems.emplace_back(parse_type(tc, tz, tp));
            ++counter;
            if (not tz.is_next_char(',')) {
                break;
            }
        }

        el.ws3 = tz.next_whitespace_token();
        if (not tz.is_next_char('}')) {
            throw compiler_exception(
                tz,
                std::format("expected '}}' to open array initializer for '{}'",
                            tp.name()));
        }
        el.ws4 = tz.next_whitespace_token();

        if (array_size != 0 and el.elems.size() > el.array_size) {
            throw compiler_exception(
                tz, std::format("array size is {} but contains {} initializers",
                                array_size, el.elems.size()));
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
                    std::format("boolean field '{}' must be true or false",
                                tp.name()));
            }
            return el;
        }

        const ident_info ii{tc.make_ident_info(el.tk, el.tk.text())};
        if (not ii.is_const()) {
            throw compiler_exception(
                el.tk, std::format("'{}' is not a constant", el.tk.text()));
        }
        el.value = ii.const_value;
        return el;
    }

    static auto parse_type(const toc& tc, tokenizer& tz, const type& tp)
        -> elem {

        elem el{};

        el.ws1 = tz.next_whitespace_token();
        if (not tz.is_next_char('{')) {
            throw compiler_exception(
                tz,
                std::format("expected '{{' to open type initializer for '{}'",
                            tp.name()));
        }
        el.ws2 = tz.next_whitespace_token();

        size_t counter{};
        const std::span<const type_field> flds{tp.fields()};
        while (true) {
            const token tk = tz.next_whitespace_token();
            if (tz.is_next_char('}')) {
                el.ws3 = tk;
                el.ws4 = tz.next_whitespace_token();
                break;
            }
            tz.put_back_token(tk);

            const type_field& tf{flds[counter]};

            // for (const type_field& tf : tp.fields()) {
            if (counter++) {
                if (not tz.is_next_char(',')) {
                    throw compiler_exception(
                        tz,
                        std::format("expected ',' and initializer for "
                                    "field '{}' in type '{}' of type '{}{}'",
                                    tf.name, tp.name(), tf.type_ptr->name(),
                                    tf.is_array ? "[]" : ""));
                }
            }
            el.elems.emplace_back(
                parse_elem(tc, tz, *tf.type_ptr, tf.is_array, tf.array_size));
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
            elroot.ws1.source_to(os);
            std::print(os, "{{");
            elroot.ws2.source_to(os);

            size_t counter{};
            for (const elem& e : elroot.elems) {
                if (counter++) {
                    std::print(os, ",");
                }
                e.ws2.source_to(os);
                e.uops.source_to(os);
                e.tk.source_to(os);
                e.ws3.source_to(os);
            }

            elroot.ws3.source_to(os);
            std::print(os, "}}");
            elroot.ws4.source_to(os);
            return;
        }

        // user type array

        print_source_type(os, tp, elroot);
    }

    static auto print_source_type(std::ostream& os, const type& tp,
                                  const elem& elroot) -> void {

        if (not elroot.is_array) {
            elroot.ws1.source_to(os);
            std::print(os, "{{");
            elroot.ws2.source_to(os);

            size_t counter{};
            const std::span<const type_field>& flds{tp.fields()};
            for (const elem& e : elroot.elems) {
                if (counter++) {
                    std::print(os, ",");
                }
                print_source_field(os, flds[counter - 1], e);
            }

            elroot.ws3.source_to(os);
            std::print(os, "}}");
            elroot.ws4.source_to(os);

            return;
        }

        // array

        elroot.ws1.source_to(os);
        std::print(os, "{{");
        elroot.ws2.source_to(os);

        size_t counter{};
        for (const elem& e : elroot.elems) {
            if (counter++) {
                std::print(os, ",");
            }
            print_source_elem(os, tp, e);
        }

        elroot.ws3.source_to(os);
        std::print(os, "}}");
        elroot.ws4.source_to(os);
    }

    static auto print_source_field(std::ostream& os, const type_field& tf,
                                   const elem& elroot) -> void {

        if (tf.type_ptr->is_built_in()) {
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

            elroot.ws1.source_to(os);
            std::print(os, "{{");
            elroot.ws2.source_to(os);

            size_t counter{};
            for (const elem& e : elroot.elems) {
                if (counter++) {
                    std::print(os, ",");
                }
                e.uops.source_to(os);
                e.tk.source_to(os);
            }

            elroot.ws3.source_to(os);
            std::print(os, "}}");
            elroot.ws4.source_to(os);

            return;
        }

        // user type

        print_source_type(os, *tf.type_ptr, elroot);
    }
};
