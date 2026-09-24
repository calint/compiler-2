#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <format>
#include <functional>
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

// buffers rv32i output until every label has an offset, then grows the jumps
// that cannot reach their targets

class almost_assembler_rv32i final {
    enum class jump_reach : uint8_t {
        // 'bcc target' within 4 KiB
        branch,
        // 'j target' within 1 MiB
        jal,
        // 'jump target, scratch' within 2 GiB
        far,
    };

    struct jump_info {
        std::string mnemonic;
        std::string inverted_mnemonic;
        std::string operands;
        std::string target;
        std::string scratch;
        jump_reach reach{};
    };

    struct line {
        std::string text;
        std::string label;
        std::unique_ptr<jump_info> jump;
        size_t size_bytes{};
        // a label in code where execution can enter
        bool entry{};
        bool removed{};
    };

    static constexpr size_t one_instruction_bytes{4};
    static constexpr size_t two_instructions_bytes{8};

    // instructions that assemble to one 4-byte word
    static constexpr std::array<std::string_view, 46> single_instructions{
        "add",  "addi", "sub",   "and",    "andi",  "or",  "ori",  "xor",
        "xori", "sll",  "slli",  "srl",    "srli",  "sra", "srai", "slt",
        "slti", "sltu", "sltiu", "lui",    "auipc", "lb",  "lbu",  "lh",
        "lhu",  "lw",   "sb",    "sh",     "sw",    "j",   "jr",   "jalr",
        "mv",   "ret",  "ecall", "ebreak", "nop",   "neg", "not",  "seqz",
        "snez", "sltz", "sgtz",  "beq",    "bne",   "blt",
    };

    // changes made by 'optimize_jumps', printed by 'finish'
    struct optimization_counts {
        size_t jumps_to_next{};
        size_t unreachable_jumps{};
        size_t same_outcome_branches{};
        size_t inverted_branches{};
    };

    std::vector<line> lines_;
    // versions being emitted by 'emit_smaller', innermost last
    std::vector<std::vector<line>> captures_;
    bool code_section_{true};
    optimization_counts optimizations_;

    [[nodiscard]] auto current_lines() -> std::vector<line>& {
        if (captures_.empty()) {
            return lines_;
        }

        return captures_.back();
    }

    [[nodiscard]] auto capture(const std::function_ref<void()> emit)
        -> std::vector<line> {

        captures_.emplace_back();
        emit();
        std::vector<line> captured{std::move(captures_.back())};
        captures_.pop_back();

        return captured;
    }

    [[nodiscard]] static auto total_size_bytes(const std::vector<line>& lines)
        -> size_t {

        size_t size_bytes{};
        for (const line& l : lines) {
            size_bytes += l.size_bytes;
        }

        return size_bytes;
    }

    // longer conditional forms start with an inverted branch around the jump
    [[nodiscard]] static auto skip_size_bytes(const jump_info& jump) -> size_t {

        if (jump.inverted_mnemonic.empty() or
            jump.reach == jump_reach::branch) {
            return 0;
        }

        return one_instruction_bytes;
    }

    [[nodiscard]] static auto reaches(const jump_info& jump,
                                      const size_t source_offset,
                                      const size_t target_offset) -> bool {

        constexpr int64_t branch_min{-4096};
        constexpr int64_t branch_max{4094};
        constexpr int64_t jal_min{-1048576};
        constexpr int64_t jal_max{1048574};

        // distances count from the jumping instruction, which follows the skip
        const int64_t distance{
            static_cast<int64_t>(target_offset) -
            static_cast<int64_t>(source_offset + skip_size_bytes(jump))};

        if (jump.reach == jump_reach::branch) {
            return distance >= branch_min and distance <= branch_max;
        }

        if (jump.reach == jump_reach::jal) {
            return distance >= jal_min and distance <= jal_max;
        }

        // the assembler reports distances beyond the 2 GiB of 'auipc' and
        // 'jalr'
        return true;
    }

