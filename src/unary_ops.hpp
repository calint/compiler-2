#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <print>
#include <ranges>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// the prefix '-' and '~' of an operand, applied after the operand is
// evaluated, the innermost first
class unary_ops final {
    token ws_pre_;          // whitespace before the ops, at where they start
    std::vector<char> ops_; // the unary ops as characters
    // the whitespace after each op but the last, e.g. the ' ' of '- -x'
    std::vector<token> ws_between_;

  public:
    explicit unary_ops(tokenizer& tz) : ws_pre_{tz.next_whitespace_token()} {
        while (true) {
            if (tz.is_next_char('~')) {
                ops_.emplace_back('~');
            } else if (tz.is_next_char('-')) {
                ops_.emplace_back('-');
            } else {
                break;
            }

            // note: the whitespace before the operand belongs to its token
            const token ws_tk{tz.next_whitespace_token()};

            if (tz.peek_char() != '~' and tz.peek_char() != '-') {
                tz.put_back_token(ws_tk);
                break;
            }

            ws_between_.emplace_back(ws_tk);
        }
    }

    unary_ops() = default;

    // applies the ops to 'dst', the innermost first
    auto compile(machine& x, const size_t indent, const token& src_loc_tk,
                 const operand& dst, const machine::runtime_check& check) const
        -> void {

        for (const char o : ops_ | std::views::reverse) {
            x.unary(src_loc_tk, indent,
                    o == '-' ? machine::arithmetic_operator::negate
                             : machine::arithmetic_operator::complement,
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

            throw compile_error{ws_pre_,
                                "constant expression overflows the type"};
        }

        return evaluate_constant(v);
    }

    [[nodiscard]] auto evaluate_constant(int64_t v) const -> int64_t {
        for (const char op : ops_ | std::views::reverse) {
            switch (op) {
            case '-':
                // unsigned negation wraps like the run-time instruction
                v = static_cast<int64_t>(-static_cast<uint64_t>(v));
                break;

            case '~':
                v = static_cast<int64_t>(~static_cast<uint64_t>(v));
                break;

            default:
                std::unreachable();
            }
        }

        return v;
    }

    [[nodiscard]] auto is_empty() const -> bool { return ops_.empty(); }

    [[nodiscard]] auto is_only_negated() const -> bool {
        return ops_.size() == 1 and ops_.back() == '-';
    }

    auto put_back(tokenizer& tz) const -> void {
        // the last character read is put back first
        for (size_t i{ops_.size()}; i > 0; --i) {
            const size_t op_ix{i - 1};

            tz.put_back_char(ops_.at(op_ix));

            if (op_ix > 0) {
                tz.put_back_token(ws_between_.at(op_ix - 1));
            }
        }

        tz.put_back_token(ws_pre_);
    }

    auto source_to(std::ostream& os) const -> void {
        ws_pre_.source_to(os);

        for (size_t i{}; i < ops_.size(); ++i) {
            if (i > 0) {
                ws_between_.at(i - 1).source_to(os);
            }

            std::print(os, "{}", ops_.at(i));
        }
    }

  private:
    // true when a negation of the list is applied to 'minimum', the most
    // negative value of the width, which has no positive counterpart
    [[nodiscard]] auto negation_overflows(int64_t v,
                                          const int64_t minimum) const -> bool {

        for (const char op : ops_ | std::views::reverse) {
            if (op == '~') {
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
};
