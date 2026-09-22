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
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "machine.hpp"
#include "token.hpp"
#include "type.hpp"

class token;
class type;

class machine_x86 final : public machine {
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    static constexpr size_t data_alignment_{16};

    static constexpr std::string_view data_qword{"dq"};
    static constexpr std::string_view data_dword{"dd"};
    static constexpr std::string_view data_word{"dw"};
    static constexpr std::string_view data_byte{"db"};

    static constexpr std::array<size_t, 4> index_register_scalings{1, 2, 4, 8};

    static constexpr size_t threshold_for_rep_stos_size_bytes{32};
    static constexpr size_t threshold_for_rep_movs_size_bytes{16};
    static constexpr int syscall_exit{60};

    struct register_names {
        std::string_view qword;
        std::string_view dword;
        std::string_view word;
        std::string_view byte;
    };

    static constexpr std::array<register_names, 16> register_names_{{
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
    }};

    struct allocated_register {
        std::string source_location;
        std::string name;
        const type* type_ptr{};
    };

    std::vector<std::string> all_registers_{
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};

    size_t all_registers_initial_count_{all_registers_.size()};
    std::vector<std::string> named_registers_{"rax", "rbx", "rcx", "rdx",
                                              "rsi", "rdi", "rbp", "rsp"};

    size_t named_registers_initial_count_{named_registers_.size()};
    std::vector<std::string> scratch_registers_{"r8",  "r9",  "r10", "r11",
                                                "r12", "r13", "r14", "r15"};

    size_t scratch_registers_initial_count_{scratch_registers_.size()};
    bool frame_base_reserved_{};
    size_t frame_base_pool_index_{};
    std::vector<allocated_register> allocated_registers_;
    size_t usage_max_scratch_regs_{};

    std::string_view source_;

    const type* default_type_{};
    const type* type_bool_{};
    const type* type_i64_{};
    const type* type_i32_{};
    const type* type_i16_{};
    const type* type_i8_{};
    const type* type_void_{};

    std::reference_wrapper<std::ostream> os_;

  public:
    explicit machine_x86(std::ostream& os_ref, const std::string_view source)
        : source_{source}, os_{os_ref} {}

    [[nodiscard]] auto default_type() const -> const type& override {
        assert(default_type_);

        return *default_type_;
    }

    auto set_builtin_types(const type& t_i64, const type& t_i32,
                           const type& t_i16, const type& t_i8,
                           const type& t_bool, const type& t_void)
        -> void override {

        default_type_ = &t_i64;
        type_i64_ = &t_i64;
        type_i32_ = &t_i32;
        type_i16_ = &t_i16;
        type_i8_ = &t_i8;
        type_bool_ = &t_bool;
        type_void_ = &t_void;
    }

    // redirects output to 'new_stream', returning the previously used stream
    // so the caller can restore it later
    auto use_stream(std::ostream& new_stream) -> std::ostream& override {
        std::ostream& prev{os_.get()};
        os_ = new_stream;

        return prev;
    }

    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void override {

        if (src_loc_tk.at_line() == 0) {
            comment_indent(indent);
            print(" ");
        } else {
            comment_start(src_loc_tk, indent);
        }
        println("{}", text);
    }

    using machine::comment;