    static auto grow(line& l) -> void {
        jump_info& jump{*l.jump};
        jump.reach = jump.reach == jump_reach::branch ? jump_reach::jal
                                                      : jump_reach::far;

        if (jump.reach == jump_reach::far and jump.scratch.empty()) {
            throw panic_exception{std::format(
                "jump to '{}' exceeds 1 MiB and no scratch register is free",
                jump.target)};
        }

        const size_t jump_bytes{jump.reach == jump_reach::far
                                    ? two_instructions_bytes
                                    : one_instruction_bytes};

        l.size_bytes = skip_size_bytes(jump) + jump_bytes;
    }

    [[nodiscard]] auto label_lines() const
        -> std::unordered_map<std::string_view, size_t> {

        std::unordered_map<std::string_view, size_t> labels;
        for (size_t index{}; index < lines_.size(); ++index) {
            if (not lines_[index].label.empty()) {
                labels.emplace(lines_[index].label, index);
            }
        }

        return labels;
    }

    [[nodiscard]] auto line_offsets() const -> std::vector<size_t> {
        std::vector<size_t> offsets;
        offsets.reserve(lines_.size());
        size_t offset{};
        for (const line& l : lines_) {
            offsets.push_back(offset);
            offset += l.size_bytes;
        }

        return offsets;
    }

    [[nodiscard]] auto grow_out_of_reach(
        const std::unordered_map<std::string_view, size_t>& labels) -> bool {

        const std::vector<size_t> offsets{line_offsets()};
        bool grown{};
        for (size_t index{}; index < lines_.size(); ++index) {
            line& l{lines_[index]};
            if (not l.jump) {
                continue;
            }

            // an unknown offset would leave the jump unchecked
            const auto target{labels.find(l.jump->target)};
            if (target == labels.end()) {
                throw panic_exception{std::format(
                    "jump to undefined label '{}'", l.jump->target)};
            }

            if (reaches(*l.jump, offsets[index], offsets[target->second])) {
                continue;
            }

            grow(l);
            grown = true;
        }

        return grown;
    }

    static auto write_long_jump(std::ostream& os, const std::string_view indent,
                                const jump_info& jump) -> void {

        if (jump.reach == jump_reach::far) {
            std::println(os, "{}jump {}, {}", indent, jump.target,
                         jump.scratch);

            return;
        }

        std::println(os, "{}j {}", indent, jump.target);
    }

    static auto write_line(std::ostream& os, const line& l, size_t& skip_count)
        -> void {

        if (l.removed) {
            return;
        }

        if (not l.jump or l.size_bytes == one_instruction_bytes) {
            std::println(os, "{}", l.text);

            return;
        }

        const std::string_view indent{leading_whitespace(l.text)};
        if (l.jump->inverted_mnemonic.empty()) {
            write_long_jump(os, indent, *l.jump);

            return;
        }

        // a named label keeps numeric 'Nf' and 'Nb' references unchanged
        const std::string skip_label{
            std::format(".Lbaz_jump.{}", skip_count++)};

        std::println(os, "{}{} {}, {}", indent, l.jump->inverted_mnemonic,
                     l.jump->operands, skip_label);

        write_long_jump(os, indent, *l.jump);
        std::println(os, "{}:", skip_label);
    }

    [[nodiscard]] static auto is_conditional(const jump_info& jump) -> bool {
        return not jump.inverted_mnemonic.empty();
    }

    [[nodiscard]] static auto
    destination(const std::unordered_map<std::string_view, size_t>& labels,
                const jump_info& jump) -> std::optional<size_t> {

        const auto found{labels.find(jump.target)};
        if (found == labels.end()) {
            return std::nullopt;
        }

        return found->second;
    }

    // labels, comments, other sections and removed lines emit no code
    [[nodiscard]] auto next_instruction(size_t index) const -> size_t {
        while (index < lines_.size() and lines_[index].size_bytes == 0) {
            ++index;
        }

        return index;
    }

    // a label in between would let execution enter between the two jumps
    [[nodiscard]] auto following_unconditional_jump(const size_t index) const
        -> std::optional<size_t> {

        for (size_t next{index + 1}; next < lines_.size(); ++next) {
            const line& l{lines_[next]};
            if (l.entry) {
                return std::nullopt;
            }

            if (l.size_bytes == 0) {
                continue;
            }

            if (not l.jump or is_conditional(*l.jump)) {
                return std::nullopt;
            }

            return next;
        }

        return std::nullopt;
    }

