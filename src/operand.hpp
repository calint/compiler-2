#pragma once

// the backend value type, the address offset arithmetic and the operators
// that the front end and the backend compute with

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

class type;

[[nodiscard]] inline auto address_offset(const size_t size_bytes) -> int64_t {
    // storage sizes are limited to the signed 64-bit range
    assert(std::in_range<int64_t>(size_bytes));

    return static_cast<int64_t>(size_bytes);
}

// whether 'count' elements of 'size_bytes', which may be negative, added to the
// offset 'base' stay in the range of an address offset
[[nodiscard]] inline auto fits_address_offset(const int64_t base,
                                              const int64_t count,
                                              const size_t size_bytes) -> bool {

    const int64_t size{address_offset(size_bytes)};

    if (size != 0 and (count > std::numeric_limits<int64_t>::max() / size or
                       count < std::numeric_limits<int64_t>::min() / size)) {

        return false;
    }

    const int64_t offset{count * size};

    return (offset <= 0 or
            base <= std::numeric_limits<int64_t>::max() - offset) and
           (offset >= 0 or
            base >= std::numeric_limits<int64_t>::min() - offset);
}

// the byte offset of 'count' elements of 'size_bytes', count may be negative
[[nodiscard]] inline auto scaled_address_offset(const int64_t count,
                                                const size_t size_bytes)
    -> int64_t {

    const int64_t size{address_offset(size_bytes)};

    // constant indices are bounds checked against the storage size
    assert(size == 0 or (count <= std::numeric_limits<int64_t>::max() / size and
                         count >= std::numeric_limits<int64_t>::min() / size));

    return count * size;
}

[[nodiscard]] inline auto add_address_offset(const int64_t base,
                                             const int64_t offset) -> int64_t {

    assert(offset <= 0 or base <= std::numeric_limits<int64_t>::max() - offset);
    assert(offset >= 0 or base >= std::numeric_limits<int64_t>::min() - offset);

    return base + offset;
}

// what an operation of the backend reads or writes: empty, a register, an
// immediate, a symbol or a memory operand '[base + index * scale +
// displacement]'; the front end builds it and the backend lowers it to what
// the target encodes
class operand final {
    // a symbol is the value of a label or a constant of the assembler, known
    // when the program is assembled
    enum class kind : uint8_t { empty, reg, memory, immediate, symbol };

    kind kind_{kind::empty};
    const type* type_ptr_{};
    // the register the backend allocated, e.g. 'rax' for the operand 'eax',
    // empty when the operand does not own the register it names
    std::string allocation_register_;
    // the register of a register operand, the base of a memory operand
    std::string base_register_;
    std::string index_register_;
    int64_t immediate_{};
    std::string symbol_;
    int64_t displacement_{};
    uint64_t scale_{1};

  public:
    operand() = default;

    [[nodiscard]] auto allocation_register() const -> const std::string& {
        return allocation_register_;
    }

    // the base of a memory operand, e.g. 'rbp' of '[rbp + 8]'
    [[nodiscard]] auto base_register() const -> const std::string& {
        assert(is_memory());

        return base_register_;
    }

    [[nodiscard]] auto displacement() const -> int64_t { return displacement_; }

    [[nodiscard]] auto immediate() const -> int64_t {
        assert(is_immediate());

        return immediate_;
    }

    auto increment_offset(const int64_t offset) -> void {
        assert(is_memory());

        displacement_ = add_address_offset(displacement_, offset);
    }

    [[nodiscard]] auto index_register() const -> const std::string& {
        return index_register_;
    }

    // the memory operand '[reg]', its address is the value of the register
    [[nodiscard]] auto is_address_in_register(const std::string_view name) const
        -> bool {

        return is_memory() and base_register_ == name and
               index_register_.empty() and displacement_ == 0;
    }

    [[nodiscard]] auto is_empty() const -> bool { return kind_ == kind::empty; }

    [[nodiscard]] auto is_immediate() const -> bool {
        return kind_ == kind::immediate;
    }

    [[nodiscard]] auto is_memory() const -> bool {
        return kind_ == kind::memory;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return kind_ == kind::reg;
    }

    [[nodiscard]] auto is_symbol() const -> bool {
        return kind_ == kind::symbol;
    }

    // the register of a register operand, e.g. 'rax'
    [[nodiscard]] auto register_name() const -> const std::string& {
        assert(is_register());

        return base_register_;
    }

    [[nodiscard]] auto scale() const -> uint64_t { return scale_; }

    auto set_allocation_register(const std::string_view name) -> void {
        assert(is_register());

        allocation_register_ = name;
    }

    [[nodiscard]] auto symbol() const -> const std::string& {
        assert(is_symbol());

        return symbol_;
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(type_ptr_);

        return *type_ptr_;
    }

    // the register operand 'name', or a memory operand with 'name' as its base
    // or index; a backend with more names for one register compares indexes
    [[nodiscard]] auto uses_register(const std::string_view name) const
        -> bool {

        if (is_register()) {
            return base_register_ == name;
        }

        return is_memory() and
               (base_register_ == name or index_register_ == name);
    }

