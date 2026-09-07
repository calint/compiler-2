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
    static constexpr size_t threshold_for_rep_stos{32};

    class elem {
      public:
        unary_ops uops;
        token tk;
        int64_t value{};
        token ws1; // when field is array the whitespace after initializer '{'
        token ws2; // when field is array the whitespace before initializer '}'
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

        // check if type declared
        if (tz.is_next_char(':')) {
            type_tk_ = tz.next_token();
            if (tz.is_next_char('[')) {
                elroot_.is_array = true;
                array_size_tk_ = tz.next_token();
                if (const std::optional<int64_t> value{toc::parse_to_constant(
                        array_size_tk_, array_size_tk_.text())};
                    value) {
                    elroot_.array_size = static_cast<size_t>(*value);
                    if (elroot_.array_size <= 0) {
                        throw compiler_exception{
                            array_size_tk_,
                            "array sizes must be greater than 0"};
                    }
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
            .type_ptr = &tp,
            .declared_at_tk{name_tk_},
            .is_array = elroot_.is_array,
            .array_size = elroot_.array_size,
        };
        tc.add_var(name_tk_, null_strm, 0, var, true);

        if (has_init_) {
            parse(tc, tz, tp, elroot_);
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

        print_source(os, tp, elroot_);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        tc.comment_source(*this, os, indent);
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr = &get_type(),
            .declared_at_tk{name_tk_},
            .is_array = elroot_.is_array,
            .array_size = elroot_.array_size,
        };
        tc.add_var(name_tk_, os, indent, var, true);
    }

    auto compile_data(const toc& tc, std::ostream& os) const -> void override {
        const type& tp{get_type()};
        if (tp.is_built_in()) {
            compile_data_builtin(tc, os, name_tk_.text(), tp, elroot_);
            return;
        }

        // user type

        if (not elroot_.is_array) {
            compile_data_type(tc, os, name_tk_.text(), tp, elroot_);
            return;
        }

        // array of user type
        // todo
    }

  private:
    static auto compile_data_builtin(const toc& tc, std::ostream& os,
                                     const std::string_view fldnm,
                                     const type& tp, const elem& elroot)
        -> void {

        // nasm define data token
        std::string_view dd{tc.get_data_def(tp.size())};
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

        if (elroot.tk.is_string()) {
            std::print(os, "{} '", dd);
            elroot.tk.compile_to(os);
            std::println(os, "'");
            size_t sz{elroot.tk.string_size_bytes()};
            // pad remaining array with 0
            if (elroot.array_size != 0 and sz < elroot.array_size) {
                std::println(os, "times {} {} 0", elroot.array_size - sz, dd);
            }
            return;
        }

        // normal case

        // array without initializer
        if (elroot.elems.empty()) {
            std::println(os, "times {} {} 0", elroot.array_size, dd);
            return;
        }

        // initializer
        std::print(os, "{} ", dd);
        int32_t counter{0};
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
        return;
    }

    static auto compile_data_type(const toc& tc, std::ostream& os,
                                  const std::string_view nm, const type& tp,
                                  const elem& elroot) -> void {

        std::println(os, "; {}: {}", nm, tp.name());
        const std::span<const type_field>& flds{tp.fields()};
        size_t counter{0};
        for (const elem& e : elroot.elems) {
            const type_field& tf{flds[counter]};
            compile_data_type_field(tc, os, tf, e);
            ++counter;
        }
    }

    static auto compile_data_type_field(const toc& tc, std::ostream& os,
                                        const type_field& tf,
                                        const elem& elroot) -> void {

        if (tf.type_ptr->is_built_in()) {
            compile_data_builtin(tc, os, tf.name, *tf.type_ptr, elroot);
            return;
        }

        // user type
        // todo

        // user type array
        // todo
    }

    static auto parse(toc& tc, tokenizer& tz, const type& tp, elem& elroot)
        -> void {

        if (not elroot.is_array) {
            if (tp.is_built_in()) {
                parse_builtin(tc, tz, tp, elroot);
                return;
            }

            // user type

            parse_type(tc, tz, tp, elroot);
            return;
        }

        // array

        if (tp.is_built_in()) {
            // special case for strings

            elroot.tk = tz.next_token();
            if (elroot.tk.is_string()) {
                if (elroot.array_size == 0) {
                    elroot.array_size = elroot.tk.string_size_bytes();
                }
                if (tp.name() == "i8") {
                    return;
                }
                throw compiler_exception(elroot.tk,
                                         "only 'i8' arrays can be strings");
            }
            tz.put_back_token(elroot.tk);

            // normal case

            if (not tz.is_next_char('{')) {
                throw compiler_exception(
                    tz, "expected '{' to open array initializer");
            }
            size_t counter{0};
            while (true) {
                elroot.elems.emplace_back(unary_ops{}, token{}, 0, token{},
                                          token{}, false, 0,
                                          std::vector<elem>{});
                parse_builtin(tc, tz, tp, elroot.elems.back());
                ++counter;
                if (not tz.is_next_char(',')) {
                    break;
                }
            }
            if (not tz.is_next_char('}')) {
                throw compiler_exception(
                    tz, "expected '}' to close array initializer");
            }

            if (elroot.array_size != 0 and
                elroot.elems.size() > elroot.array_size) {
                throw compiler_exception(
                    tz,
                    std::format("array size is {} but contains {} initializers",
                                elroot.array_size, elroot.elems.size()));
            }

            if (elroot.array_size == 0) {
                elroot.array_size = counter;
            }

            return;
        }

        // user type array
        // todo

        if (not tz.is_next_char('{')) {
            throw compiler_exception(tz,
                                     "expected '{' to open array initializer");
        }

        if (not tz.is_next_char('}')) {
            throw compiler_exception(tz,
                                     "expected '}' to close array initializer");
        }
    }

    static auto parse_builtin(toc& tc, tokenizer& tz, const type& tp,
                              elem& elroot) -> void {

        unary_ops uops{tz};
        token tk{tz.next_token()};
        elroot.uops = uops;
        elroot.tk = tk;
        if (&tp == &tc.get_type_bool()) {
            if (tk.is_text("true")) {
                elroot.value = 1;
            } else if (tk.is_text("false")) {
                elroot.value = 0;
            } else {
                throw compiler_exception(
                    tk, std::format("boolean field '{}' must be true or false",
                                    tp.name()));
            }
            return;
        }
        if (std::optional<int64_t> num{tc.parse_to_constant(tk, tk.text())}) {
            elroot.value = *num;
            return;
        }
        throw compiler_exception(
            tk,
            std::format("element of type '{}' must be a constant", tp.name()));
    }

    static auto parse_type(toc& tc, tokenizer& tz, const type& tp, elem& elroot)
        -> void {

        if (not tz.is_next_char('{')) {
            throw compiler_exception(tz,
                                     "expected '{' to open type initializer");
        }

        elroot.ws1 = tz.next_whitespace_token();

        size_t counter{0};
        for (const type_field& tf : tp.fields()) {
            if (counter++) {
                if (not tz.is_next_char(',')) {
                    throw compiler_exception(
                        tz, std::format(
                                "expected ',' and initializer for field '{}'",
                                tf.name));
                }
            }
            elroot.elems.emplace_back(unary_ops{}, token{}, 0, token{}, token{},
                                      tf.is_array, tf.array_size,
                                      std::vector<elem>{});
            elroot.elems.back().ws1 = tz.next_whitespace_token();
            parse_type_field(tc, tz, tf, elroot.elems.back());
            elroot.elems.back().ws2 = tz.next_whitespace_token();
        }

        elroot.ws2 = tz.next_whitespace_token();

        if (not tz.is_next_char('}')) {
            throw compiler_exception(tz,
                                     "expected '}' to close type initializer");
        }
    }

    static auto parse_type_field(toc& tc, tokenizer& tz, const type_field& tf,
                                 elem& elroot) -> void {

        if (tf.type_ptr->is_built_in()) {
            if (not tf.is_array) {
                parse_builtin(tc, tz, *tf.type_ptr, elroot);
                return;
            }

            // array

            // special case for strings

            elroot.tk = tz.next_token();
            if (elroot.tk.is_string()) {
                if (elroot.array_size == 0) {
                    elroot.array_size = elroot.tk.string_size_bytes();
                }
                if (tf.type_ptr->name() == "i8") {
                    return;
                }
                throw compiler_exception(elroot.tk,
                                         "only 'i8' arrays can be strings");
            }
            elroot.tk = {};
            tz.put_back_token(elroot.tk);

            // normal case

            if (not tz.is_next_char('{')) {
                throw compiler_exception(
                    tz, "expected '{' to open array initializer");
            }

            size_t counter{0};
            while (true) {
                elroot.elems.emplace_back(unary_ops{}, token{}, 0, token{},
                                          token{}, false, 0,
                                          std::vector<elem>{});
                parse_builtin(tc, tz, *tf.type_ptr, elroot.elems.back());
                ++counter;
                if (not tz.is_next_char(',')) {
                    break;
                }
                if (counter == tf.array_size) {
                    throw compiler_exception(
                        tz, std::format("initializer has more elements than "
                                        "the array size of {}",
                                        tf.array_size));
                }
            }

            if (not tz.is_next_char('}')) {
                throw compiler_exception(
                    tz, "expected '}' to close array initializer");
            }

            return;
        }

        // user type

        if (not tf.is_array) {
            // todo
            return;
        }

        // user type array

        // todo
    }

    static auto print_source(std::ostream& os, const type& tp,
                             const elem& elroot) -> void {

        if (not elroot.is_array) {
            if (tp.is_built_in()) {
                elroot.uops.source_to(os);
                elroot.tk.source_to(os);
                return;
            }

            // user type

            std::print(os, "{{");
            elroot.ws1.source_to(os);
            size_t counter{0};
            for (const type_field& tf : tp.fields()) {
                if (counter++) {
                    std::print(os, ",");
                }
                elroot.elems[counter - 1].ws1.source_to(os);
                print_source_field(os, tf, elroot.elems[counter - 1]);
                elroot.elems[counter - 1].ws2.source_to(os);
            }
            elroot.ws2.source_to(os);
            std::print(os, "}}");
            return;
        }

        // array

        if (tp.is_built_in()) {
            std::print(os, "{{");
            elroot.ws1.source_to(os);
            size_t counter{0};
            for (const elem& e : elroot.elems) {
                if (counter++) {
                    std::print(os, ",");
                }
                e.ws1.source_to(os);
                e.uops.source_to(os);
                e.tk.source_to(os);
                e.ws2.source_to(os);
            }
            elroot.ws2.source_to(os);
            std::print(os, "}}");
            return;
        }

        // user type array
    }

    static auto print_source_field(std::ostream& os, const type_field& tf,
                                   const elem& el) -> void {

        if (tf.type_ptr->is_built_in()) {
            if (not tf.is_array) {
                el.uops.source_to(os);
                el.tk.source_to(os);
                return;
            }

            // array

            // special case for string

            if (el.tk.is_string()) {
                el.tk.source_to(os);
                return;
            }

            // normal case

            std::print(os, "{{");
            size_t counter{0};
            for (const elem& e : el.elems) {
                if (counter++) {
                    std::print(os, ",");
                }
                e.uops.source_to(os);
                e.tk.source_to(os);
            }
            std::print(os, "}}");

            return;
        }

        // user type
    }
};
