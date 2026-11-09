#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "assembler_x86_64.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"

// the x86_64 linux backend, written as nasm text through 'assembler_x86_64';
// the formatter sorts the functions by name, so by topic they are:
//
//   registers    'machine' allocates them, here the names
//                'register_names_', the order 'scratch_registers_', the
//                roles in 'x86_64_target'
//   operations   'emit_op' chooses the form of an instruction,
//                'lower_address' fits an operand into one,
//                'emit_multiply', 'emit_signed_divide', 'shift'
//   memory       'copy_block', 'store_bytes', 'zero' unrolled or with
//                'rep', the bulk operations with 'rsi', 'rdi' and 'rcx'
//   comparisons  'compare_and_branch', 'store_condition', 'memory_equal'
//   checks       'emit_bounds_check' with 'test_sign' and
//                'check_upper_bound', 'check_divisor', 'check_shift_count',
//                'check_copy_overlap', 'emit_frame_capacity_check', the
//                overflow checks 'branch_on_overflow' and 'check_byte_product',
//                each jumps to its handler, all but the frame check with a stub
//                per line for '--checks=line'
//   handlers     'emit_panic_message', 'emit_report_line_routine'
//   calls        'emit_call', 'emit_target_call' through the system call
//                'io_syscall' or 'call_sector_routine', the shared exit
//                routine 'emit_exit_routine', the sector routines
//                'emit_sector_routines'
class machine_x86_64 final : public machine {
    using op = assembler_x86_64::op;

    using condition = assembler_x86_64::condition;

    static constexpr size_t size_qword_{assembler_x86_64::size_qword};
    static constexpr size_t size_dword_{assembler_x86_64::size_dword};
    static constexpr size_t size_word_{assembler_x86_64::size_word};
    static constexpr size_t size_byte_{assembler_x86_64::size_byte};

    static constexpr size_t data_alignment_{16};

    // the user address space of x86_64 Linux, 128 TiB
    static constexpr size_t user_address_space_bytes_{0x800000000000};

    static constexpr int syscall_read_{0};
    static constexpr int syscall_write_{1};
    static constexpr int syscall_exit_{60};
    static constexpr int syscall_open_{2};
    static constexpr int syscall_lseek_{8};
    static constexpr int syscall_pread_{17};
    static constexpr int syscall_pwrite_{18};
    static constexpr std::string_view sector_read_label_{"baz_sector_read"};
    static constexpr std::string_view sector_write_label_{"baz_sector_write"};
    // the routine that the handlers with a line jump to
    static constexpr std::string_view report_line_label_{"baz_report_line"};
    // the line and the exit code a handler passes to 'report_line_label_'
    static constexpr std::string_view report_line_register_{"rbp"};
    static constexpr std::string_view report_exit_code_register_{"rbx"};
    // the routine an exit jumps to with the exit code in 'rdi'
    static constexpr std::string_view exit_label_{"baz_exit"};

    //
    // code-size thresholds, each measured, see its note
    //

    // the instructions of a 'rep stosb' or 'rep movsb' with its setup: a zero
    // or a copy whose parts take more is emitted with it ('takes_rep_form'), a
    // zero takes a store per part, a copy a load and a store ('part_count')
    static constexpr size_t rep_form_instructions_{4};

    // note: measured on the tests, 'roome' and zeroes and copies of 1 to 128
    //       B, the part count was smaller by 14, 2 and 76 instructions than
    //       the best byte thresholds, and keeps 'mov' where the two forms tie

    // a string or a constant up to this size is stored as immediates, one or
    // two instructions per qword, larger ones are copied with 'rep movsb'
    static constexpr size_t immediate_store_threshold_bytes_{16};

    // note: measured on the tests, 'roome' and stores of 1 to 128 B, 16 was
    //       smallest, 24 and 32 larger by 7 and 11 instructions in the tests

    // up to this many qwords single compares cost less than loading 'rcx'
    static constexpr size_t threshold_for_repe_cmpsq_count_{2};

    // see 'target_description::max_padding_gaps_zeroed_one_by_one'
    static constexpr size_t max_padding_gaps_zeroed_one_by_one_{4};

    // note: a gap takes one to three stores, the whole instance a store per
    //       part up to 'rep_form_instructions_' parts and beyond 'rep stosb'
    //       with its setup; measured on 2500 random types with padding, from
    //       5 gaps the whole instance was never larger, at 4 gaps it was
    //       larger by one instruction in 32 of 128

    // scratch values live from which an array loop counts in memory: 'inc'
    // and 'cmp' work on memory, so a loop counter leaves its register to the
    // values that need one once many are live
    static constexpr size_t memory_counter_pressure_count_{6};

    // note: measured on the tests and 'roome', from 5 the code size is flat
    //       within 1 instruction and below it up to 9 larger; 6 to 8 accept
    //       what the tests expect, 5 accepts 0716 and from 9 1798 runs out of
    //       registers

    struct register_names {
        std::string_view qword;
        std::string_view dword;
        std::string_view word;
        std::string_view byte;
    };

    static constexpr std::array<register_names, 16> register_names_{
        {
            {.qword{"rax"}, .dword{"eax"}, .word{"ax"}, .byte{"al"}},
            {.qword{"rbx"}, .dword{"ebx"}, .word{"bx"}, .byte{"bl"}},
            {.qword{"rcx"}, .dword{"ecx"}, .word{"cx"}, .byte{"cl"}},
            {.qword{"rdx"}, .dword{"edx"}, .word{"dx"}, .byte{"dl"}},
            {.qword{"rbp"}, .dword{"ebp"}, .word{"bp"}, .byte{"bpl"}},
            {.qword{"rsi"}, .dword{"esi"}, .word{"si"}, .byte{"sil"}},
            {.qword{"rdi"}, .dword{"edi"}, .word{"di"}, .byte{"dil"}},
            {.qword{"rsp"}, .dword{"esp"}, .word{"sp"}, .byte{"spl"}},
            {.qword{"r8"}, .dword{"r8d"}, .word{"r8w"}, .byte{"r8b"}},
            {.qword{"r9"}, .dword{"r9d"}, .word{"r9w"}, .byte{"r9b"}},
            {.qword{"r10"}, .dword{"r10d"}, .word{"r10w"}, .byte{"r10b"}},
            {.qword{"r11"}, .dword{"r11d"}, .word{"r11w"}, .byte{"r11b"}},
            {.qword{"r12"}, .dword{"r12d"}, .word{"r12w"}, .byte{"r12b"}},
            {.qword{"r13"}, .dword{"r13d"}, .word{"r13w"}, .byte{"r13b"}},
            {.qword{"r14"}, .dword{"r14d"}, .word{"r14w"}, .byte{"r14b"}},
            {.qword{"r15"}, .dword{"r15d"}, .word{"r15w"}, .byte{"r15b"}},
        },
    };

    static constexpr std::array<std::string_view, 14> scratch_registers_{
        "r15", "r14", "r13", "r12", "r10", "r9",  "r8",
        "r11", "rbx", "rdx", "rax", "rsi", "rdi", "rcx",
    };

    // note: in order of likelihood they are not used by name, 'rsi', 'rdi'
    //       and 'rcx' last since 'rep movsb' and 'repe cmpsb' need all three

    // the registers of 'rep movsb' and 'repe cmpsb': the source, the
    // destination and the byte count
    static constexpr std::array<std::string_view, 3> bulk_registers_{
        "rsi",
        "rdi",
        "rcx",
    };

    // the registers of 'rep stosb': the byte, the destination and the count
    static constexpr std::array<std::string_view, 3> stosb_registers_{
        "rax",
        "rdi",
        "rcx",
    };

    assembler_x86_64 assembler_;