    //
    // statics
    //

    // numbers of any integer type keep call sites free of conversions
    template <std::integral integral_t>
    [[nodiscard]] static auto imm(const integral_t value,
                                  const type& value_type) -> operand {

        assert(std::in_range<int64_t>(value));

        operand result;
        result.kind_ = kind::immediate;
        result.type_ptr_ = &value_type;
        result.immediate_ = static_cast<int64_t>(value);

        return result;
    }

    [[nodiscard]] static auto mem(const std::string_view base,
                                  const std::string_view index,
                                  const uint64_t index_scale,
                                  const int64_t offset, const type& value_type)
        -> operand {

        assert(std::has_single_bit(index_scale));
        assert(not base.empty() or not index.empty() or offset != 0);

        operand result;
        result.kind_ = kind::memory;
        result.type_ptr_ = &value_type;
        result.base_register_ = base;
        result.index_register_ = index;
        result.scale_ = index_scale;
        result.displacement_ = offset;

        return result;
    }

    [[nodiscard]] static auto mem(const operand& address,
                                  const type& value_type) -> operand {

        assert(address.is_memory() or address.is_register());

        return mem(address.base_register_, address.index_register_,
                   address.scale_, address.displacement_, value_type);
    }

    [[nodiscard]] static auto reg(const std::string_view name,
                                  const type& value_type) -> operand {

        assert(not name.empty());

        operand result;
        result.kind_ = kind::reg;
        result.type_ptr_ = &value_type;
        result.base_register_ = name;

        return result;
    }

    // e.g. the frame size of a noinline body, defined after the body
    [[nodiscard]] static auto symbol(const std::string_view name,
                                     const type& value_type) -> operand {

        assert(not name.empty());

        operand result;
        result.kind_ = kind::symbol;
        result.type_ptr_ = &value_type;
        result.symbol_ = name;

        return result;
    }
};

// what the methods 'add_subtract', 'bitwise', 'divide', 'shift' and
// 'unary' take, each one a part of it
enum class arithmetic_operator : uint8_t {
    add,
    subtract,
    multiply,
    divide,
    remainder,
    bit_and,
    bit_or,
    bit_xor,
    shift_left,
    shift_right,
    // not taken by the methods: the op of the first element of a list that is
    // neither added nor multiplied, copied with '=' ('expr_arith')
    assign,
    negate,
    complement,
};

// what a comparison asks, e.g. 'a <= b' is 'less_equal'
enum class comparison_operator : uint8_t {
    equal,
    not_equal,
    less,
    less_equal,
    greater,
    greater_equal,
};

// the method of 'machine' that computes an operator
enum class operator_family : uint8_t {
    add_subtract,
    multiply,
    divide,
    bitwise,
    shift,
};

// the facts of a binary arithmetic operator: its spelling, the machine
// method, how tightly it binds (a higher precedence first), whether the
// constant passes may move and merge its constants, the constant that leaves
// a value unchanged, and whether its low bits depend on higher bits of an
// operand, which a narrower width would lose
struct binary_operator_traits {
    arithmetic_operator op;
    std::string_view text;
    operator_family family;
    uint8_t precedence;
    bool is_regroupable;
    std::optional<int64_t> identity;
    bool reads_high_bits;
};

inline constexpr auto binary_operators{
    std::to_array<binary_operator_traits>({
        {
            .op{arithmetic_operator::add},
            .text{"+"},
            .family{operator_family::add_subtract},
            .precedence{1},
            .is_regroupable{true},
            .identity{0},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::subtract},
            .text{"-"},
            .family{operator_family::add_subtract},
            .precedence{1},
            .is_regroupable{true},
            .identity{0},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::multiply},
            .text{"*"},
            .family{operator_family::multiply},
            .precedence{2},
            .is_regroupable{true},
            .identity{1},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::divide},
            .text{"/"},
            .family{operator_family::divide},
            .precedence{2},
            .is_regroupable{},
            .identity{1},
            .reads_high_bits{true},
        },
        {
            .op{arithmetic_operator::remainder},
            .text{"%"},
            .family{operator_family::divide},
            .precedence{2},
            .is_regroupable{},
            .identity{},
            .reads_high_bits{true},
        },
        {
            .op{arithmetic_operator::bit_and},
            .text{"&"},
            .family{operator_family::bitwise},
            .precedence{4},
            .is_regroupable{true},
            .identity{-1},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::bit_or},
            .text{"|"},
            .family{operator_family::bitwise},
            .precedence{3},
            .is_regroupable{true},
            .identity{0},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::bit_xor},
            .text{"^"},
            .family{operator_family::bitwise},
            .precedence{5},
            .is_regroupable{true},
            .identity{0},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::shift_left},
            .text{"<<"},
            .family{operator_family::shift},
            .precedence{6},
            .is_regroupable{},
            .identity{},
            .reads_high_bits{},
        },
        {
            .op{arithmetic_operator::shift_right},
            .text{">>"},
            .family{operator_family::shift},
            .precedence{6},
            .is_regroupable{},
            .identity{},
            .reads_high_bits{true},
        },
    }),
};

