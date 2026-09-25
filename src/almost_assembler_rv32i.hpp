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

#include "almost_assembler.hpp"
#include "panic_exception.hpp"

// buffers rv32i output until every label has an offset, then grows the jumps
// that cannot reach their targets

class almost_assembler_rv32i final : public almost_assembler {
    enum class jump_reach : uint8_t {
        // 'bcc target' within 4 KiB
        branch,
        // 'j target' within 1 MiB
        jal,
        // 'jump target, scratch' within 2 GiB
        far,
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

    [[nodiscard]] auto unconditional_jump_mnemonic() const
        -> std::string_view override {

        return "j";
    }

    [[nodiscard]] auto
    inverse_branch_mnemonic(const std::string_view mnemonic) const
        -> std::optional<std::string_view> override {

        return inverse(mnemonic);
    }

    [[nodiscard]] auto format_jump(const jump_info& jump) const
        -> std::string override {

        if (jump.operands.empty()) {
            return std::format("{} {}", jump.mnemonic, jump.target);
        }

        return std::format("{} {}, {}", jump.mnemonic, jump.operands,
                           jump.target);
    }

    [[nodiscard]] auto comment_prefix() const -> std::string_view override {
        return "#";
    }

    // an unsized instruction would make every later offset unreliable
    [[nodiscard]] auto text_code_size(const std::string_view text) const
        -> size_t override {

        const std::optional<size_t> size_bytes{line_size_bytes(text)};
        if (not size_bytes) {
            throw panic_exception{std::format("cannot size '{}'", text)};
        }

        return *size_bytes;
    }

    [[nodiscard]] auto is_label_text(const std::string_view text) const
        -> bool override {

        const std::string_view code{code_part(text)};

        return not code.empty() and code.back() == ':';
    }

    [[nodiscard]] static auto total_size_bytes(const std::vector<line>& lines)
        -> size_t {

        size_t size_bytes{};
        for (const line& l : lines) {
            size_bytes += l.code_size;
        }

        return size_bytes;
    }

    // the form a jump has grown to follows from its size
    [[nodiscard]] auto reach(const line& l) const -> jump_reach {
        const bool conditional{is_conditional(*l.jump)};
        if (conditional and l.code_size == one_instruction_bytes) {
            return jump_reach::branch;
        }

        // grown conditional forms start with an inverted branch around the
        // jump
        const size_t skip_bytes{conditional ? one_instruction_bytes : 0};
        if (l.code_size == skip_bytes + one_instruction_bytes) {
            return jump_reach::jal;
        }

        return jump_reach::far;
    }

    [[nodiscard]] auto skip_size_bytes(const line& l) const -> size_t {
        if (not is_conditional(*l.jump) or reach(l) == jump_reach::branch) {
            return 0;
        }

        return one_instruction_bytes;
    }

    [[nodiscard]] auto reaches(const line& l, const size_t source_offset,
                               const size_t target_offset) const -> bool {

        constexpr int64_t branch_min{-4096};
        constexpr int64_t branch_max{4094};
        constexpr int64_t jal_min{-1048576};
        constexpr int64_t jal_max{1048574};

        // distances count from the jumping instruction, which follows the skip
        const int64_t distance{
            static_cast<int64_t>(target_offset) -
            static_cast<int64_t>(source_offset + skip_size_bytes(l))};

        const jump_reach form{reach(l)};
        if (form == jump_reach::branch) {
            return distance >= branch_min and distance <= branch_max;
        }

        if (form == jump_reach::jal) {
            return distance >= jal_min and distance <= jal_max;
        }

        // the assembler reports distances beyond the 2 GiB of 'auipc' and
        // 'jalr'
        return true;
    }

    auto grow(line& l) const -> void {
        const jump_info& jump{*l.jump};
        const jump_reach grown{
            reach(l) == jump_reach::branch ? jump_reach::jal : jump_reach::far};

        if (grown == jump_reach::far and jump.scratch.empty()) {
            throw panic_exception{std::format(
                "jump to '{}' exceeds 1 MiB and no scratch register is free",
                jump.target)};
        }

        const size_t skip_bytes{is_conditional(jump) ? one_instruction_bytes
                                                     : 0};

        const size_t jump_bytes{grown == jump_reach::far
                                    ? two_instructions_bytes
                                    : one_instruction_bytes};

        l.code_size = skip_bytes + jump_bytes;
    }

    [[nodiscard]] auto line_offsets() const -> std::vector<size_t> {
        std::vector<size_t> offsets;
        offsets.reserve(lines().size());
        size_t offset{};
        for (const line& l : lines()) {
            offsets.push_back(offset);
            offset += l.code_size;
        }

        return offsets;
    }

    [[nodiscard]] auto grow_out_of_reach(
        const std::unordered_map<std::string_view, size_t>& labels) -> bool {

        const std::vector<size_t> offsets{line_offsets()};
        bool grown{};
        for (size_t index{}; index < lines().size(); ++index) {
            line& l{lines()[index]};
            if (not l.jump) {
                continue;
            }

            // an unknown offset would leave the jump unchecked
            const auto target{labels.find(l.jump->target)};
            if (target == labels.end()) {
                throw panic_exception{std::format(
                    "jump to undefined label '{}'", l.jump->target)};
            }

            if (reaches(l, offsets[index], offsets[target->second])) {
                continue;
            }

            grow(l);
            grown = true;
        }

        return grown;
    }

    auto write_long_jump(std::ostream& os, const std::string_view indent,
                         const line& l) const -> void {

        if (reach(l) == jump_reach::far) {
            std::println(os, "{}jump {}, {}", indent, l.jump->target,
                         l.jump->scratch);

            return;
        }

        std::println(os, "{}j {}", indent, l.jump->target);
    }

    auto write_line(std::ostream& os, const line& l, size_t& skip_count) const
        -> void {

        if (l.removed) {
            return;
        }

        if (not l.jump or l.code_size == one_instruction_bytes) {
            std::println(os, "{}", l.text);

            return;
        }

        const std::string_view indent{leading_whitespace(l.text)};
        if (not is_conditional(*l.jump)) {
            write_long_jump(os, indent, l);

            return;
        }

        // a named label keeps numeric 'Nf' and 'Nb' references unchanged
        const std::string skip_label{
            std::format(".Lbaz_jump.{}", skip_count++)};

        std::println(os, "{}{} {}, {}", indent,
                     inverse(l.jump->mnemonic).value_or(std::string_view{}),
                     l.jump->operands, skip_label);

        write_long_jump(os, indent, l);
        std::println(os, "{}:", skip_label);
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

    // keeps the version with less code, the first on ties
    auto emit_smaller(const std::function_ref<void()> emit_first,
                      const std::function_ref<void()> emit_second) -> void {

        std::vector<line> first{capture(emit_first)};
        std::vector<line> second{capture(emit_second)};

        std::vector<line>& kept{
            total_size_bytes(first) <= total_size_bytes(second) ? first
                                                                : second};

        append(std::move(kept));
    }

    auto resolve_and_write(std::ostream& os) -> void {
        assert(not is_capturing());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines()};

        // sizes only grow, so this ends once every jump reaches its target
        bool grown{true};
        while (grown) {
            grown = grow_out_of_reach(labels);
        }

        size_t skip_count{};
        for (const line& l : lines()) {
            write_line(os, l, skip_count);
        }
        lines().clear();
    }
};
