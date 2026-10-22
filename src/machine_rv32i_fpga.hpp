#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ostream>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "compile_error.hpp"
#include "machine_rv32i_bare_metal.hpp"
#include "operand.hpp"
#include "token.hpp"

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
    static constexpr int stack_exit_code_{250};
    static constexpr int overflow_exit_code_{249};
    // the uart addresses 0xffff'fff4 and 0xffff'fff8 are reached as sign
    // extended offsets from the zero register
    static constexpr int uart_in_offset_{-12};
    static constexpr int uart_out_offset_{-8};
    // uart_in reads -1 while no byte is received, uart_out while ready
    static constexpr int uart_idle_{-1};
    // the sd card addresses 0xffff'fff0, 0xffff'ffec, 0xffff'ffe8 and
    // 0xffff'ffe0 as offsets from the zero register like the uart
    static constexpr int sd_busy_offset_{-16};
    static constexpr int sd_read_sector_offset_{-20};
    static constexpr int sd_next_byte_offset_{-24};
    static constexpr int sd_write_sector_offset_{-32};
    // the uart and the sd card are in the last 'lui' step of the address
    // space, the memory ends below it
    static constexpr size_t max_memory_size_bytes_{0xffff'f000};
    // the comment prints the address as two 16 bit halves
    static constexpr uint32_t half_bits_{16};
    static constexpr uint32_t half_mask_{0xffff};

  public:
    machine_rv32i_fpga(const builtin_type_set& builtins,
                       const source_files* const files,
                       const target_options& options)
        : machine_rv32i_bare_metal{builtins, files, options} {

        // note: 'main' accepts only a multiple of what a 'lui' loads, the end
        //       of memory

        assert(memory_size_bytes() %
                   (size_t{1} << assembler_rv32i::lui_shift) ==
               0);

        if (memory_size_bytes() > max_memory_size_bytes_) {
            throw option_error{
                std::format("invalid --memory: {} B exceeds the {} B below "
                            "the device addresses",
                            memory_size_bytes(), max_memory_size_bytes_)};
        }
    }

    //
    // overridden methods
    //

    // a panic prints nothing and exits with the code of its failure, which
    // lights a diode, and the exit routine loops; no line is reported, see
    // 'reports_line'
    auto emit_bounds_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        assert(not with_line);

        emit_panic_exit(bounds_failure_handler_label, bounds_exit_code_);
    }

    auto emit_division_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        assert(not with_line);

        emit_panic_exit(division_failure_handler_label, division_exit_code_);
    }

    auto emit_frame_overflow_handler() -> void override {
        emit_panic_exit(frame_overflow_handler_label, frame_exit_code_);
    }

    auto emit_overflow_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        assert(not with_line);

        emit_panic_exit(overflow_failure_handler_label, overflow_exit_code_);
    }

    auto emit_overlap_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        assert(not with_line);

        emit_panic_exit(overlap_failure_handler_label, overlap_exit_code_);
    }

    auto emit_shift_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {

        assert(not with_line);

        emit_panic_exit(shift_failure_handler_label, shift_exit_code_);
    }

    auto emit_stack_overflow_handler() -> void override {
        emit_panic_exit(stack_overflow_handler_label, stack_exit_code_);
    }

    [[nodiscard]] auto reports_line() const -> bool override { return false; }

  protected:
    //
    // overridden methods
    //

    // the card has a buffer of one sector: a read fills it from the sector,
    // a write sends it to the sector, the bytes pass one at a time through
    // the next byte address; its index wraps, so every transfer moves the
    // whole sector; a sector past the end of the card is not detected, the
    // emulator stops at it
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void override {

        assembler_rv32i& a{assembler()};

        if (read_used) {
            a.label(0, sector_read_label_);
            emit_wait_while_busy(a, "1");
            a.sw(1, "a0", sd_read_sector_offset_, "zero");
            emit_wait_while_busy(a, "2");
            a.mv(1, "a5", "a1");
            a.add(1, "a6", "a1", "a2");
            a.label(0, "3");
            a.lw(1, "a4", sd_next_byte_offset_, "zero");
            a.sb(1, "a4", 0, "a5");
            a.addi(1, "a5", "a5", 1);
            a.bne(1, "a5", "a6", "3b");
            a.jr(1, "a7");
        }

        if (write_used) {
            a.label(0, sector_write_label_);
            emit_wait_while_busy(a, "1");
            a.mv(1, "a5", "a1");
            a.add(1, "a6", "a1", "a2");
            a.label(0, "2");
            a.lbu(1, "a4", 0, "a5");
            a.sw(1, "a4", sd_next_byte_offset_, "zero");
            a.addi(1, "a5", "a5", 1);
            a.bne(1, "a5", "a6", "2b");
            a.sw(1, "a0", sd_write_sector_offset_, "zero");
            emit_wait_while_busy(a, "3");
            a.jr(1, "a7");
        }
    }

    // the stack is the 'stack_size_bytes' below the end of memory
    auto emit_stack_limit_load(const size_t indent, const std::string_view reg)
        -> void override {

        // note: the constructor rejected a stack larger than the memory
        assert(stack_size_bytes() <= memory_size_bytes());

        assembler_rv32i& a{assembler()};

        a.li(indent, reg, memory_size_bytes() - stack_size_bytes());
    }

  private:
    //
    // overridden methods
    //

    // the emulator exits at 'ebreak' with a0 as its status, the loop halts
    // hardware that continues
    auto emit_exit_routine() -> void override {
        assembler_rv32i& a{assembler()};

        a.label(0, exit_label_);
        a.label(0, "1");
        a.ebreak(1);
        a.j(1, "1b");
    }

    auto emit_receive_byte() -> void override {
        assembler_rv32i& a{assembler()};

        a.label(0, "2");
        a.lw(1, "a4", uart_in_offset_, "zero");
        a.beq(1, "a4", "a3", "2b");
    }

    // the stack grows down from the end of memory
    auto emit_stack_setup() -> void override {
        assembler_rv32i& a{assembler()};

        // 'std::format' has no digit separators so the halves are printed
        // apart
        a.comment(0, std::format("load stack pointer to {:#x}:{:04x}",
                                 memory_size_bytes() >> half_bits_,
                                 memory_size_bytes() & half_mask_));

        a.lui(0, "sp", memory_size_bytes() >> assembler_rv32i::lui_shift);
        a.add_separator_newline();
    }

    auto emit_transmit_byte() -> void override {
        assembler_rv32i& a{assembler()};

        a.label(0, "2");
        a.lw(1, "a4", uart_out_offset_, "zero");
        a.bne(1, "a4", "a3", "2b");
        a.lbu(1, "a4", 0, "a5");
        a.sw(1, "a4", uart_out_offset_, "zero");
    }

    auto emit_uart_setup() -> void override {
        assembler_rv32i& a{assembler()};

        a.li(1, "a3", uart_idle_);
    }

    //
    // class methods
    //

    auto emit_panic_exit(const std::string_view handler_label,
                         const int exit_code) -> void {

        label(0, handler_label);

        exit(token{}, 1, operand::imm(exit_code, default_type()));
    }

    //
    // statics
    //

    // the card is busy while it starts up and while it transfers a sector,
    // 'label_name' is a numeric label of the routine
    static auto emit_wait_while_busy(assembler_rv32i& a,
                                     const std::string_view label_name)
        -> void {

        a.label(0, label_name);
        a.lw(1, "a4", sd_busy_offset_, "zero");
        a.bnez(1, "a4", std::format("{}b", label_name));
    }
};
