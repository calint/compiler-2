#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ios>
#include <iterator>
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
#include <vector>

#include "assembler.hpp"
#include "compile_error.hpp"

// buffers rv32i output until every label has an offset, then grows the jumps
// that cannot reach their targets, writes the assembly source and, from the
// same structured lines, a flat binary image
//
// registers are passed by name, as backend operands hold them, and the
// encoded fields follow the instruction formats of the rv32i specification

class assembler_rv32i final : public assembler {
  public:
    // 'and', 'or' and 'xor' are c++ operator names
    enum class op : uint8_t {
        add,
        sub,
        sll,
        slt,
        sltu,
        xor_op,
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
        beqz,
        bnez,
        bltz,
        bgez,
        bgtz,
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

    enum class section : uint8_t { text, rodata, data, bss };

    // a number, or a symbol resolved once addresses are known
    struct immediate {
        enum class part : uint8_t { whole, high, low };

        // numbers of any integer type keep call sites free of casts
        template <std::integral integral_t>
        explicit(false) immediate(const integral_t value)
            : number{checked_number(value)} {}

        immediate() = default;

        int64_t number{};
        std::string symbol;
        part symbol_part{};

        //
        // statics
        //

        [[nodiscard]] static auto
        of_symbol(const std::string_view name,
                  const part symbol_part = part::whole) -> immediate {

            immediate result;
            result.symbol = name;
            result.symbol_part = symbol_part;
            return result;
        }

      private:
        //
        // statics
        //

        template <std::integral integral_t>
        [[nodiscard]] static auto checked_number(const integral_t value)
            -> int64_t {

            assert(std::in_range<int64_t>(value));

            return static_cast<int64_t>(value);
        }
    };

  private:
    // starts the comment of a line, the text before it is code
    static constexpr std::string_view comment_marker_{"#"};

    enum class jump_reach : uint8_t {
        // 'bcc target' within 4 KiB
        branch,
        // 'j target' within 1 MiB
        jal,
        // 'jump target, scratch' anywhere in the 32-bit address space
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
        // opcode and function bits, the base instruction's for pseudos; zero
        // for 'li', 'la', 'j' and 'call', which encode from the named constants
        uint32_t encoding{};
        // pseudo branches such as 'bgtz' swap the base branch's operands
        bool swapped{};
    };

    static constexpr size_t op_count_{std::to_underlying(op::ret) + 1};

    static constexpr uint32_t addi_encoding_{0x00000013};
    static constexpr uint32_t lui_encoding_{0x00000037};
    static constexpr uint32_t auipc_encoding_{0x00000017};
    static constexpr uint32_t jal_encoding_{0x0000006f};
    static constexpr uint32_t jalr_encoding_{0x00000067};
    static constexpr uint32_t slli_encoding_{0x00001013};

    static constexpr uint8_t zero_register_{};
    static constexpr uint8_t return_address_register_{1};

    // the 6 bit immediate of the compressed 'c.li'
    static constexpr int64_t compressed_immediate_min_{-32};
    static constexpr int64_t compressed_immediate_max_{31};
    static constexpr int64_t shift_max_{31};
    static constexpr int64_t upper_max_{0xfffff};
    static constexpr uint32_t upper_mask_{0xfffff};
    static constexpr uint32_t low_mask_{0xfff};
    static constexpr int64_t branch_min_{-4096};
    static constexpr int64_t branch_max_{4094};
    static constexpr int64_t jal_min_{-1048576};
    static constexpr int64_t jal_max_{1048574};

    struct instruction {
        op code{};
        uint8_t rd{};
        uint8_t rs1{};
        uint8_t rs2{};
        immediate value;
        // label of branches, 'j' and 'call'
        std::string target;
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

    static constexpr size_t section_count_{
        std::to_underlying(section::bss) + 1,
    };

    struct line_position {
        section which{};
        size_t offset{};
    };

    struct image_layout {
        std::vector<line_position> positions;
        std::array<size_t, section_count_> sizes{};
        std::array<size_t, section_count_> alignments{};
        std::array<size_t, section_count_> bases{};
    };

    struct symbol_table {
        // labels and constants
        std::unordered_map<std::string_view, int64_t> values;
        // numeric labels as line index and address, in line order
        std::unordered_map<std::string_view,
                           std::vector<std::pair<size_t, int64_t>>>
            local_labels;

        // a name defined twice is a defect of the compiler
        auto add_value(const std::string_view name, const int64_t value)
            -> void {

            assert(not values.contains(name));

            values.emplace(name, value);
        }
    };

    std::vector<record> records_;
    // the jumps have been grown to reach their targets ('resolve_jumps'),
    // the image layout and the text are then final
    bool jumps_resolved_{};

