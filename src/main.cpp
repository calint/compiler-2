#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
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
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "compile_error.hpp"
#include "decouple_impl.hpp"  // IWYU pragma: keep
#include "expr_type_impl.hpp" // IWYU pragma: keep
#include "machine.hpp"
#include "machine_rv32i.hpp"
#include "machine_rv32i_fpga.hpp"
#include "machine_rv32i_qemu.hpp"
#include "machine_x86_64.hpp"
#include "program.hpp"
#include "source_files.hpp"
#include "toc.hpp"

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
// the uart and the other devices follow the memory at the end of the address
// space
constexpr size_t max_memory_size_bytes{0xfffff000};

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

struct kept_version_name {
    machine::kept_version kind;
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

struct options {
    const char* src_file_name{"prog.baz"};
    bool src_file_given{};
    target machine_target{target::x86_64};
    size_t vars_size_bytes{default_vars_size_bytes};
    size_t stack_size_bytes{default_stack_size_bytes};
    size_t memory_size_bytes{default_memory_size_bytes};
    check_options checks;
    bool optimize_jumps{true};
    bool reproduce_source{};
    report_options reports;
    machine::kept_version kept_version{machine::kept_version::smaller};
    // empty until given, the default depends on the source and target
    std::string_view binary_file_name;
    // read and written by the sector functions on x86_64 and rv32i
    std::string_view sd_card_file_name{"sdcard.img"};
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
[[nodiscard]] auto apply_memory(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_target(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_checks(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_report(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_bin(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_sdcard(std::string_view value, options& opts) -> bool;
[[nodiscard]] auto apply_versions(std::string_view value, options& opts)
    -> bool;

auto print_help(const char* const program_name) -> void;

[[nodiscard]] auto compile_file(const options& opts) -> int;

auto print_compiler_error(const source_files& files, const compile_error& e)
    -> void;

auto check_reproduced_source(const program& prg, const source_files& files)
    -> void;

[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view option,
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

// 'path' names one of the files, also through another path
[[nodiscard]] auto is_source_file(const std::string_view path,
                                  const source_files& files) -> bool;

[[nodiscard]] auto
make_backend(const target machine_target, const source_files* const files,
             const assembler::jump_mode jumps, const size_t stack_size_bytes,
             const size_t memory_size_bytes,
             const std::string_view binary_file_name)
    -> std::unique_ptr<machine>;

auto print_option_error(const std::string_view message) -> void;

auto print_unlocated_error(const std::string_view message) -> void;

auto print_call_frames(const source_files& files,
                       std::span<const compile_error::call_frame> frames)
    -> void;

// a line of the source, 'begin' is the index of its first character
struct source_line {
    size_t begin;
    std::string_view text;
};

[[nodiscard]] auto line_at(std::string_view src, size_t index) -> source_line;

auto trimmed_line_at(std::string_view src, size_t index) -> std::string_view;

auto print_source_error(const source_files& files, const size_t file_ix,
                        const size_t line, const size_t start_index,
                        const size_t end_index, const std::string_view message)
    -> void;

auto print_source_line(std::string_view src, size_t start_index,
                       size_t end_index) -> void;

[[nodiscard]] auto is_utf8_continuation(char ch) -> bool;

[[nodiscard]] auto display_columns(std::string_view text) -> size_t;
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

[[nodiscard]] auto parse_option(const char* const argument, options& opts)
    -> bool {

    constexpr std::array value_options{
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

    if (arg.starts_with('-')) {
        print_option_error(std::format("unknown option: '{}'", arg));
        return false;
    }

    if (opts.src_file_given) {
        print_option_error(
            std::format("more than one source file: '{}' and '{}'",
                        opts.src_file_name, arg));

        return false;
    }

    opts.src_file_name = argument;
    opts.src_file_given = true;

    return true;
}

[[nodiscard]] auto apply_vars(const std::string_view value, options& opts)
    -> bool {

    return store_parsed(parse_size_bytes(value, "--vars", vars_alignment),
                        opts.vars_size_bytes);
}

[[nodiscard]] auto apply_stack(const std::string_view value, options& opts)
    -> bool {

    return store_parsed(parse_size_bytes(value, "--stack", stack_alignment),
                        opts.stack_size_bytes);
}

[[nodiscard]] auto apply_memory(const std::string_view value, options& opts)
    -> bool {

    const std::optional<size_t> parsed{
        parse_size_bytes(value, "--memory", memory_alignment),
    };

    if (not parsed) {
        return false;
    }

    if (*parsed > max_memory_size_bytes) {
        print_option_error(
            std::format("invalid --memory: '{}', must be from {} to {} bytes",
                        value, memory_alignment, max_memory_size_bytes));

        return false;
    }

    opts.memory_size_bytes = *parsed;

    return true;
}

[[nodiscard]] auto apply_target(const std::string_view value, options& opts)
    -> bool {

    const std::optional<target> found{find_target(value)};

    if (not found) {
        print_option_error(
            std::format("invalid --target: '{}', supported targets are: {}",
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
        print_option_error("invalid --bin: empty file name");
        return false;
    }

    return true;
}

[[nodiscard]] auto apply_sdcard(const std::string_view value, options& opts)
    -> bool {

    opts.sd_card_file_name = value;

    if (opts.sd_card_file_name.empty()) {
        print_option_error("invalid --sdcard: empty file name");
        return false;
    }

    return true;
}

[[nodiscard]] auto apply_versions(const std::string_view value, options& opts)
    -> bool {

    for (const kept_version_name& name : kept_version_names) {
        if (name.text == value) {
            opts.kept_version = name.kind;
            return true;
        }
    }

    print_option_error(
        std::format("invalid --versions: '{}', supported values are: {}", value,
                    joined_texts(kept_version_names)));

    return false;
}

// same layout as the readme usage section which pastes this output
auto print_help(const char* const program_name) -> void {
    std::print(R"(usage: {0} [options] [file]
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
                      directory of the program (default: sdcard.img)
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
           overflow check, rv32i-fpga ignores it
  frame    runtime non-inlined function frame capacity
  alias    compile time rejection of calls where a result may share storage
           with an argument, on by default
  division runtime division by zero and 'MIN / -1'
  shift    runtime shift count below zero or not below the width of the type
  overlap  runtime 'array_copy' whose destination starts inside the source
  stack    runtime stack capacity at non-inlined calls on rv32i-qemu and
           rv32i-fpga, the other targets have an operating system that stops
           a program that overflows the stack
  overflow runtime signed overflow of '+', '-', '*' and unary '-', a constant
           expression that overflows is rejected at compile time
  noub     all checks against undefined behavior: upper, lower, frame, alias,
           division, shift, overlap, stack and overflow
  -NAME    turns a check off after the others are applied, e.g.
           noub,-division or -alias (also when noub is given), +NAME is NAME

examples:
  {0} prog.baz > prog.s
  {0} --vars=0x40000 --checks=upper prog.baz > prog.s
  {0} --checks=upper,lower,line,frame prog.baz > prog.s
  {0} --target=rv32i-qemu --stack=0x20000 prog.baz > prog.s
  {0} --target=rv32i-fpga --checks=upper,lower prog.baz > prog.s
  {0} --target=rv32i-fpga --memory=0x100000 prog.baz > prog.s
  {0} --target=rv32i-qemu --bin=image.bin prog.baz > prog.s
)",
               program_name, vars_alignment, default_vars_size_bytes,
               stack_alignment, default_stack_size_bytes, memory_alignment,
               default_memory_size_bytes, machine_rv32i::leaf_stack_bytes());
}

[[nodiscard]] auto compile_file(const options& opts) -> int {
    source_files files;

    try {
        files.add(opts.src_file_name,
                  source_files::read_file(opts.src_file_name));

        const assembler::jump_mode jumps{
            opts.optimize_jumps ? assembler::jump_mode::optimized
                                : assembler::jump_mode::resolved,
        };

        const std::string binary{
            opts.binary_file_name.empty()
                ? default_binary_file_name(opts.src_file_name,
                                           opts.machine_target)
                : std::string{opts.binary_file_name},
        };

        const std::unique_ptr<machine> backend{
            make_backend(opts.machine_target, &files, jumps,
                         opts.stack_size_bytes, opts.memory_size_bytes, binary),
        };

        // the option is wrong, not the source
        if (opts.vars_size_bytes > backend->max_storage_bytes()) {
            print_option_error(std::format(
                "invalid --vars: {} B exceeds the {} B that the target "
                "addresses",
                opts.vars_size_bytes, backend->max_storage_bytes()));

            return 1;
        }

        if (opts.reports.registers) {
            backend->enable_register_report();
        }

        backend->set_sd_card_file_name(opts.sd_card_file_name);
        backend->set_kept_version(opts.kept_version);

        // e.g. 'rv32i-fpga' exits with the code of the failure only
        check_options checks{opts.checks};
        checks.with_line = checks.with_line and backend->reports_line();

        program prg{*backend, files, opts.vars_size_bytes, checks};

        // the image would replace a source file, the parse read the includes;
        // x86_64 writes no image
        if (opts.machine_target != target::x86_64 and
            is_source_file(binary, files)) {

            print_option_error(
                std::format("invalid --bin: '{}' is a source file", binary));

            return 1;
        }

        if (opts.reproduce_source) {
            check_reproduced_source(prg, files);
        }

        prg.build(std::cout);
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

// the message of the error at its source line, the inlined calls it was found
// in and what else is known about it
auto print_compiler_error(const source_files& files, const compile_error& e)
    -> void {

    print_source_error(files, e.file_ix, e.line, e.start_index, e.end_index,
                       e.msg);

    print_call_frames(files, e.call_frames);

    if (not e.detail.empty()) {
        std::println(stderr, "\n{}", e.detail);
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

        throw std::logic_error{std::format(
            "generated source differs. diff {} diff.baz", files.name(file_ix))};
    }
}

// decimal or 0x hexadecimal, 'option' is named in the error messages
[[nodiscard]] auto parse_size_bytes(const std::string_view text,
                                    const std::string_view option,
                                    const size_t alignment)
    -> std::optional<size_t> {

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
        print_option_error(
            std::format("invalid {}: '{}' is out of range", option, text));

        return std::nullopt;
    }

    if (parsed.ec != std::errc{} or parsed.ptr != digits_end) {
        print_option_error(
            std::format("invalid {}: '{}' is not a number", option, text));

        return std::nullopt;
    }

    if (parsed_size_bytes == 0) {
        print_option_error(
            std::format("invalid {}: '{}' is zero", option, text));

        return std::nullopt;
    }

    if (parsed_size_bytes % alignment != 0) {
        print_option_error(
            std::format("invalid {}: '{}' is not a multiple of {}", option,
                        text, alignment));

        return std::nullopt;
    }

    return parsed_size_bytes;
}

// each '--report' replaces the earlier ones, empty parts are ignored like
// those of '--checks', so '--report=' turns the reports off
[[nodiscard]] auto parse_reports(const std::string_view reports)
    -> std::optional<report_options> {

    report_options parsed;

    for (const auto part : reports | std::views::split(',')) {
        const std::string_view report{part};

        if (report.empty()) {
            continue;
        }

        if (report == "registers") {
            parsed.registers = true;
        } else {
            print_option_error(std::format("invalid --report: '{}', supported "
                                           "reports are: registers",
                                           report));

            return std::nullopt;
        }
    }

    return parsed;
}

// one named check, 'is_ub' tells that 'noub' includes it
struct check_name {
    std::string_view text;
    bool check_options::* member;
    bool is_ub;
};

constexpr std::array<check_name, 10> check_names{
    {
        {.text{"upper"}, .member{&check_options::bounds_upper}, .is_ub{true}},
        {.text{"lower"}, .member{&check_options::bounds_lower}, .is_ub{true}},
        {.text{"line"}, .member{&check_options::with_line}, .is_ub{false}},
        {.text{"frame"}, .member{&check_options::frame}, .is_ub{true}},
        {.text{"alias"}, .member{&check_options::alias}, .is_ub{true}},
        {.text{"division"}, .member{&check_options::division}, .is_ub{true}},
        {.text{"shift"}, .member{&check_options::shift}, .is_ub{true}},
        {.text{"overlap"}, .member{&check_options::overlap}, .is_ub{true}},
        {.text{"stack"}, .member{&check_options::stack}, .is_ub{true}},
        {.text{"overflow"}, .member{&check_options::overflow}, .is_ub{true}},
    },
};

// e.g. 'upper, lower, line'
[[nodiscard]] auto check_texts() -> std::string {
    return joined_texts(check_names);
}

// each '--checks' replaces the earlier ones, empty parts are ignored, 'alias'
// stays on unless '-alias' is given, 'name' and '+name' enable a check,
// '-name' disables it after all enables, so the order in the list does not
// matter
[[nodiscard]] auto parse_checks(const std::string_view checks)
    -> std::optional<check_options> {

    check_options parsed;
    std::vector<bool check_options::*> disabled;

    for (const auto part : checks | std::views::split(',')) {
        std::string_view option{part};

        if (option.empty()) {
            continue;
        }

        const std::string_view written{option};
        const bool is_disable{option.starts_with('-')};

        if (is_disable or option.starts_with('+')) {
            // note: 1 because the sign is not part of the name
            option.remove_prefix(1);
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
                print_option_error(std::format(
                    "invalid --checks: '{}', supported options are: {}, "
                    "noub, each with a '+' or '-' prefix",
                    written, check_texts()));

                return std::nullopt;
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
    return joined_texts(target_names);
}

// keeps the directory so the image lands next to its source
[[nodiscard]] auto
default_binary_file_name(const std::string_view src_file_name,
                         const target machine_target) -> std::string {

    std::filesystem::path path{src_file_name};
    path.replace_extension();
    return std::format("{}-{}.bin", path.string(), target_text(machine_target));
}

auto is_source_file(const std::string_view path, const source_files& files)
    -> bool {

    for (size_t i{}; i < files.count(); ++i) {
        std::error_code ec;

        if (std::filesystem::equivalent(path, files.name(i), ec)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] auto
make_backend(const target machine_target, const source_files* const files,
             const assembler::jump_mode jumps, const size_t stack_size_bytes,
             const size_t memory_size_bytes,
             const std::string_view binary_file_name)
    -> std::unique_ptr<machine> {

    // todo: x86_64 writes no binary, see etc/todo.txt
    if (machine_target == target::x86_64) {
        return std::make_unique<machine_x86_64>(nullptr, files, jumps);
    }

    if (machine_target == target::rv32i) {
        return std::make_unique<machine_rv32i_linux>(nullptr, files, jumps,
                                                     binary_file_name);
    }

    if (machine_target == target::rv32i_qemu) {
        return std::make_unique<machine_rv32i_qemu>(
            nullptr, files, jumps, binary_file_name, stack_size_bytes);
    }

    // the command line accepts only these four targets
    assert(machine_target == target::rv32i_fpga);

    return std::make_unique<machine_rv32i_fpga>(
        nullptr, files, jumps, binary_file_name, stack_size_bytes,
        memory_size_bytes);
}

// every command line error ends with the same hint
auto print_option_error(const std::string_view message) -> void {
    print_unlocated_error(message);
    std::println(stderr, "use --help for usage information");
}

// a user error without a place in the source, e.g. a file that cannot be read
auto print_unlocated_error(const std::string_view message) -> void {
    std::println(stderr, "error: {}", message);
}

// the inlined calls the error was found in, innermost first
auto print_call_frames(const source_files& files,
                       const std::span<const compile_error::call_frame> frames)
    -> void {

    for (const compile_error::call_frame& frame : frames) {
        const std::string_view src_file_name{files.name(frame.file_ix)};
        const std::string_view src{files.text(frame.file_ix)};

        const size_t col{column_for_char_index(frame.start_index, src)};

        // the source line shows a call that is the whole line
        if (frame.text == trimmed_line_at(src, frame.start_index)) {
            std::println(stderr, "{}:{}:{}: {}", src_file_name, frame.line, col,
                         frame.reason);
        } else {
            std::println(stderr, "{}:{}:{}: {} '{}'", src_file_name, frame.line,
                         col, frame.reason, frame.text);
        }

        print_source_line(src, frame.start_index, frame.end_index);
    }
}

// the line of the character at 'index' without its line end, the character can
// be the line end itself
[[nodiscard]] auto line_at(const std::string_view src, const size_t index)
    -> source_line {

    const size_t before_index{
        index == 0 ? std::string_view::npos : src.rfind('\n', index - 1),
    };

    const size_t begin{
        before_index == std::string_view::npos ? 0 : before_index + 1,
    };
    // note: +1 because the line starts after the line end

    const size_t end{std::min(src.find('\n', index), src.size())};

    return {
        .begin{begin},
        .text{src.substr(begin, end - begin)},
    };
}

// the line of the character at 'index' without its surrounding whitespace
auto trimmed_line_at(const std::string_view src, const size_t index)
    -> std::string_view {

    constexpr std::string_view whitespace{" \t\r"};

    const std::string_view line{line_at(src, index).text};

    const size_t first{line.find_first_not_of(whitespace)};

    // note: the character at 'index' starts a token, its line is not blank
    assert(first != std::string_view::npos);

    return line.substr(first, line.find_last_not_of(whitespace) - first + 1);
}

// 'line' and 'start_index' locate the error, the column is derived from them,
// line 0 is a problem of the whole file
auto print_source_error(const source_files& files, const size_t file_ix,
                        const size_t line, const size_t start_index,
                        const size_t end_index, const std::string_view message)
    -> void {

    const std::string_view src_file_name{files.name(file_ix)};
    const std::string_view src{files.text(file_ix)};

    if (line == 0) {
        std::println(stderr, "\n{}: {}", src_file_name, message);
        return;
    }

    const size_t col{column_for_char_index(start_index, src)};

    std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line, col, message);

    print_source_line(src, start_index, end_index);
}

// the bytes after the first one of a multi-byte UTF-8 character
auto is_utf8_continuation(const char ch) -> bool {
    constexpr unsigned char first{0x80};
    constexpr unsigned char last{0xBF};

    const unsigned char byte{static_cast<unsigned char>(ch)};

    return byte >= first and byte <= last;
}

// the columns a terminal shows: a continuation byte after a lead or
// continuation byte is part of that character, a lone one is shown as a
// replacement character
auto display_columns(const std::string_view text) -> size_t {
    constexpr unsigned char first_non_ascii{0x80};

    size_t columns{};
    bool after_non_ascii{};

    for (const char ch : text) {
        if (not after_non_ascii or not is_utf8_continuation(ch)) {
            ++columns;
        }

        after_non_ascii = static_cast<unsigned char>(ch) >= first_non_ascii;
    }

    return columns;
}

// the source line of the error with a '^' under its first character and a '~'
// under the rest of the token, a position without a token has only the '^'
auto print_source_line(const std::string_view src, const size_t start_index,
                       const size_t end_index) -> void {

    if (start_index >= src.size()) {
        return;
    }

    const source_line located{line_at(src, start_index)};
    const size_t line_bgn{located.begin};
    std::string_view line{located.text};

    if (line.ends_with('\r')) {
        line.remove_suffix(1);
        // note: 1 for the carriage return of a crlf line end
    }

    // an error at the end of a file has no line to show
    if (line.empty()) {
        return;
    }

    // a tab stays a tab so that the mark lines up
    std::string padding;
    std::string_view rest{src.substr(line_bgn, start_index - line_bgn)};

    for (size_t tab{rest.find('\t')}; tab != std::string_view::npos;
         tab = rest.find('\t')) {

        padding.append(display_columns(rest.substr(0, tab)), ' ');
        padding += '\t';
        rest.remove_prefix(tab + 1);
    }

    padding.append(display_columns(rest), ' ');

    const size_t mark_end{std::min(end_index, line_bgn + line.size())};

    const size_t marked_count{
        mark_end > start_index
            ? display_columns(src.substr(start_index, mark_end - start_index))
            : 0,
    };

    // a position without a token still has the caret
    const size_t mark_size{std::max(marked_count, size_t{1})};

    std::println(stderr, "{}\n{}^{}", line, padding,
                 std::string(mark_size - 1, '~'));
    // note: -1 because the caret already marks the first character
}
} // namespace
