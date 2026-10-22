#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "assembler.hpp"

// buffers x86_64 output so its jumps can be optimized, nasm resolves the
// distances
//
// the backend emits every instruction and directive through the functions
// below so that the lines can later also be encoded to binary

class assembler_x86_64 final : public assembler {
  public:
    // 'and', 'or', 'xor' and 'not' are c++ operator names
    enum class op : uint8_t {
        mov,
        movsx,
        lea,
        add,
        sub,
        and_op,
        or_op,
        xor_op,
        cmp,
        test,
        imul,
        sal,
        sar,
        inc,
        dec,
        neg,
        not_op,
        idiv,
        div,
        push,
        pop,
        cqo,
        syscall,
        ret,
        rep_movsb,
        rep_stosb,
        repe_cmpsb,
        repe_cmpsq,
        cmpsb,
        cmpsw,
        cmpsd,
        cmpsq,
    };

    // suffixes of 'jcc' and 'setcc', 'nz' is spelled apart from 'ne' to keep
    // the handler text
    enum class condition : uint8_t { e, ne, l, le, g, ge, a, b, s, nz, ae, o };

    // 'variables' is the uninitialized section after the data
    enum class section : uint8_t { text, rodata, data, bss, variables };

    // '[symbol + base + index * scale + displacement]', zero 'size_bytes'
    // leaves the width to the other operand
    struct memory {
        std::string_view symbol;
        std::string_view base;
        std::string_view index;
        uint64_t scale{};
        int64_t displacement{};
        size_t size_bytes{};

        //
        // statics
        //

        [[nodiscard]] static auto of_base(const std::string_view base,
                                          const size_t size_bytes = {})
            -> memory {

            return {
                .symbol{},
                .base{base},
                .index{},
                .scale{},
                .displacement{},
                .size_bytes{size_bytes},
            };
        }

        [[nodiscard]] static auto of_symbol(const std::string_view name,
                                            const size_t size_bytes = {})
            -> memory {

            return {
                .symbol{name},
                .base{},
                .index{},
                .scale{},
                .displacement{},
                .size_bytes{size_bytes},
            };
        }
    };

    // nasm expression text, a number or symbols such as frame sizes
    struct immediate {
        // numbers of any integer type keep call sites free of conversions
        template <std::integral integral_t>
        explicit(false) immediate(const integral_t value)
            : expression{std::format("{}", value)} {}

        immediate() = default;

        std::string expression;
        // keeps a 64-bit field for an address placed by the linker
        bool strict_qword{};

        //
        // statics
        //

        [[nodiscard]] static auto of_expression(const std::string_view text,
                                                const bool strict_qword = {})
            -> immediate {

            immediate result;
            result.expression = text;
            result.strict_qword = strict_qword;
            return result;
        }
    };

    // a register by name, memory or an immediate
    using argument = std::variant<std::string_view, memory, immediate>;

  private:
    struct op_info {
        std::string_view mnemonic;
        size_t operand_count{};
    };

    static constexpr size_t op_count_{std::to_underlying(op::cmpsq) + 1};

  public:
    // the widths of the operands
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    //
    // overridden methods
    //

    auto comment(const size_t indent, const std::string_view text)
        -> void override {

        if (text.empty()) {
            add_text(comment_start(indent));
            return;
        }

        add_text(std::format("{} {}", comment_start(indent), text));
    }

    // 'location' is the source the comment is about
    auto comment(const size_t indent, const std::string_view location,
                 const std::string_view text) -> void override {

        add_text(
            std::format("{}[{}] {}", comment_start(indent), location, text));
    }

    //
    // class methods
    //

    auto align(const size_t size_bytes) -> void {
        add_directive(std::format("align {}", size_bytes));
    }

    auto bits64() -> void { add_directive("bits 64"); }

    auto call(const size_t indent, const std::string_view target) -> void {
        add_text(std::format("{}call {}", indentation(indent), target));
        mark_call(target);
    }

    auto data(const size_t element_size_bytes,
              const std::span<const int64_t> values) -> void {

        std::string text{
            std::format("{} ", data_directive(element_size_bytes)),
        };

        std::string_view separator;
        for (const int64_t value : values) {
            text += std::format("{}{}", separator, value);

            separator = ", ";
        }

        add_directive(std::move(text));
    }

    auto default_rel() -> void { add_directive("default rel"); }

    auto define_constant(const std::string_view name, const int64_t value)
        -> void {

        add_directive(std::format("{} equ {}", name, value));
    }

    // the bytes from label 'start' to this line
    auto define_length(const std::string_view name,
                       const std::string_view start) -> void {

        add_directive(std::format("{} equ $ - {}", name, start));
    }

    auto global(const std::string_view name) -> void {
        add_directive(std::format("global {}", name));
    }

    auto instruction(const size_t indent, const op code) -> void {
        assert(info(code).operand_count == 0);

        add_text(indentation(indent) + std::string{info(code).mnemonic});
    }

    auto instruction(const size_t indent, const op code, const argument& value)
        -> void {

        assert(info(code).operand_count == 1);

        add_text(std::format("{}{} {}", indentation(indent),
                             info(code).mnemonic, argument_text(value)));
    }

