#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "assembler_rv32i.hpp"
#include "machine_rv32i.hpp"
#include "operand.hpp"
#include "token.hpp"

// the rv32i target on linux in qemu user mode: system calls read, write, exit
// and transfer the sectors of the image file, the operating system stops a
// program that overflows the stack
class machine_rv32i_linux final : public machine_rv32i {
    using section = assembler_rv32i::section;

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

    //
    // overridden methods
    //

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);

        assembler_rv32i& a{assembler()};

        a.li(indent, "a7", syscall_exit_);
        a.ecall(indent);
    }

  protected:
    //
    // overridden methods
    //

    // an operating system loads the program where it has room
    auto
    check_memory_end([[maybe_unused]] const size_t memory_end_address) const
        -> void override {}

    // linux stops the program with a trap signal
    auto emit_divide_by_zero_trap() -> void override {
        assembler_rv32i& a{assembler()};

        a.label(0, "1");
        a.ebreak(1);
        a.j(1, "1b");
    }

    auto emit_read_call(const size_t indent) -> void override {
        assembler_rv32i& a{assembler()};

        a.li(indent, "a7", syscall_read_);
        a.ecall(indent);
    }

    // the registers as 'machine_rv32i::emit_sector_routines' says; an
    // operating system transfers the sectors of the image file: the image file
    // is opened at the first transfer, its descriptor and sector count kept in
    // '.Lbaz_sd_card', a sector past the end, a failed open or a short transfer
    // panics
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void override {

        // 'AT_FDCWD', 'O_RDWR' and 'SEEK_END' of linux
        constexpr int current_directory{-100};
        constexpr int read_write{2};
        constexpr int seek_end{2};
        // the descriptor, the sector count and the 64 bit size of the file
        constexpr std::array<int64_t, 4> sd_card_words{-1, 0, 0, 0};
        constexpr int64_t count_offset{4};
        constexpr int64_t size_low_offset{8};
        constexpr int64_t size_high_offset{12};
        // the arguments are kept while the file is opened, 'a5' with the
        // system call is not written there and an 'ecall' changes only 'a0'
        constexpr std::array<std::string_view, 3> saved{"a0", "a1", "a2"};
        // the file name ends with a zero byte for the system call
        constexpr std::array<int64_t, 1> name_end{0};
        // the byte offset of a sector is split into two words
        constexpr int sector_shift{std::countr_zero(card_sector_size_bytes)};

        constexpr int high_shift{
            static_cast<int>(register_bits_) - sector_shift,
        };

        assembler_rv32i& a{assembler()};

        if (read_used) {
            a.label(0, sector_read_label_);
            a.li(1, "a5", syscall_pread_);
            a.j(1, sector_transfer_label_);
        }

        if (write_used) {
            a.label(0, sector_write_label_);
            a.li(1, "a5", syscall_pwrite_);
        }

        // 'a5' holds the system call, 'a6' the return address
        a.label(0, sector_transfer_label_);
        a.mv(1, "a6", "a7");
        a.la(1, "a3", sd_card_label_);
        a.lw(1, "a4", 0, "a3");
        a.bgez(1, "a4", "2f");

        const size_t stack_bytes{save_registers(1, saved)};

        a.li(1, "a0", current_directory);
        a.la(1, "a1", sd_card_name_label_);
        a.li(1, "a2", read_write);
        a.li(1, "a3", 0);
        a.li(1, "a7", syscall_openat_);
        a.ecall(1);
        a.bltz(1, "a0", sd_card_failure.label);
        a.la(1, "a3", sd_card_label_);
        a.sw(1, "a0", 0, "a3");
        // 'llseek' writes the size of the file at 'a3'
        a.li(1, "a1", 0);
        a.li(1, "a2", 0);
        a.addi(1, "a3", "a3", size_low_offset);
        a.li(1, "a4", seek_end);
        a.li(1, "a7", syscall_llseek_);
        a.ecall(1);
        a.bnez(1, "a0", sd_card_failure.label);
        a.la(1, "a3", sd_card_label_);
        a.lw(1, "a4", size_low_offset, "a3");
        a.lw(1, "a0", size_high_offset, "a3");
        a.srli(1, "a4", "a4", sector_shift);
        a.slli(1, "a1", "a0", high_shift);
        a.or_op(1, "a4", "a4", "a1");
        // an image of 2 TB or more has more sectors than a sector number
        // reaches
        a.srli(1, "a0", "a0", sector_shift);
        a.beqz(1, "a0", "1f");
        a.li(1, "a4", -1);
        a.label(0, "1");
        a.sw(1, "a4", count_offset, "a3");
        restore_saved_registers(1, saved, stack_bytes);
        a.label(0, "2");
        a.lw(1, "a4", count_offset, "a3");
        a.bgeu(1, "a0", "a4", sd_card_failure.label);
        // 'a0' the descriptor, 'a3' and 'a4' the low and high word of the
        // offset
        a.lw(1, "a7", 0, "a3");
        a.slli(1, "a3", "a0", sector_shift);
        a.srli(1, "a4", "a0", high_shift);
        a.mv(1, "a0", "a7");
        a.mv(1, "a7", "a5");
        a.ecall(1);
        a.bne(1, "a0", "a2", sd_card_failure.label);
        a.jr(1, "a6");

        emit_failure_handler(sd_card_failure, false);

        a.switch_section(section::data);
        a.align(word_size_bytes_);
        a.label(0, sd_card_label_);
        a.data(word_size_bytes_, sd_card_words);
        a.switch_section(section::rodata);
        a.label(0, sd_card_name_label_);
        a.string_data(sd_card_file_name());
        a.data(1, name_end);
        a.switch_section(section::text);
    }

    auto emit_write_call(const size_t indent) -> void override {
        assembler_rv32i& a{assembler()};

        a.li(indent, "a7", syscall_write_);
        a.ecall(indent);
    }
};
