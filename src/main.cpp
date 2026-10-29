// the driver: reads the options, makes the backend that '--target' names,
// compiles the program and writes the output, or prints the first error with
// its source line
#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "assembler.hpp"
#include "compile_error.hpp"
#include "expr_type_impl.hpp" // IWYU pragma: keep
#include "machine.hpp"
#include "machine_rv32i.hpp"
#include "machine_rv32i_fpga.hpp"
#include "machine_rv32i_linux.hpp"
#include "machine_rv32i_qemu.hpp"
#include "machine_x86_64.hpp"
#include "parse_statement.hpp" // IWYU pragma: keep
#include "program.hpp"
#include "register_report.hpp" // IWYU pragma: keep
#include "source_files.hpp"
#include "stmt_identifier_impl.hpp" // IWYU pragma: keep
#include "toc.hpp"
#include "type.hpp"

namespace {
// the sizes of '--vars', '--stack' and '--memory' are multiples of their
// alignment, the stack one keeps sp 16-byte aligned and the memory one is what
// a 'lui' loads
constexpr size_t default_vars_size_bytes{0x10000};
constexpr size_t vars_alignment{16};
constexpr size_t default_stack_size_bytes{0x10000};
constexpr size_t stack_alignment{16};
constexpr size_t default_memory_size_bytes{0x800000};
constexpr size_t memory_alignment{4096};
constexpr std::string_view default_sd_card_file_name{"sdcard.img"};
constexpr std::string_view default_src_file_name{"prog.baz"};

// a target of '--target': its name, the backend it makes, whether it writes
// a binary image beside the assembly and the options only some targets read,
// the others reject them
struct target_name {
    std::string_view text;
    std::unique_ptr<machine> (*make)(const builtin_type_set& builtins,
                                     const source_files& files,
                                     const machine::target_options& options){};
    bool writes_image{};
    bool reads_stack{};
    bool reads_memory{};
    bool reads_sd_card{};
};

// the backend 'machine_t' of a target
template <class machine_t>
[[nodiscard]] auto make_machine(const builtin_type_set& builtins,
                                const source_files& files,
                                const machine::target_options& options)
    -> std::unique_ptr<machine> {

    return std::make_unique<machine_t>(builtins, files, options);
}

// the targets of '--target', in the order of the usage text
// todo: x86_64 writes no binary, see etc/todo.txt
constexpr std::array<target_name, 4> target_names{
    target_name{
        .text{"x86_64"},
        .make{&make_machine<machine_x86_64>},
        .writes_image{},
        .reads_stack{},
        .reads_memory{},
        .reads_sd_card{true},
    },
    target_name{
        .text{"rv32i"},
        .make{&make_machine<machine_rv32i_linux>},
        .writes_image{true},
        .reads_stack{},
        .reads_memory{},
        .reads_sd_card{true},
    },
    target_name{
        .text{"rv32i-qemu"},
        .make{&make_machine<machine_rv32i_qemu>},
        .writes_image{true},
        .reads_stack{true},
        .reads_memory{},
        .reads_sd_card{},
    },
    target_name{
        .text{"rv32i-fpga"},
        .make{&make_machine<machine_rv32i_fpga>},
        .writes_image{true},
        .reads_stack{true},
        .reads_memory{true},
        .reads_sd_card{},
    },
};

struct kept_version_name {
    machine::kept_version kind{};
    std::string_view text;
};

// the values of '--versions', in the order of the usage text
constexpr std::array<kept_version_name, 3> kept_version_names{
    kept_version_name{.kind{machine::kept_version::smaller}, .text{"smaller"}},
    kept_version_name{.kind{machine::kept_version::first}, .text{"first"}},
    kept_version_name{.kind{machine::kept_version::last}, .text{"last"}},
};

// what the compiler reports after the code, besides the usual statistics
struct report_options {
    bool registers{};
};

struct report_name {
    std::string_view text;
    bool report_options::* member{};
};

// the names of '--report', in the order of the usage text
constexpr std::array<report_name, 1> report_names{
    report_name{.text{"registers"}, .member{&report_options::registers}},
};

struct options {
    // the default is 'default_src_file_name'
    std::optional<std::string_view> src_file_name;
    // a row of 'target_names'; never null
    const target_name* target{&target_names.front()};
    size_t vars_size_bytes{default_vars_size_bytes};
    // the options only some targets read are empty until given
    std::optional<size_t> stack_size_bytes;
    std::optional<size_t> memory_size_bytes;
    check_options checks{};
    bool optimize_jumps{true};
    bool reproduce_source{};
    report_options reports{};
    machine::kept_version kept_version{machine::kept_version::smaller};
    // the default depends on the source and target
    std::optional<std::string_view> binary_file_name;
    // read and written by the sector functions on x86_64 and rv32i
    std::optional<std::string_view> sd_card_file_name;
};

// an option with a value, e.g. '--vars=0x40000'
struct value_option {
    std::string_view name;
    void (*apply)(std::string_view value, options& opts){};
};

// the exit code when the program should stop instead of compiling
[[nodiscard]] auto parse_options(const std::span<const char*> args,
                                 options& opts) -> std::optional<int>;

// the option functions throw 'option_error'
auto check_target_reads(const options& opts) -> void;

auto check_default_stack_fits(const options& opts) -> void;

auto parse_option(const char* const argument, options& opts) -> void;

// the appliers of the options with a value store the value in 'opts'
auto apply_vars(std::string_view value, options& opts) -> void;
auto apply_stack(std::string_view value, options& opts) -> void;
auto apply_memory(std::string_view value, options& opts) -> void;
auto apply_target(std::string_view value, options& opts) -> void;
auto apply_checks(std::string_view value, options& opts) -> void;
auto apply_report(std::string_view value, options& opts) -> void;
auto apply_bin(std::string_view value, options& opts) -> void;
auto apply_sdcard(std::string_view value, options& opts) -> void;
auto apply_versions(std::string_view value, options& opts) -> void;

auto print_help(const char* const program_name) -> void;

[[nodiscard]] auto compile_file(const options& opts) -> int;

auto print_compiler_error(const source_files& files, const compile_error& error)
    -> void;

auto check_reproduced_source(const program& prg, const source_files& files)
    -> void;

[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view option,
                                    const size_t alignment) -> size_t;

[[nodiscard]] auto parse_checks(const std::string_view checks) -> check_options;

[[nodiscard]] auto parse_reports(const std::string_view reports)
    -> report_options;

[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const target_name& target) -> std::string;

// 'path' names one of the files, also through another path
[[nodiscard]] auto is_source_file(const std::string_view path,
                                  const source_files& files) -> bool;

auto print_option_error(const std::string_view message) -> void;

auto print_unlocated_error(const std::string_view message) -> void;

auto print_call_frames(const source_files& files,
                       std::span<const compile_error::call_frame> frames)
    -> void;

[[nodiscard]] auto trimmed_line_at(std::string_view src, size_t index)
    -> std::string_view;

auto print_source_error(const source_files& files,
                        const std::optional<source_span>& where,
                        std::string_view message) -> void;

auto print_source_line(std::string_view src, size_t start_index,
                       size_t end_index) -> void;

[[nodiscard]] auto is_utf8_continuation(char ch) -> bool;

[[nodiscard]] auto is_escaped_in_line(char ch) -> bool;

[[nodiscard]] auto printable_line(std::string_view line) -> std::string;

[[nodiscard]] auto display_columns(std::string_view text) -> size_t;

[[nodiscard]] auto aligned_fill(std::string_view text, char fill)
    -> std::string;
} // namespace

