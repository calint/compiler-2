#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "compiler_exception.hpp"
#include "field_coverage.hpp"
#include "token.hpp"

// receives each read of a variable, an empty range reads the whole variable
using read_visitor = std::function_ref<void(
    const token& use_tk, std::string_view read_text,
    const std::optional<field_coverage::range>& accessed)>;

// definite-assignment walk of one variable through a function body
struct assignment_flow {
    std::string_view var;

    // reported when a 'return' may leave 'var' unset
    token func_tk;

    // bytes assigned on every path reaching the current statement
    field_coverage assigned;

    // bytes assigned at every 'break' of the innermost loop
    std::optional<field_coverage> at_breaks;

    bool is_reachable{true};

    auto assert_set_at_return() const -> void {
        if (assigned.is_full()) {
            return;
        }

        throw compiler_exception{
            func_tk, "function may return without setting its return value"};
    }

    // a path that never reaches the next statement cannot narrow a merge
    auto end_path() -> void {
        assigned = field_coverage::full(assigned.size_bytes());
        is_reachable = false;
    }

    auto record_break() -> void {
        if (at_breaks) {
            at_breaks->intersect(assigned);
        } else {
            at_breaks = assigned;
        }

        end_path();
    }
};
