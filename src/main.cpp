// reviewed: 2025-09-29

#include <cstddef>
#include <cstdint>
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

#include "almost_assembler.hpp"
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "decouple_impl.hpp" // IWYU pragma: keep
#include "machine.hpp"
#include "machine_rv32i.hpp"
#include "machine_rv32i_fpga.hpp"
#include "machine_rv32i_qemu.hpp"
#include "machine_x86.hpp"
#include "null_stream.hpp"
#include "panic_exception.hpp"
#include "program.hpp"
#include "tokenizer.hpp"

namespace {
[[nodiscard]] auto read_file_to_string(const char* const file_name)
    -> std::string;

[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view name,
                                    const size_t alignment)
    -> std::optional<size_t>;
} // namespace

// NOLINTNEXTLINE(bugprone-exception-escape)
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
    bool checks_upper{};
    bool checks_show_line{};
    bool checks_lower{};
    bool checks_frame{};
    bool checks_alias{};
    bool optimize_jumps{true};
    bool reproduce_source{};

    // parse arguments
    for (const char* argument : args | std::views::drop(1)) {
        const std::string_view arg{argument};

        if (arg == "--help" or arg == "-h") {
            const std::string_view prg{args[0]};
            std::println("Usage: {} [OPTIONS] [filename]", prg);
            std::println("");
            std::println("Options:");
            std::println("  --target=MACHINE    x86_64 (default), rv32i, "
                         "rv32i-qemu or rv32i-fpga");
            std::println("  --vars=SIZE         Set variable storage size "
                         "(default: "
                         "0x10000/65536)");

            std::println(
                "                      Supports decimal and hex (0x prefix) ");

            std::println("                      Must be a multiple of {}",
                         vars_alignment);

            std::println("  --stack=SIZE        Set rv32i-qemu stack size "
                         "(default: 0x10000/65536)");

            std::println("                      Must be a multiple of {}",
                         stack_alignment);

            std::println("  --checks=TYPE       Enable runtime checks:");
            std::println(
                "                        upper - check upper array bounds");

            std::println(
                "                        lower - check lower array bounds");

            std::println("                         line - report line number");
            std::println("                        frame - check function frame "
                         "capacity");
            std::println("                        alias - reject calls where a "
                         "result or argument may share storage");

            std::println(" upper,lower,line,frame,alias - all");
            std::println("  --nopt              No jump optimizations");
            std::println("  --reproduce-source  Write reproduced source to "
                         "diff.baz and check it matches the input");

            std::println("  --help, -h          Show this help message");
            std::println("");
            std::println("Arguments:");
            std::println(
                "  filename            Source file (default: prog.baz)");

            std::println("");
            std::println("Examples:");
            std::println("  {} myfile.baz", prg);
            std::println("  {} --vars=131072 --checks=upper prog.baz", prg);
            std::println("  {} --checks=upper,lower,line prog.baz", prg);
            std::println("  {} --checks=upper prog.baz", prg);

            return 0;
        }
        constexpr std::string_view vars_option{"--vars="};
        constexpr std::string_view stack_option{"--stack="};
        constexpr std::string_view target_option{"--target="};
        constexpr std::string_view checks_option{"--checks="};
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
                std::println(stderr,
                             "Invalid target: '{}'. Supported targets are: "
                             "x86_64, rv32i, rv32i-qemu, rv32i-fpga.",
                             target);

                std::println(stderr, "Use --help for usage information");

                return 1;
            }
        } else if (arg.starts_with(checks_option)) {
            const std::string_view checks{arg.substr(checks_option.size())};

            checks_upper = false;
            checks_lower = false;
            checks_show_line = false;
            checks_frame = false;
            checks_alias = false;

            for (const auto part : checks | std::views::split(',')) {
                const std::string_view option{part};
                if (option == "upper") {
                    checks_upper = true;
                } else if (option == "lower") {
                    checks_lower = true;
                } else if (option == "line") {
                    checks_show_line = true;
                } else if (option == "frame") {
                    checks_frame = true;
                } else if (option == "alias") {
                    checks_alias = true;
                } else if (not option.empty()) {
                    std::println(std::cerr,
                                 "Invalid --checks option: '{}'. Supported "
                                 "options are: upper, lower, line, frame, "
                                 "alias.",
                                 option);

                    std::println(stderr, "Use --help for usage information");

                    return 1;
                }
            }
        } else if (arg == nopt_option) {
            optimize_jumps = false;
        } else if (arg == "--reproduce-source") {
            reproduce_source = true;
        } else if (not arg.starts_with("--")) {
            // assume it's the filename
            src_file_name = argument;
        } else {
            std::println(stderr, "Error: Unknown option: {}", arg);
            std::println(stderr, "Use --help for usage information");

            return 1;
        }
    }

    std::string src;
    try {
        src = read_file_to_string(src_file_name);

        const almost_assembler::jump_mode jumps{
            optimize_jumps ? almost_assembler::jump_mode::optimized
                           : almost_assembler::jump_mode::resolved};

        // the output from the parse stage is discarded, compile receives the
        // output stream 'build' writes the complete assembly
        null_stream parser_output;

        std::unique_ptr<machine> backend;
        if (target == "x86_64") {
            backend = std::make_unique<machine_x86>(parser_output, src, jumps);
        } else if (target == "rv32i") {
            backend = std::make_unique<machine_rv32i>(parser_output, src, jumps,
                                                      "gen-rv32i.bin");
        } else if (target == "rv32i-qemu") {
            backend = std::make_unique<machine_rv32i_qemu>(
                parser_output, src, jumps, "gen-rv32i.bin", stack_size_bytes);
        } else if (target == "rv32i-fpga") {
            backend = std::make_unique<machine_rv32i_fpga>(
                parser_output, src, jumps, "gen-rv32i.bin");
        } else {
            throw panic_exception{std::format("unknown target '{}'", target)};
        }

        program prg{*backend,     src,          vars_size_bytes,
                    checks_upper, checks_lower, checks_show_line,
                    checks_frame, checks_alias};

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
        const auto [line, col]{
            line_and_col_num_for_char_index(e.line, e.start_index, src)};

        std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line, col, e.msg);

        return 1;
    } catch (const tokenizer_exception& e) {
        const auto [line, col]{
            line_and_col_num_for_char_index(e.line, e.start_index, src)};

        std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line, col,
                     e.what());

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
        std::println(stderr, "Could not parse {}: \"{}\"", name, text);
        std::println(stderr, "Use --help for usage information");

        return std::nullopt;
    }

    if (parsed_size_bytes % alignment != 0) {
        std::println(stderr, "Invalid {}: '{}' is not a multiple of {}", name,
                     text, alignment);

        std::println(stderr, "Use --help for usage information");

        return std::nullopt;
    }

    return static_cast<size_t>(parsed_size_bytes);
}
} // namespace
