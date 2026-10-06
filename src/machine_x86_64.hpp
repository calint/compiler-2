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
#include <limits>
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
#include "decouple.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"

class token;
class type;

class machine_x86_64 final : public machine {
    using op = assembler_x86_64::op;

    using condition = assembler_x86_64::condition;

    using section = assembler_x86_64::section;

    static constexpr size_t size_qword{assembler_x86_64::size_qword};
    static constexpr size_t size_dword{assembler_x86_64::size_dword};
    static constexpr size_t size_word{assembler_x86_64::size_word};
    static constexpr size_t size_byte{assembler_x86_64::size_byte};

    static constexpr size_t data_alignment_{16};

    static constexpr std::string_view variables_base_register_{"rbp"};

    static constexpr std::array<size_t, 4> index_register_scalings{1, 2, 4, 8};

    static constexpr size_t threshold_for_rep_stos_size_bytes{32};
    static constexpr size_t threshold_for_rep_movs_size_bytes{16};
    // up to this many qwords single compares cost less than loading 'rcx'
    static constexpr size_t threshold_for_repe_cmpsq_count{2};
    static constexpr int syscall_read{0};
    static constexpr int syscall_write{1};
    static constexpr int syscall_exit{60};

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

    // scratch values live from which an array loop counts in memory
    static constexpr size_t memory_counter_pressure_count{6};

    // the indexes are those of 'register_names_'
    register_pool registers_;
    bool variables_base_reserved_{};
    bool frame_base_reserved_{};
    // numbers the labels after the parts of a memory compare
    size_t equal_label_count_{};
    // source lines of the bounds checks, each gets a stub that reports it
    std::set<size_t> bounds_panic_lines_;

    // buffering output is no more logical state than writing to the stream
    mutable assembler_x86_64 assembler_;

    // the registers of the address of a memory operand
    struct address_registers {
        std::string_view base;
        std::string_view index;
    };

  public:
    explicit machine_x86_64(std::ostream& os_ref, const std::string_view source,
                            const jump_mode jumps = jump_mode::resolved)
        : machine{os_ref, source, jumps} {

        // output before 'start' is written as emitted in every mode
        assembler_.set_direct_output(&stream());
    }

    using machine::emit_data_array;

    //
    // overridden methods
    //

    auto add_subtract(const token& src_loc_tk, const size_t indent,
                      const arithmetic_operator operation, const operand& dst,
                      const operand& src) -> void override {

        assert(operation == arithmetic_operator::add or
               operation == arithmetic_operator::subtract);

        emit_op(src_loc_tk, indent,
                operation == arithmetic_operator::add ? op::add : op::sub, dst,
                src);
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        lea_into(src_loc_tk, indent, dst, address);
    }