  public:
    machine_x86_64(const builtin_type_set& builtins, const source_files& files,
                   const target_options& options)
        : machine{builtins, files, options, x86_64_target(), assembler_} {}

    //
    // overridden methods
    //

    auto add_subtract(const token& src_loc_tk, const size_t indent,
                      const arithmetic_operator operation, const operand& dst,
                      const operand& src, const runtime_check& check = {})
        -> void override {

        assert(operation == arithmetic_operator::add or
               operation == arithmetic_operator::subtract);

        // note: the front end folds away an add or subtract of 0, as rv32i
        //       asserts in 'assert_folded'

        assert(immediate_bits(src) != 0);

        const bool is_add{operation == arithmetic_operator::add};

        // 'inc' and 'dec' are a byte shorter than 'add' and 'sub' of 1 and set
        // the overflow flag as they do, the check reads only that flag; they
        // keep the carry flag, which nothing reads after them
        if (immediate_bits(src) == 1) {
            emit_unary(src_loc_tk, indent, is_add ? op::inc : op::dec, dst);
        } else {
            emit_op(src_loc_tk, indent, is_add ? op::add : op::sub, dst, src);
        }

        if (check.enabled) {
            branch_on_overflow(src_loc_tk, indent);
        }
    }

    // 'lea' writes only registers so a memory destination takes the address
    // through a scratch register
    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        if (dst.is_register()) {
            lea(src_loc_tk, indent, dst, address);
            return;
        }

        const operation_scope scope{*this};

        const operand reg{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        lea(src_loc_tk, indent, reg, address);
        mov(src_loc_tk, indent, dst, reg);
    }

    auto bitwise(const token& src_loc_tk, const size_t indent,
                 const arithmetic_operator operation, const operand& dst,
                 const operand& src) -> void override {

        if (operation == arithmetic_operator::bit_and) {
            emit_op(src_loc_tk, indent, op::and_op, dst, src);
            return;
        }

        if (operation == arithmetic_operator::bit_or) {
            emit_op(src_loc_tk, indent, op::or_op, dst, src);
            return;
        }

        assert(operation == arithmetic_operator::bit_xor);

        emit_op(src_loc_tk, indent, op::xor_op, dst, src);
    }

    auto branch(const size_t indent, const std::string_view target)
        -> void override {

        assembler_.jmp(indent, target);
    }

    auto compare_and_branch(const token& src_loc_tk, const size_t indent,
                            const operand& lhs, const operand& rhs,
                            const comparison_action& action) -> void override {

        cmp(src_loc_tk, indent, lhs, rhs);

        if (not action.destination.is_empty()) {
            store_condition(src_loc_tk, indent,
                            condition_for_comparison(action.operation),
                            action.destination);
        }

        if (not action.target.empty()) {
            const comparison_operator branched{
                action.branch_on_true ? action.operation
                                      : negated(action.operation),
            };

            assembler_.jcc(indent, condition_for_comparison(branched),
                           action.target);
        }
    }

