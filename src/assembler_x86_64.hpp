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
#include <ostream>
#include <print>
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
// below, not text, so an encoder like the rv32i one can be added
// (etc/todo.txt, on hold)
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

    // suffixes of 'jcc' and 'setcc'
    enum class condition : uint8_t { e, ne, l, le, g, ge, a, b, s, ae, o };

    // '[symbol + displacement]' or '[base + index * scale + displacement]',
    // zero 'size_bytes' leaves the width to the other operand
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

        [[nodiscard]] static auto of_expression(const std::string_view text)
            -> immediate {

            immediate result;
            result.expression = text;
            return result;
        }

        [[nodiscard]] static auto
        of_qword_expression(const std::string_view text) -> immediate {

            immediate result{of_expression(text)};
            result.strict_qword = true;
            return result;
        }
    };

    // a register by name, memory or an immediate
    using argument = std::variant<std::string_view, memory, immediate>;

  private:
    // starts the comment of a line, the text before it is code
    static constexpr std::string_view comment_marker_{";"};

    struct op_info {
        std::string_view mnemonic;
        size_t operand_count{};
    };

    static constexpr size_t op_count_{std::to_underlying(op::cmpsq) + 1};

    // a 'mov' between a register and memory, which 'is_reload' compares
    struct memory_move {
        std::string register_name;
        // the operand with its width, e.g. 'word [rbp + 2]'
        std::string address;
        size_t size_bytes{};
        bool stores{};
    };

    std::vector<memory_move> moves_;

  public:
    // the widths of the operands
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    //
    // overridden methods
    //

    auto align(const size_t size_bytes) -> void override {
        add_text(std::format("align {}", size_bytes));
    }

    auto comment(const size_t indent, const std::string_view text)
        -> void override {

        if (text.empty()) {
            add_text(comment_start(indent));
            return;
        }

        add_text(std::format("{} {}", comment_start(indent), text));
    }

    auto comment(const size_t indent, const std::string_view location,
                 const std::string_view text) -> void override {

        add_text(
            std::format("{}[{}] {}", comment_start(indent), location, text));
    }

    auto data(const size_t element_size_bytes,
              const std::span<const int64_t> values) -> void override {

        add_text(data_text(data_directive(element_size_bytes), values));
    }

    auto define_constant(const std::string_view name, const int64_t value)
        -> void override {

        add_text(std::format("{} equ {}", name, value));
    }

    auto reserve(const size_t size_bytes) -> void override {
        for_each_chunk(size_bytes, [&](const size_t chunk_bytes) -> void {
            add_text(std::format("resb {}", chunk_bytes));
        });
    }

    // 'bytes' as a nasm backquoted string, whose escapes nasm reads, e.g.
    // '\n'; a byte that is not printable is a hex escape, not '\01', which
    // nasm reads as one octal escape with the digits after it
    auto string_data(const std::string_view bytes) -> void override {
        const std::string text{escaped_text(bytes, "\"`", "\\x{:02x}")};

        add_text(std::format("db `{}`", text));
    }

    auto zero_data(const size_t size_bytes) -> void override {
        for_each_chunk(size_bytes, [&](const size_t chunk_bytes) -> void {
            add_text(std::format("times {} db 0", chunk_bytes));
        });
    }

    //
    // class methods
    //

    // the number buffer, apart from the variables that 'baz.ld' places right
    // after the data
    auto begin_bss_section() -> void {
        set_code_section(false);
        add_text("section .bss");
    }

    auto bits64() -> void { add_text("bits 64"); }

    auto call(const size_t indent, const std::string_view target) -> void {
        add_instruction(std::format("{}call {}", indentation(indent), target));
        mark_call(target);
    }

    auto default_rel() -> void { add_text("default rel"); }

    auto global(const std::string_view name) -> void {
        add_text(std::format("global {}", name));
    }

    auto instruction(const size_t indent, const op code) -> void {
        assert(info(code).operand_count == 0);

        add_instruction(indentation(indent) + std::string{info(code).mnemonic});
    }

    auto instruction(const size_t indent, const op code, const argument& value)
        -> void {

        assert(info(code).operand_count == 1);

        add_instruction(std::format("{}{} {}", indentation(indent),
                                    info(code).mnemonic, argument_text(value)));
    }

    auto instruction(const size_t indent, const op code, const argument& dst,
                     const argument& src) -> void {

        assert(info(code).operand_count == 2);

        std::string text{
            std::format("{}{} {}, {}", indentation(indent), info(code).mnemonic,
                        argument_text(dst), argument_text(src)),
        };

        std::optional<memory_move> move{memory_move_of(code, dst, src)};

        if (not move) {
            add_instruction(std::move(text));
            return;
        }

        moves_.push_back(std::move(*move));
        add_line(std::move(text), instruction_size(), moves_.size() - 1);
    }

    auto jcc(const size_t indent, const condition cc,
             const std::string_view target) -> void {

        add_jump(indent,
                 {
                     .mnemonic{std::format("j{}", condition_suffix(cc))},
                     .operands{},
                     .target{std::string{target}},
                     .scratch{},
                 });
    }

    // jumps go to the base as jumps so they can be optimized
    auto jmp(const size_t indent, const std::string_view target) -> void {
        add_jump(indent, {
                             .mnemonic{"jmp"},
                             .operands{},
                             .target{std::string{target}},
                             .scratch{},
                         });
    }

    // copies qword register 'rs' to 'rd' where 'rs' is written before it is
    // read again: the qword load just added, when it loads 'rs', loads 'rd'
    // instead
    auto move_consumed(const size_t indent, const std::string_view rd,
                       const std::string_view rs) -> void {

        line* const last{last_instruction()};
        memory_move* const load{qword_load(last, rs)};

        if (load == nullptr) {
            instruction(indent, op::mov, rd, rs);
            return;
        }

        load->register_name = rd;

        last->text = std::format("{}mov {}, {}", leading_whitespace(last->text),
                                 rd, load->address);
    }

    auto setcc(const size_t indent, const condition cc, const argument& dst)
        -> void {

        add_instruction(std::format("{}set{} {}", indentation(indent),
                                    condition_suffix(cc), argument_text(dst)));
    }

    // writes the lines as they are, nasm resolves the jump distances
    auto write(std::ostream& os) const -> void {
        assert(not is_capturing());

        for (const line& l : lines()) {
            if (not l.removed) {
                std::println(os, "{}", l.text);
            }
        }
    }

  private:
    //
    // overridden methods
    //

    auto add_section_directive(const section which) -> void override {
        // indexed by 'section'
        constexpr auto directives{
            std::to_array<std::string_view>({
                "section .text",
                "section .rodata",
                "section .data",
                "section .bss.vars nobits alloc write",
            }),
        };

        static_assert(directives.size() ==
                      std::to_underlying(section::variables) + 1);

        add_text(std::string{directives.at(std::to_underlying(which))});
    }

    // every instruction counts as one
    [[nodiscard]] auto instruction_size() const -> size_t override { return 1; }

    [[nodiscard]] auto
    inverse_branch_mnemonic(const std::string_view mnemonic) const
        -> std::string_view override {

        // a branch mnemonic and its inverse
        using mnemonic_pair = std::pair<std::string_view, std::string_view>;

        constexpr std::array<mnemonic_pair, 7> pairs{
            {
                {"je", "jne"},
                {"jg", "jle"},
                {"jge", "jl"},
                {"jo", "jno"},
                {"ja", "jbe"},
                {"jb", "jae"},
                {"js", "jns"},
            },
        };

        for (const auto& [first, second] : pairs) {
            if (mnemonic == first) {
                return second;
            }

            if (mnemonic == second) {
                return first;
            }
        }

        // the branches the optimizer inverts are all paired
        std::unreachable();
    }

    // a 32-bit load clears the upper half, which the register need not hold
    // clear, and an address without a width takes the register's
    [[nodiscard]] auto is_reload(const line& store, const line& load) const
        -> bool override {

        if (not store.record or not load.record) {
            return false;
        }

        const memory_move& stored{moves_.at(*store.record)};
        const memory_move& loaded{moves_.at(*load.record)};

        return stored.stores and not loaded.stores and
               stored.size_bytes != 0 and stored.size_bytes != size_dword and
               stored.register_name == loaded.register_name and
               stored.address == loaded.address;
    }

    [[nodiscard]] auto unconditional_jump_mnemonic() const
        -> std::string_view override {

        return "jmp";
    }

    //
    // class methods
    //

    auto add_instruction(std::string text) -> void {
        add_line(std::move(text), instruction_size());
    }

    // the load of 'l' when it is a qword 'mov' into 'register_name', else
    // none
    [[nodiscard]] auto qword_load(const line* const l,
                                  const std::string_view register_name)
        -> memory_move* {

        if (l == nullptr or not l->record) {
            return nullptr;
        }

        memory_move& move{moves_.at(*l->record)};

        const bool loads{
            not move.stores and move.size_bytes == size_qword and
                move.register_name == register_name,
        };

        return loads ? &move : nullptr;
    }

    //
    // statics
    //

    // the text between the brackets
    [[nodiscard]] static auto address_text(const memory& address)
        -> std::string {

        // note: a symbol is addressed relative to 'rip', without registers, any
        //       other address from a base register

        assert(address.symbol.empty() != address.base.empty());
        assert(address.symbol.empty() or address.index.empty());

        std::string memory_text{address.symbol};
        memory_text += address.base;

        if (not address.index.empty()) {
            memory_text += std::format(" + {}", address.index);

            if (address.scale > 1) {
                memory_text += std::format(" * {}", address.scale);
            }
        }

        if (address.displacement == 0) {
            return memory_text;
        }

        memory_text += address.displacement > 0 ? " + " : " - ";

        const uint64_t magnitude{
            address.displacement < 0
                ? uint64_t{} - static_cast<uint64_t>(address.displacement)
                : static_cast<uint64_t>(address.displacement),
        };

        memory_text += std::format("{}", magnitude);

        return memory_text;
    }

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

        if (indent == 0) {
            return std::string{comment_marker_};
        }

        std::string text{indentation(indent)};
        text.replace(0, comment_marker_.size(), comment_marker_);

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
    static auto for_each_chunk(const size_t size_bytes,
                               const std::function_ref<void(size_t)> emit)
        -> void {

        constexpr size_t max_chunk_bytes{std::numeric_limits<int32_t>::max()};

        size_t remaining_bytes{size_bytes};
        while (remaining_bytes > max_chunk_bytes) {
            emit(max_chunk_bytes);
            remaining_bytes -= max_chunk_bytes;
        }

        emit(remaining_bytes);
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

    // a 'mov' between a register and memory, else empty
    [[nodiscard]] static auto memory_move_of(const op code, const argument& dst,
                                             const argument& src)
        -> std::optional<memory_move> {

        if (code != op::mov) {
            return std::nullopt;
        }

        const std::string_view* const dst_register{
            std::get_if<std::string_view>(&dst),
        };

        const memory* const src_memory{std::get_if<memory>(&src)};

        if (dst_register != nullptr and src_memory != nullptr) {
            return memory_move{
                .register_name{*dst_register},
                .address{argument_text(src)},
                .size_bytes{src_memory->size_bytes},
                .stores{false},
            };
        }

        const memory* const dst_memory{std::get_if<memory>(&dst)};

        const std::string_view* const src_register{
            std::get_if<std::string_view>(&src),
        };

        if (dst_memory != nullptr and src_register != nullptr) {
            return memory_move{
                .register_name{*src_register},
                .address{argument_text(dst)},
                .size_bytes{dst_memory->size_bytes},
                .stores{true},
            };
        }

        return std::nullopt;
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
