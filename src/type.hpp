#pragma once
// reviewed: 2025-09-28
//           2026-09-09

#include <cstdint>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "token.hpp"

class type;

struct type_field {
    std::string name;       //
    const type* type_ptr{}; // element type
    size_t offset{};        // offset relative to instance address
    size_t size_bytes{};    // total size in bytes of all elements
    size_t array_count{};   // array size in elements
    bool is_array{};        //

    [[nodiscard]] auto type() const -> const type& { return *type_ptr; }
};

class type final {
    std::string name_;
    size_t size_bytes_{}; // total size of type in bytes
    std::vector<type_field> fields_;
    bool is_builtin_{};

  public:
    type(const std::string_view name, const size_t size_bytes,
         const bool is_builtin)
        : name_{name}, size_bytes_{size_bytes}, is_builtin_{is_builtin} {}

    type() = default;

    auto add_field([[maybe_unused]] const token& src_loc_tk,
                   const std::string_view name, const type& tp,
                   const bool is_array, const size_t array_count) -> void {

        const size_t total_size_bytes{
            multiply_storage_size(tp.size_bytes_, is_array ? array_count : 1)};

        fields_.emplace_back(std::string{name}, &tp, size_bytes_,
                             total_size_bytes, array_count, is_array);

        size_bytes_ = add_storage_size(size_bytes_, total_size_bytes);
    }

    [[nodiscard]] auto field(const token& src_loc_tk,
                             const std::string_view name) const
        -> const type_field& {

        for (const type_field& f : fields_) {
            if (f.name == name) {
                return f;
            }
        }

        throw compiler_exception{
            src_loc_tk,
            std::format("field '{}' not found in type '{}'", name, name_)};
    }

    [[nodiscard]] auto
    accessor(const token& src_loc_tk, const std::string_view ident,
             const std::vector<std::string>& path, const var_info& var,
             const std::string_view base_register) const -> ident_info {

        std::vector<const type*> type_path;

        type_path.emplace_back(this);

        size_t offset{};
        bool is_array{var.is_array};
        size_t array_count{var.array_len};

        const type* tp{this};
        for (const std::string& field_name : path | std::views::drop(1)) {
            // note: drop 1 because the first element is retrieved outside the
            //       loop

            const type_field& tf{tp->field(src_loc_tk, field_name)};
            offset = add_storage_size(offset, tf.offset);
            tp = tf.type_ptr;
            is_array = tf.is_array;
            array_count = tf.array_count;
            type_path.emplace_back(tp);
        }

        const int64_t idx{
            var.reg.is_empty()
                ? add_address_offset(var.offset, address_offset(offset))
                : address_offset(offset)};

        const std::string_view storage_base{
            var.base_register.empty() ? base_register : var.base_register};

        // find first field so operand gets a valid built-in
        while (not tp->is_builtin()) {
            tp = tp->fields_[0].type_ptr;
        }

        const operand op{operand::mem(
            var.reg.is_empty() ? storage_base : var.reg.base_register(), "", 1,
            idx, *tp)};

        return ident_info::make_var(std::string{ident}, path,
                                    std::move(type_path), op, idx, array_count,
                                    is_array, var.is_pointer);
    }

    [[nodiscard]] auto
    field_offset(const token& src_loc_tk,
                 const std::span<const std::string> path) const -> size_t {

        size_t offset{};

        const type* tp{this};
        for (const std::string& field_name : path | std::views::drop(1)) {
            // note: drop 1 because the first element is retrieved outside the
            //       loop

            const type_field& tf{tp->field(src_loc_tk, field_name)};
            offset = add_storage_size(offset, tf.offset);
            tp = tf.type_ptr;
        }

        return offset;
    }

    [[nodiscard]] auto field_offset(const token& src_loc_tk,
                                    const std::string_view field_name) const
        -> size_t {

        size_t offset{};

        for (const type_field& tf : fields_) {
            if (tf.name == field_name) {
                return offset;
            }
            offset += tf.size_bytes;
        }

        throw compiler_exception(
            src_loc_tk, std::format("field '{}' not found in type '{}'",
                                    field_name, name_));
    }

    [[nodiscard]] auto size_bytes() const -> size_t { return size_bytes_; }

    [[nodiscard]] auto name() const -> const std::string& { return name_; }

    auto set_name(const std::string_view name) -> void { name_ = name; }

    [[nodiscard]] auto is_builtin() const -> bool { return is_builtin_; }

    [[nodiscard]] auto fields() const -> std::span<const type_field> {
        return fields_;
    }

    [[nodiscard]] auto remaining_fields_size_bytes(const size_t first) const
        -> size_t {

        size_t size_bytes{};
        for (const type_field& f : fields_ | std::views::drop(first)) {
            size_bytes = add_storage_size(size_bytes, f.size_bytes);
        }

        return size_bytes;
    }
};
