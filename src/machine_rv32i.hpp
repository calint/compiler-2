#pragma once

#include <cstdio>
#include <print>

#include "machine.hpp"
#include "panic_exception.hpp"

class machine_rv32i final : public machine {
    [[noreturn]] static auto todo() -> void {
        std::println(stderr, "todo");
        throw panic_exception{"RV32I backend not implemented"};
    }

  public:
    using machine::comment;
    using machine::emit_data_array;

    [[nodiscard]] auto default_type() const -> const type& override { todo(); }

    auto set_builtin_types([[maybe_unused]] const type& t_i64,
                           [[maybe_unused]] const type& t_i32,
                           [[maybe_unused]] const type& t_i16,
                           [[maybe_unused]] const type& t_i8,
                           [[maybe_unused]] const type& t_bool,
                           [[maybe_unused]] const type& t_void)
        -> void override {
        todo();
    }

    auto use_stream([[maybe_unused]] std::ostream& new_stream)
        -> std::ostream& override {
        todo();
    }

    auto println() const -> void override { todo(); }

    auto comment([[maybe_unused]] const token& source_location,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const std::string_view text)
        -> void override {
        todo();
    }

    auto
    emit_most_efficient([[maybe_unused]] const token& src_loc_tk,
                        [[maybe_unused]] const size_t indent,
                        [[maybe_unused]] const std::string_view without_scratch,
                        [[maybe_unused]] const std::string_view with_scratch)
        -> void override {
        todo();
    }

    [[nodiscard]] auto
    alloc_scratch_register([[maybe_unused]] const token& src_loc_tk,
                           [[maybe_unused]] const size_t indnt,
                           [[maybe_unused]] const type& type_ref)
        -> operand override {
        todo();
    }

    [[nodiscard]] auto
    alloc_named_register([[maybe_unused]] const token& src_loc_tk,
                         [[maybe_unused]] const size_t indnt,
                         [[maybe_unused]] const std::string_view reg,
                         [[maybe_unused]] const type& type_ref)
        -> operand override {
        todo();
    }

    auto free_named_register([[maybe_unused]] const token& src_loc_tk,
                             [[maybe_unused]] const size_t indnt,
                             [[maybe_unused]] const operand& reg)
        -> void override {
        todo();
    }

    auto free_scratch_register([[maybe_unused]] const token& src_loc_tk,
                               [[maybe_unused]] const size_t indnt,
                               [[maybe_unused]] const operand& reg)
        -> void override {
        todo();
    }

    auto finish() -> void override { todo(); }

    auto copy_value([[maybe_unused]] const token& src_loc_tk,
                    [[maybe_unused]] const size_t indent,
                    [[maybe_unused]] const operand& dst,
                    [[maybe_unused]] const operand& src) -> void override {
        todo();
    }

