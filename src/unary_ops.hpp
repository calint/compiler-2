#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <ranges>
#include <vector>

#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// the prefix '-' and '~' of an operand, applied after the operand is
// evaluated, the innermost first; each op is a token with the whitespace
// before it, the whitespace before the operand belongs to the operand
class unary_ops final {
    std::vector<spelled_operator<arithmetic_operator>> ops_;

  public:
    explicit unary_ops(tokenizer& tz) {
        while (const std::optional<arithmetic_operator> op{
            unary_operator_of(tz.peek_operator()),
        }) {

            ops_.push_back({
                .tk{tz.seen_operator()},
                .op{*op},
            });
        }
    }

    unary_ops() = default;

    // applies the ops to 'dst', the innermost first, without the pairs that
    // cancel, see 'applied_ops'
    auto compile(machine& x, const size_t indent, const token& src_loc_tk,
                 const operand& dst, const machine::runtime_check& check) const
        -> void {

        for (const arithmetic_operator op : applied_ops(check)) {
            x.unary({src_loc_tk, indent}, op, dst, check);
        }
    }

    // the ops applied to a constant used at 'width', not wrapped to it; with
    // the overflow check a negation of the minimum of 'width' is an error at
    // that negation
    [[nodiscard]] auto evaluate_at(const int64_t value, const type& width,
                                   const bool checks_overflow) const
        -> int64_t {

        if (not checks_overflow) {
            return evaluate(value, std::nullopt);
        }

        return evaluate(value,
                        constant_arith::signed_minimum(width.size_bits()));
    }

    [[nodiscard]] auto evaluate_constant(const int64_t value) const -> int64_t {
        return evaluate(value, std::nullopt);
    }

    [[nodiscard]] auto is_empty() const -> bool { return ops_.empty(); }

    [[nodiscard]] auto is_only_negated() const -> bool {
        return ops_.size() == 1 and
               ops_.back().op == arithmetic_operator::negate;
    }

    auto source_to(std::ostream& os) const -> void {
        for (const spelled_operator<arithmetic_operator>& o : ops_) {
            o.tk.source_to(os);
        }
    }

  private:
    // the ops in the order they apply, an op cancelling the same op applied
    // just before it, e.g. '-~~-a' applies none; under the overflow check a
    // negation pair stays, the first traps on the minimum
    [[nodiscard]] auto applied_ops(const machine::runtime_check& check) const
        -> std::vector<arithmetic_operator> {

        std::vector<arithmetic_operator> applied;

        for (const spelled_operator<arithmetic_operator>& o :
             ops_ | std::views::reverse) {

            const bool cancels{
                not applied.empty() and applied.back() == o.op and
                    (o.op == arithmetic_operator::complement or
                     not check.enabled),
            };

            if (cancels) {
                applied.pop_back();
                continue;
            }

            applied.push_back(o.op);
        }

        return applied;
    }

    // the ops applied to 'value', the innermost first; a negation of
    // 'overflow_minimum', the most negative value of the width, which has no
    // positive counterpart, is an error at that negation
    [[nodiscard]] auto
    evaluate(int64_t value, const std::optional<int64_t> overflow_minimum) const
        -> int64_t {

        for (const spelled_operator<arithmetic_operator>& o :
             ops_ | std::views::reverse) {

            if (o.op == arithmetic_operator::complement) {
                value = static_cast<int64_t>(~static_cast<uint64_t>(value));
                continue;
            }

            if (overflow_minimum and value == *overflow_minimum) {
                throw compile_error{o.tk,
                                    "constant expression overflows the type"};
            }

            // unsigned negation wraps like the run-time instruction, a
            // minimum wider than 'overflow_minimum' is negated too
            value = static_cast<int64_t>(-static_cast<uint64_t>(value));
        }

        return value;
    }
};
