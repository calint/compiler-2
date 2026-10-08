#pragma once

#include <array>
#include <cstddef>
#include <ostream>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "machine_rv32i.hpp"
#include "operand.hpp"
#include "token.hpp"

// runs the flat image without an operating system, the targets supply the
// stack setup, the uart access and how the program ends

class machine_rv32i_bare_metal : public machine_rv32i {
    static constexpr int newline_{'\n'};
    static constexpr int carriage_return_{'\r'};
    // ctrl-d
    static constexpr int end_of_transmission_{0x04};
    // late in the allocation order so the call rarely has to save them
    static constexpr std::array<std::string_view, 4> read_clobbered_{
        "a3",
        "a4",
        "a5",
        "a6",
    };
    static constexpr std::array<std::string_view, 3> write_clobbered_{
        "a3",
        "a4",
        "a5",
    };

    size_t stack_size_bytes_{};
    bool read_used_{};
    bool write_used_{};
    bool exit_used_{};

  protected:
    // the routines a program calls, the target defines the exit one
    static constexpr std::string_view exit_label_{".Lbaz_exit"};
    static constexpr std::string_view read_label_{".Lbaz_read"};
    static constexpr std::string_view write_label_{".Lbaz_write"};

  public:
    machine_rv32i_bare_metal(std::ostream& os_ref,
                             const source_files* const files,
                             const jump_mode jumps,
                             const std::string_view binary_file_name,
                             const size_t stack_size_bytes)
        : machine_rv32i{os_ref, files, jumps, binary_file_name},
          stack_size_bytes_{stack_size_bytes} {}

    //
    // overridden methods
    //

    // only the routines the program calls are emitted
    auto begin_data(const size_t alignment) -> void override {
        if (exit_used_) {
            emit_exit_routine(assembler());
        }

        if (read_used_) {
            emit_read_routine();
        }

        if (write_used_) {
            emit_write_routine();
        }

        machine_rv32i::begin_data(alignment);
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);

        branch(indent, exit_label_);
        exit_used_ = true;
    }

    auto start() -> void override {
        read_used_ = false;
        write_used_ = false;
        exit_used_ = false;

        machine_rv32i::start();

        // no operating system sets up a stack
        emit_stack_setup(assembler());
    }

  protected:
    //
    // overridden methods
    //

    // the routines return through a7 so the call costs no more registers
    // than the system call
    auto emit_read_call(const size_t indent) -> void override {
        call_io_routine(indent, read_label_, read_clobbered_);
        read_used_ = true;
    }

    auto emit_write_call(const size_t indent) -> void override {
        call_io_routine(indent, write_label_, write_clobbered_);
        write_used_ = true;
    }

    //
    // class methods
    //

    [[nodiscard]] auto stack_size_bytes() const -> size_t {
        return stack_size_bytes_;
    }

  private:
    //
    // virtual methods
    //

    // '.Lbaz_exit' ends the program with the exit code in a0
    virtual auto emit_exit_routine(assembler_rv32i& a) const -> void = 0;

    // waits for a byte and loads it into a4, may change only a4
    virtual auto emit_receive_byte(assembler_rv32i& a) const -> void = 0;

    virtual auto emit_stack_setup(assembler_rv32i& a) const -> void = 0;

    // waits until the uart is ready and sends the byte at a5, may change
    // only a4
    virtual auto emit_transmit_byte(assembler_rv32i& a) const -> void = 0;

    // prepares a3 for 'emit_receive_byte' and 'emit_transmit_byte'
    virtual auto emit_uart_setup(assembler_rv32i& a) const -> void = 0;

    //
    // class methods
    //

    // reads until a newline or the count is reached, a0 receives the count
    // and ctrl-d ends the read without being stored like at a terminal
    auto emit_read_routine() -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, read_label_);
        emit_uart_setup(a);
        a.mv(1, "a6", "a1");
        a.li(1, "a0", 0);
        a.label(0, "1");
        a.beq(1, "a0", "a2", "4f");
        emit_receive_byte(a);
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
    auto emit_write_routine() -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, write_label_);
        emit_uart_setup(a);
        a.mv(1, "a5", "a1");
        // the descriptor is not needed so a0 holds the end
        a.add(1, "a0", "a1", "a2");
        // the end test sits at the bottom, one branch per byte, the guard
        // handles a count of zero
        a.beq(1, "a5", "a0", "3f");
        a.label(0, "1");
        emit_transmit_byte(a);
        a.addi(1, "a5", "a5", 1);
        a.bne(1, "a5", "a0", "1b");
        a.label(0, "3");
        a.mv(1, "a0", "a2");
        a.jr(1, "a7");
    }
};
