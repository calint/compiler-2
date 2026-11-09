#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "machine_rv32i.hpp"

// the rv32i target on linux in qemu user mode: system calls read, write, exit
// and transfer the sectors of the sd card image, the operating system stops a
// program that overflows the stack
class machine_rv32i_linux final : public machine_rv32i {
    // the numbers of the generic linux table, 'asm-generic/unistd.h'
    static constexpr int syscall_read_{63};
    static constexpr int syscall_write_{64};
    static constexpr int syscall_exit_{93};
    static constexpr int syscall_openat_{56};
    static constexpr int syscall_llseek_{62};
    static constexpr int syscall_pread_{67};
    static constexpr int syscall_pwrite_{68};
    // the part the system call sector routines share, and their data
    static constexpr std::string_view sector_transfer_label_{
        ".Lbaz_sector_transfer",
    };
    static constexpr std::string_view sd_card_label_{".Lbaz_sd_card"};
    static constexpr std::string_view sd_card_name_label_{".Lbaz_sd_card_name"};

  public:
    using machine_rv32i::machine_rv32i;

  protected:
    //
    // overridden methods
    //

    // the system call ends the program with the exit code in 'a0'
    auto emit_exit_routine() -> void override {
        assembler_rv32i& output{assembler()};

        output.li(1, "a7", syscall_exit_);
        output.ecall(1);
    }

    auto emit_io_call(const size_t indent, const target_call function)
        -> void override {

        assembler_rv32i& output{assembler()};

        output.li(indent, "a7",
                  function == target_call::read ? syscall_read_
                                                : syscall_write_);

        output.ecall(indent);
    }

    // the registers as 'sector_read_label_' says; an operating system
    // transfers the sectors of the image file: the image file is opened at
    // the first transfer, its descriptor and last sector kept in
    // '.Lbaz_sd_card', a sector past the end, a failed open or a short
    // transfer panics
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void override {

        // 'AT_FDCWD', 'O_RDWR' and 'SEEK_END' of linux
        constexpr int current_directory{-100};
        constexpr int read_write{2};
        constexpr int seek_end{2};
        // the 64 bit size of the file, the descriptor, -1 until the file is
        // opened, and the last sector
        constexpr std::array<int64_t, 4> sd_card_words{0, 0, -1, 0};
        constexpr int64_t size_high_offset{4};
        constexpr int64_t descriptor_offset{8};
        constexpr int64_t last_offset{12};
        // the arguments are kept while the file is opened, 'a5' with the
        // system call is not written there and an 'ecall' changes only 'a0'
        constexpr std::array<std::string_view, 3> saved{"a0", "a1", "a2"};
        // the file name ends with a zero byte for the system call
        constexpr std::array<int64_t, 1> name_end{0};
        // the byte offset of a sector is split into two words
        constexpr int sector_shift{std::countr_zero(card_sector_size_bytes_)};

        constexpr int high_shift{
            static_cast<int>(register_bits_) - sector_shift,
        };

        assembler_rv32i& output{assembler()};

        if (read_used) {
            output.label(0, sector_read_label_);
            output.li(1, "a5", syscall_pread_);

            // note: without the write entry the transfer follows at once

            if (write_used) {
                output.j(1, sector_transfer_label_);
            }
        }

        if (write_used) {
            output.label(0, sector_write_label_);
            output.li(1, "a5", syscall_pwrite_);
        }

        // 'a5' holds the system call, 'a6' the return address
        output.label(0, sector_transfer_label_);
        output.mv(1, "a6", "a7");
        output.la(1, "a3", sd_card_label_);
        output.lw(1, "a4", descriptor_offset, "a3");
        output.bgez(1, "a4", "2f");

        save_call_registers(1, saved);

        output.li(1, "a0", current_directory);
        output.la(1, "a1", sd_card_name_label_);
        output.li(1, "a2", read_write);

        // note: without 'O_CREAT' linux ignores the mode in 'a3', which keeps
        //       the address of the card data, and a system call changes only
        //       'a0'

        output.li(1, "a7", syscall_openat_);
        output.ecall(1);
        output.bltz(1, "a0", sd_card_failure_.label);
        output.sw(1, "a0", descriptor_offset, "a3");
        // 'llseek' writes the size of the file at 'a3'
        output.li(1, "a1", 0);
        output.li(1, "a2", 0);
        output.li(1, "a4", seek_end);
        output.li(1, "a7", syscall_llseek_);
        output.ecall(1);
        output.bnez(1, "a0", sd_card_failure_.label);
        output.lw(1, "a4", 0, "a3");
        output.lw(1, "a0", size_high_offset, "a3");
        output.srli(1, "a4", "a4", sector_shift);
        output.slli(1, "a1", "a0", high_shift);
        output.or_op(1, "a4", "a4", "a1");
        // the sector count is 'a4' and 'a0' above 32 bits, an image without a
        // whole sector holds none to transfer
        output.srli(1, "a0", "a0", sector_shift);
        output.or_op(1, "a1", "a4", "a0");
        output.beqz(1, "a1", sd_card_failure_.label);
        // the last sector, of an image of 2 TiB or more the last that a
        // sector number reaches, 2^32 - 1; a count would not fit 32 bits
        output.beqz(1, "a0", "1f");
        output.li(1, "a4", 0);
        output.label(0, "1");
        output.addi(1, "a4", "a4", -1);
        output.sw(1, "a4", last_offset, "a3");
        restore_call_registers(1, saved);
        output.label(0, "2");
        output.lw(1, "a4", last_offset, "a3");
        output.bltu(1, "a4", "a0", sd_card_failure_.label);
        // 'a0' the descriptor, 'a3' and 'a4' the low and high word of the
        // offset
        output.lw(1, "a7", descriptor_offset, "a3");
        output.slli(1, "a3", "a0", sector_shift);
        output.srli(1, "a4", "a0", high_shift);
        output.mv(1, "a0", "a7");
        output.mv(1, "a7", "a5");
        output.ecall(1);
        output.bne(1, "a0", "a2", sd_card_failure_.label);
        output.jr(1, "a6");

        emit_failure_handler(sd_card_failure_);

        output.switch_section(section::data);
        output.align(word_size_bytes_);
        output.label(0, sd_card_label_);
        output.data(word_size_bytes_, sd_card_words);
        output.switch_section(section::rodata);
        output.label(0, sd_card_name_label_);
        output.string_data(options().sd_card_file_name);
        output.data(1, name_end);
        output.switch_section(section::text);
    }
};
