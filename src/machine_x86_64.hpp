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
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "assembler_x86_64.hpp"
#include "constant_arith.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "register_pool.hpp"
#include "token.hpp"
#include "type.hpp"

// the x86_64 linux backend, written as nasm text through 'assembler_x86_64';
// the formatter sorts the functions by name, so by topic they are:
//
//   registers    'machine' allocates them, here the names
//                'register_names_', the order 'scratch_registers_', the
//                roles in 'x86_64_target'
//   operations   'emit_op' chooses the form of an instruction,
//                'with_lowered_addresses' fits an operand into one,
//                'multiply_by_constant', 'emit_signed_divide', 'shift'
//   memory       'copy_block', 'store_bytes', 'zero' unrolled or with
//                'rep', the bulk operations with 'rsi', 'rdi' and 'rcx'
//   comparisons  'compare_and_branch', 'store_condition', 'memory_equal'
//   checks       'check_bounds', 'check_divisor', 'check_shift_count',
//                'check_copy_overlap', 'check_frame_capacity', each jumps to
//                its handler, all but the frame check with a stub per line
//                for '--checks=line'
//   handlers     'emit_failure_handler'
//   calls        'call_function', the system calls 'io_syscall' and 'exit',
//                the sector routines 'emit_sector_routines'
class machine_x86_64 final : public machine {
    using op = assembler_x86_64::op;

    using condition = assembler_x86_64::condition;

    using section = assembler_x86_64::section;

    static constexpr size_t size_qword_{assembler_x86_64::size_qword};
    static constexpr size_t size_dword_{assembler_x86_64::size_dword};
    static constexpr size_t size_word_{assembler_x86_64::size_word};
    static constexpr size_t size_byte_{assembler_x86_64::size_byte};

    static constexpr size_t data_alignment_{16};

    // the user address space of x86_64 Linux, 128 TiB
    static constexpr size_t user_address_space_bytes_{0x800000000000};

    static constexpr std::array<size_t, 4> index_register_scalings_{1, 2, 4, 8};

    static constexpr size_t threshold_for_rep_stos_size_bytes_{32};

    // note: a gap takes one to three stores, the whole instance a few qword
    //       stores up to 32 B and beyond 'rep stosb' with its setup, four
    //       instructions; measured on 2500 random types with padding, from 5
    //       gaps the whole instance was never larger, at 4 gaps it was larger
    //       by one instruction in 32 of 128

    static constexpr size_t max_padding_gaps_zeroed_one_by_one_{4};

    static constexpr size_t threshold_for_rep_movs_size_bytes_{16};
    // up to this many qwords single compares cost less than loading 'rcx'
    static constexpr size_t threshold_for_repe_cmpsq_count_{2};
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

    struct register_names {
        std::string_view qword;
        std::string_view dword;
        std::string_view word;
        std::string_view byte;
    };

    static constexpr std::array<register_names, 16> register_names_{
        {
            {
                .qword{"rax"},
                .dword{"eax"},
                .word{"ax"},
                .byte{"al"},
            },
            {
                .qword{"rbx"},
                .dword{"ebx"},
                .word{"bx"},
                .byte{"bl"},
            },
            {
                .qword{"rcx"},
                .dword{"ecx"},
                .word{"cx"},
                .byte{"cl"},
            },
            {
                .qword{"rdx"},
                .dword{"edx"},
                .word{"dx"},
                .byte{"dl"},
            },
            {
                .qword{"rbp"},
                .dword{"ebp"},
                .word{"bp"},
                .byte{"bpl"},
            },
            {
                .qword{"rsi"},
                .dword{"esi"},
                .word{"si"},
                .byte{"sil"},
            },
            {
                .qword{"rdi"},
                .dword{"edi"},
                .word{"di"},
                .byte{"dil"},
            },
            {
                .qword{"rsp"},
                .dword{"esp"},
                .word{"sp"},
                .byte{"spl"},
            },
            {
                .qword{"r8"},
                .dword{"r8d"},
                .word{"r8w"},
                .byte{"r8b"},
            },
            {
                .qword{"r9"},
                .dword{"r9d"},
                .word{"r9w"},
                .byte{"r9b"},
            },
            {
                .qword{"r10"},
                .dword{"r10d"},
                .word{"r10w"},
                .byte{"r10b"},
            },
            {
                .qword{"r11"},
                .dword{"r11d"},
                .word{"r11w"},
                .byte{"r11b"},
            },
            {
                .qword{"r12"},
                .dword{"r12d"},
                .word{"r12w"},
                .byte{"r12b"},
            },
            {
                .qword{"r13"},
                .dword{"r13d"},
                .word{"r13w"},
                .byte{"r13b"},
            },
            {
                .qword{"r14"},
                .dword{"r14d"},
                .word{"r14w"},
                .byte{"r14b"},
            },
            {
                .qword{"r15"},
                .dword{"r15d"},
                .word{"r15w"},
                .byte{"r15b"},
            },
        },
    };

    static constexpr std::array<std::string_view, 14> scratch_registers_{
        "r15", "r14", "r13", "r12", "r10", "r9",  "r8",
        "r11", "rbx", "rdx", "rax", "rsi", "rdi", "rcx",
    };

    // note: in order of likelihood they are not used by name, 'rsi', 'rdi'
    //       and 'rcx' last since 'rep movsb' and 'repe cmpsb' need all three

    // scratch values live from which an array loop counts in memory
    static constexpr size_t memory_counter_pressure_count_{6};

    // numbers the labels after the parts of a memory compare
    size_t equal_label_count_{};
    // numbers the labels of the checks of divisions
    size_t division_label_count_{};
    // numbers the labels of the checks of copy overlaps
    size_t overlap_label_count_{};
    assembler_x86_64 assembler_;

  public:
    machine_x86_64(const builtin_type_set& builtins, const source_files& files,
                   const target_options& options = {})
        : machine{builtins, files, options, x86_64_target()} {}

    //
    // overridden methods
    //

    auto add_subtract(const token& src_loc_tk, const size_t indent,
                      const arithmetic_operator operation, const operand& dst,
                      const operand& src, const runtime_check& check = {})
        -> void override {

        assert(operation == arithmetic_operator::add or
               operation == arithmetic_operator::subtract);

        // adding or subtracting 0 cannot overflow
        if (check.enabled and src.is_immediate() and
            immediate_bits(src) == std::optional<uint64_t>{0}) {

            return;
        }

        emit_op(src_loc_tk, indent,
                operation == arithmetic_operator::add ? op::add : op::sub, dst,
                src);

        if (check.enabled) {
            branch_on_overflow(src_loc_tk, indent);
        }
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        lea_into(src_loc_tk, indent, dst, address);
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

    [[nodiscard]] auto can_lower_index_scale(const size_t size_bytes) const
        -> bool override {

        return std::ranges::contains(index_register_scalings_, size_bytes);
    }

    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const bounds_range& range,
                      const bounds_check_options& options) -> void override {

        if (not options.upper and not options.lower) {
            return;
        }

        const bounds_plan plan{
            plan_bounds_check(options, range, signed_maximum(range.checked)),
        };

        comment(src_loc_tk, indent, "bounds check begin");

        if (options.lower) {
            check_lower_bound(src_loc_tk, indent, range, plan);
        }

        if (options.upper) {
            comment(src_loc_tk, indent, "upper bound");

            // the end of a range is 'start + count', neither is negative
            // here but the sum can pass the signed maximum and then looks
            // negative to a signed compare, the unsigned compare sees it large
            // e.g. 'array_copy(x[0x7fffffffffffffff], y[0], 1)' on 'i8[4]'
            // ends at 0x8000000000000000, signed it is below 4 and passes
            const bool is_unsigned_compare{
                plan.upper_covers_lower or (options.lower and range.count),
            };

            check_upper_bound(src_loc_tk, indent, range, is_unsigned_compare);
        }

        comment(src_loc_tk, indent, "bounds check end");
    }

