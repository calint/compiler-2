#pragma once

// the integer semantics of constants: what the registers of the targets
// compute, so that a constant folded at compile time has the value the
// program would have computed; signed values of 8, 16, 32 and 64 bits, sign
// extended in an 'int64_t', arithmetic that wraps like two's complement

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "operand.hpp"
#include "type.hpp"

class constant_arith final {
  public:
    //
    // statics
    //

    // empty when the targets differ at run time: a zero divisor traps, the
    // 64-bit minimum divided by -1 traps on x86_64 and shift counts outside
    // the width are masked differently
    [[nodiscard]] static auto
    apply_operation(const int64_t lhs, const arithmetic_operator op,
                    const int64_t rhs, const type& width_type)
        -> std::optional<int64_t> {

        if (op == arithmetic_operator::divide or
            op == arithmetic_operator::remainder) {

            return apply_division(lhs, op, rhs, width_type);
        }

        if (op == arithmetic_operator::shift_left or
            op == arithmetic_operator::shift_right) {

            return apply_shift(lhs, op, rhs, width_type);
        }

        return combine(lhs, op, rhs, width_type);
    }

    // operations whose low bits do not depend on higher bits
    [[nodiscard]] static auto combine(const int64_t lhs,
                                      const arithmetic_operator op,
                                      const int64_t rhs, const type& width_type)
        -> int64_t {

        // unsigned arithmetic wraps like the registers
        const uint64_t lhs_bits{static_cast<uint64_t>(lhs)};
        const uint64_t rhs_bits{static_cast<uint64_t>(rhs)};

        if (op == arithmetic_operator::add) {
            return wrap_to_width(static_cast<int64_t>(lhs_bits + rhs_bits),
                                 width_type);
        }

        if (op == arithmetic_operator::subtract) {
            return wrap_to_width(static_cast<int64_t>(lhs_bits - rhs_bits),
                                 width_type);
        }

        if (op == arithmetic_operator::multiply) {
            return wrap_to_width(static_cast<int64_t>(lhs_bits * rhs_bits),
                                 width_type);
        }

        if (op == arithmetic_operator::bit_and) {
            return static_cast<int64_t>(lhs_bits & rhs_bits);
        }

        if (op == arithmetic_operator::bit_or) {
            return static_cast<int64_t>(lhs_bits | rhs_bits);
        }

        assert(op == arithmetic_operator::bit_xor);

        return static_cast<int64_t>(lhs_bits ^ rhs_bits);
    }

    // true when the quotient is outside the width, e.g. -128 / -1 of an 'i8'
    [[nodiscard]] static auto division_overflows(const int64_t lhs,
                                                 const int64_t rhs,
                                                 const type& width_type)
        -> bool {

        return rhs == -1 and lhs == signed_minimum(width_type.size_bits());
    }

    // the comparison of two constants
    [[nodiscard]] static auto eval_constant(const int64_t lhs,
                                            const comparison_operator op,
                                            const int64_t rhs) -> bool {

        if (op == comparison_operator::equal) {
            return lhs == rhs;
        }

        if (op == comparison_operator::not_equal) {
            return lhs != rhs;
        }

        if (op == comparison_operator::less) {
            return lhs < rhs;
        }

        if (op == comparison_operator::less_equal) {
            return lhs <= rhs;
        }

        if (op == comparison_operator::greater) {
            return lhs > rhs;
        }

        assert(op == comparison_operator::greater_equal);

        return lhs >= rhs;
    }

    // 'value' is in the range of a signed integer of 'size_bytes'
    [[nodiscard]] static auto fits_size_bytes(const int64_t value,
                                              const size_t size_bytes) -> bool {

        switch (size_bytes) {
        case sizeof(int8_t):
            return std::in_range<int8_t>(value);

        case sizeof(int16_t):
            return std::in_range<int16_t>(value);

        case sizeof(int32_t):
            return std::in_range<int32_t>(value);

        default:
            return true;
        }
    }

