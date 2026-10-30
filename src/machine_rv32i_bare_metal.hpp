#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "compile_error.hpp"
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
    // the routines a program calls to read and write
    static constexpr std::string_view read_label_{".Lbaz_read"};
    static constexpr std::string_view write_label_{".Lbaz_write"};

    size_t stack_size_bytes_{};
    // the memory of the device, the image is loaded at its start
    size_t memory_size_bytes_{};

  protected:
    //
    // statics
    //

    // no operating system stops a stack overflow, see
    // 'target_description::is_stack_bounded'
    [[nodiscard]] static constexpr auto
    bare_metal_target(const bool has_sd_card) -> target_description {

        target_description target{rv32i_target()};
        target.is_stack_bounded = true;
        target.has_sd_card = has_sd_card;
        return target;
    }

  public:
    // 'memory_size_bytes' is the memory of the target, e.g. fixed for the
    // qemu 'virt' machine
    machine_rv32i_bare_metal(const builtin_type_set& builtins,
                             const source_files& files,
                             const target_options& options,
                             const size_t memory_size_bytes,
                             const bool has_sd_card)
        : machine_rv32i{builtins, files, options,
                        bare_metal_target(has_sd_card)},
          stack_size_bytes_{options.stack_size_bytes},
          memory_size_bytes_{memory_size_bytes} {

        // note: the options are checked before the source is parsed, so an
        //       error in the source does not hide them

        check_stack_size();
    }

  protected:
    //
    // overridden methods
    //

    // the stack may not reach past the end of memory or into the variables,
    // how much of it is used at run time is checked only by
    // '--checks=stack', which checks no stack for 'main' itself
    auto check_memory_end(const size_t memory_end_address) const
        -> void override {

        const size_t memory_bytes{memory_size_bytes()};

        // note: the constructor rejected a stack larger than the memory
        assert(stack_size_bytes() <= memory_bytes);

        if (memory_end_address <= memory_bytes - stack_size_bytes()) {
            return;
        }

        throw unlocated_error{
            std::format("code, data and variables use {} B and the stack {} B, "
                        "which exceeds the {} B of device memory",
                        memory_end_address, stack_size_bytes(), memory_bytes)};
    }

    // no operating system catches a trap, the program exits with the code of
    // the division panic
    auto emit_divide_by_zero_trap() -> void override {
        exit(token{}, 1,
             operand::imm(division_failure_.exit_code, default_type()));
    }

    auto emit_read_call(const size_t indent) -> void override {
        call_io_routine(indent, read_label_);
    }

    // only the routines the program calls are emitted
    auto emit_used_routines() -> void override {
        machine_rv32i::emit_used_routines();

        if (is_used(read_label_)) {
            emit_read_routine();
        }

        if (is_used(write_label_)) {
            emit_write_routine();
        }
    }

    auto emit_write_call(const size_t indent) -> void override {
        call_io_routine(indent, write_label_);
    }

    //
    // class methods
    //

    [[nodiscard]] auto memory_size_bytes() const -> size_t {
        return memory_size_bytes_;
    }

    [[nodiscard]] auto stack_size_bytes() const -> size_t {
        return stack_size_bytes_;
    }

  private:
    //
    // virtual methods
    //

    // waits for a byte and loads it into 'a4', may change only 'a4'
    // note: the read routine uses the labels '1', '3' and '4' around it, an
    //       override may use '2'
    virtual auto emit_receive_byte() -> void = 0;

    // waits until the uart is ready and sends the byte at 'a5', may change
    // only 'a4'
    // note: the write routine uses the labels '1' and '3' around it, an
    //       override may use '2'
    virtual auto emit_transmit_byte() -> void = 0;

    // prepares 'a3' for 'emit_receive_byte' and 'emit_transmit_byte'
    virtual auto emit_uart_setup() -> void = 0;

    //
    // class methods
    //

    // the stack fits the memory and what 'main' may use without a stack
    // check, which 'check_memory_end' and the stack limit rely on
    auto check_stack_size() const -> void {
        if (stack_size_bytes() > memory_size_bytes()) {
            throw option_error{
                std::format("invalid --stack: {} B exceeds the {} B of device "
                            "memory",
                            stack_size_bytes(), memory_size_bytes())};
        }

        if (stack_size_bytes() < leaf_stack_bytes()) {
            throw option_error{
                std::format("invalid --stack: {} B is smaller than the {} B "
                            "that 'main' may use without a stack check",
                            stack_size_bytes(), leaf_stack_bytes())};
        }
    }

    // reads until a newline or the count is reached, 'a0' receives the count
    // and ctrl-d ends the read without being stored like at a terminal
    auto emit_read_routine() -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, read_label_);
        emit_uart_setup();
        a.mv(1, "a6", "a1");
        a.li(1, "a0", 0);
        a.label(0, "1");
        a.beq(1, "a0", "a2", "4f");
        emit_receive_byte();
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

    // writes the count of bytes, 'a0' receives the count
    auto emit_write_routine() -> void {
        assembler_rv32i& a{assembler()};

        a.label(0, write_label_);
        emit_uart_setup();
        a.mv(1, "a5", "a1");
        // the descriptor is not needed so 'a0' holds the end
        a.add(1, "a0", "a1", "a2");
        // the end test sits at the bottom, one branch per byte, the guard
        // handles a count of zero
        a.beq(1, "a5", "a0", "3f");
        a.label(0, "1");
        emit_transmit_byte();
        a.addi(1, "a5", "a5", 1);
        a.bne(1, "a5", "a0", "1b");
        a.label(0, "3");
        a.mv(1, "a0", "a2");
        a.jr(1, "a7");
    }
};
