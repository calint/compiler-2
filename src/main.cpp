// reviewed: 2025-09-29

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "assembler.hpp"
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "decouple_impl.hpp" // IWYU pragma: keep
#include "machine.hpp"
#include "machine_rv32i.hpp"
#include "machine_rv32i_fpga.hpp"
#include "machine_rv32i_qemu.hpp"
#include "machine_x86_64.hpp"
#include "null_stream.hpp"
#include "panic_exception.hpp"
#include "program.hpp"
#include "tokenizer.hpp"

namespace {
struct check_options {
    bool upper{};
    bool lower{};
    bool show_line{};
    bool frame{};
    bool alias{};
};

[[nodiscard]] auto read_file_to_string(const char* const file_name)
    -> std::string;

[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view name,
                                    const size_t alignment)
    -> std::optional<size_t>;

[[nodiscard]] auto parse_checks(const std::string_view checks)
    -> std::optional<check_options>;

[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const std::string_view target) -> std::string;

[[nodiscard]] auto make_backend(const std::string_view target, std::ostream& os,
                                const std::string_view src,
                                const assembler::jump_mode jumps,
                                const size_t stack_size_bytes,
                                const std::string_view binary_file_name)
    -> std::unique_ptr<machine>;

auto print_usage_error(const std::string_view message) -> void;

auto print_source_error(const std::string_view src_file_name,
                        const std::string_view src, const size_t line,
                        const size_t start_index,
                        const std::string_view message) -> void;
} // namespace

