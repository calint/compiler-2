#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_def_func_param final : public statement {
    token type_tk_;

  public:
    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : statement{tz.next_token()} {
        assert(not tok().is_name(""));

        if (not tz.is_next_char(':')) {
            // no type defined, set default
            set_type(tc.get_type_default());
            return;
        }

        // get type
        type_tk_ = tz.next_token();
        if (type_tk_.name().starts_with("reg_")) {
            // register parameter, set default type
            set_type(tc.get_type_default());
            return;
        }

        set_type(tc.get_type_or_throw(tok(), type_tk_.name()));
    }

    stmt_def_func_param() = default;
    stmt_def_func_param(const stmt_def_func_param&) = default;
    stmt_def_func_param(stmt_def_func_param&&) = default;
    auto operator=(const stmt_def_func_param&)
        -> stmt_def_func_param& = default;
    auto operator=(stmt_def_func_param&&) -> stmt_def_func_param& = default;

    ~stmt_def_func_param() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.is_name("")) {
            return;
        }
        os << ":";
        type_tk_.source_to(os);
    }

    [[nodiscard]] auto name() const -> const std::string& {
        return tok().name();
    }

    [[nodiscard]] auto get_register_name_or_empty() const -> std::string {
        const std::string& type_name{type_tk_.name()};
        if (type_name.starts_with("reg_")) {
            return type_name.substr(4, type_name.size());
        }
        return "";
    }
};
