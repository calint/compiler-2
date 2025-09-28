#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"

class expression : public statement {
  public:
    explicit expression(token tk, unary_ops uops = {})
        : statement{std::move(tk), std::move(uops)} {}

    expression() = default;
    expression(const expression&) = default;
    expression(expression&&) = default;
    auto operator=(const expression&) -> expression& = default;
    auto operator=(expression&&) -> expression& = default;

    ~expression() override = default;

    [[nodiscard]] auto is_expression() const -> bool override { return true; }
};