    // true when the sum, difference or product is outside the width, e.g.
    // '100 + 100' of an 'i8'
    [[nodiscard]] static auto overflows_width(const int64_t lhs,
                                              const arithmetic_operator op,
                                              const int64_t rhs,
                                              const type& width_type) -> bool {

        int64_t result{};
        bool wide_overflow{};

        if (op == arithmetic_operator::add) {
            wide_overflow = __builtin_add_overflow(lhs, rhs, &result);
        } else if (op == arithmetic_operator::subtract) {
            wide_overflow = __builtin_sub_overflow(lhs, rhs, &result);
        } else if (op == arithmetic_operator::multiply) {
            wide_overflow = __builtin_mul_overflow(lhs, rhs, &result);
        } else {
            return false;
        }

        return wide_overflow or result != wrap_to_width(result, width_type);
    }

    // the most negative value of a signed integer of 'size_bits', e.g. -128 for
    // 8, which has no positive counterpart; setting all bits above the sign bit
    // of the width gives it
    [[nodiscard]] static auto signed_minimum(const size_t size_bits)
        -> int64_t {

        assert(size_bits != 0 and
               size_bits <= std::numeric_limits<uint64_t>::digits);

        return static_cast<int64_t>(~uint64_t{} << (size_bits - 1));
    }

    // the value a register of 'width_type' holds, sign extended
    [[nodiscard]] static auto wrap_to_width(const int64_t value,
                                            const type& width_type) -> int64_t {

        const size_t bits{width_type.size_bits()};

        if (bits >= std::numeric_limits<uint64_t>::digits) {
            return value;
        }

        const uint64_t sign_bit{uint64_t{1} << (bits - 1)};

        const uint64_t low{
            static_cast<uint64_t>(value) & ((sign_bit << 1U) - 1U),
        };

        // flipping and subtracting the sign bit extends it
        return static_cast<int64_t>((low ^ sign_bit) - sign_bit);
    }

  private:
    //
    // statics
    //

    // empty when the division traps: 'x / 0', and the 64-bit minimum divided
    // by -1 on x86_64; a narrower minimum is divided in a wider register and
    // wraps, e.g. -128 / -1 of an 'i8' is -128
    [[nodiscard]] static auto
    apply_division(const int64_t lhs, const arithmetic_operator op,
                   const int64_t rhs, const type& width_type)
        -> std::optional<int64_t> {

        if (rhs == 0 or
            (rhs == -1 and lhs == std::numeric_limits<int64_t>::min())) {

            return std::nullopt;
        }

        return wrap_to_width(op == arithmetic_operator::divide ? lhs / rhs
                                                               : lhs % rhs,
                             width_type);
    }

    // empty when the count is not below the width, e.g. 'x << 64' of an 'i64'
    [[nodiscard]] static auto
    apply_shift(const int64_t lhs, const arithmetic_operator op,
                const int64_t rhs, const type& width_type)
        -> std::optional<int64_t> {

        if (rhs < 0 or std::cmp_greater_equal(rhs, width_type.size_bits())) {
            return std::nullopt;
        }

        return shift_constant(lhs, op, static_cast<uint64_t>(rhs), width_type);
    }

    // a count within the width shifts the same on both targets
    [[nodiscard]] static auto
    shift_constant(const int64_t lhs, const arithmetic_operator op,
                   const uint64_t count, const type& width_type) -> int64_t {

        const uint64_t bits{static_cast<uint64_t>(lhs)};

        if (op == arithmetic_operator::shift_left) {
            return wrap_to_width(static_cast<int64_t>(bits << count),
                                 width_type);
        }

        // 'lhs' is sign extended so shifting in its sign bit keeps the width
        if (lhs < 0) {
            return static_cast<int64_t>(~(~bits >> count));
        }

        return static_cast<int64_t>(bits >> count);
    }
};
