#pragma once

#include <array>
#include <cstddef>
#include <ostream>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "decouple.hpp"
#include "machine_rv32i.hpp"

// runs the flat image loaded at address 0 on the fpga or its emulator,
// input and output go through the memory mapped uart

class machine_rv32i_fpga final : public machine_rv32i {
    // the uart addresses 0xffff'fff4 and 0xffff'fff8 are reached as sign
    // extended offsets from the zero register
    static constexpr int uart_in_offset_{-12};
    static constexpr int uart_out_offset_{-8};
    // uart_in reads -1 while no byte is received, uart_out while ready
    static constexpr int uart_idle_{-1};
    // 'lui' of the end of memory 0x800000
    static constexpr int memory_end_upper_{0x800};
    static constexpr int newline_{'\n'};
    static constexpr int carriage_return_{'\r'};
    // ctrl-d
    static constexpr int end_of_transmission_{0x04};
    // late in the allocation order so the call rarely has to save them
    static constexpr std::array<std::string_view, 4> read_clobbered_{
        "a3", "a4", "a5", "a6"};
    static constexpr std::array<std::string_view, 3> write_clobbered_{
        "a3", "a4", "a5"};

    bool read_used_{};
    bool write_used_{};
    bool exit_used_{};

  public:
    machine_rv32i_fpga(std::ostream& os_ref, const std::string_view source,
                       const jump_mode jumps,
                       const std::string_view binary_file_name)
        : machine_rv32i{os_ref, source, jumps, binary_file_name} {}

    auto start() -> void override {
        read_used_ = false;
        write_used_ = false;
        exit_used_ = false;

        machine_rv32i::start();

        // no operating system sets up a stack, so it grows down from the end
        // of memory
        assembler().lui(0, "sp", memory_end_upper_);
    }

    // todo: there is no way to end the program yet, so exit halts
    auto exit([[maybe_unused]] const token& src_loc_tk, const size_t indent,
              [[maybe_unused]] const operand& exit_code) -> void override {

        branch(indent, ".Lbaz_exit");
        exit_used_ = true;
    }

    auto begin_data(const size_t alignment) -> void override {
        if (exit_used_) {
            emit_exit_routine();
        }

        if (read_used_) {
            emit_read_routine();
        }

        if (write_used_) {
            emit_write_routine();
        }

        machine_rv32i::begin_data(alignment);
    }

  protected:
    // the routines return through a7 so the call costs no more registers
    // than the system call
    auto emit_read_call(const size_t indent) -> void override {
        call_io_routine(indent, ".Lbaz_read", read_clobbered_);
        read_used_ = true;
    }

    auto emit_write_call(const size_t indent) -> void override {
        call_io_routine(indent, ".Lbaz_write", write_clobbered_);
        write_used_ = true;
    }

  private:
    // reads until a newline or the count is reached, a0 receives the count
    // and ctrl-d ends the read without being stored like at a terminal
    auto emit_read_routine() const -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, ".Lbaz_read");
        a.li(1, "a3", uart_idle_);
        a.mv(1, "a6", "a1");
        a.li(1, "a0", 0);
        a.label(0, "1");
        a.beq(1, "a0", "a2", "4f");
        a.label(0, "2");
        a.lw(1, "a4", uart_in_offset_, "zero");
        a.beq(1, "a4", "a3", "2b");
        // the uart has no end of input, so ctrl-d stands for it
        a.li(1, "a5", end_of_transmission_);
        a.beq(1, "a4", "a5", "4f");
        // terminals send a carriage return for enter where a tty reads newline
        a.li(1, "a5", carriage_return_);
        a.bne(1, "a4", "a5", "3f");
        a.li(1, "a4", newline_);
        a.label(0, "3");
        a.sb(1, "a4", 0, "a6");
        a.addi(1, "a6", "a6", 1);
        a.addi(1, "a0", "a0", 1);
        a.li(1, "a5", newline_);
        a.bne(1, "a4", "a5", "1b");
        a.label(0, "4");
        a.jr(1, "a7");
    }

    // writes the count of bytes, a0 receives the count
    auto emit_write_routine() const -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, ".Lbaz_write");
        a.li(1, "a3", uart_idle_);
        a.mv(1, "a5", "a1");
        // the descriptor is not needed so a0 holds the end
        a.add(1, "a0", "a1", "a2");
        a.label(0, "1");
        a.beq(1, "a5", "a0", "3f");
        a.label(0, "2");
        a.lw(1, "a4", uart_out_offset_, "zero");
        a.bne(1, "a4", "a3", "2b");
        a.lbu(1, "a4", 0, "a5");
        a.sw(1, "a4", uart_out_offset_, "zero");
        a.addi(1, "a5", "a5", 1);
        a.j(1, "1b");
        a.label(0, "3");
        a.mv(1, "a0", "a2");
        a.jr(1, "a7");
    }

    auto emit_exit_routine() const -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, ".Lbaz_exit");
        a.j(1, ".Lbaz_exit");
    }
};