    auto comment_variable([[maybe_unused]] const token& src_loc_tk,
                          [[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const std::string_view text,
                          [[maybe_unused]] const size_t bytes,
                          [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto comment_alias([[maybe_unused]] const token& src_loc_tk,
                       [[maybe_unused]] const size_t indent,
                       [[maybe_unused]] const std::string_view from,
                       [[maybe_unused]] const std::string_view to,
                       [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto compare_and_branch(
        [[maybe_unused]] const token& src_loc_tk,
        [[maybe_unused]] const size_t indent,
        [[maybe_unused]] const operand& lhs,
        [[maybe_unused]] const operand& rhs,
        [[maybe_unused]] const comparison_action& action,
        [[maybe_unused]] const std::span<const operand> consumed_temporaries)
        -> void override {
        todo();
    }

    auto branch([[maybe_unused]] const size_t indent,
                [[maybe_unused]] const std::string_view target)
        -> void override {
        todo();
    }

    auto invoke_syscall([[maybe_unused]] const size_t indent) -> void override {
        todo();
    }

    auto
    advance_array_iteration([[maybe_unused]] const size_t indent,
                            [[maybe_unused]] const operand& iterator,
                            [[maybe_unused]] const operand& counter,
                            [[maybe_unused]] const size_t element_size,
                            [[maybe_unused]] const size_t array_size,
                            [[maybe_unused]] const std::string_view loop_label)
        -> void override {
        todo();
    }

    auto copy([[maybe_unused]] const token& src_loc_tk,
              [[maybe_unused]] const size_t indent,
              [[maybe_unused]] const operand& src,
              [[maybe_unused]] const operand& dst,
              [[maybe_unused]] const size_t bytes_count) -> void override {
        todo();
    }

    [[nodiscard]] auto
    begin_array_copy([[maybe_unused]] const token& src_loc_tk,
                     [[maybe_unused]] const size_t indent) -> operand override {
        todo();
    }

    auto set_array_copy_source([[maybe_unused]] const size_t indent,
                               [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto set_array_copy_destination([[maybe_unused]] const size_t indent,
                                    [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto end_array_copy([[maybe_unused]] const token& src_loc_tk,
                        [[maybe_unused]] const size_t indent,
                        [[maybe_unused]] const size_t element_size)
        -> void override {
        todo();
    }

    auto begin_memory_equal([[maybe_unused]] const token& src_loc_tk,
                            [[maybe_unused]] const size_t indent)
        -> operand override {
        todo();
    }

    auto set_memory_equal_left([[maybe_unused]] const size_t indent,
                               [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto set_memory_equal_right([[maybe_unused]] const size_t indent,
                                [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto end_memory_equal([[maybe_unused]] const token& src_loc_tk,
                          [[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const size_t bytes_count,
                          [[maybe_unused]] const operand& dst)
        -> void override {
        todo();
    }

    auto end_arrays_equal([[maybe_unused]] const token& src_loc_tk,
                          [[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const size_t element_size,
                          [[maybe_unused]] const operand& dst)
        -> void override {
        todo();
    }

    auto zero([[maybe_unused]] const token& src_loc_tk,
              [[maybe_unused]] const size_t indent,
              [[maybe_unused]] const operand& dst,
              [[maybe_unused]] const size_t bytes_count) -> void override {
        todo();
    }

    auto add_subtract([[maybe_unused]] const token& src_loc_tk,
                      [[maybe_unused]] const size_t indent,
                      [[maybe_unused]] const char operation,
                      [[maybe_unused]] const operand& dst,
                      [[maybe_unused]] const operand& src) -> void override {
        todo();
    }

    auto bitwise([[maybe_unused]] const token& src_loc_tk,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const char operation,
                 [[maybe_unused]] const operand& dst,
                 [[maybe_unused]] const operand& src) -> void override {
        todo();
    }

    [[nodiscard]] auto
    needs_widened_multiply([[maybe_unused]] const operand& dst) const
        -> bool override {
        todo();
    }

    [[nodiscard]] auto
    begin_widened_multiply([[maybe_unused]] const token& src_loc_tk,
                           [[maybe_unused]] const size_t indent,
                           [[maybe_unused]] const operand& value)
        -> multiply_registers override {
        todo();
    }

    auto
    end_widened_multiply([[maybe_unused]] const token& src_loc_tk,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const operand& dst,
                         [[maybe_unused]] const multiply_registers& registers)
        -> void override {
        todo();
    }

    auto multiply([[maybe_unused]] const token& src_loc_tk,
                  [[maybe_unused]] const size_t indent,
                  [[maybe_unused]] const operand& product,
                  [[maybe_unused]] const operand& factor,
                  [[maybe_unused]] const bool reuse_source = false)
        -> void override {
        todo();
    }

    auto validate_shift_operand([[maybe_unused]] const token& src_loc_tk,
                                [[maybe_unused]] const operand& count) const
        -> void override {
        todo();
    }

    auto begin_shift([[maybe_unused]] const token& src_loc_tk,
                     [[maybe_unused]] const size_t indent,
                     [[maybe_unused]] const size_t size) -> operand override {
        todo();
    }

    auto load_shift_count([[maybe_unused]] const token& src_loc_tk,
                          [[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const operand& count,
                          [[maybe_unused]] const size_t size) -> void override {
        todo();
    }

    auto shift([[maybe_unused]] const token& src_loc_tk,
               [[maybe_unused]] const size_t indent,
               [[maybe_unused]] const char operation,
               [[maybe_unused]] const operand& dst,
               [[maybe_unused]] const operand& count) -> void override {
        todo();
    }

    auto end_shift([[maybe_unused]] const token& src_loc_tk,
                   [[maybe_unused]] const size_t indent,
                   [[maybe_unused]] const char operation,
                   [[maybe_unused]] const operand& dst) -> void override {
        todo();
    }

    auto
    validate_division_operand([[maybe_unused]] const token& src_loc_tk,
                              [[maybe_unused]] const operand& divisor) const
        -> void override {
        todo();
    }

    auto divide([[maybe_unused]] const token& src_loc_tk,
                [[maybe_unused]] const size_t indent,
                [[maybe_unused]] const char operation,
                [[maybe_unused]] const operand& dst,
                [[maybe_unused]] const operand& divisor) -> void override {
        todo();
    }

    auto store_boolean([[maybe_unused]] const token& src_loc_tk,
                       [[maybe_unused]] const size_t indent,
                       [[maybe_unused]] const operand& dst,
                       [[maybe_unused]] const bool value) -> void override {
        todo();
    }

    auto label([[maybe_unused]] const size_t indent,
               [[maybe_unused]] const std::string_view label) -> void override {
        todo();
    }

    auto address_of([[maybe_unused]] const token& src_loc_tk,
                    [[maybe_unused]] const size_t indent,
                    [[maybe_unused]] const operand& dst,
                    [[maybe_unused]] const operand& address) -> void override {
        todo();
    }

    auto unary([[maybe_unused]] const size_t indent,
               [[maybe_unused]] const char operation,
               [[maybe_unused]] const operand& dst) -> void override {
        todo();
    }

    [[nodiscard]] auto
    can_encode_index_scale([[maybe_unused]] const size_t size) const
        -> bool override {
        todo();
    }

    auto scale_index([[maybe_unused]] const token& src_loc_tk,
                     [[maybe_unused]] const size_t indent,
                     [[maybe_unused]] const operand& index,
                     [[maybe_unused]] const size_t element_size)
        -> void override {
        todo();
    }

    auto exit_process([[maybe_unused]] const token& src_loc_tk,
                      [[maybe_unused]] const size_t indent,
                      [[maybe_unused]] const int exit_code) -> void override {
        todo();
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        todo();
    }

    [[nodiscard]] auto
    is_variables_base([[maybe_unused]] const operand& reg) const
        -> bool override {
        todo();
    }

    auto address_of_variable([[maybe_unused]] const token& src_loc_tk,
                             [[maybe_unused]] const size_t indent,
                             [[maybe_unused]] const operand& dst,
                             [[maybe_unused]] const int32_t offset)
        -> void override {
        todo();
    }

    auto reserve_variables_base() -> void override { todo(); }

    auto release_variables_base() -> void override { todo(); }

    auto program_start() -> void override { todo(); }

    auto program_end() -> void override { todo(); }

    auto check_bounds([[maybe_unused]] const token& src_loc_tk,
                      [[maybe_unused]] const size_t indent,
                      [[maybe_unused]] const operand& reg_to_check,
                      [[maybe_unused]] const size_t array_size,
                      [[maybe_unused]] const bool allow_end,
                      [[maybe_unused]] const operand& reg_size,
                      [[maybe_unused]] const bounds_check_options& options)
        -> void override {
        todo();
    }

    auto emit_bounds_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {
        todo();
    }

    [[nodiscard]] auto data_alignment() const -> size_t override { todo(); }

    auto begin_data([[maybe_unused]] const size_t alignment) -> void override {
        todo();
    }

    auto reserve_variables([[maybe_unused]] const size_t alignment,
                           [[maybe_unused]] const size_t bytes_count)
        -> void override {
        todo();
    }

    auto emit_data([[maybe_unused]] const size_t element_size,
                   [[maybe_unused]] const data_initializer& value)
        -> void override {
        todo();
    }

    auto emit_string_data([[maybe_unused]] const std::string_view value)
        -> void override {
        todo();
    }

    auto emit_zero_data([[maybe_unused]] const size_t bytes_count) const
        -> void override {
        todo();
    }

    auto
    emit_repeated_data([[maybe_unused]] const size_t element_size,
                       [[maybe_unused]] const size_t count,
                       [[maybe_unused]] const data_initializer& value) const
        -> void override {
        todo();
    }

    [[nodiscard]] auto
    register_size([[maybe_unused]] const std::string_view operand) const
        -> size_t override {
        todo();
    }

    [[nodiscard]] auto reg([[maybe_unused]] const std::string_view name) const
        -> operand override {
        todo();
    }

    [[nodiscard]] auto
    sized_register([[maybe_unused]] const std::string_view reg,
                   [[maybe_unused]] const size_t size) const
        -> operand override {
        todo();
    }

    [[nodiscard]] auto sized_register([[maybe_unused]] const operand& reg,
                                      [[maybe_unused]] const size_t size) const
        -> operand override {
        todo();
    }

    auto emit_data_array(
        [[maybe_unused]] size_t element_size,
        [[maybe_unused]] std::function_ref<bool(data_initializer&)> next)
        -> void override {
        todo();
    }
};
