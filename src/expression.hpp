#pragma once

#include <utility>

#include "statement.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

// the base of the nodes whose value is computed, not a name or a constant read
// in place: calls, arithmetic, conditions and the built-ins with a value; a
// condition that is one name or literal is an 'expr_arith' in a value
// ('release_name_or_literal'); 'expr_any' and 'expr_type' derive from
// 'statement': they are never an element of a list, the only place that asks
// 'is_expression'
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
