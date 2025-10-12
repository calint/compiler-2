#pragma once
// reviewed: 2025-09-28

#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "token.hpp"

class type;

struct type_field {
    std::string_view name;
    const type* tp{};
    size_t size{};
    size_t array_size{};
    bool is_array{};
    size_t offset{};
};

struct accessor_info {
    std::string id_nasm;
    const type& tp;
    bool is_array{};
    size_t array_size{};
    size_t size{};
};

class type final {
    static constexpr std::string_view size_qword{"qword"};
    static constexpr std::string_view size_dword{"dword"};
    static constexpr std::string_view size_word{"word"};
    static constexpr std::string_view size_byte{"byte"};

    std::string_view name_;
    size_t size_{};
    std::vector<type_field> fields_;
    bool is_built_in_{};

  public:
    type(const std::string_view name, const size_t size, const bool is_built_in)
        : name_{name}, size_{size}, is_built_in_{is_built_in} {}

    ~type() = default;

    type() = default;
    type(const type&) = default;
    type(type&&) = default;
    auto operator=(const type&) -> type& = default;
    auto operator=(type&&) -> type& = default;

    auto add_field([[maybe_unused]] const token& tk,
                   const std::string_view name, const type& tp,
                   const bool is_array, const size_t array_size) -> void {

        fields_.emplace_back(
            type_field{.name = name,
                       .tp = &tp,
                       .size = tp.size_ * (is_array ? array_size : 1),
                       .array_size = array_size,
                       .is_array = is_array,
                       .offset = size_});

        size_ += tp.size_ * (is_array ? array_size : 1);
    }

    [[nodiscard]] auto field(const token& tk, const std::string_view name) const
        -> const type_field& {

        for (const type_field& fld : fields_) {
            if (fld.name == name) {
                return fld;
            }
        }

        throw compiler_exception{
            tk, std::format("field '{}' not found in type '{}'", name, name_)};
    }

    [[nodiscard]] auto
    accessor(const token& tk, const std::vector<std::string_view>& path,
             const int stack_idx_base, const bool base_is_array,
             const size_t base_array_size, const size_t base_size) const
        -> accessor_info {

        const type* tp{this};
        size_t offset{};
        bool is_array{base_is_array};
        size_t array_size{base_array_size};
        size_t size{base_size};
        for (const auto& field_name : path | std::views::drop(1)) {
            const type_field& fld{tp->field(tk, field_name)};
            offset += fld.offset;
            tp = fld.tp;
            is_array = fld.is_array;
            array_size = fld.array_size;
            size = fld.size;
        }
        const int stack_idx{stack_idx_base + static_cast<int>(offset)};

        // find the first built-in type to have a valid operand size for the
        // address
        const type* tp_first_field{tp};
        while (not tp_first_field->is_built_in()) {
            tp_first_field = tp_first_field->fields_.at(0).tp;
        }

        const std::string_view memsize{
            type::get_size_specifier(tk, tp_first_field->size())};
        const std::string accessor{
            std::format("{} [rsp - {}]", memsize, -stack_idx)};
        // note: -stack_idx for nicer source formatting
        return {.id_nasm = accessor,
                .tp = *tp,
                .is_array = is_array,
                .array_size = array_size,
                .size = size};
    }

    [[nodiscard]] auto size() const -> size_t { return size_; }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    auto set_name(const std::string_view nm) -> void { name_ = nm; }

    [[nodiscard]] auto is_built_in() const -> bool { return is_built_in_; }

    [[nodiscard]] auto fields() const -> const std::vector<type_field>& {
        return fields_;
    }

    static auto get_size_specifier(const token& tk, const size_t size)
        -> std::string_view {
        switch (size) {
        case 8:
            return size_qword;
        case 4:
            return size_dword;
        case 2:
            return size_word;
        case 1:
            return size_byte;
        default:
            throw compiler_exception{
                tk, std::format("illegal size for memory operand: {}", size)};
        }
    }
};
