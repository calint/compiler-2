#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "almost_assembler.hpp"
#include "panic_exception.hpp"

// buffers rv32i output until every label has an offset, then grows the jumps
// that cannot reach their targets, writes the assembly source and, from the
// same structured lines, a flat binary image

class almost_assembler_rv32i final : public almost_assembler {
  public:
    static constexpr std::array<std::string_view, 32> register_names{
        "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    };

    // 'and', 'or' and 'xor' are c++ operator names
    enum class op : uint8_t {
        add,
        sub,
        sll,
        slt,
        sltu,
        xor_op,
        srl,
        sra,
        or_op,
        and_op,
        addi,
        slti,
        sltiu,
        xori,
        ori,
        andi,
        slli,
        srli,
        srai,
        lui,
        lb,
        lh,
        lw,
        lbu,
        lhu,
        sb,
        sh,
        sw,
        beq,
        bne,
        blt,
        bge,
        bltu,
        bgeu,
        bgt,
        ble,
        bgtu,
        bleu,
        beqz,
        bnez,
        bltz,
        bgez,
        bgtz,
        blez,
        ecall,
        ebreak,
        li,
        la,
        mv,
        j,
        jr,
        call,
        ret,
    };

    enum class section : uint8_t { text, rodata, data };

    // a number, or a symbol resolved once addresses are known
    struct immediate {
        enum class part : uint8_t { whole, high, low };

        int64_t number{};
        std::string symbol;
        part symbol_part{};

        immediate() = default;

        // numbers of any integer type keep call sites free of casts
        template <std::integral integral_t>
        explicit(false) immediate(const integral_t value)
            : number{checked_number(value)} {}

        [[nodiscard]] static auto
        of_symbol(const std::string_view name,
                  const part symbol_part = part::whole) -> immediate {

            immediate result;
            result.symbol = name;
            result.symbol_part = symbol_part;

            return result;
        }

      private:
        template <std::integral integral_t>
        [[nodiscard]] static auto checked_number(const integral_t value)
            -> int64_t {

            if (not std::in_range<int64_t>(value)) {
                throw panic_exception{
                    std::format("immediate {} exceeds 64 bits", value)};
            }

            return static_cast<int64_t>(value);
        }
    };

    // abi names, 'x' numbers and 'fp' name the same registers
    [[nodiscard]] static auto register_number(const std::string_view name)
        -> std::optional<uint8_t> {

        for (size_t number{}; number < register_names.size(); ++number) {
            if (name == register_names.at(number)) {
                return static_cast<uint8_t>(number);
            }
        }

        if (name == "fp") {
            return frame_pointer_register;
        }

        // 'x05' does not name a register
        if (name.size() < 2 or name.front() != 'x' or
            (name.size() > 2 and name.at(1) == '0')) {

            return std::nullopt;
        }

        const std::string_view digits{name.substr(1)};
        const char* const end{std::to_address(digits.end())};
        uint8_t number{};

        const std::from_chars_result parsed{
            std::from_chars(std::to_address(digits.begin()), end, number)};

        if (parsed.ec != std::errc{} or parsed.ptr != end or
            number >= register_names.size()) {

            return std::nullopt;
        }

        return number;
    }

  private:
    enum class jump_reach : uint8_t {
        // 'bcc target' within 4 KiB
        branch,
        // 'j target' within 1 MiB
        jal,
        // 'jump target, scratch' within 2 GiB
        far,
    };

    // how the operands are written and encoded
    enum class form : uint8_t {
        registers,
        immediate,
        shift,
        upper,
        load,
        store,
        branch,
        branch_zero,
        system,
        load_immediate,
        load_address,
        move,
        jump,
        jump_register,
        call,
        ret,
    };

    struct op_info {
        std::string_view mnemonic;
        form operands{};
        // opcode and function bits, the base instruction's for pseudos
        uint32_t encoding{};
        // pseudo branches such as 'bgt' swap the base branch's operands
        bool swapped{};
    };

    static constexpr size_t op_count{std::to_underlying(op::ret) + 1};

    static constexpr uint32_t addi_encoding{0x00000013};
    static constexpr uint32_t lui_encoding{0x00000037};
    static constexpr uint32_t auipc_encoding{0x00000017};
    static constexpr uint32_t jal_encoding{0x0000006f};
    static constexpr uint32_t jalr_encoding{0x00000067};

    static constexpr uint8_t zero_register{};
    static constexpr uint8_t return_address_register{1};
    static constexpr uint8_t frame_pointer_register{8};

    static constexpr int64_t immediate_min{-2048};
    static constexpr int64_t immediate_max{2047};
    static constexpr int64_t shift_max{31};
    static constexpr int64_t upper_max{0xfffff};
    static constexpr uint32_t upper_mask{0xfffff};
    static constexpr uint32_t low_mask{0xfff};
    static constexpr int64_t branch_min{-4096};
    static constexpr int64_t branch_max{4094};
    static constexpr int64_t jal_min{-1048576};
    static constexpr int64_t jal_max{1048574};

    struct instruction {
        op code{};
        uint8_t rd{};
        uint8_t rs1{};
        uint8_t rs2{};
        immediate value;
        // label of branches, 'j' and 'call'
        std::string target;
    };

    // registers as the caller named them, so text keeps names such as 'x11'
    struct spelling {
        std::string_view rd;
        std::string_view rs1;
        std::string_view rs2;
    };

    // the registers a branch given to the jump resolver compares
    struct jump_registers {
        uint8_t rs1{};
        uint8_t rs2{};
    };

    struct data_values {
        size_t element_size_bytes{};
        std::vector<int64_t> values;
        size_t repeat_count{};
    };

    struct alignment {
        size_t size_bytes{};
    };

    struct section_start {
        section which{};
    };

    struct constant {
        std::string name;
        int64_t value{};
    };

    using record = std::variant<instruction, jump_registers, data_values,
                                alignment, section_start, constant>;

    static constexpr size_t section_count{3};

    struct line_position {
        section which{};
        size_t offset{};
    };

    struct image_layout {
        std::vector<line_position> positions;
        std::array<size_t, section_count> sizes{};
        std::array<size_t, section_count> alignments{};
        std::array<size_t, section_count> bases{};
    };

    struct symbol_table {
        // labels and constants
        std::unordered_map<std::string_view, int64_t> values;
        // numeric labels as line index and address, in line order
        std::unordered_map<std::string_view,
                           std::vector<std::pair<size_t, int64_t>>>
            local_labels;
    };

    std::vector<record> records_;

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
    [[nodiscard]] static auto li_value_size_bytes(const int64_t value)
        -> size_t {

        const uint32_t bits{static_cast<uint32_t>(value)};
        const int32_t number{std::bit_cast<int32_t>(bits)};
        if ((number >= immediate_min and number <= immediate_max) or
            (bits & low_mask) == 0) {

            return one_instruction_bytes;
        }

        return two_instructions_bytes;
    }

    [[nodiscard]] static auto li_size_bytes(const std::string_view arguments)
        -> size_t {

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

        return li_value_size_bytes(parsed);
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

    // the table is inside a function because 'op_info' default member
    // initializers are usable only once the class is complete
    [[nodiscard]] static auto info(const op code) -> const op_info& {
        // indexed by 'op'
        static constexpr std::array<op_info, op_count> infos{{
            {.mnemonic{"add"}, .operands{form::registers}, .encoding{0x33}},
            {.mnemonic{"sub"},
             .operands{form::registers},
             .encoding{0x40000033}},
            {.mnemonic{"sll"}, .operands{form::registers}, .encoding{0x1033}},
            {.mnemonic{"slt"}, .operands{form::registers}, .encoding{0x2033}},
            {.mnemonic{"sltu"}, .operands{form::registers}, .encoding{0x3033}},
            {.mnemonic{"xor"}, .operands{form::registers}, .encoding{0x4033}},
            {.mnemonic{"srl"}, .operands{form::registers}, .encoding{0x5033}},
            {.mnemonic{"sra"},
             .operands{form::registers},
             .encoding{0x40005033}},
            {.mnemonic{"or"}, .operands{form::registers}, .encoding{0x6033}},
            {.mnemonic{"and"}, .operands{form::registers}, .encoding{0x7033}},
            {.mnemonic{"addi"}, .operands{form::immediate}, .encoding{0x13}},
            {.mnemonic{"slti"}, .operands{form::immediate}, .encoding{0x2013}},
            {.mnemonic{"sltiu"}, .operands{form::immediate}, .encoding{0x3013}},
            {.mnemonic{"xori"}, .operands{form::immediate}, .encoding{0x4013}},
            {.mnemonic{"ori"}, .operands{form::immediate}, .encoding{0x6013}},
            {.mnemonic{"andi"}, .operands{form::immediate}, .encoding{0x7013}},
            {.mnemonic{"slli"}, .operands{form::shift}, .encoding{0x1013}},
            {.mnemonic{"srli"}, .operands{form::shift}, .encoding{0x5013}},
            {.mnemonic{"srai"}, .operands{form::shift}, .encoding{0x40005013}},
            {.mnemonic{"lui"}, .operands{form::upper}, .encoding{0x37}},
            {.mnemonic{"lb"}, .operands{form::load}, .encoding{0x03}},
            {.mnemonic{"lh"}, .operands{form::load}, .encoding{0x1003}},
            {.mnemonic{"lw"}, .operands{form::load}, .encoding{0x2003}},
            {.mnemonic{"lbu"}, .operands{form::load}, .encoding{0x4003}},
            {.mnemonic{"lhu"}, .operands{form::load}, .encoding{0x5003}},
            {.mnemonic{"sb"}, .operands{form::store}, .encoding{0x23}},
            {.mnemonic{"sh"}, .operands{form::store}, .encoding{0x1023}},
            {.mnemonic{"sw"}, .operands{form::store}, .encoding{0x2023}},
            {.mnemonic{"beq"}, .operands{form::branch}, .encoding{0x63}},
            {.mnemonic{"bne"}, .operands{form::branch}, .encoding{0x1063}},
            {.mnemonic{"blt"}, .operands{form::branch}, .encoding{0x4063}},
            {.mnemonic{"bge"}, .operands{form::branch}, .encoding{0x5063}},
            {.mnemonic{"bltu"}, .operands{form::branch}, .encoding{0x6063}},
            {.mnemonic{"bgeu"}, .operands{form::branch}, .encoding{0x7063}},
            {.mnemonic{"bgt"},
             .operands{form::branch},
             .encoding{0x4063},
             .swapped{true}},
            {.mnemonic{"ble"},
             .operands{form::branch},
             .encoding{0x5063},
             .swapped{true}},
            {.mnemonic{"bgtu"},
             .operands{form::branch},
             .encoding{0x6063},
             .swapped{true}},
            {.mnemonic{"bleu"},
             .operands{form::branch},
             .encoding{0x7063},
             .swapped{true}},
            {.mnemonic{"beqz"}, .operands{form::branch_zero}, .encoding{0x63}},
            {.mnemonic{"bnez"},
             .operands{form::branch_zero},
             .encoding{0x1063}},
            {.mnemonic{"bltz"},
             .operands{form::branch_zero},
             .encoding{0x4063}},
            {.mnemonic{"bgez"},
             .operands{form::branch_zero},
             .encoding{0x5063}},
            {.mnemonic{"bgtz"},
             .operands{form::branch_zero},
             .encoding{0x4063},
             .swapped{true}},
            {.mnemonic{"blez"},
             .operands{form::branch_zero},
             .encoding{0x5063},
             .swapped{true}},
            {.mnemonic{"ecall"}, .operands{form::system}, .encoding{0x73}},
            {.mnemonic{"ebreak"}, .operands{form::system}, .encoding{0x100073}},
            {.mnemonic{"li"}, .operands{form::load_immediate}, .encoding{0x13}},
            {.mnemonic{"la"}, .operands{form::load_address}, .encoding{0x17}},
            {.mnemonic{"mv"}, .operands{form::move}, .encoding{0x13}},
            {.mnemonic{"j"}, .operands{form::jump}, .encoding{0x6f}},
            {.mnemonic{"jr"}, .operands{form::jump_register}, .encoding{0x67}},
            {.mnemonic{"call"}, .operands{form::call}, .encoding{0x17}},
            {.mnemonic{"ret"}, .operands{form::ret}, .encoding{0x8067}},
        }};

        return infos.at(std::to_underlying(code));
    }

    [[nodiscard]] static auto find_op(const std::string_view mnemonic) -> op {
        for (size_t index{}; index < op_count; ++index) {
            const op code{static_cast<op>(index)};
            if (info(code).mnemonic == mnemonic) {
                return code;
            }
        }

        throw panic_exception{
            std::format("unknown instruction '{}'", mnemonic)};
    }

    [[nodiscard]] static auto number_of(const std::string_view name)
        -> uint8_t {

        const std::optional<uint8_t> number{register_number(name)};
        if (not number) {
            throw panic_exception{std::format("unknown register '{}'", name)};
        }

        return *number;
    }

    [[nodiscard]] static auto fits(const int64_t number, const int64_t min,
                                   const int64_t max) -> bool {

        return number >= min and number <= max;
    }

    [[nodiscard]] static auto immediate_text(const immediate& value)
        -> std::string {

        if (value.symbol.empty()) {
            return std::format("{}", value.number);
        }

        if (value.symbol_part == immediate::part::high) {
            return std::format("%hi({})", value.symbol);
        }

        if (value.symbol_part == immediate::part::low) {
            return std::format("%lo({})", value.symbol);
        }

        return value.symbol;
    }

    [[nodiscard]] static auto instruction_text(const instruction& ins,
                                               const spelling& names)
        -> std::string {

        const std::string_view mnemonic{info(ins.code).mnemonic};
        const form operands{info(ins.code).operands};

        if (operands == form::registers) {
            return std::format("{} {}, {}, {}", mnemonic, names.rd, names.rs1,
                               names.rs2);
        }

        if (operands == form::immediate or operands == form::shift) {
            return std::format("{} {}, {}, {}", mnemonic, names.rd, names.rs1,
                               immediate_text(ins.value));
        }

        if (operands == form::upper or operands == form::load_immediate or
            operands == form::load_address) {

            return std::format("{} {}, {}", mnemonic, names.rd,
                               immediate_text(ins.value));
        }

        if (operands == form::load) {
            return std::format("{} {}, {}({})", mnemonic, names.rd,
                               immediate_text(ins.value), names.rs1);
        }

        if (operands == form::store) {
            return std::format("{} {}, {}({})", mnemonic, names.rs2,
                               immediate_text(ins.value), names.rs1);
        }

        if (operands == form::branch) {
            return std::format("{} {}, {}, {}", mnemonic, names.rs1, names.rs2,
                               ins.target);
        }

        if (operands == form::branch_zero) {
            return std::format("{} {}, {}", mnemonic, names.rs1, ins.target);
        }

        if (operands == form::move) {
            return std::format("{} {}, {}", mnemonic, names.rd, names.rs1);
        }

        if (operands == form::jump_register) {
            return std::format("{} {}", mnemonic, names.rs1);
        }

        // 'ra' is the implied link register
        if (operands == form::call and ins.rd != return_address_register) {
            return std::format("{} {}, {}", mnemonic, names.rd, ins.target);
        }

        if (operands == form::jump or operands == form::call) {
            return std::format("{} {}", mnemonic, ins.target);
        }

        // 'ecall', 'ebreak' and 'ret' have no operands
        return std::string{mnemonic};
    }

    // an immediate out of range would assemble to a different instruction
    [[nodiscard]] static auto has_valid_immediate(const instruction& ins)
        -> bool {

        const immediate& value{ins.value};
        const bool symbolic{not value.symbol.empty()};
        const form operands{info(ins.code).operands};

        if (operands == form::immediate) {
            if (symbolic) {
                return value.symbol_part == immediate::part::low;
            }

            return fits(value.number, immediate_min, immediate_max);
        }

        if (operands == form::shift) {
            return not symbolic and fits(value.number, 0, shift_max);
        }

        if (operands == form::upper) {
            if (symbolic) {
                return value.symbol_part == immediate::part::high;
            }

            return fits(value.number, 0, upper_max);
        }

        if (operands == form::load or operands == form::store) {
            return not symbolic and
                   fits(value.number, immediate_min, immediate_max);
        }

        if (operands == form::load_immediate) {
            if (symbolic) {
                return value.symbol_part == immediate::part::whole;
            }

            return fits(value.number, std::numeric_limits<int32_t>::min(),
                        std::numeric_limits<uint32_t>::max());
        }

        if (operands == form::load_address) {
            return symbolic and value.symbol_part == immediate::part::whole;
        }

        // the remaining forms have no immediate
        return true;
    }

    // sizes under '.option norvc' and '.option norelax', matching the text
    // sizes
    [[nodiscard]] static auto encoded_size_bytes(const instruction& ins)
        -> size_t {

        const form operands{info(ins.code).operands};
        if (operands == form::load_immediate) {
            // a symbol's value is unknown until the end
            if (not ins.value.symbol.empty()) {
                return two_instructions_bytes;
            }

            return li_value_size_bytes(ins.value.number);
        }

        if (operands == form::load_address or operands == form::call) {
            return two_instructions_bytes;
        }

        return one_instruction_bytes;
    }

    // written lines are not assembled again, so they keep no record
    [[nodiscard]] auto store_record(record structured)
        -> std::optional<size_t> {

        if (not is_buffering()) {
            return std::nullopt;
        }

        records_.push_back(std::move(structured));

        return records_.size() - 1;
    }

    auto add_record(std::string text, const size_t code_size, record structured)
        -> void {

        const std::optional<size_t> index{store_record(std::move(structured))};
        add_record_line(std::move(text), code_size, index);
    }

    auto add_instruction(const size_t indent, instruction ins,
                         const spelling& names) -> void {

        if (not has_valid_immediate(ins)) {
            throw panic_exception{std::format("immediate out of range in '{}'",
                                              instruction_text(ins, names))};
        }

        std::string text{indentation(indent) + instruction_text(ins, names)};
        const size_t size_bytes{encoded_size_bytes(ins)};
        add_record(std::move(text), size_bytes, std::move(ins));
    }

    [[nodiscard]] static auto section_directive(const section which)
        -> std::string_view {

        constexpr std::array<std::string_view, section_count> directives{
            ".text",
            ".section .rodata",
            ".data",
        };

        return directives.at(section_index(which));
    }

    [[nodiscard]] static auto data_directive(const size_t element_size_bytes)
        -> std::string_view {

        switch (element_size_bytes) {
        case 1:
            return ".byte";

        case 2:
            return ".half";

        case 4:
            return ".word";

        default:
            throw panic_exception{
                std::format("no rv32i data directive for {} byte elements",
                            element_size_bytes)};
        }
    }

    // applies the innermost operation first, wrapping like the assembler
    [[nodiscard]] static auto
    evaluate_unary_operations(const std::string_view operations,
                              const int64_t value) -> int64_t {

        uint64_t bits{static_cast<uint64_t>(value)};
        for (const char operation : operations | std::views::reverse) {
            if (operation == '-') {
                bits = uint64_t{} - bits;
                continue;
            }

            if (operation != '~') {
                throw panic_exception{
                    std::format("unknown unary operation '{}'", operation)};
            }

            bits = ~bits;
        }

        return static_cast<int64_t>(bits);
    }

    [[nodiscard]] static auto ascii_text(const std::string_view bytes)
        -> std::string {

        std::string encoded;
        for (const char character : bytes) {
            const unsigned char byte{static_cast<unsigned char>(character)};
            switch (byte) {
            case '\n':
                encoded += "\\n";
                break;

            case '\r':
                encoded += "\\r";
                break;

            case '\t':
                encoded += "\\t";
                break;

            case '"':
            case '\\':
                encoded += '\\';
                encoded += character;
                break;

            default:
                if (byte >= ' ' and byte <= '~') {
                    encoded += character;
                    break;
                }

                encoded +=
                    std::format("\\{:03o}", static_cast<unsigned int>(byte));
                break;
            }
        }

        return encoded;
    }

    [[nodiscard]] auto record_of(const line& l) const -> const record* {
        if (not l.record) {
            return nullptr;
        }

        return &records_.at(*l.record);
    }

    // note: fields follow the instruction formats of the rv32i specification

    [[nodiscard]] static auto register_fields(const uint8_t rd,
                                              const uint8_t rs1,
                                              const uint8_t rs2) -> uint32_t {

        const uint32_t rd_field{uint32_t{rd} << 7U};
        const uint32_t rs1_field{uint32_t{rs1} << 15U};
        const uint32_t rs2_field{uint32_t{rs2} << 20U};

        return rd_field | rs1_field | rs2_field;
    }

    [[nodiscard]] static auto
    encode_immediate(const uint32_t encoding, const uint8_t rd,
                     const uint8_t rs1, const int64_t value) -> uint32_t {

        const uint32_t imm_11_0{(static_cast<uint32_t>(value) & low_mask)
                                << 20U};

        return encoding | imm_11_0 | register_fields(rd, rs1, zero_register);
    }

    [[nodiscard]] static auto encode_store(const uint32_t encoding,
                                           const uint8_t base,
                                           const uint8_t src,
                                           const int64_t offset) -> uint32_t {

        const uint32_t bits{static_cast<uint32_t>(offset)};
        const uint32_t imm_11_5{((bits >> 5U) & 0x7fU) << 25U};
        const uint32_t imm_4_0{(bits & 0x1fU) << 7U};

        return encoding | imm_11_5 | imm_4_0 |
               register_fields(zero_register, base, src);
    }

    [[nodiscard]] static auto encode_branch(const uint32_t encoding,
                                            const uint8_t rs1,
                                            const uint8_t rs2,
                                            const int64_t offset) -> uint32_t {

        if (not fits(offset, branch_min, branch_max) or offset % 2 != 0) {
            throw panic_exception{
                std::format("branch offset {} out of range", offset)};
        }

        const uint32_t bits{static_cast<uint32_t>(offset)};
        const uint32_t imm_12{((bits >> 12U) & 1U) << 31U};
        const uint32_t imm_10_5{((bits >> 5U) & 0x3fU) << 25U};
        const uint32_t imm_4_1{((bits >> 1U) & 0xfU) << 8U};
        const uint32_t imm_11{((bits >> 11U) & 1U) << 7U};

        return encoding | imm_12 | imm_10_5 | imm_4_1 | imm_11 |
               register_fields(zero_register, rs1, rs2);
    }

    [[nodiscard]] static auto encode_upper(const uint32_t encoding,
                                           const uint8_t rd,
                                           const uint32_t upper) -> uint32_t {

        const uint32_t imm_31_12{(upper & upper_mask) << 12U};

        return encoding | imm_31_12 |
               register_fields(rd, zero_register, zero_register);
    }

    [[nodiscard]] static auto encode_jal(const uint8_t rd, const int64_t offset)
        -> uint32_t {

        if (not fits(offset, jal_min, jal_max) or offset % 2 != 0) {
            throw panic_exception{
                std::format("jump offset {} out of range", offset)};
        }

        const uint32_t bits{static_cast<uint32_t>(offset)};
        const uint32_t imm_20{((bits >> 20U) & 1U) << 31U};
        const uint32_t imm_10_1{((bits >> 1U) & 0x3ffU) << 21U};
        const uint32_t imm_11{((bits >> 11U) & 1U) << 20U};
        const uint32_t imm_19_12{((bits >> 12U) & 0xffU) << 12U};

        return jal_encoding | imm_20 | imm_10_1 | imm_11 | imm_19_12 |
               register_fields(rd, zero_register, zero_register);
    }

    // '%hi' rounds up when the sign-extended '%lo' is negative
    [[nodiscard]] static auto upper_part(const int64_t value) -> uint32_t {
        constexpr uint32_t rounding{0x800};
        constexpr unsigned low_bits{12};

        return ((static_cast<uint32_t>(value) + rounding) >> low_bits) &
               upper_mask;
    }

    [[nodiscard]] static auto lower_part(const int64_t value) -> int32_t {
        constexpr int32_t low_range{4096};

        const int32_t low{
            static_cast<int32_t>(static_cast<uint32_t>(value) & low_mask)};

        if (low > immediate_max) {
            return low - low_range;
        }

        return low;
    }

    [[nodiscard]] static auto branch_word(const op code, const uint8_t first,
                                          const uint8_t second,
                                          const int64_t offset) -> uint32_t {

        const op_info& details{info(code)};
        if (details.swapped) {
            return encode_branch(details.encoding, second, first, offset);
        }

        return encode_branch(details.encoding, first, second, offset);
    }

    // 'auipc' and 'second' reach 'distance' from the 'auipc'
    [[nodiscard]] static auto pc_relative(const uint32_t second_encoding,
                                          const uint8_t rd, const uint8_t base,
                                          const int64_t distance)
        -> std::vector<uint32_t> {

        if (not std::in_range<int32_t>(distance)) {
            throw panic_exception{
                std::format("distance {} exceeds 2 GiB", distance)};
        }

        return {
            encode_upper(auipc_encoding, base, upper_part(distance)),
            encode_immediate(second_encoding, rd, base, lower_part(distance)),
        };
    }

    // the sequence 'li' expands to, both instructions for a symbol since its
    // size was fixed before its value was known
    [[nodiscard]] static auto
    load_immediate(const uint8_t rd, const int64_t value, const bool symbolic)
        -> std::vector<uint32_t> {

        const int32_t number{
            std::bit_cast<int32_t>(static_cast<uint32_t>(value))};

        if (not symbolic and fits(number, immediate_min, immediate_max)) {
            return {encode_immediate(addi_encoding, rd, zero_register, number)};
        }

        const uint32_t upper{encode_upper(lui_encoding, rd, upper_part(value))};
        const int32_t lower{lower_part(value)};
        if (not symbolic and lower == 0) {
            return {upper};
        }

        return {upper, encode_immediate(addi_encoding, rd, rd, lower)};
    }

    [[nodiscard]] static auto is_local_label(const std::string_view name)
        -> bool {

        return not name.empty() and
               std::ranges::all_of(name, [](const char c) -> bool {
                   return c >= '0' and c <= '9';
               });
    }

    // 'Nf' and 'Nb' refer to the nearest numeric label 'N' after or before
    // the line
    [[nodiscard]] static auto symbol_value(const symbol_table& symbols,
                                           const std::string_view name,
                                           const size_t line_index) -> int64_t {

        const auto found{symbols.values.find(name)};
        if (found != symbols.values.end()) {
            return found->second;
        }

        const auto locals{
            symbols.local_labels.find(name.substr(0, name.size() - 1))};

        if (name.size() < 2 or locals == symbols.local_labels.end()) {
            throw panic_exception{std::format("undefined symbol '{}'", name)};
        }

        const std::vector<std::pair<size_t, int64_t>>& definitions{
            locals->second};

        const auto line_of = &std::pair<size_t, int64_t>::first;

        if (name.back() == 'f') {
            const auto next{
                std::ranges::upper_bound(definitions, line_index, {}, line_of)};
            if (next != definitions.end()) {
                return next->second;
            }
        }

        if (name.back() == 'b') {
            const auto next{
                std::ranges::lower_bound(definitions, line_index, {}, line_of)};
            if (next != definitions.begin()) {
                return std::prev(next)->second;
            }
        }

        throw panic_exception{std::format("undefined symbol '{}'", name)};
    }

    [[nodiscard]] static auto immediate_value(const symbol_table& symbols,
                                              const immediate& value,
                                              const size_t line_index)
        -> int64_t {

        if (value.symbol.empty()) {
            return value.number;
        }

        const int64_t resolved{symbol_value(symbols, value.symbol, line_index)};

        if (value.symbol_part == immediate::part::high) {
            return upper_part(resolved);
        }

        if (value.symbol_part == immediate::part::low) {
            return lower_part(resolved);
        }

        return resolved;
    }

    [[nodiscard]] static auto
    encode_instruction(const instruction& ins, const size_t line_index,
                       const int64_t address, const symbol_table& symbols)
        -> std::vector<uint32_t> {

        const op_info& details{info(ins.code)};
        const form operands{details.operands};
        const int64_t value{immediate_value(symbols, ins.value, line_index)};

        if (operands == form::registers) {
            return {details.encoding |
                    register_fields(ins.rd, ins.rs1, ins.rs2)};
        }

        if (operands == form::immediate or operands == form::shift or
            operands == form::load) {

            return {encode_immediate(details.encoding, ins.rd, ins.rs1, value)};
        }

        if (operands == form::upper) {
            return {encode_upper(details.encoding, ins.rd,
                                 static_cast<uint32_t>(value))};
        }

        if (operands == form::store) {
            return {encode_store(details.encoding, ins.rs1, ins.rs2, value)};
        }

        if (operands == form::system or operands == form::ret) {
            return {details.encoding};
        }

        if (operands == form::load_immediate) {
            return load_immediate(ins.rd, value, not ins.value.symbol.empty());
        }

        if (operands == form::load_address) {
            return pc_relative(addi_encoding, ins.rd, ins.rd, value - address);
        }

        if (operands == form::move) {
            return {encode_immediate(addi_encoding, ins.rd, ins.rs1, 0)};
        }

        if (operands == form::jump_register) {
            return {
                encode_immediate(details.encoding, zero_register, ins.rs1, 0)};
        }

        // the remaining forms go to a label
        const int64_t distance{symbol_value(symbols, ins.target, line_index) -
                               address};

        if (operands == form::jump) {
            return {encode_jal(zero_register, distance)};
        }

        if (operands == form::call) {
            return pc_relative(jalr_encoding, ins.rd, ins.rd, distance);
        }

        return {branch_word(ins.code, ins.rs1, ins.rs2, distance)};
    }

    [[nodiscard]] static auto long_jump(const jump_reach reach_form,
                                        const jump_info& jump,
                                        const int64_t distance)
        -> std::vector<uint32_t> {

        if (reach_form == jump_reach::far) {
            return pc_relative(jalr_encoding, zero_register,
                               number_of(jump.scratch), distance);
        }

        return {encode_jal(zero_register, distance)};
    }

    // the forms 'write_line' writes for the jump's grown size
    [[nodiscard]] auto encode_jump(const line& l, const size_t line_index,
                                   const int64_t address,
                                   const symbol_table& symbols) const
        -> std::vector<uint32_t> {

        const jump_info& jump{*l.jump};
        const int64_t target{symbol_value(symbols, jump.target, line_index)};
        const jump_reach reach_form{reach(l)};
        if (not is_conditional(jump)) {
            return long_jump(reach_form, jump, target - address);
        }

        const auto* const compared{std::get_if<jump_registers>(record_of(l))};
        if (compared == nullptr) {
            throw panic_exception{
                std::format("no registers for '{}'", trim(l.text))};
        }

        if (reach_form == jump_reach::branch) {
            return {branch_word(find_op(jump.mnemonic), compared->rs1,
                                compared->rs2, target - address)};
        }

        const std::optional<std::string_view> inverted{inverse(jump.mnemonic)};
        if (not inverted) {
            throw panic_exception{
                std::format("no inverse for '{}'", jump.mnemonic)};
        }

        // the inverted branch skips the jump that follows it
        std::vector<uint32_t> words{
            branch_word(find_op(*inverted), compared->rs1, compared->rs2,
                        static_cast<int64_t>(l.code_size))};

        const int64_t jump_address{address +
                                   static_cast<int64_t>(one_instruction_bytes)};

        std::ranges::copy(long_jump(reach_form, jump, target - jump_address),
                          std::back_inserter(words));

        return words;
    }

    [[nodiscard]] static auto section_index(const section which) -> size_t {
        return std::to_underlying(which);
    }

    [[nodiscard]] static auto align_up(const size_t value,
                                       const size_t alignment_bytes) -> size_t {

        return (value + alignment_bytes - 1) / alignment_bytes *
               alignment_bytes;
    }

    [[nodiscard]] static auto data_size_bytes(const data_values& data)
        -> size_t {

        return data.element_size_bytes * data.values.size() * data.repeat_count;
    }

    [[nodiscard]] auto image_size_bytes(const line& l) const -> size_t {
        const auto* const data{std::get_if<data_values>(record_of(l))};
        if (data == nullptr) {
            return l.code_size;
        }

        return data_size_bytes(*data);
    }

    // sections follow each other in the order text, rodata, data from
    // address zero, like a linker script listing them in that order
    [[nodiscard]] auto layout_image() const -> image_layout {
        image_layout image;
        image.positions.reserve(lines().size());
        image.alignments = {one_instruction_bytes, 1, 1};

        section current{section::text};
        for (const line& l : lines()) {
            const record* const structured{record_of(l)};

            const auto* const start{std::get_if<section_start>(structured)};
            if (start != nullptr) {
                current = start->which;
            }

            const size_t index{section_index(current)};
            size_t& offset{image.sizes.at(index)};

            const auto* const align{std::get_if<alignment>(structured)};
            if (align != nullptr) {
                offset = align_up(offset, align->size_bytes);

                image.alignments.at(index) =
                    std::max(image.alignments.at(index), align->size_bytes);
            }

            image.positions.push_back({
                .which{current},
                .offset{offset},
            });

            offset += image_size_bytes(l);
        }

        const size_t text{section_index(section::text)};
        const size_t rodata{section_index(section::rodata)};
        const size_t data{section_index(section::data)};

        image.bases.at(rodata) =
            align_up(image.sizes.at(text), image.alignments.at(rodata));

        image.bases.at(data) =
            align_up(image.bases.at(rodata) + image.sizes.at(rodata),
                     image.alignments.at(data));

        return image;
    }

    [[nodiscard]] auto collect_symbols(const image_layout& image) const
        -> symbol_table {

        symbol_table symbols;
        for (size_t index{}; index < lines().size(); ++index) {
            const line& l{lines()[index]};

            const auto* const defined{std::get_if<constant>(record_of(l))};
            if (defined != nullptr) {
                symbols.values.emplace(defined->name, defined->value);
            }

            if (l.label.empty()) {
                continue;
            }

            const line_position& position{image.positions[index]};

            const int64_t address{static_cast<int64_t>(
                image.bases.at(section_index(position.which)) +
                position.offset)};

            if (is_local_label(l.label)) {
                symbols.local_labels[l.label].emplace_back(index, address);
                continue;
            }

            symbols.values.emplace(l.label, address);
        }

        return symbols;
    }

    static auto write_zeros(std::ostream& os, size_t count) -> void {
        static constexpr std::array<char, 4096> zeros{};
        while (count != 0) {
            const size_t chunk{std::min(count, zeros.size())};
            os.write(zeros.data(), static_cast<std::streamsize>(chunk));
            count -= chunk;
        }
    }

    // little endian regardless of the host
    static auto write_value(std::ostream& os, const uint64_t value,
                            const size_t size_bytes) -> void {

        constexpr unsigned byte_bits{8};
        constexpr uint64_t byte_mask{0xff};

        for (size_t byte{}; byte < size_bytes; ++byte) {
            os.put(
                static_cast<char>((value >> (byte * byte_bits)) & byte_mask));
        }
    }

    static auto write_data(std::ostream& os, const data_values& data) -> void {
        // zero fills such as the variables area can be large
        if (std::ranges::all_of(
                data.values, [](const int64_t v) -> bool { return v == 0; })) {
            write_zeros(os, data_size_bytes(data));

            return;
        }

        for (size_t repeat{}; repeat < data.repeat_count; ++repeat) {
            for (const int64_t value : data.values) {
                write_value(os, static_cast<uint64_t>(value),
                            data.element_size_bytes);
            }
        }
    }

    auto write_code(std::ostream& os, const line& l, const size_t line_index,
                    const int64_t address, const symbol_table& symbols) const
        -> void {

        if (l.code_size == 0) {
            return;
        }

        const auto* const ins{std::get_if<instruction>(record_of(l))};
        if (not l.jump and ins == nullptr) {
            throw panic_exception{
                std::format("no binary form for '{}'", trim(l.text))};
        }

        const std::vector<uint32_t> words{
            l.jump ? encode_jump(l, line_index, address, symbols)
                   : encode_instruction(*ins, line_index, address, symbols)};

        assert(words.size() * one_instruction_bytes == l.code_size);

        for (const uint32_t word : words) {
            write_value(os, word, one_instruction_bytes);
        }
    }

    auto write_image(std::ostream& os) const -> void {
        const image_layout image{layout_image()};
        const symbol_table symbols{collect_symbols(image)};

        size_t written{};
        for (const section which :
             {section::text, section::rodata, section::data}) {

            const size_t base{image.bases.at(section_index(which))};
            for (size_t index{}; index < lines().size(); ++index) {
                const line_position& position{image.positions[index]};
                if (position.which != which) {
                    continue;
                }

                // alignment gaps and the gaps between sections are zero
                const size_t address{base + position.offset};
                write_zeros(os, address - written);

                const line& l{lines()[index]};
                const auto* const data{std::get_if<data_values>(record_of(l))};
                if (data != nullptr) {
                    write_data(os, *data);
                    written = address + data_size_bytes(*data);
                    continue;
                }

                write_code(os, l, index, static_cast<int64_t>(address),
                           symbols);

                written = address + l.code_size;
            }

            // a trailing alignment still belongs to the section
            const size_t end{base + image.sizes.at(section_index(which))};
            write_zeros(os, end - written);
            written = end;
        }
    }

    auto resolve_jumps() -> void {
        assert(not is_capturing());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines()};

        // sizes only grow, so this ends once every jump reaches its target
        bool grown{true};
        while (grown) {
            grown = grow_out_of_reach(labels);
        }
    }

    auto write_text(std::ostream& os) const -> void {
        size_t skip_count{};
        for (const line& l : lines()) {
            write_line(os, l, skip_count);
        }
    }

    auto clear() -> void {
        lines().clear();
        records_.clear();
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

    [[nodiscard]] static auto indentation(const size_t indent) -> std::string {
        std::string text;
        text.resize(indent * 4, ' ');

        return text;
    }

    // note: registers are passed by name, as backend operands hold them

    auto register_op(const size_t indent, const op code,
                     const std::string_view rd, const std::string_view rs1,
                     const std::string_view rs2) -> void {

        assert(info(code).operands == form::registers);

        add_instruction(indent,
                        {
                            .code{code},
                            .rd{number_of(rd)},
                            .rs1{number_of(rs1)},
                            .rs2{number_of(rs2)},
                            .value{},
                            .target{},
                        },
                        {.rd{rd}, .rs1{rs1}, .rs2{rs2}});
    }

    auto immediate_op(const size_t indent, const op code,
                      const std::string_view rd, const std::string_view rs1,
                      immediate value) -> void {

        assert(info(code).operands == form::immediate or
               info(code).operands == form::shift);

        add_instruction(indent,
                        {
                            .code{code},
                            .rd{number_of(rd)},
                            .rs1{number_of(rs1)},
                            .rs2{},
                            .value{std::move(value)},
                            .target{},
                        },
                        {.rd{rd}, .rs1{rs1}, .rs2{}});
    }

    auto load(const size_t indent, const op code, const std::string_view rd,
              immediate offset, const std::string_view base) -> void {

        assert(info(code).operands == form::load);

        add_instruction(indent,
                        {
                            .code{code},
                            .rd{number_of(rd)},
                            .rs1{number_of(base)},
                            .rs2{},
                            .value{std::move(offset)},
                            .target{},
                        },
                        {.rd{rd}, .rs1{base}, .rs2{}});
    }

    auto store(const size_t indent, const op code, const std::string_view src,
               immediate offset, const std::string_view base) -> void {

        assert(info(code).operands == form::store);

        add_instruction(indent,
                        {
                            .code{code},
                            .rd{},
                            .rs1{number_of(base)},
                            .rs2{number_of(src)},
                            .value{std::move(offset)},
                            .target{},
                        },
                        {.rd{}, .rs1{base}, .rs2{src}});
    }

    // a branch to a numeric label such as '1f', which is never grown
    auto branch(const size_t indent, const op code, const std::string_view rs1,
                const std::string_view rs2, const std::string_view target)
        -> void {

        assert(info(code).operands == form::branch);

        add_instruction(indent,
                        {
                            .code{code},
                            .rd{},
                            .rs1{number_of(rs1)},
                            .rs2{number_of(rs2)},
                            .value{},
                            .target{std::string{target}},
                        },
                        {.rd{}, .rs1{rs1}, .rs2{rs2}});
    }

    auto branch_zero(const size_t indent, const op code,
                     const std::string_view rs, const std::string_view target)
        -> void {

        assert(info(code).operands == form::branch_zero);

        add_instruction(indent,
                        {
                            .code{code},
                            .rd{},
                            .rs1{number_of(rs)},
                            .rs2{zero_register},
                            .value{},
                            .target{std::string{target}},
                        },
                        {.rd{}, .rs1{rs}, .rs2{}});
    }

    auto add(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::add, rd, rs1, rs2);
    }

    auto sub(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::sub, rd, rs1, rs2);
    }

