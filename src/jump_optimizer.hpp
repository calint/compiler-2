#pragma once
// jump optimizations applied to the generated assembly as a post-processing
// pass

// NOLINTBEGIN(misc-definitions-in-headers)

#include <algorithm>
#include <array>
#include <istream>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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

static auto optimize(std::istream& is, std::ostream& os) -> void {
    constexpr size_t instruction_size_bytes{4};
    constexpr size_t branch_limit_bytes{4094};
    std::vector<assembly_line> lines;
    std::unordered_map<std::string, std::vector<size_t>> labels;
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
                    line.max_size = 2 * instruction_size_bytes;
                } else {
                    constexpr std::array<std::string_view, 46> single{
                        "add",   "addi", "sub",   "and",  "andi",  "or",
                        "ori",   "xor",  "xori",  "sll",  "slli",  "srl",
                        "srli",  "sra",  "srai",  "slt",  "slti",  "sltu",
                        "sltiu", "lui",  "auipc", "lb",   "lbu",   "lh",
                        "lhu",   "lw",   "sb",    "sh",   "sw",    "j",
                        "jr",    "jalr", "mv",    "ret",  "ecall", "ebreak",
                        "nop",   "neg",  "not",   "seqz", "snez",  "sltz",
                        "sgtz",  "beq",  "bne",   "blt",
                    };
                    if (inverse(line.mnemonic) or
                        std::ranges::find(single, line.mnemonic) !=
                            single.end()) {
                        line.max_size = instruction_size_bytes;
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
        std::string_view name{line.target};
        if (name.empty()) {
            continue;
        }
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
            continue;
        }
        const std::vector<size_t>& definitions{found->second};
        if (not directional) {
            if (definitions.size() == 1) {
                line.destination = definitions.front();
            }
        } else {
            const auto next{std::ranges::upper_bound(definitions, index)};
            if (direction == 'f' and next != definitions.end()) {
                line.destination = *next;
            } else if (direction == 'b' and next != definitions.begin()) {
                line.destination = *std::prev(next);
            }
        }
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

} // namespace rv32i

} // namespace jump_optimizer

// NOLINTEND(misc-definitions-in-headers)
