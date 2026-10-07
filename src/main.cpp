// reviewed: 2025-09-29

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
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
#include "program.hpp"
#include "toc.hpp"

namespace {
// the sizes of '--vars' and '--stack' are multiples of their alignment, the
// stack one keeps sp 16-byte aligned
constexpr size_t default_vars_size_bytes{0x10000};
constexpr size_t vars_alignment{16};
constexpr size_t default_stack_size_bytes{0x10000};
constexpr size_t stack_alignment{16};

enum class target : uint8_t { x86_64, rv32i, rv32i_qemu, rv32i_fpga };

struct target_name {
    target kind;
    std::string_view text;
};

// the names of '--target', in the order of the usage text
constexpr std::array<target_name, 4> target_names{
    target_name{.kind{target::x86_64}, .text{"x86_64"}},
    target_name{.kind{target::rv32i}, .text{"rv32i"}},
    target_name{.kind{target::rv32i_qemu}, .text{"rv32i-qemu"}},
    target_name{.kind{target::rv32i_fpga}, .text{"rv32i-fpga"}},
};

// what the compiler reports after the code, besides the usual statistics
struct report_options {
    bool registers{};
};

struct options {
    const char* src_file_name{"prog.baz"};
    target machine_target{target::x86_64};
    size_t vars_size_bytes{default_vars_size_bytes};
    size_t stack_size_bytes{default_stack_size_bytes};
    check_options checks{};
    bool optimize_jumps{true};
    bool reproduce_source{};
    report_options reports{};
    // empty until given, the default depends on the source and target
    std::string_view binary_file_name;
};

// an option with a value, e.g. '--vars=0x40000'
struct value_option {
    std::string_view name;
    bool (*apply)(std::string_view value, options& opts);
};

// the exit code when the program should stop instead of compiling
[[nodiscard]] auto parse_options(const std::span<const char*> args,
                                 options& opts) -> std::optional<int>;

// false after printing the error
[[nodiscard]] auto parse_option(const char* const argument, options& opts)
    -> bool;

// the appliers of the options with a value store the value in 'opts', false
// after printing the error
[[nodiscard]] auto apply_vars(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_stack(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_target(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_checks(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_report(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_bin(std::string_view value, options& opts) -> bool;

auto print_help(const char* const program_name) -> void;

[[nodiscard]] auto compile_file(const options& opts) -> int;

auto print_compiler_error(const options& opts, std::string_view src,
                          const compiler_exception& e) -> void;

auto check_reproduced_source(const program& prg, const options& opts,
                             std::string_view src) -> void;

[[nodiscard]] auto read_file_to_string(const char* const file_name)
    -> std::string;

[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view name,
                                    const size_t alignment)
    -> std::optional<size_t>;

[[nodiscard]] auto parse_checks(const std::string_view checks)
    -> std::optional<check_options>;

[[nodiscard]] auto parse_reports(const std::string_view reports)
    -> std::optional<report_options>;

[[nodiscard]] auto find_target(const std::string_view text)
    -> std::optional<target>;

[[nodiscard]] auto target_text(target kind) -> std::string_view;

[[nodiscard]] auto supported_target_texts() -> std::string;

[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const target machine_target) -> std::string;

[[nodiscard]] auto make_backend(const target machine_target, std::ostream& os,
                                const std::string_view src,
                                const assembler::jump_mode jumps,
                                const size_t stack_size_bytes,
                                const std::string_view binary_file_name)
    -> std::unique_ptr<machine>;

auto print_usage_error(const std::string_view message) -> void;

auto print_call_frames(std::string_view src_file_name, std::string_view src,
                       std::span<const compiler_exception::call_frame> frames)
    -> void;

auto trimmed_line_at(std::string_view src, size_t index) -> std::string_view;

auto print_source_error(const std::string_view src_file_name,
                        const std::string_view src, const size_t line,
                        const size_t start_index, const size_t end_index,
                        const std::string_view message) -> void;

auto print_source_line(std::string_view src, size_t start_index,
                       size_t end_index) -> void;

[[nodiscard]] auto is_utf8_continuation(char ch) -> bool;
} // namespace

auto main(const int argc, const char** const argv) -> int {
    // a counted view of a pointer is a span that the compiler accepts as safe
    const std::span<const char*> args{std::views::counted(argv, argc)};

    options opts;

    if (const std::optional<int> exit_code{parse_options(args, opts)}) {
        return *exit_code;
    }

    return compile_file(opts);
}

namespace {
[[nodiscard]] auto parse_options(const std::span<const char*> args,
                                 options& opts) -> std::optional<int> {

    for (const char* const argument : args | std::views::drop(1)) {
        const std::string_view arg{argument};

        if (arg == "--help" or arg == "-h") {
            print_help(args.at(0));
            return 0;
        }

        if (not parse_option(argument, opts)) {
            return 1;
        }
    }

    return std::nullopt;
}

// e.g. '0x40000' of '--vars=0x40000'
[[nodiscard]] auto option_value(const std::string_view arg,
                                const std::string_view name)
    -> std::optional<std::string_view> {

    if (not arg.starts_with(name)) {
        return std::nullopt;
    }

    return arg.substr(name.size());
}

// a failed parse has printed its error
template <typename T>
[[nodiscard]] auto store_parsed(const std::optional<T>& parsed, T& destination)
    -> bool {

    if (not parsed) {
        return false;
    }

    destination = *parsed;

    return true;
}

[[nodiscard]] auto parse_option(const char* const argument, options& opts)
    -> bool {

    constexpr std::array value_options{
        value_option{.name{"--vars="}, .apply{apply_vars}},
        value_option{.name{"--stack="}, .apply{apply_stack}},
        value_option{.name{"--target="}, .apply{apply_target}},
        value_option{.name{"--checks="}, .apply{apply_checks}},
        value_option{.name{"--report="}, .apply{apply_report}},
        value_option{.name{"--bin="}, .apply{apply_bin}},
    };

    const std::string_view arg{argument};

    for (const value_option& option : value_options) {
        if (const std::optional<std::string_view> value{
                option_value(arg, option.name),
            }) {

            return option.apply(*value, opts);
        }
    }

    if (arg == "--nopt") {
        opts.optimize_jumps = false;
        return true;
    }

    if (arg == "--reproduce-source") {
        opts.reproduce_source = true;
        return true;
    }

    // assume it's the filename
    if (not arg.starts_with("--")) {
        opts.src_file_name = argument;
        return true;
    }

    print_usage_error(std::format("Unknown option: '{}'", arg));

    return false;
}

[[nodiscard]] auto apply_vars(const std::string_view value, options& opts)
    -> bool {

    return store_parsed(
        parse_size_bytes(value, "variable storage size", vars_alignment),
        opts.vars_size_bytes);
}

[[nodiscard]] auto apply_stack(const std::string_view value, options& opts)
    -> bool {

    return store_parsed(parse_size_bytes(value, "stack size", stack_alignment),
                        opts.stack_size_bytes);
}

[[nodiscard]] auto apply_target(const std::string_view value, options& opts)
    -> bool {

    const std::optional<target> found{find_target(value)};

    if (not found) {
        print_usage_error(
            std::format("Invalid target: '{}'. Supported targets are: {}.",
                        value, supported_target_texts()));

        return false;
    }

    opts.machine_target = *found;

    return true;
}

[[nodiscard]] auto apply_checks(const std::string_view value, options& opts)
    -> bool {

    return store_parsed(parse_checks(value), opts.checks);
}

[[nodiscard]] auto apply_report(const std::string_view value, options& opts)
    -> bool {

    return store_parsed(parse_reports(value), opts.reports);
}

[[nodiscard]] auto apply_bin(const std::string_view value, options& opts)
    -> bool {

    opts.binary_file_name = value;

    if (opts.binary_file_name.empty()) {
        print_usage_error("Invalid --bin: empty file name");
        return false;
    }

    return true;
}

// same layout as the readme usage section which pastes this output
auto print_help(const char* const program_name) -> void {
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
  --report=LIST       comma separated reports after the code, replaces earlier
                      --report
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --nopt              no jump optimizations
  --reproduce-source  write reproduced source to diff.baz and check that it
                      matches the input
  --help, -h          this help

reports:
  registers  how the scratch registers are used at the busiest point: what
             each call frame holds, what a noinline frame would save and what
             each callee holds

checks:
  upper  runtime upper array bounds only, a negative index passes
  lower  runtime lower array bounds, catches negative indexes
  line   report line number on failed bounds check
  frame  runtime non-inlined function frame capacity
  alias  compile time rejection of calls where a result may share storage
         with an argument
  noub   all checks against undefined behavior: upper, lower, frame and
         alias

examples:
  {0} prog.baz > prog.s
  {0} --vars=0x40000 --checks=upper prog.baz > prog.s
  {0} --checks=upper,lower,line,frame prog.baz > prog.s
  {0} --target=rv32i-qemu --stack=0x20000 prog.baz > prog.s
  {0} --target=rv32i-fpga --checks=upper,line prog.baz > prog.s
  {0} --target=rv32i-qemu --bin=image.bin prog.baz > prog.s
)",
               program_name, vars_alignment, default_vars_size_bytes,
               stack_alignment, default_stack_size_bytes);
}

// discards what is written to it, the output of the parser is not used
class null_stream final : public std::ostream {
    class null_buffer final : public std::streambuf {
      protected:
        //
        // overridden methods
        //

        auto overflow(const int c) -> int override { return c; }
    } nb_{};

  public:
    null_stream() : std::ostream{&nb_} {}
};

[[nodiscard]] auto compile_file(const options& opts) -> int {
    std::string src;
    try {
        src = read_file_to_string(opts.src_file_name);

        const assembler::jump_mode jumps{
            opts.optimize_jumps ? assembler::jump_mode::optimized
                                : assembler::jump_mode::resolved,
        };

        // the output from the parse stage is discarded, compile receives the
        // output stream 'build' writes the complete assembly
        null_stream parser_output;

        const std::string binary{
            opts.binary_file_name.empty()
                ? default_binary_file_name(opts.src_file_name,
                                           opts.machine_target)
                : std::string{opts.binary_file_name},
        };

        const std::unique_ptr<machine> backend{
            make_backend(opts.machine_target, parser_output, src, jumps,
                         opts.stack_size_bytes, binary),
        };

        if (opts.reports.registers) {
            backend->enable_register_report();
        }

        program prg{*backend, src, opts.vars_size_bytes, opts.checks};

        if (opts.reproduce_source) {
            check_reproduced_source(prg, opts, src);
        }

        prg.build(std::cout);

    } catch (const compiler_exception& e) {
        print_compiler_error(opts, src, e);
        return 1;
    } catch (const std::runtime_error& e) {
        std::println(stderr, "\npanic: {}", e.what());
        return 1;
    }

    return 0;
}

// the message of the error at its source line, the inlined calls it was found
// in and what else is known about it
auto print_compiler_error(const options& opts, const std::string_view src,
                          const compiler_exception& e) -> void {

    print_source_error(opts.src_file_name, src, e.line, e.start_index,
                       e.end_index, e.msg);

    print_call_frames(opts.src_file_name, src, e.call_frames);

    if (not e.detail.empty()) {
        std::println(stderr, "\n{}", e.detail);
    }
}

// the source written back from the parsed program equals the input
auto check_reproduced_source(const program& prg, const options& opts,
                             const std::string_view src) -> void {

    std::ofstream reproduced_source{"diff.baz"};
    prg.source_to(reproduced_source);
    reproduced_source.close();

    if (src != read_file_to_string("diff.baz")) {
        throw std::runtime_error{std::format(
            "generated source differs. diff {} diff.baz", opts.src_file_name)};
    }
}

[[nodiscard]] auto read_file_to_string(const char* const file_name)
    -> std::string {

    std::ifstream fs{file_name};

    if (not fs.is_open()) {
        throw std::runtime_error{
            std::format("cannot open file '{}'", file_name)};
    }

    return std::string{std::istreambuf_iterator<char>{fs},
                       std::istreambuf_iterator<char>{}};
}

// decimal or 0x hexadecimal, 'name' describes the size in error messages
[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view name,
                                    const size_t alignment)
    -> std::optional<size_t> {

    constexpr int decimal_base{10};
    constexpr int hex_base{16};

    std::string_view digits{text};
    int base{decimal_base};

    if (digits.starts_with("0x") or digits.starts_with("0X")) {
        base = hex_base;
        digits.remove_prefix(2);
        // note: 2 for the '0x' prefix
    }

    size_t parsed_size_bytes{};
    const char* const digits_end{std::to_address(digits.end())};

    const std::from_chars_result parsed{
        std::from_chars(std::to_address(digits.begin()), digits_end,
                        parsed_size_bytes, base),
    };

    if (parsed.ec != std::errc{} or parsed.ptr != digits_end) {
        print_usage_error(
            std::format("Invalid {}: '{}' is not a number", name, text));

        return std::nullopt;
    }

    if (parsed_size_bytes == 0) {
        print_usage_error(std::format("Invalid {}: '{}' is zero", name, text));
        return std::nullopt;
    }

    if (parsed_size_bytes % alignment != 0) {
        print_usage_error(std::format(
            "Invalid {}: '{}' is not a multiple of {}", name, text, alignment));

        return std::nullopt;
    }

    return parsed_size_bytes;
}

// each '--report' replaces the earlier ones, an empty name is an error
[[nodiscard]] auto parse_reports(const std::string_view reports)
    -> std::optional<report_options> {

    // note: splitting an empty text gives no parts
    if (reports.empty()) {
        print_usage_error("Invalid --report: empty report name");
        return std::nullopt;
    }

    report_options parsed{};

    for (const auto part : reports | std::views::split(',')) {
        const std::string_view report{part};

        if (report == "registers") {
            parsed.registers = true;
        } else if (report.empty()) {
            print_usage_error("Invalid --report: empty report name");
            return std::nullopt;
        } else {
            print_usage_error(
                std::format("Invalid --report option: '{}'. Supported "
                            "reports are: registers.",
                            report));

            return std::nullopt;
        }
    }

    return parsed;
}

// each '--checks' replaces the earlier ones, empty parts are ignored
[[nodiscard]] auto parse_checks(const std::string_view checks)
    -> std::optional<check_options> {

    check_options parsed{};

    for (const auto part : checks | std::views::split(',')) {
        const std::string_view option{part};

        if (option == "upper") {
            parsed.bounds_upper = true;
        } else if (option == "lower") {
            parsed.bounds_lower = true;
        } else if (option == "line") {
            parsed.bounds_with_line = true;
        } else if (option == "frame") {
            parsed.frame = true;
        } else if (option == "alias") {
            parsed.alias = true;
        } else if (option == "noub") {
            // 'line' only changes the report, it prevents no undefined
            // behavior
            parsed.bounds_upper = true;
            parsed.bounds_lower = true;
            parsed.frame = true;
            parsed.alias = true;
        } else if (not option.empty()) {
            print_usage_error(
                std::format("Invalid --checks option: '{}'. Supported options "
                            "are: upper, lower, line, frame, alias, noub.",
                            option));

            return std::nullopt;
        }
    }

    return parsed;
}

[[nodiscard]] auto find_target(const std::string_view text)
    -> std::optional<target> {

    for (const target_name& name : target_names) {
        if (name.text == text) {
            return name.kind;
        }
    }

    return std::nullopt;
}

[[nodiscard]] auto target_text(const target kind) -> std::string_view {
    for (const target_name& name : target_names) {
        if (name.kind == kind) {
            return name.text;
        }
    }

    std::unreachable();
}

// e.g. 'x86_64, rv32i, rv32i-qemu, rv32i-fpga'
[[nodiscard]] auto supported_target_texts() -> std::string {
    std::string texts;

    for (const target_name& name : target_names) {
        if (not texts.empty()) {
            texts += ", ";
        }

        texts += name.text;
    }

    return texts;
}

// keeps the directory so the image lands next to its source
[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const target machine_target) -> std::string {

    std::filesystem::path path{src_file_name};
    path.replace_extension();
    return std::format("{}-{}.bin", path.string(), target_text(machine_target));
}

// 'os' receives the output of the parse stage
[[nodiscard]] auto make_backend(const target machine_target, std::ostream& os,
                                const std::string_view src,
                                const assembler::jump_mode jumps,
                                const size_t stack_size_bytes,
                                const std::string_view binary_file_name)
    -> std::unique_ptr<machine> {

    // todo: x86_64 writes no binary, see etc/todo.txt
    if (machine_target == target::x86_64) {
        return std::make_unique<machine_x86_64>(os, src, jumps);
    }

    if (machine_target == target::rv32i) {
        return std::make_unique<machine_rv32i>(os, src, jumps,
                                               binary_file_name);
    }

    if (machine_target == target::rv32i_qemu) {
        return std::make_unique<machine_rv32i_qemu>(
            os, src, jumps, binary_file_name, stack_size_bytes);
    }

    // the command line accepts only these four targets
    assert(machine_target == target::rv32i_fpga);

    return std::make_unique<machine_rv32i_fpga>(
        os, src, jumps, binary_file_name, stack_size_bytes);
}

// every command line error ends with the same hint
auto print_usage_error(const std::string_view message) -> void {
    std::println(stderr, "{}", message);
    std::println(stderr, "Use --help for usage information");
}

// the inlined calls the error was found in, innermost first
auto print_call_frames(
    const std::string_view src_file_name, const std::string_view src,
    const std::span<const compiler_exception::call_frame> frames) -> void {

    for (const compiler_exception::call_frame& frame : frames) {
        const auto [line_num, col]{
            line_and_col_num_for_char_index(frame.line, frame.start_index, src),
        };

        // the source line shows a call that is the whole line
        if (frame.text == trimmed_line_at(src, frame.start_index)) {
            std::println(stderr, "{}:{}:{}: {}", src_file_name, line_num, col,
                         frame.reason);
        } else {
            std::println(stderr, "{}:{}:{}: {} '{}'", src_file_name, line_num,
                         col, frame.reason, frame.text);
        }

        print_source_line(src, frame.start_index, frame.end_index);
    }
}

// the line of the character at 'index' without its surrounding whitespace
auto trimmed_line_at(const std::string_view src, const size_t index)
    -> std::string_view {

    constexpr std::string_view whitespace{" \t\r"};

    const size_t line_start{src.rfind('\n', index) + 1};
    // note: +1 because the line starts after the newline, npos + 1 is 0

    const size_t line_end{std::min(src.find('\n', index), src.size())};
    const std::string_view line{src.substr(line_start, line_end - line_start)};

    const size_t first{line.find_first_not_of(whitespace)};

    // note: the character at 'index' starts a token, its line is not blank
    assert(first != std::string_view::npos);

    return line.substr(first, line.find_last_not_of(whitespace) - first + 1);
    // note: +1 because the end position is inclusive
}

// 'line' and 'start_index' locate the error, the column is derived from them,
// line 0 is a problem of the whole file
auto print_source_error(const std::string_view src_file_name,
                        const std::string_view src, const size_t line,
                        const size_t start_index, const size_t end_index,
                        const std::string_view message) -> void {

    if (line == 0) {
        std::println(stderr, "\n{}: {}", src_file_name, message);
        return;
    }

    const auto [line_num,
                col]{line_and_col_num_for_char_index(line, start_index, src)};

    std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line_num, col,
                 message);

    print_source_line(src, start_index, end_index);
}

// the bytes after the first one of a multi-byte UTF-8 character
auto is_utf8_continuation(const char ch) -> bool {
    constexpr unsigned char first{0x80};
    constexpr unsigned char last{0xBF};

    const unsigned char byte{static_cast<unsigned char>(ch)};

    return byte >= first and byte <= last;
}

// the source line of the error with a '^' under its first character and a '~'
// under the rest of the token, a position without a token has only the '^'
auto print_source_line(const std::string_view src, const size_t start_index,
                       const size_t end_index) -> void {

    if (start_index >= src.size()) {
        return;
    }

    // note: the character at 'start_index' can be the line end
    const size_t before_start{
        start_index == 0 ? std::string_view::npos
                         : src.rfind('\n', start_index - 1),
    };

    const size_t line_bgn{
        before_start == std::string_view::npos ? 0 : before_start + 1,
    };
    // note: +1 because the line starts after the line end

    const size_t line_end{std::min(src.find('\n', start_index), src.size())};

    std::string_view line{src.substr(line_bgn, line_end - line_bgn)};

    if (line.ends_with('\r')) {
        line.remove_suffix(1);
        // note: 1 for the carriage return of a crlf line end
    }

    // an error at the end of a file has no line to show
    if (line.empty()) {
        return;
    }

    // a tab stays a tab so that the mark lines up, a character counts once
    std::string padding;
    for (const char ch : src.substr(line_bgn, start_index - line_bgn)) {
        if (ch == '\t') {
            padding += '\t';
        } else if (not is_utf8_continuation(ch)) {
            padding += ' ';
        }
    }

    const size_t mark_end{std::min(end_index, line_bgn + line.size())};
    const size_t mark_size{mark_end > start_index ? mark_end - start_index : 1};

    std::println(stderr, "{}\n{}^{}", line, padding,
                 std::string(mark_size - 1, '~'));
    // note: -1 because the caret already marks the first character
}
} // namespace
