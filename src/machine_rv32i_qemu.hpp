#pragma once

#include <cstddef>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "machine_rv32i_bare_metal.hpp"

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
    // the 'virt' machine loads the image at the start of its ram
    static constexpr size_t ram_start_address_{0x8000'0000};
    static constexpr size_t ram_size_bytes_{0x7e0'0000};
    // note: 126 MiB, the 128 MiB of the 'virt' machine without '-m' less the
    //       2 MiB that qemu keeps after the image for its device tree, fixed,
    //       '--memory' is for 'rv32i-fpga'

  public:
    machine_rv32i_qemu(const builtin_type_set& builtins,
                       const source_files& files, const target_options& options)
        : machine_rv32i_bare_metal{builtins,           files,           options,
                                   ram_start_address_, ram_size_bytes_, false} {
    }

  protected:
    //
    // overridden methods
    //

    // ends qemu with the exit code 'a0' as its status
    auto emit_exit_routine() -> void override {
        assembler_rv32i& output{assembler()};

        output.slli(1, "a0", "a0", finisher_code_shift_);
        output.li(1, "t0", finisher_pass_);
        output.or_op(1, "a0", "a0", "t0");
        output.lui(1, "t0", test_finisher_upper_);
        output.sw(1, "a0", 0, "t0");
        // qemu shuts down after the store completes
        output.label(0, "1");
        output.j(1, "1b");
    }

  private:
    //
    // overridden methods
    //

    auto emit_receive_byte() -> void override {
        assembler_rv32i& output{assembler()};

        output.label(0, "2");
        output.lbu(1, "a4", line_status_offset_, "a3");
        output.andi(1, "a4", "a4", line_status_data_ready_);
        output.beqz(1, "a4", "2b");
        output.lbu(1, "a4", 0, "a3");
    }

    auto emit_transmit_byte() -> void override {
        assembler_rv32i& output{assembler()};

        output.label(0, "2");
        output.lbu(1, "a4", line_status_offset_, "a3");
        output.andi(1, "a4", "a4", line_status_transmit_empty_);
        output.beqz(1, "a4", "2b");
        output.lbu(1, "a4", 0, "a5");
        output.sb(1, "a4", 0, "a3");
    }

    auto emit_uart_setup() -> void override {
        assembler_rv32i& output{assembler()};

        output.lui(1, "a3", uart_upper_);
    }
};
