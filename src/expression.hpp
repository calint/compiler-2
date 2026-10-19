#pragma once

#include <utility>

#include "statement.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

class expression : public statement {
  public:
    explicit expression(const token tk, unary_ops uops = {})
        : statement{tk, std::move(uops)} {}

    expression() = default;

    //
    // overridden methods
    //

    [[nodiscard]] auto is_expression() const -> bool override { return true; }
};