    auto check_frame_capacity(const token& src_loc_tk, const size_t indent,
                              const operand& frame_address,
                              const operand& frame_size_bytes)
        -> void override {

        assert(frame_address.is_memory());
        assert(frame_size_bytes.is_symbol());

        comment(src_loc_tk, indent, "frame capacity check begin");

        const operand start{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        lea(src_loc_tk, indent, start, frame_address);

        assembler_.instruction(
            indent, op::lea, to_argument(remaining),
            assembler_x86_64::memory::of_symbol(variables_label));

        cmp(src_loc_tk, indent, start, remaining);

        branch_to_failure(src_loc_tk, indent, condition::b,
                          frame_overflow_failure);

        // an absolute address reaches beyond the 2 GiB of 'rip' relative ones
        assembler_.instruction(indent, op::mov, to_argument(remaining),
                               assembler_x86_64::immediate::of_qword_expression(
                                   variables_end_label));

        cmp(src_loc_tk, indent, start, remaining);

        branch_to_failure(src_loc_tk, indent, condition::a,
                          frame_overflow_failure);

        emit_op(src_loc_tk, indent, op::sub, remaining, start);

        // note: the value of 'start' is no longer needed, its register is
        //       reused

        mov(src_loc_tk, indent, start, frame_size_bytes);
        cmp(src_loc_tk, indent, start, remaining);

        branch_to_failure(src_loc_tk, indent, condition::a,
                          frame_overflow_failure);

        free_scratch_register(src_loc_tk, indent, remaining);
        free_scratch_register(src_loc_tk, indent, start);
        comment(src_loc_tk, indent, "frame capacity check end");
    }

    auto compare_and_branch(const token& src_loc_tk, const size_t indent,
                            const operand& lhs, const operand& rhs,
                            const comparison_action& action) -> void override {

        cmp(src_loc_tk, indent, lhs, rhs);

        if (not action.destination.is_empty()) {
            store_condition(
                src_loc_tk, indent,
                condition_for_comparison(action.operation, action.inverted),
                action.destination);
        }

        if (not action.target.empty()) {
            branch_comparison(indent, action.operation,
                              action.branch_on_true ? action.inverted
                                                    : not action.inverted,
                              action.target);
        }
    }

    // x86 accesses unaligned memory so the alignment is not needed
    auto copy_block(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src,
                    const size_t size_bytes,
                    [[maybe_unused]] const size_t alignment) -> void override {

        if (size_bytes > threshold_for_rep_movs_size_bytes_) {
            copy_with_rep_movsb(src_loc_tk, indent, dst, size_bytes,
                                [&](const operand& pointer) -> void {
                                    lea(src_loc_tk, indent, pointer, src);
                                });

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_movs_size_bytes_);

        const operand tmp{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        for_each_part(
            size_bytes, size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const operand reg{sized_register(tmp, part_size_bytes)};

                mov(src_loc_tk, indent, reg,
                    memory_part(src, part_size_bytes, offset));

                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset), reg);
            });

