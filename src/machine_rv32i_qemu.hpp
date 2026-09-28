#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "decouple.hpp"
#include "machine_rv32i_bare_metal.hpp"
#include "panic_exception.hpp"

// runs the flat image without an operating system on the qemu 'virt' machine,
// input and output go through its ns16550a uart and exit ends qemu with the
// exit code as its status

class machine_rv32i_qemu final : public machine_rv32i_bare_metal {
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

  public:
    machine_rv32i_qemu(std::ostream& os_ref, const std::string_view source,
                       const jump_mode jumps,
                       const std::string_view binary_file_name,
                       const size_t stack_size_bytes)
        : machine_rv32i_bare_metal{os_ref, source, jumps, binary_file_name,
                                   stack_size_bytes} {

        if (stack_size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw panic_exception{"stack size exceeds RV32I address range"};
        }
    }

  private:
    //
    // overridden methods
    //

    // ends qemu with the exit code a0 as its status
    auto emit_exit_routine(assembler_rv32i& a) const -> void override {
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

    auto emit_receive_byte(assembler_rv32i& a) const -> void override {
        a.label(0, "2");
        a.lbu(1, "a4", line_status_offset_, "a3");
        a.andi(1, "a4", "a4", line_status_data_ready_);
        a.beqz(1, "a4", "2b");
        a.lbu(1, "a4", 0, "a3");
    }

    // the stack follows the variables
    auto emit_stack_setup(assembler_rv32i& a) const -> void override {
        a.la(0, "sp", "vars.end");
        a.li(0, "t0", stack_size_bytes());
        a.add(0, "sp", "sp", "t0");
    }

    auto emit_transmit_byte(assembler_rv32i& a) const -> void override {
        a.label(0, "2");
        a.lbu(1, "a4", line_status_offset_, "a3");
        a.andi(1, "a4", "a4", line_status_transmit_empty_);
        a.beqz(1, "a4", "2b");
        a.lbu(1, "a4", 0, "a5");
        a.sb(1, "a4", 0, "a3");
    }

    auto emit_uart_setup(assembler_rv32i& a) const -> void override {
        a.lui(1, "a3", uart_upper_);
    }
};
