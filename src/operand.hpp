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

#include "compile_error.hpp"
#include "token.hpp"

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
    // the first element of a list that is neither added nor multiplied
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

// the operator that gives the same result with the operands swapped
[[nodiscard]] inline auto mirrored(const comparison_operator op)
    -> comparison_operator {

    if (op == comparison_operator::less) {
        return comparison_operator::greater;
    }

    if (op == comparison_operator::less_equal) {
        return comparison_operator::greater_equal;
    }

    if (op == comparison_operator::greater) {
        return comparison_operator::less;
    }

    if (op == comparison_operator::greater_equal) {
        return comparison_operator::less_equal;
    }

    return op;
}

// the spelling of each binary arithmetic operator in the source
struct arithmetic_spelling {
    arithmetic_operator op;
    std::string_view text;
};

inline constexpr auto arithmetic_spellings{
    std::to_array<arithmetic_spelling>({
        {.op{arithmetic_operator::add}, .text{"+"}},
        {.op{arithmetic_operator::subtract}, .text{"-"}},
        {.op{arithmetic_operator::multiply}, .text{"*"}},
        {.op{arithmetic_operator::divide}, .text{"/"}},
        {.op{arithmetic_operator::remainder}, .text{"%"}},
        {.op{arithmetic_operator::bit_and}, .text{"&"}},
        {.op{arithmetic_operator::bit_or}, .text{"|"}},
        {.op{arithmetic_operator::bit_xor}, .text{"^"}},
        {.op{arithmetic_operator::shift_left}, .text{"<<"}},
        {.op{arithmetic_operator::shift_right}, .text{">>"}},
    }),
};

// the operator as written in the source, 'assign' as the '=' of a copy
[[nodiscard]] inline auto source_text(const arithmetic_operator op)
    -> std::string_view {

    if (op == arithmetic_operator::negate) {
        return "-";
    }

    if (op == arithmetic_operator::assign) {
        return "=";
    }

    for (const arithmetic_spelling& spelling : arithmetic_spellings) {
        if (spelling.op == op) {
            return spelling.text;
        }
    }

    // note: 'complement' is a unary operator, 'unary_ops' writes it
    std::unreachable();
}

// the binary arithmetic operator 'text' spells, none for any other text
[[nodiscard]] inline auto binary_operator_of(const std::string_view text)
    -> std::optional<arithmetic_operator> {

    for (const arithmetic_spelling& spelling : arithmetic_spellings) {
        if (spelling.text == text) {
            return spelling.op;
        }
    }

    return std::nullopt;
}

// the spelling of each comparison operator in the source
struct comparison_spelling {
    comparison_operator op;
    std::string_view text;
};

inline constexpr auto comparison_spellings{
    std::to_array<comparison_spelling>({
        {.op{comparison_operator::equal}, .text{"=="}},
        {.op{comparison_operator::not_equal}, .text{"!="}},
        {.op{comparison_operator::less}, .text{"<"}},
        {.op{comparison_operator::less_equal}, .text{"<="}},
        {.op{comparison_operator::greater}, .text{">"}},
        {.op{comparison_operator::greater_equal}, .text{">="}},
    }),
};

// the operator as written in the source
[[nodiscard]] inline auto source_text(const comparison_operator op)
    -> std::string_view {

    for (const comparison_spelling& spelling : comparison_spellings) {
        if (spelling.op == op) {
            return spelling.text;
        }
    }

    std::unreachable();
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
