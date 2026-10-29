#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "token.hpp"

//
// a type and its storage, the types and functions below in reading order:
//
//   'storage_limits', 'add_storage_size' and the other size functions
//                            sizes and alignments, checked against the limit
//   'array_shape'            not an array, or an array of a length
//   'type_field', 'byte_range', 'padding_gap', 'type_kind'
//                            the parts of a type
//   'type'                   a built-in or user type with its fields
//   'builtin_type_set'       the built-in types of a compile
//

// what the storage size functions below accept
namespace storage_limits {

// every sum, product and aligned size of storage is checked against it, so
// the offsets stay in the signed 64-bit range
inline constexpr size_t max_size_bytes{
    static_cast<size_t>(std::numeric_limits<int64_t>::max()) - 16,
};

// note: -16, the largest alignment (the stack's), so that a size within the
//       limit rounded up to an alignment stays in the signed 64-bit range

} // namespace storage_limits

[[nodiscard]] inline auto add_storage_size(const token& src_loc_tk,
                                           const size_t base,
                                           const size_t size_bytes) -> size_t {

    if (size_bytes > storage_limits::max_size_bytes or
        base > storage_limits::max_size_bytes - size_bytes) {

        throw compile_error{src_loc_tk,
                            "storage size exceeds signed 64-bit range"};
    }

    return base + size_bytes;
}

[[nodiscard]] inline auto multiply_storage_size(const token& src_loc_tk,
                                                const size_t size_bytes,
                                                const size_t count) -> size_t {

    if (count != 0 and size_bytes > storage_limits::max_size_bytes / count) {
        throw compile_error{src_loc_tk,
                            "storage size exceeds signed 64-bit range"};
    }

    return size_bytes * count;
}

