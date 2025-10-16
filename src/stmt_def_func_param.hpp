#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_def_func_param final : public statement {
    token type_tk_;
    bool is_array_{};

  public:
    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : statement{tz.next_token()} {
        assert(not tok().is_text(""));

        if (not tz.is_next_char(':')) {
            // no type defined, set default
            set_type(tc.get_type_default());
            return;
        }

        // get type
        type_tk_ = tz.next_token();
        if (type_tk_.text().starts_with("reg_")) {
            // register parameter, set default type
            set_type(tc.get_type_default());
            return;
        }

        set_type(tc.get_type_or_throw(type_tk_, type_tk_.text()));

        if (tz.is_next_char('[')) {
            if (not tz.is_next_char(']')) {
                throw compiler_exception{tz, "expected ']'"};
            }
            is_array_ = true;
        }
    }

    ~stmt_def_func_param() override = default;

    stmt_def_func_param() = default;
    stmt_def_func_param(const stmt_def_func_param&) = default;
    stmt_def_func_param(stmt_def_func_param&&) = default;
    auto operator=(const stmt_def_func_param&)
        -> stmt_def_func_param& = default;
    auto operator=(stmt_def_func_param&&) -> stmt_def_func_param& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.is_text("")) {
            return;
        }
        std::print(os, ":");
        type_tk_.source_to(os);
        if (is_array_) {
            std::print(os, "[]");
        }
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {}

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

    [[nodiscard]] auto get_register_name_or_empty() const -> std::string_view {
        const std::string_view type_name{type_tk_.text()};
        if (type_name.starts_with("reg_")) {
            return type_name.substr(4, type_name.size() - 4);
            // note: 4 is the length of "reg_"
        }
        return "";
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }
};
