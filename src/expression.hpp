#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

#include <utility>

class expression : public statement {
  public:
    explicit expression(const token tk, unary_ops uops = {})
        : statement{tk, std::move(uops)} {}

    expression() = default;

    //
    // overridden methods
    //

    [[nodiscard]] auto is_expression() const -> bool override { return true; }

  protected:
    // e.g. a 'bool' result has no meaningful negation
    auto assert_no_unary_ops() const -> void {
        if (statement::get_unary_ops().is_empty()) {
            return;
        }

        throw compiler_exception{
            tok(), "this built-in function does not accept unary operations"};
    }
};