// the product of a size and a count that an accepted size bounds, e.g. some
// elements of an array whose size was accepted
[[nodiscard]] inline auto product_storage_size(const size_t size_bytes,
                                               const size_t count) -> size_t {

    assert(count == 0 or size_bytes <= storage_limits::max_size_bytes / count);

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

// the bytes that round 'size_bytes' up to a multiple of 'alignment', a power
// of two
[[nodiscard]] inline auto alignment_padding_bytes(const size_t size_bytes,
                                                  const size_t alignment)
    -> size_t {

    assert(std::has_single_bit(alignment));

    return (alignment - (size_bytes % alignment)) % alignment;
}

// rounds 'size_bytes' up to a multiple of 'alignment', a power of two
[[nodiscard]] inline auto align_storage_size(const size_t size_bytes,
                                             const size_t alignment) -> size_t {

    return sum_storage_size(size_bytes,
                            alignment_padding_bytes(size_bytes, alignment));
}

// 'align_storage_size' of a size that the source gives, which the padding may
// take past the limit, e.g. the end of the fields of a type
[[nodiscard]] inline auto checked_align_storage_size(const token& src_loc_tk,
                                                     const size_t size_bytes,
                                                     const size_t alignment)
    -> size_t {

    return add_storage_size(src_loc_tk, size_bytes,
                            alignment_padding_bytes(size_bytes, alignment));
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

class type;

// the length that an array of unknown length is parsed and compiled with,
// what it gives is discarded, see 'array_shape::is_length_unknown'
inline constexpr size_t discarded_array_len{};

// whether a name is an array and of how many elements: not an array, an
// array of a length, or an unsized array parameter without one
class array_shape final {
    bool is_array_{};
    std::optional<size_t> length_;

  public:
    // not an array
    array_shape() = default;

    // 1 for a name that is not an array, see 'discarded_array_len'
    [[nodiscard]] auto element_count() const -> size_t {
        return is_array_ ? length_.value_or(discarded_array_len) : 1;
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    // an unsized array parameter where no call gave it a length: in the parse
    // and in the check of a function without its calls
    [[nodiscard]] auto is_length_unknown() const -> bool {
        return is_array_ and not length_;
    }

    // empty when not an array or not known
    [[nodiscard]] auto length() const -> std::optional<size_t> {
        return length_;
    }

    // the bytes of a value of elements of 'element_type'
    [[nodiscard]] auto value_size_bytes(const token& src_loc_tk,
                                        const type& element_type) const
        -> size_t;

    //
    // statics
    //

    // an array, of unknown length when 'length' is empty
    [[nodiscard]] static auto array(const std::optional<size_t> length)
        -> array_shape {

        return {true, length};
    }

  private:
    array_shape(const bool is_array, const std::optional<size_t> length)
        : is_array_{is_array}, length_{length} {}
};

struct type_field {
    std::string name;
    const type* type_ptr{}; // element type
    size_t offset{};        // offset relative to instance address
    size_t size_bytes{};    // total size in bytes of all elements
    // an array of a length, or not an array
    array_shape shape;

    // 1 for a field that is not an array
    [[nodiscard]] auto element_count() const -> size_t {
        return shape.element_count();
    }

    [[nodiscard]] auto is_array() const -> bool { return shape.is_array(); }

    [[nodiscard]] auto type_ref() const -> const type& { return *type_ptr; }
};

// a run of bytes at 'offset' within a value
struct byte_range {
    size_t offset{};
    size_t size_bytes{};

    // the offset after the last byte
    [[nodiscard]] auto end() const -> size_t { return offset + size_bytes; }
};

// bytes of an instance that no field covers, from its start, e.g. the 3 B at
// offset 1 of 'type r { a i8, b i32 }'; 'alignment' is known for the first
// byte
struct padding_gap {
    size_t offset{};
    size_t size_bytes{};
    size_t alignment{};
};

// a type the source defines, or a built-in: an integer, the 'bool' that
// machines treat differently from the integer of its size, or the 'void' of
// no value
enum class type_kind : uint8_t { user, integer, boolean, empty };

// a built-in type or a user type with its fields at aligned offsets; the
// size is rounded up so that the elements of an array stay aligned
class type final {
    std::string name_;
    size_t size_bytes_{};       // total size of type in bytes including padding
    size_t fields_end_bytes_{}; // end of the last field before tail padding
    size_t alignment_{1};
    std::vector<type_field> fields_;
    type_kind kind_{type_kind::user};

  public:
    // builtins are aligned to their size, a zero size needs no alignment
    type(const std::string_view name, const size_t size_bytes,
         const type_kind kind)
        : name_{name}, size_bytes_{size_bytes},
          alignment_{std::max(size_bytes, size_t{1})}, kind_{kind} {}

    type() = default;

    // fields are placed at offsets aligned to their type and the size is
    // rounded up so that array elements stay aligned; 'array_count' is empty
    // for a field that is not an array
    auto add_field(const token& src_loc_tk, const std::string_view name,
                   const type& tp, const std::optional<size_t> array_count)
        -> void {

        type_field field{
            .name{name},
            .type_ptr{&tp},
            .offset{
                checked_align_storage_size(src_loc_tk, fields_end_bytes_,
                                           tp.alignment_),
            },
            .size_bytes{},
            .shape{
                array_count ? array_shape::array(array_count) : array_shape{},
            },
        };

        field.size_bytes = multiply_storage_size(src_loc_tk, tp.size_bytes_,
                                                 field.element_count());

        fields_end_bytes_ =
            add_storage_size(src_loc_tk, field.offset, field.size_bytes);

        fields_.emplace_back(std::move(field));

        alignment_ = std::max(alignment_, tp.alignment_);

        size_bytes_ = checked_align_storage_size(src_loc_tk, fields_end_bytes_,
                                                 alignment_);
    }

    [[nodiscard]] auto alignment() const -> size_t { return alignment_; }

    [[nodiscard]] auto field(const token& src_loc_tk,
                             const std::string_view name) const
        -> const type_field& {

        return fields_.at(field_index(src_loc_tk, name));
    }

    [[nodiscard]] auto fields() const -> std::span<const type_field> {
        return fields_;
    }

    // the type a memory operand of this type is read as: the first built-in
    // field of an instance, which starts at its address
    [[nodiscard]] auto first_builtin() const -> const type& {
        const type* tp{this};

        while (not tp->is_builtin()) {
            tp = tp->fields().front().type_ptr;
        }

        return *tp;
    }

    [[nodiscard]] auto has_field(const std::string_view name) const -> bool {
        return std::ranges::contains(fields_, name, &type_field::name);
    }

    [[nodiscard]] auto is_bool() const -> bool {
        return kind_ == type_kind::boolean;
    }

    [[nodiscard]] auto is_builtin() const -> bool {
        return kind_ != type_kind::user;
    }

    [[nodiscard]] auto is_integer() const -> bool {
        return kind_ == type_kind::integer;
    }

    // types are unique objects, a type is only the same as itself
    [[nodiscard]] auto is_same(const type& other) const -> bool {
        return this == &other;
    }

    [[nodiscard]] auto name() const -> const std::string& { return name_; }

    // where the padding after the field ends: at the next field, none for
    // the last field
    [[nodiscard]] auto
    next_field_offset(const token& src_loc_tk,
                      const std::string_view field_name) const
        -> std::optional<size_t> {

        const size_t i{field_index(src_loc_tk, field_name)};

        if (i + 1 == fields_.size()) {
            return std::nullopt;
        }

        return fields_.at(i + 1).offset;
    }

    // the bytes that no field covers, between the data ranges and after the
    // last one; the first of them when there are more than 'max_count'
    [[nodiscard]] auto padding_gaps(const size_t max_count) const
        -> std::vector<padding_gap> {

        // note: + 1 because a gap is between two ranges, so more than
        //       'max_count' + 1 ranges give more than 'max_count' gaps

        const size_t max_range_count{max_count + 1};

        const std::vector<byte_range> ranges{data_ranges(max_range_count)};

        std::vector<padding_gap> gaps;
        size_t covered_bytes{};

        for (const byte_range& r : ranges) {
            append_gap(gaps, covered_bytes, r.offset, alignment_);
            covered_bytes = r.end();
        }

        // the last range of a stopped search may be incomplete
        if (ranges.size() > max_range_count) {
            return gaps;
        }

        append_gap(gaps, covered_bytes, size_bytes_, alignment_);

        return gaps;
    }

    auto set_name(const std::string_view name) -> void { name_ = name; }

    [[nodiscard]] auto size_bits() const -> size_t {
        return size_bytes_ * std::numeric_limits<unsigned char>::digits;
    }

    [[nodiscard]] auto size_bytes() const -> size_t { return size_bytes_; }

  private:
    // the bytes of a value without padding, adjacent fields are merged; stops
    // at more than 'max_count' ranges, the last of which may then be
    // incomplete, so the work does not grow with the size of an array
    [[nodiscard]] auto data_ranges(const size_t max_count) const
        -> std::vector<byte_range> {

        std::vector<byte_range> ranges;

        if (is_builtin()) {
            append_range(ranges, 0, size_bytes_);
            return ranges;
        }

        for (const type_field& f : fields_) {
            if (ranges.size() > max_count) {
                break;
            }

            const std::vector<byte_range> element_ranges{
                f.type_ref().data_ranges(max_count),
            };

            const bool is_unpadded{
                element_ranges.size() == 1 and
                    element_ranges.front().size_bytes ==
                        f.type_ref().size_bytes_,
            };

            if (is_unpadded) {
                append_range(ranges, f.offset, f.size_bytes);
                continue;
            }

            // note: each padded element after the first adds a range, so the
            //       loop ends within 'max_count' + 2 elements

            for (size_t i{};
                 i < f.element_count() and ranges.size() <= max_count; ++i) {

                const size_t element_offset{
                    f.offset + (i * f.type_ref().size_bytes_),
                };

                for (const byte_range& r : element_ranges) {
                    append_range(ranges, element_offset + r.offset,
                                 r.size_bytes);
                }
            }
        }

        return ranges;
    }

    [[nodiscard]] auto field_index(const token& src_loc_tk,
                                   const std::string_view name) const
        -> size_t {

        for (size_t i{}; i < fields_.size(); ++i) {
            if (fields_.at(i).name == name) {
                return i;
            }
        }

        // e.g. the 'x' of 'true.x', like 'constant 'c' has no fields'
        if (is_builtin()) {
            throw compile_error{src_loc_tk, std::format("type {} has no fields",
                                                        in_quotes(name_))};
        }

        throw compile_error{src_loc_tk,
                            std::format("field {} not found in type {}",
                                        in_quotes(name), in_quotes(name_))};
    }

    //
    // statics
    //

    // the bytes from 'start' to 'end' are a gap unless there are none,
    // 'alignment' is that of the instance
    static auto append_gap(std::vector<padding_gap>& gaps, const size_t start,
                           const size_t end, const size_t alignment) -> void {

        if (start == end) {
            return;
        }

        gaps.push_back({
            .offset{start},
            .size_bytes{end - start},
            .alignment{offset_alignment(start, alignment)},
        });
    }

    // fields that touch each other form one range
    static auto append_range(std::vector<byte_range>& ranges,
                             const size_t offset, const size_t size_bytes)
        -> void {

        // note: a field or an array is never empty
        assert(size_bytes != 0);

        if (not ranges.empty() and ranges.back().end() == offset) {

            ranges.back().size_bytes += size_bytes;
            return;
        }

        ranges.push_back({
            .offset{offset},
            .size_bytes{size_bytes},
        });
    }
};

inline auto array_shape::value_size_bytes(const token& src_loc_tk,
                                          const type& element_type) const
    -> size_t {

    return multiply_storage_size(src_loc_tk, element_type.size_bytes(),
                                 element_count());
}

// the types of the language that the source does not define, made once before
// the machine and the program, which refer to them; a type is compared by
// address, so the set is not copied
class builtin_type_set final {
  public:
    builtin_type_set() = default;
    builtin_type_set(const builtin_type_set&) = delete;
    builtin_type_set(builtin_type_set&&) = delete;
    auto operator=(const builtin_type_set&) -> builtin_type_set& = delete;
    auto operator=(builtin_type_set&&) -> builtin_type_set& = delete;
    ~builtin_type_set() = default;

    const type void_type{"void", 0, type_kind::empty};
    const type boolean{"bool", sizeof(int8_t), type_kind::boolean};
    const type i64{"i64", sizeof(int64_t), type_kind::integer};
    const type i32{"i32", sizeof(int32_t), type_kind::integer};
    const type i16{"i16", sizeof(int16_t), type_kind::integer};
    const type i8{"i8", sizeof(int8_t), type_kind::integer};

    static constexpr size_t type_count{6};

    // every one, the source names them like the types it defines
    [[nodiscard]] auto all() const -> std::array<const type*, type_count> {
        return {&i64, &i32, &i16, &i8, &boolean, &void_type};
    }
};
