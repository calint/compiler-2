#pragma once

#include <array>
#include <bit>
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
#include <unordered_map>
#include <utility>
#include <variant>

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

    // note: nasm truncates a symbol at 4095 characters, so two long labels
    //       with a common prefix would collide, e.g. the bodies of generic
    //       instances with long arguments

    static constexpr size_t max_symbol_size_{4000};

    struct op_info {
        std::string_view mnemonic;
        size_t operand_count{};
    };

    // a 'mov' between a register and memory, read by 'is_reload' and
    // 'move_consumed'
    struct memory_move {
        std::string register_name;
        // the operand with its width, e.g. 'word [rbp + 2]'
        std::string address;
        size_t size_bytes{};
        bool stores{};
    };

    // a 'setcc' and the test of its operand that a branch after it need not
    // repeat, read by 'retested_branch'
    struct condition_store {
        condition cc{};
        // e.g. 'test r15b, r15b' or 'cmp byte [rbp + 8], 0'
        std::string retest;
    };

    // the structured form of a line
    using record = std::variant<memory_move, condition_store>;

    line_records<record> records_;

  public:
    // the widths of the operands
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    // the tables by width have the byte first and the qword last
    static constexpr size_t width_count{4};

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

        const std::string_view directive{data_directive(element_size_bytes)};

        add_text(data_text(directive, values));
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
            indentation(indent) +
                instruction_text(code, argument_text(dst), argument_text(src)),
        };

        std::optional<memory_move> move{memory_move_of(code, dst, src)};

        if (not move) {
            add_instruction(std::move(text));
            return;
        }

        add_line(std::move(text), instruction_size(),
                 records_.add(std::move(*move)));
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

        replace_text(*last, instruction_text(op::mov, rd, load->address));
    }

    auto setcc(const size_t indent, const condition cc, const argument& dst)
        -> void {

        const std::string operand{argument_text(dst)};

        // a byte in memory is compared with zero, a register tested
        const std::string retest{
            std::holds_alternative<memory>(dst)
                ? instruction_text(op::cmp, operand, "0")
                : instruction_text(op::test, operand, operand),
        };

        const size_t index{
            records_.add(condition_store{
                .cc{cc},
                .retest{retest},
            }),
        };

        add_line(std::format("{}set{} {}", indentation(indent),
                             condition_suffix(cc), operand),
                 instruction_size(), index);
    }

    // writes the lines as they are, nasm resolves the jump distances, a long
    // symbol shortened by 'shorten_symbols'
    auto write(std::ostream& os) const -> void {
        assert(not is_capturing());

        // the long symbols, keyed by views of the line texts
        std::unordered_map<std::string_view, std::string> short_names;

        for (const line& l : lines()) {
            if (not l.removed) {
                std::println(os, "{}", shorten_symbols(l.text, short_names));
            }
        }
    }

    //
    // statics
    //

    // the index of an operand width in the tables by width
    [[nodiscard]] static constexpr auto width_index(const size_t size_bytes)
        -> size_t {

        assert(std::has_single_bit(size_bytes) and size_bytes <= size_qword);

        return static_cast<size_t>(std::countr_zero(size_bytes));
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

        static_assert(directives.size() == section_count_);

        add_text(std::string{directives.at(std::to_underlying(which))});
    }

    // todo: the lines are text, a copy is folded once they are structured
    auto fold_copy([[maybe_unused]] const std::span<line> block)
        -> bool override {

        return false;
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

    // an address without a width takes the register's; a 32-bit load would
    // clear the upper half, which no reader of a dword value takes: it reads
    // the 32-bit view or extends it with 'movsx'
    [[nodiscard]] auto is_reload(const line& store, const line& load) const
        -> bool override {

        const memory_move* const stored{records_.find<memory_move>(store)};
        const memory_move* const loaded{records_.find<memory_move>(load)};

        if (stored == nullptr or loaded == nullptr) {
            return false;
        }

        return stored->stores and not loaded->stores and
               stored->size_bytes != 0 and
               stored->register_name == loaded->register_name and
               stored->address == loaded->address;
    }

    // 'jne' after the test is taken when the stored value is nonzero
    [[nodiscard]] auto retested_branch(const line& set, const line& test,
                                       const std::string_view branch) const
        -> std::optional<std::string> override {

        const condition_store* const stored{
            records_.find<condition_store>(set),
        };

        if (stored == nullptr or code_text(test) != stored->retest) {
            return std::nullopt;
        }

        std::string taken{
            std::format("j{}", condition_suffix(stored->cc)),
        };

        if (branch == "jne") {
            return taken;
        }

        // note: a bool is branched on with 'je' or 'jne'
        assert(branch == "je");

        return std::string{inverse_branch_mnemonic(taken)};
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

        if (l == nullptr) {
            return nullptr;
        }

        memory_move* const move{records_.find<memory_move>(*l)};

        if (move == nullptr) {
            return nullptr;
        }

        const bool loads{
            not move->stores and move->size_bytes == size_qword and
                move->register_name == register_name,
        };

        return loads ? move : nullptr;
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

    // the instruction of 'l' without its indentation
    [[nodiscard]] static auto code_text(const line& l) -> std::string_view {
        const std::string_view text{l.text};
        return text.substr(leading_whitespace(text).size());
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

        constexpr std::array<std::string_view, width_count> directives{
            "db",
            "dw",
            "dd",
            "dq",
        };

        return directives.at(width_index(element_size_bytes));
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

        static_assert(infos.size() == std::to_underlying(op::cmpsq) + 1);

        return infos.at(std::to_underlying(code));
    }

    // a two-operand instruction without its indentation
    [[nodiscard]] static auto instruction_text(const op code,
                                               const std::string_view dst,
                                               const std::string_view src)
        -> std::string {

        return std::format("{} {}, {}", info(code).mnemonic, dst, src);
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

    // 'text' with each symbol over 'max_symbol_size_' characters in its code
    // written as its first 'max_symbol_size_' characters, '$' and a number,
    // the same for each use; no other symbol has a '$'; the code ends at a
    // comment or a string, whose names nasm does not read as symbols
    [[nodiscard]] static auto shorten_symbols(
        const std::string_view text,
        std::unordered_map<std::string_view, std::string>& short_names)
        -> std::string {

        const std::string_view code{text.substr(0, text.find_first_of(";`"))};

        if (code.size() <= max_symbol_size_) {
            return std::string{text};
        }

        std::string shortened;
        size_t begin{};
        while (begin < code.size()) {
            size_t end{begin};
            while (end < code.size() and is_symbol_char(code.at(end))) {
                ++end;
            }

            const std::string_view symbol{code.substr(begin, end - begin)};

            if (symbol.size() > max_symbol_size_) {
                const auto [found, is_new]{short_names.try_emplace(symbol)};

                if (is_new) {
                    found->second =
                        std::format("{}${}", symbol.substr(0, max_symbol_size_),
                                    short_names.size());
                }

                shortened += found->second;
            } else {
                shortened += symbol;
            }

            // the character after the symbol, if any
            if (end < code.size()) {
                shortened += code.at(end);
            }

            begin = end + 1;
        }

        shortened += text.substr(code.size());

        return shortened;
    }

    [[nodiscard]] static auto size_specifier(const size_t size_bytes)
        -> std::string_view {

        constexpr std::array<std::string_view, width_count> specifiers{
            "byte",
            "word",
            "dword",
            "qword",
        };

        return specifiers.at(width_index(size_bytes));
    }
};
