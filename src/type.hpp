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
    size_t size{};          // total size in bytes of all elements
    size_t array_size{};    // array size in elements
    bool is_array{};        //

    [[nodiscard]] auto type() const -> const type& { return *type_ptr; }
};

class type final {
    std::string name_;
    size_t size_{}; // total size of type in bytes
    std::vector<type_field> fields_;
    bool is_builtin_{};

  public:
    type(const std::string_view name, const size_t size, const bool is_builtin)
        : name_{name}, size_{size}, is_builtin_{is_builtin} {}

    type() = default;

    auto add_field([[maybe_unused]] const token& src_loc_tk,
                   const std::string_view name, const type& tp,
                   const bool is_array, const size_t array_size) -> void {

        const size_t total_size{tp.size_ * (is_array ? array_size : 1)};

        fields_.emplace_back(std::string{name}, &tp, size_, total_size,
                             array_size, is_array);

        size_ += total_size;
    }

    [[nodiscard]] auto field(const token& src_loc_tk,
                             const std::string_view name) const
        -> const type_field& {

        for (const type_field& fld : fields_) {
            if (fld.name == name) {
                return fld;
            }
        }

        throw compiler_exception{
            src_loc_tk,
            std::format("field '{}' not found in type '{}'", name, name_)};
    }

    [[nodiscard]] auto
    accessor(const token& src_loc_tk, const std::string_view ident,
             const std::vector<std::string>& path, const var_info& var,
             const std::string_view variables_base_register) const
        -> ident_info {

        std::vector<const type*> type_path;

        type_path.emplace_back(this);

        size_t offset{};
        bool is_array{var.is_array};
        size_t array_size{var.array_size};

        const type* tp{this};
        for (const std::string& field_name : path | std::views::drop(1)) {
            // note: drop 1 because the first element is retrieved outside the
            //       loop

            const type_field& tf{tp->field(src_loc_tk, field_name)};
            offset += tf.offset;
            tp = tf.type_ptr;
            is_array = tf.is_array;
            array_size = tf.array_size;
            type_path.emplace_back(tp);
        }

        const int32_t stack_idx{
            var.reg.is_empty() ? var.stack_idx + static_cast<int32_t>(offset)
                               : static_cast<int32_t>(offset)};

        // find the first built-in type to have a valid operand size for the
        // address

        const type* tp_first_field{tp};
        while (not tp_first_field->is_builtin()) {
            tp_first_field = tp_first_field->fields_[0].type_ptr;
        }

        operand op{operand::mem(var.reg.is_empty() ? variables_base_register
                                                   : var.reg.base_register,
                                "", 1, stack_idx)};
        op.size = tp_first_field->size();
        op.type_ptr = tp_first_field;

        return ident_info::make_var(std::string{ident}, path,
                                    std::move(type_path), op, stack_idx,
                                    array_size, is_array);
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
            offset += tf.offset;
            tp = tf.type_ptr;
        }

        return offset;
    }

    [[nodiscard]] auto size() const -> size_t { return size_; }

    [[nodiscard]] auto name() const -> const std::string& { return name_; }

    auto set_name(const std::string_view nm) -> void { name_ = nm; }

    [[nodiscard]] auto is_builtin() const -> bool { return is_builtin_; }

    [[nodiscard]] auto fields() const -> std::span<const type_field> {
        return fields_;
    }

    [[nodiscard]] auto remaining_fields_size(const size_t first) const
        -> size_t {

        size_t bytes{};
        for (const type_field& field : fields_ | std::views::drop(first)) {
            bytes += field.size;
        }

        return bytes;
    }
};
