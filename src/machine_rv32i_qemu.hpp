#pragma once

#include <cstddef>
#include <ostream>
#include <string_view>
#include <utility>

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
    // note: 128 MiB, the memory of the 'virt' machine without '-m'
    static constexpr size_t ram_size_bytes_{0x800'0000};

  public:
    machine_rv32i_qemu(std::ostream* const direct_output,
                       const builtin_type_set& builtins,
                       const source_files* const files,
                       const target_options& options)
        : machine_rv32i_bare_metal{direct_output, builtins, files,
                                   with_ram_size(options)} {}

    //
    // overridden methods
    //

    // the machine has no sd card, so the sector functions are rejected
    [[nodiscard]] auto sector_size_bytes() const -> size_t override {
        return 0;
    }

  protected:
    //
    // overridden methods
    //

    // note: 'sector_size_bytes' is 0, so the sector functions are rejected
    auto emit_sector_routines([[maybe_unused]] const bool read_used,
                              [[maybe_unused]] const bool write_used)
        -> void override {

        std::unreachable();
    }

    // the stack follows the variables, see 'emit_stack_setup'
    auto emit_stack_limit_load(const size_t indent, const std::string_view reg)
        -> void override {

        assembler().la(indent, reg, variables_end_label);
    }

  private:
    //
    // overridden methods
    //

    // ends qemu with the exit code a0 as its status
    auto emit_exit_routine(assembler_rv32i& a) const -> void override {
        a.label(0, exit_label_);
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
        a.la(0, "sp", variables_end_label);
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

    //
    // statics
    //

    // the memory of the 'virt' machine is fixed, '--memory' is for
    // 'rv32i-fpga'
    [[nodiscard]] static auto with_ram_size(target_options options)
        -> target_options {

        options.memory_size_bytes = ram_size_bytes_;
        return options;
    }
};