auto main(const int argc, const char** const argv) -> int {
    // a counted view of a pointer is a span that the compiler accepts as safe
    const std::span<const char*> args{std::views::counted(argv, argc)};

    options opts{};

    if (const std::optional<int> exit_code{parse_options(args, opts)}) {
        return *exit_code;
    }

    return compile_file(opts);
}

namespace {
[[nodiscard]] auto parse_options(const std::span<const char*> args,
                                 options& opts) -> std::optional<int> {

    try {
        for (const char* const argument : args | std::views::drop(1)) {
            const std::string_view arg{argument};

            if (arg == "--help" or arg == "-h") {
                print_help(args.at(0));
                return 0;
            }

            parse_option(argument, opts);
        }

        // note: after the loop because '--target' may follow the options
        check_target_reads(opts);
        check_default_stack_fits(opts);
    } catch (const option_error& e) {
        print_option_error(e.what());
        return 1;
    }

    return std::nullopt;
}

// the backend checks the stack against the memory without knowing which was
// given, a default stack that does not fit is the memory's error
auto check_default_stack_fits(const options& opts) -> void {
    if (not opts.memory_size_bytes or opts.stack_size_bytes or
        *opts.memory_size_bytes >= default_stack_size_bytes) {

        return;
    }

    throw option_error{
        std::format("invalid --memory: {} B is smaller than the default stack "
                    "of {} B, give a smaller --stack",
                    *opts.memory_size_bytes, default_stack_size_bytes)};
}

// an option of the target rows, e.g. '--stack' and 'reads_stack'
struct target_option {
    std::string_view name;
    bool target_name::* reads{};
    bool given{};
};

auto check_target_reads(const options& opts) -> void {
    const std::array<target_option, 3> target_options{
        target_option{
            .name{"--stack"},
            .reads{&target_name::reads_stack},
            .given{opts.stack_size_bytes.has_value()},
        },
        target_option{
            .name{"--memory"},
            .reads{&target_name::reads_memory},
            .given{opts.memory_size_bytes.has_value()},
        },
        target_option{
            .name{"--sdcard"},
            .reads{&target_name::reads_sd_card},
            .given{opts.sd_card_file_name.has_value()},
        },
    };

    const target_name& chosen{*opts.target};

    for (const target_option& option : target_options) {
        if (not option.given or chosen.*option.reads) {
            continue;
        }

        std::string readers;

        for (const target_name& name : target_names) {
            if (not(name.*option.reads)) {
                continue;
            }

            if (not readers.empty()) {
                readers += ", ";
            }

            readers += name.text;
        }

        throw option_error{
            std::format("invalid {}: target {} does not read it, the targets "
                        "that do are: {}",
                        option.name, in_quotes(chosen.text), readers)};
    }
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

// the 'text' of each row of a table joined by ', ', e.g. 'upper, lower, line'
template <typename Name, size_t count>
[[nodiscard]] auto joined_texts(const std::array<Name, count>& names)
    -> std::string {

    std::string texts;

    for (const Name& name : names) {
        if (not texts.empty()) {
            texts += ", ";
        }

        texts += name.text;
    }

    return texts;
}

auto parse_option(const char* const argument, options& opts) -> void {
    constexpr std::array<value_option, 9> value_options{
        value_option{.name{"--vars="}, .apply{apply_vars}},
        value_option{.name{"--stack="}, .apply{apply_stack}},
        value_option{.name{"--memory="}, .apply{apply_memory}},
        value_option{.name{"--target="}, .apply{apply_target}},
        value_option{.name{"--checks="}, .apply{apply_checks}},
        value_option{.name{"--report="}, .apply{apply_report}},
        value_option{.name{"--bin="}, .apply{apply_bin}},
        value_option{.name{"--sdcard="}, .apply{apply_sdcard}},
        value_option{.name{"--versions="}, .apply{apply_versions}},
    };

    const std::string_view arg{argument};

    for (const value_option& option : value_options) {
        if (const std::optional<std::string_view> value{
                option_value(arg, option.name),
            }) {

            option.apply(*value, opts);
            return;
        }
    }

    if (arg == "--nopt") {
        opts.optimize_jumps = false;
        return;
    }

    if (arg == "--reproduce-source") {
        opts.reproduce_source = true;
        return;
    }

    if (arg.starts_with('-')) {
        throw option_error{std::format("unknown option: {}", in_quotes(arg))};
    }

    if (opts.src_file_name) {
        throw option_error{std::format("more than one source file: {} and {}",
                                       in_quotes(*opts.src_file_name),
                                       in_quotes(arg))};
    }

    opts.src_file_name = arg;
}

auto apply_vars(const std::string_view value, options& opts) -> void {
    opts.vars_size_bytes = parse_size_bytes(value, "--vars", vars_alignment);
}

auto apply_stack(const std::string_view value, options& opts) -> void {
    opts.stack_size_bytes = parse_size_bytes(value, "--stack", stack_alignment);
}

auto apply_memory(const std::string_view value, options& opts) -> void {
    opts.memory_size_bytes =
        parse_size_bytes(value, "--memory", memory_alignment);
}

auto apply_target(const std::string_view value, options& opts) -> void {
    const auto* const found{
        std::ranges::find(target_names, value, &target_name::text),
    };

    if (found == target_names.end()) {
        throw option_error{
            std::format("invalid --target: {}, supported targets are: {}",
                        in_quotes(value), joined_texts(target_names))};
    }

    opts.target = found;
}

auto apply_checks(const std::string_view value, options& opts) -> void {
    opts.checks = parse_checks(value);
}

auto apply_report(const std::string_view value, options& opts) -> void {
    opts.reports = parse_reports(value);
}

// 'option' is named in the error message
[[nodiscard]] auto parse_file_name(const std::string_view text,
                                   const std::string_view option)
    -> std::string_view {

    if (text.empty()) {
        throw option_error{std::format("invalid {}: empty file name", option)};
    }

    return text;
}

auto apply_bin(const std::string_view value, options& opts) -> void {
    opts.binary_file_name = parse_file_name(value, "--bin");
}

auto apply_sdcard(const std::string_view value, options& opts) -> void {
    opts.sd_card_file_name = parse_file_name(value, "--sdcard");
}

auto apply_versions(const std::string_view value, options& opts) -> void {
    const auto* const found{
        std::ranges::find(kept_version_names, value, &kept_version_name::text),
    };

    if (found == kept_version_names.end()) {
        throw option_error{
            std::format("invalid --versions: {}, supported values are: {}",
                        in_quotes(value), joined_texts(kept_version_names))};
    }

    opts.kept_version = found->kind;
}

// the text of '--help', which the readme usage section pastes; the format
// arguments are listed in 'print_help'
constexpr std::string_view usage_format{
    R"(usage: {0} [options] [file]
compiles file (default: prog.baz) to assembly on stdout, rv32i targets also
write a binary image

options:
  --target=MACHINE    x86_64 (default): linux, nasm
                      rv32i: linux, llvm assembler, qemu user mode
                      rv32i-qemu: bare-metal image for the qemu virt machine,
                        fails when code, data, variables and stack exceed its
                        128 MiB
                      rv32i-fpga: bare-metal image for the fpga soft core,
                        stack grows down from the end of the memory (see
                        --memory), fails when code, data, variables and stack
                        do not fit
  --vars=SIZE         variable storage in bytes, decimal or 0x hex, must be a
                      multiple of {1} (default: {2})
  --stack=SIZE        rv32i-qemu and rv32i-fpga stack in bytes, decimal or 0x
                      hex, must be a multiple of {3}, at least {7} and at
                      most the memory of the target (default: {4})
  --memory=SIZE       rv32i-fpga memory in bytes, decimal or 0x hex, must be a
                      multiple of {5} (default: {6}), the emulator is
                      built for the default
  --checks=LIST       comma separated checks, replaces earlier --checks
  --report=LIST       comma separated reports after the code, replaces earlier
                      --report
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --sdcard=FILE       x86_64 and rv32i sd card image that 'sector_read' and
                      'sector_write' open at run time, relative to the working
                      directory of the program (default: {8})
  --nopt              no jump optimizations
  --versions=KEPT     of two versions of the same code keep the smaller
                      (default), or for testing the first or the last
  --reproduce-source  check that the source written back from the parse
                      matches the input, else write it to diff.baz
  --help, -h          this help

reports:
  registers  how the scratch registers are used at the busiest point: what
             each call frame holds, what a noinline frame would save and what
             each callee holds

checks:
  upper    runtime upper array bounds only, a negative index passes
  lower    runtime lower array bounds, catches negative indexes
  line     report line number on failed bounds, division, shift, overlap or
           overflow check
  frame    runtime non-inlined function frame capacity
  division runtime division by zero and 'MIN / -1', a constant 'MIN / -1' is
           rejected at compile time
  shift    runtime shift count below zero or not below the width of the type
  overlap  runtime 'array_copy' whose destination starts inside the source
  stack    runtime stack capacity at non-inlined calls on rv32i-qemu and
           rv32i-fpga, the other targets have an operating system that stops
           a program that overflows the stack
  overflow runtime signed overflow of '+', '-', '*' and unary '-', a constant
           expression that overflows is rejected at compile time
  noub     all checks against undefined behavior: upper, lower, frame,
           division, shift, overlap, stack and overflow; a program accepted
           with 'noub' has no undefined behavior, by default none is on;
           arguments, a result and globals that may share storage are
           rejected at compile time always
  -NAME    turns a check off after the others are applied, e.g.
           noub,-division, +NAME is NAME

examples:
  {0} prog.baz > prog.s
  {0} --vars=0x40000 --checks=upper prog.baz > prog.s
  {0} --checks=upper,lower,line,frame prog.baz > prog.s
  {0} --target=rv32i-qemu --stack=0x20000 prog.baz > prog.s
  {0} --target=rv32i-fpga --checks=upper,lower prog.baz > prog.s
  {0} --target=rv32i-fpga --memory=0x100000 prog.baz > prog.s
  {0} --target=rv32i-qemu --bin=image.bin prog.baz > prog.s
)"};

auto print_help(const char* const program_name) -> void {
    std::print(usage_format, program_name, vars_alignment,
               default_vars_size_bytes, stack_alignment,
               default_stack_size_bytes, memory_alignment,
               default_memory_size_bytes, machine_rv32i::leaf_stack_bytes(),
               default_sd_card_file_name);
}

[[nodiscard]] auto compile_file(const options& opts) -> int {
    source_files files;

    // the machine and the program refer to them
    const builtin_type_set builtins;

    try {
        const std::string_view src_file_name{
            opts.src_file_name.value_or(default_src_file_name),
        };

        files.add(std::string{src_file_name},
                  source_files::read_file(src_file_name));

        const std::string binary{
            opts.binary_file_name
                ? std::string{*opts.binary_file_name}
                : default_binary_file_name(src_file_name, *opts.target),
        };

        const machine::target_options target_opts{
            .jumps{
                opts.optimize_jumps ? assembler::jump_mode::optimized
                                    : assembler::jump_mode::as_emitted,
            },
            .kept{opts.kept_version},
            .register_report{opts.reports.registers},
            .sd_card_file_name{
                opts.sd_card_file_name.value_or(default_sd_card_file_name),
            },
            .binary_file_name{binary},
            .stack_size_bytes{
                opts.stack_size_bytes.value_or(default_stack_size_bytes),
            },
            .memory_size_bytes{
                opts.memory_size_bytes.value_or(default_memory_size_bytes),
            },
            .reports_line{opts.checks.reports_line},
        };

        const std::unique_ptr<machine> backend{
            opts.target->make(builtins, files, target_opts),
        };

        // the checks the target cannot honor are off, e.g. x86_64 has no
        // stack bound
        check_options checks{opts.checks};
        checks.stack = checks.stack and backend->is_stack_bounded();

        program prg{*backend, builtins, files, opts.vars_size_bytes, checks};

        // the image would replace a source file, the parse read the includes
        if (opts.target->writes_image and is_source_file(binary, files)) {
            throw option_error{std::format("invalid --bin: {} is a source file",
                                           in_quotes(binary))};
        }

        if (opts.reproduce_source) {
            check_reproduced_source(prg, files);
        }

        prg.build(std::cout);

        // e.g. a full disk, the listing is incomplete
        std::cout.flush();

        if (not std::cout) {
            throw unlocated_error{"cannot write the assembly to stdout"};
        }
    } catch (const compile_error& e) {
        print_compiler_error(files, e);
        return 1;
    } catch (const unlocated_error& e) {
        print_unlocated_error(e.what());
        return 1;
    } catch (const option_error& e) {
        print_option_error(e.what());
        return 1;
    } catch (const std::exception& e) {
        // a defect of the compiler, the input is not at fault
        std::println(stderr, "\ninternal error: {}", e.what());
        return 1;
    }

    return 0;
}

// the message of the error at its source line, the inlined calls and generic
// instances it was found through and what else is known about it
auto print_compiler_error(const source_files& files, const compile_error& error)
    -> void {

    print_source_error(files, error.where, error.msg);

    print_call_frames(files, error.call_frames);

    if (not error.detail.empty()) {
        std::println(stderr, "\n{}", error.detail);
    }
}

// the source written back from the parsed program equals the input, file by
// file; a difference is a defect of the compiler, written to 'diff.baz'
auto check_reproduced_source(const program& prg, const source_files& files)
    -> void {

    for (size_t file_ix{}; file_ix < files.count(); ++file_ix) {
        std::ostringstream reproduced_source;
        prg.source_to(file_ix, reproduced_source);

        if (files.text(file_ix) == reproduced_source.view()) {
            continue;
        }

        std::ofstream diff_file{"diff.baz"};
        diff_file << reproduced_source.view();

        throw std::logic_error{
            std::format("generated source differs, see 'diff {} diff.baz'",
                        files.name(file_ix))};
    }
}

// decimal or 0x hexadecimal, 'option' is named in the error messages
[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view option,
                                    const size_t alignment) -> size_t {

    constexpr int decimal_base{10};
    constexpr int hex_base{16};

    std::string_view digits{text};
    int base{decimal_base};

    if (digits.starts_with("0x") or digits.starts_with("0X")) {
        base = hex_base;
        digits.remove_prefix(std::string_view{"0x"}.size());
    }

    size_t parsed_size_bytes{};
    const char* const digits_end{std::to_address(digits.end())};

    const std::from_chars_result parsed{
        std::from_chars(std::to_address(digits.begin()), digits_end,
                        parsed_size_bytes, base),
    };

    if (parsed.ec == std::errc::result_out_of_range) {
        throw option_error{std::format("invalid {}: {} is out of range", option,
                                       in_quotes(text))};
    }

    if (parsed.ec != std::errc{} or parsed.ptr != digits_end) {
        throw option_error{std::format("invalid {}: {} is not a number", option,
                                       in_quotes(text))};
    }

    if (parsed_size_bytes == 0) {
        throw option_error{
            std::format("invalid {}: {} is zero", option, in_quotes(text))};
    }

    if (parsed_size_bytes % alignment != 0) {
        throw option_error{std::format("invalid {}: {} is not a multiple of {}",
                                       option, in_quotes(text), alignment)};
    }

    return parsed_size_bytes;
}

// each '--report' replaces the earlier ones, empty parts are ignored like
// those of '--checks', so '--report=' turns the reports off
[[nodiscard]] auto parse_reports(const std::string_view reports)
    -> report_options {

    report_options parsed{};

    for (const auto part : reports | std::views::split(',')) {
        const std::string_view report{part};

        if (report.empty()) {
            continue;
        }

        const auto* const found{
            std::ranges::find(report_names, report, &report_name::text),
        };

        if (found == report_names.end()) {
            throw option_error{
                std::format("invalid --report: {}, supported reports are: {}",
                            in_quotes(report), joined_texts(report_names))};
        }

        parsed.*found->member = true;
    }

    return parsed;
}

// one named check, 'is_ub' tells that 'noub' includes it
struct check_name {
    std::string_view text;
    bool check_options::* member{};
    bool is_ub{};
};

constexpr std::array<check_name, 9> check_names{
    {
        {.text{"upper"}, .member{&check_options::bounds_upper}, .is_ub{true}},
        {.text{"lower"}, .member{&check_options::bounds_lower}, .is_ub{true}},
        {.text{"line"}, .member{&check_options::reports_line}, .is_ub{false}},
        {.text{"frame"}, .member{&check_options::frame}, .is_ub{true}},
        {.text{"division"}, .member{&check_options::division}, .is_ub{true}},
        {.text{"shift"}, .member{&check_options::shift}, .is_ub{true}},
        {.text{"overlap"}, .member{&check_options::overlap}, .is_ub{true}},
        {.text{"stack"}, .member{&check_options::stack}, .is_ub{true}},
        {.text{"overflow"}, .member{&check_options::overflow}, .is_ub{true}},
    },
};

// the help lists every check as a line of its own, and its 'noub' entry
// names every check that 'noub' turns on, so the text follows the table
static_assert([] -> bool {
    const std::string_view checks{
        usage_format.substr(usage_format.find("\nchecks:\n")),
    };

    const std::string_view noub{
        checks.substr(checks.find("\n  noub "),
                      checks.find("\n  -NAME") - checks.find("\n  noub ")),
    };

    return std::ranges::all_of(
        check_names, [&](const check_name& name) -> bool {
            const std::string line_start{
                std::string{"\n  "} + std::string{name.text} + " ",
            };

            return checks.contains(line_start) and
                   (not name.is_ub or noub.contains(name.text));
        });
}());

// each '--checks' replaces the earlier ones, empty parts are ignored, 'name'
// and '+name' enable a check, '-name' disables it after all enables, so the
// order in the list does not matter
[[nodiscard]] auto parse_checks(const std::string_view checks)
    -> check_options {

    check_options parsed{};
    std::vector<bool check_options::*> disabled;

    for (const auto part : checks | std::views::split(',')) {
        std::string_view option{part};

        if (option.empty()) {
            continue;
        }

        const std::string_view written{option};
        const bool is_disable{option.starts_with('-')};

        if (is_disable or option.starts_with('+')) {
            option.remove_prefix(1);
            // note: 1 because the sign is not part of the name
        }

        // the checks the option names, e.g. '-noub' disables every check
        // that 'noub' enables
        std::vector<bool check_options::*> members;

        if (option == "noub") {
            for (const check_name& name : check_names) {
                if (name.is_ub) {
                    members.push_back(name.member);
                }
            }
        } else {
            const auto* const found{
                std::ranges::find(check_names, option, &check_name::text),
            };

            if (found == check_names.end()) {
                throw option_error{std::format(
                    "invalid --checks: {}, supported checks are: {}, "
                    "noub, each with a '+' or '-' prefix",
                    in_quotes(written), joined_texts(check_names))};
            }

            members.push_back(found->member);
        }

        if (is_disable) {
            disabled.append_range(members);
        } else {
            for (bool check_options::* const member : members) {
                parsed.*member = true;
            }
        }
    }

    for (bool check_options::* const member : disabled) {
        parsed.*member = false;
    }

    return parsed;
}

// keeps the directory so the image lands next to its source
[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const target_name& target) -> std::string {

    std::filesystem::path path{src_file_name};
    path.replace_extension();
    return std::format("{}-{}.bin", path.string(), target.text);
}

[[nodiscard]] auto is_source_file(const std::string_view path,
                                  const source_files& files) -> bool {

    for (size_t i{}; i < files.count(); ++i) {
        std::error_code ec;

        if (std::filesystem::equivalent(path, files.name(i), ec)) {
            return true;
        }
    }

    return false;
}

// every command line error ends with the same hint
auto print_option_error(const std::string_view message) -> void {
    print_unlocated_error(message);
    std::println(stderr, "use --help for usage information");
}

// a user error without a place in the source, e.g. a file that cannot be read
auto print_unlocated_error(const std::string_view message) -> void {
    std::println(stderr, "error: {}", printable(message));
}

// the inlined calls and generic instances the error was found through,
// innermost first
auto print_call_frames(const source_files& files,
                       const std::span<const compile_error::call_frame> frames)
    -> void {

    const source_locations locations{files};

    for (const compile_error::call_frame& frame : frames) {
        const source_span& where{frame.where};
        const std::string_view src{files.text(where.file_ix)};

        // the source line shows a call that is the whole line
        if (frame.text == trimmed_line_at(src, where.start_index)) {
            std::println(stderr, "{}: {}", locations.for_error(where),
                         frame.reason);
        } else {
            std::println(stderr, "{}: {} {}", locations.for_error(where),
                         frame.reason, in_quotes(printable(frame.text)));
        }

        print_source_line(src, where.start_index, where.end_index);
    }
}

// the line of the character at 'index' without its surrounding whitespace
[[nodiscard]] auto trimmed_line_at(const std::string_view src,
                                   const size_t index) -> std::string_view {

    constexpr std::string_view whitespace{" \t\r"};

    const std::string_view line{line_at(src, index).text};

    const size_t first{line.find_first_not_of(whitespace)};

    // note: the character at 'index' starts a token, its line is not blank
    assert(first != std::string_view::npos);

    return line.substr(first, line.find_last_not_of(whitespace) - first + 1);
}

// an error without a place is a problem of the whole program, reported at the
// main file
auto print_source_error(const source_files& files,
                        const std::optional<source_span>& where,
                        const std::string_view message) -> void {

    if (not where) {
        std::println(stderr, "\n{}: {}", printable(files.name(0)),
                     printable(message));

        return;
    }

    const source_locations locations{files};

    std::println(stderr, "\n{}: {}", locations.for_error(*where),
                 printable(message));

    print_source_line(files.text(where->file_ix), where->start_index,
                      where->end_index);
}

// the bytes after the first one of a multi-byte UTF-8 character
[[nodiscard]] auto is_utf8_continuation(const char ch) -> bool {
    constexpr unsigned char first{0x80};
    constexpr unsigned char last{0xBF};

    const unsigned char byte{static_cast<unsigned char>(ch)};

    return byte >= first and byte <= last;
}

// the columns a terminal shows: a continuation byte after a lead or
// continuation byte is part of that character, a lone one is shown as a
// replacement character, a control byte as its escape
[[nodiscard]] auto display_columns(const std::string_view text) -> size_t {
    constexpr unsigned char first_non_ascii{0x80};
    constexpr size_t escape_columns{4};
    // note: 4 for the '\xNN' that 'printable_line' shows

    size_t columns{};
    bool after_non_ascii{};

    for (const char ch : text) {
        if (is_escaped_in_line(ch)) {
            columns += escape_columns;
        } else if (not after_non_ascii or not is_utf8_continuation(ch)) {
            ++columns;
        }

        after_non_ascii = static_cast<unsigned char>(ch) >= first_non_ascii;
    }

    return columns;
}

// a tab stays in a shown line so that the mark lines up, the other control
// bytes are escaped
[[nodiscard]] auto is_escaped_in_line(const char ch) -> bool {
    return is_control_char(ch) and ch != '\t';
}

// the line as 'printable' shows it, with its tabs
[[nodiscard]] auto printable_line(const std::string_view line) -> std::string {
    std::string result;

    for (const char ch : line) {
        if (is_escaped_in_line(ch)) {
            result += printable(std::string_view{&ch, 1});
            continue;
        }

        result.push_back(ch);
    }

    return result;
}

// the source line of the error with a '^' under its first character and a '~'
// under the rest of the token, a position without a token has only the '^'
auto print_source_line(const std::string_view src, const size_t start_index,
                       const size_t end_index) -> void {

    // note: the end of a source without a final line end is on its last line
    if (start_index > src.size()) {
        return;
    }

    const source_line located{line_at(src, start_index)};
    const size_t line_bgn{located.begin};
    std::string_view line{located.text};

    if (line.ends_with('\r')) {
        line.remove_suffix(1);
        // note: 1 for the carriage return of a crlf line end
    }

    // e.g. an error after an empty last line
    if (line.empty()) {
        return;
    }

    const std::string padding{
        aligned_fill(src.substr(line_bgn, start_index - line_bgn), ' '),
    };

    const size_t mark_end{std::min(end_index, line_bgn + line.size())};

    std::string mark{
        mark_end > start_index
            ? aligned_fill(src.substr(start_index, mark_end - start_index), '~')
            : "",
    };

    // a position without a token still has the caret
    if (mark.empty()) {
        mark = "^";
    } else {
        mark.front() = '^';
    }

    std::println(stderr, "{}\n{}{}", printable_line(line), padding, mark);
}

// 'fill' under each column of 'text' as a terminal shows it, a tab stays a
// tab so that what follows lines up
[[nodiscard]] auto aligned_fill(std::string_view text, const char fill)
    -> std::string {

    std::string result;

    for (size_t tab{text.find('\t')}; tab != std::string_view::npos;
         tab = text.find('\t')) {

        result.append(display_columns(text.substr(0, tab)), fill);
        result += '\t';
        text.remove_prefix(tab + 1);
    }

    result.append(display_columns(text), fill);

    return result;
}
} // namespace
