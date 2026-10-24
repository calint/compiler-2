#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "operand.hpp"
#include "token.hpp"

class type;

struct type_field {
    std::string name;
    const type* type_ptr{}; // element type
    size_t offset{};        // offset relative to instance address
    size_t size_bytes{};    // total size in bytes of all elements
    // array size in elements, empty when not an array
    std::optional<size_t> array_count;

    // 1 for a field that is not an array
    [[nodiscard]] auto element_count() const -> size_t {
        return array_count.value_or(1);
    }

    [[nodiscard]] auto is_array() const -> bool {
        return array_count.has_value();
    }

    [[nodiscard]] auto type() const -> const type& { return *type_ptr; }
};

// a run of bytes at 'offset' within a value
struct byte_range {
    size_t offset{};
    size_t size_bytes{};

    [[nodiscard]] auto overlaps(const byte_range& other) const -> bool {
        return offset < other.offset + other.size_bytes and
               other.offset < offset + size_bytes;
    }
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
            .array_count{array_count},
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
                f.type().data_ranges(max_count),
            };

            const bool is_unpadded{
                element_ranges.size() == 1 and
                    element_ranges.front().size_bytes == f.type().size_bytes_,
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
                    f.offset + (i * f.type().size_bytes_),
                };

                for (const byte_range& r : element_ranges) {
                    append_range(ranges, element_offset + r.offset,
                                 r.size_bytes);
                }
            }
        }

        return ranges;
    }

    [[nodiscard]] auto field(const token& src_loc_tk,
                             const std::string_view name) const
        -> const type_field& {

        return fields_.at(field_index(src_loc_tk, name));
    }

    // the field and the padding after it up to the next field or the end
    [[nodiscard]] auto
    field_extent_bytes(const token& src_loc_tk,
                       const std::string_view field_name) const -> size_t {

        const size_t i{field_index(src_loc_tk, field_name)};

        const size_t next_offset{
            i + 1 < fields_.size() ? fields_.at(i + 1).offset : size_bytes_,
        };

        return next_offset - fields_.at(i).offset;
    }

    [[nodiscard]] auto
    field_offset(const token& src_loc_tk,
                 const std::span<const std::string> path) const -> size_t {

        size_t offset{};

        const type* tp{this};
        for (const std::string& field_name : path | std::views::drop(1)) {
            // note: drop 1 because the first element names the variable of
            //       this type, not a field

            const type_field& tf{tp->field(src_loc_tk, field_name)};
            offset = sum_storage_size(offset, tf.offset);
            tp = tf.type_ptr;
        }

        return offset;
    }

    [[nodiscard]] auto field_offset(const token& src_loc_tk,
                                    const std::string_view field_name) const
        -> size_t {

        return field(src_loc_tk, field_name).offset;
    }

    [[nodiscard]] auto fields() const -> std::span<const type_field> {
        return fields_;
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

    auto set_name(const std::string_view name) -> void { name_ = name; }

    [[nodiscard]] auto size_bits() const -> size_t {
        return size_bytes_ * std::numeric_limits<unsigned char>::digits;
    }

    [[nodiscard]] auto size_bytes() const -> size_t { return size_bytes_; }

  private:
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

    // fields that touch each other form one range
    static auto append_range(std::vector<byte_range>& ranges,
                             const size_t offset, const size_t size_bytes)
        -> void {

        // note: a field or an array is never empty
        assert(size_bytes != 0);

        if (not ranges.empty() and
            ranges.back().offset + ranges.back().size_bytes == offset) {

            ranges.back().size_bytes += size_bytes;
            return;
        }

        ranges.push_back({
            .offset{offset},
            .size_bytes{size_bytes},
        });
    }
};

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