    auto sll(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::sll, rd, rs1, rs2);
    }

    auto slt(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::slt, rd, rs1, rs2);
    }

    auto sltu(const size_t indent, const std::string_view rd,
              const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::sltu, rd, rs1, rs2);
    }

    auto xor_op(const size_t indent, const std::string_view rd,
                const std::string_view rs1, const std::string_view rs2)
        -> void {
        register_op(indent, op::xor_op, rd, rs1, rs2);
    }

    auto srl(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::srl, rd, rs1, rs2);
    }

    auto sra(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::sra, rd, rs1, rs2);
    }

    auto or_op(const size_t indent, const std::string_view rd,
               const std::string_view rs1, const std::string_view rs2) -> void {
        register_op(indent, op::or_op, rd, rs1, rs2);
    }

    auto and_op(const size_t indent, const std::string_view rd,
                const std::string_view rs1, const std::string_view rs2)
        -> void {
        register_op(indent, op::and_op, rd, rs1, rs2);
    }

    auto addi(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::addi, rd, rs1, std::move(value));
    }

    auto slti(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::slti, rd, rs1, std::move(value));
    }

    auto sltiu(const size_t indent, const std::string_view rd,
               const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::sltiu, rd, rs1, std::move(value));
    }

    auto xori(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::xori, rd, rs1, std::move(value));
    }

    auto ori(const size_t indent, const std::string_view rd,
             const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::ori, rd, rs1, std::move(value));
    }

    auto andi(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::andi, rd, rs1, std::move(value));
    }

    auto slli(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::slli, rd, rs1, std::move(value));
    }

    auto srli(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::srli, rd, rs1, std::move(value));
    }

    auto srai(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {
        immediate_op(indent, op::srai, rd, rs1, std::move(value));
    }

    auto lui(const size_t indent, const std::string_view rd, immediate value)
        -> void {

        add_instruction(indent,
                        {
                            .code{op::lui},
                            .rd{number_of(rd)},
                            .rs1{},
                            .rs2{},
                            .value{std::move(value)},
                            .target{},
                        },
                        {.rd{rd}, .rs1{}, .rs2{}});
    }

    auto lb(const size_t indent, const std::string_view rd, immediate offset,
            const std::string_view base) -> void {
        load(indent, op::lb, rd, std::move(offset), base);
    }

    auto lh(const size_t indent, const std::string_view rd, immediate offset,
            const std::string_view base) -> void {
        load(indent, op::lh, rd, std::move(offset), base);
    }

    auto lw(const size_t indent, const std::string_view rd, immediate offset,
            const std::string_view base) -> void {
        load(indent, op::lw, rd, std::move(offset), base);
    }

    auto lbu(const size_t indent, const std::string_view rd, immediate offset,
             const std::string_view base) -> void {
        load(indent, op::lbu, rd, std::move(offset), base);
    }

    auto lhu(const size_t indent, const std::string_view rd, immediate offset,
             const std::string_view base) -> void {
        load(indent, op::lhu, rd, std::move(offset), base);
    }

    auto sb(const size_t indent, const std::string_view src, immediate offset,
            const std::string_view base) -> void {
        store(indent, op::sb, src, std::move(offset), base);
    }

    auto sh(const size_t indent, const std::string_view src, immediate offset,
            const std::string_view base) -> void {
        store(indent, op::sh, src, std::move(offset), base);
    }

    auto sw(const size_t indent, const std::string_view src, immediate offset,
            const std::string_view base) -> void {
        store(indent, op::sw, src, std::move(offset), base);
    }

    auto beq(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::beq, rs1, rs2, target);
    }

    auto bne(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bne, rs1, rs2, target);
    }

    auto blt(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::blt, rs1, rs2, target);
    }

    auto bge(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bge, rs1, rs2, target);
    }

    auto bltu(const size_t indent, const std::string_view rs1,
              const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bltu, rs1, rs2, target);
    }

    auto bgeu(const size_t indent, const std::string_view rs1,
              const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bgeu, rs1, rs2, target);
    }

    auto bgt(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bgt, rs1, rs2, target);
    }

    auto ble(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::ble, rs1, rs2, target);
    }

    auto bgtu(const size_t indent, const std::string_view rs1,
              const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bgtu, rs1, rs2, target);
    }

    auto bleu(const size_t indent, const std::string_view rs1,
              const std::string_view rs2, const std::string_view target)
        -> void {
        branch(indent, op::bleu, rs1, rs2, target);
    }

    auto beqz(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {
        branch_zero(indent, op::beqz, rs, target);
    }

    auto bnez(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {
        branch_zero(indent, op::bnez, rs, target);
    }

    auto bltz(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {
        branch_zero(indent, op::bltz, rs, target);
    }

    auto bgez(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {
        branch_zero(indent, op::bgez, rs, target);
    }

    auto bgtz(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {
        branch_zero(indent, op::bgtz, rs, target);
    }

    auto blez(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {
        branch_zero(indent, op::blez, rs, target);
    }

    auto ecall(const size_t indent) -> void {
        add_instruction(indent,
                        {
                            .code{op::ecall},
                            .rd{},
                            .rs1{},
                            .rs2{},
                            .value{},
                            .target{},
                        },
                        {});
    }

    auto ebreak(const size_t indent) -> void {
        add_instruction(indent,
                        {
                            .code{op::ebreak},
                            .rd{},
                            .rs1{},
                            .rs2{},
                            .value{},
                            .target{},
                        },
                        {});
    }

    auto li(const size_t indent, const std::string_view rd, immediate value)
        -> void {

        add_instruction(indent,
                        {
                            .code{op::li},
                            .rd{number_of(rd)},
                            .rs1{},
                            .rs2{},
                            .value{std::move(value)},
                            .target{},
                        },
                        {.rd{rd}, .rs1{}, .rs2{}});
    }

    auto la(const size_t indent, const std::string_view rd,
            const std::string_view symbol) -> void {

        add_instruction(indent,
                        {
                            .code{op::la},
                            .rd{number_of(rd)},
                            .rs1{},
                            .rs2{},
                            .value{immediate::of_symbol(symbol)},
                            .target{},
                        },
                        {.rd{rd}, .rs1{}, .rs2{}});
    }

    auto mv(const size_t indent, const std::string_view rd,
            const std::string_view rs) -> void {

        add_instruction(indent,
                        {
                            .code{op::mv},
                            .rd{number_of(rd)},
                            .rs1{number_of(rs)},
                            .rs2{},
                            .value{},
                            .target{},
                        },
                        {.rd{rd}, .rs1{rs}, .rs2{}});
    }

    // a jump to a numeric label such as '1b', which is never grown
    auto j(const size_t indent, const std::string_view target) -> void {
        add_instruction(indent,
                        {
                            .code{op::j},
                            .rd{},
                            .rs1{},
                            .rs2{},
                            .value{},
                            .target{std::string{target}},
                        },
                        {});
    }

    // routines returning with 'jr' may take the return address in 'link'
    auto call(const size_t indent, const std::string_view target,
              const std::string_view link = "ra") -> void {

        add_instruction(indent,
                        {
                            .code{op::call},
                            .rd{number_of(link)},
                            .rs1{},
                            .rs2{},
                            .value{},
                            .target{std::string{target}},
                        },
                        {.rd{link}, .rs1{}, .rs2{}});
    }

    auto jr(const size_t indent, const std::string_view rs) -> void {
        add_instruction(indent,
                        {
                            .code{op::jr},
                            .rd{},
                            .rs1{number_of(rs)},
                            .rs2{},
                            .value{},
                            .target{},
                        },
                        {.rd{}, .rs1{rs}, .rs2{}});
    }

    auto ret(const size_t indent) -> void {
        add_instruction(indent,
                        {
                            .code{op::ret},
                            .rd{},
                            .rs1{},
                            .rs2{},
                            .value{},
                            .target{},
                        },
                        {});
    }

    // an unconditional jump grown until it reaches 'target'
    auto resolved_jump(const size_t indent, const std::string_view target,
                       const std::string_view scratch) -> void {

        add_jump(std::format("{}j {}", indentation(indent), target), "j", {},
                 target, scratch);
    }

    // a branch grown until it reaches 'target'
    auto resolved_branch(const size_t indent, const op code,
                         const std::string_view rs1, const std::string_view rs2,
                         const std::string_view target,
                         const std::string_view scratch) -> void {

        assert(info(code).operands == form::branch);

        const std::string_view mnemonic{info(code).mnemonic};
        const std::string operands{std::format("{}, {}", rs1, rs2)};

        const std::optional<size_t> index{store_record(jump_registers{
            .rs1{number_of(rs1)},
            .rs2{number_of(rs2)},
        })};

        add_jump(std::format("{}{} {}, {}", indentation(indent), mnemonic,
                             operands, target),
                 mnemonic, operands, target, scratch, index);
    }

    auto label(const size_t indent, const std::string_view name) -> void {
        add_label(std::string{name},
                  std::format("{}{}:", indentation(indent), name));
    }

    // sizes only count in code, so code must be in the text section
    auto switch_section(const section which) -> void {
        set_code_section(which == section::text);

        add_record(std::string{section_directive(which)}, 0,
                   section_start{.which{which}});
    }

    auto define_constant(const std::string_view name, const int64_t value)
        -> void {

        add_record(std::format(".equ {}, {}", name, value), 0,
                   constant{
                       .name{std::string{name}},
                       .value{value},
                   });
    }

    auto align(const size_t size_bytes) -> void {
        add_record(std::format(".balign {}", size_bytes), 0,
                   alignment{.size_bytes{size_bytes}});
    }

    auto zero(const size_t size_bytes) -> void {
        add_record(std::format(".zero {}", size_bytes), 0,
                   data_values{
                       .element_size_bytes{1},
                       .values{0},
                       .repeat_count{size_bytes},
                   });
    }

    auto data(const size_t element_size_bytes,
              const std::span<const int64_t> values) -> void {

        std::string text{data_directive(element_size_bytes)};
        std::string_view separator{" "};
        for (const int64_t value : values) {
            text += std::format("{}{}", separator, value);
            separator = ", ";
        }

        add_record(std::move(text), 0,
                   data_values{
                       .element_size_bytes{element_size_bytes},
                       .values{values.begin(), values.end()},
                       .repeat_count{1},
                   });
    }

    // 'unary_operations' such as '-~' apply to 'value' from right to left
    auto repeated_data(const size_t element_size_bytes, const size_t count,
                       const std::string_view unary_operations,
                       const int64_t value) -> void {

        add_text(std::format(".rept {}", count));

        add_record(
            std::format("{} {}{}", data_directive(element_size_bytes),
                        unary_operations, value),
            0,
            data_values{
                .element_size_bytes{element_size_bytes},
                .values{evaluate_unary_operations(unary_operations, value)},
                .repeat_count{count},
            });

        add_text(".endr");
    }

    auto ascii(const std::string_view bytes) -> void {
        std::vector<int64_t> values;
        values.reserve(bytes.size());
        for (const char byte : bytes) {
            values.push_back(static_cast<unsigned char>(byte));
        }

        add_record(std::format(".ascii \"{}\"", ascii_text(bytes)), 0,
                   data_values{
                       .element_size_bytes{1},
                       .values{std::move(values)},
                       .repeat_count{1},
                   });
    }

    auto resolve_and_write(std::ostream& os) -> void {
        resolve_jumps();
        write_text(os);
        clear();
    }

    // 'binary' receives the sections as one image from address zero
    auto resolve_and_write(std::ostream& os, std::ostream& binary) -> void {
        resolve_jumps();
        write_text(os);
        write_image(binary);
        clear();
    }
};
