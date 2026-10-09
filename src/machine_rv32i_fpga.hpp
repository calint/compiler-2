#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <ostream>
#include <stdexcept>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "machine_rv32i_bare_metal.hpp"

// runs the flat image loaded at address 0 on the fpga or its emulator,
// input and output go through the memory mapped uart

class machine_rv32i_fpga final : public machine_rv32i_bare_metal {
    // the exit code of each failure, the program prints no message so the code
    // tells the failure apart
    static constexpr int bounds_exit_code_{255};
    static constexpr int frame_exit_code_{254};
    static constexpr int division_exit_code_{253};
    static constexpr int shift_exit_code_{252};
    static constexpr int overlap_exit_code_{251};
    // the uart addresses 0xffff'fff4 and 0xffff'fff8 are reached as sign
    // extended offsets from the zero register
    static constexpr int uart_in_offset_{-12};
    static constexpr int uart_out_offset_{-8};
    // uart_in reads -1 while no byte is received, uart_out while ready
    static constexpr int uart_idle_{-1};
    // 'lui' of the end of memory 0x800000
    static constexpr int memory_end_upper_{0x800};
    // 'lui' places the immediate in the upper 20 bits
    static constexpr uint32_t lui_shift_{12};
    static constexpr uint32_t memory_size_bytes_{
        uint32_t{memory_end_upper_} << lui_shift_,
    };
    // the comment prints the address as two 16 bit halves
    static constexpr uint32_t half_bits_{16};
    static constexpr uint32_t half_mask_{0xffff};

  public:
    machine_rv32i_fpga(std::ostream* const direct_output,
                       const source_files* const files, const jump_mode jumps,
                       const std::string_view binary_file_name,
                       const size_t stack_size_bytes)
        : machine_rv32i_bare_metal{direct_output, files, jumps,
                                   binary_file_name, stack_size_bytes} {}

    //
    // overridden methods
    //

    // the fpga has no console, a panic exits with the code of its failure,
    // which lights a diode, and the exit routine loops, 'a0' holds the line of
    // a bounds or division failure and is dropped
    auto emit_bounds_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        emit_panic_exit(bounds_failure_handler_label, bounds_exit_code_);
    }

    auto emit_division_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        emit_panic_exit(division_failure_handler_label, division_exit_code_);
    }

    auto emit_frame_overflow_handler() -> void override {
        emit_panic_exit(frame_overflow_handler_label, frame_exit_code_);
    }

    auto emit_overlap_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        emit_panic_exit(overlap_failure_handler_label, overlap_exit_code_);
    }

    auto emit_shift_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        emit_panic_exit(shift_failure_handler_label, shift_exit_code_);
    }

  protected:
    //
    // overridden methods
    //

    // the stack below the end of memory may not reach into the variables,
    // how much of it is used at runtime is not checked
    auto check_memory_end(const size_t memory_end_address) const
        -> void override {

        if (stack_size_bytes() <= memory_size_bytes_ and
            memory_end_address <= memory_size_bytes_ - stack_size_bytes()) {

            return;
        }

        throw std::runtime_error{std::format(
            "code, data and variables use {} B and the stack {} B, "
            "which exceeds the {} B of device memory",
            memory_end_address, stack_size_bytes(), memory_size_bytes_)};
    }

  private:
    //
    // overridden methods
    //

    // the emulator exits at 'ebreak' with a0 as its status, the loop halts
    // hardware that continues
    auto emit_exit_routine(assembler_rv32i& a) const -> void override {
        a.label(0, exit_label_);
        a.label(0, "1");
        a.ebreak(1);
        a.j(1, "1b");
    }

    auto emit_receive_byte(assembler_rv32i& a) const -> void override {
        a.label(0, "2");
        a.lw(1, "a4", uart_in_offset_, "zero");
        a.beq(1, "a4", "a3", "2b");
    }

    // the stack grows down from the end of memory
    auto emit_stack_setup(assembler_rv32i& a) const -> void override {
        // 'std::format' has no digit separators so the halves are printed
        // apart
        a.comment(0, std::format("load stack pointer to {:#x}:{:04x}",
                                 memory_size_bytes_ >> half_bits_,
                                 memory_size_bytes_ & half_mask_));

        a.lui(0, "sp", memory_end_upper_);
        a.add_separator_newline();
    }

    auto emit_transmit_byte(assembler_rv32i& a) const -> void override {
        a.label(0, "2");
        a.lw(1, "a4", uart_out_offset_, "zero");
        a.bne(1, "a4", "a3", "2b");
        a.lbu(1, "a4", 0, "a5");
        a.sw(1, "a4", uart_out_offset_, "zero");
    }

    auto emit_uart_setup(assembler_rv32i& a) const -> void override {
        a.li(1, "a3", uart_idle_);
    }

    //
    // class methods
    //

    auto emit_panic_exit(const std::string_view handler_label,
                         const int exit_code) -> void {

        label(0, handler_label);

        exit(token{}, 1,
             operand::imm(std::format("{}", exit_code), default_type()));
    }
};
