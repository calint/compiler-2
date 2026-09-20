#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "decouple.hpp"

class token;
class type;

class machine {
  public:
    machine() = default;
    machine(const machine&) = delete;
    machine(machine&&) = delete;
    auto operator=(const machine&) -> machine& = delete;
    auto operator=(machine&&) -> machine& = delete;
    virtual ~machine() = default;

    struct comparison_action {
        std::string_view operation;
        bool inverted{};
        operand destination;
        std::string_view target;
        bool branch_on_true{};
    };

    struct bounds_check_options {
        bool upper{};
        bool lower{};
        bool with_line{};
    };

    struct data_initializer {
        int64_t value{};
        std::string_view uops; // unary operations
    };

    [[nodiscard]] virtual auto default_type() const -> const type& = 0;

    virtual auto set_builtin_types(const type& t_i64, const type& t_i32,
                                   const type& t_i16, const type& t_i8,
                                   const type& t_bool, const type& t_void)
        -> void = 0;

    virtual auto use_stream(std::ostream& new_stream) -> std::ostream& = 0;

    virtual auto println() const -> void = 0;

    virtual auto comment(const token& src_loc_tk, const size_t indent,
                         const std::string_view text) -> void = 0;

    virtual auto emit_most_efficient(const token& src_loc_tk,
                                     const size_t indent,
                                     const std::string_view without_scratch,
                                     const std::string_view with_scratch)
        -> void = 0;

    [[nodiscard]] virtual auto alloc_scratch_register(const token& src_loc_tk,
                                                      const size_t indent,
                                                      const type& type_ref)
        -> operand = 0;

