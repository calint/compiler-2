#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "compile_error.hpp"
#include "machine_rv32i.hpp"

// runs the flat image without an operating system, the targets supply the
// memory, the uart access, the sector routines where there is a card and how
// the program ends
class machine_rv32i_bare_metal : public machine_rv32i {
    // the routines a program calls to read and write
    static constexpr std::string_view read_label_{".Lbaz_read"};
    static constexpr std::string_view write_label_{".Lbaz_write"};
    // the result of a descriptor the uart does not serve, the negated 'EBADF'
    // that x86_64 and rv32i return for a descriptor that is not open
    static constexpr int bad_descriptor_{-9};

    // the memory of the device, the image is loaded at its start, the stack
    // grows down from its end
    size_t memory_start_address_{};
    size_t memory_size_bytes_{};

  public:
    // 'memory_size_bytes_in' is the memory of the target, e.g. fixed for the
    // qemu 'virt' machine
    machine_rv32i_bare_metal(const builtin_type_set& builtins,
                             const source_files& files,
                             const target_options& options,
                             const size_t memory_start_address_in,
                             const size_t memory_size_bytes_in)
        : machine_rv32i{builtins, files, options,
                        bare_metal_target(options, memory_start_address_in,
                                          memory_size_bytes_in)},
          memory_start_address_{memory_start_address_in},
          memory_size_bytes_{memory_size_bytes_in} {}

  protected:
    //
    // overridden methods
    //

    auto emit_io_call(const size_t indent, const target_call function)
        -> void override {

        call_io_routine(indent, function == target_call::read ? read_label_
                                                              : write_label_);
    }

