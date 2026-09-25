#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <ranges>
#include <span>
#include <string_view>

#include "almost_assembler_rv32i.hpp"
#include "decouple.hpp"
#include "machine_rv32i.hpp"
#include "panic_exception.hpp"

// runs the flat image without an operating system on the qemu 'virt' machine,
// input and output go through its ns16550a uart and exit ends qemu with the
// exit code as its status

class machine_rv32i_qemu final : public machine_rv32i {
    // 'lui' of the uart address 0x10000000
    static constexpr int uart_upper_{0x10000};
    static constexpr int line_status_offset_{5};
    static constexpr int line_status_data_ready_{0x01};
    static constexpr int line_status_transmit_empty_{0x20};
    // 'lui' of the test finisher address 0x100000
    static constexpr int test_finisher_upper_{0x100};
    // the finisher takes the exit code in the upper 16 bits
    static constexpr int finisher_code_shift_{16};
    static constexpr int finisher_pass_{0x5555};
    static constexpr int newline_{'\n'};
    static constexpr int carriage_return_{'\r'};
    // ctrl-d
    static constexpr int end_of_transmission_{0x04};
    static constexpr size_t word_size_bytes_{4};
    // the save area keeps sp 16-byte aligned
    static constexpr int save_size_bytes_{16};

    size_t stack_size_bytes_{};
    bool read_used_{};
    bool write_used_{};
    bool exit_used_{};

  public:
    machine_rv32i_qemu(std::ostream& os_ref, const std::string_view source,
                       const jump_mode jumps,
                       const std::string_view binary_file_name,
                       const size_t stack_size_bytes)
        : machine_rv32i{os_ref, source, jumps, binary_file_name},
          stack_size_bytes_{stack_size_bytes} {

        if (stack_size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw panic_exception{"stack size exceeds RV32I address range"};
        }
    }

    auto start() -> void override {
        read_used_ = false;
        write_used_ = false;
        exit_used_ = false;

        machine_rv32i::start();

        // no operating system sets up a stack, so it follows the variables
        almost_assembler_rv32i& a{assembler()};

        a.la(0, "sp", "vars.end");
        a.li(0, "t0", stack_size_bytes_);
        a.add(0, "sp", "sp", "t0");
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);

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
        assembler().call(indent, ".Lbaz_read", "a7");
        read_used_ = true;
    }

    auto emit_write_call(const size_t indent) -> void override {
        assembler().call(indent, ".Lbaz_write", "a7");
        write_used_ = true;
    }

  private:
    // registers other than the result keep their values like in a system call
    auto save_registers(const std::span<const std::string_view> names) const
        -> void {

        almost_assembler_rv32i& a{assembler()};

        a.addi(1, "sp", "sp", -save_size_bytes_);
        for (const auto [index, name] : std::views::enumerate(names)) {
            a.sw(1, name, static_cast<size_t>(index) * word_size_bytes_, "sp");
        }
    }

    auto restore_registers_and_return(
        const std::span<const std::string_view> names) const -> void {

        almost_assembler_rv32i& a{assembler()};

        for (const auto [index, name] : std::views::enumerate(names)) {
            a.lw(1, name, static_cast<size_t>(index) * word_size_bytes_, "sp");
        }
        a.addi(1, "sp", "sp", save_size_bytes_);
        a.jr(1, "a7");
    }

    // reads until a newline or the count is reached, a0 receives the count
    // and ctrl-d ends the read without being stored like at a terminal
    auto emit_read_routine() const -> void {
        static constexpr std::array<std::string_view, 4> saved{"t0", "t1", "t2",
                                                               "a1"};

        almost_assembler_rv32i& a{assembler()};

        a.label(0, ".Lbaz_read");
        save_registers(saved);
        a.lui(1, "t0", uart_upper_);
        a.li(1, "a0", 0);
        a.label(0, "1");
        a.beq(1, "a0", "a2", "4f");
        a.label(0, "2");
        a.lbu(1, "t1", line_status_offset_, "t0");
        a.andi(1, "t1", "t1", line_status_data_ready_);
        a.beqz(1, "t1", "2b");
        a.lbu(1, "t1", 0, "t0");
        // the uart has no end of input, so ctrl-d stands for it
        a.li(1, "t2", end_of_transmission_);
        a.beq(1, "t1", "t2", "4f");
        // terminals send a carriage return for enter where a tty reads newline
        a.li(1, "t2", carriage_return_);
        a.bne(1, "t1", "t2", "3f");
        a.li(1, "t1", newline_);
        a.label(0, "3");
        a.sb(1, "t1", 0, "a1");
        a.addi(1, "a1", "a1", 1);
        a.addi(1, "a0", "a0", 1);
        a.li(1, "t2", newline_);
        a.bne(1, "t1", "t2", "1b");
        a.label(0, "4");
        restore_registers_and_return(saved);
    }

    // writes the count of bytes, a0 receives the count
    auto emit_write_routine() const -> void {
        static constexpr std::array<std::string_view, 3> saved{"t0", "t1",
                                                               "a1"};

        almost_assembler_rv32i& a{assembler()};

        a.label(0, ".Lbaz_write");
        save_registers(saved);
        a.lui(1, "t0", uart_upper_);
        // the descriptor is not needed so a0 holds the end
        a.add(1, "a0", "a1", "a2");
        a.label(0, "1");
        a.beq(1, "a1", "a0", "3f");
        a.label(0, "2");
        a.lbu(1, "t1", line_status_offset_, "t0");
        a.andi(1, "t1", "t1", line_status_transmit_empty_);
        a.beqz(1, "t1", "2b");
        a.lbu(1, "t1", 0, "a1");
        a.sb(1, "t1", 0, "t0");
        a.addi(1, "a1", "a1", 1);
        a.j(1, "1b");
        a.label(0, "3");
        a.mv(1, "a0", "a2");
        restore_registers_and_return(saved);
    }

    // ends qemu with the exit code a0 as its status
    auto emit_exit_routine() const -> void {
        almost_assembler_rv32i& a{assembler()};

        a.label(0, ".Lbaz_exit");
        a.slli(1, "a0", "a0", finisher_code_shift_);
        a.li(1, "t0", finisher_pass_);
        a.or_op(1, "a0", "a0", "t0");
        a.lui(1, "t0", test_finisher_upper_);
        a.sw(1, "a0", 0, "t0");
        // qemu shuts down after the store completes
        a.label(0, "1");
        a.j(1, "1b");
    }
};
