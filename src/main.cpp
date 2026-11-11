// the driver: reads the options, makes the backend that '--target' names,
// compiles the program and writes the output, or prints the first error with
// its source line
#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdio>
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

// note: the headers kept without a use here hold the definitions that close
//       the include cycles (README 7.7)

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

// the version '--version' prints; 'make.sh' defines 'BAZ_VERSION' from 'git
// describe', other builds do not
#ifdef BAZ_VERSION
constexpr std::string_view baz_version{BAZ_VERSION};
#else
constexpr std::string_view baz_version{"unknown"};
#endif

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
constexpr std::array<kept_version_name, 4> kept_version_names{
    kept_version_name{.kind{machine::kept_version::smaller}, .text{"smaller"}},
    kept_version_name{.kind{machine::kept_version::nested}, .text{"nested"}},
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

// the text of '--help', which the readme usage section pastes; the format
// arguments are listed in 'print_help'
constexpr std::string_view usage_format{
    R"(usage: {0} [options] [file]
compiles file (default: {9}) to assembly on stdout, rv32i targets also
write a binary image

options:
  --target=MACHINE    x86_64 (default): linux, nasm
                      rv32i: linux, llvm assembler, qemu user mode
                      rv32i-qemu: bare-metal image for the qemu virt machine,
                        fails when code, data, variables and stack exceed its
                        126 MiB
                      rv32i-fpga: bare-metal image for the fpga soft core,
                        stack grows down from the end of the memory (see
                        --memory), fails when code, data, variables and stack
                        do not fit
  --vars=SIZE         variable storage in bytes, decimal or 0x hex, must be a
                      nonzero multiple of {1} (default: {2})
  --stack=SIZE        rv32i-qemu and rv32i-fpga stack in bytes, decimal or 0x
                      hex, must be a multiple of {3}, at least {7} and at
                      most the memory of the target (default: {4})
  --memory=SIZE       rv32i-fpga memory in bytes, decimal or 0x hex, must be a
                      nonzero multiple of {5} (default: {6}), the
                      emulator is built for the default
  --checks=LIST       comma separated checks, replaces earlier --checks
  --report=LIST       comma separated reports after the code, replaces earlier
                      --report
  --bin=FILE          rv32i targets binary image (default: file without
                      extension followed by -MACHINE.bin)
  --sdcard=FILE       x86_64 and rv32i sd card image that 'sector_read' and
                      'sector_write' open at run time, relative to the working
                      directory of the program, it must exist and hold the
                      sector (default: {8})
  --nopt              no jump optimizations
  --versions=KEPT     of two versions of the same code keep the smaller,
                      nested choices taking the first ('smaller', default),
                      the smaller also of nested choices ('nested', compile
                      time doubles per level), or for testing 'first' or
                      'last', also of nested choices (compile time doubles
                      per level)
  --reproduce-source  check that the source written back from the parse
                      matches the input, else write it to diff.baz
  --version           the version of the compiler
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
  division runtime division by zero and 'MIN / -1', also of '%'; a constant
           'MIN / -1' is rejected at compile time
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
               default_sd_card_file_name, default_src_file_name);
}

// the 'text' of each row of a table joined by ', ', e.g. 'upper, lower, line'
// note: by value, a filtered view is not iterated through a const reference
template <std::ranges::input_range Names>
[[nodiscard]] auto joined_texts(Names names) -> std::string {
    std::string texts;

    for (const auto& name : names) {
        if (not texts.empty()) {
            texts += ", ";
        }

        texts += name.text;
    }

    return texts;
}