    // no operating system sets up a stack, it grows down from the end of
    // memory
    auto emit_target_setup() -> void override {
        // the comment prints the address as two 16 bit halves
        constexpr size_t half_bits{16};
        constexpr size_t half_mask{0xffff};

        const size_t stack_top_address{
            memory_start_address_ + memory_size_bytes_,
        };

        // note: the start of memory and its size, of the 'virt' machine or
        //       of '--memory', are multiples of what a 'lui' loads

        assert(stack_top_address % (size_t{1} << assembler_rv32i::lui_shift) ==
               0);

        assembler_rv32i& output{assembler()};

        // 'std::format' has no digit separators so the halves are printed
        // apart
        output.comment(0, std::format("load stack pointer to {:#x}:{:04x}",
                                      stack_top_address >> half_bits,
                                      stack_top_address & half_mask));

        output.lui(0, "sp", stack_top_address >> assembler_rv32i::lui_shift);
        output.add_separator_newline();
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

    // the stack may not reach past the end of memory or into the variables,
    // how much of it is used at run time is checked only by
    // '--checks=stack', which checks no stack for 'main' itself
    auto finish_target() -> void override {
        machine_rv32i::finish_target();

        const assembler_rv32i& output{assembler()};

        const size_t memory_end_address{output.memory_end_address()};
        const size_t stack_bytes{options().stack_size_bytes};

        // note: 'bare_metal_target' rejected a stack larger than the memory
        assert(stack_bytes <= memory_size_bytes_);

        if (memory_end_address <= memory_size_bytes_ - stack_bytes) {
            return;
        }

        throw unlocated_error{std::format(
            "code, data and variables use {} B and the stack {} B, which "
            "exceeds the {} B of device memory",
            memory_end_address, stack_bytes, memory_size_bytes_)};
    }

    //
    // class methods
    //

    [[nodiscard]] auto memory_size_bytes() const -> size_t {
        return memory_size_bytes_;
    }

  private:
    //
    // virtual methods
    //

    // waits for a byte and loads it into 'a4', may change only 'a4'
    // note: the read routine uses the labels '1', '3', '4' and '5' around it,
    //       an override may use '2'
    virtual auto emit_receive_byte() -> void = 0;

    // waits until the uart is ready and sends the byte at 'a5', may change
    // only 'a4'
    // note: the write routine uses the labels '1', '3' and '5' around it, an
    //       override may use '2'
    virtual auto emit_transmit_byte() -> void = 0;

    // prepares 'a3' for 'emit_receive_byte' and 'emit_transmit_byte'
    virtual auto emit_uart_setup() -> void = 0;

    //
    // class methods
    //

    // the label '5' of a routine returns 'bad_descriptor_'
    auto emit_bad_descriptor_return() -> void {
        assembler_rv32i& output{assembler()};

        output.label(0, "5");
        output.li(1, "a0", bad_descriptor_);
        output.jr(1, "a7");
    }

    // reads from descriptor 0 until a newline or the count is reached, 'a0'
    // receives the count and ctrl-d ends the read without being stored like
    // at a terminal
    auto emit_read_routine() -> void {
        constexpr int newline{'\n'};
        constexpr int carriage_return{'\r'};
        // ctrl-d
        constexpr int end_of_transmission{0x04};

        assembler_rv32i& output{assembler()};

        output.label(0, read_label_);
        output.bnez(1, "a0", "5f");
        emit_uart_setup();
        output.mv(1, "a6", "a1");
        output.li(1, "a0", 0);
        output.label(0, "1");
        output.beq(1, "a0", "a2", "4f");
        emit_receive_byte();
        // the uart has no end of input, so ctrl-d stands for it
        output.li(1, "a5", end_of_transmission);
        output.beq(1, "a4", "a5", "4f");
        // terminals send a carriage return for enter where a tty reads newline
        output.li(1, "a5", carriage_return);
        output.bne(1, "a4", "a5", "3f");
        output.li(1, "a4", newline);
        output.label(0, "3");
        output.sb(1, "a4", 0, "a6");
        output.addi(1, "a6", "a6", 1);
        output.addi(1, "a0", "a0", 1);
        output.li(1, "a5", newline);
        output.bne(1, "a4", "a5", "1b");
        output.label(0, "4");
        output.jr(1, "a7");
        emit_bad_descriptor_return();
    }

    // writes the count of bytes to descriptor 1 or 2, 'a0' receives the
    // count
    auto emit_write_routine() -> void {
        // the uart serves the descriptors 1 and 2
        constexpr int served_count{2};

        assembler_rv32i& output{assembler()};

        output.label(0, write_label_);
        // 1 and 2 less 1 are below 'served_count' unsigned, 0 and a negative
        // descriptor are not
        output.addi(1, "a5", "a0", -1);
        output.li(1, "a4", served_count);
        output.bgeu(1, "a5", "a4", "5f");
        emit_uart_setup();
        output.mv(1, "a5", "a1");
        // the descriptor is checked so 'a0' holds the end
        output.add(1, "a0", "a1", "a2");
        // the end test sits at the bottom, one branch per byte, the guard
        // handles a count of zero
        output.beq(1, "a5", "a0", "3f");
        output.label(0, "1");
        emit_transmit_byte();
        output.addi(1, "a5", "a5", 1);
        output.bne(1, "a5", "a0", "1b");
        output.label(0, "3");
        output.mv(1, "a0", "a2");
        output.jr(1, "a7");
        emit_bad_descriptor_return();
    }

    //
    // statics
    //

    // no operating system stops a stack overflow, see
    // 'target_description::stack_limit_address'; the stack is the
    // 'stack_size_bytes' below the end of memory
    [[nodiscard]] static auto
    bare_metal_target(const target_options& options,
                      const size_t memory_start_address,
                      const size_t memory_size_bytes) -> target_description {

        const size_t stack_size_bytes{options.stack_size_bytes};

        // note: the options are checked before the source is parsed, so an
        //       error in the source does not hide them

        // the stack fits the memory and what 'main' may use without a stack
        // check, which 'finish_target' and the stack limit rely on
        if (stack_size_bytes > memory_size_bytes) {
            throw option_error{
                std::format("invalid --stack: {} B exceeds the {} B of device "
                            "memory",
                            stack_size_bytes, memory_size_bytes)};
        }

        if (stack_size_bytes < leaf_stack_bytes()) {
            throw option_error{
                std::format("invalid --stack: {} B is smaller than the {} B "
                            "that 'main' may use without a stack check",
                            stack_size_bytes, leaf_stack_bytes())};
        }

        target_description target{rv32i_target()};

        target.stack_limit_address =
            memory_start_address + memory_size_bytes - stack_size_bytes;

        return target;
    }
};