    static auto remove(line& l) -> void {
        l.jump.reset();
        l.size_bytes = 0;
        l.removed = true;
    }

    static auto invert(line& l, std::string target) -> void {
        jump_info& jump{*l.jump};
        std::swap(jump.mnemonic, jump.inverted_mnemonic);
        jump.target = std::move(target);

        l.text = std::format("{}{} {}, {}", leading_whitespace(l.text),
                             jump.mnemonic, jump.operands, jump.target);
    }

    [[nodiscard]] auto
    optimize_jump(const size_t index,
                  const std::unordered_map<std::string_view, size_t>& labels)
        -> bool {

        line& branch{lines_[index]};
        if (not branch.jump) {
            return false;
        }

        // an undefined target is reported when resolving
        const std::optional<size_t> target{destination(labels, *branch.jump)};
        if (not target) {
            return false;
        }

        const size_t target_code{next_instruction(*target)};

        // execution continues at the target anyway
        if (target_code == next_instruction(index + 1)) {
            remove(branch);
            ++optimizations_.jumps_to_next;

            return true;
        }

        const std::optional<size_t> jump_index{
            following_unconditional_jump(index)};

        if (not jump_index) {
            return false;
        }

        line& jump{lines_[*jump_index]};

        // nothing reaches a jump right after an unconditional jump
        if (not is_conditional(*branch.jump)) {
            remove(jump);
            ++optimizations_.unreachable_jumps;

            return true;
        }

        const std::optional<size_t> jump_target{
            destination(labels, *jump.jump)};

        if (not jump_target) {
            return false;
        }

        // both outcomes continue at the same place
        if (target_code == next_instruction(*jump_target)) {
            remove(branch);
            ++optimizations_.same_outcome_branches;

            return true;
        }

        // branching over the jump is the inverse branch to its target
        if (target_code == next_instruction(*jump_index + 1)) {
            invert(branch, jump.jump->target);
            remove(jump);
            ++optimizations_.inverted_branches;

            return true;
        }

        return false;
    }

    [[nodiscard]] static auto leading_whitespace(const std::string_view text)
        -> std::string_view {

        return text.substr(0, text.find_first_not_of(" \t"));
    }

    [[nodiscard]] static auto trim(const std::string_view text)
        -> std::string_view {

        const size_t first{text.find_first_not_of(" \t\r")};
        if (first == std::string_view::npos) {
            return {};
        }

        return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
    }

    [[nodiscard]] static auto code_part(const std::string_view text)
        -> std::string_view {

        return trim(text.substr(0, text.find('#')));
    }

    [[nodiscard]] static auto is_label_text(const std::string_view text)
        -> bool {

        const std::string_view code{code_part(text)};

        return not code.empty() and code.back() == ':';
    }

    // directives that emit bytes would make offsets wrong, so only these are
    // expected in code
    [[nodiscard]] static auto is_sizeless_directive(const std::string_view code)
        -> bool {

        const std::string_view name{code.substr(0, code.find_first_of(" \t"))};

        return name == ".option" or name == ".globl" or name == ".equ" or
               name == ".text" or name == ".data" or name == ".section";
    }

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

        // norelax keeps address, call and long jump sequences at two
        // instructions
        if (mnemonic == "la" or mnemonic == "call" or mnemonic == "jump" or
            mnemonic == "tail") {

            return two_instructions_bytes;
        }

        if (inverse(mnemonic) or
            std::ranges::find(single_instructions, mnemonic) !=
                single_instructions.end()) {

            return one_instruction_bytes;
        }