    [[nodiscard]] auto address_size_bytes() const -> size_t override {
        return size_qword;
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand override {

        reserve_named_register(src_loc_tk, indent, register_name, type_ref);

        operand result{make_register_operand(register_name, type_ref)};

        result.set_allocation_register(register_name);

        return result;
    }

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref)
        -> operand override {

        for (const std::string_view register_name : scratch_registers_) {
            if (registers_.is_unavailable(register_bit(register_name))) {
                continue;
            }

            comment(src_loc_tk, indent, "allocate scratch register -> {}",
                    register_name);

            push_allocation(src_loc_tk, register_name, type_ref, false);

            record_scratch_registers(registers_);

            operand result{make_register_operand(register_name, type_ref)};

            result.set_allocation_register(register_name);

            return result;
        }

        throw register_error(src_loc_tk,
                             "out of scratch registers. try to reduce "
                             "expression complexity",
                             registers_);
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
                       const operand& frame_address) -> void override {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(frame_address.base_register() != "rsp");

        // the callee may use every register but the variables base, so only
        // values live at the call need saving
        std::vector<operand> saved;
        for (const register_pool::allocation& allocated :
             registers_.allocations()) {

            const std::string_view name{
                register_names_.at(allocated.index).qword};

            if (name == variables_base_register_) {
                continue;
            }

            saved.push_back(qword_register(name));
        }

        record_noinline_call(src_loc_tk, label, saved.size());

        if (not saved.empty()) {
            comment(src_loc_tk, indent,
                    "before call: save allocated registers");
        }

        for (const operand& reg : saved) {
            push(indent, reg);
        }

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

        return std::ranges::contains(index_register_scalings, size_bytes);
    }

    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const operand& reg_to_check, const size_t array_count,
                      const bool allow_end, const operand& reg_count,
                      const bounds_check_options& options) -> void override {

        if (not options.upper and not options.lower) {
            return;
        }

        const bounds_plan plan{
            plan_bounds_check(options, reg_count, array_count,
                              signed_maximum(reg_to_check),
                              registers_.is_lower_checked(
                                  register_bit(reg_count.base_register()))),
        };

        const std::optional<size_t> reported_line{
            options.with_line ? std::optional<size_t>{src_loc_tk.at_line()}
                              : std::nullopt,
        };

        comment(src_loc_tk, indent, "bounds check begin");

        if (options.lower) {
            check_lower_bound(src_loc_tk, indent, reg_to_check, reg_count, plan,
                              reported_line);
        }

        if (options.upper) {
            comment(src_loc_tk, indent, "upper bound");

            compare_upper_bound(src_loc_tk, indent, reg_to_check, array_count,
                                reg_count);

            branch_to_bounds_panic(indent,
                                   out_of_bounds_condition(allow_end, plan),
                                   reported_line);
        }

        comment(src_loc_tk, indent, "bounds check end");
    }

    auto check_frame_capacity(const token& src_loc_tk, const size_t indent,
                              const operand& frame_address,
                              const operand& frame_size_bytes,
                              const bool enabled = {}) -> void override {

        if (not enabled) {
            return;
        }

        assert(frame_address.is_memory());
        assert(frame_size_bytes.is_immediate());

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
    auto copy(const token& src_loc_tk, const size_t indent, const operand& src,
              const operand& dst, const size_t size_bytes,
              [[maybe_unused]] const size_t alignment) -> void override {

        if (size_bytes > threshold_for_rep_movs_size_bytes) {
            copy_with_rep_movsb(src_loc_tk, indent, dst, size_bytes,
                                [&](const operand& pointer) -> void {
                                    lea(src_loc_tk, indent, pointer, src);
                                });

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_movs_size_bytes);

        reserve_named_register(src_loc_tk, indent, "rax", default_type());

        for_each_part(
            size_bytes, size_qword,
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

        if (bytes.size() > threshold_for_rep_movs_size_bytes) {
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
                threshold_for_rep_movs_size_bytes);

        for_each_part(
            bytes.size(), size_qword,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                const int64_t value{
                    little_endian_value(bytes.substr(offset, part_size_bytes)),
                };

                // a qword store takes only a sign-extended 32-bit immediate
                if (part_size_bytes == size_qword and
                    not std::in_range<int32_t>(value)) {

                    store_immediate_part(src_loc_tk, indent, bytes, dst,
                                         size_dword, offset);

                    store_immediate_part(src_loc_tk, indent, bytes, dst,
                                         size_dword, offset + size_dword);

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
                const operand& divisor) -> void override {

        assert(operation == arithmetic_operator::divide or
               operation == arithmetic_operator::remainder);

        reserve_named_register(src_loc_tk, indent, "rax", default_type());
        mov(src_loc_tk, indent, qword_register("rax"), dst);

        reserve_named_register(src_loc_tk, indent, "rdx", default_type());
        assembler_.instruction(indent, op::cqo);

        emit_signed_divide(src_loc_tk, indent, divisor);

        mov(src_loc_tk, indent, dst,
            qword_register(operation == arithmetic_operator::divide ? "rax"
                                                                    : "rdx"));

        release_named_register(src_loc_tk, indent, "rdx");
        release_named_register(src_loc_tk, indent, "rax");
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        if (not with_line) {
            assembler_.label(0, bounds_failure_handler_label);
            emit_panic_exit();
            return;
        }

        // one stub per line sets rbp and enters the handler
        for (const size_t line : bounds_panic_lines_) {
            assembler_.label(0, bounds_panic_label(line));
            assembler_.instruction(1, op::mov, "rbp", line);
            assembler_.jmp(1, bounds_failure_handler_label);
        }

        assembler_.label(0, bounds_failure_handler_label);
        emit_panic_message("msg_panic");

        constexpr int newline{10};
        constexpr int decimal_base{10};
        // the digits of a 64-bit number and a newline
        constexpr size_t number_buffer_size_bytes{21};

        assembler_.comment(1, "line number is in `rbp`");
        assembler_.instruction(1, op::mov, "rax", "rbp");
        assembler_.comment(1, "convert to string");

        assembler_.instruction(1, op::mov, "rdi",
                               assembler_x86_64::immediate::of_expression(
                                   "num_buffer + 19", true));

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
        assembler_.instruction(1, op::mov, "rax", syscall_write);
        assembler_.instruction(1, op::mov, "rsi", "rdi");

        assembler_.instruction(1, op::mov, "rdx",
                               assembler_x86_64::immediate::of_expression(
                                   "num_buffer + 20", true));

        assembler_.instruction(1, op::sub, "rdx", "rdi");
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor);
        assembler_.instruction(1, op::syscall);
        emit_panic_exit();
        assembler_.switch_section(section::rodata);
        assembler_.label(0, "msg_panic");
        assembler_.string_data("panic: bounds at line ");
        assembler_.define_length("msg_panic_len", "msg_panic");
        assembler_.switch_section(section::bss);
        assembler_.label(0, "num_buffer");
        assembler_.reserve(number_buffer_size_bytes);
    }

    auto emit_data(const size_t element_size_bytes,
                   const data_initializer& value) -> void override {

        const std::array<assembler_x86_64::data_value, 1> values{
            {
                {
                    .value{value.value},
                    .unary_operations{value.uops},
                },
            },
        };

        assembler_.data(element_size_bytes, values);
    }

    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(data_initializer&)> next)
        -> void override {

        std::vector<assembler_x86_64::data_value> values;
        data_initializer value;
        while (next(value)) {
            values.push_back({
                .value{value.value},
                .unary_operations{value.uops},
            });
        }

        assembler_.data(element_size_bytes, values);
    }

    auto emit_frame_overflow_handler() -> void override {
        assembler_.label(0, frame_overflow_handler_label);
        emit_panic_message("msg_frame_overflow");
        emit_panic_exit();
        assembler_.switch_section(section::rodata);
        assembler_.label(0, "msg_frame_overflow");
        assembler_.string_data("panic: frame overflow\\n");

        assembler_.define_length("msg_frame_overflow_len",
                                 "msg_frame_overflow");

        // the bounds handler may follow and must stay in the code section
        assembler_.switch_section(section::text);
    }

    auto
    emit_most_efficient(const token& src_loc_tk, const size_t indent,
                        const std::function_ref<void()> emit_without_scratch,
                        const std::function_ref<void()> emit_with_scratch)
        -> void override {

        // both versions are buffered to count instructions, even when output
        // is otherwise written as emitted
        assembler_.emit_buffered([&] -> void {
            std::vector<assembler::line> without_scratch{
                assembler_.capture(emit_without_scratch),
            };

            std::vector<assembler::line> with_scratch{
                assembler_.capture(emit_with_scratch),
            };

            const size_t without_count{
                assembler_x86_64::count_instructions(without_scratch),
            };

            const size_t with_count{
                assembler_x86_64::count_instructions(with_scratch),
            };

            comment(src_loc_tk, indent,
                    "instructions without scratch register {}, with {}",
                    without_count, with_count);

            assembler_.append(std::move(
                without_count <= with_count ? without_scratch : with_scratch));
        });
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const data_initializer& value) const
        -> void override {

        assembler_.repeated_data(element_size_bytes, count, value.uops,
                                 value.value);
    }

    auto emit_string_constants(const std::span<const string_constant> strings)
        -> void override {

        assembler_.switch_section(section::rodata);
        for (const string_constant& s : strings) {
            assembler_.label(0, s.label);
            assembler_.string_data(s.text);
        }

        assembler_.switch_section(section::text);
    }

    auto emit_string_data(const std::string_view value) -> void override {
        assembler_.string_data(value);
    }

    auto emit_zero_data(const size_t size_bytes) const -> void override {
        const size_t count{size_bytes / size_byte};
        emit_repeated_data(size_byte, count, {});
    }

    auto end_main() -> void override {
        exit(token{}, 1, immediate(0));
        assembler_.add_separator_newline();
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, qword_register("rdi"), exit_code);

        mov(src_loc_tk, indent, qword_register("rax"), immediate(syscall_exit));

        syscall(indent);
    }

    // asserts register pools are balanced and prints usage stats; called
    // once at the end of the compile pass
    auto finish() -> void override {
        // reserved by 'start', not set in backend testing mode
        if (variables_base_reserved_) {
            release_variables_base();
        }

        finish_output();

        assert(registers_.is_empty());
        assert(registers_.unavailable_mask() == 0);
        assert(not frame_base_reserved_);
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
        return registers_.scratch_count() >= memory_counter_pressure_count;
    }

    [[nodiscard]] auto frame_base_register() const
        -> std::string_view override {

        return "rbx";
    }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void override {

        assert(reg.is_register() and not reg.allocation_register().empty());

        release_named_register(src_loc_tk, indent, reg.allocation_register());
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indent,
                               const operand& reg) -> void override {

        assert(reg.is_register() and not reg.allocation_register().empty());

        comment(src_loc_tk, indent, "free scratch register {}",
                reg.allocation_register());

        assert(not registers_.top().named);

        pop_allocation(reg.allocation_register());
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
        return storage_limits::max_size_bytes;
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
                immediate(size_bytes / size_qword));
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
                  const bool reuse_source = false) -> void override {

        if (multiply_by_constant(src_loc_tk, indent, product, factor)) {
            return;
        }

        if (product.type_ref().size_bytes() == size_byte) {
            const operand left{
                alloc_scratch_register(src_loc_tk, indent, default_type()),
            };

            const operand right{
                alloc_scratch_register(src_loc_tk, indent, default_type()),
            };

            mov(src_loc_tk, indent, left, product);
            mov(src_loc_tk, indent, right, factor);
            imul(src_loc_tk, indent, left, right);
            mov(src_loc_tk, indent, product, left);
            free_scratch_register(src_loc_tk, indent, right);
            free_scratch_register(src_loc_tk, indent, left);

            return;
        }

        if (product.is_register()) {
            imul(src_loc_tk, indent, product, factor);
            return;
        }

        if (reuse_source) {
            imul(src_loc_tk, indent, factor, product);
            mov(src_loc_tk, indent, product, factor);
            return;
        }

        const operand reg{
            alloc_scratch_register(
                src_loc_tk, indent,
                builtin_type_for_size_bytes(product.type_ref().size_bytes())),
        };

        mov(src_loc_tk, indent, reg, product);
        imul(src_loc_tk, indent, reg, factor);
        mov(src_loc_tk, indent, product, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto read(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& descriptor, const operand& address,
              const operand& count) -> void override {

        io_syscall(src_loc_tk, indent, dst, descriptor, address, count,
                   syscall_read);
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

        return {
            .arguments{io_args},
            .result{"rax"},
        };
    }

    auto release_frame_base() -> void override {
        assert(frame_base_reserved_);
        assert(registers_.scratch_count() == 0);

        pop_allocation(frame_base_register());

        frame_base_reserved_ = false;
    }

    auto release_variables_base() -> void override {
        assert(variables_base_reserved_);

        release_named_register(token{}, 0, variables_base_register_);

        variables_base_reserved_ = false;
    }

    auto reserve_frame_base() -> void override {
        assert(not frame_base_reserved_);
        assert(registers_.scratch_count() == 0);

        push_allocation(token{}, frame_base_register(), default_type(), true);

        frame_base_reserved_ = true;
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

    auto reserve_variables_base() -> void override {
        assert(not variables_base_reserved_);

        reserve_named_register(token{}, 0, variables_base_register_,
                               default_type());

        variables_base_reserved_ = true;
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

    [[nodiscard]] auto scratch_register_total() const -> size_t override {
        return scratch_registers_.size();
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const operand& count) -> void override {

        assert(operation == arithmetic_operator::shift_left or
               operation == arithmetic_operator::shift_right);

        const op code{
            operation == arithmetic_operator::shift_left ? op::sal : op::sar,
        };

        if (count.is_immediate()) {
            emit_op(src_loc_tk, indent, code, dst,
                    shift_count_immediate(count));

            return;
        }

        validate_shift_operand(src_loc_tk, count);
        reserve_named_register(src_loc_tk, indent, "rcx", default_type());

        mov(src_loc_tk, indent,
            sized_register("rcx", dst.type_ref().size_bytes()), count);

        emit_op(src_loc_tk, indent, code, dst,
                sized_register("rcx", size_byte));

        release_named_register(src_loc_tk, indent, "rcx");
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
               const arithmetic_operator operation, const operand& dst)
        -> void override {

        if (operation == arithmetic_operator::complement) {
            not_op(src_loc_tk, indent, dst);
            return;
        }

        assert(operation == arithmetic_operator::negate);

        neg(src_loc_tk, indent, dst);
    }

    auto validate_data_element_size(
        [[maybe_unused]] const token& src_loc_tk,
        [[maybe_unused]] const size_t element_size_bytes) const
        -> void override {}

    auto
    validate_division_operand([[maybe_unused]] const token& src_loc_tk,
                              [[maybe_unused]] const operand& divisor) const
        -> void override {

        // only a loop counter held in a register could be 'rdx' or 'rax', the
        // last scratch registers, and a loop counts in memory before that
        assert(
            not(divisor.is_register() and (divisor.base_register() == "rdx" or
                                           divisor.base_register() == "rax")));
    }

    auto validate_shift_operand([[maybe_unused]] const token& src_loc_tk,
                                [[maybe_unused]] const operand& count) const
        -> void override {

        // only a loop counter held in a register could be 'rcx', the last
        // scratch register, and a loop counts in memory before that
        assert(not(count.is_register() and count.base_register() == "rcx"));
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
                   syscall_write);
    }

    auto write_assembly(std::ostream& os) -> void override {
        assembler_.write(os);
        assembler_.set_direct_output(&stream());
    }

    auto zero(const token& src_loc_tk, const size_t indent, const operand& dst,
              const size_t size_bytes, [[maybe_unused]] const size_t alignment)
        -> void override {

        if (size_bytes > threshold_for_rep_stos_size_bytes) {
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
                threshold_for_rep_stos_size_bytes);

        for_each_part(
            size_bytes, size_qword,
            [&](const size_t part_size_bytes, const size_t offset) -> void {
                mov(src_loc_tk, indent,
                    memory_part(dst, part_size_bytes, offset), immediate(0));
            });
    }

    //
    // class methods
    //

    [[nodiscard]] auto
    allocated_register_type(const std::string_view name) const -> const type* {

        const size_t size_bytes{register_size_bytes(name)};

        assert(size_bytes != 0);

        const size_t index{register_index(name)};

        for (const register_pool::allocation& allocated :
             registers_.allocations()) {

            if (allocated.index == index) {
                return allocated.type_ptr->size_bytes() == size_bytes
                           ? allocated.type_ptr
                           : &builtin_type_for_size_bytes(size_bytes);
            }
        }

        return nullptr;
    }

    auto check_lower_bound(const token& src_loc_tk, const size_t indent,
                           const operand& reg_to_check,
                           const operand& reg_count, const bounds_plan& plan,
                           const std::optional<size_t> reported_line) -> void {

        comment_lower_bound(src_loc_tk, indent, reg_to_check, reg_count, plan);

        if (not plan.upper_covers_lower) {
            test_sign(src_loc_tk, indent, reg_to_check, reported_line);

            // a negative count passes 'start + count' but spans the address
            // space
            if (not plan.count_known) {
                test_sign(src_loc_tk, indent, reg_count, reported_line);
            }
        }

        registers_.mark_lower_checked(
            register_bit(lower_checked_register(reg_to_check, reg_count)));
    }

    auto invoke_syscall(const size_t indent) -> void {
        std::vector<operand> saved;
        for (const std::string_view name : {"rcx", "r11"}) {
            if (allocated_register_type(name) != nullptr) {
                saved.push_back(qword_register(name));
                push(indent, saved.back());
            }
        }

        syscall(indent);
        for (const operand& reg : saved | std::views::reverse) {
            pop(indent, reg);
        }
    }

    // a negative value is out of bounds, an empty operand has none to test
    auto test_sign(const token& src_loc_tk, const size_t indent,
                   const operand& value,
                   const std::optional<size_t> reported_line) -> void {

        if (value.is_empty()) {
            return;
        }

        test(src_loc_tk, indent, value, value);
        branch_to_bounds_panic(indent, condition::s, reported_line);
    }

    //
    // statics
    //

    // the unsigned upper comparison also fails a negative value when the lower
    // bound is covered by it
    [[nodiscard]] static auto out_of_bounds_condition(const bool allow_end,
                                                      const bounds_plan& plan)
        -> condition {

        if (plan.upper_covers_lower) {
            return allow_end ? condition::a : condition::ae;
        }

        return allow_end ? condition::g : condition::ge;
    }

    // returns 0 if name is not a register
    [[nodiscard]] static auto register_size_bytes(const std::string_view name)
        -> size_t {

        for (const register_names& names : register_names_) {
            if (name == names.qword) {
                return size_qword;
            }

            if (name == names.dword) {
                return size_dword;
            }

            if (name == names.word) {
                return size_word;
            }

            if (name == names.byte) {
                return size_byte;
            }
        }

        return 0;
    }

    // the most that a value of the width of 'value' holds as a signed number
    [[nodiscard]] static auto signed_maximum(const operand& value) -> uint64_t {
        return (uint64_t{1} << (value.type_ref().size_bits() - 1)) - 1;
        // note: -1 gives the signed maximum 2^(bits - 1) - 1
    }

  protected:
    //
    // overridden methods
    //

    [[nodiscard]] auto target_assembler() const -> assembler& override {
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

    auto branch_comparison(const size_t indent,
                           const comparison_operator comparison,
                           const bool inverted, const std::string_view label)
        -> void {

        assembler_.jcc(indent, condition_for_comparison(comparison, inverted),
                       label);
    }

    // a reported line goes through the stub of its line, which sets rbp for
    // the handler, so a passing check keeps rbp and loads nothing
    auto branch_to_bounds_panic(const size_t indent, const condition failed,
                                const std::optional<size_t> line) -> void {

        if (not line.has_value()) {
            assembler_.jcc(indent, failed, bounds_failure_handler_label);
            return;
        }

        bounds_panic_lines_.insert(*line);
        assembler_.jcc(indent, failed, bounds_panic_label(*line));
    }

    // returns the cached built-in type (i64/i32/i16/i8) matching 'size'
    [[nodiscard]] auto
    builtin_type_for_size_bytes(const size_t size_bytes) const -> const type& {

        switch (size_bytes) {
        case size_qword:
            return builtin_type_i64();

        case size_dword:
            return builtin_type_i32();

        case size_word:
            return builtin_type_i16();

        case size_byte:
            return builtin_type_i8();

        default:
            std::unreachable();
        }
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
                             const operand& reg_to_check,
                             const size_t array_count, const operand& reg_count)
        -> void {

        if (reg_count.is_empty()) {
            cmp_lowered(src_loc_tk, indent, reg_to_check,
                        immediate(array_count));

            return;
        }

        const operand reg_top_idx{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        // index and count registers have the default type
        assert(reg_count.type_ref().size_bytes() == size_qword and
               reg_to_check.type_ref().size_bytes() == size_qword);

        lea(src_loc_tk, indent, reg_top_idx,
            operand::mem(reg_count.base_register(),
                         reg_to_check.base_register(), 1, 0,
                         builtin_type_i64()));

        cmp_lowered(src_loc_tk, indent, reg_top_idx, immediate(array_count));
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
        assembler_.instruction(1, op::mov, "rax", syscall_exit);
        assembler_.instruction(1, op::mov, "rdi", panic_exit_code);
        assembler_.instruction(1, op::syscall);
    }

    // shared by the panic handlers, which run after all allocations
    auto emit_panic_message(const std::string_view message_label) -> void {
        assembler_.comment(1, "print message to stderr");
        assembler_.instruction(1, op::mov, "rax", syscall_write);
        assembler_.instruction(1, op::mov, "rdi", stderr_descriptor);

        assembler_.instruction(
            1, op::lea, "rsi",
            assembler_x86_64::memory::of_symbol(message_label));

        assembler_.instruction(1, op::mov, "rdx",
                               assembler_x86_64::immediate::of_expression(
                                   std::format("{}_len", message_label)));

        assembler_.instruction(1, op::syscall);
    }

    // idiv takes the divisor as a qword register or memory operand
    auto emit_signed_divide(const token& src_loc_tk, const size_t indent,
                            const operand& divisor) -> void {

        if (not divisor.is_immediate() and
            divisor.type_ref().size_bytes() == size_qword) {

            idiv(src_loc_tk, indent, divisor);
            return;
        }

        const operand scratch_reg{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        mov(src_loc_tk, indent, scratch_reg, divisor);
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
        return operand::imm(std::format("{}", value), default_type());
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
                   value->type_ref().size_bytes() == size_qword);
        }

        assert(dst.base_register() == "rax");
        assert(descriptor.base_register() == "rdi");
        assert(address.base_register() == "rsi");
        assert(count.base_register() == "rdx");

        mov(src_loc_tk, indent, dst, immediate(syscall_number));
        invoke_syscall(indent);
    }

    auto lea(const token& src_loc_tk, const size_t indent, const operand& dst,
             const operand& address, const bool explicit_displacement = false)
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

    [[nodiscard]] auto lower_address(const token& src_loc_tk,
                                     const size_t indent, const operand& value,
                                     std::vector<operand>& registers)
        -> operand {

        if (address_is_encodable(value)) {
            return value;
        }

        const operand address{
            alloc_scratch_register(src_loc_tk, indent, builtin_type_i64()),
        };

        registers.push_back(address);
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
    [[nodiscard]] auto multiply_by_constant(const token& src_loc_tk,
                                            const size_t indent,
                                            const operand& product,
                                            const operand& factor) -> bool {

        const std::optional<uint64_t> bits{immediate_bits(factor)};

        if (not bits) {
            return false;
        }

        const size_t width_bits{product.type_ref().size_bits()};

        const uint64_t mask{
            width_bits >= std::numeric_limits<uint64_t>::digits
                ? std::numeric_limits<uint64_t>::max()
                : (uint64_t{1} << width_bits) - 1,
        };
        // note: -1 makes a mask of 'width_bits' ones

        const uint64_t multiplier{*bits & mask};

        // 'xor' is the shorter idiom but cannot target memory
        if (multiplier == 0 and product.is_register()) {
            xor_op(src_loc_tk, indent, product, product);
            return true;
        }

        if (multiplier == 0) {
            mov(src_loc_tk, indent, product, immediate(0));
            return true;
        }

        if (multiplier == 1) {
            return true;
        }

        // all low bits set is multiplication by minus one at this width
        if (multiplier == mask) {
            neg(src_loc_tk, indent, product);
            return true;
        }

        if (not std::has_single_bit(multiplier)) {
            return false;
        }

        emit_op(src_loc_tk, indent, op::sal, product,
                immediate(std::countr_zero(multiplier)));

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
        assert(dst.is_register() and dst.type_ref().size_bytes() == size_qword);

        assembler_.instruction(indent, op::pop, to_argument(dst));
    }

    // registers are released in reverse order of allocation
    auto pop_allocation(const std::string_view reg) -> void {
        registers_.pop(register_index(reg));
    }

    auto push(const size_t indent, const operand& src) -> void {
        assert(src.is_register() and src.type_ref().size_bytes() == size_qword);

        assembler_.instruction(indent, op::push, to_argument(src));
    }

    auto push_allocation(const token& src_loc_tk, const std::string_view reg,
                         const type& type_ref, const bool named) -> void {

        registers_.push({
            .index{register_index(reg)},
            .src_loc_tk{src_loc_tk},
            .indent{},
            .type_ptr{&type_ref},
            .named{named},
            .frame{current_call_frame()},
        });

        record_register_use(registers_);
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

    auto release_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view reg) -> void {

        comment(src_loc_tk, indent, "free named register {}", reg);

        assert(registers_.top().named);

        pop_allocation(reg);
    }

    auto reserve_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view reg,
                                const type& type_ref) -> void {

        comment(src_loc_tk, indent, "allocate named register {}", reg);

        if (not registers_.is_unavailable(register_bit(reg))) {
            push_allocation(src_loc_tk, reg, type_ref, true);
            return;
        }

        throw_register_in_use(src_loc_tk, reg);
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

    // human-readable "line:col" for a token, using the cached source text
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {

        assert(src_loc_tk.at_line() != 0);

        const auto [line, col]{
            line_and_col_num_for_char_index(src_loc_tk.at_line(),
                                            src_loc_tk.start_index(), source()),
        };

        return std::format("{}:{}", line, col);
    }

    auto store_comparison(const token& src_loc_tk, const size_t indent,
                          const comparison_operator comparison,
                          const bool inverted, const operand& dst) -> void {

        if (dst.is_memory()) {
            setcc(src_loc_tk, indent,
                  condition_for_comparison(comparison, inverted),
                  sized_memory(dst, size_byte));

            return;
        }

        setcc(src_loc_tk, indent,
              condition_for_comparison(comparison, inverted), dst);
    }

    auto store_equal_result(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const bool inverted) -> void {

        const condition cc{inverted ? condition::ne : condition::e};

        if (dst.is_register()) {
            setcc(src_loc_tk, indent, cc, sized_register(dst, size_byte));
            return;
        }

        setcc(src_loc_tk, indent, cc, sized_memory(dst, size_byte));
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

    [[noreturn]] auto throw_register_in_use(const token& src_loc_tk,
                                            const std::string_view reg) const
        -> void {

        const std::span<const register_pool::allocation> allocations{
            registers_.allocations(),
        };

        const auto holder{
            std::ranges::find(allocations, register_index(reg),
                              &register_pool::allocation::index),
        };

        // operands are protected only while lowering a single instruction
        assert(holder != allocations.end());

        // a register named by the backend itself has no place in the source
        const std::string holder_location{
            holder->src_loc_tk.at_line() == 0
                ? std::string{}
                : source_location_hr(holder->src_loc_tk),
        };

        // the last resort scratch registers are also needed by instructions
        if (not holder->named) {
            throw register_error(
                src_loc_tk,
                std::format("cannot allocate register {} because it holds a "
                            "scratch value allocated at {}. try to reduce "
                            "expression complexity",
                            reg, holder_location),
                registers_);
        }

        throw register_error(
            src_loc_tk,
            std::format("cannot allocate register {} because it was allocated "
                        "at {}",
                        reg, holder_location),
            registers_);
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

        const uint32_t saved_unavailable{registers_.unavailable_mask()};

        // registers the operands refer to must not be picked for lowering
        for (const operand* value : {&dst, &src}) {
            if (value->is_register() or value->is_memory()) {
                registers_.protect(register_bit(value->base_register()));
            }

            if (value->is_memory()) {
                registers_.protect(register_bit(value->index_register()));
            }
        }

        std::vector<operand> registers;

        const operand lowered_dst{
            lower_address(src_loc_tk, indent, dst, registers),
        };

        const operand lowered_src{
            lower_address(src_loc_tk, indent, src, registers),
        };

        emit(lowered_dst, lowered_src);
        free_scratch_registers(src_loc_tk, indent, registers);
        registers_.restore_unavailable(saved_unavailable);
    }

    auto xor_op(const token& src_loc_tk, const size_t indent,
                const operand& dst, const operand& src) -> void {

        emit_binary(src_loc_tk, indent, op::xor_op, dst, src);
    }

    //
    // statics
    //

    [[nodiscard]] static auto bounds_panic_label(const size_t line)
        -> std::string {

        return std::format("baz_bounds_line_{}", line);
    }

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

    // the qwords, then the remaining dword, word and byte
    [[nodiscard]] static auto memory_equal_compares(const size_t size_bytes)
        -> std::vector<op> {

        std::vector<op> compares;
        size_t remaining_bytes{size_bytes};

        if (size_bytes / size_qword > threshold_for_repe_cmpsq_count) {
            compares.push_back(op::repe_cmpsq);
            remaining_bytes %= size_qword;
        }

        for_each_part(remaining_bytes, size_qword,
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

        if (dst.type_ref().size_bytes() != size_qword) {
            return false;
        }

        if (code == op::mov and dst.is_register()) {
            return false;
        }

        const std::optional<uint64_t> bits{immediate_bits(src)};

        // symbolic immediates such as frame sizes do not reach here
        assert(bits);

        return not std::in_range<int32_t>(std::bit_cast<int64_t>(*bits));
    }

    [[nodiscard]] static auto register_bit(const std::string_view name)
        -> uint16_t {

        if (register_size_bytes(name) == 0) {
            return 0;
        }

        const std::string qword{sized_register_name(name, size_qword)};

        for (const auto [index, names] :
             std::views::enumerate(register_names_)) {

            if (names.qword == qword) {
                return static_cast<uint16_t>(1U << static_cast<size_t>(index));
            }
        }

        std::unreachable();
    }

    // bit in the mask of 'registers_' for any size alias of a register, 0 for
    // other text such as labels
    // the index in 'register_names_' of a register of any size
    [[nodiscard]] static auto register_index(const std::string_view name)
        -> size_t {

        const uint16_t bit{register_bit(name)};

        assert(bit != 0);

        return static_cast<size_t>(std::countr_zero(bit));
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

    // an index without a base and with a scale of 1 is the base, e.g. '[r15 *
    // 1]' is the address '[r15]'
    [[nodiscard]] static auto registers_of(const operand& memory)
        -> address_registers {

        if (memory.base_register().empty() and memory.scale() <= 1) {
            return {.base{memory.index_register()}, .index{}};
        }

        return {.base{memory.base_register()}, .index{memory.index_register()}};
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

        const address_registers lhs_registers{registers_of(lhs)};
        const address_registers rhs_registers{registers_of(rhs)};

        return lhs.type_ref().size_bytes() == rhs.type_ref().size_bytes() and
               lhs_registers.base == rhs_registers.base and
               lhs_registers.index == rhs_registers.index and
               lhs.displacement() == rhs.displacement() and
               (lhs_registers.index.empty() or
                std::max(lhs.scale(), uint64_t{1}) ==
                    std::max(rhs.scale(), uint64_t{1}));
    }

    // the hardware keeps only the low bits of a shift count and nasm warns
    // about a negative one as a signed byte, so it is written as an unsigned
    // byte
    [[nodiscard]] static auto shift_count_immediate(const operand& count)
        -> operand {

        const std::string& text{count.immediate()};

        // note: at most 20 characters, '-' and the 19 digits of an int64_t
        const bool is_negative_number{
            text.size() > 1 and text.size() <= 20 and text.front() == '-' and
                std::ranges::all_of(
                    text | std::views::drop(1),
                    [](const char c) -> bool { return c >= '0' and c <= '9'; }),
        };

        if (not is_negative_number) {
            return count;
        }

        const int64_t value{std::stoll(text)};
        const uint8_t low_byte{static_cast<uint8_t>(value)};

        return operand::imm(std::format("{}", low_byte), count.type_ref());
    }

    [[nodiscard]] static auto sized_register_name(const std::string_view name,
                                                  const size_t size_bytes)
        -> std::string {

        // map canonical 64-bit register names to size-specific aliases
        for (const register_names& names : register_names_) {
            if (name != names.qword and name != names.dword and
                name != names.word and name != names.byte) {
                continue;
            }

            switch (size_bytes) {
            case size_qword:
                return std::string{names.qword};

            case size_dword:
                return std::string{names.dword};

            case size_word:
                return std::string{names.word};

            case size_byte:
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
        case size_qword:
            return op::cmpsq;

        case size_dword:
            return op::cmpsd;

        case size_word:
            return op::cmpsw;

        case size_byte:
            return op::cmpsb;

        default:
            std::unreachable();
        }
    }

    // the address without a width, as 'lea' and comments take it
    [[nodiscard]] static auto
    to_address(const operand& value, const bool explicit_displacement = false)
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
            return assembler_x86_64::immediate::of_expression(
                value.immediate());
        }

        if (value.is_register()) {
            return std::string_view{value.base_register()};
        }

        assembler_x86_64::memory address{to_address(value)};
        address.size_bytes = value.type_ref().size_bytes();

        return address;
    }
};