  public:
    static constexpr std::array<std::string_view, 32> register_names{
        "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    };

    static constexpr size_t one_instruction_bytes{4};
    static constexpr size_t two_instructions_bytes{8};
    // the 12-bit signed immediate of the I and S formats
    static constexpr int64_t immediate_min{-2048};
    static constexpr int64_t immediate_max{2047};
    // 'lui' places its immediate in the upper 20 bits
    static constexpr unsigned lui_shift{12};
    static constexpr size_t address_space_bytes{0x1'0000'0000};

    //
    // overridden methods
    //

    auto align(const size_t size_bytes) -> void override {
        // note: 'grow_out_of_reach' measures the code without padding
        assert(not is_code_section());

        add_record(std::format(".balign {}", size_bytes), 0,
                   alignment{
                       .size_bytes{size_bytes},
                   });
    }

    auto begin_data_section() -> void override {
        switch_section(section::data);
    }

    auto begin_read_only_section() -> void override {
        switch_section(section::rodata);
    }

    auto begin_variables_section() -> void override {
        switch_section(section::bss);
    }

    auto comment(const size_t indent, const std::string_view text)
        -> void override {

        if (text.empty()) {
            add_text(indentation(indent) + std::string{comment_marker_});
            return;
        }

        add_text(
            std::format("{}{} {}", indentation(indent), comment_marker_, text));
    }

    // 'location' is the source the comment is about
    auto comment(const size_t indent, const std::string_view location,
                 const std::string_view text) -> void override {

        add_text(std::format("{}{} [{}] {}", indentation(indent),
                             comment_marker_, location, text));
    }

    auto data(const size_t element_size_bytes,
              const std::span<const int64_t> values) -> void override {

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

    auto define_constant(const std::string_view name, const int64_t value)
        -> void override {

        add_record(std::format(".equ {}, {}", name, value), 0,
                   constant{
                       .name{std::string{name}},
                       .value{value},
                   });
    }

    auto repeated_data(const size_t element_size_bytes, const size_t count,
                       const int64_t value) -> void override {

        add_text(std::format(".rept {}", count));

        add_record(
            std::format("{} {}", data_directive(element_size_bytes), value), 0,
            data_values{
                .element_size_bytes{element_size_bytes},
                .values{value},
                .repeat_count{count},
            });

        add_text(".endr");
    }

    auto reserve(const size_t size_bytes) -> void override {
        zero_data(size_bytes);
    }

    auto string_data(const std::string_view bytes) -> void override {
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

    auto zero_data(const size_t size_bytes) -> void override {
        add_record(std::format(".zero {}", size_bytes), 0,
                   data_values{
                       .element_size_bytes{1},
                       .values{0},
                       .repeat_count{size_bytes},
                   });
    }

    //
    // class methods
    //

    auto add(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {

        register_op(indent, op::add, rd, rs1, rs2);
    }

    auto addi(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::addi, rd, rs1, std::move(value));
    }

    auto andi(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::andi, rd, rs1, std::move(value));
    }

    auto beq(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {

        branch(indent, op::beq, rs1, rs2, target);
    }

    auto beqz(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {

        branch_zero(indent, op::beqz, rs, target);
    }

    auto bgeu(const size_t indent, const std::string_view rs1,
              const std::string_view rs2, const std::string_view target)
        -> void {

        branch(indent, op::bgeu, rs1, rs2, target);
    }

    auto bgez(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {

        branch_zero(indent, op::bgez, rs, target);
    }

    auto bgtz(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {

        branch_zero(indent, op::bgtz, rs, target);
    }

    auto bltu(const size_t indent, const std::string_view rs1,
              const std::string_view rs2, const std::string_view target)
        -> void {

        branch(indent, op::bltu, rs1, rs2, target);
    }

    auto bltz(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {

        branch_zero(indent, op::bltz, rs, target);
    }

    auto bne(const size_t indent, const std::string_view rs1,
             const std::string_view rs2, const std::string_view target)
        -> void {

        branch(indent, op::bne, rs1, rs2, target);
    }

    auto bnez(const size_t indent, const std::string_view rs,
              const std::string_view target) -> void {

        branch_zero(indent, op::bnez, rs, target);
    }

    // a branch that is never grown, to a nearby label such as '1f' or a
    // handler
    auto branch(const size_t indent, const op code, const std::string_view rs1,
                const std::string_view rs2, const std::string_view target)
        -> void {

        assert(info(code).operands == form::branch);

        add_instruction(indent, {
                                    .code{code},
                                    .rd{},
                                    .rs1{number_of(rs1)},
                                    .rs2{number_of(rs2)},
                                    .value{},
                                    .target{std::string{target}},
                                });
    }

    auto branch_zero(const size_t indent, const op code,
                     const std::string_view rs, const std::string_view target)
        -> void {

        assert(info(code).operands == form::branch_zero);

        add_instruction(indent, {
                                    .code{code},
                                    .rd{},
                                    .rs1{number_of(rs)},
                                    .rs2{zero_register_},
                                    .value{},
                                    .target{std::string{target}},
                                });
    }

    // routines returning with 'jr' may take the return address in 'link'
    auto call(const size_t indent, const std::string_view target,
              const std::string_view link = "ra") -> void {

        add_instruction(indent, {
                                    .code{op::call},
                                    .rd{number_of(link)},
                                    .rs1{},
                                    .rs2{},
                                    .value{},
                                    .target{std::string{target}},
                                });

        mark_call(target);
    }

    auto ebreak(const size_t indent) -> void {
        add_instruction(indent, {
                                    .code{op::ebreak},
                                    .rd{},
                                    .rs1{},
                                    .rs2{},
                                    .value{},
                                    .target{},
                                });
    }

    auto ecall(const size_t indent) -> void {
        add_instruction(indent, {
                                    .code{op::ecall},
                                    .rd{},
                                    .rs1{},
                                    .rs2{},
                                    .value{},
                                    .target{},
                                });
    }

    auto globl(const std::string_view name) -> void {
        add_text(std::format(".globl {}", name));
    }

    auto immediate_op(const size_t indent, const op code,
                      const std::string_view rd, const std::string_view rs1,
                      immediate value) -> void {

        assert(info(code).operands == form::immediate or
               info(code).operands == form::shift);

        add_instruction(indent, {
                                    .code{code},
                                    .rd{number_of(rd)},
                                    .rs1{number_of(rs1)},
                                    .rs2{},
                                    .value{std::move(value)},
                                    .target{},
                                });
    }

    // a jump that is never grown, to a nearby label such as '1b' or a handler
    auto j(const size_t indent, const std::string_view target) -> void {
        add_instruction(indent, {
                                    .code{op::j},
                                    .rd{},
                                    .rs1{},
                                    .rs2{},
                                    .value{},
                                    .target{std::string{target}},
                                });
    }

    auto jr(const size_t indent, const std::string_view rs) -> void {
        add_instruction(indent, {
                                    .code{op::jr},
                                    .rd{},
                                    .rs1{number_of(rs)},
                                    .rs2{},
                                    .value{},
                                    .target{},
                                });
    }

    auto la(const size_t indent, const std::string_view rd,
            const std::string_view symbol) -> void {

        add_instruction(indent, {
                                    .code{op::la},
                                    .rd{number_of(rd)},
                                    .rs1{},
                                    .rs2{},
                                    .value{immediate::of_symbol(symbol)},
                                    .target{},
                                });
    }

    auto lbu(const size_t indent, const std::string_view rd, immediate offset,
             const std::string_view base) -> void {

        load(indent, op::lbu, rd, std::move(offset), base);
    }

    auto li(const size_t indent, const std::string_view rd, immediate value)
        -> void {

        add_instruction(indent, {
                                    .code{op::li},
                                    .rd{number_of(rd)},
                                    .rs1{},
                                    .rs2{},
                                    .value{std::move(value)},
                                    .target{},
                                });
    }

    auto load(const size_t indent, const op code, const std::string_view rd,
              immediate offset, const std::string_view base) -> void {

        assert(info(code).operands == form::load);

        add_instruction(indent, {
                                    .code{code},
                                    .rd{number_of(rd)},
                                    .rs1{number_of(base)},
                                    .rs2{},
                                    .value{std::move(offset)},
                                    .target{},
                                });
    }

    auto lui(const size_t indent, const std::string_view rd, immediate value)
        -> void {

        add_instruction(indent, {
                                    .code{op::lui},
                                    .rd{number_of(rd)},
                                    .rs1{},
                                    .rs2{},
                                    .value{std::move(value)},
                                    .target{},
                                });
    }

    auto lw(const size_t indent, const std::string_view rd, immediate offset,
            const std::string_view base) -> void {

        load(indent, op::lw, rd, std::move(offset), base);
    }

    // the address after the bss section, valid once jumps are resolved
    [[nodiscard]] auto memory_end_address() const -> size_t {
        assert(jumps_resolved_);

        const image_layout image{layout_image()};
        const size_t bss{section_index(section::bss)};

        return image.bases.at(bss) + image.sizes.at(bss);
    }

    auto mv(const size_t indent, const std::string_view rd,
            const std::string_view rs) -> void {

        add_instruction(indent, {
                                    .code{op::mv},
                                    .rd{number_of(rd)},
                                    .rs1{number_of(rs)},
                                    .rs2{},
                                    .value{},
                                    .target{},
                                });
    }

    auto option_norelax() -> void { add_text(".option norelax"); }

    // the sizes and the binary image assume both options
    auto option_norvc() -> void { add_text(".option norvc"); }

    auto or_op(const size_t indent, const std::string_view rd,
               const std::string_view rs1, const std::string_view rs2) -> void {

        register_op(indent, op::or_op, rd, rs1, rs2);
    }

    auto ori(const size_t indent, const std::string_view rd,
             const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::ori, rd, rs1, std::move(value));
    }

    auto register_op(const size_t indent, const op code,
                     const std::string_view rd, const std::string_view rs1,
                     const std::string_view rs2) -> void {

        assert(info(code).operands == form::registers);

        add_instruction(indent, {
                                    .code{code},
                                    .rd{number_of(rd)},
                                    .rs1{number_of(rs1)},
                                    .rs2{number_of(rs2)},
                                    .value{},
                                    .target{},
                                });
    }

    auto resolve_jumps() -> void {
        assert(not is_capturing());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines(),
        };

        // sizes only grow, so this ends once every jump reaches its target
        bool grown{true};
        while (grown) {
            grown = grow_out_of_reach(labels);
        }

        jumps_resolved_ = true;
    }

    // a branch grown until it reaches 'target'
    auto resolved_branch(const size_t indent, const op code,
                         const std::string_view rs1, const std::string_view rs2,
                         const std::string_view target,
                         const std::string_view scratch) -> void {

        assert(info(code).operands == form::branch);

        const std::string_view mnemonic{info(code).mnemonic};
        const std::string operands{std::format("{}, {}", rs1, rs2)};

        const size_t index{
            store_record(jump_registers{
                .rs1{number_of(rs1)},
                .rs2{number_of(rs2)},
            }),
        };

        add_jump(std::format("{}{} {}, {}", indentation(indent), mnemonic,
                             operands, target),
                 mnemonic, operands, target, scratch, index);
    }

    // an unconditional jump grown until it reaches 'target'
    auto resolved_jump(const size_t indent, const std::string_view target,
                       const std::string_view scratch) -> void {

        add_jump(std::format("{}j {}", indentation(indent), target), "j", {},
                 target, scratch);
    }

    auto ret(const size_t indent) -> void {
        add_instruction(indent, {
                                    .code{op::ret},
                                    .rd{},
                                    .rs1{},
                                    .rs2{},
                                    .value{},
                                    .target{},
                                });
    }

    auto sb(const size_t indent, const std::string_view src, immediate offset,
            const std::string_view base) -> void {

        store(indent, op::sb, src, std::move(offset), base);
    }

    auto slli(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::slli, rd, rs1, std::move(value));
    }

    auto slt(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {

        register_op(indent, op::slt, rd, rs1, rs2);
    }

    auto slti(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::slti, rd, rs1, std::move(value));
    }

    auto sltiu(const size_t indent, const std::string_view rd,
               const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::sltiu, rd, rs1, std::move(value));
    }

    auto sltu(const size_t indent, const std::string_view rd,
              const std::string_view rs1, const std::string_view rs2) -> void {

        register_op(indent, op::sltu, rd, rs1, rs2);
    }

    auto srai(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::srai, rd, rs1, std::move(value));
    }

    auto srli(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::srli, rd, rs1, std::move(value));
    }

    auto store(const size_t indent, const op code, const std::string_view src,
               immediate offset, const std::string_view base) -> void {

        assert(info(code).operands == form::store);

        add_instruction(indent, {
                                    .code{code},
                                    .rd{},
                                    .rs1{number_of(base)},
                                    .rs2{number_of(src)},
                                    .value{std::move(offset)},
                                    .target{},
                                });
    }

    auto sub(const size_t indent, const std::string_view rd,
             const std::string_view rs1, const std::string_view rs2) -> void {

        register_op(indent, op::sub, rd, rs1, rs2);
    }

    auto sw(const size_t indent, const std::string_view src, immediate offset,
            const std::string_view base) -> void {

        store(indent, op::sw, src, std::move(offset), base);
    }

    // sizes only count in code, so code must be in the text section
    auto switch_section(const section which) -> void {
        set_code_section(which == section::text);

        add_record(std::string{section_directive(which)}, 0,
                   section_start{
                       .which{which},
                   });
    }

    // 'binary' receives the sections as one image from address zero
    auto write_resolved(std::ostream& os, std::ostream& binary) -> void {
        assert(jumps_resolved_);

        write_text(os);
        write_image(binary);
        clear();
    }

    auto xor_op(const size_t indent, const std::string_view rd,
                const std::string_view rs1, const std::string_view rs2)
        -> void {

        register_op(indent, op::xor_op, rd, rs1, rs2);
    }

    auto xori(const size_t indent, const std::string_view rd,
              const std::string_view rs1, immediate value) -> void {

        immediate_op(indent, op::xori, rd, rs1, std::move(value));
    }

    //
    // statics
    //

    // the size of the expansion that 'write_code' encodes, so the layout and
    // the image agree
    [[nodiscard]] static auto li_value_size_bytes(const int64_t value)
        -> size_t {

        return load_immediate(zero_register_, value).size() *
               one_instruction_bytes;
    }

    [[nodiscard]] static auto lower_part(const int64_t value) -> int32_t {
        constexpr int32_t low_range{4096};

        const int32_t low{
            static_cast<int32_t>(static_cast<uint32_t>(value) & low_mask_),
        };

        if (low > immediate_max) {
            return low - low_range;
        }

        return low;
    }

    // abi names only, the backend names registers that way
    [[nodiscard]] static auto register_number(const std::string_view name)
        -> std::optional<uint8_t> {

        for (size_t number{}; number < register_names.size(); ++number) {
            if (name == register_names.at(number)) {
                return static_cast<uint8_t>(number);
            }
        }

        return std::nullopt;
    }

    // '%hi' rounds up when the sign-extended '%lo' is negative
    [[nodiscard]] static auto upper_part(const int64_t value) -> uint32_t {
        constexpr uint32_t rounding{0x800};

        return ((static_cast<uint32_t>(value) + rounding) >> lui_shift) &
               upper_mask_;
    }

  private:
    //
    // overridden methods
    //

    [[nodiscard]] auto format_jump(const jump_info& jump) const
        -> std::string override {

        // only branches are formatted, when inverted
        assert(not jump.operands.empty());

        return std::format("{} {}, {}", jump.mnemonic, jump.operands,
                           jump.target);
    }

    // pseudo instructions count as the ones they expand to
    [[nodiscard]] auto instructions_in(const size_t code_size) const
        -> size_t override {

        return code_size / one_instruction_bytes;
    }

    [[nodiscard]] auto
    inverse_branch_mnemonic(const std::string_view mnemonic) const
        -> std::string_view override {

        return inverse(mnemonic);
    }

    // directives and comments occupy no space, instructions are added
    // as records except for jumps, which start as one instruction
    [[nodiscard]] auto text_code_size(const std::string_view text) const
        -> size_t override {

        const std::string_view code{code_part(text)};

        if (code.empty() or is_sizeless_directive(code)) {
            return 0;
        }

        return one_instruction_bytes;
    }

    [[nodiscard]] auto unconditional_jump_mnemonic() const
        -> std::string_view override {

        return "j";
    }

    //
    // class methods
    //

    auto add_instruction(const size_t indent, instruction ins) -> void {
        assert(has_valid_immediate(ins));

        std::string text{indentation(indent) + instruction_text(ins)};

        const size_t size_bytes{encoded_size_bytes(ins)};
        add_record(std::move(text), size_bytes, std::move(ins));
    }

    auto add_record(std::string text, const size_t code_size, record structured)
        -> void {

        const size_t index{store_record(std::move(structured))};
        add_record_line(std::move(text), code_size, index);
    }

    // bss is not part of the image, so it may only reserve zero bytes
    auto assert_bss_uninitialized(const image_layout& image) const -> void {
        for (size_t index{}; index < lines().size(); ++index) {
            if (image.positions.at(index).which != section::bss) {
                continue;
            }

            const line& l{lines().at(index)};

            const data_values* const data{
                std::get_if<data_values>(record_of(l)),
            };

            assert(l.code_size == 0 and
                   (data == nullptr or is_zero_fill(*data)));
        }
    }

    auto clear() -> void {
        lines().clear();
        records_.clear();
        jumps_resolved_ = false;
    }

    [[nodiscard]] auto collect_symbols(const image_layout& image) const
        -> symbol_table {

        symbol_table symbols{};
        for (size_t index{}; index < lines().size(); ++index) {
            const line& l{lines().at(index)};

            const constant* const defined{std::get_if<constant>(record_of(l))};

            if (defined != nullptr) {
                symbols.add_value(defined->name, defined->value);
            }

            if (l.label.empty()) {
                continue;
            }

            const line_position& position{image.positions.at(index)};

            const int64_t address{
                static_cast<int64_t>(
                    image.bases.at(section_index(position.which)) +
                    position.offset),
            };

            if (is_local_label(l.label)) {
                symbols.local_labels[l.label].emplace_back(index, address);
                continue;
            }

            symbols.add_value(l.label, address);
        }

        return symbols;
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

        const jump_registers* const compared{
            std::get_if<jump_registers>(record_of(l)),
        };

        assert(compared != nullptr);

        if (reach_form == jump_reach::branch) {
            return {
                branch_word(find_op(jump.mnemonic), compared->rs1,
                            compared->rs2, target - address),
            };
        }

        const std::string_view inverted{inverse(jump.mnemonic)};

        // the inverted branch skips the jump that follows it
        std::vector<uint32_t> words{
            branch_word(find_op(inverted), compared->rs1, compared->rs2,
                        static_cast<int64_t>(l.code_size)),
        };

        const int64_t jump_address{
            address + static_cast<int64_t>(one_instruction_bytes),
        };

        std::ranges::copy(long_jump(reach_form, jump, target - jump_address),
                          std::back_inserter(words));

        return words;
    }

    [[nodiscard]] auto encode_line(const line& l, const size_t line_index,
                                   const int64_t address,
                                   const symbol_table& symbols) const
        -> std::vector<uint32_t> {

        if (l.jump) {
            return encode_jump(l, line_index, address, symbols);
        }

        const instruction* const ins{std::get_if<instruction>(record_of(l))};

        assert(ins != nullptr);

        return encode_instruction(*ins, line_index, address, symbols);
    }

    auto grow(line& l) const -> void {
        const jump_info& jump{*l.jump};

        const jump_reach grown{
            reach(l) == jump_reach::branch ? jump_reach::jal : jump_reach::far,
        };

        if (grown == jump_reach::far and jump.scratch.empty()) {
            throw unlocated_error{std::format(
                "jump to {} exceeds 1 MiB and no scratch register is free",
                in_quotes(jump.target))};
        }

        const size_t skip_bytes{
            is_conditional(jump) ? one_instruction_bytes : 0,
        };

        const size_t jump_bytes{
            grown == jump_reach::far ? two_instructions_bytes
                                     : one_instruction_bytes,
        };

        l.code_size = skip_bytes + jump_bytes;
    }

    [[nodiscard]] auto grow_out_of_reach(
        const std::unordered_map<std::string_view, size_t>& labels) -> bool {

        const std::vector<size_t> offsets{line_offsets()};
        bool grown{};
        for (size_t index{}; index < lines().size(); ++index) {
            line& l{lines().at(index)};

            if (not l.jump) {
                continue;
            }

            // an unknown offset would leave the jump unchecked
            const auto target{labels.find(l.jump->target)};

            assert(target != labels.end());

            if (reaches(l, offsets.at(index), offsets.at(target->second))) {
                continue;
            }

            grow(l);
            grown = true;
        }

        return grown;
    }

    [[nodiscard]] auto image_size_bytes(const line& l) const -> size_t {
        const data_values* const data{std::get_if<data_values>(record_of(l))};

        if (data == nullptr) {
            return l.code_size;
        }

        return data_size_bytes(*data);
    }

    // sections follow each other in the order text, rodata, data, bss from
    // address zero, like a linker script listing them in that order
    [[nodiscard]] auto layout_image() const -> image_layout {
        image_layout image{};
        image.positions.reserve(lines().size());
        image.alignments = {one_instruction_bytes, 1, 1, 1};

        section current{section::text};
        for (const line& l : lines()) {
            const record* const structured{record_of(l)};

            const section_start* const start{
                std::get_if<section_start>(structured),
            };

            if (start != nullptr) {
                current = start->which;
            }

            const size_t index{section_index(current)};
            size_t& offset{image.sizes.at(index)};

            const alignment* const align{std::get_if<alignment>(structured)};

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
        const size_t bss{section_index(section::bss)};

        image.bases.at(rodata) =
            align_up(image.sizes.at(text), image.alignments.at(rodata));

        image.bases.at(data) =
            align_up(image.bases.at(rodata) + image.sizes.at(rodata),
                     image.alignments.at(data));

        image.bases.at(bss) =
            align_up(image.bases.at(data) + image.sizes.at(data),
                     image.alignments.at(bss));

        return image;
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

    [[nodiscard]] auto reaches(const line& l, const size_t source_offset,
                               const size_t target_offset) const -> bool {

        // distances count from the jumping instruction, which follows the skip
        const int64_t distance{
            static_cast<int64_t>(target_offset) -
                static_cast<int64_t>(source_offset + skip_size_bytes(l)),
        };

        const jump_reach form{reach(l)};

        if (form == jump_reach::branch) {
            return distance >= branch_min_ and distance <= branch_max_;
        }

        if (form == jump_reach::jal) {
            return distance >= jal_min_ and distance <= jal_max_;
        }

        // 'auipc' and 'jalr' reach any address of the 32-bit address space
        return true;
    }

    [[nodiscard]] auto record_of(const line& l) const -> const record* {
        if (not l.record) {
            return nullptr;
        }

        return &records_.at(*l.record);
    }

    [[nodiscard]] auto skip_size_bytes(const line& l) const -> size_t {
        if (not is_conditional(*l.jump) or reach(l) == jump_reach::branch) {
            return 0;
        }

        return one_instruction_bytes;
    }

    [[nodiscard]] auto store_record(record structured) -> size_t {
        records_.push_back(std::move(structured));
        return records_.size() - 1;
    }

    auto write_code(std::ostream& os, const line& l, const size_t line_index,
                    const int64_t address, const symbol_table& symbols) const
        -> void {

        if (l.code_size == 0) {
            return;
        }

        const std::vector<uint32_t> words{
            encode_line(l, line_index, address, symbols),
        };

        assert(words.size() * one_instruction_bytes == l.code_size);

        for (const uint32_t word : words) {
            write_value(os, word, one_instruction_bytes);
        }
    }

    auto write_image(std::ostream& os) const -> void {
        const image_layout image{layout_image()};
        const symbol_table symbols{collect_symbols(image)};

        assert_bss_uninitialized(image);

        // bss follows the image in memory with unspecified content
        size_t written{};
        for (const section which :
             {section::text, section::rodata, section::data}) {

            // an empty section adds no alignment gap, like 'objcopy -O binary'
            if (image.sizes.at(section_index(which)) == 0) {
                continue;
            }

            written = write_section(os, image, symbols, which, written);
        }
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
            std::format(".Lbaz_jump.{}", skip_count++),
        };

        std::println(os, "{}{} {}, {}", indent, inverse(l.jump->mnemonic),
                     l.jump->operands, skip_label);

        write_long_jump(os, indent, l);
        std::println(os, "{}:", skip_label);
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

    // returns the end address of the section, 'written' is where the image
    // written so far ends
    auto write_section(std::ostream& os, const image_layout& image,
                       const symbol_table& symbols, const section which,
                       size_t written) const -> size_t {

        const size_t base{image.bases.at(section_index(which))};
        for (size_t index{}; index < lines().size(); ++index) {
            const line_position& position{image.positions.at(index)};

            if (position.which != which) {
                continue;
            }

            // alignment gaps and the gaps between sections are zero
            const size_t address{base + position.offset};
            write_zeros(os, address - written);

            const line& l{lines().at(index)};

            const data_values* const data{
                std::get_if<data_values>(record_of(l)),
            };

            if (data != nullptr) {
                write_data(os, *data);
                written = address + data_size_bytes(*data);
                continue;
            }

            write_code(os, l, index, static_cast<int64_t>(address), symbols);

            written = address + l.code_size;
        }

        // a trailing alignment still belongs to the section
        const size_t end{base + image.sizes.at(section_index(which))};
        write_zeros(os, end - written);

        return end;
    }

    auto write_text(std::ostream& os) const -> void {
        size_t skip_count{};
        for (const line& l : lines()) {
            write_line(os, l, skip_count);
        }
    }

    //
    // statics
    //

    [[nodiscard]] static auto align_up(const size_t value,
                                       const size_t alignment_bytes) -> size_t {

        return (value + alignment_bytes - 1) / alignment_bytes *
               alignment_bytes;
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

    [[nodiscard]] static auto branch_word(const op code, const uint8_t first,
                                          const uint8_t second,
                                          const int64_t offset) -> uint32_t {

        const op_info& details{info(code)};

        if (details.swapped) {
            return encode_branch(details.encoding, second, first, offset);
        }

        return encode_branch(details.encoding, first, second, offset);
    }

    [[nodiscard]] static auto code_part(const std::string_view text)
        -> std::string_view {

        return code_before(text, comment_marker_);
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

            // note: 'toc::get_type_or_throw' rejects a type wider than the
            //       default type

        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto data_size_bytes(const data_values& data)
        -> size_t {

        return data.element_size_bytes * data.values.size() * data.repeat_count;
    }

    [[nodiscard]] static auto encode_branch(const uint32_t encoding,
                                            const uint8_t rs1,
                                            const uint8_t rs2,
                                            const int64_t offset) -> uint32_t {

        assert(fits(offset, branch_min_, branch_max_) and offset % 2 == 0);

        const uint32_t bits{static_cast<uint32_t>(offset)};
        const uint32_t imm_12{((bits >> 12U) & 1U) << 31U};
        const uint32_t imm_10_5{((bits >> 5U) & 0x3fU) << 25U};
        const uint32_t imm_4_1{((bits >> 1U) & 0xfU) << 8U};
        const uint32_t imm_11{((bits >> 11U) & 1U) << 7U};

        return encoding | imm_12 | imm_10_5 | imm_4_1 | imm_11 |
               register_fields(zero_register_, rs1, rs2);
    }

    [[nodiscard]] static auto
    encode_immediate(const uint32_t encoding, const uint8_t rd,
                     const uint8_t rs1, const int64_t value) -> uint32_t {

        const uint32_t imm_11_0{
            (static_cast<uint32_t>(value) & low_mask_) << 20U,
        };

        return encoding | imm_11_0 | register_fields(rd, rs1, zero_register_);
    }

    [[nodiscard]] static auto
    encode_instruction(const instruction& ins, const size_t line_index,
                       const int64_t address, const symbol_table& symbols)
        -> std::vector<uint32_t> {

        const op_info& details{info(ins.code)};
        const form operands{details.operands};
        const int64_t value{immediate_value(symbols, ins.value, line_index)};

        if (operands == form::registers) {
            return {
                details.encoding | register_fields(ins.rd, ins.rs1, ins.rs2),
            };
        }

        if (operands == form::immediate or operands == form::shift or
            operands == form::load) {

            return {encode_immediate(details.encoding, ins.rd, ins.rs1, value)};
        }

        if (operands == form::upper) {
            return {
                encode_upper(details.encoding, ins.rd,
                             static_cast<uint32_t>(value)),
            };
        }

        if (operands == form::store) {
            return {encode_store(details.encoding, ins.rs1, ins.rs2, value)};
        }

        if (operands == form::system or operands == form::ret) {
            return {details.encoding};
        }

        if (operands == form::load_immediate) {
            return load_immediate(ins.rd, value);
        }

        if (operands == form::load_address) {
            return pc_relative(addi_encoding_, ins.rd, ins.rd, value - address);
        }

        if (operands == form::move) {
            return {encode_immediate(details.encoding, ins.rd, ins.rs1, 0)};
        }

        if (operands == form::jump_register) {
            return {
                encode_immediate(details.encoding, zero_register_, ins.rs1, 0),
            };
        }

        // the remaining forms go to a label
        const int64_t distance{
            symbol_value(symbols, ins.target, line_index) - address,
        };

        if (operands == form::jump) {
            return {encode_jal(zero_register_, distance)};
        }

        if (operands == form::call) {
            return pc_relative(jalr_encoding_, ins.rd, ins.rd, distance);
        }

        return {branch_word(ins.code, ins.rs1, ins.rs2, distance)};
    }

    [[nodiscard]] static auto encode_jal(const uint8_t rd, const int64_t offset)
        -> uint32_t {

        assert(fits(offset, jal_min_, jal_max_) and offset % 2 == 0);

        const uint32_t bits{static_cast<uint32_t>(offset)};
        const uint32_t imm_20{((bits >> 20U) & 1U) << 31U};
        const uint32_t imm_10_1{((bits >> 1U) & 0x3ffU) << 21U};
        const uint32_t imm_11{((bits >> 11U) & 1U) << 20U};
        const uint32_t imm_19_12{((bits >> 12U) & 0xffU) << 12U};

        return jal_encoding_ | imm_20 | imm_10_1 | imm_11 | imm_19_12 |
               register_fields(rd, zero_register_, zero_register_);
    }

    [[nodiscard]] static auto encode_store(const uint32_t encoding,
                                           const uint8_t base,
                                           const uint8_t src,
                                           const int64_t offset) -> uint32_t {

        const uint32_t bits{static_cast<uint32_t>(offset)};
        const uint32_t imm_11_5{((bits >> 5U) & 0x7fU) << 25U};
        const uint32_t imm_4_0{(bits & 0x1fU) << 7U};

        return encoding | imm_11_5 | imm_4_0 |
               register_fields(zero_register_, base, src);
    }

    [[nodiscard]] static auto encode_upper(const uint32_t encoding,
                                           const uint8_t rd,
                                           const uint32_t upper) -> uint32_t {

        const uint32_t imm_31_12{(upper & upper_mask_) << 12U};

        return encoding | imm_31_12 |
               register_fields(rd, zero_register_, zero_register_);
    }

    // sizes under '.option norvc' and '.option norelax', matching the text
    // sizes
    [[nodiscard]] static auto encoded_size_bytes(const instruction& ins)
        -> size_t {

        const form operands{info(ins.code).operands};

        if (operands == form::load_immediate) {
            assert(ins.value.symbol.empty());

            return li_value_size_bytes(ins.value.number);
        }

        if (operands == form::load_address or operands == form::call) {
            return two_instructions_bytes;
        }

        return one_instruction_bytes;
    }

    [[nodiscard]] static auto find_op(const std::string_view mnemonic) -> op {
        for (size_t index{}; index < op_count_; ++index) {
            const op code{static_cast<op>(index)};

            if (info(code).mnemonic == mnemonic) {
                return code;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] static auto fits(const int64_t number, const int64_t min,
                                   const int64_t max) -> bool {

        return number >= min and number <= max;
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
            return not symbolic and fits(value.number, 0, shift_max_);
        }

        if (operands == form::upper) {
            if (symbolic) {
                return value.symbol_part == immediate::part::high;
            }

            return fits(value.number, 0, upper_max_);
        }

        if (operands == form::load or operands == form::store) {
            return not symbolic and
                   fits(value.number, immediate_min, immediate_max);
        }

        if (operands == form::load_immediate) {
            return not symbolic and
                   fits(value.number, std::numeric_limits<int32_t>::min(),
                        std::numeric_limits<uint32_t>::max());
        }

        if (operands == form::load_address) {
            return symbolic and value.symbol_part == immediate::part::whole;
        }

        // the remaining forms have no immediate
        return true;
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

    // the table is inside a function because 'op_info' default member
    // initializers are usable only once the class is complete
    [[nodiscard]] static auto info(const op code) -> const op_info& {
        // one row of the table
        constexpr auto row = [](const std::string_view mnemonic,
                                const form operands, const uint32_t encoding,
                                const bool swapped = false) -> op_info {
            return {
                .mnemonic{mnemonic},
                .operands{operands},
                .encoding{encoding},
                .swapped{swapped},
            };
        };

        // indexed by 'op'
        static constexpr auto infos{
            std::to_array<op_info>({
                row("add", form::registers, 0x33),
                row("sub", form::registers, 0x40000033),
                row("sll", form::registers, 0x1033),
                row("slt", form::registers, 0x2033),
                row("sltu", form::registers, 0x3033),
                row("xor", form::registers, 0x4033),
                row("sra", form::registers, 0x40005033),
                row("or", form::registers, 0x6033),
                row("and", form::registers, 0x7033),
                row("addi", form::immediate, 0x13),
                row("slti", form::immediate, 0x2013),
                row("sltiu", form::immediate, 0x3013),
                row("xori", form::immediate, 0x4013),
                row("ori", form::immediate, 0x6013),
                row("andi", form::immediate, 0x7013),
                row("slli", form::shift, 0x1013),
                row("srli", form::shift, 0x5013),
                row("srai", form::shift, 0x40005013),
                row("lui", form::upper, 0x37),
                row("lb", form::load, 0x03),
                row("lh", form::load, 0x1003),
                row("lw", form::load, 0x2003),
                row("lbu", form::load, 0x4003),
                row("lhu", form::load, 0x5003),
                row("sb", form::store, 0x23),
                row("sh", form::store, 0x1023),
                row("sw", form::store, 0x2023),
                row("beq", form::branch, 0x63),
                row("bne", form::branch, 0x1063),
                row("blt", form::branch, 0x4063),
                row("bge", form::branch, 0x5063),
                row("bltu", form::branch, 0x6063),
                row("bgeu", form::branch, 0x7063),
                row("beqz", form::branch_zero, 0x63),
                row("bnez", form::branch_zero, 0x1063),
                row("bltz", form::branch_zero, 0x4063),
                row("bgez", form::branch_zero, 0x5063),
                row("bgtz", form::branch_zero, 0x4063, true),
                row("ecall", form::system, 0x73),
                row("ebreak", form::system, 0x100073),
                row("li", form::load_immediate, 0),
                row("la", form::load_address, 0),
                row("mv", form::move, 0x13),
                row("j", form::jump, 0),
                row("jr", form::jump_register, 0x67),
                row("call", form::call, 0),
                row("ret", form::ret, 0x8067),
            }),
        };

        static_assert(infos.size() == op_count_);

        return infos.at(std::to_underlying(code));
    }

    [[nodiscard]] static auto instruction_text(const instruction& ins)
        -> std::string {

        const std::string_view mnemonic{info(ins.code).mnemonic};
        const form operands{info(ins.code).operands};
        const std::string_view rd{register_names.at(ins.rd)};
        const std::string_view rs1{register_names.at(ins.rs1)};
        const std::string_view rs2{register_names.at(ins.rs2)};

        if (operands == form::registers) {
            return std::format("{} {}, {}, {}", mnemonic, rd, rs1, rs2);
        }

        if (operands == form::immediate or operands == form::shift) {
            return std::format("{} {}, {}, {}", mnemonic, rd, rs1,
                               immediate_text(ins.value));
        }

        if (operands == form::upper or operands == form::load_immediate or
            operands == form::load_address) {

            return std::format("{} {}, {}", mnemonic, rd,
                               immediate_text(ins.value));
        }

        if (operands == form::load) {
            return std::format("{} {}, {}({})", mnemonic, rd,
                               immediate_text(ins.value), rs1);
        }

        if (operands == form::store) {
            return std::format("{} {}, {}({})", mnemonic, rs2,
                               immediate_text(ins.value), rs1);
        }

        if (operands == form::branch) {
            return std::format("{} {}, {}, {}", mnemonic, rs1, rs2, ins.target);
        }

        if (operands == form::branch_zero) {
            return std::format("{} {}, {}", mnemonic, rs1, ins.target);
        }

        if (operands == form::move) {
            return std::format("{} {}, {}", mnemonic, rd, rs1);
        }

        if (operands == form::jump_register) {
            return std::format("{} {}", mnemonic, rs1);
        }

        // 'ra' is the implied link register
        if (operands == form::call and ins.rd != return_address_register_) {
            return std::format("{} {}, {}", mnemonic, rd, ins.target);
        }

        if (operands == form::jump or operands == form::call) {
            return std::format("{} {}", mnemonic, ins.target);
        }

        // 'ecall', 'ebreak' and 'ret' have no operands
        return std::string{mnemonic};
    }

    [[nodiscard]] static auto inverse(const std::string_view mnemonic)
        -> std::string_view {

        constexpr std::array<mnemonic_pair, 3> pairs{
            {
                {"beq", "bne"},
                {"blt", "bge"},
                {"bltu", "bgeu"},
            },
        };

        return paired_mnemonic(pairs, mnemonic);
    }

    [[nodiscard]] static auto is_local_label(const std::string_view name)
        -> bool {

        return not name.empty() and is_numeric(name);
    }

    // directives that emit bytes would make offsets wrong, so only these are
    // expected in code
    [[nodiscard]] static auto is_sizeless_directive(const std::string_view code)
        -> bool {

        const std::string_view name{code.substr(0, code.find_first_of(" \t"))};
        return name == ".option" or name == ".globl";
    }

    [[nodiscard]] static auto is_zero_fill(const data_values& data) -> bool {
        return std::ranges::all_of(
            data.values, [](const int64_t v) -> bool { return v == 0; });
    }

    // the sequence 'li' expands to
    [[nodiscard]] static auto load_immediate(const uint8_t rd,
                                             const int64_t value)
        -> std::vector<uint32_t> {

        const int32_t number{
            std::bit_cast<int32_t>(static_cast<uint32_t>(value)),
        };

        if (fits(number, immediate_min, immediate_max)) {
            return {
                encode_immediate(addi_encoding_, rd, zero_register_, number),
            };
        }

        // e.g. 2048 is 'addi 1' and 'slli 11': llvm loads a small value
        // shifted left so, as it compresses better, and the image must equal
        // what llvm assembles from the listing
        const uint32_t bits{static_cast<uint32_t>(value)};
        const int shift{std::countr_zero(bits)};

        // note: the low bits are not all zero, so the shift is below 12 and
        //       the division is exact

        if ((bits & low_mask_) != 0 and shift != 0) {
            const uint32_t factor{1U << static_cast<uint32_t>(shift)};
            const int32_t shifted{number / static_cast<int32_t>(factor)};

            if (fits(shifted, compressed_immediate_min_,
                     compressed_immediate_max_)) {

                return {
                    encode_immediate(addi_encoding_, rd, zero_register_,
                                     shifted),
                    encode_immediate(slli_encoding_, rd, rd, shift),
                };
            }
        }

        const uint32_t upper{
            encode_upper(lui_encoding_, rd, upper_part(value)),
        };

        const int32_t lower{lower_part(value)};

        if (lower == 0) {
            return {upper};
        }

        return {upper, encode_immediate(addi_encoding_, rd, rd, lower)};
    }

    [[nodiscard]] static auto long_jump(const jump_reach reach_form,
                                        const jump_info& jump,
                                        const int64_t distance)
        -> std::vector<uint32_t> {

        if (reach_form == jump_reach::far) {
            return pc_relative(jalr_encoding_, zero_register_,
                               number_of(jump.scratch), distance);
        }

        return {encode_jal(zero_register_, distance)};
    }

    [[nodiscard]] static auto number_of(const std::string_view name)
        -> uint8_t {

        const std::optional<uint8_t> number{register_number(name)};

        assert(number);

        return *number;
    }

    // 'auipc' and 'second' reach 'distance' from the 'auipc'
    [[nodiscard]] static auto pc_relative(const uint32_t second_encoding,
                                          const uint8_t rd, const uint8_t base,
                                          const int64_t distance)
        -> std::vector<uint32_t> {

        // addresses are 32 bit, so the distance wraps around the address space
        // note: the address space fits a signed distance
        constexpr int64_t space_bytes{
            static_cast<int64_t>(address_space_bytes),
        };

        assert(distance > -space_bytes and distance < space_bytes);

        return {
            encode_upper(auipc_encoding_, base, upper_part(distance)),
            encode_immediate(second_encoding, rd, base, lower_part(distance)),
        };
    }

    [[nodiscard]] static auto register_fields(const uint8_t rd,
                                              const uint8_t rs1,
                                              const uint8_t rs2) -> uint32_t {

        const uint32_t rd_field{uint32_t{rd} << 7U};
        const uint32_t rs1_field{uint32_t{rs1} << 15U};
        const uint32_t rs2_field{uint32_t{rs2} << 20U};
        return rd_field | rs1_field | rs2_field;
    }

    [[nodiscard]] static auto section_directive(const section which)
        -> std::string_view {

        constexpr std::array<std::string_view, section_count_> directives{
            ".text",
            ".section .rodata",
            ".data",
            ".bss",
        };

        return directives.at(section_index(which));
    }

    [[nodiscard]] static auto section_index(const section which) -> size_t {
        return std::to_underlying(which);
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

        assert(name.size() >= 2);

        const auto locals{
            symbols.local_labels.find(name.substr(0, name.size() - 1)),
        };
        // note: -1 because the reference ends with 'f' or 'b'

        assert(locals != symbols.local_labels.end());

        const std::vector<std::pair<size_t, int64_t>>& definitions{
            locals->second,
        };

        const auto line_of{&std::pair<size_t, int64_t>::first};

        if (name.back() == 'f') {
            const auto next{
                std::ranges::upper_bound(definitions, line_index, {}, line_of),
            };

            if (next != definitions.end()) {
                return next->second;
            }
        }

        if (name.back() == 'b') {
            const auto next{
                std::ranges::lower_bound(definitions, line_index, {}, line_of),
            };

            if (next != definitions.begin()) {
                return std::prev(next)->second;
            }
        }

        // the backend refers to labels it defines
        std::unreachable();
    }

    static auto write_data(std::ostream& os, const data_values& data) -> void {
        // zero fills such as the variables area can be large
        if (is_zero_fill(data)) {
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

    static auto write_zeros(std::ostream& os, size_t count) -> void {
        static constexpr std::array<char, 4096> zeros{};
        while (count != 0) {
            const size_t chunk{std::min(count, zeros.size())};
            os.write(zeros.data(), static_cast<std::streamsize>(chunk));
            count -= chunk;
        }
    }
};
