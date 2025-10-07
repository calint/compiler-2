#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

class toc;
class type;

class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

  public:
    explicit statement(token tk, unary_ops uops = {})
        : token_{tk}, uops_{std::move(uops)} {

        validate_identifier_name(token_);
    }

    statement() = default;
    statement(statement&&) = default;
    statement(const statement&) = default;
    auto operator=(const statement&) -> statement& = default;
    auto operator=(statement&&) -> statement& = default;

    virtual ~statement() = default;

    virtual auto compile([[maybe_unused]] toc& tc, std::ostream& os,
                         [[maybe_unused]] size_t indent,
                         [[maybe_unused]] std::string_view dst = "") const
        -> void {
        uops_.source_to(os);
        token_.compile_to(os);
    }

    virtual auto source_to(std::ostream& os) const -> void {
        uops_.source_to(os);
        token_.source_to(os);
    }

    [[nodiscard]] auto tok() const -> const token& { return token_; }

    [[nodiscard]] virtual auto is_in_data_section() const -> bool {
        return false;
    }

    [[nodiscard]] virtual auto is_expression() const -> bool { return false; }

    [[nodiscard]] virtual auto identifier() const -> std::string_view {
        return token_.text();
    }

    [[nodiscard]] virtual auto get_unary_ops() const -> const unary_ops& {
        return uops_;
    }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    [[nodiscard]] virtual auto get_type() const -> const type& {
        return *type_;
    }

    static auto validate_identifier_name(const token& tk) -> void {
        if (tk.text().ends_with(".")) {
            throw compiler_exception{
                tk,
                std::format("unexpected '.' at the end of '{}'", tk.text())};
        }
    }
};