// the row of 'rows' whose 'text' is 'value'; the message of an unknown one
// names 'option' and lists the rows as the supported 'kind'
template <class Row, size_t row_count>
[[nodiscard]] auto named_row(const std::array<Row, row_count>& rows,
                             const std::string_view value,
                             const std::string_view option,
                             const std::string_view kind) -> const Row& {

    const auto found{std::ranges::find(rows, value, &Row::text)};

    if (found == rows.end()) {
        throw option_error{
            std::format("invalid {}: {}, supported {} are: {}", option,
                        in_quotes(value), kind, joined_texts(rows)),
        };
    }

    return *found;
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
    const std::array<target_option, 4> target_options{
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
        target_option{
            .name{"--bin"},
            .reads{&target_name::writes_image},
            .given{opts.binary_file_name.has_value()},
        },
    };

    const target_name& chosen{*opts.target};

    for (const target_option& option : target_options) {
        if (not option.given or chosen.*option.reads) {
            continue;
        }

        const auto reads{
            [&option](const target_name& name) -> bool {
                return name.*option.reads;
            },
        };

        const std::string readers{
            joined_texts(target_names | std::views::filter(reads)),
        };

        throw option_error{
            std::format("invalid {}: target {} does not read it, the targets "
                        "that do are: {}",
                        option.name, in_quotes(chosen.text), readers)};
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

// 'option' is named in the error message
[[nodiscard]] auto parse_file_name(const std::string_view text,
                                   const std::string_view option)
    -> std::string_view {

    if (text.empty()) {
        throw option_error{std::format("invalid {}: empty file name", option)};
    }

    return text;
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

        const report_name& row{
            named_row(report_names, report, "--report", "reports"),
        };

        parsed.*row.member = true;
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

// the checks 'name' turns on, e.g. every no-UB check for 'noub';
// 'written' is the option as given, for the error
[[nodiscard]] auto checks_named(const std::string_view name,
                                const std::string_view written)
    -> std::vector<bool check_options::*> {

    if (name == "noub") {
        std::vector<bool check_options::*> members;

        for (const check_name& check : check_names) {
            if (check.is_ub) {
                members.push_back(check.member);
            }
        }

        return members;
    }

    const auto* const found{
        std::ranges::find(check_names, name, &check_name::text),
    };

    if (found == check_names.end()) {
        throw option_error{
            std::format("invalid --checks: {}, supported checks are: {}, "
                        "noub, each with a '+' or '-' prefix",
                        in_quotes(written), joined_texts(check_names))};
    }

    return {found->member};
}

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

        // e.g. '-noub' disables every check that 'noub' enables
        const std::vector<bool check_options::*> members{
            checks_named(option, written),
        };

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

// the appliers of the options with a value store the value in 'opts'
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
    opts.target = &named_row(target_names, value, "--target", "targets");
}

auto apply_checks(const std::string_view value, options& opts) -> void {
    opts.checks = parse_checks(value);
}

auto apply_report(const std::string_view value, options& opts) -> void {
    opts.reports = parse_reports(value);
}

auto apply_bin(const std::string_view value, options& opts) -> void {
    opts.binary_file_name = parse_file_name(value, "--bin");
}

auto apply_sdcard(const std::string_view value, options& opts) -> void {
    opts.sd_card_file_name = parse_file_name(value, "--sdcard");
}

auto apply_versions(const std::string_view value, options& opts) -> void {
    const kept_version_name& row{
        named_row(kept_version_names, value, "--versions", "values"),
    };

    opts.kept_version = row.kind;
}

auto parse_option(const std::string_view arg, options& opts) -> void {
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

    for (const value_option& option : value_options) {
        if (arg.starts_with(option.name)) {
            // e.g. '0x40000' of '--vars=0x40000'
            const std::string_view value{arg.substr(option.name.size())};
            option.apply(value, opts);
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

// a user error without a place in the source, e.g. a file that cannot be read
auto print_unlocated_error(const std::string_view message) -> void {
    std::println(stderr, "error: {}", printable(message));
}

// every command line error ends with the same hint
auto print_option_error(const std::string_view message) -> void {
    print_unlocated_error(message);
    std::println(stderr, "use --help for usage information");
}

// 0 when 'what' written to stdout reached it, else 1 with a message
// note: the flush reports a write that stdio buffered, e.g. to a closed
//       stdout
[[nodiscard]] auto flushed_stdout(const std::string_view what) -> int {
    if (std::fflush(stdout) == 0) {
        return 0;
    }

    std::println(stderr, "error: cannot write {} to stdout", what);

    return 1;
}

// the exit code when the program should stop instead of compiling
[[nodiscard]] auto parse_options(const std::span<const char*> args,
                                 options& opts) -> std::optional<int> {

    try {
        for (const char* const argument : args | std::views::drop(1)) {
            const std::string_view arg{argument};

            if (arg == "--help" or arg == "-h") {
                print_help(args.at(0));
                return flushed_stdout("the help");
            }

            if (arg == "--version") {
                std::println("baz {}", baz_version);
                return flushed_stdout("the version");
            }

            parse_option(arg, opts);
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

// an error at the end of the source is shown after its last character that
// is not whitespace, e.g. after the last token of an unclosed block
[[nodiscard]] auto located_in_text(const std::string_view src,
                                   const source_span& where) -> source_span {

    if (where.start_index != src.size()) {
        return where;
    }

    const size_t last{src.find_last_not_of(" \t\r\n")};

    // note: an error at the end follows a token of the file, so the file is
    //       not only whitespace

    assert(last != std::string_view::npos);

    const size_t end{last + 1};

    const size_t line{
        static_cast<size_t>(std::ranges::count(src.substr(0, end), '\n')) + 1,
    };
    // note: +1 because the lines start at 1

    return {
        .file_ix{where.file_ix},
        .line{line},
        .start_index{end},
        .end_index{end},
    };
}

// the line as 'printable' shows it, with its tabs so that the mark lines up
[[nodiscard]] auto printable_line(std::string_view line) -> std::string {
    std::string result;

    for (size_t tab{line.find('\t')}; tab != std::string_view::npos;
         tab = line.find('\t')) {

        result += printable(line.substr(0, tab));
        result += '\t';
        line.remove_prefix(tab + 1);
    }

    result += printable(line);

    return result;
}

// 'fill' under each character of 'text' as 'printable_line' shows it, a tab
// stays a tab so that what follows lines up
[[nodiscard]] auto aligned_fill(const std::string_view text, const char fill)
    -> std::string {

    std::string result;

    for (const char ch : printable_line(text)) {
        if (ch == '\t') {
            result += '\t';
        } else if (not token::is_utf8_continuation(ch)) {
            result += fill;
        }
    }

    return result;
}

// the source line of the error with a '^' under its first character and a '~'
// under the rest of the token, a position without a token has only the '^'
auto print_source_line(const std::string_view src, const size_t start_index,
                       const size_t end_index) -> void {

    assert(start_index <= src.size());

    const source_line located{line_at(src, start_index)};
    const size_t line_bgn{located.begin};
    std::string_view line{located.text};

    // a carriage return without a line feed after it is the lone one of the
    // error, 'assert_source_well_formed', and stays shown
    const bool has_line_feed{line_bgn + line.size() < src.size()};

    if (has_line_feed and line.ends_with('\r')) {
        line.remove_suffix(1);
        // note: 1 for the carriage return of a crlf line end
    }

    // note: an error at the end of the source is placed after its last
    //       character, 'located_in_text'

    assert(not line.empty());

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

// the includes that loaded the file 'file_ix', the last one in the main file
auto print_include_chain(const source_files& files, size_t file_ix) -> void {
    const source_locations locations{files};

    while (const std::optional<source_span>& at{files.included_at(file_ix)}) {
        std::println(stderr, "{}: included from", locations.for_error(*at));

        print_source_line(files.text(at->file_ix), at->start_index,
                          at->end_index);

        file_ix = at->file_ix;
    }
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
    const std::string_view src{files.text(where->file_ix)};
    const source_span shown{located_in_text(src, *where)};

    std::println(stderr, "\n{}: {}", locations.for_error(shown),
                 printable(message));

    print_source_line(src, shown.start_index, shown.end_index);
}

// the message of the error at its source line, the inlined calls and generic
// instances it was found through and what else is known about it
auto print_compiler_error(const source_files& files, const compile_error& error)
    -> void {

    print_source_error(files, error.where, error.msg);

    print_call_frames(files, error.call_frames);

    // after the calls, so that a call frame does not read as an include
    if (error.where) {
        print_include_chain(files, error.where->file_ix);
    }

    if (not error.detail.empty()) {
        std::println(stderr, "\n{}", error.detail);
    }
}

// the options of the backend, '--bin' resolved to 'binary'
[[nodiscard]] auto make_target_options(const options& opts,
                                       const std::string& binary)
    -> machine::target_options {

    return {
        .jumps{
            opts.optimize_jumps ? assembler::jump_mode::optimized
                                : assembler::jump_mode::as_emitted,
        },
        .kept{opts.kept_version},
        .register_report{opts.reports.registers},
        .sd_card_file_name{
            std::string{
                opts.sd_card_file_name.value_or(default_sd_card_file_name),
            },
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

// '--bin' or by default in the directory of the source so the image lands
// next to it
[[nodiscard]] auto
resolved_binary_file_name(const options& opts,
                          const std::string_view src_file_name) -> std::string {

    if (opts.binary_file_name) {
        return std::string{*opts.binary_file_name};
    }

    std::filesystem::path path{src_file_name};
    path.replace_extension();

    return std::format("{}-{}.bin", path.string(), opts.target->text);
}

// 'binary' is not the sd card, neither the same file nor the same path
auto check_image_is_not_sd_card(const options& opts, const std::string& binary)
    -> void {

    const std::string_view sd_card{
        opts.sd_card_file_name.value_or(default_sd_card_file_name),
    };

    std::error_code error;

    const bool is_same_file{
        std::filesystem::equivalent(binary, sd_card, error),
    };

    // note: a card that does not exist yet is compared by its absolute path,
    //       which is empty when it cannot be resolved

    const std::filesystem::path image_path{
        std::filesystem::weakly_canonical(
            std::filesystem::absolute(binary, error), error),
    };

    const std::filesystem::path card_path{
        std::filesystem::weakly_canonical(
            std::filesystem::absolute(sd_card, error), error),
    };

    const bool is_same_path{
        not image_path.empty() and image_path == card_path,
    };

    if (is_same_file or is_same_path) {
        throw option_error{std::format("invalid --bin: {} is the sd card image",
                                       in_quotes(binary))};
    }
}

// the image replaces no input of the compile or of the program
auto check_image_destination(const options& opts, const source_files& files,
                             const std::string& binary) -> void {

    if (not opts.target->writes_image) {
        return;
    }

    // the image would replace a source file, the parse read the includes
    if (files.contains(binary)) {
        throw option_error{std::format("invalid --bin: {} is a source file",
                                       in_quotes(binary))};
    }

    // the image would replace the sd card that the program reads
    if (opts.target->reads_sd_card) {
        check_image_is_not_sd_card(opts, binary);
    }
}

// e.g. a full disk, the listing is incomplete; a failed compile leaves no
// image
auto check_listing_written(const options& opts, const std::string& binary)
    -> void {

    std::cout.flush();

    if (std::cout) {
        return;
    }

    if (opts.target->writes_image) {
        std::error_code error;
        std::filesystem::remove(binary, error);
    }

    throw unlocated_error{"cannot write the assembly to stdout"};
}

// the source file compiled for the target, the listing written to stdout;
// 'files' holds the sources read, for the report of an error
auto compile_to_stdout(const options& opts, source_files& files,
                       const builtin_type_set& builtins) -> void {

    const std::string_view src_file_name{
        opts.src_file_name.value_or(default_src_file_name),
    };

    std::string src_text{source_files::read_file(src_file_name)};

    files.add(std::string{src_file_name}, std::move(src_text), std::nullopt);

    const std::string binary{resolved_binary_file_name(opts, src_file_name)};

    const std::unique_ptr<machine> backend{
        opts.target->make(builtins, files, make_target_options(opts, binary)),
    };

    // the checks the target cannot honor are off, e.g. x86_64 has no
    // stack bound
    check_options checks{opts.checks};
    checks.stack = checks.stack and backend->is_stack_bounded();

    program prg{*backend, builtins, files, opts.vars_size_bytes, checks};

    check_image_destination(opts, files, binary);

    if (opts.reproduce_source) {
        check_reproduced_source(prg, files);
    }

    prg.build(std::cout);

    check_listing_written(opts, binary);
}

[[nodiscard]] auto compile_file(const options& opts) -> int {
    source_files files;

    // the machine and the program refer to them
    const builtin_type_set builtins;

    try {
        compile_to_stdout(opts, files, builtins);
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
        // note: the text may hold a file name, e.g. that of
        //       'check_reproduced_source'
        std::println(stderr, "\ninternal error: {}", printable(e.what()));
        return 1;
    }

    return 0;
}
} // namespace

auto main(const int argc, const char** const argv) -> int {
    // a counted view of a pointer is a span that the compiler accepts as safe
    const std::span<const char*> args{std::views::counted(argv, argc)};

    // a message that cannot be written, e.g. to a full or closed stderr,
    // throws from 'std::print'; the exit code still tells the failure
    try {
        options opts{};

        if (const std::optional<int> exit_code{parse_options(args, opts)}) {
            return *exit_code;
        }

        return compile_file(opts);
    } catch (const std::system_error&) {
        return 1;
    }
}
