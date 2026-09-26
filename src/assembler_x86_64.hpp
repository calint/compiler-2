#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

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
        shl,
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
        repe_cmpsw,
        repe_cmpsd,
        repe_cmpsq,
    };

    // suffixes of 'jcc', 'setcc' and 'cmovcc', 'nz' is spelled apart from
    // 'ne' to keep the handler text
    enum class condition : uint8_t { e, ne, l, le, g, ge, a, b, s, nz };

    // 'variables' is the uninitialized section after the data
    enum class section : uint8_t { text, rodata, data, bss, variables };

    // nasm expression text, a number or symbols such as frame sizes
    struct immediate {
        std::string expression;
        // keeps a 64-bit field for an address placed by the linker
        bool strict_qword{};

        immediate() = default;

        // numbers of any integer type keep call sites free of conversions
        template <std::integral integral_t>
        explicit(false) immediate(const integral_t value)
            : expression{std::format("{}", value)} {}

        [[nodiscard]] static auto of_expression(const std::string_view text,
                                                const bool strict_qword = {})
            -> immediate {

            immediate result;
            result.expression = text;
            result.strict_qword = strict_qword;

            return result;
        }
    };

    // '[symbol + base + index * scale + displacement]', zero 'size_bytes'
    // leaves the width to the other operand
    struct memory {
        std::string_view symbol;
        std::string_view base;
        std::string_view index;
        uint64_t scale{};
        int64_t displacement{};
        size_t size_bytes{};
        // writes a zero displacement such as 'rbp + 0'
        bool explicit_displacement{};

        [[nodiscard]] static auto of_symbol(const std::string_view name)
            -> memory {

            return {
                .symbol{name},
                .base{},
                .index{},
                .scale{},
                .displacement{},
                .size_bytes{},
                .explicit_displacement{},
            };
        }

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
                .explicit_displacement{},
            };
        }
    };

    // a register by name, memory or an immediate
    using argument = std::variant<std::string_view, memory, immediate>;

    // numbers with unary operations such as '-~' applied from right to left
    struct data_value {
        int64_t value{};
        std::string_view unary_operations;
    };

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

    auto setcc(const size_t indent, const condition cc, const argument& dst)
        -> void {

        add_text(std::format("{}set{} {}", indentation(indent),
                             condition_suffix(cc), argument_text(dst)));
    }

    auto cmovcc(const size_t indent, const condition cc, const argument& dst,
                const argument& src) -> void {

        add_text(std::format("{}cmov{} {}, {}", indentation(indent),
                             condition_suffix(cc), argument_text(dst),
                             argument_text(src)));
    }

    // jumps go to the base as jumps so they can be optimized
    auto jmp(const size_t indent, const std::string_view target) -> void {
        add_jump(std::format("{}jmp {}", indentation(indent), target), "jmp",
                 {}, target, {});
    }

    auto jcc(const size_t indent, const condition cc,
             const std::string_view target) -> void {

        const std::string mnemonic{std::format("j{}", condition_suffix(cc))};

        add_jump(std::format("{}{} {}", indentation(indent), mnemonic, target),
                 mnemonic, {}, target, {});
    }

    auto call(const size_t indent, const std::string_view target) -> void {
        add_text(std::format("{}call {}", indentation(indent), target));
    }

    auto label(const size_t indent, const std::string_view name) -> void {
        add_label(std::string{name},
                  std::format("{}{}:", indentation(indent), name));
    }

    // 'text' follows the comment marker and indentation as it is
    auto comment(const size_t indent, const std::string_view text) -> void {
        std::string line{";"};
        if (indent != 0) {
            line += "   ";
        }
        for (size_t i{1}; i < indent; ++i) {
            line += "    ";
        }
        line += text;

        add_text(std::move(line));
    }

    auto empty_line() -> void { add_text(""); }

    auto default_rel() -> void { add_text("default rel"); }

    auto bits64() -> void { add_text("bits 64"); }

    auto global(const std::string_view name) -> void {
        add_text(std::format("global {}", name));
    }

    // sizes only count in code, so code must be in the text section
    auto switch_section(const section which) -> void {
        set_code_section(which == section::text);

        add_text(std::string{section_directive(which)});
    }

    // writes the lines 'emit_body' adds between '%macro' and '%endmacro'
    auto define_macro(const std::string_view name,
                      const std::function_ref<void()> emit_body) -> void {

        add_text(std::format("%macro {} 0", name));
        emit_body();
        add_text("%endmacro");
    }

    auto use_macro(const size_t indent, const std::string_view name) -> void {
        add_text(indentation(indent) + std::string{name});
    }

    auto align(const size_t size_bytes) -> void {
        add_text(std::format("align {}", size_bytes));
    }

    auto define_constant(const std::string_view name, const int64_t value)
        -> void {

        add_text(std::format("{} equ {}", name, value));
    }

    // the bytes from label 'start' to this line
    auto define_length(const std::string_view name,
                       const std::string_view start) -> void {

        add_text(std::format("{} equ $ - {}", name, start));
    }

    auto data(const size_t element_size_bytes,
              const std::span<const data_value> values) -> void {

        std::string text{
            std::format("{} ", data_directive(element_size_bytes))};
        std::string_view separator;
        for (const data_value& value : values) {
            text += std::format("{}{}{}", separator, value.unary_operations,
                                value.value);
            separator = ", ";
        }

        add_text(std::move(text));
    }

    auto repeated_data(const size_t element_size_bytes, const size_t count,
                       const std::string_view unary_operations,
                       const int64_t value) -> void {

        add_text(std::format("times {} {} {}{}", count,
                             data_directive(element_size_bytes),
                             unary_operations, value));
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
        line += "`";

        add_text(std::move(line));
    }

    // nasm rejects a single reservation above the signed 32-bit range
    auto reserve(const size_t size_bytes) -> void {
        constexpr size_t max_chunk{std::numeric_limits<int32_t>::max()};

        size_t remaining{size_bytes};
        while (remaining > max_chunk) {
            add_text(std::format("resb {}", max_chunk));
            remaining -= max_chunk;
        }

        add_text(std::format("resb {}", remaining));
    }

    // the text between the brackets, also used in comments
    [[nodiscard]] static auto address_text(const memory& address)
        -> std::string {

        std::string s{address.symbol};

        if (not address.base.empty()) {
            if (not s.empty()) {
                s += " + ";
            }
            s += address.base;
        }

        if (not address.index.empty()) {
            if (not s.empty()) {
                s += " + ";
            }
            s += address.index;
            if (address.scale > 1) {
                s += std::format(" * {}", address.scale);
            }
        }

        if (address.explicit_displacement) {
            s += std::format(" + {}", address.displacement);

            return s;
        }

        if (address.displacement == 0) {
            return s;
        }

        if (not s.empty()) {
            s += address.displacement > 0 ? " + " : " - ";
        }

        const uint64_t magnitude{
            address.displacement < 0
                ? uint64_t{} - static_cast<uint64_t>(address.displacement)
                : static_cast<uint64_t>(address.displacement)};

        s += std::format("{}", magnitude);

        return s;
    }

    // lines other than comments and blank lines, the measure for choosing
    // between versions of the same code
    [[nodiscard]] static auto count_instructions(const std::vector<line>& lines)
        -> size_t {

        size_t count{};
        for (const line& l : lines) {
            if (not is_comment_or_blank(l.text)) {
                ++count;
            }
        }

        return count;
    }

  private:
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    struct op_info {
        std::string_view mnemonic;
        size_t operand_count{};
    };

    static constexpr size_t op_count{std::to_underlying(op::repe_cmpsq) + 1};

    // the table is inside a function because 'op_info' default member
    // initializers are usable only once the class is complete
    [[nodiscard]] static auto info(const op code) -> const op_info& {
        // indexed by 'op'
        static constexpr std::array<op_info, op_count> infos{{
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
            {.mnemonic{"shl"}, .operand_count{2}},
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
            {.mnemonic{"repe cmpsw"}, .operand_count{}},
            {.mnemonic{"repe cmpsd"}, .operand_count{}},
            {.mnemonic{"repe cmpsq"}, .operand_count{}},
        }};

        return infos.at(std::to_underlying(code));
    }

    [[nodiscard]] static auto condition_suffix(const condition cc)
        -> std::string_view {

        // indexed by 'condition'
        constexpr std::array<std::string_view, 10> suffixes{
            "e", "ne", "l", "le", "g", "ge", "a", "b", "s", "nz"};

        return suffixes.at(std::to_underlying(cc));
    }

    [[nodiscard]] static auto section_directive(const section which)
        -> std::string_view {

        // indexed by 'section'
        constexpr std::array<std::string_view, 5> directives{
            "section .text",
            "section .rodata",
            "section .data",
            "section .bss",
            "section .bss.vars nobits alloc write",
        };

        return directives.at(std::to_underlying(which));
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

    [[nodiscard]] static auto indentation(const size_t indent) -> std::string {
        std::string text;
        text.resize(indent * 4, ' ');

        return text;
    }

    [[nodiscard]] static auto argument_text(const argument& value)
        -> std::string {

        if (const std::string_view* const name{
                std::get_if<std::string_view>(&value)}) {
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

    [[nodiscard]] auto unconditional_jump_mnemonic() const
        -> std::string_view override {

        return "jmp";
    }

    [[nodiscard]] auto
    inverse_branch_mnemonic(const std::string_view mnemonic) const
        -> std::optional<std::string_view> override {

        constexpr std::array<std::pair<std::string_view, std::string_view>, 3>
            pairs{{
                {"je", "jne"},
                {"jg", "jle"},
                {"jge", "jl"},
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

    [[nodiscard]] auto format_jump(const jump_info& jump) const
        -> std::string override {

        return std::format("{} {}", jump.mnemonic, jump.target);
    }

    [[nodiscard]] auto comment_prefix() const -> std::string_view override {
        return ";";
    }

    // every instruction or directive counts as one
    [[nodiscard]] auto text_code_size(const std::string_view text) const
        -> size_t override {

        const std::string_view code{code_part(text)};
        if (code.empty() or code.back() == ':' or
            code.starts_with("section ")) {
            return 0;
        }

        return 1;
    }

    [[nodiscard]] auto is_label_text(const std::string_view text) const
        -> bool override {

        const std::string_view code{code_part(text)};

        return not code.empty() and code.back() == ':';
    }

    [[nodiscard]] static auto is_comment_or_blank(const std::string_view text)
        -> bool {

        const size_t first{text.find_first_not_of(" \t\n\r\f\v")};

        return first == std::string_view::npos or text[first] == ';';
    }

    [[nodiscard]] static auto code_part(const std::string_view text)
        -> std::string_view {

        const std::string_view code{text.substr(0, text.find(';'))};
        const size_t first{code.find_first_not_of(" \t")};
        if (first == std::string_view::npos) {
            return {};
        }

        return code.substr(first, code.find_last_not_of(" \t") - first + 1);
    }
};