auto main(const int argc, const char** const argv) -> int {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
    const std::span<const char*> args{argv, static_cast<size_t>(argc)};
#pragma clang diagnostic pop

    constexpr size_t default_vars_size_bytes{0x10000};
    constexpr size_t vars_alignment{16};
    constexpr size_t default_stack_size_bytes{0x10000};
    // keeps sp 16-byte aligned
    constexpr size_t stack_alignment{16};
    // note: to avoid "magic number" lint

    // default values
    const char* src_file_name{"prog.baz"};
    std::string_view target{"x86_64"};
    size_t vars_size_bytes{default_vars_size_bytes};
    size_t stack_size_bytes{default_stack_size_bytes};
    check_options checks{};
    bool optimize_jumps{true};
    bool reproduce_source{};
    // empty until given, the default depends on the source and target
    std::string_view binary_file_name{};

    // parse arguments
    for (const char* argument : args | std::views::drop(1)) {
        const std::string_view arg{argument};

        if (arg == "--help" or arg == "-h") {
            // same layout as the readme usage section which pastes this output
            std::print(R"(usage: {0} [options] [file]
compiles file (default: prog.baz) to assembly on stdout, rv32i targets also
write a binary image

options:
  --target=MACHINE    x86_64 (default): linux, nasm
                      rv32i: linux, llvm assembler, qemu user mode
                      rv32i-qemu: bare-metal image for the qemu virt machine
                      rv32i-fpga: bare-metal image for the fpga soft core,
                        stack grows down from the end of the 8 mib memory,
                        fails when code, data, variables and stack do not fit
  --vars=SIZE         variable storage in bytes, decimal or 0x hex, must be a
                      multiple of {1} (default: {2})
  --stack=SIZE        rv32i-qemu and rv32i-fpga stack in bytes, decimal or 0x
                      hex, must be a multiple of {3} (default: {4})
  --checks=LIST       comma separated checks, replaces earlier --checks
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --nopt              no jump optimizations
  --reproduce-source  write reproduced source to diff.baz and check that it
                      matches the input
  --help, -h          this help

checks:
  upper  runtime upper array bounds, often enough to also catch negative
         indexes
  lower  runtime lower array bounds
  line   report line number on failed bounds check
  frame  runtime non-inlined function frame capacity
  alias  compile time rejection of calls where a result or argument may
         share storage

examples:
  {0} prog.baz > prog.s
  {0} --vars=0x40000 --checks=upper prog.baz > prog.s
  {0} --checks=upper,lower,line,frame prog.baz > prog.s
  {0} --target=rv32i-qemu --stack=0x20000 prog.baz > prog.s
  {0} --target=rv32i-fpga --checks=upper,line prog.baz > prog.s
  {0} --target=rv32i-qemu --bin=image.bin prog.baz > prog.s
)",
                       args[0], vars_alignment, default_vars_size_bytes,
                       stack_alignment, default_stack_size_bytes);

            return 0;
        }
        constexpr std::string_view vars_option{"--vars="};
        constexpr std::string_view stack_option{"--stack="};
        constexpr std::string_view target_option{"--target="};
        constexpr std::string_view checks_option{"--checks="};
        constexpr std::string_view bin_option{"--bin="};
        constexpr std::string_view nopt_option{"--nopt"};
        if (arg.starts_with(vars_option)) {
            const std::optional<size_t> parsed{
                parse_size_bytes(arg.substr(vars_option.size()),
                                 "variable storage size", vars_alignment)};

            if (not parsed) {
                return 1;
            }

            vars_size_bytes = *parsed;
        } else if (arg.starts_with(stack_option)) {
            const std::optional<size_t> parsed{
                parse_size_bytes(arg.substr(stack_option.size()), "stack size",
                                 stack_alignment)};

            if (not parsed) {
                return 1;
            }

            stack_size_bytes = *parsed;
        } else if (arg.starts_with(target_option)) {
            target = arg.substr(target_option.size());
            if (target != "x86_64" and target != "rv32i" and
                target != "rv32i-qemu" and target != "rv32i-fpga") {
                print_usage_error(
                    std::format("Invalid target: '{}'. Supported targets are: "
                                "x86_64, rv32i, rv32i-qemu, rv32i-fpga.",
                                target));

                return 1;
            }
        } else if (arg.starts_with(checks_option)) {
            const std::optional<check_options> parsed{
                parse_checks(arg.substr(checks_option.size()))};

            if (not parsed) {
                return 1;
            }

            checks = *parsed;
        } else if (arg.starts_with(bin_option)) {
            binary_file_name = arg.substr(bin_option.size());
            if (binary_file_name.empty()) {
                print_usage_error("Invalid --bin: empty file name");

                return 1;
            }
        } else if (arg == nopt_option) {
            optimize_jumps = false;
        } else if (arg == "--reproduce-source") {
            reproduce_source = true;
        } else if (not arg.starts_with("--")) {
            // assume it's the filename
            src_file_name = argument;
        } else {
            print_usage_error(std::format("Error: Unknown option: {}", arg));

            return 1;
        }
    }

    std::string src;
    try {
        src = read_file_to_string(src_file_name);

        const assembler::jump_mode jumps{optimize_jumps
                                             ? assembler::jump_mode::optimized
                                             : assembler::jump_mode::resolved};

        // the output from the parse stage is discarded, compile receives the
        // output stream 'build' writes the complete assembly
        null_stream parser_output;

        const std::string binary{
            binary_file_name.empty()
                ? default_binary_file_name(src_file_name, target)
                : std::string{binary_file_name}};

        const std::unique_ptr<machine> backend{make_backend(
            target, parser_output, src, jumps, stack_size_bytes, binary)};

        program prg{*backend,     src,          vars_size_bytes,
                    checks.upper, checks.lower, checks.show_line,
                    checks.frame, checks.alias};

        if (reproduce_source) {
            std::ofstream reproduced_source{"diff.baz"};
            prg.source_to(reproduced_source);
            reproduced_source.close();
            if (src != read_file_to_string("diff.baz")) {
                throw panic_exception{
                    std::format("generated source differs. diff {} diff.baz",
                                src_file_name)};
            }
        }

        prg.build(std::cout);

    } catch (const compiler_exception& e) {
        print_source_error(src_file_name, src, e.line, e.start_index, e.msg);

        return 1;
    } catch (const tokenizer_exception& e) {
        print_source_error(src_file_name, src, e.line, e.start_index, e.what());

        return 1;
    } catch (const panic_exception& e) {
        std::println(stderr, "\npanic: {}", e.what());

        return 1;
    } catch (const std::overflow_error& e) {
        std::println(stderr, "\n{}", e.what());

        return 1;
    } catch (...) {
        std::println(stderr, "\nunknown exception");

        return 1;
    }
}