    auto emit_most_efficient(const token& src_loc_tk, const size_t indent,
                             const std::string_view without_scratch,
                             const std::string_view with_scratch)
        -> void override {

        const size_t without_count{count_instructions(without_scratch)};
        const size_t with_count{count_instructions(with_scratch)};

        comment(src_loc_tk, indent,
                "instructions without scratch register {}, with {}",
                without_count, with_count);

        if (without_count <= with_count) {
            emit_buffer(without_scratch);
        } else {
            emit_buffer(with_scratch);
        }
    }

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref)
        -> operand override {

        if (scratch_registers_.empty()) {
            throw compiler_exception{src_loc_tk,
                                     "out of scratch registers. try to reduce "
                                     "expression complexity"};
        }

        std::string register_name{std::move(scratch_registers_.back())};
        scratch_registers_.pop_back();

        comment(src_loc_tk, indent, "allocate scratch register -> {}",
                register_name);

        const size_t n{scratch_registers_initial_count_ -
                       scratch_registers_.size()};

        usage_max_scratch_regs_ = std::max(n, usage_max_scratch_regs_);

        allocated_registers_.emplace_back(source_location_hr(src_loc_tk),
                                          std::move(register_name), &type_ref);

        const std::string& allocated_name{allocated_registers_.back().name};
        operand result{reg(allocated_name, type_ref)};
        result.set_allocation_register(allocated_name);

        return result;
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand override {

        reserve_named_register(src_loc_tk, indent, register_name, type_ref);
        operand result{reg(register_name, type_ref)};
        result.set_allocation_register(register_name);

        return result;
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

        assert(allocated_registers_.back().name == reg.allocation_register());

        scratch_registers_.emplace_back(
            std::move(allocated_registers_.back().name));

        allocated_registers_.pop_back();
    }

    // asserts register pools are balanced and prints usage stats; called
    // once at the end of the compile pass
    auto finish() -> void override {
        println("\n; max scratch registers in use: {}",
                usage_max_scratch_regs_);

        assert(all_registers_.size() == all_registers_initial_count_);
        assert(allocated_registers_.empty());
        assert(named_registers_.size() == named_registers_initial_count_);
        assert(scratch_registers_.size() == scratch_registers_initial_count_);
        assert(not frame_base_reserved_);

        usage_max_scratch_regs_ = 0;
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        assert(dst.is_register() or dst.is_memory());

        mov(src_loc_tk, indent, dst, src);
    }

    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t size_bytes,
                          const operand& address) -> void override {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, size_bytes,
                format_address(address));
    }

    auto comment_alias(const token& src_loc_tk, const size_t indent,
                       const std::string_view from, const std::string_view to,
                       const operand& address) -> void override {

        if (address.is_empty()) {
            comment(src_loc_tk, indent, "alias {} -> {}", from, to);
        } else {
            comment(src_loc_tk, indent, "alias {} -> {} (lea: {})", from, to,
                    format_address(address));
        }
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
            store_comparison(indent, action.operation, action.inverted,
                             action.destination);
        }

        branch_comparison(indent, action.operation,
                          action.branch_on_true ? action.inverted
                                                : not action.inverted,
                          action.target);
    }

    auto branch(const size_t indent, const std::string_view target)
        -> void override {
        jmp(indent, target);
    }

    auto invoke_syscall(const size_t indent) -> void override {
        const operand saved_register{reg("r11", *default_type_)};
        // note: syscall clobbers r11

        push(indent, saved_register);
        syscall(indent);
        pop(indent, saved_register);
    }

    auto advance_array_iteration(const size_t indent, const operand& iterator,
                                 const operand& counter,
                                 const size_t element_size_bytes,
                                 const size_t array_count,
                                 const std::string_view loop_label)
        -> void override {

        add(indent, iterator, immediate(element_size_bytes));
        inc(indent, counter);
        cmp(indent, counter, immediate(array_count));
        jne(indent, loop_label);
    }

    auto copy(const token& src_loc_tk, const size_t indent, const operand& src,
              const operand& dst, const size_t size_bytes) -> void override {

        if (size_bytes > threshold_for_rep_movs_size_bytes) {
            reserve_named_register(src_loc_tk, indent, "rsi", *default_type_);
            reserve_named_register(src_loc_tk, indent, "rdi", *default_type_);
            reserve_named_register(src_loc_tk, indent, "rcx", *default_type_);

            lea(indent, machine_x86::reg("rsi", *type_i64_), src);
            lea(indent, machine_x86::reg("rdi", *type_i64_), dst);
            mov(src_loc_tk, indent, machine_x86::reg("rcx", *type_i64_),
                immediate(size_bytes));

            rep_movs(indent, 'b');

            release_bulk_registers(src_loc_tk, indent);

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_movs_size_bytes);

        reserve_named_register(src_loc_tk, indent, "rax", *default_type_);
        size_t remaining_size_bytes{size_bytes};
        operand src_operand{src};
        operand dst_operand{dst};
        for (size_t width_size_bytes{size_qword}; width_size_bytes >= size_byte;
             width_size_bytes /= 2) {

            while (remaining_size_bytes >= width_size_bytes) {
                const operand reg{sized_register("rax", width_size_bytes)};
                mov(src_loc_tk, indent, reg,
                    sized_memory(src_operand, width_size_bytes));
                mov(src_loc_tk, indent,
                    sized_memory(dst_operand, width_size_bytes), reg);

                remaining_size_bytes -= width_size_bytes;
                if (remaining_size_bytes != 0) {
                    src_operand.increment_offset(
                        static_cast<int32_t>(width_size_bytes));
                    dst_operand.increment_offset(
                        static_cast<int32_t>(width_size_bytes));
                }
            }
        }
        release_named_register(src_loc_tk, indent, "rax");
    }

    [[nodiscard]] auto begin_array_copy(const token& src_loc_tk,
                                        const size_t indent)
        -> operand override {

        return alloc_bulk_registers(src_loc_tk, indent);
    }

    auto set_array_copy_source(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rsi", *type_i64_), address);
    }

    auto set_array_copy_destination(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rdi", *type_i64_), address);
    }

    auto end_array_copy(const token& src_loc_tk, const size_t indent,
                        const size_t element_size_bytes) -> void override {

        scale_by_element_size_bytes(src_loc_tk, indent,
                                    machine_x86::reg("rcx", *type_i64_),
                                    element_size_bytes);

        rep_movs(indent, 'b');
        release_bulk_registers(src_loc_tk, indent);
    }

    auto begin_memory_equal(const token& src_loc_tk, const size_t indent)
        -> operand override {

        return alloc_bulk_registers(src_loc_tk, indent);
    }

    auto set_memory_equal_left(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rsi", *type_i64_), address);
    }

    auto set_memory_equal_right(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rdi", *type_i64_), address);
    }

    auto end_memory_equal(const token& src_loc_tk, const size_t indent,
                          const size_t size_bytes, const operand& dst)
        -> void override {

        char size_suffix{'b'};
        size_t count{size_bytes};
        if ((count % size_qword) == 0) {
            size_suffix = 'q';
            count /= size_qword;
        } else if ((count % size_dword) == 0) {
            size_suffix = 'd';
            count /= size_dword;
        } else if ((count % size_word) == 0) {
            size_suffix = 'w';
            count /= size_word;
        }
        mov(src_loc_tk, indent, machine_x86::reg("rcx", *type_i64_),
            immediate(count));
        repe_cmps(indent, size_suffix);
        release_bulk_registers(src_loc_tk, indent);
        store_equal_result(indent, dst);
    }

    auto end_arrays_equal(const token& src_loc_tk, const size_t indent,
                          const size_t element_size_bytes, const operand& dst)
        -> void override {

        scale_by_element_size_bytes(src_loc_tk, indent,
                                    machine_x86::reg("rcx", *type_i64_),
                                    element_size_bytes);

        repe_cmps(indent, 'b');
        release_bulk_registers(src_loc_tk, indent);
        store_equal_result(indent, dst);
    }

    auto zero(const token& src_loc_tk, const size_t indent, const operand& dst,
              const size_t size_bytes) -> void override {

        if (size_bytes > threshold_for_rep_stos_size_bytes) {
            reserve_named_register(src_loc_tk, indent, "rax", *default_type_);
            reserve_named_register(src_loc_tk, indent, "rdi", *default_type_);
            reserve_named_register(src_loc_tk, indent, "rcx", *default_type_);
            xor_op(indent, machine_x86::reg("al", *type_i8_),
                   machine_x86::reg("al", *type_i8_));
            lea(indent, machine_x86::reg("rdi", *type_i64_), dst);
            mov(src_loc_tk, indent, machine_x86::reg("rcx", *type_i64_),
                immediate(size_bytes));
            rep_stos(indent, 'b');
            release_named_register(src_loc_tk, indent, "rcx");
            release_named_register(src_loc_tk, indent, "rdi");
            release_named_register(src_loc_tk, indent, "rax");

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_stos_size_bytes);

        size_t remaining_size_bytes{size_bytes};
        operand dst_operand{dst};
        for (size_t width_size_bytes{size_qword}; width_size_bytes >= size_byte;
             width_size_bytes /= 2) {

            while (remaining_size_bytes >= width_size_bytes) {
                mov(src_loc_tk, indent,
                    sized_memory(dst_operand, width_size_bytes), immediate(0));

                remaining_size_bytes -= width_size_bytes;
                if (remaining_size_bytes != 0) {
                    dst_operand.increment_offset(
                        static_cast<int32_t>(width_size_bytes));
                }
            }
        }
    }

    auto add_subtract(const token& src_loc_tk, const size_t indent,
                      const char operation, const operand& dst,
                      const operand& src) -> void override {

        assert(operation == '+' or operation == '-');

        op(src_loc_tk, indent, operation == '+' ? "add" : "sub", dst, src);
    }

    auto bitwise(const token& src_loc_tk, const size_t indent,
                 const char operation, const operand& dst, const operand& src)
        -> void override {

        switch (operation) {
        case '&':
            op(src_loc_tk, indent, "and", dst, src);

            return;

        case '|':
            op(src_loc_tk, indent, "or", dst, src);

            return;

        case '^':
            op(src_loc_tk, indent, "xor", dst, src);

            return;

        default:
            std::unreachable();
        }
    }

    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  const bool reuse_source = false) -> void override {

        if (product.type_ref().size_bytes() == size_byte) {
            const operand left{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};

            const operand right{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};

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

        const operand reg{alloc_scratch_register(
            src_loc_tk, indent,
            builtin_type_for_size_bytes(product.type_ref().size_bytes()))};

        mov(src_loc_tk, indent, reg, product);
        imul(src_loc_tk, indent, reg, factor);
        mov(src_loc_tk, indent, product, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto validate_shift_operand(const token& src_loc_tk,
                                const operand& count) const -> void override {

        if (count.is_register() and count.base_register() == "rcx") {
            throw compiler_exception{
                src_loc_tk, "cannot use 'rcx' as a shift operand; it is "
                            "reserved for the count"};
        }
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const char operation, const operand& dst, const operand& count)
        -> void override {

        assert(operation == '<' or operation == '>');

        if (count.is_immediate()) {
            op(src_loc_tk, indent, operation == '<' ? "sal" : "sar", dst,
               count);

            return;
        }

        validate_shift_operand(src_loc_tk, count);
        reserve_named_register(src_loc_tk, indent, "rcx", *default_type_);
        mov(src_loc_tk, indent,
            sized_register("rcx", dst.type_ref().size_bytes()), count);
        op(src_loc_tk, indent, operation == '<' ? "sal" : "sar", dst,
           sized_register("rcx", size_byte));
        release_named_register(src_loc_tk, indent, "rcx");
    }

    auto validate_division_operand(const token& src_loc_tk,
                                   const operand& divisor) const
        -> void override {

        if (divisor.is_register() and (divisor.base_register() == "rdx" or
                                       divisor.base_register() == "rax")) {
            throw compiler_exception{
                src_loc_tk, "cannot use 'rdx' or 'rax' for division; they are "
                            "reserved"};
        }
    }

    auto divide(const token& src_loc_tk, const size_t indent,
                const char operation, const operand& dst,
                const operand& divisor) -> void override {

        assert(operation == '/' or operation == '%');

        reserve_named_register(src_loc_tk, indent, "rax", *default_type_);
        mov(src_loc_tk, indent, machine_x86::reg("rax", *type_i64_), dst);

        reserve_named_register(src_loc_tk, indent, "rdx", *default_type_);
        asm_line(indent, "cqo");

        if (divisor.is_immediate() or
            divisor.type_ref().size_bytes() != size_qword) {
            const operand scratch_reg{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};

            mov(src_loc_tk, indent, scratch_reg, divisor);
            idiv(indent, scratch_reg);
            free_scratch_register(src_loc_tk, indent, scratch_reg);
        } else {
            idiv(indent, divisor);
        }

        mov(src_loc_tk, indent, dst,
            machine_x86::reg(operation == '/' ? "rax" : "rdx", *type_i64_));

        release_named_register(src_loc_tk, indent, "rdx");
        release_named_register(src_loc_tk, indent, "rax");
    }

    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void override {

        mov(src_loc_tk, indent, dst, immediate(value ? 1 : 0));
    }

    auto label(const size_t indent, const std::string_view label)
        -> void override {
        asm_line(indent, "{}:", label);
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        if (dst.is_register()) {
            lea(indent, dst, address);

            return;
        }

        const operand reg{
            alloc_scratch_register(src_loc_tk, indent, *default_type_)};

        lea(indent, reg, address);
        mov(src_loc_tk, indent, dst, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto unary(const size_t indent, const char operation, const operand& dst)
        -> void override {

        switch (operation) {
        case '~':
            not_op(indent, dst);

            return;

        case '-':
            neg(indent, dst);

            return;

        default:
            std::unreachable();
        }
    }

    [[nodiscard]] auto can_encode_index_scale(const size_t size_bytes) const
        -> bool override {

        return std::ranges::contains(index_register_scalings, size_bytes);
    }

    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size_bytes)
        -> void override {

        scale_by_element_size_bytes(src_loc_tk, indent, index,
                                    element_size_bytes);
    }

    auto exit_process(const token& src_loc_tk, const size_t indent,
                      const int32_t exit_code) -> void override {

        mov(src_loc_tk, indent, machine_x86::reg("rdi", *type_i64_),
            immediate(exit_code));

        mov(src_loc_tk, indent, machine_x86::reg("rax", *type_i64_),
            immediate(syscall_exit));

        syscall(indent);
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        return "rbp";
    }

    [[nodiscard]] auto is_variables_base(const operand& reg) const
        -> bool override {

        return not reg.is_indexed() and reg.base_register() == "rbp";
    }

    auto address_of_variable(const token& src_loc_tk, const size_t indent,
                             const operand& dst, const int32_t offset,
                             const type& value_type) -> void override {

        const operand address{operand::mem("rbp", {}, 1, offset, value_type)};
        if (dst.is_register()) {
            lea(indent, dst, address, true);

            return;
        }
        const operand reg{
            alloc_scratch_register(src_loc_tk, indent, *default_type_)};

        lea(indent, reg, address, true);
        mov(src_loc_tk, indent, dst, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto reserve_variables_base() -> void override {
        reserve_named_register(token{}, 0, "rbp", *default_type_);
    }

    auto release_variables_base() -> void override {
        release_named_register(token{}, 0, "rbp");
    }

    [[nodiscard]] auto frame_base_register() const
        -> std::string_view override {
        return "rbx";
    }

    auto reserve_frame_base() -> void override {
        assert(not frame_base_reserved_);
        assert(scratch_registers_.size() == scratch_registers_initial_count_);

        const auto position{
            std::ranges::find(named_registers_, frame_base_register())};

        assert(position != named_registers_.end());
        frame_base_pool_index_ =
            static_cast<size_t>(position - named_registers_.begin());

        named_registers_.erase(position);
        allocated_registers_.emplace_back(
            "", std::string{frame_base_register()}, default_type_);

        frame_base_reserved_ = true;
    }

    auto release_frame_base() -> void override {
        assert(frame_base_reserved_);
        assert(scratch_registers_.size() == scratch_registers_initial_count_);

        assert(not allocated_registers_.empty());
        assert(allocated_registers_.back().name == frame_base_register());

        named_registers_.insert(
            named_registers_.begin() +
                static_cast<std::vector<std::string>::difference_type>(
                    frame_base_pool_index_),
            std::move(allocated_registers_.back().name));

        allocated_registers_.pop_back();
        frame_base_reserved_ = false;
    }

    auto call_function(const size_t indent, const std::string_view label,
                       const operand& frame_address) -> void override {
        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(frame_address.base_register() != "rsp");

        asm_line(indent, "PUSH_REGS");
        lea(indent, reg(frame_base_register(), *default_type_), frame_address,
            true);

        asm_line(indent, "call {}", label);
        asm_line(indent, "POP_REGS");
    }

    auto return_function(const size_t indent) -> void override {
        asm_line(indent, "ret");
    }

    auto check_frame_capacity(const token& src_loc_tk, const size_t indent,
                              const operand& frame_address,
                              const operand& frame_size_bytes,
                              const std::string_view failure_label,
                              const bool enabled = {}) -> void override {

        if (not enabled) {
            return;
        }

        assert(frame_address.is_memory());
        assert(frame_size_bytes.is_immediate());

        const operand start{
            alloc_scratch_register(src_loc_tk, indent, *default_type_)};

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, *default_type_)};

        lea(indent, start, frame_address, true);
        lea(indent, remaining, operand::mem("vars", {}, 1, 0, *default_type_));
        cmp(indent, start, remaining);
        jcc(indent, "b", failure_label);
        lea(indent, remaining,
            operand::mem("vars.end", {}, 1, 0, *default_type_));

        cmp(indent, start, remaining);
        jcc(indent, "a", failure_label);
        op(src_loc_tk, indent, "sub", remaining, start);
        mov(src_loc_tk, indent, start, frame_size_bytes);
        cmp(indent, start, remaining);
        jcc(indent, "a", failure_label);
        free_scratch_register(src_loc_tk, indent, remaining);
        free_scratch_register(src_loc_tk, indent, start);
    }

    auto define_constant(const std::string_view name, const size_t value)
        -> void override {
        println("{} equ {}", name, value);
    }

    auto program_start() -> void override {
        println(";\n; generated by baz\n;\n\ndefault rel\n");
        const std::array<std::string_view, 15> saved_registers{
            "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "r8",
            "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};

        println("%macro PUSH_REGS 0");
        for (const std::string_view name : saved_registers) {
            push(1, reg(name, *default_type_));
        }
        println("%endmacro\n");
        println("%macro POP_REGS 0");
        for (const std::string_view name :
             saved_registers | std::views::reverse) {
            pop(1, reg(name, *default_type_));
        }
        println("%endmacro");
        println("\nsection .text\nbits 64\nglobal _start\n_start:\nlea rbp, "
                "[dat]\n\n;\n; "
                "program\n;\n");
    }

    auto program_end() -> void override {
        println("    ; system call: exit 0");
        println("    mov rax, 60");
        println("    mov rdi, 0");
        println("    syscall");
        println();
    }

    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const operand& reg_to_check, const size_t array_count,
                      const bool allow_end, const operand& reg_count,
                      const bounds_check_options& options) -> void override {

        if (not options.upper and not options.lower) {
            return;
        }

        const std::string_view comparison{allow_end ? "g" : "ge"};
        comment(src_loc_tk, indent, "bounds check");

        operand reg_line_num;
        if (options.with_line) {
            reg_line_num =
                alloc_scratch_register(src_loc_tk, indent, *default_type_);

            comment(src_loc_tk, indent, "line number");
            mov(src_loc_tk, indent, reg_line_num,
                immediate(src_loc_tk.at_line()));
        }

        if (options.lower) {
            test(indent, reg_to_check, reg_to_check);
            if (options.with_line) {
                cmovs(indent, machine_x86::reg("rbp", *type_i64_),
                      reg_line_num);
            }
            jcc(indent, "s", "baz_bounds_panic");
        }

        if (options.upper) {
            if (not reg_count.is_empty()) {
                const operand reg_top_idx{
                    alloc_scratch_register(src_loc_tk, indent, *default_type_)};

                mov(src_loc_tk, indent, reg_top_idx, reg_count);
                add(indent, reg_top_idx, reg_to_check);
                cmp(indent, reg_top_idx, immediate(array_count));
                free_scratch_register(src_loc_tk, indent, reg_top_idx);
            } else {
                cmp(indent, reg_to_check, immediate(array_count));
            }
            if (options.with_line) {
                op(src_loc_tk, indent, std::format("cmov{}", comparison),
                   machine_x86::reg("rbp", *type_i64_), reg_line_num);
            }
            jcc(indent, comparison, "baz_bounds_panic");
        }

        if (options.with_line) {
            free_scratch_register(src_loc_tk, indent, reg_line_num);
        }
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        if (not with_line) {
            println();
            println("baz_bounds_panic:");
            println("    ; system call: exit 255");
            println("    mov rax, 60");
            println("    mov rdi, 255");
            println("    syscall");
        } else {
            println("baz_bounds_panic:");
            println(";   print message to stderr");
            println("    mov rax, 1");
            println("    mov rdi, 2");
            println("    lea rsi, [msg_panic]");
            println("    mov rdx, msg_panic_len");
            println("    syscall");
            println(";   line number is in `rbp`");
            println("    mov rax, rbp");
            println(";   convert to string");
            println("    lea rdi, [num_buffer + 19]");
            println("    mov byte [rdi], 10");
            println("    dec rdi");
            println("    mov rcx, 10");
            println(".convert_loop:");
            println("    xor rdx, rdx");
            println("    div rcx");
            println("    add dl, '0'");
            println("    mov [rdi], dl");
            println("    dec rdi");
            println("    test rax, rax");
            println("    jnz .convert_loop");
            println("    inc rdi");
            println(";   print line number to stderr");
            println("    mov rax, 1");
            println("    mov rsi, rdi");
            println("    lea rdx, [num_buffer + 20]");
            println("    sub rdx, rdi");
            println("    mov rdi, 2");
            println("    syscall");
            println(";   exit with error code 255");
            println("    mov rax, 60");
            println("    mov rdi, 255");
            println("    syscall");
            println("section .rodata");
            println("    msg_panic: db 'panic: bounds at line '");
            println("    msg_panic_len equ $ - msg_panic");
            println("section .bss");
            println("    num_buffer: resb 21");
        }
    }

    [[nodiscard]] auto data_alignment() const -> size_t override {
        return data_alignment_;
    }

    auto begin_data(const size_t alignment) -> void override {
        println("\nsection .data\nalign {}\ndat:", alignment);
    }

    auto reserve_variables(const size_t alignment, const size_t size_bytes)
        -> void override {

        println("dat.end:\n\nsection .bss.vars nobits alloc write\nalign "
                "{}\nvars:\nvars "
                "resb {}\nvars.end:",
                alignment, size_bytes);
    }

    auto emit_data(const size_t element_size_bytes,
                   const data_initializer& value) -> void override {

        print("{} ", get_data_def(element_size_bytes));
        emit_data_value(value);
        println();
    }

    using machine::emit_data_array;

    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(data_initializer&)> next)
        -> void override {

        print("{} ", get_data_def(element_size_bytes));
        bool first{true};
        data_initializer value;
        while (next(value)) {
            if (not first) {
                print(", ");
            }
            emit_data_value(value);
            first = false;
        }
        println();
    }

    auto emit_string_data(const std::string_view value) -> void override {
        print("db `");
        for (const auto [i, p] :
             std::views::enumerate(value | std::views::split('`'))) {

            if (i != 0) {
                print("\\`");
            }

            print("{}", std::string_view{p});
        }
        println("`");
    }

    auto emit_zero_data(const size_t size_bytes) const -> void override {
        const size_t count{size_bytes / size_byte};
        emit_repeated_data(size_byte, count, {});
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const data_initializer& value) const
        -> void override {

        println("times {} {} {}{}", count, get_data_def(element_size_bytes),
                value.uops, value.value);
    }

    // returns 0 if name is not a register
    [[nodiscard]] auto register_size_bytes(const std::string_view name) const
        -> size_t override {

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
        if (name == "ah" or name == "bh" or name == "ch" or name == "dh") {

            return size_byte;
        }

        return 0;
    }

    [[nodiscard]] auto
    allocated_register_type(const std::string_view name) const
        -> const type* override {

        const size_t size_bytes{register_size_bytes(name)};
        if (size_bytes == 0) {
            return nullptr;
        }

        const std::string canonical_name{sized_register_name(name, size_qword)};

        for (const allocated_register& allocated : allocated_registers_) {
            if (canonical_name == allocated.name) {
                return allocated.type_ptr->size_bytes() == size_bytes
                           ? allocated.type_ptr
                           : &builtin_type_for_size_bytes(size_bytes);
            }
        }

        return nullptr;
    }

    [[nodiscard]] auto reg(const std::string_view name,
                           const type& value_type) const -> operand override {
        const size_t size_bytes{register_size_bytes(name)};
        if (size_bytes == 0) {
            throw std::invalid_argument{"unknown register"};
        }

        if (size_bytes == value_type.size_bytes()) {
            return operand::reg(name, value_type);
        }

        return operand::reg(sized_register_name(name, value_type.size_bytes()),
                            value_type);
    }

  private:
    [[nodiscard]] auto sized_register(const std::string_view name,
                                      const size_t size_bytes) const
        -> operand {

        return reg(name, builtin_type_for_size_bytes(size_bytes));
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

    template <std::integral value_t>
    [[nodiscard]] auto immediate(const value_t value) const -> operand {
        return operand::imm(std::format("{}", value), *default_type_);
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

    [[nodiscard]] static auto same_operand(const operand& lhs,
                                           const operand& rhs) -> bool {

        if (lhs.is_register()) {
            return rhs.is_register() and
                   lhs.base_register() == rhs.base_register();
        }
        if (lhs.is_immediate()) {
            return rhs.is_immediate() and lhs.immediate() == rhs.immediate();
        }
        if (not lhs.is_memory() or not rhs.is_memory()) {
            return lhs.is_empty() and rhs.is_empty();
        }

        const bool lhs_index_as_base{lhs.base_register().empty() and
                                     lhs.scale() <= 1};

        const bool rhs_index_as_base{rhs.base_register().empty() and
                                     rhs.scale() <= 1};

        const std::string_view lhs_base{
            lhs_index_as_base ? lhs.index_register() : lhs.base_register()};

        const std::string_view rhs_base{
            rhs_index_as_base ? rhs.index_register() : rhs.base_register()};

        const std::string_view lhs_index{
            lhs_index_as_base ? std::string_view{} : lhs.index_register()};

        const std::string_view rhs_index{
            rhs_index_as_base ? std::string_view{} : rhs.index_register()};

        return lhs.type_ref().size_bytes() == rhs.type_ref().size_bytes() and
               lhs_base == rhs_base and lhs_index == rhs_index and
               lhs.displacement() == rhs.displacement() and
               (lhs_index.empty() or std::max(lhs.scale(), uint8_t{1}) ==
                                         std::max(rhs.scale(), uint8_t{1}));
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

    [[nodiscard]] static auto
    format_address(const operand& value,
                   const bool explicit_displacement = false) -> std::string {

        assert(not value.is_immediate());

        std::string s;

        if (not value.base_register().empty()) {
            s += value.base_register();
        }

        if (not value.index_register().empty()) {
            if (not s.empty()) {
                s += " + ";
            }
            s += value.index_register();
            if (value.scale() > 1) {
                s += " * " + std::format("{}", value.scale());
            }
        }

        if (explicit_displacement) {
            s += std::format(" + {}", value.displacement());
        } else if (value.displacement() != 0) {
            if (not s.empty()) {
                if (value.displacement() > 0) {
                    s += " + ";
                } else {
                    s += " - ";
                }
            }
            s += std::format("{}", value.displacement() < 0
                                       ? -value.displacement()
                                       : value.displacement());
        }

        return s;
    }

    [[nodiscard]] static auto format_operand(const operand& value)
        -> std::string {
        if (value.is_immediate()) {
            return value.immediate();
        }

        return value.is_register()
                   ? value.base_register()
                   : format_operand(value, value.type_ref().size_bytes());
    }

    [[nodiscard]] static auto format_operand(const operand& value,
                                             const size_t size_bytes)
        -> std::string {
        if (value.is_immediate()) {
            return value.immediate();
        }
        std::string s;
        if (size_bytes != 0) {
            s.append(size_specifier(size_bytes));
            s.append(" [");
        }
        s.append(format_address(value));
        if (size_bytes != 0) {
            s.append("]");
        }

        return s;
    }
    auto reserve_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view reg,
                                const type& type_ref) -> void {

        comment(src_loc_tk, indent, "allocate named register {}", reg);

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            std::string loc;
            for (const allocated_register& allocated : allocated_registers_) {
                if (allocated.name == reg) {
                    loc = allocated.source_location;
                    break;
                }
            }
            throw compiler_exception{
                src_loc_tk, std::format("cannot allocate register {} because "
                                        "it was allocated at {}",
                                        reg, loc)};
        }

        allocated_registers_.emplace_back(source_location_hr(src_loc_tk),
                                          std::move(*reg_iter), &type_ref);

        named_registers_.erase(reg_iter);
    }

    auto release_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view reg) -> void {

        comment(src_loc_tk, indent, "free named register {}", reg);

        assert(allocated_registers_.back().name == reg);

        named_registers_.emplace_back(
            std::move(allocated_registers_.back().name));

        allocated_registers_.pop_back();
    }

    [[nodiscard]] auto alloc_bulk_registers(const token& src_loc_tk,
                                            const size_t indent) -> operand {

        reserve_named_register(src_loc_tk, indent, "rsi", *default_type_);
        reserve_named_register(src_loc_tk, indent, "rdi", *default_type_);

        return alloc_named_register(src_loc_tk, indent, "rcx", *default_type_);
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
            shl(indent, value, immediate(std::countr_zero(element_size_bytes)));

            return;
        }
        imul(src_loc_tk, indent, value, immediate(element_size_bytes));
    }

    [[nodiscard]] static auto sized_register_name(const std::string_view name,
                                                  const size_t size_bytes)
        -> std::string {

        // map canonical 64-bit register names to size-specific aliases
        for (const register_names& names : register_names_) {
            if (name != names.qword and name != names.dword and
                name != names.word and name != names.byte and
                (name.size() != 2 or name[1] != 'h' or names.byte.size() != 2 or
                 names.byte[1] != 'l' or name[0] != names.byte[0])) {
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

        // numbered registers are accepted as rN/rNd/rNw/rNb
        if (name.size() < 2 or name[0] != 'r') {
            std::unreachable();
        }

        const size_t digits_start{1};
        size_t digits_end{digits_start};
        while (digits_end < name.size() and name[digits_end] >= '0' and
               name[digits_end] <= '9') {

            ++digits_end;
        }

        if (digits_end == digits_start) {
            std::unreachable();
        }

        const std::string_view register_number{
            name.substr(digits_start, digits_end - digits_start)};

        switch (size_bytes) {
        case size_qword:
            return std::format("r{}", register_number);

        case size_dword:
            return std::format("r{}d", register_number);

        case size_word:
            return std::format("r{}w", register_number);

        case size_byte:
            return std::format("r{}b", register_number);

        default:
            std::unreachable();
        }
    }

    auto emit_data_value(const data_initializer& value) const -> void {
        print("{}{}", value.uops, value.value);
    }

    auto branch_comparison(const size_t indent,
                           const std::string_view comparison,
                           const bool inverted, const std::string_view label)
        -> void {

        jcc(indent, asm_cc_for_op(comparison, inverted), label);
    }

    auto store_comparison(const size_t indent,
                          const std::string_view comparison,
                          const bool inverted, const operand& dst) -> void {

        if (dst.is_memory()) {
            setcc(indent, asm_cc_for_op(comparison, inverted),
                  sized_memory(dst, size_byte));

            return;
        }
        setcc(indent, asm_cc_for_op(comparison, inverted), dst);
    }

    auto store_equal_result(const size_t indent, const operand& dst) -> void {
        if (dst.is_register()) {
            setcc(indent, "e", sized_register(dst, size_byte));

            return;
        }
        setcc(indent, "e", sized_memory(dst, size_byte));
    }

    template <typename... args_t>
    auto print(const std::format_string<args_t...> format,
               args_t&&... args) const -> void {

        std::print(os_.get(), format, std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto println(const std::format_string<args_t...> format,
                 args_t&&... args) const -> void {

        std::println(os_.get(), format, std::forward<args_t>(args)...);
    }

    auto println() const -> void { std::println(os_.get()); }

    auto comment_start(const token& src_loc_tk, const size_t indent) -> void {

        const auto [line, column]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        comment_indent(indent);
        print("[{}:{}] ", line, column);
    }

    auto emit_buffer(const std::string_view text) const -> void {
        print("{}", text);
    }

    [[nodiscard]] static auto count_instructions(const std::string_view text)
        -> size_t {

        size_t count{};
        for (const auto p : text | std::views::split('\n')) {
            const std::string_view s{p};
            if (not is_nasm_comment_or_empty_line(s)) {
                ++count;
            }
        }

        return count;
    }

    [[nodiscard]] static auto
    is_nasm_comment_or_empty_line(const std::string_view line) -> bool {

        const size_t first{line.find_first_not_of(" \t\n\r\f\v")};

        return first == std::string_view::npos or line[first] == ';';
    }

    auto mov(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        op(src_loc_tk, indent, "mov", dst_op, src_op);
    }

    auto cmp(const token& src_loc_tk, const size_t indent,
             const operand& dst_op, const operand& src_op) -> void {

        op(src_loc_tk, indent, "cmp", dst_op, src_op);
    }

    auto add(const size_t indent, const operand& dst, const operand& src)
        -> void {

        asm_line(indent, "add {}, {}", format_operand(dst),
                 format_operand(src));
    }

    auto cmp(const size_t indent, const operand& dst, const operand& src)
        -> void {

        asm_line(indent, "cmp {}, {}", format_operand(dst),
                 format_operand(src));
    }

    auto inc(const size_t indent, const operand& dst) -> void {
        asm_line(indent, "inc {}", format_operand(dst));
    }

    auto push(const size_t indent, const operand& src) -> void {
        assert(src.is_register() and src.type_ref().size_bytes() == size_qword);
        asm_line(indent, "push {}", format_operand(src));
    }

    auto pop(const size_t indent, const operand& dst) -> void {
        assert(dst.is_register() and dst.type_ref().size_bytes() == size_qword);
        asm_line(indent, "pop {}", format_operand(dst));
    }

    auto jmp(const size_t indent, const std::string_view label) -> void {
        asm_line(indent, "jmp {}", label);
    }

    auto jne(const size_t indent, const std::string_view label) -> void {
        asm_line(indent, "jne {}", label);
    }

    auto lea(const size_t indent, const operand& dst, const operand& address,
             const bool explicit_displacement = false) -> void {

        asm_line(indent, "lea {}, [{}]", format_operand(dst),
                 format_address(address, explicit_displacement));
    }

    auto syscall(const size_t indent) -> void { asm_line(indent, "syscall"); }

    template <typename... args_t>
    auto asm_line(const size_t indent,
                  const std::format_string<args_t...> format, args_t&&... args)
        -> void {

        for (size_t i{}; i < indent; ++i) {
            print("    ");
        }
        println(format, std::forward<args_t>(args)...);
    }

    auto imul(const token& src_loc_tk, const size_t indent,
              const operand& dst_op, const operand& src_op) -> void {

        op(src_loc_tk, indent, "imul", dst_op, src_op);
    }

    auto op(const token& src_loc_tk, const size_t indent,
            const std::string_view op, const operand& dst_op,
            const operand& src_op) -> void {

        if (op == "mov" and same_operand(dst_op, src_op)) {
            return;
        }

        const size_t dst_size_bytes{dst_op.type_ref().size_bytes()};
        const size_t src_size_bytes{src_op.type_ref().size_bytes()};

        if (src_op.is_immediate()) {
            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(src_op));

            return;
        }

        if (dst_op.is_memory() and src_op.is_memory()) {
            const operand reg{alloc_scratch_register(
                src_loc_tk, indent,
                builtin_type_for_size_bytes(dst_size_bytes))};

            if (dst_size_bytes > src_size_bytes) {
                asm_line(indent, "movsx {}, {}", format_operand(reg),
                         format_operand(src_op));
            } else if (dst_size_bytes < src_size_bytes) {
                asm_line(indent, "mov {}, {}", format_operand(reg),
                         format_operand(src_op, dst_size_bytes));
            } else {
                asm_line(indent, "mov {}, {}", format_operand(reg),
                         format_operand(src_op));
            }

            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(reg));

            free_scratch_register(src_loc_tk, indent, reg);

            return;
        }

        if (dst_size_bytes == src_size_bytes) {
            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(src_op));

            return;
        }

        if (dst_size_bytes > src_size_bytes) {
            if (op == "mov") {
                asm_line(indent, "movsx {}, {}", format_operand(dst_op),
                         format_operand(src_op));

                return;
            }
            if (op == "sal" or op == "sar") {
                asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                         format_operand(src_op));

                return;
            }
            const operand reg_sx{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};

            asm_line(indent, "movsx {}, {}", format_operand(reg_sx),
                     format_operand(src_op));

            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(reg_sx));

            free_scratch_register(src_loc_tk, indent, reg_sx);

            return;
        }

        if (src_op.is_register()) {
            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(sized_register(src_op, dst_size_bytes)));

            return;
        }
        assert(dst_op.is_register() and src_op.is_memory());

        asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                 format_operand(src_op, dst_size_bytes));
    }

    auto cmovs(const size_t indent, const operand& dst, const operand& src)
        -> void {

        asm_line(indent, "cmovs {}, {}", format_operand(dst),
                 format_operand(src));
    }

    auto idiv(const size_t indent, const operand& value) -> void {
        asm_line(indent, "idiv {}", format_operand(value));
    }

    auto jcc(const size_t indent, const std::string_view comparison,
             const std::string_view label) -> void {

        asm_line(indent, "j{} {}", comparison, label);
    }

    auto neg(const size_t indent, const operand& value) -> void {
        asm_line(indent, "neg {}", format_operand(value));
    }

    auto not_op(const size_t indent, const operand& value) -> void {
        asm_line(indent, "not {}", format_operand(value));
    }

    auto rep_movs(const size_t indent, const char size_suffix) -> void {
        asm_line(indent, "rep movs{}", size_suffix);
    }

    auto rep_stos(const size_t indent, const char size_suffix) -> void {
        asm_line(indent, "rep stos{}", size_suffix);
    }

    auto repe_cmps(const size_t indent, const char size_suffix) -> void {
        asm_line(indent, "repe cmps{}", size_suffix);
    }

    auto setcc(const size_t indent, const std::string_view comparison,
               const operand& value) -> void {

        asm_line(indent, "set{} {}", comparison, format_operand(value));
    }

    auto shl(const size_t indent, const operand& dst, const operand& src)
        -> void {

        asm_line(indent, "shl {}, {}", format_operand(dst),
                 format_operand(src));
    }

    auto test(const size_t indent, const operand& dst, const operand& src)
        -> void {

        asm_line(indent, "test {}, {}", format_operand(dst),
                 format_operand(src));
    }

    auto xor_op(const size_t indent, const operand& dst, const operand& src)
        -> void {

        asm_line(indent, "xor {}, {}", format_operand(dst),
                 format_operand(src));
    }

    [[nodiscard]] static auto get_data_def(const size_t size_bytes)
        -> std::string_view {

        switch (size_bytes) {
        case size_qword:
            return data_qword;

        case size_dword:
            return data_dword;

        case size_word:
            return data_word;

        case size_byte:
            return data_byte;

        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto asm_cc_for_op(const std::string_view op,
                                            const bool inverted)
        -> std::string_view {

        if (op == "==") {
            return inverted ? "ne" : "e";
        }
        if (op == "!=") {
            return inverted ? "e" : "ne";
        }
        if (op == "<") {
            return inverted ? "ge" : "l";
        }
        if (op == "<=") {
            return inverted ? "g" : "le";
        }
        if (op == ">") {
            return inverted ? "le" : "g";
        }
        if (op == ">=") {
            return inverted ? "l" : "ge";
        }
        std::unreachable();
    }

    auto comment_indent(const size_t indent) -> void {
        print(";");

        if (indent != 0) {
            print("   ");
        }
        for (size_t i{1}; i < indent; ++i) {
            print("    ");
        }
    }

    // human-readable "line:col" for a token, using the cached source text
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {

        if (src_loc_tk.at_line() == 0) {
            return "0:0";
        }

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}:{}", line, col);
    }

    // returns the cached built-in type (i64/i32/i16/i8) matching 'size'
    [[nodiscard]] auto
    builtin_type_for_size_bytes(const size_t size_bytes) const -> const type& {

        switch (size_bytes) {
        case size_qword:
            return *type_i64_;

        case size_dword:
            return *type_i32_;

        case size_word:
            return *type_i16_;

        case size_byte:
            return *type_i8_;

        default:
            std::unreachable();
        }
    }
};
