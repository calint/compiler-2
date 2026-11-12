#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <vector>
// #define RV32I_DEBUG
#include "rv32i.hpp"
//
#include "main_config.hpp"
#include "sdcard.hpp"

// #define LOG_UART_IN_TO_STDERR

using namespace std;

// initialize RAM with -1 being the default value from flash
static vector<uint8_t> ram(osqa::memory_end, 0xff);

// SD card image, sectors are read and written in the file
static osqa::sdcard sdcard;

// preserved terminal settings
static struct termios saved_termios;

// writes a device register, the soft core decodes the whole address and
// ignores the access width
static auto store_device(uint32_t const address, uint32_t const data)
    -> rv32i::bus_status {

    switch (address) {
    case osqa::sdcard_busy: {
        // address does not support write
        return 2;
    }
    case osqa::sdcard_status: {
        // address does not support write
        return 3;
    }
    case osqa::sdcard_next_byte: {
        sdcard.next_byte(uint8_t(data));
        return 0;
    }
    case osqa::sdcard_write_sector: {
        // sector past the end of the image or failed write
        if (!sdcard.write_sector(data)) {
            return 4;
        }
        return 0;
    }
    case osqa::sdcard_read_sector: {
        // sector past the end of the image or failed read
        if (!sdcard.read_sector(data)) {
            return 5;
        }
        return 0;
    }
    case osqa::uart_out: {
        putchar(data & 0xff);
        fflush(stdout);
        return 0;
    }
    case osqa::uart_in: {
        // address does not support write
        return 6;
    }
    case osqa::led: {
        // do nothing when writing to address LED
        return 0;
    }
    default: {
        // no device register at the address, e.g. the second byte of one
        return 10;
    }
    }
}

// reads a device register, the soft core returns the whole word for every
// access width
static auto load_device(uint32_t const address, uint32_t& data)
    -> rv32i::bus_status {

    switch (address) {
    case osqa::sdcard_status: {
        data = 6;
        return 0;
    }
    case osqa::sdcard_busy: {
        data = 0;
        return 0;
    }
    case osqa::sdcard_next_byte: {
        data = sdcard.next_byte();
        return 0;
    }
    case osqa::sdcard_read_sector: {
        // address does not support read
        return 7;
    }
    case osqa::sdcard_write_sector: {
        // address does not support read
        return 8;
    }
    case osqa::led: {
        // address does not support read
        return 9;
    }
    case osqa::uart_out: {
        data = 0xffff'ffff; // -1
        return 0;
    }
    case osqa::uart_in: {
        int const ch = getchar();
        // convert terminal to serial
        switch (ch) {
        case EOF:               // no data available
            data = 0xffff'ffff; // -1
            break;
        case '\n': // newline to carriage return
            data = '\r';
            break;
        case 0x08: // backspace
            data = 0x7f;
            break;
        default:
            data = uint32_t(ch);
            break;
        }
#ifdef LOG_UART_IN_TO_STDERR
        if (ch != -1) {
            fputc(char(data), stderr);
        }
#endif
        return 0;
    }
    default: {
        // no device register at the address, e.g. the second byte of one
        return 10;
    }
    }
}

// bus callback
static auto bus(uint32_t const address, rv32i::bus_op_width const op_width,
                bool const is_store, uint32_t& data) -> rv32i::bus_status {

    // the device registers are at the end of the address space, above the
    // memory
    if (address >= osqa::io_addresses_start) {
        return is_store ? store_device(address, data)
                        : load_device(address, data);
    }

    // address
    //
    //   31                         2 1    0
    //  +----------------------------+------+
    //  |        ram_address         | lane |
    //  +----------------------------+------+
    //
    //  RAM word at ram_address, bytes accessed per op_width and lane:
    //
    //              byte 0   byte 1   byte 2   byte 3
    //             +--------+--------+--------+--------+
    //  BYTE      0|   x    |        |        |        |
    //            1|        |   x    |        |        |
    //            2|        |        |   x    |        |
    //            3|        |        |        |   x    |
    //             +--------+--------+--------+--------+
    //  HALF_WORD 0|   x    |   x    |        |        |
    //            2|        |        |   x    |   x    |
    //             +--------+--------+--------+--------+
    //  WORD      0|   x    |   x    |   x    |   x    |
    //             +--------+--------+--------+--------+

    uint32_t const ram_address = address & ~uint32_t(3);

    // byte lane within the word
    uint32_t const lane = address & uint32_t(3);

    // note: the core traps a misaligned half word or word before the bus
    //       ('ramio' would ignore it), so every access is aligned
    assert(lane % uint32_t(op_width) == 0);

    // outside the memory range
    if (ram_address + sizeof(uint32_t) > ram.size()) {
        return 1;
    }

    if (is_store) {
        for (uint32_t i = 0; i < uint32_t(op_width); ++i) {
            ram[ram_address + lane + i] = uint8_t(data >> (i * 8));
        }
    } else {
        data = 0;
        for (uint32_t i = 0; i < uint32_t(op_width); ++i) {
            data |= uint32_t(ram[ram_address + lane + i]) << (i * 8);
        }
    }

    return 0;
}

static auto load_file(char const* file_name, char const* data_name,
                      vector<uint8_t>& data) -> bool {

    ifstream file{file_name, ios::binary | ios::ate};
    if (!file) {
        printf("%s: error opening file '%s'\n", data_name, file_name);
        return false;
    }

    streamsize const size = file.tellg();
    if (size == -1) {
        printf("%s: error determining size of file '%s'\n", data_name,
               file_name);
        return false;
    }

    if (size > streamsize(data.size())) {
        printf(
            "%s: size of file (%zu B) exceeds size of data container (%zu B)\n",
            data_name, size, data.size());
        return false;
    }

    file.seekg(0, ios::beg);
    if (file.fail()) {
        printf("%s: error seeking to beginning of file '%s'\n", data_name,
               file_name);
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        printf("%s: error reading file '%s'\n", data_name, file_name);
        return false;
    }

    file.close();

    return true;
}

auto main(int argc, char** argv) -> int {
    if (argc != 3) {
        printf("Usage: %s <firmware.bin> <sdcard.bin>\n", argv[0]);
        return 1;
    }

    if (!load_file(argv[1], "Firmware", ram)) {
        return 2;
    }

    if (!sdcard.open(argv[2])) {
        printf("SD card: error opening file '%s'\n", argv[2]);
        return 3;
    }

    // configure terminal to not echo and enable non-blocking getchar()
    tcgetattr(STDIN_FILENO, &saved_termios);
    struct termios newt = saved_termios;
    newt.c_lflag &= tcflag_t(~(ICANON | ECHO));
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    { // note: code block to not get shadowed by 'flags' in 'atexit'
        int const flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    // reset terminal settings at exit
    atexit([] {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        int const flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    });

    rv32i::cpu cpu{bus};

    while (true) {
        if (rv32i::cpu::status const s = cpu.tick()) {
            printf("CPU error: %u\n", s);
            return int32_t(s);
        }
        // the program ends with 'ebreak' and the exit code in a0
        if (cpu.is_halted()) {
            return cpu.reg(10);
        }
    }

    return 0;
}
