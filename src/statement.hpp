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

    virtual ~statement() = default;

    statement() = default;
    statement(statement&&) = default;
    statement(const statement&) = default;
    auto operator=(const statement&) -> statement& = default;
    auto operator=(statement&&) -> statement& = default;

    virtual auto compile([[maybe_unused]] toc& tc,
                         [[maybe_unused]] std::ostream& os,
                         [[maybe_unused]] size_t indent,
                         [[maybe_unused]] std::string_view dst) const
        -> void = 0;

    virtual auto source_to(std::ostream& os) const -> void {
        uops_.source_to(os);
        token_.source_to(os);
    }

    [[nodiscard]] auto tok() const -> const token& { return token_; }

    auto set_type(const type& tp) -> void { type_ = &tp; }

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

    [[nodiscard]] virtual auto get_type() const -> const type& {
        return *type_;
    }

    [[nodiscard]] virtual auto is_identifier() const -> bool { return false; }

    [[nodiscard]] virtual auto compile_lea(
        [[maybe_unused]] const token& src_loc_tk, [[maybe_unused]] toc& tc,
        [[maybe_unused]] std::ostream& os, [[maybe_unused]] const size_t indent,
        [[maybe_unused]] std::vector<std::string>& allocated_registers,
        [[maybe_unused]] const std::string& reg_size,
        [[maybe_unused]] const std::span<const std::string> lea_path) const
        -> std::string {
        return "";
    }

    // used in UB check
    // returns true if `var` is set at this statement
    [[nodiscard]] virtual auto
    is_var_set([[maybe_unused]] const std::string_view var) const -> bool {
        return false;
    }

    // used in UB check
    // throws if `var` is used in this statement
    virtual auto assert_var_not_used(const std::string_view var) const -> void {
        if (identifier() == var) {
            throw compiler_exception{
                token_, std::format("use of uninitialized variable '{}'",
                                    identifier())};
        }
    }

    // used in UB check
    // returns true if code after statement in block is considered "dead code"
    // applies to: `return`, `break`, `continue`
    [[nodiscard]] virtual auto is_code_after_this_unreachable() const -> bool {
        return false;
    }

  private:
    static auto validate_identifier_name(const token& tk) -> void {
        if (tk.text().ends_with(".")) {
            throw compiler_exception{
                tk,
                std::format("unexpected '.' at the end of '{}'", tk.text())};
        }
    }
};
