#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_def_field final : public statement {
    token name_tk_;
    unary_ops uops_;
    token initial_value_;

  public:
    stmt_def_field(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (name_tk_.is_empty()) {
            throw compiler_exception{name_tk_, "expected field name"};
        }

        if (not tz.is_next_char('=')) {
            throw compiler_exception{tz, "expected '=' and initial value"};
        }

        uops_ = unary_ops{tz};
        initial_value_ = tz.next_token();
        if (initial_value_.is_string()) {
            set_type(tc.get_type_void()); //? not implemented
        } else {
            set_type(tc.get_type_default()); //? only default type supported
        }

        tc.add_field(tok(), std::string{name_tk_.text()}, *this,
                     initial_value_.is_string());
    }

    ~stmt_def_field() override = default;

    stmt_def_field() = default;
    stmt_def_field(const stmt_def_field&) = default;
    stmt_def_field(stmt_def_field&&) = default;
    auto operator=(const stmt_def_field&) -> stmt_def_field& = default;
    auto operator=(stmt_def_field&&) -> stmt_def_field& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        std::print(os, "=");
        uops_.source_to(os);
        initial_value_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);
        std::print(os, "{}:", name_tk_.text());
        if (initial_value_.is_string()) {
            std::print(os, " db '");
            initial_value_.compile_to(os);
            std::println(os, "'");
            toc::indent(os, indent);
            // the length of the string
            std::println(os, "{}.len equ $-{}", name_tk_.text(),
                         name_tk_.text());
            return;
        }

        // the default type is i64
        std::print(os, " dq {}", uops_.to_string());
        initial_value_.compile_to(os);
        std::println(os, "");
    }

    [[nodiscard]] auto is_in_data_section() const -> bool override {
        return true;
    }

    // [[nodiscard]] auto is_string_field() const -> bool {
    //     return initial_value_.is_string();
    // }
};
