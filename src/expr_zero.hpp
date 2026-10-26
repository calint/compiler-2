#pragma once

#include <optional>
#include <ostream>
#include <string_view>

#include "ident_info.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"

// '{}' where a built-in is expected is its zero value, 0 or 'false', as '{}'
// is the zero value of an instance or an array, e.g. 'n = {}' and 'res = {}'
// of 'func zero<T type>() res T' for any 'T'; it resolves like the literal.
// the bare type is the zero value of that type, e.g. the 'i8' of
// 'var x = i8', and keeps its type like a typed constant
class expr_zero final : public statement {
    // none for the bare type
    std::optional<token> close_brace_tk_;
    // null for '{}'
    const type* own_type_{};

  public:
    expr_zero(const type& tp, const token open_brace_tk,
              const token close_brace_tk)
        : statement{open_brace_tk}, close_brace_tk_{close_brace_tk} {

        set_type(tp);
    }

    // e.g. the 'i8' of 'var x = i8'
    expr_zero(const type& tp, const token type_tk)
        : statement{type_tk}, own_type_{&tp} {

        set_type(tp);
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        if (close_brace_tk_) {
            close_brace_tk_->source_to(os);
        }
    }

    [[nodiscard]] auto fold([[maybe_unused]] const toc& tc,
                            [[maybe_unused]] const value_use& use) const
        -> std::optional<folded_value> override {

        return folded_value{
            .value{},
            .own_type{own_type_},
        };
    }

    // a leaf
    auto visit_operands([[maybe_unused]] const operand_visitor visitor) const
        -> void override {}
};
