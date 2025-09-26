#pragma once

#include "statement.hpp"

class expression : public statement {
public:
  inline explicit expression(token tk, unary_ops uops = {})
      : statement{std::move(tk), std::move(uops)} {}

  inline expression() = default;
  inline expression(const expression &) = default;
  inline expression(expression &&) = default;
  inline auto operator=(const expression &) -> expression & = default;
  inline auto operator=(expression &&) -> expression & = default;

  inline ~expression() override = default;

  [[nodiscard]] inline auto is_expression() const -> bool override {
    return true;
  }
};