// a binary operator, not 'assign' or a unary one
[[nodiscard]] constexpr auto is_binary(const arithmetic_operator op) -> bool {
    return std::ranges::contains(binary_operators, op,
                                 &binary_operator_traits::op);
}

// the row of the binary operator 'op'
[[nodiscard]] constexpr auto traits_of(const arithmetic_operator op)
    -> const binary_operator_traits& {

    assert(is_binary(op));

    for (const binary_operator_traits& traits : binary_operators) {
        if (traits.op == op) {
            return traits;
        }
    }

    std::unreachable();
}

// above every operator, where a list starts
inline constexpr uint8_t list_start_precedence{
    std::ranges::max(binary_operators, {}, &binary_operator_traits::precedence)
            .precedence +
        1,
};

// the spelling of a prefix operator in the source
struct arithmetic_spelling {
    arithmetic_operator op;
    std::string_view text;
};

// the prefix operators of an operand
inline constexpr auto unary_spellings{
    std::to_array<arithmetic_spelling>({
        {.op{arithmetic_operator::negate}, .text{"-"}},
        {.op{arithmetic_operator::complement}, .text{"~"}},
    }),
};

// the operator as written in the source, 'assign' as the '=' of a copy
[[nodiscard]] inline auto source_text(const arithmetic_operator op)
    -> std::string_view {

    if (op == arithmetic_operator::assign) {
        return "=";
    }

    if (is_binary(op)) {
        return traits_of(op).text;
    }

    for (const arithmetic_spelling& spelling : unary_spellings) {
        if (spelling.op == op) {
            return spelling.text;
        }
    }

    std::unreachable();
}

// the binary arithmetic operator 'text' spells, none for any other text
[[nodiscard]] inline auto binary_operator_of(const std::string_view text)
    -> std::optional<arithmetic_operator> {

    for (const binary_operator_traits& traits : binary_operators) {
        if (traits.text == text) {
            return traits.op;
        }
    }

    return std::nullopt;
}

// the prefix operator 'text' spells, none for any other text
[[nodiscard]] inline auto unary_operator_of(const std::string_view text)
    -> std::optional<arithmetic_operator> {

    for (const arithmetic_spelling& spelling : unary_spellings) {
        if (spelling.text == text) {
            return spelling.op;
        }
    }

    return std::nullopt;
}

// the facts of a comparison operator: its spelling, the operator true when
// it is false, and the one with the same result when the operands swap
struct comparison_spelling {
    comparison_operator op;
    std::string_view text;
    comparison_operator negated;
    comparison_operator mirrored;
};

inline constexpr auto comparison_spellings{
    std::to_array<comparison_spelling>({
        {
            .op{comparison_operator::equal},
            .text{"=="},
            .negated{comparison_operator::not_equal},
            .mirrored{comparison_operator::equal},
        },
        {
            .op{comparison_operator::not_equal},
            .text{"!="},
            .negated{comparison_operator::equal},
            .mirrored{comparison_operator::not_equal},
        },
        {
            .op{comparison_operator::less},
            .text{"<"},
            .negated{comparison_operator::greater_equal},
            .mirrored{comparison_operator::greater},
        },
        {
            .op{comparison_operator::less_equal},
            .text{"<="},
            .negated{comparison_operator::greater},
            .mirrored{comparison_operator::greater_equal},
        },
        {
            .op{comparison_operator::greater},
            .text{">"},
            .negated{comparison_operator::less_equal},
            .mirrored{comparison_operator::less},
        },
        {
            .op{comparison_operator::greater_equal},
            .text{">="},
            .negated{comparison_operator::less},
            .mirrored{comparison_operator::less_equal},
        },
    }),
};

// the row of the comparison 'op'
[[nodiscard]] constexpr auto facts_of(const comparison_operator op)
    -> const comparison_spelling& {

    for (const comparison_spelling& spelling : comparison_spellings) {
        if (spelling.op == op) {
            return spelling;
        }
    }

    std::unreachable();
}

// the operator that is true when 'op' is false
[[nodiscard]] constexpr auto negated(const comparison_operator op)
    -> comparison_operator {

    return facts_of(op).negated;
}

// the operator that gives the same result with the operands swapped
[[nodiscard]] constexpr auto mirrored(const comparison_operator op)
    -> comparison_operator {

    return facts_of(op).mirrored;
}

// negating or mirroring twice gives the operator back
static_assert(std::ranges::all_of(
    comparison_spellings, [](const comparison_spelling& spelling) -> bool {
        return negated(spelling.negated) == spelling.op and
               mirrored(spelling.mirrored) == spelling.op;
    }));

// the operator as written in the source
[[nodiscard]] inline auto source_text(const comparison_operator op)
    -> std::string_view {

    return facts_of(op).text;
}

// the comparison 'text' spells, none for any other text
[[nodiscard]] inline auto comparison_of(const std::string_view text)
    -> std::optional<comparison_operator> {

    for (const comparison_spelling& spelling : comparison_spellings) {
        if (spelling.text == text) {
            return spelling.op;
        }
    }

    return std::nullopt;
}
