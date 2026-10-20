#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>

#include "expression.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"

// '{}' where a built-in is expected is its zero value, 0 or 'false', as '{}'
// is the zero value of an instance or an array, e.g. 'n = {}' and 'res = {}'
// of 'func zero<T type>() res T' for any 'T'; it resolves like the literal
class expr_zero final : public expression {
    token close_brace_tk_;

  public:
    expr_zero(const type& tp, const token open_brace_tk,
              const token close_brace_tk)
        : expression{open_brace_tk}, close_brace_tk_{close_brace_tk} {

        set_type(tp);
    }

    expr_zero() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.copy_value(tok(), indent, dst.operand(), operand::imm(0, get_type()));
    }

    [[nodiscard]] auto
    folded_constant([[maybe_unused]] const toc& tc,
                    [[maybe_unused]] const type& width_type) const
        -> std::optional<int64_t> override {

        return 0;
    }

    [[nodiscard]] auto is_name_or_literal() const -> bool override {
        return true;
    }

    // zero has no high bits to lose
    [[nodiscard]] auto keeps_low_bits_when_narrowed() const -> bool override {
        return true;
    }

    [[nodiscard]] auto keeps_low_bits_when_widened() const -> bool override {
        return true;
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        return get_type().is_bool() ? reserved_names::false_value : "0";
    }
};
