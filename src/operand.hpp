#pragma once
// reviewed: 2025-09-28

// the backend value type and the storage size and address offset arithmetic

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "token.hpp"

class type;

// what the storage size functions below accept
namespace storage_limits {

// alignment padding of a size at the limit stays in the signed 64-bit range
inline constexpr size_t max_size_bytes{
    static_cast<size_t>(std::numeric_limits<int64_t>::max()) - 16,
};
// note: -16 leaves room for the alignment padding

} // namespace storage_limits

[[nodiscard]] inline auto add_storage_size(const token& src_loc_tk,
                                           const size_t base,
                                           const size_t size_bytes) -> size_t {

    if (size_bytes > storage_limits::max_size_bytes or
        base > storage_limits::max_size_bytes - size_bytes) {

        throw compiler_exception{src_loc_tk,
                                 "storage size exceeds signed 64-bit range"};
    }

    return base + size_bytes;
}

[[nodiscard]] inline auto multiply_storage_size(const token& src_loc_tk,
                                                const size_t size_bytes,
                                                const size_t count) -> size_t {

    if (count != 0 and size_bytes > storage_limits::max_size_bytes / count) {
        throw compiler_exception{src_loc_tk,
                                 "storage size exceeds signed 64-bit range"};
    }

    return size_bytes * count;
}

// the sum of sizes that 'add_storage_size' or 'multiply_storage_size' already
// accepted, in a place with no source location
[[nodiscard]] inline auto sum_storage_size(const size_t base,
                                           const size_t size_bytes) -> size_t {

    assert(size_bytes <= storage_limits::max_size_bytes);
    assert(base <= storage_limits::max_size_bytes - size_bytes);

    return base + size_bytes;
}

// rounds 'size_bytes' up to a multiple of 'alignment', a power of two
[[nodiscard]] inline auto align_storage_size(const size_t size_bytes,
                                             const size_t alignment) -> size_t {

    assert(std::has_single_bit(alignment));

    return sum_storage_size(size_bytes,
                            (alignment - (size_bytes % alignment)) % alignment);
}

// the alignment known at 'offset' bytes from an address aligned to
// 'alignment'
[[nodiscard]] inline auto offset_alignment(const size_t offset,
                                           const size_t alignment) -> size_t {

    if (offset == 0) {
        return alignment;
    }

    const unsigned trailing_zeros{
        static_cast<unsigned>(std::countr_zero(offset)),
    };

    return std::min(alignment, size_t{1} << trailing_zeros);
}

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

class operand final {
    enum class kind : uint8_t { empty, reg, memory, immediate };

    kind kind_{kind::empty};
    const type* type_ptr_{};
    std::string allocation_register_;
    std::string base_register_;
    std::string index_register_;
    std::string immediate_;
    int64_t displacement_{};
    uint64_t scale_{1};

  public:
    operand() = default;

    [[nodiscard]] auto allocation_register() const -> const std::string& {
        return allocation_register_;
    }

    [[nodiscard]] auto base_register() const -> const std::string& {
        return base_register_;
    }

    [[nodiscard]] auto displacement() const -> int64_t { return displacement_; }

    [[nodiscard]] auto immediate() const -> const std::string& {
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
               index_register_.empty() and displacement_ == 0 and
               immediate_.empty();
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

    [[nodiscard]] auto scale() const -> uint64_t { return scale_; }

    auto set_allocation_register(const std::string_view name) -> void {
        assert(is_register());

        allocation_register_ = name;
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(type_ptr_);

        return *type_ptr_;
    }

    //
    // statics
    //

    [[nodiscard]] static auto imm(std::string value, const type& value_type)
        -> operand {

        assert(not value.empty());

        operand result;
        result.kind_ = kind::immediate;
        result.type_ptr_ = &value_type;
        result.immediate_ = std::move(value);

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
};
