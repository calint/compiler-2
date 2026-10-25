#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <ranges>
#include <string_view>
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
    std::vector<token> ops_;

  public:
    explicit unary_ops(tokenizer& tz) {
        while (is_unary(tz.peek_operator())) {
            ops_.emplace_back(tz.seen_operator());
        }
    }

    unary_ops() = default;

    // applies the ops to 'dst', the innermost first
    auto compile(machine& x, const size_t indent, const token& src_loc_tk,
                 const operand& dst, const machine::runtime_check& check) const
        -> void {

        for (const token& op_tk : ops_ | std::views::reverse) {
            x.unary(src_loc_tk, indent,
                    op_tk.is_text("-") ? arithmetic_operator::negate
                                       : arithmetic_operator::complement,
                    dst, check);
        }
    }

    // the ops applied to a constant used at 'width', not wrapped to it; with
    // the overflow check a negation of the minimum of 'width' is an error at
    // the ops
    [[nodiscard]] auto evaluate_at(const int64_t v, const type& width,
                                   const bool checks_overflow) const
        -> int64_t {

        if (checks_overflow and
            negation_overflows(
                v, constant_arith::signed_minimum(width.size_bits()))) {

            throw compile_error{ops_.front(),
                                "constant expression overflows the type"};
        }

        return evaluate_constant(v);
    }

    [[nodiscard]] auto evaluate_constant(int64_t v) const -> int64_t {
        for (const token& op_tk : ops_ | std::views::reverse) {
            if (op_tk.is_text("~")) {
                v = static_cast<int64_t>(~static_cast<uint64_t>(v));
                continue;
            }

            // unsigned negation wraps like the run-time instruction
            v = static_cast<int64_t>(-static_cast<uint64_t>(v));
        }

        return v;
    }

    [[nodiscard]] auto is_empty() const -> bool { return ops_.empty(); }

    [[nodiscard]] auto is_only_negated() const -> bool {
        return ops_.size() == 1 and ops_.back().is_text("-");
    }

    // the last token read is put back first
    auto put_back(tokenizer& tz) const -> void {
        for (const token& op_tk : ops_ | std::views::reverse) {
            tz.put_back_token(op_tk);
        }
    }

    auto source_to(std::ostream& os) const -> void {
        for (const token& op_tk : ops_) {
            op_tk.source_to(os);
        }
    }

  private:
    // true when a negation of the list is applied to 'minimum', the most
    // negative value of the width, which has no positive counterpart
    [[nodiscard]] auto negation_overflows(int64_t v,
                                          const int64_t minimum) const -> bool {

        for (const token& op_tk : ops_ | std::views::reverse) {
            if (op_tk.is_text("~")) {
                v = static_cast<int64_t>(~static_cast<uint64_t>(v));
                continue;
            }

            if (v == minimum) {
                return true;
            }

            // unsigned negation wraps, a minimum wider than 'minimum' is
            // negated too
            v = static_cast<int64_t>(-static_cast<uint64_t>(v));
        }

        return false;
    }

    //
    // statics
    //

    [[nodiscard]] static auto is_unary(const std::string_view text) -> bool {
        return text == "-" or text == "~";
    }
};