    auto instruction(const size_t indent, const op code, const argument& dst,
                     const argument& src) -> void {

        assert(info(code).operand_count == 2);

        add_text(std::format("{}{} {}, {}", indentation(indent),
                             info(code).mnemonic, argument_text(dst),
                             argument_text(src)));
    }

    auto jcc(const size_t indent, const condition cc,
             const std::string_view target) -> void {

        const std::string mnemonic{std::format("j{}", condition_suffix(cc))};

        add_jump(std::format("{}{} {}", indentation(indent), mnemonic, target),
                 mnemonic, {}, target, {});
    }

    // jumps go to the base as jumps so they can be optimized
    auto jmp(const size_t indent, const std::string_view target) -> void {
        add_jump(std::format("{}jmp {}", indentation(indent), target), "jmp",
                 {}, target, {});
    }

    auto repeated_data(const size_t element_size_bytes, const size_t count,
                       const int64_t value) -> void {

        for_each_chunk(count, [&](const size_t chunk) -> void {
            add_directive(std::format("times {} {} {}", chunk,
                                      data_directive(element_size_bytes),
                                      value));
        });
    }

    auto reserve(const size_t size_bytes) -> void {
        for_each_chunk(size_bytes, [&](const size_t chunk) -> void {
            add_directive(std::format("resb {}", chunk));
        });
    }

    auto setcc(const size_t indent, const condition cc, const argument& dst)
        -> void {

        add_text(std::format("{}set{} {}", indentation(indent),
                             condition_suffix(cc), argument_text(dst)));
    }

    // 'text' keeps its escapes such as '\n', nasm interprets them in
    // backquoted strings
    auto string_data(const std::string_view text) -> void {
        std::string line{"db `"};
        for (const auto [i, part] :
             std::views::enumerate(text | std::views::split('`'))) {

            if (i != 0) {
                line += "\\`";
            }

            line += std::string_view{part};
        }

        line += '`';

        add_directive(std::move(line));
    }

    // sizes only count in code, so code must be in the text section
    auto switch_section(const section which) -> void {
        set_code_section(which == section::text);

        add_directive(std::string{section_directive(which)});
    }

    //
    // statics
    //

    // the text between the brackets, also used in comments
    [[nodiscard]] static auto address_text(const memory& address)
        -> std::string {

        std::string memory_text{address.symbol};

        // a symbol is addressed relative to 'rip', without registers
        assert(address.symbol.empty() or
               (address.base.empty() and address.index.empty()));

        if (not address.base.empty()) {
            memory_text += address.base;
        }

        if (not address.index.empty()) {
            if (not memory_text.empty()) {
                memory_text += " + ";
            }

            memory_text += address.index;

            if (address.scale > 1) {
                memory_text += std::format(" * {}", address.scale);
            }
        }

        if (address.displacement == 0) {
            return memory_text;
        }

        if (not memory_text.empty()) {
            memory_text += address.displacement > 0 ? " + " : " - ";
        }

        const uint64_t magnitude{
            address.displacement < 0
                ? uint64_t{} - static_cast<uint64_t>(address.displacement)
                : static_cast<uint64_t>(address.displacement),
        };

        memory_text += std::format("{}", magnitude);

        return memory_text;
    }

  private:
    //
    // overridden methods
    //

    [[nodiscard]] auto format_jump(const jump_info& jump) const
        -> std::string override {

        return std::format("{} {}", jump.mnemonic, jump.target);
    }

    // every instruction counts as one
    [[nodiscard]] auto instructions_in(const size_t code_size) const
        -> size_t override {

        return code_size;
    }

    [[nodiscard]] auto
    inverse_branch_mnemonic(const std::string_view mnemonic) const
        -> std::string_view override {

        constexpr std::array<mnemonic_pair, 8> pairs{
            {
                {"je", "jne"},
                {"jg", "jle"},
                {"jge", "jl"},
                {"jo", "jno"},
                {"ja", "jbe"},
                {"jb", "jae"},
                {"js", "jns"},
                {"jnz", "jz"},
            },
        };

        return paired_mnemonic(pairs, mnemonic);
    }

    // every instruction counts as one
    [[nodiscard]] auto text_code_size(const std::string_view text) const
        -> size_t override {

        // note: the directives are added with size 0 by 'add_directive'
        return code_before(text, ";").empty() ? 0 : 1;
    }

    [[nodiscard]] auto unconditional_jump_mnemonic() const
        -> std::string_view override {

        return "jmp";
    }

    //
    // class methods
    //

    // a directive emits no instruction, e.g. 'align 16' or 'x equ 3'
    auto add_directive(std::string text) -> void {
        add_record_line(std::move(text), 0, std::nullopt);
    }

    //
    // statics
    //

    [[nodiscard]] static auto argument_text(const argument& value)
        -> std::string {

        if (const std::string_view* const name{
                std::get_if<std::string_view>(&value),
            }) {

            return std::string{*name};
        }

        if (const immediate* const number{std::get_if<immediate>(&value)}) {
            if (number->strict_qword) {
                return std::format("strict qword {}", number->expression);
            }

            return number->expression;
        }

        const memory& address{std::get<memory>(value)};

        if (address.size_bytes == 0) {
            return std::format("[{}]", address_text(address));
        }

        return std::format("{} [{}]", size_specifier(address.size_bytes),
                           address_text(address));
    }