        return std::nullopt;
    }

  public:
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

        return std::nullopt;
    }
    // labels, directives and comments occupy no space, unknown instructions
    // have no size
    [[nodiscard]] static auto line_size_bytes(const std::string_view text)
        -> std::optional<size_t> {

        const std::string_view code{code_part(text)};

        if (code.empty() or code.back() == ':') {
            return 0;
        }

        if (code.front() == '.') {
            if (is_sizeless_directive(code)) {
                return 0;
            }

            return std::nullopt;
        }

        const size_t split{code.find_first_of(" \t")};

        const std::string_view arguments{split == std::string_view::npos
                                             ? std::string_view{}
                                             : code.substr(split)};

        return instruction_size_bytes(code.substr(0, split), arguments);
    }

    auto set_code_section(const bool code_section) -> void {
        code_section_ = code_section;
    }

    auto add_text(std::string text) -> void {
        if (not code_section_) {
            current_lines().push_back({
                .text{std::move(text)},
                .label{},
                .jump{},
                .size_bytes{},
                .entry{},
                .removed{},
            });

            return;
        }

        const std::optional<size_t> size_bytes{line_size_bytes(text)};

        // an unsized instruction would make every later offset unreliable
        if (not size_bytes) {
            throw panic_exception{std::format("cannot size '{}'", text)};
        }

        // numeric labels are text but still let execution enter
        const bool entry{is_label_text(text)};

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .jump{},
            .size_bytes{*size_bytes},
            .entry{entry},
            .removed{},
        });
    }

    auto add_label(std::string name, std::string text) -> void {
        current_lines().push_back({
            .text{std::move(text)},
            .label{std::move(name)},
            .jump{},
            .size_bytes{},
            .entry{code_section_},
            .removed{},
        });
    }

    // 'mnemonic' is 'j' or a conditional branch taking 'operands'
    auto add_jump(std::string text, const std::string_view mnemonic,
                  const std::string_view operands,
                  const std::string_view target, const std::string_view scratch)
        -> void {

        const std::optional<std::string_view> inverted{inverse(mnemonic)};

        assert(inverted.has_value() or mnemonic == "j");

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .jump{std::make_unique<jump_info>(jump_info{
                .mnemonic{std::string{mnemonic}},
                .inverted_mnemonic{
                    std::string{inverted.value_or(std::string_view{})}},
                .operands{std::string{operands}},
                .target{std::string{target}},
                .scratch{std::string{scratch}},
                .reach{inverted ? jump_reach::branch : jump_reach::jal},
            })},
            .size_bytes{one_instruction_bytes},
            .entry{},
            .removed{},
        });
    }

    // keeps the version with less code, the first on ties
    auto emit_smaller(const std::function_ref<void()> emit_first,
                      const std::function_ref<void()> emit_second) -> void {

        std::vector<line> first{capture(emit_first)};
        std::vector<line> second{capture(emit_second)};

        std::vector<line>& kept{
            total_size_bytes(first) <= total_size_bytes(second) ? first
                                                                : second};

        std::ranges::move(kept, std::back_inserter(current_lines()));
    }

    // removes jumps that change nothing and turns a branch over a jump into
    // the inverse branch, repeating because each change can enable another
    auto optimize_jumps() -> void {
        assert(captures_.empty());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines()};

        bool changed{true};
        while (changed) {
            changed = false;
            for (size_t index{}; index < lines_.size(); ++index) {
                changed = optimize_jump(index, labels) or changed;
            }
        }
    }

    auto resolve_and_write(std::ostream& os) -> void {
        assert(captures_.empty());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines()};

        // sizes only grow, so this ends once every jump reaches its target
        bool grown{true};
        while (grown) {
            grown = grow_out_of_reach(labels);
        }

        size_t skip_count{};
        for (const line& l : lines_) {
            write_line(os, l, skip_count);
        }
        lines_.clear();
    }

    // prints the optimization counts as comments, aligned with the usage
    // statistics that follow
    auto finish(std::ostream& os) -> void {
        std::println(os);

        std::println(os, "# {:>28}: {}", "removed jumps to next code",
                     optimizations_.jumps_to_next);

        std::println(os, "# {:>28}: {}", "removed unreachable jumps",
                     optimizations_.unreachable_jumps);

        std::println(os, "# {:>28}: {}", "removed same target branches",
                     optimizations_.same_outcome_branches);

        std::println(os, "# {:>28}: {}", "inverted branches over jumps",
                     optimizations_.inverted_branches);

        optimizations_ = {};
    }
};
