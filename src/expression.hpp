#pragma once

#include "statement.hpp"

class expression : public statement {
public:
  inline expression(token tk, unary_ops uops)
      : statement{move(tk), move(uops)} {}

  inline expression() = default;
  inline expression(const expression &) = default;
  inline expression(expression &&) = default;
  inline expression &operator=(const expression &) = default;
  inline expression &operator=(expression &&) = default;

  inline ~expression() override = default;

  inline bool is_expression() const override { return true; }
};