    // the marker replaces the first spaces of the indentation
    [[nodiscard]] static auto comment_start(const size_t indent)
        -> std::string {

        std::string text{";"};

        if (indent != 0) {
            text += "   ";
        }

        for (size_t i{1}; i < indent; ++i) {
            text += "    ";
        }

        return text;
    }

    [[nodiscard]] static auto condition_suffix(const condition cc)
        -> std::string_view {

        // indexed by 'condition'
        constexpr auto suffixes{
            std::to_array<std::string_view>({
                "e",
                "ne",
                "l",
                "le",
                "g",
                "ge",
                "a",
                "b",
                "s",
                "nz",
                "ae",
                "o",
            }),
        };

        static_assert(suffixes.size() == std::to_underlying(condition::o) + 1);

        return suffixes.at(std::to_underlying(cc));
    }

    [[nodiscard]] static auto data_directive(const size_t element_size_bytes)
        -> std::string_view {

        switch (element_size_bytes) {
        case size_qword:
            return "dq";

        case size_dword:
            return "dd";

        case size_word:
            return "dw";

        case size_byte:
            return "db";

        default:
            std::unreachable();
        }
    }

    // nasm rejects a count above the signed 32-bit range in 'times' and 'resb',
    // a larger one is emitted in parts, the last one with the rest
    static auto for_each_chunk(const size_t count,
                               const std::function_ref<void(size_t)> emit)
        -> void {

        constexpr size_t max_chunk{std::numeric_limits<int32_t>::max()};

        size_t remaining{count};
        while (remaining > max_chunk) {
            emit(max_chunk);
            remaining -= max_chunk;
        }

        emit(remaining);
    }

    // the table is inside a function because 'op_info' default member
    // initializers are usable only once the class is complete
    [[nodiscard]] static auto info(const op code) -> const op_info& {
        // indexed by 'op'
        static constexpr auto infos{
            std::to_array<op_info>({
                {.mnemonic{"mov"}, .operand_count{2}},
                {.mnemonic{"movsx"}, .operand_count{2}},
                {.mnemonic{"lea"}, .operand_count{2}},
                {.mnemonic{"add"}, .operand_count{2}},
                {.mnemonic{"sub"}, .operand_count{2}},
                {.mnemonic{"and"}, .operand_count{2}},
                {.mnemonic{"or"}, .operand_count{2}},
                {.mnemonic{"xor"}, .operand_count{2}},
                {.mnemonic{"cmp"}, .operand_count{2}},
                {.mnemonic{"test"}, .operand_count{2}},
                {.mnemonic{"imul"}, .operand_count{2}},
                {.mnemonic{"sal"}, .operand_count{2}},
                {.mnemonic{"sar"}, .operand_count{2}},
                {.mnemonic{"inc"}, .operand_count{1}},
                {.mnemonic{"dec"}, .operand_count{1}},
                {.mnemonic{"neg"}, .operand_count{1}},
                {.mnemonic{"not"}, .operand_count{1}},
                {.mnemonic{"idiv"}, .operand_count{1}},
                {.mnemonic{"div"}, .operand_count{1}},
                {.mnemonic{"push"}, .operand_count{1}},
                {.mnemonic{"pop"}, .operand_count{1}},
                {.mnemonic{"cqo"}, .operand_count{}},
                {.mnemonic{"syscall"}, .operand_count{}},
                {.mnemonic{"ret"}, .operand_count{}},
                {.mnemonic{"rep movsb"}, .operand_count{}},
                {.mnemonic{"rep stosb"}, .operand_count{}},
                {.mnemonic{"repe cmpsb"}, .operand_count{}},
                {.mnemonic{"repe cmpsq"}, .operand_count{}},
                {.mnemonic{"cmpsb"}, .operand_count{}},
                {.mnemonic{"cmpsw"}, .operand_count{}},
                {.mnemonic{"cmpsd"}, .operand_count{}},
                {.mnemonic{"cmpsq"}, .operand_count{}},
            }),
        };

        static_assert(infos.size() == op_count_);

        return infos.at(std::to_underlying(code));
    }

    [[nodiscard]] static auto section_directive(const section which)
        -> std::string_view {

        // indexed by 'section'
        constexpr auto directives{
            std::to_array<std::string_view>({
                "section .text",
                "section .rodata",
                "section .data",
                "section .bss",
                "section .bss.vars nobits alloc write",
            }),
        };

        static_assert(directives.size() ==
                      std::to_underlying(section::variables) + 1);

        return directives.at(std::to_underlying(which));
    }

    [[nodiscard]] static auto size_specifier(const size_t size_bytes)
        -> std::string_view {

        switch (size_bytes) {
        case size_qword:
            return "qword";

        case size_dword:
            return "dword";

        case size_word:
            return "word";

        case size_byte:
            return "byte";

        default:
            std::unreachable();
        }
    }
};
