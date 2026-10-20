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
#include "register_report.hpp"
#include "token.hpp"
#include "type.hpp"

// the x86_64 linux backend, written as nasm text through 'assembler_x86_64';
// the formatter sorts the functions by name, so by topic they are:
//
//   registers    'machine' allocates them, here the names
//                'register_names_', the order 'scratch_registers_', the frame
//                base 'rbx' and the variables base 'rbp'
//   operations   'emit_op' and 'emit_binary' choose the form of an
//                instruction, 'with_lowered_addresses' fits an operand into
//                one, 'multiply_by_constant', 'emit_signed_divide', 'shift'
//   memory       'copy', 'copy_bytes', 'zero' unrolled or with 'rep', the
//                bulk operations with 'rsi', 'rdi' and 'rcx'
//   comparisons  'compare_and_branch', 'store_comparison', 'memory_equal'
//   checks       'check_bounds', 'check_divisor', 'check_shift_count',
//                'check_copy_overlap', 'check_frame_capacity', each jumps to
//                its handler, with a stub per line for '--checks=line'
//   handlers     'emit_*_failure_handler', 'emit_line_panic_handler'
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

    static constexpr std::string_view variables_base_register_{"rbp"};

    static constexpr std::array<size_t, 4> index_register_scalings_{1, 2, 4, 8};

    static constexpr size_t threshold_for_rep_stos_size_bytes_{32};
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
    //       and 'rcx' last since every copy and compare of memory needs all
    //       three
    //       'r11' and 'rcx' are saved around syscalls if they are allocated
    //       because 'syscall' clobbers them

    static constexpr std::string_view slot_register_{"r11"};

    // scratch values live from which an array loop counts in memory
    static constexpr size_t memory_counter_pressure_count_{6};

    // numbers the labels after the parts of a memory compare
    size_t equal_label_count_{};
    // numbers the labels of the checks of divisions
    size_t division_label_count_{};
    // numbers the labels of the checks of copy overlaps
    size_t overlap_label_count_{};
    // the shared tail of the handlers that report a line was emitted
    bool line_report_emitted_{};
    // the lines that have a stub of the division failure handler
    std::set<size_t> division_panic_lines_;
    // the lines that have a stub of the overflow failure handler
    std::set<size_t> overflow_panic_lines_;
    // the lines that have a stub of the overlap failure handler
    std::set<size_t> overlap_panic_lines_;
    // the lines that have a stub of the shift failure handler
    std::set<size_t> shift_panic_lines_;
    // source lines of the bounds checks, each gets a stub that reports it
    std::set<size_t> bounds_panic_lines_;

    assembler_x86_64 assembler_;

  public:
    explicit machine_x86_64(std::ostream* const direct_output,
                            const source_files* const files,
                            const jump_mode jumps = jump_mode::resolved)
        : machine{direct_output, files, jumps} {

        // output before 'start' is written as emitted
        assembler_.set_direct_output(direct_stream());
    }

    using machine::emit_data_array;

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
            branch_on_overflow(src_loc_tk, indent, check);
        }
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        lea_into(src_loc_tk, indent, dst, address);
    }

    [[nodiscard]] auto address_size_bytes() const -> size_t override {
        return size_qword_;
    }

    auto arrays_equal(const token& src_loc_tk, const size_t indent,
                      const size_t element_size_bytes,
                      const std::function_ref<void(const operand&)> emit_count,
                      const equality_request& request) -> void override {

        const operand count{alloc_bulk_registers(src_loc_tk, indent)};

        emit_count(count);

        request.lhs(count, {}, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rsi"), address);
        });

        request.rhs(count, {}, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rdi"), address);
        });

        scale_by_element_size_bytes(src_loc_tk, indent, qword_register("rcx"),
                                    element_size_bytes);

        test(src_loc_tk, indent, qword_register("rcx"), qword_register("rcx"));
        assembler_.instruction(indent, op::repe_cmpsb);
        release_bulk_registers(src_loc_tk, indent);
        store_equal_result(src_loc_tk, indent, request.dst, request.inverted);
    }

    auto begin_data(const size_t alignment) -> void override {
        emit_used_sector_routines();

        assembler_.add_separator_newline();
        assembler_.switch_section(section::data);
        assembler_.align(alignment);
        assembler_.label(0, data_label);
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

    auto call_function(const token& src_loc_tk, const size_t indent,
                       const std::string_view label,
                       const operand& frame_address,
                       const operand& slot_address) -> void override {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(frame_address.base_register() != "rsp");

        // the callee may use every register but the variables base, so only
        // values live at the call need saving
        std::vector<operand> saved;
        for (const size_t index : registers_saved_by_call()) {
            saved.push_back(qword_register(register_names_.at(index).qword));
        }

        record_noinline_call(src_loc_tk, label, saved.size());

        if (not saved.empty()) {
            comment(src_loc_tk, indent,
                    "before call: save allocated registers");
        }

        for (const operand& reg : saved) {
            push(indent, reg);
        }

        load_slot_address(src_loc_tk, indent, slot_address);

        comment(src_loc_tk, indent, "set function frame base");

        lea(src_loc_tk, indent,
            make_register_operand(frame_base_register(), default_type()),
            frame_address, true);

        assembler_.call(indent, label);

        if (not saved.empty()) {
            comment(src_loc_tk, indent, "after call: restore saved registers");
        }

        for (const operand& reg : saved | std::views::reverse) {
            pop(indent, reg);
        }
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
            check_lower_bound(src_loc_tk, indent, range, plan,
                              options.with_line);
        }

        if (options.upper) {
            comment(src_loc_tk, indent, "upper bound");

            compare_upper_bound(src_loc_tk, indent, range);

            // the end of a range is 'start + count', neither is negative
            // here but the sum can pass the signed maximum and then looks
            // negative to a signed compare, the unsigned compare sees it large
            // e.g. 'array_copy(x[0x7fffffffffffffff], y[0], 1)' on 'i8[4]'
            // ends at 0x8000000000000000, signed it is below 4 and passes
            const bool is_unsigned_compare{
                plan.upper_covers_lower or
                    (options.lower and not range.count.is_empty()),
            };

            branch_to_bounds_panic(
                src_loc_tk, indent,
                out_of_bounds_condition(range.allow_end, is_unsigned_compare),
                options.with_line);
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

        lea(src_loc_tk, indent, start, frame_address, true);

        assembler_.instruction(
            indent, op::lea, to_argument(remaining),
            assembler_x86_64::memory::of_symbol(variables_label));

        cmp_lowered(src_loc_tk, indent, start, remaining);
        assembler_.jcc(indent, condition::b, frame_overflow_handler_label);

        // an absolute address reaches beyond the 2 GiB of 'rip' relative ones
        assembler_.instruction(indent, op::mov, to_argument(remaining),
                               assembler_x86_64::immediate::of_expression(
                                   variables_end_label, true));

        cmp_lowered(src_loc_tk, indent, start, remaining);
        assembler_.jcc(indent, condition::a, frame_overflow_handler_label);
        emit_op(src_loc_tk, indent, op::sub, remaining, start);
        mov(src_loc_tk, indent, start, frame_size_bytes);
        cmp_lowered(src_loc_tk, indent, start, remaining);
        assembler_.jcc(indent, condition::a, frame_overflow_handler_label);
        free_scratch_register(src_loc_tk, indent, remaining);
        free_scratch_register(src_loc_tk, indent, start);
        comment(src_loc_tk, indent, "frame capacity check end");
    }

    // the operating system stops a program that overflows the stack
    auto check_stack_capacity([[maybe_unused]] const token& src_loc_tk,
                              [[maybe_unused]] const size_t indent)
        -> void override {}

    auto comment_alias(const token& src_loc_tk, const size_t indent,
                       const std::string_view from, const std::string_view to,
                       const operand& address) -> void override {

        if (address.is_empty()) {
            comment(src_loc_tk, indent, "alias {} -> {}", from, to);
            return;
        }

        comment(src_loc_tk, indent, "alias {} -> {} (lea: {})", from, to,
                assembler_x86_64::address_text(to_address(address)));
    }

    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t size_bytes,
                          const operand& address) -> void override {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, size_bytes,
                assembler_x86_64::address_text(to_address(address)));
    }

    auto
    compare_and_branch(const token& src_loc_tk, const size_t indent,
                       const operand& lhs, const operand& rhs,
                       const comparison_action& action,
                       const std::span<const operand> scratch_registers_to_free)
        -> void override {

        cmp(src_loc_tk, indent, lhs, rhs);

        free_scratch_registers(src_loc_tk, indent, scratch_registers_to_free);

        if (not action.destination.is_empty()) {
            store_comparison(src_loc_tk, indent, action.operation,
                             action.inverted, action.destination);
        }

        if (not action.target.empty()) {
            branch_comparison(indent, action.operation,
                              action.branch_on_true ? action.inverted
                                                    : not action.inverted,
                              action.target);
        }
    }

    // x86 accesses unaligned memory so the alignment is not needed
    auto copy(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& src, const size_t size_bytes,
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

        reserve_named_register(src_loc_tk, indent, "rax", default_type());

        for_each_part(
            size_bytes, size_qword_,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const operand reg{sized_register("rax", part_size_bytes)};

                mov(src_loc_tk, indent, reg,
                    memory_part(src, part_size_bytes, offset));

                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset), reg);
            });

        release_named_register(src_loc_tk, indent, "rax");
    }

    // an immediate store is one instruction without a register while a copy
    // needs a load and a store for each qword
    auto copy_bytes(const token& src_loc_tk, const size_t indent,
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

                    store_immediate_part(src_loc_tk, indent, bytes, dst,
                                         size_dword_, offset);

                    store_immediate_part(src_loc_tk, indent, bytes, dst,
                                         size_dword_, offset + size_dword_);

                    return;
                }

                store_immediate_part(src_loc_tk, indent, bytes, dst,
                                     part_size_bytes, offset);
            });
    }

    auto copy_elements(const token& src_loc_tk, const size_t indent,
                       const size_t element_size_bytes,
                       const std::function_ref<void(const operand&)> emit_count,
                       const copy_request& request) -> void override {

        const operand count{alloc_bulk_registers(src_loc_tk, indent)};

        emit_count(count);

        request.src(count, {}, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rsi"), address);
        });

        request.dst(count, {}, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rdi"), address);
        });

        scale_by_element_size_bytes(src_loc_tk, indent, qword_register("rcx"),
                                    element_size_bytes);

        if (request.overlap.enabled) {
            check_copy_overlap(src_loc_tk, indent, request.overlap);
        }

        assembler_.instruction(indent, op::rep_movsb);
        release_bulk_registers(src_loc_tk, indent);
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        assert(dst.is_register() or dst.is_memory());

        mov(src_loc_tk, indent, dst, src);
    }

    [[nodiscard]] auto data_alignment() const -> size_t override {
        return data_alignment_;
    }

    [[nodiscard]] auto default_type() const -> const type& override {
        return builtin_type_i64();
    }

    auto define_constant(const std::string_view name, const size_t value)
        -> void override {

        assembler_.define_constant(name, static_cast<int64_t>(value));
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

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        emit_line_panic_handler(with_line, "bounds", bounds_panic_lines_,
                                bounds_failure_handler_label);
    }

    auto emit_data(const size_t element_size_bytes, const int64_t value)
        -> void override {

        const std::array<int64_t, 1> values{value};

        assembler_.data(element_size_bytes, values);
    }

    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(int64_t&)> next)
        -> void override {

        std::vector<int64_t> values;
        int64_t value{};
        while (next(value)) {
            values.push_back(value);
        }

        assembler_.data(element_size_bytes, values);
    }

    auto emit_division_failure_handler(const bool with_line) -> void override {
        emit_line_panic_handler(with_line, "division", division_panic_lines_,
                                division_failure_handler_label);
    }

    auto emit_frame_overflow_handler() -> void override {
        emit_line_panic_handler(false, "frame overflow", {},
                                frame_overflow_handler_label);
    }

    auto emit_overflow_failure_handler(const bool with_line) -> void override {
        emit_line_panic_handler(with_line, "overflow", overflow_panic_lines_,
                                overflow_failure_handler_label);
    }

    auto emit_overlap_failure_handler(const bool with_line) -> void override {
        emit_line_panic_handler(with_line, "overlap", overlap_panic_lines_,
                                overlap_failure_handler_label);
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const int64_t value) -> void override {

        assembler_.repeated_data(element_size_bytes, count, value);
    }

    auto emit_shift_failure_handler(const bool with_line) -> void override {
        emit_line_panic_handler(with_line, "shift", shift_panic_lines_,
                                shift_failure_handler_label);
    }

    // the operating system stops a program that overflows the stack
    auto emit_stack_overflow_handler() -> void override {}

    auto emit_string_constants(const std::span<const string_constant> strings)
        -> void override {

        assembler_.switch_section(section::rodata);
        for (const string_constant& s : strings) {
            assembler_.label(0, s.label);
            emit_string_data(s.text);
        }

        assembler_.switch_section(section::text);
    }

    // the escapes of the source are not those of nasm, e.g. nasm reads '\01'
    // as one octal escape and '\`' would end the string, so the bytes are
    // written again with the escapes both read the same
    auto emit_string_data(const std::string_view value) -> void override {
        assembler_.string_data(
            token::encode_string(token::decode_string(value)));
    }

    auto emit_zero_data(const size_t size_bytes) -> void override {
        const size_t count{size_bytes / size_byte_};
        emit_repeated_data(size_byte_, count, 0);
    }

    auto end_main() -> void override {
        exit(token{}, 1, immediate(0));
        assembler_.add_separator_newline();
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, qword_register("rdi"), exit_code);

        mov(src_loc_tk, indent, qword_register("rax"),
            immediate(syscall_exit_));

        syscall(indent);
    }

    // releases the variables base, optimizes the jumps and asserts that the
    // registers are balanced; called once at the end of the compile pass
    auto finish() -> void override {
        finish_registers();
        finish_output();
    }

    auto foo_advance_iteration(const token& src_loc_tk, const size_t indent,
                               const operand& iterator, const operand& counter,
                               const size_t element_size_bytes,
                               const operand& limit,
                               const std::string_view loop_label)
        -> void override {

        add(src_loc_tk, indent, iterator, immediate(element_size_bytes));
        inc(src_loc_tk, indent, counter);
        cmp_lowered(src_loc_tk, indent, counter, limit);
        assembler_.jcc(indent, condition::ne, loop_label);
    }

    // 'inc' and 'cmp' work on memory, so a loop counter leaves its register to
    // the values that need one once many are live
    [[nodiscard]] auto foo_counter_in_memory() const -> bool override {
        return registers().scratch_count() >= memory_counter_pressure_count_;
    }

    [[nodiscard]] auto frame_base_register() const
        -> std::string_view override {

        return "rbx";
    }

    // e.g. '[rbx + rcx * 8 + 16]'
    [[nodiscard]] auto has_indexed_addressing() const -> bool override {
        return true;
    }

    auto label(const size_t indent, const std::string_view label)
        -> void override {

        assembler_.label(indent, label);
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

    [[nodiscard]] auto max_storage_bytes() const -> size_t override {
        return user_address_space_bytes_;
    }

    auto memory_equal(const token& src_loc_tk, const size_t indent,
                      const size_t size_bytes, const equality_request& request)
        -> void override {

        std::ignore = alloc_bulk_registers(src_loc_tk, indent);

        request.lhs({}, {}, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rsi"), address);
        });

        request.rhs({}, {}, [&](const operand& address) -> void {
            lea(src_loc_tk, indent, qword_register("rdi"), address);
        });

        const std::vector<op> compares{memory_equal_compares(size_bytes)};

        assert(not compares.empty());

        if (compares.front() == op::repe_cmpsq) {
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

        release_bulk_registers(src_loc_tk, indent);
        store_equal_result(src_loc_tk, indent, request.dst, request.inverted);
    }

    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  const bool reuse_source = {}, const runtime_check& check = {})
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
                check_byte_product(src_loc_tk, indent, left, right, check);
            }

            mov(src_loc_tk, indent, product, left);
            free_scratch_register(src_loc_tk, indent, right);
            free_scratch_register(src_loc_tk, indent, left);

            return;
        }

        if (product.is_register()) {
            imul(src_loc_tk, indent, product, factor);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent, check);
            }

            return;
        }

        if (reuse_source) {
            imul(src_loc_tk, indent, factor, product);

            // the move keeps the flags
            mov(src_loc_tk, indent, product, factor);

            if (check.enabled) {
                branch_on_overflow(src_loc_tk, indent, check);
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
            branch_on_overflow(src_loc_tk, indent, check);
        }

        mov(src_loc_tk, indent, product, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto read(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& descriptor, const operand& address,
              const operand& count) -> void override {

        io_syscall(src_loc_tk, indent, dst, descriptor, address, count,
                   syscall_read_);
    }

    [[nodiscard]] auto register_display_name(const size_t index) const
        -> std::string override {

        return std::string{register_names_.at(index).qword};
    }

    [[nodiscard]] auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers override {

        static constexpr std::array<std::string_view, 3> io_args{
            "rdi",
            "rsi",
            "rdx",
        };

        static constexpr std::array<std::string_view, 1> exit_args{"rdi"};

        if (function == builtin_function::exit) {
            return {
                .arguments{exit_args},
                .result{},
            };
        }

        // the sector, the address and the size
        if (function == builtin_function::sector_read or
            function == builtin_function::sector_write) {

            return {
                .arguments{io_args},
                .result{},
            };
        }

        return {
            .arguments{io_args},
            .result{"rax"},
        };
    }

    auto reserve_variables(const size_t alignment, const size_t size_bytes)
        -> void override {

        assembler_.label(0, data_end_label);
        assembler_.add_separator_newline();
        assembler_.switch_section(section::variables);
        assembler_.align(alignment);
        assembler_.label(0, variables_label);
        assembler_.reserve(size_bytes);
        assembler_.label(0, variables_end_label);
    }

    auto return_function(const size_t indent) -> void override {
        assembler_.instruction(indent, op::ret);
    }

    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size_bytes)
        -> void override {

        scale_by_element_size_bytes(src_loc_tk, indent, index,
                                    element_size_bytes);
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

        // a count wider than the value is rejected as narrowed
        assert(count.type_ref().size_bytes() <= dst.type_ref().size_bytes());

        mov(src_loc_tk, indent,
            sized_register("rcx", dst.type_ref().size_bytes()), count);

        if (check.enabled) {
            check_shift_count(src_loc_tk, indent, dst.type_ref(), check);
        }

        emit_op(src_loc_tk, indent, code, dst,
                sized_register("rcx", size_byte_));

        release_named_register(src_loc_tk, indent, "rcx");
    }

    [[nodiscard]] auto slot_register() const -> std::string_view override {
        return slot_register_;
    }

    auto start() -> void override {
        start_output();

        assembler_.default_rel();
        assembler_.add_separator_newline();

        assembler_.switch_section(section::text);
        assembler_.bits64();
        assembler_.global("_start");
        assembler_.label(0, "_start");
        assembler_.add_separator_newline();
        reserve_variables_base();

        assembler_.instruction(0, op::lea, variables_base_register_,
                               assembler_x86_64::memory::of_symbol(data_label));

        assembler_.add_separator_newline();
    }

    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void override {

        mov(src_loc_tk, indent, dst, immediate(value ? 1 : 0));
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
            branch_on_overflow(src_loc_tk, indent, check);
        }
    }

    [[nodiscard]] auto variables_base_past_vars_bytes() const
        -> std::optional<size_t> override {

        return std::nullopt;
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        return variables_base_register_;
    }

    auto write(const token& src_loc_tk, const size_t indent, const operand& dst,
               const operand& descriptor, const operand& address,
               const operand& count) -> void override {

        io_syscall(src_loc_tk, indent, dst, descriptor, address, count,
                   syscall_write_);
    }

    auto write_assembly(std::ostream& os) -> void override {
        assembler_.write(os);
        assembler_.set_direct_output(direct_stream());
    }

    auto zero(const token& src_loc_tk, const size_t indent, const operand& dst,
              const size_t size_bytes, [[maybe_unused]] const size_t alignment)
        -> void override {

        if (size_bytes > threshold_for_rep_stos_size_bytes_) {
            reserve_named_register(src_loc_tk, indent, "rax", default_type());
            reserve_named_register(src_loc_tk, indent, "rdi", default_type());
            reserve_named_register(src_loc_tk, indent, "rcx", default_type());

            xor_op(
                src_loc_tk, indent,
                machine_x86_64::make_register_operand("al", builtin_type_i8()),
                machine_x86_64::make_register_operand("al", builtin_type_i8()));

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

    // 'syscall' changes 'rcx' and 'r11', the allocated ones are saved
    auto invoke_syscall(const size_t indent) -> void {
        static constexpr std::array<std::string_view, 2> clobbered{
            "rcx",
            "r11",
        };

        with_saved_registers(indent, clobbered,
                             [&] -> void { syscall(indent); });
    }

  protected:
    //
    // overridden methods
    //

    // the routine changes 'rax', 'rcx', 'r10' and 'r11' besides the
    // arguments, the allocated ones are saved around the call
    auto call_sector_routine(const token& src_loc_tk, const size_t indent,
                             const operand& sector, const operand& address,
                             const operand& size,
                             const builtin_function function) -> void override {

        for (const operand* value : {&sector, &address, &size}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == size_qword_);
        }

        assert(sector.base_register() == "rdi");
        assert(address.base_register() == "rsi");
        assert(size.base_register() == "rdx");

        static constexpr std::array<std::string_view, 4> clobbered{
            "rax",
            "rcx",
            "r10",
            "r11",
        };

        const std::string_view label{
            function == builtin_function::sector_read ? sector_read_label_
                                                      : sector_write_label_,
        };

        comment(src_loc_tk, indent, "sector of the sd card");

        with_saved_registers(indent, clobbered,
                             [&] -> void { assembler_.call(indent, label); });
    }

    // the routines take the sector in 'rdi', the address in 'rsi' and the size
    // in 'rdx'; the image file is opened at the first transfer, a sector past
    // its end or outside 32 bits, a failed open or a short transfer panics
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void override {

        // 'O_RDWR' and 'SEEK_END' of linux
        constexpr int read_write{2};
        constexpr int seek_end{2};
        constexpr int sector_shift{std::countr_zero(card_sector_size_bytes)};

        // the descriptor is -1 until the file is opened
        constexpr std::array<int64_t, 1> not_opened{-1};

        constexpr std::array<int64_t, 1> zero_value{0};

        // every failure jumps here and exits with a panic message
        constexpr std::string_view handler_label{"baz_sd_card_panic"};

        // the open file descriptor, a qword in '.data'
        assembler_x86_64::memory descriptor{
            assembler_x86_64::memory::of_symbol("baz_sd_card_descriptor"),
        };

        descriptor.size_bytes = size_qword_;

        // the file size in whole sectors, set once at open
        assembler_x86_64::memory sector_count{
            assembler_x86_64::memory::of_symbol("baz_sd_card_sectors"),
        };

        sector_count.size_bytes = size_qword_;

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

        // the shared body of read and write
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
        assembler_.jcc(1, condition::s, handler_label);
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
        assembler_.jcc(1, condition::s, handler_label);
        // bytes to whole sectors, a partial last sector is not counted
        assembler_.instruction(1, op::sar, "rax", sector_shift);
        assembler_.instruction(1, op::mov, sector_count, "rax");
        // restore the arguments in reverse order of the pushes
        assembler_.instruction(1, op::pop, "rdx");
        assembler_.instruction(1, op::pop, "rsi");
        assembler_.instruction(1, op::pop, "rdi");
        // here the file is open and only the system call is on the stack
        assembler_.label(0, "baz_sector_opened");
        assembler_.comment(1, "the sector is an unsigned 32 bit number");
        // zero extending 'edi' differs from 'rdi' when the upper half is set
        assembler_.instruction(1, op::mov, "eax", "edi");
        assembler_.instruction(1, op::cmp, "rax", "rdi");
        assembler_.jcc(1, condition::ne, handler_label);
        // a sector at or past the end of the file panics
        assembler_.instruction(1, op::cmp, "rdi", sector_count);
        assembler_.jcc(1, condition::ae, handler_label);
        // the byte offset, the fourth system call argument, goes in 'r10'
        assembler_.instruction(1, op::mov, "r10", "rdi");
        assembler_.instruction(1, op::shl, "r10", sector_shift);
        // the descriptor replaces the sector as the first argument
        assembler_.instruction(1, op::mov, "rdi", descriptor);
        // the saved system call number, which also balances the stack
        assembler_.instruction(1, op::pop, "rax");
        // pread64 or pwrite64(descriptor, address 'rsi', size 'rdx', offset)
        assembler_.instruction(1, op::syscall);
        // anything but the full size, an error or a short transfer, panics
        assembler_.instruction(1, op::cmp, "rax", "rdx");
        assembler_.jcc(1, condition::ne, handler_label);
        assembler_.instruction(1, op::ret);

        // the stack is not unwound because the program exits
        assembler_.label(0, handler_label);
        emit_panic_message("msg_sd_card");
        emit_panic_exit();

        // the panic message and its length symbol
        assembler_.switch_section(section::rodata);
        assembler_.label(0, "msg_sd_card");
        assembler_.string_data("panic: sd card\\n");
        assembler_.define_length("msg_sd_card_len", "msg_sd_card");
        assembler_.comment(0, "the file name ends with a zero byte");
        // the image file name passed to 'open'
        assembler_.label(0, "baz_sd_card_name");

        assembler_.string_data(token::encode_string(sd_card_file_name()));

        // the terminator 'open' expects
        assembler_.data(size_byte_, zero_value);

        // writable state, qword aligned for the 64 bit accesses
        assembler_.switch_section(section::data);
        assembler_.align(size_qword_);
        assembler_.label(0, "baz_sd_card_descriptor");
        assembler_.data(size_qword_, not_opened);
        assembler_.label(0, "baz_sd_card_sectors");
        assembler_.data(size_qword_, zero_value);
        // later code is emitted into '.text' again
        assembler_.switch_section(section::text);
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

    [[nodiscard]] auto scratch_registers() const
        -> std::span<const std::string_view> override {

        return scratch_registers_;
    }

    [[nodiscard]] auto target_assembler() -> assembler& override {
        return assembler_;
    }

    [[nodiscard]] auto target_assembler() const -> const assembler& override {
        return assembler_;
    }

  private:
    auto add(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& src) -> void {

        emit_binary(src_loc_tk, indent, op::add, dst, src);
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

        return not value.is_memory() or
               (std::in_range<int32_t>(value.displacement()) and
                (value.index_register().empty() or
                 (value.index_register() != "rsp" and
                  can_lower_index_scale(value.scale()))));
    }

    [[nodiscard]] auto alloc_bulk_registers(const token& src_loc_tk,
                                            const size_t indent) -> operand {

        reserve_named_register(src_loc_tk, indent, "rsi", default_type());
        reserve_named_register(src_loc_tk, indent, "rdi", default_type());
        return alloc_named_register(src_loc_tk, indent, "rcx", default_type());
    }

    [[nodiscard]] auto
    allocated_register_type(const std::string_view name) const -> const type* {

        const size_t size_bytes{register_size_bytes(name)};

        assert(size_bytes != 0);

        const std::optional<size_t> index{register_index_of(name)};

        for (const register_pool::allocation& allocated :
             registers().allocations()) {

            if (allocated.index == index) {
                return allocated.type_ptr->size_bytes() == size_bytes
                           ? allocated.type_ptr
                           : &builtin_type_for_size_bytes(size_bytes);
            }
        }

        return nullptr;
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
    auto branch_on_overflow(const token& src_loc_tk, const size_t indent,
                            const runtime_check& check) -> void {

        assembler_.jcc(indent, condition::o,
                       failure_label("overflow", overflow_panic_lines_,
                                     overflow_failure_handler_label, src_loc_tk,
                                     check.with_line));
    }

    // a reported line goes through the stub of its line, which sets rbp for
    // the handler, so a passing check keeps rbp and loads nothing
    auto branch_to_bounds_panic(const token& src_loc_tk, const size_t indent,
                                const condition failed, const bool with_line)
        -> void {

        assembler_.jcc(indent, failed,
                       failure_label("bounds", bounds_panic_lines_,
                                     bounds_failure_handler_label, src_loc_tk,
                                     with_line));
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

    // 'left' holds the product of two bytes in a register of the default
    // width, which fits when the low byte sign extended is the same value
    auto check_byte_product(const token& src_loc_tk, const size_t indent,
                            const operand& left, const operand& right,
                            const runtime_check& check) -> void {

        const operand low_byte{
            make_register_operand(left.base_register(), builtin_type_i8()),
        };

        mov(src_loc_tk, indent, right, low_byte);
        cmp(src_loc_tk, indent, right, left);

        assembler_.jcc(indent, condition::ne,
                       failure_label("overflow", overflow_panic_lines_,
                                     overflow_failure_handler_label, src_loc_tk,
                                     check.with_line));
    }

    // the source in 'rsi', the destination in 'rdi' and the byte count in
    // 'rcx' of a copy: a distance from the source to the destination that is
    // not zero and below the count jumps to the failure handler, the copy
    // would overwrite source bytes it has not read yet
    auto check_copy_overlap(const token& src_loc_tk, const size_t indent,
                            const runtime_check& check) -> void {

        comment(src_loc_tk, indent, "overlap check begin");

        const std::string failed{
            failure_label("overlap", overlap_panic_lines_,
                          overlap_failure_handler_label, src_loc_tk,
                          check.with_line),
        };

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
        assembler_.jcc(indent, condition::b, failed);
        assembler_.label(indent, passed);
        free_scratch_register(src_loc_tk, indent, distance);

        comment(src_loc_tk, indent, "overlap check end");
    }

    // a zero divisor and the minimum of the dividend width divided by -1 jump
    // to the failure handler, the dividend is sign extended in 'rax'
    auto check_divisor(const token& src_loc_tk, const size_t indent,
                       const operand& divisor, const size_t dividend_size_bytes,
                       const bool with_line) -> void {

        const int64_t minimum{
            constant_arith::signed_minimum(dividend_size_bytes * bits_per_byte),
        };

        const std::string not_minimum{
            std::format(".Lbaz_division.{}", division_label_count_++),
        };

        comment(src_loc_tk, indent, "division check begin");

        const std::string failed{
            failure_label("division", division_panic_lines_,
                          division_failure_handler_label, src_loc_tk,
                          with_line),
        };

        comment(src_loc_tk, indent, "zero divisor");

        cmp(src_loc_tk, indent, divisor, immediate(0));
        assembler_.jcc(indent, condition::e, failed);

        comment(src_loc_tk, indent, "minimum divided by -1 overflows");

        cmp(src_loc_tk, indent, divisor, immediate(-1));
        assembler_.jcc(indent, condition::ne, not_minimum);
        mov(src_loc_tk, indent, qword_register("rdx"), immediate(minimum));
        cmp(src_loc_tk, indent, qword_register("rax"), qword_register("rdx"));
        assembler_.jcc(indent, condition::e, failed);
        assembler_.label(indent, not_minimum);

        comment(src_loc_tk, indent, "division check end");
    }

    auto check_lower_bound(const token& src_loc_tk, const size_t indent,
                           const bounds_range& range, const bounds_plan& plan,
                           const bool with_line) -> void {

        comment_lower_bound(src_loc_tk, indent, range, plan);

        if (not plan.upper_covers_lower) {
            test_sign(src_loc_tk, indent, range.checked, with_line);

            // a negative count passes 'start + count' but spans the address
            // space
            if (not plan.count_known) {
                test_sign(src_loc_tk, indent, range.count, with_line);
            }
        }

        mark_lower_checked(range);
    }

    // a count in 'rcx' below zero or not below the width jumps to the failure
    // handler, a negative count is a large unsigned one
    auto check_shift_count(const token& src_loc_tk, const size_t indent,
                           const type& dst_type, const runtime_check& check)
        -> void {

        comment(src_loc_tk, indent, "shift check begin");

        const std::string failed{
            failure_label("shift", shift_panic_lines_,
                          shift_failure_handler_label, src_loc_tk,
                          check.with_line),
        };

        cmp(src_loc_tk, indent, sized_register("rcx", dst_type.size_bytes()),
            immediate(static_cast<int64_t>(dst_type.size_bits())));

        assembler_.jcc(indent, condition::ae, failed);

        comment(src_loc_tk, indent, "shift check end");
    }

    auto cmp(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        emit_op(src_loc_tk, indent, op::cmp, dst_op, src_op);
    }

    auto cmp_lowered(const token& src_loc_tk, const size_t indent,
                     const operand& dst, const operand& src) -> void {

        emit_binary(src_loc_tk, indent, op::cmp, dst, src);
    }

    // a range 'index + count' may end at the array count
    auto compare_upper_bound(const token& src_loc_tk, const size_t indent,
                             const bounds_range& range) -> void {

        if (range.count.is_empty()) {
            cmp_lowered(src_loc_tk, indent, range.checked,
                        immediate(range.array_count));

            return;
        }

        const operand reg_top_idx{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        // index and count registers have the default type
        assert(range.count.type_ref().size_bytes() == size_qword_ and
               range.checked.type_ref().size_bytes() == size_qword_);

        lea(src_loc_tk, indent, reg_top_idx,
            operand::mem(range.count.base_register(),
                         range.checked.base_register(), 1, 0,
                         builtin_type_i64()));

        cmp_lowered(src_loc_tk, indent, reg_top_idx,
                    immediate(range.array_count));

        free_scratch_register(src_loc_tk, indent, reg_top_idx);
    }

    // 'load_source_address' sets the 'rep movsb' source pointer
    auto copy_with_rep_movsb(
        const token& src_loc_tk, const size_t indent, const operand& dst,
        const size_t size_bytes,
        const std::function_ref<void(const operand&)> load_source_address)
        -> void {

        reserve_named_register(src_loc_tk, indent, "rsi", default_type());
        reserve_named_register(src_loc_tk, indent, "rdi", default_type());
        reserve_named_register(src_loc_tk, indent, "rcx", default_type());

        load_source_address(qword_register("rsi"));
        lea(src_loc_tk, indent, qword_register("rdi"), dst);
        mov(src_loc_tk, indent, qword_register("rcx"), immediate(size_bytes));

        assembler_.instruction(indent, op::rep_movsb);

        release_bulk_registers(src_loc_tk, indent);
    }

    // writes the instruction with the operands as they are
    auto emit(const size_t indent, const op code, const operand& dst,
              const operand& src) -> void {

        assembler_.instruction(indent, code, to_argument(dst),
                               to_argument(src));
    }

    auto emit_binary(const token& src_loc_tk, const size_t indent,
                     const op code, const operand& dst, const operand& src)
        -> void {

        with_lowered_addresses(src_loc_tk, indent, dst, src,
                               [&](const operand& lowered_dst,
                                   const operand& lowered_src) -> void {
                                   if (lowered_src.is_immediate()) {
                                       emit_immediate_op(src_loc_tk, indent,
                                                         code, lowered_dst,
                                                         lowered_src);

                                       return;
                                   }

                                   emit(indent, code, lowered_dst, lowered_src);
                               });
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

    // the handler of a check that may report the line: one stub per line sets
    // rbp and enters the handler, then the message 'panic: NAME' is printed
    auto emit_line_panic_handler(const bool with_line,
                                 const std::string_view name,
                                 const std::set<size_t>& panic_lines,
                                 const std::string_view handler_label) -> void {

        // a label has no space, 'frame overflow' is 'msg_frame_overflow'
        std::string label_name{name};
        std::ranges::replace(label_name, ' ', '_');

        const std::string message_label{std::format("msg_{}", label_name)};

        if (with_line) {
            for (const size_t line : panic_lines) {
                assembler_.label(0, line_panic_label(name, line));
                assembler_.instruction(1, op::mov, "rbp", line);
                assembler_.jmp(1, handler_label);
            }
        }

        assembler_.label(0, handler_label);
        emit_panic_message(message_label);

        if (with_line) {
            emit_report_line();
        } else {
            emit_panic_exit();
        }

        assembler_.switch_section(section::rodata);
        assembler_.label(0, message_label);

        assembler_.string_data(with_line
                                   ? std::format("panic: {} at line ", name)
                                   : std::format("panic: {}\\n", name));

        assembler_.define_length(std::format("{}_len", message_label),
                                 message_label);

        assembler_.switch_section(section::text);
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

    auto emit_panic_exit() -> void {
        assembler_.comment(1, "exit with error code 255");
        assembler_.instruction(1, op::mov, "rax", syscall_exit_);
        assembler_.instruction(1, op::mov, "rdi", panic_exit_code);
        assembler_.instruction(1, op::syscall);
    }

    // shared by the panic handlers, which run after all allocations
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

    // prints the line number in 'rbp' and exits, the code is emitted once and
    // shared by the handlers that report a line
    auto emit_report_line() -> void {
        constexpr std::string_view report_label{"baz_report_line"};

        if (line_report_emitted_) {
            assembler_.jmp(1, report_label);
            return;
        }

        line_report_emitted_ = true;

        assembler_.label(0, report_label);

        constexpr int newline{'\n'};
        constexpr int decimal_base{10};
        // the 20 digits of a 64-bit number and a newline, the digits are
        // written backwards before the newline at the end of the buffer
        constexpr size_t number_buffer_size_bytes{21};
        constexpr size_t newline_offset{number_buffer_size_bytes - 1};

        assembler_.comment(1, "line number is in `rbp`");
        assembler_.instruction(1, op::mov, "rax", "rbp");
        assembler_.comment(1, "convert to string");

        assembler_.instruction(
            1, op::mov, "rdi",
            assembler_x86_64::immediate::of_expression(
                std::format("num_buffer + {}", newline_offset), true));

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
            assembler_x86_64::immediate::of_expression(
                std::format("num_buffer + {}", number_buffer_size_bytes),
                true));

        assembler_.instruction(1, op::sub, "rdx", "rdi");
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor);
        assembler_.instruction(1, op::syscall);
        emit_panic_exit();
        assembler_.switch_section(section::bss);
        assembler_.label(0, "num_buffer");
        assembler_.reserve(number_buffer_size_bytes);
    }

    // idiv takes the divisor as a qword register or memory operand, 'rax'
    // holds the dividend, 'rdx' is free until it is extended
    auto emit_signed_divide(const token& src_loc_tk, const size_t indent,
                            const operand& divisor, const runtime_check& check,
                            const size_t dividend_size_bytes) -> void {

        if (not divisor.is_immediate() and
            divisor.type_ref().size_bytes() == size_qword_) {

            if (check.enabled) {
                check_divisor(src_loc_tk, indent, divisor, dividend_size_bytes,
                              check.with_line);
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
            check_divisor(src_loc_tk, indent, scratch_reg, dividend_size_bytes,
                          check.with_line);
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

        assert(dst.base_register() == "rax");
        assert(descriptor.base_register() == "rdi");
        assert(address.base_register() == "rsi");
        assert(count.base_register() == "rdx");

        mov(src_loc_tk, indent, dst, immediate(syscall_number));
        invoke_syscall(indent);
    }

    auto lea(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& address, const bool explicit_displacement = {})
        -> void {

        with_lowered_addresses(
            src_loc_tk, indent, dst, address,
            [&](const operand& lowered_dst,
                const operand& lowered_address) -> void {
                assembler_.instruction(
                    indent, op::lea, to_argument(lowered_dst),
                    to_address(lowered_address, explicit_displacement));
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

    // the register it allocates is freed by the 'address_scope' of the
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

        const std::string_view sum{address.base_register()};
        assembler_.instruction(indent, op::mov, sum, value.displacement());

        if (not value.base_register().empty()) {
            assembler_.instruction(indent, op::lea, sum,
                                   register_sum(value.base_register(), sum, 1));
        }

        if (not value.index_register().empty()) {
            add_scaled_index(indent, sum, value);
        }

        return operand::mem(address.base_register(), {}, 1, 0,
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
            // 'xor' is the shorter idiom but cannot target memory
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
                branch_on_overflow(src_loc_tk, indent, check);
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

    auto release_bulk_registers(const token& src_loc_tk, const size_t indent)
        -> void {

        release_named_register(src_loc_tk, indent, "rcx");
        release_named_register(src_loc_tk, indent, "rdi");
        release_named_register(src_loc_tk, indent, "rsi");
    }

    auto scale_by_element_size_bytes(const token& src_loc_tk,
                                     const size_t indent, const operand& value,
                                     const size_t element_size_bytes) -> void {

        if (element_size_bytes <= 1) {
            return;
        }

        if (std::has_single_bit(element_size_bytes)) {
            shl(src_loc_tk, indent, value,
                immediate(std::countr_zero(element_size_bytes)));

            return;
        }

        imul(src_loc_tk, indent, value, immediate(element_size_bytes));
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

    auto shl(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& src) -> void {

        emit_binary(src_loc_tk, indent, op::shl, dst, src);
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

        operand result{sized_register(reg.base_register(), size_bytes)};
        result.set_allocation_register(reg.allocation_register());

        return result;
    }

    auto store_comparison(const token& src_loc_tk, const size_t indent,
                          const comparison_operator comparison,
                          const bool inverted, const operand& dst) -> void {

        if (dst.is_memory()) {
            setcc(src_loc_tk, indent,
                  condition_for_comparison(comparison, inverted),
                  sized_memory(dst, size_byte_));

            return;
        }

        setcc(src_loc_tk, indent,
              condition_for_comparison(comparison, inverted), dst);
    }

    auto store_equal_result(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const bool inverted) -> void {

        const condition cc{inverted ? condition::ne : condition::e};

        if (dst.is_register()) {
            setcc(src_loc_tk, indent, cc, sized_register(dst, size_byte_));
            return;
        }

        setcc(src_loc_tk, indent, cc, sized_memory(dst, size_byte_));
    }

    auto store_immediate_part(const token& src_loc_tk, const size_t indent,
                              const std::string_view bytes, const operand& dst,
                              const size_t part_size_bytes, const size_t offset)
        -> void {

        mov(src_loc_tk, indent, memory_part(dst, part_size_bytes, offset),
            immediate(
                little_endian_value(bytes.substr(offset, part_size_bytes))));
    }

    auto syscall(const size_t indent) -> void {
        assembler_.instruction(indent, op::syscall);
    }

    auto test(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& src) -> void {

        emit_binary(src_loc_tk, indent, op::test, dst, src);
    }

    // a negative value is out of bounds, an empty operand has none to test
    auto test_sign(const token& src_loc_tk, const size_t indent,
                   const operand& value, const bool with_line) -> void {

        if (value.is_empty()) {
            return;
        }

        test(src_loc_tk, indent, value, value);
        branch_to_bounds_panic(src_loc_tk, indent, condition::s, with_line);
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
        const address_scope scope{*this, dst, src};

        const operand lowered_dst{lower_address(src_loc_tk, indent, dst)};
        const operand lowered_src{lower_address(src_loc_tk, indent, src)};

        emit(lowered_dst, lowered_src);
    }

    // 'emit' changes the registers 'clobbered', the allocated ones are pushed
    // before and popped after
    auto with_saved_registers(const size_t indent,
                              const std::span<const std::string_view> clobbered,
                              const std::function_ref<void()> emit) -> void {

        std::vector<operand> saved;
        for (const std::string_view name : clobbered) {
            if (allocated_register_type(name) != nullptr) {
                saved.push_back(qword_register(name));
                push(indent, saved.back());
            }
        }

        emit();

        for (const operand& reg : saved | std::views::reverse) {
            pop(indent, reg);
        }
    }

    auto xor_op(const token& src_loc_tk, const size_t indent,
                const operand& dst, const operand& src) -> void {

        emit_binary(src_loc_tk, indent, op::xor_op, dst, src);
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

    // the handler 'handler_label' to jump to, or with the line the stub of
    // the line, which 'panic_lines' records so the handler emits it
    [[nodiscard]] static auto
    failure_label(const std::string_view name, std::set<size_t>& panic_lines,
                  const std::string_view handler_label, const token& src_loc_tk,
                  const bool with_line) -> std::string {

        if (not with_line) {
            return std::string{handler_label};
        }

        panic_lines.insert(src_loc_tk.at_line());

        return line_panic_label(name, src_loc_tk.at_line());
    }

    [[nodiscard]] static auto line_panic_label(const std::string_view name,
                                               const size_t line)
        -> std::string {

        return std::format("baz_{}_line_{}", name, line);
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
            .explicit_displacement{},
        };
    }

    [[nodiscard]] static auto same_operand(const operand& lhs,
                                           const operand& rhs) -> bool {

        if (lhs.is_register()) {
            return rhs.is_register() and
                   lhs.base_register() == rhs.base_register();
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
    [[nodiscard]] static auto to_address(const operand& value,
                                         const bool explicit_displacement = {})
        -> assembler_x86_64::memory {

        assert(not value.is_immediate());

        return {
            .symbol{},
            .base{value.base_register()},
            .index{value.index_register()},
            .scale{value.scale()},
            .displacement{value.displacement()},
            .size_bytes{},
            .explicit_displacement{explicit_displacement},
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
            return std::string_view{value.base_register()};
        }

        assembler_x86_64::memory address{to_address(value)};
        address.size_bytes = value.type_ref().size_bytes();

        return address;
    }
};
