#pragma once
// reviewed: 2025-09-28

#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "null_stream.hpp"
#include "statement.hpp"
#include "toc.hpp"
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
        std::vector<elem> elems;
    };

    token name_tk_;
    token type_tk_;
    token array_size_tk_;
    size_t array_size_{};
    token ws1_; // whitespace after '='
    token ws2_; // whitespace after ']'
    bool is_array_{};
    std::vector<elem> elems_;
    token tk_str_;
    bool is_string_{};

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
                is_array_ = true;
                array_size_tk_ = tz.next_token();
                if (const std::optional<int64_t> value{toc::parse_to_constant(
                        array_size_tk_, array_size_tk_.text())};
                    value) {
                    array_size_ = static_cast<size_t>(*value);
                    if (array_size_ <= 0) {
                        throw compiler_exception{
                            array_size_tk_,
                            "array sizes must be greater than 0"};
                    }
                }
                if (not tz.is_next_char(']')) {
                    throw compiler_exception{type_tk_,
                                             "expected array size and ']'"};
                }
                ws2_ = tz.next_whitespace_token();
            }
        }

        // get type reference from the token
        const type& tp{type_tk_.text().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.text())};
        set_type(tp);

        // expect initialization
        const bool init_required{tz.is_next_char('=')};

        ws1_ = tz.next_whitespace_token();

        // add var to toc without causing output by passing a null stream
        null_stream null_strm;
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr = &tp,
            .declared_at_tk{name_tk_},
            .is_array = is_array_,
            .array_size = array_size_,
        };
        tc.add_var(name_tk_, null_strm, 0, var, true);

        if (init_required) {
            parse(tc, tz, tp, elems_);
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
            if (is_array_) {
                std::print(os, "[");
                if (array_size_ != 0) {
                    array_size_tk_.source_to(os);
                }
                std::print(os, "]");
                ws2_.source_to(os);
            }
        }

        // special case for i8 string
        if (tk_str_.is_string()) {
            std::print(os, "=");
            ws1_.source_to(os);
            tk_str_.source_to(os);
            return;
        }

        if (elems_.empty()) {
            return;
        }

        const type& tp{get_type()};
        std::print(os, "=");
        ws1_.source_to(os);

        print_source(os, tp, elems_);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        tc.comment_source(*this, os, indent);
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr = &get_type(),
            .declared_at_tk{name_tk_},
            .is_array = is_array_,
            .array_size = array_size_,
        };
        tc.add_var(name_tk_, os, indent, var, true);
    }

    auto compile_data(const toc& tc, std::ostream& os) const -> void override {
        const type& tp{get_type()};
        if (tp.is_built_in()) {
            std::string_view dd{tc.get_data_def(tp.size())};
            if (not is_array_) {
                std::println(os, "; {}: {}", name_tk_.text(), tp.name());
                if (elems_.empty()) {
                    std::println(os, "{} 0", dd);
                    return;
                }

                std::print(os, "{} ", dd);
                elems_[0].uops.source_to(os);
                std::println(os, "{}", elems_[0].value);
                return;
            }

            // array of built-ins
            std::println(os, "; {}: {}[{}]", name_tk_.text(), tp.name(),
                         array_size_);

            // special case for strings
            if (tk_str_.is_string()) {
                std::print(os, "{} '", dd);
                tk_str_.compile_to(os);
                std::println(os, "'");
                size_t sz{tk_str_.string_size_bytes()};
                if (array_size_ != 0 and sz < array_size_) {
                    std::println(os, "times {} {} 0", array_size_ - sz, dd);
                }
                return;
            }

            // arrau without initializer
            if (elems_.empty()) {
                std::println(os, "times {} {} 0", array_size_, dd);
                return;
            }

            // initializer
            std::print(os, "{} ", dd);
            int32_t counter{0};
            for (const elem& e : elems_) {
                if (counter++) {
                    std::print(os, ", ");
                }
                std::print(os, "{}", e.value);
            }

            std::println(os);

            // pad remaining array with 0
            if (array_size_ != elems_.size()) {
                std::println(os, "times {} {} 0", array_size_ - elems_.size(),
                             dd);
            }
            return;
        }

        // user type
        compile_data_type(tc, os, name_tk_.text(), tp, elems_);
    }

  private:
    auto compile_data_type(const toc& tc, std::ostream& os,
                           const std::string_view nm, const type& tp,
                           const std::vector<elem>& els) const -> void {

        std::println(os, "; {}: {}", nm, tp.name());
        size_t counter{0};
        for (const elem& e : els) {
            const type_field& tf{tp.fields()[counter]};
            compile_data_type_field(tc, os, tf, e);
            ++counter;
        }
    }

    auto compile_data_type_field(const toc& tc, std::ostream& os,
                                 const type_field& tf, const elem& el) const
        -> void {

        if (not tf.is_array) {
            std::string_view dd{tc.get_data_def(tf.type_ptr->size())};
            if (tf.type_ptr->is_built_in()) {
                std::println(os, "; {}: {}", tf.name, tf.type_ptr->name());
                if (el.tk.is_empty()) {
                    std::println(os, "{} 0", dd);
                    return;
                }

                std::print(os, "{} ", dd);
                el.uops.source_to(os);
                std::println(os, "{}", el.value);
                return;
            }
        }
    }

    auto parse(toc& tc, tokenizer& tz, const type& tp, std::vector<elem>& els)
        -> void {

        if (tp.is_built_in()) {
            if (not is_array_) {
                unary_ops uo{tz};
                token tk{tz.next_token()};
                if (&tp == &tc.get_type_bool()) {
                    if (tk.is_text("true")) {
                        els.emplace_back(uo, tk, 1, std::vector<elem>{});
                    } else if (tk.is_text("false")) {
                        els.emplace_back(uo, tk, 0, std::vector<elem>{});
                    } else {
                        throw compiler_exception(
                            tk, std::format(
                                    "boolean field '{}' must be true or false",
                                    tp.name()));
                    }
                    return;
                }
                if (std::optional<int64_t> num{
                        tc.parse_to_constant(tk, tk.text())}) {
                    els.emplace_back(uo, tk, *num, std::vector<elem>{});
                    return;
                }
                throw compiler_exception(
                    tk, std::format("element of type '{}' must be a constant",
                                    tp.name()));
            }

            // array of built-ins

            // special case for strings
            tk_str_ = tz.next_token();
            if (tk_str_.is_string()) {
                is_string_ = true;
                if (array_size_ == 0) {
                    array_size_ = tk_str_.string_size_bytes();
                }
                if (tp.name() == "i8") {
                    return;
                }
                throw compiler_exception(tk_str_,
                                         "only 'i8' arrays can be strings");
            }

            tz.put_back_token(tk_str_);

            // normal case
            if (not tz.is_next_char('{')) {
                throw compiler_exception(
                    tz, "expected '{' to open array initializer");
            }
            size_t ninitializers{0};
            while (true) {
                ++ninitializers;
                unary_ops uo{tz};
                token tk{tz.next_token()};
                if (&tp == &tc.get_type_bool()) {
                    if (tk.is_text("true")) {
                        els.emplace_back(uo, tk, 1, std::vector<elem>{});
                    } else if (tk.is_text("false")) {
                        els.emplace_back(uo, tk, 0, std::vector<elem>{});
                    } else {
                        throw compiler_exception(
                            tk,
                            std::format(
                                "boolean field '{}' must be 'true' or 'false'",
                                tp.name()));
                    }
                    if (tz.is_next_char(',')) {
                        continue;
                    }
                    break;
                }
                if (std::optional<int64_t> num{
                        tc.parse_to_constant(tk, tk.text())}) {
                    els.emplace_back(uo, tk, *num, std::vector<elem>{});
                    if (tz.is_next_char(',')) {
                        continue;
                    }
                    break;
                }
                throw compiler_exception(
                    tk, std::format("'{}' must be a constant", tp.name()));
            }
            if (not tz.is_next_char('}')) {
                throw compiler_exception(
                    tz, "expected '}' to close array initializer");
            }

            if (array_size_ != 0 and elems_.size() > array_size_) {
                throw compiler_exception(
                    tz,
                    std::format("array size is {} but contains {} initializers",
                                array_size_, elems_.size()));
            }

            if (array_size_ == 0) {
                array_size_ = ninitializers;
            }

            return;
        }

        // user type

        if (not is_array_) {
            parse_type(tc, tz, tp, els);
            return;
        }

        if (not tz.is_next_char('{')) {
            throw compiler_exception(tz,
                                     "expected '{' to open array initializer");
        }

        parse_type(tc, tz, tp, els);

        if (not tz.is_next_char('}')) {
            throw compiler_exception(tz,
                                     "expected '}' to close array initializer");
        }
    }

    auto parse_type(toc& tc, tokenizer& tz, const type& tp,
                    std::vector<elem>& els) -> void {

        if (not tz.is_next_char('{')) {
            throw compiler_exception(tz,
                                     "expected '{' to open type initializer");
        }

        size_t counter{0};
        for (const type_field& f : tp.fields()) {
            if (counter++) {
                if (not tz.is_next_char(',')) {
                    throw compiler_exception(
                        tz, std::format(
                                "expected ',' and initializer for field '{}'",
                                f.name));
                }
            }
            parse_type_field(tc, tz, f, els);
        }

        if (not tz.is_next_char('}')) {
            throw compiler_exception(tz,
                                     "expected '}' to close type initializer");
        }
    }

    auto parse_type_field(toc& tc, tokenizer& tz, const type_field& tf,
                          std::vector<elem>& els) -> void {

        if (tf.type_ptr->is_built_in()) {
            unary_ops uo{tz};
            token tk{tz.next_token()};
            if (tf.type_ptr == &tc.get_type_bool()) {
                if (tk.is_text("true")) {
                    els.emplace_back(uo, tk, 1, std::vector<elem>{});
                } else if (tk.is_text("false")) {
                    els.emplace_back(uo, tk, 0, std::vector<elem>{});
                } else {
                    throw compiler_exception(
                        tk,
                        std::format("boolean field '{}' must be true or false",
                                    tf.name));
                }
                return;
            }
            if (std::optional<int64_t> num{
                    tc.parse_to_constant(tk, tk.text())}) {
                els.emplace_back(uo, tk, *num, std::vector<elem>{});
                return;
            }
            throw compiler_exception(
                tk, std::format("field '{}' must be a constant", tf.name));
        }

        // array
        if (not tz.is_next_char('{')) {
            throw compiler_exception(tz,
                                     "expected '{' to open array initializer");
        }

        if (not tz.is_next_char('}')) {
            throw compiler_exception(tz,
                                     "expected '}' to close array initializer");
        }
    }

    auto print_source(std::ostream& os, const type& tp,
                      const std::vector<elem>& els) const -> void {

        if (tp.is_built_in()) {
            if (not is_array_) {
                els[0].uops.source_to(os);
                els[0].tk.source_to(os);
                return;
            }
            std::print(os, "{{");
            size_t counter{0};
            for (const elem& e : elems_) {
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
        std::print(os, "{{");
        size_t counter{0};
        for (const type_field& f : tp.fields()) {
            if (counter++) {
                std::print(os, ",");
            }
            print_source_field(os, f, els[counter - 1]);
        }
        std::print(os, "}}");
    }

    auto print_source_field(std::ostream& os, const type_field& tf,
                            const elem& el) const -> void {

        if (tf.type_ptr->is_built_in()) {
            el.uops.source_to(os);
            el.tk.source_to(os);
            return;
        }
    }
};
