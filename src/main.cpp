// review: 2025-09-29

#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <print>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple_impl.hpp" // IWYU pragma: keep
#include "jump_optimizer.hpp"
#include "panic_exception.hpp"
#include "program.hpp"
#include "tokenizer.hpp"
#include "utils.hpp"

namespace {
[[nodiscard]] auto read_file_to_string(const char* file_name) -> std::string;
} // namespace

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(const int argc, const char* argv[]) -> int {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
    const std::span<const char*> args{argv, static_cast<size_t>(argc)};
#pragma clang diagnostic pop

    constexpr size_t default_stack_size{0x10000};
    // note: to avoid "magic number" lint

    // default values
    const char* src_file_name{"prog.baz"};
    size_t stack_size{default_stack_size};
    bool checks_upper{};
    bool checks_show_line{};
    bool checks_lower{};
    bool optimize_jumps{true};

    // parse arguments
    for (size_t i{1}; i < args.size(); ++i) {
        const std::string_view arg{args[i]};

        if (arg == "--help" or arg == "-h") {
            const std::string_view prg{args[0]};
            std::println("Usage: {} [OPTIONS] [filename]", prg);
            std::println("");
            std::println("Options:");
            std::println("  --stack=SIZE        Set stack size (default: "
                         "0x10000/65536)");
            std::println(
                "                      Supports decimal and hex (0x prefix) ");
            std::println("  --checks=TYPE       Enable runtime checks:");
            std::println(
                "                        upper - check upper array bounds");
            std::println(
                "                        lower - check lower array bounds");
            std::println("                         line - report line number");
            std::println("             upper,lower,line - all");
            std::println("  --nopt              No jump optimizations");
            std::println("  --help, -h          Show this help message");
            std::println("");
            std::println("Arguments:");
            std::println(
                "  filename            Source file (default: prog.baz)");
            std::println("");
            std::println("Examples:");
            std::println("  {} myfile.baz", prg);
            std::println("  {} --stack=131072 --checks=upper prog.baz", prg);
            std::println("  {} --checks=upper,lower,line prog.baz", prg);
            std::println("  {} --checks=upper prog.baz", prg);
            return 0;
        }
        constexpr std::string_view stack_option{"--stack="};
        constexpr std::string_view checks_option{"--checks="};
        constexpr std::string_view nopt_option{"--nopt"};
        if (arg.starts_with(stack_option)) {
            try {
                const std::string stack_text{arg.substr(stack_option.size())};
                size_t chars_read{};
                const uint64_t parsed_size{
                    std::stoull(stack_text, &chars_read, 0)};
                if (stack_text.empty() or stack_text.starts_with('-') or
                    chars_read != stack_text.size() or parsed_size == 0 or
                    not std::in_range<size_t>(parsed_size)) {
                    throw std::invalid_argument{"invalid stack size"};
                }
                stack_size = static_cast<size_t>(parsed_size);
            } catch (...) {
                std::println(stderr, "Could not parse stack size: \"{}\"",
                             arg.substr(stack_option.size()));
                std::println(stderr, "Use --help for usage information");
                return 1;
            }
        } else if (arg.starts_with(checks_option)) {
            const std::string_view checks{arg.substr(checks_option.size())};
            std::istringstream iss{std::string{checks}};

            checks_upper = false;
            checks_lower = false;
            checks_show_line = false;

            std::string option;
            while (std::getline(iss, option, ',')) {
                if (option == "upper") {
                    checks_upper = true;
                } else if (option == "lower") {
                    checks_lower = true;
                } else if (option == "line") {
                    checks_show_line = true;
                } else if (not option.empty()) {
                    std::println(std::cerr,
                                 "Invalid --checks option: '{}'. Supported "
                                 "options are: upper, lower, line.",
                                 option);
                    std::println(stderr, "Use --help for usage information");
                    return 1;
                }
            }
        } else if (arg == nopt_option) {
            optimize_jumps = false;
        } else if (not arg.starts_with("--")) {
            // assume it's the filename
            src_file_name = args[i];
        } else {
            std::println(stderr, "Error: Unknown option: {}", arg);
            std::println(stderr, "Use --help for usage information");
            return 1;
        }
    }

    std::string src;
    try {
        src = read_file_to_string(src_file_name);
        program prg{src, stack_size, checks_upper, checks_lower,
                    checks_show_line};
        std::ofstream reproduced_source{"diff.baz"};
        prg.source_to(reproduced_source);
        reproduced_source.close();
        if (src != read_file_to_string("diff.baz")) {
            throw panic_exception{std::format(
                "generated source differs. diff {} diff.baz", src_file_name)};
        }

        if (optimize_jumps) {
            // with jump optimizations
            std::stringstream ss1;
            std::stringstream ss2;
            prg.build(ss1);
            jump_optimizer::pass1(ss1, ss2);
            jump_optimizer::pass2(ss2, std::cout);
        } else {
            prg.build(std::cout);
        }

    } catch (const compiler_exception& e) {
        const auto [line, col]{
            utils::line_and_col_num_for_char_index(e.line, e.start_index, src)};
        std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line, col, e.msg);
        return 1;
    } catch (const tokenizer_exception& e) {
        const auto [line, col]{
            utils::line_and_col_num_for_char_index(e.line, e.start_index, src)};
        std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line, col,
                     e.what());
        return 1;
    } catch (const panic_exception& e) {
        std::println(stderr, "\npanic: {}", e.what());
        return 1;
    } catch (...) {
        std::println(stderr, "\nunknown exception");
        return 1;
    }
}

namespace {
[[nodiscard]] auto read_file_to_string(const char* file_name) -> std::string {
    std::ifstream fs{file_name};
    if (not fs.is_open()) {
        throw panic_exception(std::format("cannot open file '{}'", file_name));
    }
    return std::string{std::istreambuf_iterator<char>{fs},
                       std::istreambuf_iterator<char>{}};
}
} // namespace
