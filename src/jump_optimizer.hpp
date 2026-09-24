#pragma once
// jump optimizations applied to the generated assembly as a post-processing
// pass

// NOLINTBEGIN(misc-definitions-in-headers)

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <format>
#include <istream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "panic_exception.hpp"

namespace jump_optimizer {

[[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
    return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r' or ch == '\f' or
           ch == '\v';
}

[[nodiscard]] static auto leading_ws(const std::string_view line)
    -> std::string_view {

    size_t first{};
    while (first < line.size() and is_ascii_space(line[first])) {
        ++first;
    }

    return line.substr(0, first);
}

// keep target-specific parsing separate because branch syntax differs
namespace x86 {

struct jump_info {
    std::string_view mnemonic;
    std::string_view label;
};

[[nodiscard]] static auto is_ascii_lower(const char ch) -> bool {
    return ch >= 'a' and ch <= 'z';
}

[[nodiscard]] static auto is_ascii_alpha(const char ch) -> bool {
    return (ch >= 'a' and ch <= 'z') or (ch >= 'A' and ch <= 'Z');
}

[[nodiscard]] static auto is_ascii_alnum(const char ch) -> bool {
    return is_ascii_alpha(ch) or (ch >= '0' and ch <= '9');
}

[[nodiscard]] static auto parse_jump(const std::string_view line)
    -> std::optional<jump_info> {

    size_t i{leading_ws(line).size()};
    if (i == line.size() or line[i] != 'j') {
        return std::nullopt;
    }

    const size_t mnemonic_start{i};
    ++i;
    size_t mnemonic_letters{};
    while (i < line.size() and mnemonic_letters < 2 and
           is_ascii_lower(line[i])) {
        ++i;
        ++mnemonic_letters;
    }

    if (mnemonic_letters == 0 or i >= line.size() or
        not is_ascii_space(line[i])) {

        return std::nullopt;
    }

    const size_t mnemonic_end{i};

    while (i < line.size() and is_ascii_space(line[i])) {
        ++i;
    }
    if (i >= line.size()) {
        return std::nullopt;
    }

    const size_t label_start{i};
    size_t label_end{line.size()};
    while (label_end > label_start and is_ascii_space(line[label_end - 1])) {
        --label_end;
    }
    if (label_start == label_end) {
        return std::nullopt;
    }

    return jump_info{
        .mnemonic{line.substr(mnemonic_start, mnemonic_end - mnemonic_start)},
        .label{line.substr(label_start, label_end - label_start)},
    };
}

[[nodiscard]] static auto parse_label_strict(const std::string_view line)
    -> std::optional<std::string_view> {

    size_t i{leading_ws(line).size()};
    if (i == line.size()) {
        return std::nullopt;
    }

    const size_t label_start{i};
    if (not is_ascii_alpha(line[i]) and line[i] != '_') {
        return std::nullopt;
    }

    ++i;
    // function labels are dot-separated, e.g. 'func.f.12.45.end'
    while (i < line.size() and
           (is_ascii_alnum(line[i]) or line[i] == '_' or line[i] == '.')) {
        ++i;
    }

    if (i == line.size() or line[i] != ':') {
        return std::nullopt;
    }

    const size_t label_end{i};
    ++i;
    while (i < line.size()) {
        if (not is_ascii_space(line[i])) {
            return std::nullopt;
        }
        ++i;
    }

    return line.substr(label_start, label_end - label_start);
}

[[nodiscard]] static auto parse_label_any(const std::string_view line)
    -> std::optional<std::string_view> {

    const size_t start{leading_ws(line).size()};
    if (start == line.size()) {
        return std::nullopt;
    }

    const size_t colon{line.find(':', start)};
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    size_t end{colon};
    while (end > start and is_ascii_space(line[end - 1])) {
        --end;
    }
    if (end == start) {
        return std::nullopt;
    }

    return line.substr(start, end - start);
}

[[nodiscard]] static auto invert_jcc(const std::string_view jcc)
    -> std::optional<std::string_view> {

    if (jcc == "jne") {
        return "je";
    }
    if (jcc == "je") {
        return "jne";
    }
    if (jcc == "jg") {
        return "jle";
    }
    if (jcc == "jge") {
        return "jl";
    }
    if (jcc == "jl") {
        return "jge";
    }
    if (jcc == "jle") {
        return "jg";
    }

    return std::nullopt;
}

auto pass1(std::istream& is, std::ostream& os) -> void;
auto pass2(std::istream& is, std::ostream& os) -> void;

//
// pass 1
//
//  example:
//    jmp cmp_13_26
//    cmp_13_26:
//  to
//    cmp_13_26:
//
//  example:
//    jne bool_end_15_9
//    jmp bool_end_15_9
//    bool_end_15_9:
//  to
//    bool_end_15_9:
//
auto pass1(std::istream& is, std::ostream& os) -> void {

    size_t opts_count{};

    std::vector<std::string> pending_jumps;
    std::optional<std::string> pending_label;

    auto flush_pending{[&]() -> void {
        for (const std::string& buffered_line : pending_jumps) {
            std::println(os, "{}", buffered_line);
        }
        pending_jumps.clear();
        pending_label.reset();
    }};

    std::string line;
    while (getline(is, line)) {
        if (const std::optional<jump_info> jump{parse_jump(line)}) {
            // keep buffering only while jumps target the same label
            if (pending_label and *pending_label != jump->label) {
                flush_pending();
            }
            pending_jumps.emplace_back(line);
            pending_label = std::string{jump->label};
            continue;
        }

        if (const std::optional<std::string_view> lbl{
                parse_label_strict(line)}) {

            // target label reached: drop pending jumps, print label
            if (pending_label and *pending_label == *lbl) {
                opts_count += pending_jumps.size();
                pending_jumps.clear();
                pending_label.reset();
                std::println(os, "{}", line);
                continue;
            }
        }

        flush_pending();
        std::println(os, "{}", line);
    }

    flush_pending();
    std::println(os, ";          optimization pass 1: {}", opts_count);
}

//
// pass 2
//
// example:
//   jne cmp_14_26
//   jmp if_14_8_code
//   cmp_14_26:
// to
//   je if_14_8_code
//   cmp_14_26:
//
auto pass2(std::istream& is, std::ostream& os) -> void {

    size_t optimizations{};

    auto print2{
        [&](const std::string_view a, const std::string_view b) -> void {
            std::println(os, "{}", a);
            std::println(os, "{}", b);
        }};

    auto print3{[&](const std::string_view a, const std::string_view b,
                    const std::string_view c) -> void {
        std::println(os, "{}", a);
        std::println(os, "{}", b);
        std::println(os, "{}", c);
    }};

    std::string first_line;
    while (getline(is, first_line)) {
        const std::optional<jump_info> jcc_match{parse_jump(first_line)};
        if (not jcc_match) {
            std::println(os, "{}", first_line);
            continue;
        }

        const std::string_view jcc{jcc_match->mnemonic};
        const std::string_view jcc_label{jcc_match->label};

        std::string second_line;
        if (not getline(is, second_line)) {
            std::println(os, "{}", first_line);

            return;
        }

        const std::optional<jump_info> jmp_match{parse_jump(second_line)};
        if (not jmp_match or jmp_match->mnemonic != "jmp") {
            print2(first_line, second_line);
            continue;
        }

        const std::string_view jmp_label{jmp_match->label};

        std::string third_line;
        if (not getline(is, third_line)) {
            print2(first_line, second_line);

            return;
        }

        const std::optional<std::string_view> lbl_match{
            parse_label_any(third_line)};

        if (not lbl_match) {
            print3(first_line, second_line, third_line);
            continue;
        }

        const std::string_view label{*lbl_match};

        if (jcc_label != label) {
            print3(first_line, second_line, third_line);
            continue;
        }

        //   jne cmp_14_26
        //   jmp if_14_8_code
        //   cmp_14_26:
        const std::optional<std::string_view> inverted_jcc{invert_jcc(jcc)};
        if (not inverted_jcc) {
            print3(first_line, second_line, third_line);
            continue;
        }
        //   je if_14_8_code
        //   cmp_14_26:

        // get the whitespace
        const std::string_view ws_before{leading_ws(first_line)};

        std::println(os, "{}{} {}", ws_before, *inverted_jcc, jmp_label);
        std::println(os, "{}", third_line);
        ++optimizations;
    }

    std::println(os, ";          optimization pass 2: {}", optimizations);
}

} // namespace x86

namespace rv32i {

struct assembly_line {
    std::string text;
    std::string label;
    std::string mnemonic;
    std::string operands;
    std::string target;
    std::optional<size_t> destination;
    size_t max_size{};
    bool barrier{};
    bool removed{};
};

[[nodiscard]] static auto trim(const std::string_view text)
    -> std::string_view {
    const size_t first{text.find_first_not_of(" \t\r")};
    if (first == std::string_view::npos) {
        return {};
    }

    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

[[nodiscard]] static auto inverse(const std::string_view mnemonic)
    -> std::optional<std::string_view> {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 8>
        pairs{{
            {"beq", "bne"},
            {"blt", "bge"},
            {"bltu", "bgeu"},
            {"bgt", "ble"},
            {"bgtu", "bleu"},
            {"beqz", "bnez"},
            {"bltz", "bgez"},
            {"bgtz", "blez"},
        }};
    for (const auto& [first, second] : pairs) {
        if (mnemonic == first) {
            return second;
        }
        if (mnemonic == second) {
            return first;
        }
    }

    return {};
}

// instructions that assemble to one 4-byte word
constexpr std::array<std::string_view, 46> single_instructions{
    "add",  "addi", "sub",   "and",    "andi",  "or",  "ori",  "xor",
    "xori", "sll",  "slli",  "srl",    "srli",  "sra", "srai", "slt",
    "slti", "sltu", "sltiu", "lui",    "auipc", "lb",  "lbu",  "lh",
    "lhu",  "lw",   "sb",    "sh",     "sw",    "j",   "jr",   "jalr",
    "mv",   "ret",  "ecall", "ebreak", "nop",   "neg", "not",  "seqz",
    "snez", "sltz", "sgtz",  "beq",    "bne",   "blt",
};

constexpr size_t one_instruction_bytes{4};
constexpr size_t two_instructions_bytes{8};

// an 'li' constant that fits 'addi' or has no low part is one instruction
[[nodiscard]] static auto li_size_bytes(const std::string_view arguments)
    -> size_t {

    constexpr int32_t immediate_min{-2048};
    constexpr int32_t immediate_max{2047};
    constexpr uint32_t low_mask{0xfff};

    const size_t comma{arguments.find(',')};
    if (comma == std::string_view::npos) {
        return two_instructions_bytes;
    }

    const std::string_view literal{trim(arguments.substr(comma + 1))};
    if (literal.empty()) {
        return two_instructions_bytes;
    }

    int64_t parsed{};
    const char* const end{std::to_address(literal.end())};

    const std::from_chars_result conversion{
        std::from_chars(std::to_address(literal.begin()), end, parsed)};

    // unresolved expressions retain the conservative size
    if (conversion.ec != std::errc{} or conversion.ptr != end or
        parsed < std::numeric_limits<int32_t>::min() or
        std::cmp_greater(parsed, std::numeric_limits<uint32_t>::max())) {

        return two_instructions_bytes;
    }

    const uint32_t bits{static_cast<uint32_t>(parsed)};
    const int32_t immediate{std::bit_cast<int32_t>(bits)};
    if ((immediate >= immediate_min and immediate <= immediate_max) or
        (bits & low_mask) == 0) {

        return one_instruction_bytes;
    }

    return two_instructions_bytes;
}

// sizes under '.option norvc' and '.option norelax'
[[nodiscard]] static auto
instruction_size_bytes(const std::string_view mnemonic,
                       const std::string_view arguments)
    -> std::optional<size_t> {

    if (mnemonic == "li") {
        return li_size_bytes(arguments);
    }

    // norelax keeps address, call and long jump sequences at two instructions
    if (mnemonic == "la" or mnemonic == "call" or mnemonic == "jump" or
        mnemonic == "tail") {

        return two_instructions_bytes;
    }

    if (inverse(mnemonic) or std::ranges::find(single_instructions, mnemonic) !=
                                 single_instructions.end()) {

        return one_instruction_bytes;
    }

    return std::nullopt;
}

using label_definitions = std::unordered_map<std::string, std::vector<size_t>>;

// numeric labels repeat so 'Nf' and 'Nb' resolve to the nearest definition
[[nodiscard]] static auto resolve_destination(const label_definitions& labels,
                                              std::string_view name,
                                              const size_t index)
    -> std::optional<size_t> {

    const bool directional{
        name.size() > 1 and (name.back() == 'f' or name.back() == 'b') and
        name.substr(0, name.size() - 1).find_first_not_of("0123456789") ==
            std::string_view::npos};

    const char direction{name.back()};
    if (directional) {
        name.remove_suffix(1);
    }

    const auto found{labels.find(std::string{name})};
    if (found == labels.end()) {
        return std::nullopt;
    }

    const std::vector<size_t>& definitions{found->second};
    if (not directional) {
        // a repeated plain label is ambiguous
        if (definitions.size() != 1) {
            return std::nullopt;
        }

        return definitions.front();
    }

    const auto next{std::ranges::upper_bound(definitions, index)};
    if (direction == 'f') {
        if (next == definitions.end()) {
            return std::nullopt;
        }

        return *next;
    }

    if (next == definitions.begin()) {
        return std::nullopt;
    }

    return *std::prev(next);
}

static auto optimize(std::istream& is, std::ostream& os) -> void {
    constexpr size_t word_size_bytes{4};
    constexpr size_t branch_limit_bytes{4094};
    std::vector<assembly_line> lines;
    label_definitions labels;
    std::string text;
    while (getline(is, text)) {
        assembly_line line{
            .text{text},
            .label{},
            .mnemonic{},
            .operands{},
            .target{},
            .destination{},
            .max_size{},
            .barrier{},
            .removed{},
        };

        const std::string_view code{
            trim(std::string_view{text}.substr(0, text.find('#')))};

        if (not code.empty()) {
            if (code.back() == ':' and
                code.find_first_of(" \t") == std::string_view::npos) {
                line.label = code.substr(0, code.size() - 1);
                labels[line.label].push_back(lines.size());
            } else if (code.front() == '.') {
                // directives may change sections, alignment or instruction
                // widths
                line.barrier = true;
            } else {
                const size_t split{code.find_first_of(" \t")};
                line.mnemonic = code.substr(0, split);
                const std::string_view arguments{
                    split == std::string_view::npos ? std::string_view{}
                                                    : trim(code.substr(split))};
                if (line.mnemonic == "j") {
                    line.target = arguments;
                } else if (inverse(line.mnemonic)) {
                    const size_t comma{arguments.rfind(',')};
                    if (comma != std::string_view::npos) {
                        line.operands = arguments.substr(0, comma + 1);
                        line.target = trim(arguments.substr(comma + 1));
                    }
                }
                // upper bounds avoid shortening a long jump beyond branch reach
                if (line.mnemonic == "li" or line.mnemonic == "la" or
                    line.mnemonic == "call") {
                    line.max_size = 2 * word_size_bytes;
                } else {
                    if (inverse(line.mnemonic) or
                        std::ranges::find(single_instructions, line.mnemonic) !=
                            single_instructions.end()) {
                        line.max_size = word_size_bytes;
                    } else {
                        // unknown assembler constructs are not safe to size or
                        // cross
                        line.barrier = true;
                    }
                }
            }
        }
        lines.push_back(std::move(line));
    }

    // resolve numeric references before editing so repeated labels remain
    // distinct
    for (size_t index{}; index < lines.size(); ++index) {
        assembly_line& line{lines[index]};
        if (line.target.empty()) {
            continue;
        }
        line.destination = resolve_destination(labels, line.target, index);
    }

    const auto next_instruction = [&](size_t index) -> size_t {
        while (index < lines.size() and
               (lines[index].removed or
                (lines[index].mnemonic.empty() and not lines[index].barrier))) {
            ++index;
        }

        return index;
    };
    const auto in_branch_range = [&](const size_t source,
                                     const size_t destination) -> bool {
        size_t distance{};
        for (size_t index{std::min(source, destination)};
             index < std::max(source, destination); ++index) {
            if (lines[index].barrier) {
                return false;
            }
            if (not lines[index].removed) {
                distance += lines[index].max_size;
            }
            // the conservative bound also covers backward branches
            if (distance > branch_limit_bytes) {
                return false;
            }
        }

        return true;
    };

    // deletions expose more fallthroughs but labels must remain for other users
    bool changed{true};
    while (changed) {
        changed = false;
        for (size_t index{}; index < lines.size(); ++index) {
            assembly_line& line{lines[index]};
            if (line.removed or not line.destination) {
                continue;
            }
            const size_t next{next_instruction(index + 1)};
            if (next_instruction(*line.destination) == next) {
                line.removed = true;
                changed = true;
                continue;
            }
            // only comments may separate the pair because labels allow entry
            // midway
            size_t following{index + 1};
            while (following < lines.size() and
                   lines[following].label.empty() and
                   not lines[following].barrier and
                   (lines[following].removed or
                    lines[following].mnemonic.empty())) {
                ++following;
            }
            if (following == lines.size() or
                not lines[following].label.empty()) {
                continue;
            }
            assembly_line& jump{lines[following]};
            if (jump.mnemonic != "j" or not jump.destination) {
                continue;
            }
            if (line.mnemonic == "j") {
                // no entry point exists between these jumps so the second is
                // unreachable
                jump.removed = true;
                changed = true;
            } else if (next_instruction(*line.destination) ==
                       next_instruction(*jump.destination)) {
                // either outcome takes the same path so the condition is
                // irrelevant
                line.removed = true;
                changed = true;
            } else if (const std::optional<std::string_view> inverted{
                           inverse(line.mnemonic)};
                       inverted and
                       next_instruction(*line.destination) ==
                           next_instruction(following + 1) and
                       in_branch_range(index, *jump.destination)) {
                // moving the target across no labels preserves numeric f/b
                // references
                line.mnemonic = *inverted;
                line.target = jump.target;
                line.destination = jump.destination;
                line.text =
                    std::format("{}{} {} {}", leading_ws(line.text),
                                line.mnemonic, line.operands, line.target);
                jump.removed = true;
                changed = true;
            }
        }
    }
    for (const assembly_line& line : lines) {
        if (not line.removed) {
            std::println(os, "{}", line.text);
        }
    }
}

// jumps grow from one instruction to longer sequences until the target is in
// reach
enum class jump_reach : uint8_t {
    none,
    // 'bcc target' within 4 KiB
    branch,
    // 'j target' within 1 MiB
    jal,
    // 'jump target, scratch' within 2 GiB
    far,
};

struct resolved_line {
    std::string_view text;
    std::string_view mnemonic;
    std::string_view inverted_mnemonic;
    std::string_view operands;
    std::string_view target;
    std::string_view scratch;
    std::optional<size_t> destination;
    size_t offset{};
    size_t size_bytes{};
    jump_reach reach{};
    bool conditional{};
};

// the backend marks a register without a live value at the jump as '# baz: t3'
[[nodiscard]] static auto scratch_hint(const std::string_view comment)
    -> std::string_view {

    constexpr std::string_view marker{"# baz:"};
    if (not comment.starts_with(marker)) {
        return {};
    }

    return trim(comment.substr(marker.size()));
}

// offsets must be exact, so code may only contain size-free directives
[[nodiscard]] static auto is_text_after(const std::string_view directive,
                                        const bool in_text) -> bool {

    const std::string_view name{
        directive.substr(0, directive.find_first_of(" \t"))};

    if (name == ".text") {
        return true;
    }

    if (name == ".data" or name == ".bss") {
        return false;
    }

    if (name == ".section") {
        return trim(directive.substr(name.size())).starts_with(".text");
    }

    if (not in_text or name == ".option" or name == ".globl" or
        name == ".equ") {

        return in_text;
    }

    throw panic_exception{std::format("cannot size '{}' in .text", directive)};
}

static auto parse_instruction(resolved_line& line, const std::string_view code)
    -> void {

    const size_t split{code.find_first_of(" \t")};
    line.mnemonic = code.substr(0, split);

    const std::string_view arguments{split == std::string_view::npos
                                         ? std::string_view{}
                                         : trim(code.substr(split))};

    const std::optional<size_t> size_bytes{
        instruction_size_bytes(line.mnemonic, arguments)};

    // an unknown size would make every later offset unreliable
    if (not size_bytes) {
        throw panic_exception{
            std::format("cannot size instruction '{}'", code)};
    }
    line.size_bytes = *size_bytes;

    if (line.mnemonic == "j") {
        line.target = arguments;
        line.reach = jump_reach::jal;

        return;
    }

    const std::optional<std::string_view> inverted{inverse(line.mnemonic)};
    const size_t comma{arguments.rfind(',')};
    if (not inverted or comma == std::string_view::npos) {
        return;
    }

    line.inverted_mnemonic = *inverted;
    line.operands = arguments.substr(0, comma + 1);
    line.target = trim(arguments.substr(comma + 1));
    line.reach = jump_reach::branch;
    line.conditional = true;
}

// longer conditional forms start with an inverted branch around the jump
[[nodiscard]] static auto skip_size_bytes(const resolved_line& line) -> size_t {

    if (line.conditional and line.reach != jump_reach::branch) {
        return one_instruction_bytes;
    }

    return 0;
}

[[nodiscard]] static auto reaches(const resolved_line& line,
                                  const size_t target_offset) -> bool {

    constexpr int64_t branch_min{-4096};
    constexpr int64_t branch_max{4094};
    constexpr int64_t jal_min{-1048576};
    constexpr int64_t jal_max{1048574};

    // distances count from the jumping instruction, which follows the skip
    const int64_t distance{
        static_cast<int64_t>(target_offset) -
        static_cast<int64_t>(line.offset + skip_size_bytes(line))};

    if (line.reach == jump_reach::branch) {
        return distance >= branch_min and distance <= branch_max;
    }

    if (line.reach == jump_reach::jal) {
        return distance >= jal_min and distance <= jal_max;
    }

    // the assembler reports distances beyond the 2 GiB of 'auipc' and 'jalr'
    return true;
}

static auto grow(resolved_line& line) -> void {
    line.reach =
        line.reach == jump_reach::branch ? jump_reach::jal : jump_reach::far;

    if (line.reach == jump_reach::far and line.scratch.empty()) {
        throw panic_exception{std::format(
            "jump to '{}' exceeds 1 MiB and no scratch register is free",
            line.target)};
    }

    const size_t jump_bytes{line.reach == jump_reach::far
                                ? two_instructions_bytes
                                : one_instruction_bytes};

    line.size_bytes = skip_size_bytes(line) + jump_bytes;
}

static auto print_jump(std::ostream& os, const std::string_view indent,
                       const resolved_line& line) -> void {

    if (line.reach == jump_reach::far) {
        std::println(os, "{}jump {}, {}", indent, line.target, line.scratch);

        return;
    }

    std::println(os, "{}j {}", indent, line.target);
}

// the backend emits the shortest jumps and this pass grows those out of reach,
// which must happen even without 'optimize' for the output to assemble
static auto resolve_jumps(std::istream& is, std::ostream& os) -> void {
    const std::string input{std::istreambuf_iterator<char>{is},
                            std::istreambuf_iterator<char>{}};

    std::vector<resolved_line> lines;
    label_definitions labels;
    bool in_text{true};
    std::string_view remaining{input};
    while (not remaining.empty()) {
        const size_t end{remaining.find('\n')};
        const std::string_view text{remaining.substr(0, end)};

        remaining.remove_prefix(end == std::string_view::npos ? remaining.size()
                                                              : end + 1);

        const size_t comment{text.find('#')};

        lines.push_back({
            .text{text},
            .mnemonic{},
            .inverted_mnemonic{},
            .operands{},
            .target{},
            .scratch{comment == std::string_view::npos
                         ? std::string_view{}
                         : scratch_hint(text.substr(comment))},
            .destination{},
            .offset{},
            .size_bytes{},
            .reach{},
            .conditional{},
        });

        const std::string_view code{trim(text.substr(0, comment))};
        if (code.empty()) {
            continue;
        }

        if (code.back() == ':' and
            code.find_first_of(" \t") == std::string_view::npos) {
            labels[std::string{code.substr(0, code.size() - 1)}].push_back(
                lines.size() - 1);
            continue;
        }

        if (code.front() == '.') {
            in_text = is_text_after(code, in_text);
            continue;
        }

        if (in_text) {
            parse_instruction(lines.back(), code);
        }
    }

    for (size_t index{}; index < lines.size(); ++index) {
        resolved_line& line{lines[index]};
        if (line.reach == jump_reach::none or line.target.empty()) {
            continue;
        }
        line.destination = resolve_destination(labels, line.target, index);
    }

    // sizes only grow, so the passes end once every jump reaches its target
    bool grown{true};
    while (grown) {
        size_t offset{};
        for (resolved_line& line : lines) {
            line.offset = offset;
            offset += line.size_bytes;
        }

        grown = false;
        for (resolved_line& line : lines) {
            // unresolved targets are left for the assembler to report
            if (not line.destination or
                reaches(line, lines[*line.destination].offset)) {
                continue;
            }
            grow(line);
            grown = true;
        }
    }

    size_t skip_count{};
    for (const resolved_line& line : lines) {
        const bool grown_jump{
            line.reach == jump_reach::far or
            (line.conditional and line.reach == jump_reach::jal)};

        if (not grown_jump) {
            std::println(os, "{}", line.text);
            continue;
        }

        const std::string_view indent{leading_ws(line.text)};
        if (not line.conditional) {
            print_jump(os, indent, line);
            continue;
        }

        // a named label keeps numeric 'Nf' and 'Nb' references unchanged
        const std::string skip_label{
            std::format(".Lbaz_jump.{}", skip_count++)};

        std::println(os, "{}{} {} {}", indent, line.inverted_mnemonic,
                     line.operands, skip_label);

        print_jump(os, indent, line);
        std::println(os, "{}:", skip_label);
    }
}

} // namespace rv32i

} // namespace jump_optimizer

// NOLINTEND(misc-definitions-in-headers)
