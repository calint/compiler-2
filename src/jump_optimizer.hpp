#pragma once
// x86 jump optimizations applied to the generated assembly as a
// post-processing pass, rv32i optimizes in 'assembler_rv32i'
//
// jumps_to_next:
//     jmp if.16.8.code
//     if.16.8.code:
//   to
//     if.16.8.code:
//
// unreachable_jumps:
//     jmp loop.10.5.end
//     jmp loop.10.5
//   to
//     jmp loop.10.5.end
//
// same_outcome_branches:
//     jne bool.15.19.end
//     jmp bool.15.19.end
//   to
//     jmp bool.15.19.end
//
// inverted_branches:
//     jne cmp.19.27
//     jmp if.19.8.code
//     cmp.19.27:
//   to
//     je if.19.8.code
//     cmp.19.27:

// NOLINTBEGIN(misc-definitions-in-headers)

#include <format>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
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

// same output as 'assembler_rv32i::optimization_counts'
struct optimization_counts {
    size_t jumps_to_next{};
    size_t unreachable_jumps{};
    size_t same_outcome_branches{};
    size_t inverted_branches{};

    // aligned with the usage statistics
    auto print(std::ostream& os) const -> void {
        std::println(os);

        std::println(os, "; {:>28}: {}", "removed jumps to next code",
                     jumps_to_next);

        std::println(os, "; {:>28}: {}", "removed unreachable jumps",
                     unreachable_jumps);

        std::println(os, "; {:>28}: {}", "removed same target branches",
                     same_outcome_branches);

        std::println(os, "; {:>28}: {}", "inverted branches over jumps",
                     inverted_branches);
    }
};

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

// owns its text because 'jump_info' views into a line being read
struct jump {
    std::string mnemonic;
    std::string target;
};

struct line {
    std::string text;
    std::string label;
    std::unique_ptr<jump> jump_to;
    // emits an instruction
    bool code{};
    // a label in code where execution can enter
    bool entry{};
    bool removed{};
};

auto optimize(std::istream& is, std::ostream& os, optimization_counts& counts)
    -> void;

// comments, blank lines and other sections emit no code, like in rv32i
[[nodiscard]] static auto make_line(std::string text, bool& code_section)
    -> line {

    line l{
        .text{std::move(text)},
        .label{},
        .jump_to{},
        .code{},
        .entry{},
        .removed{},
    };

    const std::string_view trimmed{
        std::string_view{l.text}.substr(leading_ws(l.text).size())};

    constexpr std::string_view section{"section "};
    if (trimmed.starts_with(section)) {
        code_section = trimmed.substr(section.size()).starts_with(".text");

        return l;
    }

    if (not code_section or trimmed.empty() or trimmed.starts_with(';')) {
        return l;
    }

    if (const std::optional<std::string_view> label{
            parse_label_strict(trimmed)}) {

        l.label = *label;
        l.entry = true;

        return l;
    }

    l.code = true;
    if (const std::optional<jump_info> parsed{parse_jump(trimmed)}) {
        l.jump_to = std::make_unique<jump>(jump{
            .mnemonic{parsed->mnemonic},
            .target{parsed->label},
        });
    }

    return l;
}

[[nodiscard]] static auto is_conditional(const jump& j) -> bool {
    return j.mnemonic != "jmp";
}

// labels, comments, other sections and removed lines emit no code
[[nodiscard]] static auto next_instruction(const std::vector<line>& lines,
                                           size_t index) -> size_t {

    while (index < lines.size() and not lines[index].code) {
        ++index;
    }

    return index;
}

// a label in between would let execution enter between the two jumps
[[nodiscard]] static auto
following_unconditional_jump(const std::vector<line>& lines, const size_t index)
    -> std::optional<size_t> {

    for (size_t next{index + 1}; next < lines.size(); ++next) {
        const line& l{lines[next]};
        if (l.entry) {
            return std::nullopt;
        }

        if (not l.code) {
            continue;
        }

        if (not l.jump_to or is_conditional(*l.jump_to)) {
            return std::nullopt;
        }

        return next;
    }

    return std::nullopt;
}

[[nodiscard]] static auto
destination(const std::unordered_map<std::string_view, size_t>& labels,
            const jump& j) -> std::optional<size_t> {

    const auto found{labels.find(j.target)};
    if (found == labels.end()) {
        return std::nullopt;
    }

    return found->second;
}

static auto remove(line& l) -> void {
    l.jump_to.reset();
    l.code = false;
    l.removed = true;
}

// the same rules in the same order as 'assembler_rv32i::optimize_jump'
// so both targets count the same optimizations
[[nodiscard]] static auto
optimize_jump(std::vector<line>& lines, const size_t index,
              const std::unordered_map<std::string_view, size_t>& labels,
              optimization_counts& counts) -> bool {

    line& branch{lines[index]};
    if (not branch.jump_to) {
        return false;
    }

    const std::optional<size_t> target{destination(labels, *branch.jump_to)};
    if (not target) {
        return false;
    }

    const size_t target_code{next_instruction(lines, *target)};

    // execution continues at the target anyway
    if (target_code == next_instruction(lines, index + 1)) {
        remove(branch);
        ++counts.jumps_to_next;

        return true;
    }

    const std::optional<size_t> jump_index{
        following_unconditional_jump(lines, index)};

    if (not jump_index) {
        return false;
    }

    line& jmp{lines[*jump_index]};

    // nothing reaches a jump right after an unconditional jump
    if (not is_conditional(*branch.jump_to)) {
        remove(jmp);
        ++counts.unreachable_jumps;

        return true;
    }

    const std::optional<size_t> jmp_target{destination(labels, *jmp.jump_to)};
    if (not jmp_target) {
        return false;
    }

    // both outcomes continue at the same place
    if (target_code == next_instruction(lines, *jmp_target)) {
        remove(branch);
        ++counts.same_outcome_branches;

        return true;
    }

    // branching over the jump is the inverse branch to its target
    if (target_code != next_instruction(lines, *jump_index + 1)) {
        return false;
    }

    const std::optional<std::string_view> inverted{
        invert_jcc(branch.jump_to->mnemonic)};

    if (not inverted) {
        return false;
    }

    branch.jump_to->mnemonic = *inverted;
    branch.jump_to->target = jmp.jump_to->target;
    branch.text = std::format("{}{} {}", leading_ws(branch.text),
                              branch.jump_to->mnemonic, branch.jump_to->target);
    remove(jmp);
    ++counts.inverted_branches;

    return true;
}

// removes jumps that change nothing and turns a branch over a jump into the
// inverse branch, repeating because each change can enable another
auto optimize(std::istream& is, std::ostream& os, optimization_counts& counts)
    -> void {

    std::vector<line> lines;
    bool code_section{true};
    std::string text;
    while (getline(is, text)) {
        lines.push_back(make_line(std::move(text), code_section));
    }

    // views into 'lines' stay valid because it is no longer resized
    std::unordered_map<std::string_view, size_t> labels;
    for (size_t i{}; i < lines.size(); ++i) {
        if (not lines[i].label.empty()) {
            labels.emplace(lines[i].label, i);
        }
    }

    bool changed{true};
    while (changed) {
        changed = false;
        for (size_t i{}; i < lines.size(); ++i) {
            changed = optimize_jump(lines, i, labels, counts) or changed;
        }
    }

    for (const line& l : lines) {
        if (not l.removed) {
            std::println(os, "{}", l.text);
        }
    }
}

} // namespace x86

} // namespace jump_optimizer

// NOLINTEND(misc-definitions-in-headers)