    [[nodiscard]] virtual auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view reg, const type& type_ref)
        -> operand = 0;

    virtual auto free_named_register(const token& src_loc_tk,
                                     const size_t indent, const operand& reg)
        -> void = 0;

    virtual auto free_scratch_register(const token& src_loc_tk,
                                       const size_t indent, const operand& reg)
        -> void = 0;

    auto free_scratch_registers(const token& src_loc_tk, const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& reg : registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, reg);
        }
    }

    virtual auto finish() -> void = 0;

    virtual auto copy_value(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& src) -> void = 0;

    virtual auto comment_variable(const token& src_loc_tk, const size_t indent,
                                  const std::string_view text,
                                  const size_t bytes, const operand& address)
        -> void = 0;

    virtual auto comment_alias(const token& src_loc_tk, const size_t indent,
                               const std::string_view from,
                               const std::string_view to,
                               const operand& address) -> void = 0;

    virtual auto compare_and_branch(
        const token& src_loc_tk, const size_t indent, const operand& lhs,
        const operand& rhs, const comparison_action& action,
        const std::span<const operand> scratch_registers_to_free) -> void = 0;

    virtual auto branch(const size_t indent, const std::string_view target)
        -> void = 0;

    virtual auto invoke_syscall(const size_t indent) -> void = 0;

    virtual auto
    advance_array_iteration(const size_t indent, const operand& iterator,
                            const operand& counter, const size_t element_size,
                            const size_t array_size,
                            const std::string_view loop_label) -> void = 0;

    virtual auto copy(const token& src_loc_tk, const size_t indent,
                      const operand& src, const operand& dst,
                      const size_t bytes_count) -> void = 0;

    [[nodiscard]] virtual auto begin_array_copy(const token& src_loc_tk,
                                                const size_t indent)
        -> operand = 0;

    virtual auto set_array_copy_source(const size_t indent,
                                       const operand& address) -> void = 0;

    virtual auto set_array_copy_destination(const size_t indent,
                                            const operand& address) -> void = 0;

    virtual auto end_array_copy(const token& src_loc_tk, const size_t indent,
                                const size_t element_size) -> void = 0;

    virtual auto begin_memory_equal(const token& src_loc_tk,
                                    const size_t indent) -> operand = 0;

    virtual auto set_memory_equal_left(const size_t indent,
                                       const operand& address) -> void = 0;

    virtual auto set_memory_equal_right(const size_t indent,
                                        const operand& address) -> void = 0;

    virtual auto end_memory_equal(const token& src_loc_tk, const size_t indent,
                                  const size_t bytes_count, const operand& dst)
        -> void = 0;

    virtual auto end_arrays_equal(const token& src_loc_tk, const size_t indent,
                                  const size_t element_size, const operand& dst)
        -> void = 0;

    virtual auto zero(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const size_t bytes_count) -> void = 0;

    virtual auto add_subtract(const token& src_loc_tk, const size_t indent,
                              const char operation, const operand& dst,
                              const operand& src) -> void = 0;

    virtual auto bitwise(const token& src_loc_tk, const size_t indent,
                         const char operation, const operand& dst,
                         const operand& src) -> void = 0;

    virtual auto multiply(const token& src_loc_tk, const size_t indent,
                          const operand& product, const operand& factor,
                          const bool reuse_source = false) -> void = 0;

    virtual auto validate_shift_operand(const token& src_loc_tk,
                                        const operand& count) const -> void = 0;

    virtual auto shift(const token& src_loc_tk, const size_t indent,
                       const char operation, const operand& dst,
                       const operand& count) -> void = 0;

    virtual auto validate_division_operand(const token& src_loc_tk,
                                           const operand& divisor) const
        -> void = 0;

    virtual auto divide(const token& src_loc_tk, const size_t indent,
                        const char operation, const operand& dst,
                        const operand& divisor) -> void = 0;

    virtual auto store_boolean(const token& src_loc_tk, const size_t indent,
                               const operand& dst, const bool value)
        -> void = 0;

    virtual auto label(const size_t indent, const std::string_view label)
        -> void = 0;

    virtual auto address_of(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& address)
        -> void = 0;

    virtual auto unary(const size_t indent, const char operation,
                       const operand& dst) -> void = 0;

    [[nodiscard]] virtual auto can_encode_index_scale(const size_t size) const
        -> bool = 0;

    virtual auto scale_index(const token& src_loc_tk, const size_t indent,
                             const operand& index, const size_t element_size)
        -> void = 0;

    virtual auto exit_process(const token& src_loc_tk, const size_t indent,
                              const int exit_code) -> void = 0;

    [[nodiscard]] virtual auto variables_base_register() const
        -> std::string_view = 0;

    [[nodiscard]] virtual auto is_variables_base(const operand& reg) const
        -> bool = 0;

    virtual auto address_of_variable(const token& src_loc_tk,
                                     const size_t indent, const operand& dst,
                                     const int32_t offset) -> void = 0;

    virtual auto reserve_variables_base() -> void = 0;

    virtual auto release_variables_base() -> void = 0;

    virtual auto program_start() -> void = 0;

    virtual auto program_end() -> void = 0;

    virtual auto check_bounds(const token& src_loc_tk, const size_t indent,
                              const operand& reg_to_check,
                              const size_t array_size, const bool allow_end,
                              const operand& reg_size,
                              const bounds_check_options& options) -> void = 0;

    virtual auto emit_bounds_failure_handler(const bool with_line) -> void = 0;

    [[nodiscard]] virtual auto data_alignment() const -> size_t = 0;

    virtual auto begin_data(const size_t alignment) -> void = 0;

    virtual auto reserve_variables(const size_t alignment,
                                   const size_t bytes_count) -> void = 0;

    virtual auto emit_data(const size_t element_size,
                           const data_initializer& value) -> void = 0;

    virtual auto emit_string_data(const std::string_view value) -> void = 0;

    virtual auto emit_zero_data(const size_t bytes_count) const -> void = 0;

    virtual auto emit_repeated_data(const size_t element_size,
                                    const size_t count,
                                    const data_initializer& value) const
        -> void = 0;

    [[nodiscard]] virtual auto
    register_size(const std::string_view name) const -> size_t = 0;

    [[nodiscard]] virtual auto reg(const std::string_view name) const
        -> operand = 0;

    virtual auto
    emit_data_array(const size_t element_size,
                    const std::function_ref<bool(data_initializer&)> next)
        -> void = 0;

    template <typename... args_t>
    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::format_string<args_t...> format, args_t&&... args)
        -> void {

        const std::string text{
            std::format(format, std::forward<args_t>(args)...)};
        comment(src_loc_tk, indent, std::string_view{text});
    }

    template <std::ranges::input_range values_t>
    auto emit_data_array(const size_t element_size, values_t&& values) -> void {
        auto&& range{std::forward<values_t>(values)};
        auto current{std::ranges::begin(range)};
        const auto end{std::ranges::end(range)};
        auto next{[&](data_initializer& value) -> bool {
            if (current == end) {
                return false;
            }
            value = *current;
            ++current;

            return true;
        }};

        emit_data_array(element_size,
                        std::function_ref<bool(data_initializer&)>{next});
    }
};
