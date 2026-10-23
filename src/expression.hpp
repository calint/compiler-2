#pragma once

#include <utility>

#include "statement.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

// the base of the nodes whose value is computed, not a name or a constant read
// in place: calls, arithmetic, conditions and the built-ins with a value;
// 'expr_any' derives from 'statement' because whether its value is computed
// depends on what it holds, it answers 'is_expression' by it; a condition that
// is one name or literal is an 'expr_arith' in a value ('release_name_or_
// literal'); 'expr_type' derives from it too, an instance value is copied as
// bytes, never computed
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
