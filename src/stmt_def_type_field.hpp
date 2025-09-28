#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_def_type_field final : public statement {
    token type_tk_;

  public:
    stmt_def_type_field(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)} {

        set_type(tc.get_type_void());

        // is type specified?
        if (not tz.is_next_char(':')) {
            // it is not
            return;
        }
        // get type name
        type_tk_ = tz.next_token();
    }

    stmt_def_type_field() = default;
    stmt_def_type_field(const stmt_def_type_field&) = default;
    stmt_def_type_field(stmt_def_type_field&&) = default;
    auto operator=(const stmt_def_type_field&)
        -> stmt_def_type_field& = default;
    auto operator=(stmt_def_type_field&&) -> stmt_def_type_field& = default;

    ~stmt_def_type_field() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.is_empty()) {
            return;
        }
        os << ':';
        type_tk_.source_to(os);
    }

    [[nodiscard]] auto name() const -> const std::string& {
        return tok().name();
    }

    [[nodiscard]] auto type_str() const -> const std::string& {
        return type_tk_.name();
    }
};