    // x86 accesses unaligned memory so the alignment is not needed
    auto copy_block(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src,
                    const size_t size_bytes,
                    [[maybe_unused]] const size_t alignment) -> void override {

        if (size_bytes == 0) {
            return;
        }

        const size_t parts{part_count(size_bytes)};

        // note: 2 because a part is a load and a store
        if (takes_rep_form(2 * parts, bulk_registers_)) {
            copy_with_rep_movsb(src_loc_tk, indent, dst, size_bytes,
                                [&](const operand& pointer) -> void {
                                    lea(src_loc_tk, indent, pointer, src);
                                });

            return;
        }

        comment(src_loc_tk, indent, "{}, use mov", counted(parts, "part"));

        const operation_scope scope{*this};

        const operand tmp{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        for_each_part(
            size_bytes, size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const operand reg{sized_operand(tmp, part_size_bytes)};

                mov(src_loc_tk, indent, reg,
                    memory_part(src, part_size_bytes, offset));

                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset), reg);
            });
    }

    auto copy_elements(const token& src_loc_tk, const size_t indent,
                       const size_t element_size_bytes,
                       const std::function_ref<void(const operand&)> emit_count,
                       const copy_request& request) -> void override {

        const operation_scope bulk{*this};

        load_bulk_registers(src_loc_tk, indent, element_size_bytes, emit_count,
                            request.src, request.dst);

        if (request.overlap.enabled) {
            check_copy_overlap(src_loc_tk, indent);
        }

        assembler_.instruction(indent, op::rep_movsb);
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        assert(dst.is_register() or dst.is_memory());

        mov(src_loc_tk, indent, dst, src);
    }

    auto divide(const token& src_loc_tk, const size_t indent,
                const arithmetic_operator operation, const operand& dst,
                const operand& divisor, const runtime_check& check)
        -> void override {

        assert(operation == arithmetic_operator::divide or
               operation == arithmetic_operator::remainder);

        const operation_scope named{*this};

        reserve_named_register(src_loc_tk, indent, "rax");

        // note: a register 'dst' receives the result after the division

        const bool consumed{
            dst.is_register() and dst.type_ref().size_bytes() == size_qword_ and
                (registers_mask(divisor) & registers_mask(dst)) == 0,
        };

        if (consumed) {
            assembler_.move_consumed(indent, "rax", dst.register_name());
        } else {
            mov(src_loc_tk, indent, qword_register("rax"), dst);
        }

        reserve_named_register(src_loc_tk, indent, "rdx");

        emit_signed_divide(src_loc_tk, indent, divisor, check,
                           dst.type_ref().size_bytes());

        mov(src_loc_tk, indent, dst,
            qword_register(operation == arithmetic_operator::divide ? "rax"
                                                                    : "rdx"));
    }

    auto
    equal_elements(const token& src_loc_tk, const size_t indent,
                   const size_t element_size_bytes,
                   const std::function_ref<void(const operand&)> emit_count,
                   const address_emitter& lhs, const address_emitter& rhs,
                   [[maybe_unused]] const size_t alignment, const operand& dst)
        -> void override {

        const operation_scope bulk{*this};

        load_bulk_registers(src_loc_tk, indent, element_size_bytes, emit_count,
                            lhs, rhs);

        // note: a zero count compares nothing and leaves the flags of
        //       'test', ZF set, so it compares equal

        emit_op(src_loc_tk, indent, op::test, qword_register("rcx"),
                qword_register("rcx"));

        assembler_.instruction(indent, op::repe_cmpsb);

        store_condition(src_loc_tk, indent, condition::e, dst);
    }

    auto memory_equal(const token& src_loc_tk, const size_t indent,
                      const size_t size_bytes, const operand& lhs,
                      const operand& rhs, const equality_request& request)
        -> void override {

        const std::vector<op> compares{memory_equal_compares(size_bytes)};

        assert(not compares.empty());

        // only 'repe cmpsq' counts in 'rcx'
        const bool is_counted{compares.front() == op::repe_cmpsq};

        const operation_scope bulk{*this};

        reserve_named_register(src_loc_tk, indent, "rsi");
        reserve_named_register(src_loc_tk, indent, "rdi");

        if (is_counted) {
            reserve_named_register(src_loc_tk, indent, "rcx");
        }

        lea(src_loc_tk, indent, qword_register("rsi"), lhs);
        lea(src_loc_tk, indent, qword_register("rdi"), rhs);

        if (is_counted) {
            mov(src_loc_tk, indent, qword_register("rcx"),
                immediate(size_bytes / size_qword_));
        }

        // a part that differs decides the result, so the rest is skipped
        std::string end_label;

        if (compares.size() > 1) {
            end_label =
                std::format(".Lbaz_equal.{}", next_local_label_number());
        }

        for (const auto [i, compare] : std::views::enumerate(compares)) {
            if (i != 0) {
                assembler_.jcc(indent, condition::ne, end_label);
            }

            assembler_.instruction(indent, compare);
        }

        if (not end_label.empty()) {
            assembler_.label(indent, end_label);
        }

        store_condition(src_loc_tk, indent,
                        request.inverted ? condition::ne : condition::e,
                        request.dst);
    }

    auto return_function(const size_t indent) -> void override {
        assembler_.instruction(indent, op::ret);
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const operand& count, const runtime_check& check)
        -> void override {

        assert(operation == arithmetic_operator::shift_left or
               operation == arithmetic_operator::shift_right);

        const op code{
            operation == arithmetic_operator::shift_left ? op::sal : op::sar,
        };

        if (count.is_immediate()) {
            // note: the front end rejects a constant count outside the width
            //       and drops a count of 0

            assert(
                count.immediate() > 0 and
                std::cmp_less(count.immediate(), dst.type_ref().size_bits()));

            emit_op(src_loc_tk, indent, code, dst, count);

            return;
        }

        const operation_scope named{*this};

        reserve_named_register(src_loc_tk, indent, "rcx");

        // note: a count wider than the value is rejected as narrowed
        assert(count.type_ref().size_bytes() <= dst.type_ref().size_bytes());

        mov(src_loc_tk, indent,
            sized_register("rcx", dst.type_ref().size_bytes()), count);

        if (check.enabled) {
            check_shift_count(src_loc_tk, indent, dst.type_ref());
        }

        emit_op(src_loc_tk, indent, code, dst,
                sized_register("rcx", size_byte_));
    }

    // up to 'immediate_store_threshold_bytes_' as immediates, larger bytes
    // copied from read-only data with 'rep movsb', or as immediates when one
    // of its registers is held, e.g. 'rsi' by an enclosing 'array_copy'
    auto store_bytes(
        const token& src_loc_tk, const size_t indent,
        const std::string_view bytes, const operand& dst,
        [[maybe_unused]] const size_t alignment,
        const std::function_ref<std::string(std::string_view)> add_constant)
        -> void override {

        if (bytes.size() > immediate_store_threshold_bytes_ and
            not is_any_held(bulk_registers_)) {

            const std::string label{add_constant(bytes)};

            // rip-relative label addresses need no address register
            copy_with_rep_movsb(
                src_loc_tk, indent, dst, bytes.size(),
                [&](const operand& pointer) -> void {
                    assembler_.instruction(
                        indent, op::lea, to_argument(pointer),
                        assembler_x86_64::memory::of_symbol(label));
                });

            return;
        }

        if (bytes.size() > immediate_store_threshold_bytes_) {
            comment(src_loc_tk, indent,
                    "size > {} B, 'rep movsb' registers held, use immediates",
                    immediate_store_threshold_bytes_);
        } else {
            comment(src_loc_tk, indent, "size <= {} B, use immediates",
                    immediate_store_threshold_bytes_);
        }

        for_each_part(
            bytes.size(), size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const int64_t value{
                    little_endian_value(bytes.substr(offset, part_size_bytes)),
                };

                // a qword store takes only a sign-extended 32-bit immediate
                if (part_size_bytes == size_qword_ and
                    not std::in_range<int32_t>(value)) {

                    for (const size_t half_offset :
                         {offset, offset + size_dword_}) {

                        const int64_t half{
                            little_endian_value(
                                bytes.substr(half_offset, size_dword_)),
                        };

                        mov(src_loc_tk, indent,
                            memory_part(dst, size_dword_, half_offset),
                            immediate(half));
                    }

                    return;
                }

                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset),
                    immediate(value));
            });
    }

    auto unary(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const runtime_check& check = {}) -> void override {

        if (operation == arithmetic_operator::complement) {
            emit_unary(src_loc_tk, indent, op::not_op, dst);
            return;
        }

        assert(operation == arithmetic_operator::negate);

        emit_unary(src_loc_tk, indent, op::neg, dst);

        if (check.enabled) {
            branch_on_overflow(src_loc_tk, indent);
        }
    }

    auto write_assembly(std::ostream& os) -> void override {
        assembler_.write(os);
    }

    auto zero(const token& src_loc_tk, const size_t indent, const operand& dst,
              const size_t size_bytes, [[maybe_unused]] const size_t alignment)
        -> void override {

        const size_t parts{part_count(size_bytes)};

        if (takes_rep_form(parts, stosb_registers_)) {
            const operation_scope named{*this};

            for (const std::string_view name : stosb_registers_) {
                reserve_named_register(src_loc_tk, indent, name);
            }

            const operand al{sized_register("rax", size_byte_)};
            emit_op(src_loc_tk, indent, op::xor_op, al, al);

            lea(src_loc_tk, indent, qword_register("rdi"), dst);

            mov(src_loc_tk, indent, qword_register("rcx"),
                immediate(size_bytes));

            assembler_.instruction(indent, op::rep_stosb);

            return;
        }

        comment(src_loc_tk, indent, "{}, use mov", counted(parts, "part"));

        for_each_part(
            size_bytes, size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset), immediate(0));
            });
    }

  protected:
    //
    // overridden methods
    //

    auto emit_bounds_check(const token& src_loc_tk, const size_t indent,
                           const bounds_range& range, const bounds_plan& plan)
        -> void override {

        for (const operand& value : plan.sign_tested) {
            test_sign(src_loc_tk, indent, value);
        }

        if (plan.upper) {
            comment(src_loc_tk, indent, "upper bound");

            check_upper_bound(src_loc_tk, indent, range, plan.lower_checked);
        }
    }

    auto emit_call(const size_t indent, const std::string_view label)
        -> void override {

        assembler_.call(indent, label);
    }

    auto emit_entry() -> void override {
        assembler_.default_rel();
        assembler_.add_separator_newline();

        assembler_.switch_section(section::text);
        assembler_.bits64();
        assembler_.global("_start");
        assembler_.label(0, "_start");
        assembler_.add_separator_newline();
    }

    // ends the program with the exit code in 'rdi'
    auto emit_exit_routine() -> void override {
        assembler_.instruction(1, op::mov, "rax", syscall_exit_);
        assembler_.instruction(1, op::syscall);
    }

    auto emit_frame_capacity_check(const token& src_loc_tk, const size_t indent,
                                   const operand& frame_address,
                                   const std::string_view frame_size_label)
        -> void override {

        // note: the frame follows the storage in use from the base
        assert(frame_address.displacement() >= 0);

        const operand last_base{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        // the highest base at which the frame fits: the end of 'vars' less
        // the frame size and the displacement, all in the 64-bit immediate,
        // since 'cmp' takes only 32 bits; an absolute address reaches beyond
        // the 2 GiB of 'rip' relative ones
        const std::string last_base_expression{
            std::format("{} - {} - {}", variables_end_label_, frame_size_label,
                        frame_address.displacement()),
        };

        assembler_.instruction(indent, op::mov, to_argument(last_base),
                               assembler_x86_64::immediate::of_qword_expression(
                                   last_base_expression));

        // note: signed, the highest base is negative when the frame size and
        //       the displacement pass the address of 'vars.end'; addresses and
        //       sizes stay below 2^47, so nothing wraps

        assembler_.instruction(indent, op::cmp, to_argument(last_base),
                               frame_address.base_register());

        branch_to_failure(src_loc_tk, indent, condition::l,
                          frame_overflow_failure_);
    }

    auto emit_multiply(const token& src_loc_tk, const size_t indent,
                       const operand& product, const operand& factor,
                       const factor_use use, const runtime_check& check)
        -> void override {

        if (product.type_ref().size_bytes() == size_byte_) {
            const operation_scope scope{*this};

            // a register product holds the wide product in place, its low
            // byte is the result
            const bool is_in_place{product.is_register()};

            const operand left{
                is_in_place ? register_view(product, default_type())
                            : alloc_scratch_register(src_loc_tk, indent,
                                                     default_type()),
            };

            const operand right{
                alloc_scratch_register(src_loc_tk, indent, default_type()),
            };

            mov(src_loc_tk, indent, left, product);
            mov(src_loc_tk, indent, right, factor);
            emit_op(src_loc_tk, indent, op::imul, left, right);

            if (check.enabled) {
                check_byte_product(src_loc_tk, indent, left, right);
            }

            if (is_in_place) {
                return;
            }

            mov(src_loc_tk, indent, product, left);

            return;
        }

        if (product.is_register()) {
            emit_op(src_loc_tk, indent, op::imul, product, factor);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent);
            }

            return;
        }

        if (use == factor_use::may_overwrite) {
            emit_op(src_loc_tk, indent, op::imul, factor, product);

            // the move keeps the flags
            mov(src_loc_tk, indent, product, factor);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent);
            }

            return;
        }

        const operation_scope scope{*this};

        const operand reg{
            alloc_scratch_register(
                src_loc_tk, indent,
                builtin_type_for_size_bytes(product.type_ref().size_bytes())),
        };

        mov(src_loc_tk, indent, reg, product);
        emit_op(src_loc_tk, indent, op::imul, reg, factor);

        if (check.enabled) {
            branch_on_overflow(src_loc_tk, indent);
        }

        mov(src_loc_tk, indent, product, reg);
    }

    auto emit_no_return_jump(const size_t indent, const std::string_view target)
        -> void override {

        assembler_.jmp(indent, target);
    }

    auto emit_panic_message(const std::string_view message_label,
                            const std::string_view message) -> void override {

        assembler_.comment(1, "print message to stderr");
        assembler_.instruction(1, op::mov, "rax", syscall_write_);
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor_);

        assembler_.instruction(
            1, op::lea, "rsi",
            assembler_x86_64::memory::of_symbol(message_label));

        assembler_.instruction(1, op::mov, "rdx", message.size());

        assembler_.instruction(1, op::syscall);
    }

    // prints the line number in 'report_line_register_' and exits with the
    // code in 'report_exit_code_register_'
    auto emit_report_line_routine() -> void override {
        constexpr int newline{'\n'};
        constexpr int decimal_base{10};
        // the 20 digits of a 64-bit number and a newline, the digits are
        // written backwards before the newline at the end of the buffer
        constexpr size_t number_buffer_size_bytes{21};
        constexpr size_t newline_offset{number_buffer_size_bytes - 1};

        const std::string line_comment{
            std::format("line number is in {}",
                        in_quotes(report_line_register_)),
        };

        assembler_.comment(1, line_comment);
        assembler_.instruction(1, op::mov, "rax", report_line_register_);
        assembler_.comment(1, "convert to string");

        const std::string newline_address{
            std::format("num_buffer + {}", newline_offset),
        };

        assembler_.instruction(
            1, op::mov, "rdi",
            assembler_x86_64::immediate::of_qword_expression(newline_address));

        assembler_.instruction(
            1, op::mov, assembler_x86_64::memory::of_base("rdi", size_byte_),
            newline);

        assembler_.instruction(1, op::dec, "rdi");
        assembler_.instruction(1, op::mov, "rcx", decimal_base);
        assembler_.label(0, ".convert_loop");
        assembler_.instruction(1, op::xor_op, "rdx", "rdx");
        assembler_.instruction(1, op::div, "rcx");

        assembler_.instruction(
            1, op::add, "dl",
            assembler_x86_64::immediate::of_expression("'0'"));

        assembler_.instruction(1, op::mov,
                               assembler_x86_64::memory::of_base("rdi"), "dl");

        assembler_.instruction(1, op::dec, "rdi");
        assembler_.instruction(1, op::test, "rax", "rax");
        assembler_.jcc(1, condition::ne, ".convert_loop");
        assembler_.instruction(1, op::inc, "rdi");
        assembler_.comment(1, "print line number to stderr");
        assembler_.instruction(1, op::mov, "rax", syscall_write_);
        assembler_.instruction(1, op::mov, "rsi", "rdi");

        // the length is the end of the buffer minus the first digit
        const std::string buffer_end{
            std::format("num_buffer + {}", number_buffer_size_bytes),
        };

        assembler_.instruction(
            1, op::mov, "rdx",
            assembler_x86_64::immediate::of_qword_expression(buffer_end));

        assembler_.instruction(1, op::sub, "rdx", "rdi");
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor_);
        assembler_.instruction(1, op::syscall);
        exit(token{}, 1, qword_register(report_exit_code_register_));
        assembler_.begin_bss_section();
        assembler_.label(0, "num_buffer");
        assembler_.reserve(number_buffer_size_bytes);
        assembler_.switch_section(section::text);
    }

    // the routines take the sector in 'rdi', the address in 'rsi' and the size
    // in 'rdx'; the image file is opened at the first transfer, a sector past
    // its end or outside 32 bits, a failed open or a short transfer panics
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void override {

        // 'O_RDWR' and 'SEEK_END' of linux
        constexpr int read_write{2};
        constexpr int seek_end{2};
        constexpr int sector_shift{std::countr_zero(card_sector_size_bytes_)};

        // the descriptor is -1 until the file is opened
        constexpr std::array<int64_t, 1> not_opened{-1};

        constexpr std::array<int64_t, 1> zero_value{0};

        // the open file descriptor, a qword in '.data'
        const assembler_x86_64::memory descriptor{
            assembler_x86_64::memory::of_symbol("baz_sd_card_descriptor",
                                                size_qword_),
        };

        // the file size in whole sectors, set once at open
        const assembler_x86_64::memory sector_count{
            assembler_x86_64::memory::of_symbol("baz_sd_card_sectors",
                                                size_qword_),
        };

        assembler_.add_separator_newline();

        // the read entry selects 'pread64' and joins the shared body
        if (read_used) {
            assembler_.label(0, sector_read_label_);
            assembler_.instruction(1, op::mov, "eax", syscall_pread_);
            assembler_.jmp(1, "baz_sector_transfer");
        }

        // the write entry selects 'pwrite64' and falls through into the body
        if (write_used) {
            assembler_.label(0, sector_write_label_);
            assembler_.instruction(1, op::mov, "eax", syscall_pwrite_);
        }

        assembler_.label(0, "baz_sector_transfer");
        assembler_.comment(1, "the system call is kept on the stack");
        // 'rax' is reused by 'open' and 'lseek', so the number is saved
        assembler_.instruction(1, op::push, "rax");
        // a descriptor of 0 or more means the file is already open
        assembler_.instruction(1, op::cmp, descriptor, 0);
        assembler_.jcc(1, condition::ge, "baz_sector_opened");
        // first transfer: save the arguments, 'open' and 'lseek' clobber them
        assembler_.instruction(1, op::push, "rdi");
        assembler_.instruction(1, op::push, "rsi");
        assembler_.instruction(1, op::push, "rdx");
        // open(name, 'O_RDWR')
        assembler_.instruction(1, op::mov, "eax", syscall_open_);

        assembler_.instruction(
            1, op::lea, "rdi",
            assembler_x86_64::memory::of_symbol("baz_sd_card_name"));

        assembler_.instruction(1, op::mov, "esi", read_write);
        assembler_.instruction(1, op::syscall);
        // a negative result is '-errno', the open failed
        assembler_.instruction(1, op::test, "rax", "rax");
        assembler_.jcc(1, condition::s, sd_card_failure_.label);
        // keep the descriptor for later transfers
        assembler_.instruction(1, op::mov, descriptor, "rax");
        assembler_.comment(1, "the size of the file in sectors");
        // lseek(descriptor, 0, 'SEEK_END') returns the file size in bytes
        assembler_.instruction(1, op::mov, "rdi", "rax");
        assembler_.instruction(1, op::xor_op, "esi", "esi");
        assembler_.instruction(1, op::mov, "edx", seek_end);
        assembler_.instruction(1, op::mov, "eax", syscall_lseek_);
        assembler_.instruction(1, op::syscall);
        // a negative result is '-errno', the seek failed
        assembler_.instruction(1, op::test, "rax", "rax");
        assembler_.jcc(1, condition::s, sd_card_failure_.label);
        // bytes to whole sectors, a partial last sector is not counted
        assembler_.instruction(1, op::sar, "rax", sector_shift);
        assembler_.instruction(1, op::mov, sector_count, "rax");
        assembler_.instruction(1, op::pop, "rdx");
        assembler_.instruction(1, op::pop, "rsi");
        assembler_.instruction(1, op::pop, "rdi");
        // here the file is open and only the system call is on the stack
        assembler_.label(0, "baz_sector_opened");
        assembler_.comment(1, "the sector is an unsigned 32 bit number");
        // zero extending 'edi' differs from 'rdi' when the upper half is set
        assembler_.instruction(1, op::mov, "eax", "edi");
        assembler_.instruction(1, op::cmp, "rax", "rdi");
        assembler_.jcc(1, condition::ne, sd_card_failure_.label);
        // a sector at or past the end of the file panics
        assembler_.instruction(1, op::cmp, "rdi", sector_count);
        assembler_.jcc(1, condition::ae, sd_card_failure_.label);
        // the byte offset, the fourth system call argument, goes in 'r10'
        assembler_.instruction(1, op::mov, "r10", "rdi");
        assembler_.instruction(1, op::sal, "r10", sector_shift);
        // the descriptor replaces the sector as the first argument
        assembler_.instruction(1, op::mov, "rdi", descriptor);
        // the saved system call number, which also balances the stack
        assembler_.instruction(1, op::pop, "rax");
        // pread64 or pwrite64(descriptor, address 'rsi', size 'rdx', offset)
        assembler_.instruction(1, op::syscall);
        // anything but the full size, an error or a short transfer, panics
        assembler_.instruction(1, op::cmp, "rax", "rdx");
        assembler_.jcc(1, condition::ne, sd_card_failure_.label);
        assembler_.instruction(1, op::ret);

        // the stack is not unwound because the program exits
        emit_failure_handler(sd_card_failure_);

        assembler_.switch_section(section::rodata);
        assembler_.comment(0, "the file name ends with a zero byte");
        assembler_.label(0, "baz_sd_card_name");

        assembler_.string_data(options().sd_card_file_name);

        assembler_.data(size_byte_, zero_value);

        // writable state, qword aligned for the 64 bit accesses
        assembler_.switch_section(section::data);
        assembler_.align(size_qword_);
        assembler_.label(0, "baz_sd_card_descriptor");
        assembler_.data(size_qword_, not_opened);
        assembler_.label(0, "baz_sd_card_sectors");
        assembler_.data(size_qword_, zero_value);
        assembler_.switch_section(section::text);
    }

    // 'read' and 'write' are system calls, the sector functions routines
    auto emit_target_call(const token& src_loc_tk, const size_t indent,
                          const target_call function, const operand& result)
        -> void override {

        if (function == target_call::read) {
            io_syscall(src_loc_tk, indent, result, syscall_read_);
            return;
        }

        if (function == target_call::write) {
            io_syscall(src_loc_tk, indent, result, syscall_write_);
            return;
        }

        call_sector_routine(src_loc_tk, indent, function);
    }

    auto emit_variables_base_load() -> void override {
        assembler_.instruction(
            0, op::lea, variables_base_register(),
            assembler_x86_64::memory::of_symbol(data_label_));
    }

    [[nodiscard]] auto make_register_operand(const std::string_view name,
                                             const type& value_type) const
        -> operand override {

        return operand::reg(sized_register_name(name, value_type.size_bytes()),
                            value_type);
    }

    // the index in 'register_names_' of a register of any size
    [[nodiscard]] auto register_index_of(const std::string_view name) const
        -> std::optional<size_t> override {

        for (const auto [index, names] :
             std::views::enumerate(register_names_)) {

            if (name == names.qword or name == names.dword or
                name == names.word or name == names.byte) {

                return static_cast<size_t>(index);
            }
        }

        return std::nullopt;
    }

    auto restore_call_registers(const size_t indent,
                                const std::span<const std::string_view> saved)
        -> void override {

        for (const std::string_view name : saved | std::views::reverse) {
            assembler_.instruction(indent, op::pop, name);
        }
    }

    auto save_call_registers(const size_t indent,
                             const std::span<const std::string_view> saved)
        -> void override {

        for (const std::string_view name : saved) {
            assembler_.instruction(indent, op::push, name);
        }
    }

  private:
    //
    // overridden methods
    //

    [[nodiscard]] auto register_display_name(const size_t index) const
        -> std::string_view override {

        return register_names_.at(index).qword;
    }

    //
    // class methods
    //

    [[nodiscard]] auto address_is_encodable(const operand& value) const
        -> bool {

        if (not value.is_memory()) {
            return true;
        }

        // note: an index is a scratch register with an encodable scale,
        //       'stmt_identifier' applies another scale with 'scale_index'

        assert(value.index_register().empty() or
               (value.index_register() != "rsp" and
                can_lower_index_scale(value.scale())));

        return std::in_range<int32_t>(value.displacement());
    }

    // reserves 'bulk_registers_', freed by the 'operation_scope' of the
    // caller, and returns the count register
    [[nodiscard]] auto begin_bulk(const token& src_loc_tk, const size_t indent)
        -> operand {

        for (const std::string_view name : bulk_registers_) {
            reserve_named_register(src_loc_tk, indent, name);
        }

        return qword_register("rcx");
    }

    // jumps to the overflow failure handler when the instruction before set
    // the overflow flag
    auto branch_on_overflow(const token& src_loc_tk, const size_t indent)
        -> void {

        branch_to_failure(src_loc_tk, indent, condition::o, overflow_failure_);
    }

    // a reported line goes through the stub of its line, which sets
    // 'report_line_register_' for the handler, so a passing check keeps it and
    // loads nothing
    auto branch_to_failure(const token& src_loc_tk, const size_t indent,
                           const condition failed,
                           const failure_handler& handler) -> void {

        const std::string target{failure_target(handler, src_loc_tk)};

        assembler_.jcc(indent, failed, target);
    }

    // the built-in integer type of 'size_bytes'
    [[nodiscard]] auto
    builtin_type_for_size_bytes(const size_t size_bytes) const -> const type& {

        switch (size_bytes) {
        case size_qword_:
            return builtins().i64;

        case size_dword_:
            return builtins().i32;

        case size_word_:
            return builtins().i16;

        case size_byte_:
            return builtins().i8;

        default:
            std::unreachable();
        }
    }

    // the routine changes 'rax', 'rcx', 'r10' and 'r11' besides the
    // arguments, the allocated ones are saved around the call
    auto call_sector_routine(const token& src_loc_tk, const size_t indent,
                             const target_call function) -> void {

        static constexpr std::array<std::string_view, 4> clobbered{
            "rax",
            "rcx",
            "r10",
            "r11",
        };

        const std::string_view label{
            function == target_call::sector_read ? sector_read_label_
                                                 : sector_write_label_,
        };

        comment(src_loc_tk, indent, "sector of the sd card");
        mark_used(label);

        with_saved_registers(indent, clobbered,
                             [&] -> void { assembler_.call(indent, label); });
    }

    // 'left' holds the product of two bytes in a register of the default
    // width, which fits when the low byte sign extended is the same value
    auto check_byte_product(const token& src_loc_tk, const size_t indent,
                            const operand& left, const operand& right) -> void {

        mov(src_loc_tk, indent, right, sized_operand(left, size_byte_));
        cmp(src_loc_tk, indent, right, left);

        branch_to_failure(src_loc_tk, indent, condition::ne, overflow_failure_);
    }

    // the source in 'rsi', the destination in 'rdi' and the byte count in
    // 'rcx' of a copy: a distance from the source to the destination that is
    // not zero and below the count jumps to the failure handler, the copy
    // would overwrite source bytes it has not read yet
    auto check_copy_overlap(const token& src_loc_tk, const size_t indent)
        -> void {

        comment(src_loc_tk, indent, "overlap check begin");

        const std::string passed{
            std::format(".Lbaz_overlap.{}", next_local_label_number()),
        };

        const operation_scope scope{*this};

        const operand distance{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        mov(src_loc_tk, indent, distance, qword_register("rdi"));
        emit_op(src_loc_tk, indent, op::sub, distance, qword_register("rsi"));

        comment(src_loc_tk, indent, "the same range is not an overlap");

        assembler_.jcc(indent, condition::e, passed);

        comment(src_loc_tk, indent, "destination starts inside the source");

        cmp(src_loc_tk, indent, distance, qword_register("rcx"));
        branch_to_failure(src_loc_tk, indent, condition::b, overlap_failure_);
        assembler_.label(indent, passed);

        comment(src_loc_tk, indent, "overlap check end");
    }

    // a zero divisor and the minimum of the dividend width divided by -1 jump
    // to the failure handler, the dividend is in 'rax'
    auto check_divisor(const token& src_loc_tk, const size_t indent,
                       const operand& divisor, const size_t dividend_size_bytes)
        -> void {

        const std::string not_minus_one{
            std::format(".Lbaz_division.{}", next_local_label_number()),
        };

        comment(src_loc_tk, indent, "division check begin");

        comment(src_loc_tk, indent, "zero divisor");

        cmp(src_loc_tk, indent, divisor, immediate(0));
        branch_to_failure(src_loc_tk, indent, condition::e, division_failure_);

        comment(src_loc_tk, indent, "minimum divided by -1 overflows");

        cmp(src_loc_tk, indent, divisor, immediate(-1));
        assembler_.jcc(indent, condition::ne, not_minus_one);

        // note: a compare with 1 overflows only for the minimum of the width,
        //       so the minimum needs no immediate, which takes a register for
        //       a qword

        emit_op(src_loc_tk, indent, op::cmp,
                sized_register("rax", dividend_size_bytes), immediate(1));

        branch_to_failure(src_loc_tk, indent, condition::o, division_failure_);
        assembler_.label(indent, not_minus_one);

        comment(src_loc_tk, indent, "division check end");
    }

    // a count in 'rcx' below zero or not below the width jumps to the failure
    // handler, a negative count is a large unsigned one
    auto check_shift_count(const token& src_loc_tk, const size_t indent,
                           const type& dst_type) -> void {

        comment(src_loc_tk, indent, "shift check begin");

        cmp(src_loc_tk, indent, sized_register("rcx", dst_type.size_bytes()),
            immediate(static_cast<int64_t>(dst_type.size_bits())));

        branch_to_failure(src_loc_tk, indent, condition::ae, shift_failure_);

        comment(src_loc_tk, indent, "shift check end");
    }

    // branches to the failure handler when the range passes the array count, a
    // range 'index + count' may end at it
    auto check_upper_bound(const token& src_loc_tk, const size_t indent,
                           const bounds_range& range, const bool lower_checked)
        -> void {

        const condition out_of_bounds{
            out_of_bounds_condition(range.allow_end, lower_checked),
        };

        if (not range.count) {
            cmp(src_loc_tk, indent, range.checked,
                immediate(range.array_count));

            branch_to_failure(src_loc_tk, indent, out_of_bounds,
                              bounds_failure_);

            return;
        }

        const operand& count{*range.count};

        const operation_scope scope{*this};

        const operand reg_top_idx{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        // note: index and count registers have the default type
        assert(count.type_ref().size_bytes() == size_qword_ and
               range.checked.type_ref().size_bytes() == size_qword_);

        // the end of a range is 'start + count'; after the lower checks
        // neither is negative, the sum can pass the signed maximum and then
        // looks negative to a signed compare, the unsigned compare sees it
        // large, e.g. 'array_copy(x[i], y, n)' on 'i8[4]' with 'i' 1 and 'n'
        // 0x7fffffffffffffff ends at 0x8000000000000000, signed it is below 4
        // and passes
        if (lower_checked) {
            lea(src_loc_tk, indent, reg_top_idx,
                operand::mem(count.register_name(),
                             range.checked.register_name(), 1, 0,
                             builtins().i64));
        } else {
            // an end that does not fit 64 bits fails, a negative end is left
            // to the lower check
            mov(src_loc_tk, indent, reg_top_idx, range.checked);
            emit_op(src_loc_tk, indent, op::add, reg_top_idx, count);

            branch_to_failure(src_loc_tk, indent, condition::o,
                              bounds_failure_);
        }

        cmp(src_loc_tk, indent, reg_top_idx, immediate(range.array_count));
        branch_to_failure(src_loc_tk, indent, out_of_bounds, bounds_failure_);
    }

    // 'test' of a register with itself sets the flags of a compare with 0 and
    // is shorter
    auto cmp(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        if (dst_op.is_register() and src_op.is_immediate() and
            src_op.immediate() == 0) {

            emit_op(src_loc_tk, indent, op::test, dst_op, dst_op);
            return;
        }

        emit_op(src_loc_tk, indent, op::cmp, dst_op, src_op);
    }

    // 'load_source_address' sets the 'rep movsb' source pointer
    auto copy_with_rep_movsb(
        const token& src_loc_tk, const size_t indent, const operand& dst,
        const size_t size_bytes,
        const std::function_ref<void(const operand&)> load_source_address)
        -> void {

        const operation_scope bulk{*this};
        const operand count{begin_bulk(src_loc_tk, indent)};

        load_source_address(qword_register("rsi"));
        lea(src_loc_tk, indent, qword_register("rdi"), dst);
        mov(src_loc_tk, indent, count, immediate(size_bytes));

        assembler_.instruction(indent, op::rep_movsb);
    }

    // writes the instruction with the operands as they are
    auto emit(const size_t indent, const op code, const operand& dst,
              const operand& src) -> void {

        assembler_.instruction(indent, code, to_argument(dst),
                               to_argument(src));
    }

    // matches the widths of the operands with scratch registers and sign
    // extension, and lowers addresses x86 cannot encode
    auto emit_op(const token& src_loc_tk, const size_t indent, const op code,
                 const operand& dst_op, const operand& src_op) -> void {

        if (code == op::mov and same_operand(dst_op, src_op)) {
            return;
        }

        if (not address_is_encodable(dst_op) or
            not address_is_encodable(src_op)) {

            // registers the operands refer to must not be picked for lowering
            const operation_scope scope{*this, dst_op, src_op};

            const operand dst{lower_address(src_loc_tk, indent, dst_op)};
            const operand src{lower_address(src_loc_tk, indent, src_op)};

            emit_op(src_loc_tk, indent, code, dst, src);

            return;
        }

        if (src_op.is_immediate() and
            not needs_immediate_register(code, dst_op, src_op)) {

            emit(indent, code, dst_op, src_op);
            return;
        }

        // a wide immediate, and memory to memory, which x86 has no form for
        if (src_op.is_immediate() or
            (dst_op.is_memory() and src_op.is_memory())) {

            emit_through_register(src_loc_tk, indent, code, dst_op, src_op);
            return;
        }

        const size_t dst_size_bytes{dst_op.type_ref().size_bytes()};
        const size_t src_size_bytes{src_op.type_ref().size_bytes()};

        if (dst_size_bytes == src_size_bytes) {
            emit(indent, code, dst_op, src_op);
            return;
        }

        if (dst_size_bytes > src_size_bytes) {
            emit_widening_op(src_loc_tk, indent, code, dst_op, src_op);
            return;
        }

        assert(dst_op.is_register() or src_op.is_register());

        // the low part of the source is used, so a narrower destination needs
        // no extension
        emit(indent, code, dst_op, sized_operand(src_op, dst_size_bytes));
    }

    // idiv takes the divisor as a qword register or memory operand and the
    // dividend in 'rax', which 'cqo' extends into 'rdx'
    auto emit_signed_divide(const token& src_loc_tk, const size_t indent,
                            const operand& divisor, const runtime_check& check,
                            const size_t dividend_size_bytes) -> void {

        const operation_scope scope{*this};

        operand qword_divisor{divisor};

        if (divisor.is_immediate() or
            divisor.type_ref().size_bytes() != size_qword_) {

            qword_divisor =
                alloc_scratch_register(src_loc_tk, indent, default_type());

            mov(src_loc_tk, indent, qword_divisor, divisor);
        }

        if (check.enabled) {
            check_divisor(src_loc_tk, indent, qword_divisor,
                          dividend_size_bytes);
        }

        assembler_.instruction(indent, op::cqo);
        emit_unary(src_loc_tk, indent, op::idiv, qword_divisor);
    }

    // the source goes through a register of the destination width, which
    // sign extends it
    auto emit_through_register(const token& src_loc_tk, const size_t indent,
                               const op code, const operand& dst_op,
                               const operand& src_op) -> void {

        const operation_scope scope{*this};

        const operand reg{
            scope.alloc_load_register(
                src_loc_tk, indent,
                builtin_type_for_size_bytes(dst_op.type_ref().size_bytes()),
                dst_op, src_op),
        };

        mov(src_loc_tk, indent, reg, src_op);
        emit(indent, code, dst_op, reg);
    }

    auto emit_unary(const token& src_loc_tk, const size_t indent, const op code,
                    const operand& value) -> void {

        const operation_scope scope{*this, value};
        const operand lowered{lower_address(src_loc_tk, indent, value)};

        assembler_.instruction(indent, code, to_argument(lowered));
    }

    // the source is sign extended to the destination width
    auto emit_widening_op(const token& src_loc_tk, const size_t indent,
                          const op code, const operand& dst_op,
                          const operand& src_op) -> void {

        const size_t dst_size_bytes{dst_op.type_ref().size_bytes()};

        // 'movsx' needs a register destination, so the source register is
        // extended in place, its low bits keep the value
        if (code == op::mov and dst_op.is_memory()) {
            const operand wide{sized_operand(src_op, dst_size_bytes)};

            emit(indent, op::movsx, wide, src_op);
            emit(indent, op::mov, dst_op, wide);

            return;
        }

        if (code == op::mov) {
            emit(indent, op::movsx, dst_op, src_op);
            return;
        }

        // the count of a shift is not extended
        if (code == op::sal or code == op::sar) {
            emit(indent, code, dst_op, src_op);
            return;
        }

        emit_through_register(src_loc_tk, indent, code, dst_op, src_op);
    }

    template <std::integral value_t>
    [[nodiscard]] auto immediate(const value_t value) const -> operand {
        return operand::imm(value, default_type());
    }

    // 'syscall' changes 'rcx' and 'r11', the allocated ones are saved
    auto io_syscall(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const int syscall_number) -> void {

        static constexpr std::array<std::string_view, 2> clobbered{
            "rcx",
            "r11",
        };

        mov(src_loc_tk, indent, dst, immediate(syscall_number));

        with_saved_registers(indent, clobbered, [&] -> void {
            assembler_.instruction(indent, op::syscall);
        });
    }

    [[nodiscard]] auto
    is_any_held(const std::span<const std::string_view> named) const -> bool {

        return std::ranges::any_of(named,
                                   [&](const std::string_view name) -> bool {
                                       return is_register_unavailable(name);
                                   });
    }

    auto lea(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& address) -> void {

        // note: 'lea' writes only registers, 'address_of' moves through one

        assert(dst.is_register());

        const operation_scope scope{*this, dst, address};
        const operand lowered{lower_address(src_loc_tk, indent, address)};

        assembler_.instruction(indent, op::lea, to_argument(dst),
                               to_address(lowered));
    }

    // the bulk registers of 'rep movsb' and 'repe cmpsb': 'rcx' the count in
    // bytes, 'rsi' the address of 'to_rsi' and 'rdi' that of 'to_rdi'
    auto load_bulk_registers(
        const token& src_loc_tk, const size_t indent,
        const size_t element_size_bytes,
        const std::function_ref<void(const operand&)> emit_count,
        const address_emitter& to_rsi, const address_emitter& to_rdi) -> void {

        const operand count{begin_bulk(src_loc_tk, indent)};

        to_rsi(std::nullopt, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rsi"), address);
        });

        to_rdi(std::nullopt, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rdi"), address);
        });

        emit_count(count);

        scale_index(src_loc_tk, indent, count, element_size_bytes);
    }

    // the register it allocates is freed by the 'operation_scope' of the
    // caller
    [[nodiscard]] auto lower_address(const token& src_loc_tk,
                                     const size_t indent, const operand& value)
        -> operand {

        if (address_is_encodable(value)) {
            return value;
        }

        const operand address{
            alloc_scratch_register(src_loc_tk, indent, builtins().i64),
        };

        const std::string_view sum{address.register_name()};
        assembler_.instruction(indent, op::mov, sum, value.displacement());

        assembler_.instruction(indent, op::lea, sum,
                               register_sum(value.base_register(), sum, 1));

        // note: 'address_is_encodable' asserted the index and its scale
        if (not value.index_register().empty()) {
            assembler_.instruction(
                indent, op::lea, sum,
                register_sum(sum, value.index_register(), value.scale()));
        }

        return operand::mem(address.register_name(), {}, 1, 0,
                            value.type_ref());
    }

    // an unrolled part of 'address'
    [[nodiscard]] auto memory_part(const operand& address,
                                   const size_t part_size_bytes,
                                   const size_t offset) const -> operand {

        const operand part{sized_operand(address, part_size_bytes)};
        return offset_by(part, offset);
    }

    auto mov(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        emit_op(src_loc_tk, indent, op::mov, dst_op, src_op);
    }

    // e.g. the fixed registers of 'rep movsb' and syscalls
    [[nodiscard]] auto qword_register(const std::string_view name) const
        -> operand {

        return make_register_operand(name, builtins().i64);
    }

    // a register as its part of the width, a memory operand as an access
    // of the width
    [[nodiscard]] auto sized_operand(const operand& value,
                                     const size_t size_bytes) const -> operand {

        const type& sized_type{builtin_type_for_size_bytes(size_bytes)};

        if (value.is_register()) {
            return register_view(value, sized_type);
        }

        assert(value.is_memory());

        return operand::mem(value, sized_type);
    }

    [[nodiscard]] auto sized_register(const std::string_view name,
                                      const size_t size_bytes) const
        -> operand {

        return make_register_operand(name,
                                     builtin_type_for_size_bytes(size_bytes));
    }

    [[nodiscard]] auto sized_register_name(const std::string_view name,
                                           const size_t size_bytes) const
        -> std::string {

        const std::optional<size_t> row{register_index_of(name)};

        // note: every register name is in 'register_names_'
        assert(row);

        // a register is allocated by its 64-bit name, an operand names the
        // part of its width
        const register_names& names{register_names_.at(*row)};

        switch (size_bytes) {
        case size_qword_:
            return std::string{names.qword};

        case size_dword_:
            return std::string{names.dword};

        case size_word_:
            return std::string{names.word};

        case size_byte_:
            return std::string{names.byte};

        default:
            std::unreachable();
        }
    }

    // 'setcc' writes one byte
    auto store_condition(const token& src_loc_tk, const size_t indent,
                         const condition cc, const operand& dst) -> void {

        const operation_scope scope{*this, dst};
        const operand byte{sized_operand(dst, size_byte_)};
        const operand lowered{lower_address(src_loc_tk, indent, byte)};

        assembler_.setcc(indent, cc, to_argument(lowered));
    }

    // the rep form when the parts take more instructions than it; when one of
    // its 'named' registers is held, only beyond twice its instructions, so a
    // value that is not small is never unrolled
    [[nodiscard]] auto
    takes_rep_form(const size_t part_instructions,
                   const std::span<const std::string_view> named) const
        -> bool {

        if (part_instructions <= rep_form_instructions_) {
            return false;
        }

        return not is_any_held(named) or
               part_instructions > 2 * rep_form_instructions_;
    }

    // a negative value is out of bounds
    auto test_sign(const token& src_loc_tk, const size_t indent,
                   const operand& value) -> void {

        emit_op(src_loc_tk, indent, op::test, value, value);
        branch_to_failure(src_loc_tk, indent, condition::s, bounds_failure_);
    }

    //
    // statics
    //

    [[nodiscard]] static auto
    condition_for_comparison(const comparison_operator holds) -> condition {

        if (holds == comparison_operator::equal) {
            return condition::e;
        }

        if (holds == comparison_operator::not_equal) {
            return condition::ne;
        }

        if (holds == comparison_operator::less) {
            return condition::l;
        }

        if (holds == comparison_operator::less_equal) {
            return condition::le;
        }

        if (holds == comparison_operator::greater) {
            return condition::g;
        }

        assert(holds == comparison_operator::greater_equal);

        return condition::ge;
    }

    // unrolled accesses take the widest parts first so each narrower width
    // covers at most one remaining part
    static auto for_each_part(
        const size_t size_bytes, const size_t widest_size_bytes,
        const std::function_ref<void(size_t part_size_bytes, size_t offset)>
            emit_part) -> void {

        size_t offset{};
        for (size_t w{widest_size_bytes}; w != 0; w /= 2) {
            while (size_bytes - offset >= w) {
                emit_part(w, offset);
                offset += w;
            }
        }
    }

    // the qwords, then the remaining dword, word and byte
    [[nodiscard]] static auto memory_equal_compares(const size_t size_bytes)
        -> std::vector<op> {

        std::vector<op> compares;
        size_t remaining_bytes{size_bytes};

        if (size_bytes / size_qword_ > threshold_for_repe_cmpsq_count_) {
            compares.push_back(op::repe_cmpsq);
            remaining_bytes %= size_qword_;
        }

        for_each_part(remaining_bytes, size_qword_,
                      [&](const size_t part_size_bytes,
                          [[maybe_unused]] const size_t offset) -> void {
                          compares.push_back(string_compare(part_size_bytes));
                      });

        return compares;
    }

    // x86 sign-extends 32-bit immediates and only 'mov' to a register takes 64
    // bits
    [[nodiscard]] static auto needs_immediate_register(const op code,
                                                       const operand& dst,
                                                       const operand& src)
        -> bool {

        if (dst.type_ref().size_bytes() != size_qword_) {
            return false;
        }

        if (code == op::mov and dst.is_register()) {
            return false;
        }

        const std::optional<uint64_t> bits{immediate_bits(src)};

        assert(bits);

        return not std::in_range<int32_t>(std::bit_cast<int64_t>(*bits));
    }

    // the unsigned upper comparison also fails a negative value when the lower
    // bound is covered by it
    [[nodiscard]] static auto out_of_bounds_condition(const bool allow_end,
                                                      const bool lower_checked)
        -> condition {

        if (lower_checked) {
            return allow_end ? condition::a : condition::ae;
        }

        return allow_end ? condition::g : condition::ge;
    }

    // the parts 'for_each_part' takes from qwords down, e.g. 3 for 13 B: a
    // qword, a dword and a byte
    [[nodiscard]] static auto part_count(const size_t size_bytes) -> size_t {
        const size_t qword_count{size_bytes / size_qword_};
        const size_t tail_bytes{size_bytes % size_qword_};
        return qword_count + static_cast<size_t>(std::popcount(tail_bytes));
    }

    [[nodiscard]] static auto register_sum(const std::string_view base,
                                           const std::string_view index,
                                           const uint64_t scale)
        -> assembler_x86_64::memory {

        return {
            .symbol{},
            .base{base},
            .index{index},
            .scale{scale},
            .displacement{},
            .size_bytes{},
        };
    }

    [[nodiscard]] static auto same_operand(const operand& lhs,
                                           const operand& rhs) -> bool {

        if (lhs.is_register()) {
            return rhs.is_register() and
                   lhs.register_name() == rhs.register_name();
        }

        assert(lhs.is_memory());

        if (not rhs.is_memory()) {
            return false;
        }

        return lhs.type_ref().size_bytes() == rhs.type_ref().size_bytes() and
               lhs.is_same_address(rhs);
    }

    [[nodiscard]] static auto string_compare(const size_t size_bytes) -> op {
        switch (size_bytes) {
        case size_qword_:
            return op::cmpsq;

        case size_dword_:
            return op::cmpsd;

        case size_word_:
            return op::cmpsw;

        case size_byte_:
            return op::cmpsb;

        default:
            std::unreachable();
        }
    }

    // the address without a width, as 'lea' takes it
    [[nodiscard]] static auto to_address(const operand& value)
        -> assembler_x86_64::memory {

        assert(not value.is_immediate());

        return {
            .symbol{},
            .base{value.base_register()},
            .index{value.index_register()},
            .scale{value.scale()},
            .displacement{value.displacement()},
            .size_bytes{},
        };
    }

    [[nodiscard]] static auto to_argument(const operand& value)
        -> assembler_x86_64::argument {

        if (value.is_immediate()) {
            return assembler_x86_64::immediate{value.immediate()};
        }

        if (value.is_register()) {
            return std::string_view{value.register_name()};
        }

        assembler_x86_64::memory address{to_address(value)};
        address.size_bytes = value.type_ref().size_bytes();

        return address;
    }

    // see 'target_description'
    [[nodiscard]] static constexpr auto x86_64_target() -> target_description {
        return {
            .address_size_bytes{size_qword_},
            .data_alignment{data_alignment_},
            .max_storage_bytes{user_address_space_bytes_},
            .has_indexed_addressing{true},
            .max_index_scale_bytes{size_qword_},
            .max_padding_gaps_zeroed_one_by_one{
                max_padding_gaps_zeroed_one_by_one_,
            },
            .memory_counter_pressure_count{memory_counter_pressure_count_},
            .has_sd_card{true},
            .string_alignment{1},
            .is_stack_bounded{},
            .frame_base_register{"rbx"},
            .variables_base_register{"rbp"},
            .slot_register{"r11"},
            .variables_base_past_vars_bytes{std::nullopt},
            .scratch_registers{scratch_registers_},
            .target_calls{
                .arguments{"rdi", "rsi", "rdx"},
                .result{"rax"},
            },
            .routines{
                .exit{exit_label_},
                .report_line{report_line_label_},
                .sector_read{sector_read_label_},
                .sector_write{sector_write_label_},
            },
            .report_line_register{report_line_register_},
            .report_exit_code_register{report_exit_code_register_},
        };
    }
};