namespace {
[[nodiscard]] auto read_file_to_string(const char* const file_name)
    -> std::string {
    std::ifstream fs{file_name};
    if (not fs.is_open()) {
        throw panic_exception{std::format("cannot open file '{}'", file_name)};
    }

    return std::string{std::istreambuf_iterator<char>{fs},
                       std::istreambuf_iterator<char>{}};
}

// decimal or 0x hexadecimal, 'name' describes the size in error messages
[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view name,
                                    const size_t alignment)
    -> std::optional<size_t> {

    uint64_t parsed_size_bytes{};
    try {
        const std::string digits{text};
        size_t chars_read{};
        parsed_size_bytes = std::stoull(digits, &chars_read, 0);

        if (digits.empty() or digits.starts_with('-') or
            chars_read != digits.size() or parsed_size_bytes == 0 or
            not std::in_range<size_t>(parsed_size_bytes)) {

            throw std::invalid_argument{std::format("invalid {}", name)};
        }
    } catch (...) {
        print_usage_error(
            std::format("Could not parse {}: \"{}\"", name, text));

        return std::nullopt;
    }

    if (parsed_size_bytes % alignment != 0) {
        print_usage_error(std::format(
            "Invalid {}: '{}' is not a multiple of {}", name, text, alignment));

        return std::nullopt;
    }

    return static_cast<size_t>(parsed_size_bytes);
}

// each '--checks' replaces the earlier ones, empty parts are ignored
[[nodiscard]] auto parse_checks(const std::string_view checks)
    -> std::optional<check_options> {

    check_options parsed{};

    for (const auto part : checks | std::views::split(',')) {
        const std::string_view option{part};
        if (option == "upper") {
            parsed.upper = true;
        } else if (option == "lower") {
            parsed.lower = true;
        } else if (option == "line") {
            parsed.show_line = true;
        } else if (option == "frame") {
            parsed.frame = true;
        } else if (option == "alias") {
            parsed.alias = true;
        } else if (not option.empty()) {
            print_usage_error(
                std::format("Invalid --checks option: '{}'. Supported options "
                            "are: upper, lower, line, frame, alias.",
                            option));

            return std::nullopt;
        }
    }

    return parsed;
}

// keeps the directory so the image lands next to its source
[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const std::string_view target) -> std::string {

    std::filesystem::path path{src_file_name};
    path.replace_extension();

    return std::format("{}-{}.bin", path.string(), target);
}

// 'os' receives the output of the parse stage
[[nodiscard]] auto make_backend(const std::string_view target, std::ostream& os,
                                const std::string_view src,
                                const assembler::jump_mode jumps,
                                const size_t stack_size_bytes,
                                const std::string_view binary_file_name)
    -> std::unique_ptr<machine> {

    // todo: x86_64 writes no binary, see etc/todo.txt
    if (target == "x86_64") {
        return std::make_unique<machine_x86_64>(os, src, jumps);
    }

    if (target == "rv32i") {
        return std::make_unique<machine_rv32i>(os, src, jumps,
                                               binary_file_name);
    }

    if (target == "rv32i-qemu") {
        return std::make_unique<machine_rv32i_qemu>(
            os, src, jumps, binary_file_name, stack_size_bytes);
    }

    if (target == "rv32i-fpga") {
        return std::make_unique<machine_rv32i_fpga>(
            os, src, jumps, binary_file_name, stack_size_bytes);
    }

    throw panic_exception{std::format("unknown target '{}'", target)};
}

// every command line error ends with the same hint
auto print_usage_error(const std::string_view message) -> void {
    std::println(stderr, "{}", message);
    std::println(stderr, "Use --help for usage information");
}

// 'line' and 'start_index' locate the error, the column is derived from them
auto print_source_error(const std::string_view src_file_name,
                        const std::string_view src, const size_t line,
                        const size_t start_index,
                        const std::string_view message) -> void {

    const auto [line_num,
                col]{line_and_col_num_for_char_index(line, start_index, src)};

    std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line_num, col,
                 message);
}
} // namespace
