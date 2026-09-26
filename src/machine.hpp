#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
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
        // '==', '!=', '<', '<=', '>' or '>='
        std::string_view operation;

        // negate the comparison before storing or branching
        bool inverted{};

        // receives 0 or 1, empty skips storing
        operand destination;

        // branch label, empty skips branching
        std::string_view target;

        // branch on true after 'inverted', otherwise on false
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

    // read-only bytes at 'label', 'text' keeps its escapes such as '\n'
    struct string_constant {
        std::string label;
        std::string_view text;
    };

    enum class builtin_function : uint8_t { read, write, exit };

    struct builtin_function_registers {
        // argument register names in parameter order
        std::span<const std::string_view> arguments;

        // result register name, empty for 'exit'
        std::string_view result;
    };

    [[nodiscard]] virtual auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers = 0;

    [[nodiscard]] virtual auto default_type() const -> const type& = 0;

    virtual auto set_builtin_types(const type& t_i64, const type& t_i32,
                                   const type& t_i16, const type& t_i8,
                                   const type& t_bool, const type& t_void)
        -> void = 0;

    virtual auto comment(const token& src_loc_tk, const size_t indent,
                         const std::string_view text) -> void = 0;

    // emits both versions and keeps the one with less code, the first on ties
    virtual auto
    emit_most_efficient(const token& src_loc_tk, const size_t indent,
                        const std::function_ref<void()> emit_without_scratch,
                        const std::function_ref<void()> emit_with_scratch)
        -> void = 0;

    [[nodiscard]] virtual auto alloc_scratch_register(const token& src_loc_tk,
                                                      const size_t indent,
                                                      const type& type_ref)
        -> operand = 0;

    [[nodiscard]] virtual auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand = 0;

    virtual auto free_named_register(const token& src_loc_tk,
                                     const size_t indent, const operand& reg)
        -> void = 0;

    virtual auto free_scratch_register(const token& src_loc_tk,
                                       const size_t indent, const operand& reg)
        -> void = 0;

    auto free_scratch_registers(const token& src_loc_tk, const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& r : registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, r);
        }
    }

    auto free_named_registers(const token& src_loc_tk, const size_t indent,
                              const std::span<const operand> registers)
        -> void {
        for (const operand& reg : registers | std::views::reverse) {
            free_named_register(src_loc_tk, indent, reg);
        }
    }

    virtual auto finish() -> void = 0;

    // 'as_emitted' output was already written, so nothing is buffered
    virtual auto write_assembly(std::ostream& os) -> void = 0;

    [[nodiscard]] virtual auto address_size_bytes() const -> size_t = 0;

    virtual auto copy_value(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& src) -> void = 0;

    virtual auto comment_variable(const token& src_loc_tk, const size_t indent,
                                  const std::string_view text,
                                  const size_t size_bytes,
                                  const operand& address) -> void = 0;

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

    virtual auto read(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const operand& descriptor,
                      const operand& address, const operand& count) -> void = 0;

    virtual auto write(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const operand& descriptor,
                       const operand& address, const operand& count)
        -> void = 0;

    virtual auto advance_array_iteration(
        const size_t indent, const operand& iterator, const operand& counter,
        const size_t element_size_bytes, const size_t array_count,
        const std::string_view loop_label) -> void = 0;

    // 'alignment' is the alignment known for both addresses
    virtual auto copy(const token& src_loc_tk, const size_t indent,
                      const operand& src, const operand& dst,
                      const size_t size_bytes, const size_t alignment)
        -> void = 0;

    // copies 'size_bytes' of the string constant at 'label'
    virtual auto copy_from_label(const token& src_loc_tk, const size_t indent,
                                 const std::string_view label,
                                 const operand& dst, const size_t size_bytes)
        -> void = 0;

    [[nodiscard]] virtual auto begin_array_copy(const token& src_loc_tk,
                                                const size_t indent)
        -> operand = 0;

    // an empty operand keeps address preparation independent of backend setup
    [[nodiscard]] virtual auto array_copy_source_register() const -> operand {
        return {};
    }

    [[nodiscard]] virtual auto array_copy_destination_register() const
        -> operand {
        return {};
    }

    virtual auto set_array_copy_source(const size_t indent,
                                       const operand& address) -> void = 0;

    virtual auto set_array_copy_destination(const size_t indent,
                                            const operand& address) -> void = 0;

    virtual auto end_array_copy(const token& src_loc_tk, const size_t indent,
                                const size_t element_size_bytes,
                                const size_t alignment) -> void = 0;

    virtual auto begin_memory_equal(const token& src_loc_tk,
                                    const size_t indent) -> operand = 0;

    [[nodiscard]] virtual auto memory_equal_left_register() const -> operand {
        return {};
    }

    [[nodiscard]] virtual auto memory_equal_right_register() const -> operand {
        return {};
    }

    virtual auto set_memory_equal_left(const size_t indent,
                                       const operand& address) -> void = 0;

    virtual auto set_memory_equal_right(const size_t indent,
                                        const operand& address) -> void = 0;

    virtual auto end_memory_equal(const token& src_loc_tk, const size_t indent,
                                  const size_t size_bytes,
                                  const size_t alignment, const operand& dst,
                                  const bool inverted = false) -> void = 0;

    virtual auto end_arrays_equal(const token& src_loc_tk, const size_t indent,
                                  const size_t element_size_bytes,
                                  const size_t alignment, const operand& dst,
                                  const bool inverted = false) -> void = 0;

    virtual auto zero(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const size_t size_bytes,
                      const size_t alignment) -> void = 0;

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

    [[nodiscard]] virtual auto
    can_lower_index_scale(const size_t size_bytes) const -> bool = 0;

    virtual auto scale_index(const token& src_loc_tk, const size_t indent,
                             const operand& index,
                             const size_t element_size_bytes) -> void = 0;

    virtual auto exit(const token& src_loc_tk, const size_t indent,
                      const operand& exit_code) -> void = 0;

    [[nodiscard]] virtual auto variables_base_register() const
        -> std::string_view = 0;

    [[nodiscard]] virtual auto is_variables_base(const operand& reg) const
        -> bool = 0;

    virtual auto address_of_variable(const token& src_loc_tk,
                                     const size_t indent, const operand& dst,
                                     const int64_t offset,
                                     const type& value_type) -> void = 0;

    virtual auto reserve_variables_base() -> void = 0;

    virtual auto release_variables_base() -> void = 0;

    [[nodiscard]] virtual auto frame_base_register() const
        -> std::string_view = 0;

    virtual auto reserve_frame_base() -> void = 0;

    virtual auto release_frame_base() -> void = 0;

    virtual auto call_function(const size_t indent,
                               const std::string_view label,
                               const operand& frame_address) -> void = 0;

    virtual auto return_function(const size_t indent) -> void = 0;

    virtual auto define_constant(const std::string_view name,
                                 const size_t value) -> void = 0;

    virtual auto check_frame_capacity(const token& src_loc_tk,
                                      const size_t indent,
                                      const operand& frame_address,
                                      const operand& frame_size_bytes,
                                      const std::string_view failure_label,
                                      const bool enabled = {}) -> void = 0;

    virtual auto start() -> void = 0;

    virtual auto end_main() -> void = 0;

    virtual auto check_bounds(const token& src_loc_tk, const size_t indent,
                              const operand& reg_to_check,
                              const size_t array_count, const bool allow_end,
                              const operand& reg_count,
                              const bounds_check_options& options) -> void = 0;

    virtual auto emit_bounds_failure_handler(const bool with_line) -> void = 0;

    // prints 'panic: frame overflow' to stderr and exits with 255
    virtual auto emit_frame_overflow_handler() -> void = 0;

    // leaves the code section current
    virtual auto
    emit_string_constants(const std::span<const string_constant> strings)
        -> void = 0;

    [[nodiscard]] virtual auto data_alignment() const -> size_t = 0;

    virtual auto begin_data(const size_t alignment) -> void = 0;

    virtual auto reserve_variables(const size_t alignment,
                                   const size_t size_bytes) -> void = 0;

    virtual auto emit_data(const size_t element_size_bytes,
                           const data_initializer& value) -> void = 0;

    virtual auto emit_string_data(const std::string_view value) -> void = 0;

    virtual auto emit_zero_data(const size_t size_bytes) const -> void = 0;

    virtual auto emit_repeated_data(const size_t element_size_bytes,
                                    const size_t count,
                                    const data_initializer& value) const
        -> void = 0;

    [[nodiscard]] virtual auto
    make_register_operand(const std::string_view name,
                          const type& value_type) const -> operand = 0;

    virtual auto
    emit_data_array(const size_t element_size_bytes,
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
    auto emit_data_array(const size_t element_size_bytes, values_t&& values)
        -> void {
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

        emit_data_array(element_size_bytes,
                        std::function_ref<bool(data_initializer&)>{next});
    }

  protected:
    // immediates are decimal numbers prefixed by unary '-' and '~' operators
    // returns two's complement bits or empty for symbolic expressions
    [[nodiscard]] static auto immediate_bits(const operand& value)
        -> std::optional<uint64_t> {

        if (not value.is_immediate()) {
            return std::nullopt;
        }

        const std::string_view text{value.immediate()};
        const size_t digits{text.find_first_not_of("-~")};
        if (digits == std::string_view::npos) {
            return std::nullopt;
        }

        const std::string_view number{text.substr(digits)};
        const char* const end{std::to_address(number.end())};
        uint64_t bits{};
        const std::from_chars_result parsed{
            std::from_chars(std::to_address(number.begin()), end, bits)};

        if (parsed.ec != std::errc{} or parsed.ptr != end) {
            return std::nullopt;
        }

        for (const char operation :
             text.substr(0, digits) | std::views::reverse) {
            // unsigned negation wraps instead of overflowing
            if (operation == '-') {
                bits = uint64_t{} - bits;
                continue;
            }

            bits = ~bits;
        }

        return bits;
    }
};