        free_scratch_register(src_loc_tk, indent, tmp);
    }

    auto copy_elements(const token& src_loc_tk, const size_t indent,
                       const size_t element_size_bytes,
                       const std::function_ref<void(const operand&)> emit_count,
                       const copy_request& request) -> void override {

        load_bulk_registers(src_loc_tk, indent, element_size_bytes, emit_count,
                            request.src, request.dst);

        if (request.overlap.enabled) {
            check_copy_overlap(src_loc_tk, indent);
        }

        assembler_.instruction(indent, op::rep_movsb);
        release_bulk(src_loc_tk, indent);
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

        reserve_named_register(src_loc_tk, indent, "rax", default_type());
        mov(src_loc_tk, indent, qword_register("rax"), dst);

        reserve_named_register(src_loc_tk, indent, "rdx", default_type());

        emit_signed_divide(src_loc_tk, indent, divisor, check,
                           dst.type_ref().size_bytes());

        mov(src_loc_tk, indent, dst,
            qword_register(operation == arithmetic_operator::divide ? "rax"
                                                                    : "rdx"));

        release_named_register(src_loc_tk, indent, "rdx");
        release_named_register(src_loc_tk, indent, "rax");
    }

    auto end_main() -> void override {
        exit(token{}, 1, immediate(0));
        assembler_.add_separator_newline();
    }

    auto
    equal_elements(const token& src_loc_tk, const size_t indent,
                   const size_t element_size_bytes,
                   const std::function_ref<void(const operand&)> emit_count,
                   const equality_request& request) -> void override {

        load_bulk_registers(src_loc_tk, indent, element_size_bytes, emit_count,
                            request.lhs, request.rhs);

        // note: a zero count compares nothing and leaves the flags of 'test',
        //       ZF set, so it compares equal

        test(src_loc_tk, indent, qword_register("rcx"), qword_register("rcx"));
        assembler_.instruction(indent, op::repe_cmpsb);
        release_bulk(src_loc_tk, indent);

        store_condition(src_loc_tk, indent,
                        condition_for_comparison(comparison_operator::equal,
                                                 request.inverted),
                        request.dst);
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, qword_register("rdi"), exit_code);

        mov(src_loc_tk, indent, qword_register("rax"),
            immediate(syscall_exit_));

        syscall(indent);
    }

    auto foo_advance_iteration(const token& src_loc_tk, const size_t indent,
                               const operand& iterator, const operand& counter,
                               const size_t element_size_bytes,
                               const operand& limit,
                               const std::string_view loop_label)
        -> void override {

        add(src_loc_tk, indent, iterator, immediate(element_size_bytes));
        inc(src_loc_tk, indent, counter);
        cmp(src_loc_tk, indent, counter, limit);
        assembler_.jcc(indent, condition::ne, loop_label);
    }

    // 'inc' and 'cmp' work on memory, so a loop counter leaves its register to
    // the values that need one once many are live
    [[nodiscard]] auto foo_counter_in_memory() const -> bool override {
        return registers().scratch_count() >= memory_counter_pressure_count_;
    }

    [[nodiscard]] auto make_register_operand(const std::string_view name,
                                             const type& value_type) const
        -> operand override {

        const size_t size_bytes{register_size_bytes(name)};

        assert(size_bytes != 0);

        if (size_bytes == value_type.size_bytes()) {
            return operand::reg(name, value_type);
        }

        return operand::reg(sized_register_name(name, value_type.size_bytes()),
                            value_type);
    }

    auto memory_equal(const token& src_loc_tk, const size_t indent,
                      const size_t size_bytes, const equality_request& request)
        -> void override {

        const std::vector<op> compares{memory_equal_compares(size_bytes)};

        assert(not compares.empty());

        // only 'repe cmpsq' counts in 'rcx'
        const bool is_counted{compares.front() == op::repe_cmpsq};

        reserve_named_register(src_loc_tk, indent, "rsi", default_type());
        reserve_named_register(src_loc_tk, indent, "rdi", default_type());

        if (is_counted) {
            reserve_named_register(src_loc_tk, indent, "rcx", default_type());
        }

        load_bulk_addresses(src_loc_tk, indent, std::nullopt, request.lhs,
                            request.rhs);

        if (is_counted) {
            mov(src_loc_tk, indent, qword_register("rcx"),
                immediate(size_bytes / size_qword_));
        }

        // a part that differs decides the result, so the rest is skipped
        std::string end_label;

        if (compares.size() > 1) {
            end_label = std::format(".Lbaz_equal.{}", equal_label_count_++);
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

        if (is_counted) {
            release_named_register(src_loc_tk, indent, "rcx");
        }

        release_named_register(src_loc_tk, indent, "rdi");
        release_named_register(src_loc_tk, indent, "rsi");

        store_condition(src_loc_tk, indent,
                        condition_for_comparison(comparison_operator::equal,
                                                 request.inverted),
                        request.dst);
    }

    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  const factor_use use, const runtime_check& check = {})
        -> void override {

        if (multiply_by_constant(src_loc_tk, indent, product, factor, check)) {
            return;
        }

        if (product.type_ref().size_bytes() == size_byte_) {
            const operand left{
                alloc_scratch_register(src_loc_tk, indent, default_type()),
            };

            const operand right{
                alloc_scratch_register(src_loc_tk, indent, default_type()),
            };

            mov(src_loc_tk, indent, left, product);
            mov(src_loc_tk, indent, right, factor);
            imul(src_loc_tk, indent, left, right);

            if (check.enabled) {
                check_byte_product(src_loc_tk, indent, left, right);
            }

            mov(src_loc_tk, indent, product, left);
            free_scratch_register(src_loc_tk, indent, right);
            free_scratch_register(src_loc_tk, indent, left);

            return;
        }

        if (product.is_register()) {
            imul(src_loc_tk, indent, product, factor);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent);
            }

            return;
        }

        if (use == factor_use::may_overwrite) {
            imul(src_loc_tk, indent, factor, product);

            // the move keeps the flags
            mov(src_loc_tk, indent, product, factor);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent);
            }

            return;
        }

        const operand reg{
            alloc_scratch_register(
                src_loc_tk, indent,
                builtin_type_for_size_bytes(product.type_ref().size_bytes())),
        };

        mov(src_loc_tk, indent, reg, product);
        imul(src_loc_tk, indent, reg, factor);

        if (check.enabled) {
            branch_on_overflow(src_loc_tk, indent);
        }

        mov(src_loc_tk, indent, product, reg);
        free_scratch_register(src_loc_tk, indent, reg);
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
            assert(
                count.immediate() >= 0 and
                std::cmp_less(count.immediate(), dst.type_ref().size_bits()));

            emit_op(src_loc_tk, indent, code, dst, count);

            return;
        }

        reserve_named_register(src_loc_tk, indent, "rcx", default_type());

        // note: a count wider than the value is rejected as narrowed
        assert(count.type_ref().size_bytes() <= dst.type_ref().size_bytes());

        mov(src_loc_tk, indent,
            sized_register("rcx", dst.type_ref().size_bytes()), count);

        if (check.enabled) {
            check_shift_count(src_loc_tk, indent, dst.type_ref());
        }

        emit_op(src_loc_tk, indent, code, dst,
                sized_register("rcx", size_byte_));

        release_named_register(src_loc_tk, indent, "rcx");
    }

    // an immediate store is one instruction without a register while a copy
    // needs a load and a store for each qword
    auto store_bytes(const token& src_loc_tk, const size_t indent,
                     const std::string_view bytes, const operand& dst,
                     [[maybe_unused]] const size_t alignment,
                     const std::function_ref<std::string()> add_constant)
        -> void override {

        if (bytes.size() > threshold_for_rep_movs_size_bytes_) {
            const std::string label{add_constant()};

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

        comment(src_loc_tk, indent, "size <= {} B, use immediates",
                threshold_for_rep_movs_size_bytes_);

        for_each_part(
            bytes.size(), size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const int64_t value{
                    little_endian_value(bytes.substr(offset, part_size_bytes)),
                };

                // a qword store takes only a sign-extended 32-bit immediate
                if (part_size_bytes == size_qword_ and
                    not std::in_range<int32_t>(value)) {

                    const size_t high_offset{offset + size_dword_};

                    const int64_t low{
                        little_endian_value(bytes.substr(offset, size_dword_)),
                    };

                    const int64_t high{
                        little_endian_value(
                            bytes.substr(high_offset, size_dword_)),
                    };

                    store_immediate_part(src_loc_tk, indent, dst, size_dword_,
                                         offset, low);

                    store_immediate_part(src_loc_tk, indent, dst, size_dword_,
                                         high_offset, high);

                    return;
                }

                store_immediate_part(src_loc_tk, indent, dst, part_size_bytes,
                                     offset, value);
            });
    }

    auto unary(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const runtime_check& check = {}) -> void override {

        if (operation == arithmetic_operator::complement) {
            not_op(src_loc_tk, indent, dst);
            return;
        }

        assert(operation == arithmetic_operator::negate);

        neg(src_loc_tk, indent, dst);

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

        if (size_bytes > threshold_for_rep_stos_size_bytes_) {
            reserve_named_register(src_loc_tk, indent, "rax", default_type());
            reserve_named_register(src_loc_tk, indent, "rdi", default_type());
            reserve_named_register(src_loc_tk, indent, "rcx", default_type());

            const operand al{sized_register("rax", size_byte_)};
            xor_op(src_loc_tk, indent, al, al);

            lea(src_loc_tk, indent, qword_register("rdi"), dst);

            mov(src_loc_tk, indent, qword_register("rcx"),
                immediate(size_bytes));

            assembler_.instruction(indent, op::rep_stosb);
            release_named_register(src_loc_tk, indent, "rcx");
            release_named_register(src_loc_tk, indent, "rdi");
            release_named_register(src_loc_tk, indent, "rax");

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_stos_size_bytes_);

        for_each_part(
            size_bytes, size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset), immediate(0));
            });
    }

    //
    // class methods
    //

    // 'syscall' changes 'rcx' and 'r11', the allocated ones are saved; public
    // for the backend tests
    auto invoke_syscall(const size_t indent) -> void {
        static constexpr std::array<std::string_view, 2> clobbered{
            "rcx",
            "r11",
        };

        with_saved_registers(indent, clobbered,
                             [&] -> void { syscall(indent); });
    }

    //
    // statics
    //

    // see 'machine::target_description'
    [[nodiscard]] static constexpr auto x86_64_target() -> target_description {
        return {
            .address_size_bytes{size_qword_},
            .data_alignment{data_alignment_},
            .max_storage_bytes{user_address_space_bytes_},
            .has_indexed_addressing{true},
            .max_padding_gaps_zeroed_one_by_one{
                max_padding_gaps_zeroed_one_by_one_,
            },
            .has_sd_card{true},
            .string_alignment{1},
            .is_stack_bounded{},
            .frame_base_register{"rbx"},
            .stack_pointer_register{"rsp"},
            .variables_base_register{"rbp"},
            .slot_register{"r11"},
            .variables_base_past_vars_bytes{std::nullopt},
            .scratch_registers{scratch_registers_},
            .target_calls{
                .arguments{"rdi", "rsi", "rdx"},
                .result{"rax"},
            },
        };
    }

  protected:
    //
    // overridden methods
    //

    [[nodiscard]] auto address_text(const operand& address) const
        -> std::string override {

        return assembler_x86_64::address_text(to_address(address));
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

    // the handler of a check: without the line it prints 'panic: NAME'; with
    // it, one stub per failure location sets rbp to the line and enters the
    // entry of its file, which prints 'panic: NAME at FILE:', sets rbx to the
    // exit code and enters the shared line report
    auto emit_failure_handler(const failure_handler& handler,
                              const bool with_line) -> void override {

        const failure_lines lines_by_file{failure_lines_of(handler.label)};

        const std::string_view name{handler.name};
        const std::string_view handler_label{handler.label};

        const std::string label_name{handler.symbol()};

        if (not with_line) {
            const std::string message_label{std::format("msg_{}", label_name)};

            assembler_.label(0, handler_label);
            emit_panic_message(message_label);
            emit_failure_exit(handler.exit_code);

            emit_message_data(message_label,
                              std::format("{}\n", panic_message(name)));

            return;
        }

        for (const auto& [file_ix, lines] : lines_by_file) {
            for (const size_t line : lines) {
                assembler_.label(0,
                                 line_failure_label(label_name, file_ix, line));

                assembler_.instruction(1, op::mov, "rbp", line);
                assembler_.jmp(1, failure_entry_label(handler_label, file_ix));
            }
        }

        for (const size_t file_ix : lines_by_file | std::views::keys) {
            const std::string message_label{
                std::format("msg_{}_file_{}", label_name, file_ix),
            };

            assembler_.label(0, failure_entry_label(handler_label, file_ix));
            emit_panic_message(message_label);
            assembler_.instruction(1, op::mov, "rbx", handler.exit_code);
            assembler_.jmp(1, report_line_label_);
            mark_used(report_line_label_);

            // note: a backslash or a control byte of a path is written as an
            //       escape of a nasm string

            emit_message_data(message_label, failure_message(name, file_ix));
        }
    }

    // 'read' and 'write' are system calls, the sector functions routines
    auto emit_target_call(const token& src_loc_tk, const size_t indent,
                          const target_call function,
                          const std::span<const operand> args,
                          const operand& result) -> void override {

        if (function == target_call::read) {
            io_syscall(src_loc_tk, indent, result, args.at(0), args.at(1),
                       args.at(2), syscall_read_);

            return;
        }

        if (function == target_call::write) {
            io_syscall(src_loc_tk, indent, result, args.at(0), args.at(1),
                       args.at(2), syscall_write_);

            return;
        }

        call_sector_routine(src_loc_tk, indent, args.at(0), args.at(1),
                            args.at(2), function);
    }

    // unused routines contribute no code
    auto emit_used_routines() -> void override {
        if (is_used(report_line_label_)) {
            emit_report_line_routine();
        }

        const bool is_read_used{is_used(sector_read_label_)};
        const bool is_write_used{is_used(sector_write_label_)};

        if (is_read_used or is_write_used) {
            emit_sector_routines(is_read_used, is_write_used);
        }
    }

    auto emit_variables_base_load() -> void override {
        assembler_.instruction(0, op::lea, variables_base_register(),
                               assembler_x86_64::memory::of_symbol(data_label));
    }

    // the index in 'register_names_' of a register of any size
    [[nodiscard]] auto register_index_of(const std::string_view name) const
        -> std::optional<size_t> override {

        if (register_size_bytes(name) == 0) {
            return std::nullopt;
        }

        const std::string qword{sized_register_name(name, size_qword_)};

        for (const auto [index, names] :
             std::views::enumerate(register_names_)) {

            if (names.qword == qword) {
                return static_cast<size_t>(index);
            }
        }

        std::unreachable();
    }

    auto restore_call_registers(const size_t indent,
                                const std::span<const size_t> saved)
        -> void override {

        for (const size_t index : saved | std::views::reverse) {
            pop(indent, qword_register(register_names_.at(index).qword));
        }
    }

    auto save_call_registers(const size_t indent,
                             const std::span<const size_t> saved)
        -> void override {

        for (const size_t index : saved) {
            push(indent, qword_register(register_names_.at(index).qword));
        }
    }

    [[nodiscard]] auto target_assembler() -> assembler& override {
        return assembler_;
    }

    [[nodiscard]] auto target_assembler() const -> const assembler& override {
        return assembler_;
    }

  private:
    //
    // overridden methods
    //

    [[nodiscard]] auto register_display_name(const size_t index) const
        -> std::string override {

        return std::string{register_names_.at(index).qword};
    }

    //
    // class methods
    //

    auto add(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& src) -> void {

        emit_op(src_loc_tk, indent, op::add, dst, src);
    }

    // adds 'index * scale' of 'value' to the register 'sum'
    auto add_scaled_index(const size_t indent, const std::string_view sum,
                          const operand& value) -> void {

        // scratch index registers and encodable scales only
        assert(value.index_register() != "rsp" and
               can_lower_index_scale(value.scale()));

        assembler_.instruction(
            indent, op::lea, sum,
            register_sum(sum, value.index_register(), value.scale()));
    }

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

    [[nodiscard]] auto begin_bulk(const token& src_loc_tk, const size_t indent)
        -> operand {

        reserve_named_register(src_loc_tk, indent, "rsi", default_type());
        reserve_named_register(src_loc_tk, indent, "rdi", default_type());
        return alloc_named_register(src_loc_tk, indent, "rcx", default_type());
    }

    auto branch_comparison(const size_t indent,
                           const comparison_operator comparison,
                           const bool inverted, const std::string_view label)
        -> void {

        assembler_.jcc(indent, condition_for_comparison(comparison, inverted),
                       label);
    }

    // jumps to the overflow failure handler when the instruction before set
    // the overflow flag
    auto branch_on_overflow(const token& src_loc_tk, const size_t indent)
        -> void {

        branch_to_failure(src_loc_tk, indent, condition::o, overflow_failure);
    }

    // a reported line goes through the stub of its line, which sets rbp for
    // the handler, so a passing check keeps rbp and loads nothing
    auto branch_to_failure(const token& src_loc_tk, const size_t indent,
                           const condition failed,
                           const failure_handler& handler) -> void {

        assembler_.jcc(indent, failed, failure_label(handler, src_loc_tk));
    }

    // the built-in integer type of 'size_bytes'
    [[nodiscard]] auto
    builtin_type_for_size_bytes(const size_t size_bytes) const -> const type& {

        switch (size_bytes) {
        case size_qword_:
            return builtin_type_i64();

        case size_dword_:
            return builtin_type_i32();

        case size_word_:
            return builtin_type_i16();

        case size_byte_:
            return builtin_type_i8();

        default:
            std::unreachable();
        }
    }

    // the routine changes 'rax', 'rcx', 'r10' and 'r11' besides the
    // arguments, the allocated ones are saved around the call
    auto call_sector_routine(const token& src_loc_tk, const size_t indent,
                             const operand& sector, const operand& address,
                             const operand& size, const target_call function)
        -> void {

        for (const operand* value : {&sector, &address, &size}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == size_qword_);
        }

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

        const operand low_byte{
            make_register_operand(left.register_name(), builtin_type_i8()),
        };

        mov(src_loc_tk, indent, right, low_byte);
        cmp(src_loc_tk, indent, right, left);

        branch_to_failure(src_loc_tk, indent, condition::ne, overflow_failure);
    }

    // the source in 'rsi', the destination in 'rdi' and the byte count in
    // 'rcx' of a copy: a distance from the source to the destination that is
    // not zero and below the count jumps to the failure handler, the copy
    // would overwrite source bytes it has not read yet
    auto check_copy_overlap(const token& src_loc_tk, const size_t indent)
        -> void {

        comment(src_loc_tk, indent, "overlap check begin");

        const std::string passed{
            std::format(".Lbaz_overlap.{}", overlap_label_count_++),
        };

        const operand distance{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        mov(src_loc_tk, indent, distance, qword_register("rdi"));
        emit_op(src_loc_tk, indent, op::sub, distance, qword_register("rsi"));

        comment(src_loc_tk, indent, "the same range is not an overlap");

        assembler_.jcc(indent, condition::e, passed);

        comment(src_loc_tk, indent, "destination starts inside the source");

        cmp(src_loc_tk, indent, distance, qword_register("rcx"));
        branch_to_failure(src_loc_tk, indent, condition::b, overlap_failure);
        assembler_.label(indent, passed);
        free_scratch_register(src_loc_tk, indent, distance);

        comment(src_loc_tk, indent, "overlap check end");
    }

    // a zero divisor and the minimum of the dividend width divided by -1 jump
    // to the failure handler, the dividend is sign extended in 'rax'
    auto check_divisor(const token& src_loc_tk, const size_t indent,
                       const operand& divisor, const size_t dividend_size_bytes)
        -> void {

        const int64_t minimum{
            constant_arith::signed_minimum(dividend_size_bytes * bits_per_byte),
        };

        const std::string not_minimum{
            std::format(".Lbaz_division.{}", division_label_count_++),
        };

        comment(src_loc_tk, indent, "division check begin");

        comment(src_loc_tk, indent, "zero divisor");

        cmp(src_loc_tk, indent, divisor, immediate(0));
        branch_to_failure(src_loc_tk, indent, condition::e, division_failure);

        comment(src_loc_tk, indent, "minimum divided by -1 overflows");

        cmp(src_loc_tk, indent, divisor, immediate(-1));
        assembler_.jcc(indent, condition::ne, not_minimum);
        mov(src_loc_tk, indent, qword_register("rdx"), immediate(minimum));
        cmp(src_loc_tk, indent, qword_register("rax"), qword_register("rdx"));
        branch_to_failure(src_loc_tk, indent, condition::e, division_failure);
        assembler_.label(indent, not_minimum);

        comment(src_loc_tk, indent, "division check end");
    }

    auto check_lower_bound(const token& src_loc_tk, const size_t indent,
                           const bounds_range& range, const bounds_plan& plan)
        -> void {

        comment_lower_bound(src_loc_tk, indent, range, plan);

        if (not plan.upper_covers_lower) {
            test_sign(src_loc_tk, indent, range.checked);

            // a negative count passes 'start + count' but spans the address
            // space
            if (range.count and not plan.count_known) {
                test_sign(src_loc_tk, indent, *range.count);
            }
        }

        mark_lower_checked(range);
    }

    // a count in 'rcx' below zero or not below the width jumps to the failure
    // handler, a negative count is a large unsigned one
    auto check_shift_count(const token& src_loc_tk, const size_t indent,
                           const type& dst_type) -> void {

        comment(src_loc_tk, indent, "shift check begin");

        cmp(src_loc_tk, indent, sized_register("rcx", dst_type.size_bytes()),
            immediate(static_cast<int64_t>(dst_type.size_bits())));

        branch_to_failure(src_loc_tk, indent, condition::ae, shift_failure);

        comment(src_loc_tk, indent, "shift check end");
    }

    // branches to the failure handler when the range passes the array count, a
    // range 'index + count' may end at it; a signed end that passes the signed
    // maximum is out of bounds, the unsigned compare sees it large
    auto check_upper_bound(const token& src_loc_tk, const size_t indent,
                           const bounds_range& range,
                           const bool is_unsigned_compare) -> void {

        const condition out_of_bounds{
            out_of_bounds_condition(range.allow_end, is_unsigned_compare),
        };

        if (not range.count) {
            cmp(src_loc_tk, indent, range.checked,
                immediate(range.array_count));

            branch_to_failure(src_loc_tk, indent, out_of_bounds,
                              bounds_failure);

            return;
        }

        const operand& count{*range.count};

        const operand reg_top_idx{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        // index and count registers have the default type
        assert(count.type_ref().size_bytes() == size_qword_ and
               range.checked.type_ref().size_bytes() == size_qword_);

        if (is_unsigned_compare) {
            lea(src_loc_tk, indent, reg_top_idx,
                operand::mem(count.register_name(),
                             range.checked.register_name(), 1, 0,
                             builtin_type_i64()));
        } else {
            mov(src_loc_tk, indent, reg_top_idx, range.checked);
            add(src_loc_tk, indent, reg_top_idx, count);
            branch_to_failure(src_loc_tk, indent, condition::o, bounds_failure);
        }

        cmp(src_loc_tk, indent, reg_top_idx, immediate(range.array_count));

        free_scratch_register(src_loc_tk, indent, reg_top_idx);

        branch_to_failure(src_loc_tk, indent, out_of_bounds, bounds_failure);
    }

    auto cmp(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        emit_op(src_loc_tk, indent, op::cmp, dst_op, src_op);
    }

    // 'load_source_address' sets the 'rep movsb' source pointer
    auto copy_with_rep_movsb(
        const token& src_loc_tk, const size_t indent, const operand& dst,
        const size_t size_bytes,
        const std::function_ref<void(const operand&)> load_source_address)
        -> void {

        std::ignore = begin_bulk(src_loc_tk, indent);

        load_source_address(qword_register("rsi"));
        lea(src_loc_tk, indent, qword_register("rdi"), dst);
        mov(src_loc_tk, indent, qword_register("rcx"), immediate(size_bytes));

        assembler_.instruction(indent, op::rep_movsb);

        release_bulk(src_loc_tk, indent);
    }

    // writes the instruction with the operands as they are
    auto emit(const size_t indent, const op code, const operand& dst,
              const operand& src) -> void {

        assembler_.instruction(indent, code, to_argument(dst),
                               to_argument(src));
    }

    auto emit_failure_exit(const int exit_code) -> void {
        assembler_.comment(1,
                           std::format("exit with error code {}", exit_code));

        assembler_.instruction(1, op::mov, "rax", syscall_exit_);
        assembler_.instruction(1, op::mov, "rdi", exit_code);
        assembler_.instruction(1, op::syscall);
    }

    // an immediate that does not fit the instruction goes through a register
    auto emit_immediate_op(const token& src_loc_tk, const size_t indent,
                           const op code, const operand& dst_op,
                           const operand& src_op) -> void {

        if (not needs_immediate_register(code, dst_op, src_op)) {
            emit(indent, code, dst_op, src_op);
            return;
        }

        const operand reg{
            alloc_scratch_register(src_loc_tk, indent, builtin_type_i64()),
        };

        emit(indent, op::mov, reg, src_op);
        emit(indent, code, dst_op, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    // x86 has no memory to memory form, the source goes through a register of
    // the destination width
    auto emit_memory_to_memory_op(const token& src_loc_tk, const size_t indent,
                                  const op code, const operand& dst_op,
                                  const operand& src_op) -> void {

        const size_t dst_size_bytes{dst_op.type_ref().size_bytes()};
        const size_t src_size_bytes{src_op.type_ref().size_bytes()};

        const operand reg{
            alloc_scratch_register(src_loc_tk, indent,
                                   builtin_type_for_size_bytes(dst_size_bytes)),
        };

        if (dst_size_bytes > src_size_bytes) {
            emit(indent, op::movsx, reg, src_op);
        } else if (dst_size_bytes < src_size_bytes) {
            emit(indent, op::mov, reg, sized_memory(src_op, dst_size_bytes));
        } else {
            emit(indent, op::mov, reg, src_op);
        }

        emit(indent, code, dst_op, reg);

        free_scratch_register(src_loc_tk, indent, reg);
    }

    // the text of a message in read-only data with its length
    auto emit_message_data(const std::string_view message_label,
                           const std::string_view bytes) -> void {

        assembler_.switch_section(section::rodata);
        assembler_.label(0, message_label);
        assembler_.string_data(bytes);

        assembler_.define_length(std::format("{}_len", message_label),
                                 message_label);

        assembler_.switch_section(section::text);
    }

    // the low part of the source is used, so a narrower destination needs no
    // extension
    auto emit_narrowing_op(const size_t indent, const op code,
                           const operand& dst_op, const operand& src_op)
        -> void {

        const size_t dst_size_bytes{dst_op.type_ref().size_bytes()};

        if (src_op.is_register()) {
            emit(indent, code, dst_op, sized_register(src_op, dst_size_bytes));
            return;
        }

        assert(dst_op.is_register() and src_op.is_memory());

        emit(indent, code, dst_op, sized_memory(src_op, dst_size_bytes));
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

            with_lowered_addresses(
                src_loc_tk, indent, dst_op, src_op,
                [&](const operand& dst, const operand& src) -> void {
                    emit_op(src_loc_tk, indent, code, dst, src);
                });

            return;
        }

        if (src_op.is_immediate()) {
            emit_immediate_op(src_loc_tk, indent, code, dst_op, src_op);
            return;
        }

        if (dst_op.is_memory() and src_op.is_memory()) {
            emit_memory_to_memory_op(src_loc_tk, indent, code, dst_op, src_op);
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

        emit_narrowing_op(indent, code, dst_op, src_op);
    }

    // shared by the failure handlers, which run after all allocations
    auto emit_panic_message(const std::string_view message_label) -> void {
        assembler_.comment(1, "print message to stderr");
        assembler_.instruction(1, op::mov, "rax", syscall_write_);
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor);

        assembler_.instruction(
            1, op::lea, "rsi",
            assembler_x86_64::memory::of_symbol(message_label));

        assembler_.instruction(1, op::mov, "rdx",
                               assembler_x86_64::immediate::of_expression(
                                   std::format("{}_len", message_label)));

        assembler_.instruction(1, op::syscall);
    }

    // prints the line number in 'rbp' and exits with the code in 'rbx'
    auto emit_report_line_routine() -> void {
        assembler_.label(0, report_line_label_);

        constexpr int newline{'\n'};
        constexpr int decimal_base{10};
        // the 20 digits of a 64-bit number and a newline, the digits are
        // written backwards before the newline at the end of the buffer
        constexpr size_t number_buffer_size_bytes{21};
        constexpr size_t newline_offset{number_buffer_size_bytes - 1};

        assembler_.comment(1, "line number is in 'rbp'");
        assembler_.instruction(1, op::mov, "rax", "rbp");
        assembler_.comment(1, "convert to string");

        assembler_.instruction(
            1, op::mov, "rdi",
            assembler_x86_64::immediate::of_qword_expression(
                std::format("num_buffer + {}", newline_offset)));

        assembler_.instruction(
            1, op::mov, assembler_x86_64::memory::of_base("rdi", 1), newline);

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
        assembler_.jcc(1, condition::nz, ".convert_loop");
        assembler_.instruction(1, op::inc, "rdi");
        assembler_.comment(1, "print line number to stderr");
        assembler_.instruction(1, op::mov, "rax", syscall_write_);
        assembler_.instruction(1, op::mov, "rsi", "rdi");

        // the length is the end of the buffer minus the first digit
        assembler_.instruction(
            1, op::mov, "rdx",
            assembler_x86_64::immediate::of_qword_expression(
                std::format("num_buffer + {}", number_buffer_size_bytes)));

        assembler_.instruction(1, op::sub, "rdx", "rdi");
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor);
        assembler_.instruction(1, op::syscall);
        assembler_.comment(1, "exit with the error code in 'rbx'");
        assembler_.instruction(1, op::mov, "rax", syscall_exit_);
        assembler_.instruction(1, op::mov, "rdi", "rbx");
        assembler_.instruction(1, op::syscall);
        assembler_.switch_section(section::bss);
        assembler_.label(0, "num_buffer");
        assembler_.reserve(number_buffer_size_bytes);
        assembler_.switch_section(section::text);
    }

    // the routines take the sector in 'rdi', the address in 'rsi' and the size
    // in 'rdx'; the image file is opened at the first transfer, a sector past
    // its end or outside 32 bits, a failed open or a short transfer panics
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void {

        // 'O_RDWR' and 'SEEK_END' of linux
        constexpr int read_write{2};
        constexpr int seek_end{2};
        constexpr int sector_shift{std::countr_zero(card_sector_size_bytes)};

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
        assembler_.jcc(1, condition::s, sd_card_failure.label);
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
        assembler_.jcc(1, condition::s, sd_card_failure.label);
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
        assembler_.jcc(1, condition::ne, sd_card_failure.label);
        // a sector at or past the end of the file panics
        assembler_.instruction(1, op::cmp, "rdi", sector_count);
        assembler_.jcc(1, condition::ae, sd_card_failure.label);
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
        assembler_.jcc(1, condition::ne, sd_card_failure.label);
        assembler_.instruction(1, op::ret);

        // the stack is not unwound because the program exits
        emit_failure_handler(sd_card_failure, false);

        assembler_.switch_section(section::rodata);
        assembler_.comment(0, "the file name ends with a zero byte");
        assembler_.label(0, "baz_sd_card_name");

        assembler_.string_data(sd_card_file_name());

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

    // idiv takes the divisor as a qword register or memory operand, 'rax'
    // holds the dividend, 'rdx' is free until it is extended
    auto emit_signed_divide(const token& src_loc_tk, const size_t indent,
                            const operand& divisor, const runtime_check& check,
                            const size_t dividend_size_bytes) -> void {

        if (not divisor.is_immediate() and
            divisor.type_ref().size_bytes() == size_qword_) {

            if (check.enabled) {
                check_divisor(src_loc_tk, indent, divisor, dividend_size_bytes);
            }

            assembler_.instruction(indent, op::cqo);
            idiv(src_loc_tk, indent, divisor);

            return;
        }

        const operand scratch_reg{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        mov(src_loc_tk, indent, scratch_reg, divisor);

        if (check.enabled) {
            check_divisor(src_loc_tk, indent, scratch_reg, dividend_size_bytes);
        }

        assembler_.instruction(indent, op::cqo);
        idiv(src_loc_tk, indent, scratch_reg);
        free_scratch_register(src_loc_tk, indent, scratch_reg);
    }

    auto emit_unary(const token& src_loc_tk, const size_t indent, const op code,
                    const operand& value) -> void {

        with_lowered_addresses(
            src_loc_tk, indent, value, operand{},
            [&](const operand& lowered,
                [[maybe_unused]] const operand& empty) -> void {
                assembler_.instruction(indent, code, to_argument(lowered));
            });
    }

    // the source is sign extended to the destination width
    auto emit_widening_op(const token& src_loc_tk, const size_t indent,
                          const op code, const operand& dst_op,
                          const operand& src_op) -> void {

        const size_t dst_size_bytes{dst_op.type_ref().size_bytes()};

        // 'movsx' needs a register destination, so the source register is
        // extended in place, its low bits keep the value
        if (code == op::mov and dst_op.is_memory()) {
            const operand wide{sized_register(src_op, dst_size_bytes)};

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

        // the scratch register must match 'dst' so the op has equal-size
        // operands
        const operand reg_sx{
            alloc_scratch_register(src_loc_tk, indent,
                                   builtin_type_for_size_bytes(dst_size_bytes)),
        };

        emit(indent, op::movsx, reg_sx, src_op);

        emit(indent, code, dst_op, reg_sx);

        free_scratch_register(src_loc_tk, indent, reg_sx);
    }

    // the handler to jump to, or with the line the stub of the line
    [[nodiscard]] auto failure_label(const failure_handler& handler,
                                     const token& src_loc_tk) -> std::string {

        if (not mark_failure(handler, src_loc_tk)) {
            return std::string{handler.label};
        }

        return line_failure_label(handler.symbol(), src_loc_tk.file_index(),
                                  src_loc_tk.at_line());
    }

    auto idiv(const token& src_loc_tk, const size_t indent,
              const operand& value) -> void {

        emit_unary(src_loc_tk, indent, op::idiv, value);
    }

    template <std::integral value_t>
    [[nodiscard]] auto immediate(const value_t value) const -> operand {
        return operand::imm(value, default_type());
    }

    auto imul(const token& src_loc_tk, const size_t indent,
              const operand& dst_op, const operand& src_op) -> void {

        emit_op(src_loc_tk, indent, op::imul, dst_op, src_op);
    }

    auto inc(const token& src_loc_tk, const size_t indent, const operand& dst)
        -> void {

        emit_unary(src_loc_tk, indent, op::inc, dst);
    }

    auto io_syscall(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& descriptor,
                    const operand& address, const operand& count,
                    const int syscall_number) -> void {

        for (const operand* value : {&dst, &descriptor, &address, &count}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == size_qword_);
        }

        mov(src_loc_tk, indent, dst, immediate(syscall_number));
        invoke_syscall(indent);
    }

    auto lea(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& address) -> void {

        with_lowered_addresses(src_loc_tk, indent, dst, address,
                               [&](const operand& lowered_dst,
                                   const operand& lowered_address) -> void {
                                   assembler_.instruction(
                                       indent, op::lea,
                                       to_argument(lowered_dst),
                                       to_address(lowered_address));
                               });
    }

    // 'lea' writes only registers so a memory destination takes the address
    // through a scratch register
    auto lea_into(const token& src_loc_tk, const size_t indent,
                  const operand& dst, const operand& address) -> void {

        if (dst.is_register()) {
            lea(src_loc_tk, indent, dst, address);
            return;
        }

        const operand reg{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        lea(src_loc_tk, indent, reg, address);
        mov(src_loc_tk, indent, dst, reg);

        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto load_bulk_addresses(const token& src_loc_tk, const size_t indent,
                             const std::optional<operand>& count,
                             const address_emitter& to_rsi,
                             const address_emitter& to_rdi) -> void {

        to_rsi(count, std::nullopt, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rsi"), address);
        });

        to_rdi(count, std::nullopt, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rdi"), address);
        });
    }

    // the bulk registers of 'rep movsb' and 'repe cmpsb': 'rcx' the count in
    // bytes, 'rsi' the address of 'to_rsi' and 'rdi' that of 'to_rdi'
    auto load_bulk_registers(
        const token& src_loc_tk, const size_t indent,
        const size_t element_size_bytes,
        const std::function_ref<void(const operand&)> emit_count,
        const address_emitter& to_rsi, const address_emitter& to_rdi) -> void {

        const operand count{begin_bulk(src_loc_tk, indent)};

        emit_count(count);
        load_bulk_addresses(src_loc_tk, indent, count, to_rsi, to_rdi);

        scale_index(src_loc_tk, indent, qword_register("rcx"),
                    element_size_bytes);
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
            alloc_scratch_register(src_loc_tk, indent, builtin_type_i64()),
        };

        const std::string_view sum{address.register_name()};
        assembler_.instruction(indent, op::mov, sum, value.displacement());

        if (not value.base_register().empty()) {
            assembler_.instruction(indent, op::lea, sum,
                                   register_sum(value.base_register(), sum, 1));
        }

        if (not value.index_register().empty()) {
            add_scaled_index(indent, sum, value);
        }

        return operand::mem(address.register_name(), {}, 1, 0,
                            value.type_ref());
    }

    // an unrolled part of 'address'
    [[nodiscard]] auto memory_part(const operand& address,
                                   const size_t part_size_bytes,
                                   const size_t offset) const -> operand {

        operand part{sized_memory(address, part_size_bytes)};
        part.increment_offset(address_offset(offset));
        return part;
    }

    auto mov(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        emit_op(src_loc_tk, indent, op::mov, dst_op, src_op);
    }

    // a constant factor is resolved at compile time: zero clears, one needs no
    // code, minus one negates and a power of two is a shift, false when a
    // multiplication is still needed
    [[nodiscard]] auto
    multiply_by_constant(const token& src_loc_tk, const size_t indent,
                         const operand& product, const operand& factor,
                         const runtime_check& check) -> bool {

        const std::optional<uint64_t> bits{immediate_bits(factor)};

        if (not bits) {
            return false;
        }

        const constant_factor factor_info{
            classify_factor(*bits, product.type_ref().size_bits()),
        };

        if (factor_info.kind == factor_kind::zero) {
            // 'xor' is the shorter idiom, a memory product has no
            // 'xor m, m' form
            if (product.is_register()) {
                xor_op(src_loc_tk, indent, product, product);
            } else {
                mov(src_loc_tk, indent, product, immediate(0));
            }

            return true;
        }

        if (factor_info.kind == factor_kind::one) {
            return true;
        }

        if (factor_info.kind == factor_kind::minus_one) {
            neg(src_loc_tk, indent, product);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent);
            }

            return true;
        }

        // a shift does not report the bits it loses, 'imul' does
        if (factor_info.kind == factor_kind::other or check.enabled) {
            return false;
        }

        emit_op(src_loc_tk, indent, op::sal, product,
                immediate(factor_info.shift));

        return true;
    }

    auto neg(const token& src_loc_tk, const size_t indent, const operand& value)
        -> void {

        emit_unary(src_loc_tk, indent, op::neg, value);
    }

    auto not_op(const token& src_loc_tk, const size_t indent,
                const operand& value) -> void {

        emit_unary(src_loc_tk, indent, op::not_op, value);
    }

    auto pop(const size_t indent, const operand& dst) -> void {
        assert(dst.is_register() and
               dst.type_ref().size_bytes() == size_qword_);

        assembler_.instruction(indent, op::pop, to_argument(dst));
    }

    auto push(const size_t indent, const operand& src) -> void {
        assert(src.is_register() and
               src.type_ref().size_bytes() == size_qword_);

        assembler_.instruction(indent, op::push, to_argument(src));
    }

    // e.g. the fixed registers of 'rep movsb' and syscalls
    [[nodiscard]] auto qword_register(const std::string_view name) const
        -> operand {

        return make_register_operand(name, builtin_type_i64());
    }

    auto release_bulk(const token& src_loc_tk, const size_t indent) -> void {
        release_named_register(src_loc_tk, indent, "rcx");
        release_named_register(src_loc_tk, indent, "rdi");
        release_named_register(src_loc_tk, indent, "rsi");
    }

    auto setcc(const token& src_loc_tk, const size_t indent, const condition cc,
               const operand& value) -> void {

        with_lowered_addresses(
            src_loc_tk, indent, value, operand{},
            [&](const operand& lowered,
                [[maybe_unused]] const operand& empty) -> void {
                assembler_.setcc(indent, cc, to_argument(lowered));
            });
    }

    [[nodiscard]] auto sized_memory(const operand& value,
                                    const size_t size_bytes) const -> operand {

        assert(value.is_memory());

        if (not value.type_ref().is_builtin() or
            value.type_ref().size_bytes() != size_bytes) {

            return operand::mem(value, builtin_type_for_size_bytes(size_bytes));
        }

        return value;
    }

    [[nodiscard]] auto sized_register(const std::string_view name,
                                      const size_t size_bytes) const
        -> operand {

        return make_register_operand(name,
                                     builtin_type_for_size_bytes(size_bytes));
    }

    [[nodiscard]] auto sized_register(const operand& reg,
                                      const size_t size_bytes) const
        -> operand {

        assert(reg.is_register());

        if (reg.type_ref().size_bytes() == size_bytes) {
            return reg;
        }

        operand result{sized_register(reg.register_name(), size_bytes)};
        result.set_allocation_register(reg.allocation_register());

        return result;
    }

    // 'setcc' writes one byte
    auto store_condition(const token& src_loc_tk, const size_t indent,
                         const condition cc, const operand& dst) -> void {

        if (dst.is_register()) {
            setcc(src_loc_tk, indent, cc, sized_register(dst, size_byte_));
            return;
        }

        setcc(src_loc_tk, indent, cc, sized_memory(dst, size_byte_));
    }

    auto store_immediate_part(const token& src_loc_tk, const size_t indent,
                              const operand& dst, const size_t part_size_bytes,
                              const size_t offset, const int64_t value)
        -> void {

        mov(src_loc_tk, indent, memory_part(dst, part_size_bytes, offset),
            immediate(value));
    }

    auto syscall(const size_t indent) -> void {
        assembler_.instruction(indent, op::syscall);
    }

    auto test(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& src) -> void {

        emit_op(src_loc_tk, indent, op::test, dst, src);
    }

    // a negative value is out of bounds, an empty operand has none to test
    auto test_sign(const token& src_loc_tk, const size_t indent,
                   const operand& value) -> void {

        test(src_loc_tk, indent, value, value);
        branch_to_failure(src_loc_tk, indent, condition::s, bounds_failure);
    }

    auto with_lowered_addresses(
        const token& src_loc_tk, const size_t indent, const operand& dst,
        const operand& src,
        const std::function_ref<void(const operand&, const operand&)> emit)
        -> void {

        if (address_is_encodable(dst) and address_is_encodable(src)) {
            emit(dst, src);
            return;
        }

        // registers the operands refer to must not be picked for lowering
        const operation_scope scope{*this, dst, src};

        const operand lowered_dst{lower_address(src_loc_tk, indent, dst)};
        const operand lowered_src{lower_address(src_loc_tk, indent, src)};

        emit(lowered_dst, lowered_src);
    }

    auto xor_op(const token& src_loc_tk, const size_t indent,
                const operand& dst, const operand& src) -> void {

        emit_op(src_loc_tk, indent, op::xor_op, dst, src);
    }

    //
    // statics
    //

    [[nodiscard]] static auto
    condition_for_comparison(const comparison_operator comparison,
                             const bool inverted) -> condition {

        const comparison_operator holds{
            inverted ? negated(comparison) : comparison,
        };

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

    // a location in the main file has no file index, so the label stays the
    // same for a program of one file
    [[nodiscard]] static auto line_failure_label(const std::string_view name,
                                                 const size_t file_ix,
                                                 const size_t line)
        -> std::string {

        if (file_ix == 0) {
            return std::format("baz_{}_line_{}", name, line);
        }

        return std::format("baz_{}_f{}_line_{}", name, file_ix, line);
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
                                                      const bool is_unsigned)
        -> condition {

        if (is_unsigned) {
            return allow_end ? condition::a : condition::ae;
        }

        return allow_end ? condition::g : condition::ge;
    }

    // returns 0 if name is not a register
    [[nodiscard]] static auto register_size_bytes(const std::string_view name)
        -> size_t {

        for (const register_names& names : register_names_) {
            if (name == names.qword) {
                return size_qword_;
            }

            if (name == names.dword) {
                return size_dword_;
            }

            if (name == names.word) {
                return size_word_;
            }

            if (name == names.byte) {
                return size_byte_;
            }
        }

        return 0;
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

        assert(not lhs.is_immediate());

        if (not lhs.is_memory() or not rhs.is_memory()) {
            return lhs.is_empty() and rhs.is_empty();
        }

        // note: an index without a base has a scale above 1, with a scale of 1
        //       it would be the base, e.g. '[r15 * 1]' is the address '[r15]'

        assert(not lhs.base_register().empty() or lhs.scale() > 1);
        assert(not rhs.base_register().empty() or rhs.scale() > 1);

        return lhs.type_ref().size_bytes() == rhs.type_ref().size_bytes() and
               lhs.base_register() == rhs.base_register() and
               lhs.index_register() == rhs.index_register() and
               lhs.displacement() == rhs.displacement() and
               lhs.scale() == rhs.scale();
    }

    // the most that a value of the width of 'value' holds as a signed number
    [[nodiscard]] static auto signed_maximum(const operand& value) -> uint64_t {
        return (uint64_t{1} << (value.type_ref().size_bits() - 1)) - 1;
    }

    [[nodiscard]] static auto sized_register_name(const std::string_view name,
                                                  const size_t size_bytes)
        -> std::string {

        // a register is allocated by its 64-bit name, an operand names the
        // part of its width
        for (const register_names& names : register_names_) {
            if (name != names.qword and name != names.dword and
                name != names.word and name != names.byte) {

                continue;
            }

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

        // every register name is in 'register_names_'
        std::unreachable();
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

    // the address without a width, as 'lea' and comments take it
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

        if (value.is_symbol()) {
            return assembler_x86_64::immediate::of_expression(value.symbol());
        }

        if (value.is_register()) {
            return std::string_view{value.register_name()};
        }

        assembler_x86_64::memory address{to_address(value)};
        address.size_bytes = value.type_ref().size_bytes();

        return address;
    }
};
