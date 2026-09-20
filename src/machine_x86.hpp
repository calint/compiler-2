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
    static constexpr size_t data_alignment_{16};

    static constexpr std::string_view data_qword{"dq"};
    static constexpr std::string_view data_dword{"dd"};
    static constexpr std::string_view data_word{"dw"};
    static constexpr std::string_view data_byte{"db"};

    static constexpr size_t threshold_for_rep_stos{32};
    static constexpr size_t threshold_for_rep_movs{16};
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
    };

    std::vector<std::string> all_registers_{
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};

    size_t all_registers_initial_size_{all_registers_.size()};
    std::vector<std::string> named_registers_{"rax", "rbx", "rcx", "rdx",
                                              "rsi", "rdi", "rbp"};

    size_t named_registers_initial_size_{named_registers_.size()};
    std::vector<std::string> scratch_registers_{"r8",  "r9",  "r10", "r12",
                                                "r13", "r14", "r15"};
    // note: r11 gets clobbered by syscall

    size_t scratch_registers_initial_size_{scratch_registers_.size()};
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

    auto println() const -> void override { std::println(os_.get()); }

    auto comment(const token& source_location, const size_t indent,
                 const std::string_view text) -> void override {

        if (source_location.is_empty()) {
            comment_indent(indent);
            print(" ");
        } else {
            comment_start(source_location, indent);
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
                                              const size_t indnt,
                                              const type& type_ref)
        -> operand override {

        if (scratch_registers_.empty()) {
            throw compiler_exception{src_loc_tk,
                                     "out of scratch registers. try to reduce "
                                     "expression complexity"};
        }

        std::string reg{std::move(scratch_registers_.back())};
        scratch_registers_.pop_back();

        comment(src_loc_tk, indnt, "allocate scratch register -> {}", reg);

        const size_t n{scratch_registers_initial_size_ -
                       scratch_registers_.size()};

        usage_max_scratch_regs_ = std::max(n, usage_max_scratch_regs_);

        allocated_registers_.emplace_back(source_location_hr(src_loc_tk),
                                          std::move(reg));

        const std::string& allocated_name{allocated_registers_.back().name};
        operand result{sized_register(allocated_name, type_ref.size())};
        result.allocation_register = allocated_name;
        result.type_ptr = &type_ref;

        return result;
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indnt,
                         const std::string_view reg, const type& type_ref)
        -> operand override {

        reserve_named_register(src_loc_tk, indnt, reg);
        operand result{sized_register(reg, type_ref.size())};
        result.allocation_register = reg;
        result.type_ptr = &type_ref;

        return result;
    }

    auto free_named_register(const token& src_loc_tk, const size_t indnt,
                             const operand& reg) -> void override {

        assert(reg.is_register() and not reg.allocation_register.empty());

        release_named_register(src_loc_tk, indnt, reg.allocation_register);
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indnt,
                               const operand& reg) -> void override {

        assert(reg.is_register() and not reg.allocation_register.empty());

        comment(src_loc_tk, indnt, "free scratch register {}",
                reg.allocation_register);

        assert(allocated_registers_.back().name == reg.allocation_register);

        scratch_registers_.emplace_back(
            std::move(allocated_registers_.back().name));

        allocated_registers_.pop_back();
    }

    // asserts register pools are balanced and prints usage stats; called
    // once at the end of the compile pass
    auto finish() -> void override {
        println("\n; max scratch registers in use: {}",
                usage_max_scratch_regs_);

        assert(all_registers_.size() == all_registers_initial_size_);
        assert(allocated_registers_.empty());
        assert(named_registers_.size() == named_registers_initial_size_);
        assert(scratch_registers_.size() == scratch_registers_initial_size_);

        usage_max_scratch_regs_ = 0;
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        assert(dst.is_register() or dst.is_memory());

        mov(src_loc_tk, indent, dst, src);
    }

    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t bytes,
                          const operand& address) -> void override {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, bytes,
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

    auto compare_and_branch(const token& src_loc_tk, const size_t indent,
                            const operand& lhs, const operand& rhs,
                            const comparison_action& action,
                            const std::span<const operand> consumed_temporaries)
        -> void override {

        cmp(src_loc_tk, indent, lhs, rhs);

        free_scratch_registers(src_loc_tk, indent, consumed_temporaries);

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
        syscall(indent);
    }

    auto advance_array_iteration(const size_t indent, const operand& iterator,
                                 const operand& counter,
                                 const size_t element_size,
                                 const size_t array_size,
                                 const std::string_view loop_label)
        -> void override {

        add(indent, iterator, immediate(element_size));
        inc(indent, counter);
        cmp(indent, counter, immediate(array_size));
        jne(indent, loop_label);
    }

    auto copy(const token& src_loc_tk, const size_t indent, const operand& src,
              const operand& dst, const size_t bytes_count) -> void override {

        if (bytes_count > threshold_for_rep_movs) {
            reserve_named_register(src_loc_tk, indent, "rsi");
            reserve_named_register(src_loc_tk, indent, "rdi");
            reserve_named_register(src_loc_tk, indent, "rcx");

            lea(indent, machine_x86::reg("rsi"), src);
            lea(indent, machine_x86::reg("rdi"), dst);
            mov(src_loc_tk, indent, machine_x86::reg("rcx"),
                immediate(bytes_count));

            rep_movs(indent, 'b');

            release_bulk_registers(src_loc_tk, indent);

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_movs);

        reserve_named_register(src_loc_tk, indent, "rax");
        size_t rest{bytes_count};
        operand src_operand{src};
        operand dst_operand{dst};
        for (size_t width{operand::size_qword}; width >= operand::size_byte;
             width /= 2) {

            while (rest >= width) {
                const operand reg{sized_register("rax", width)};
                mov(src_loc_tk, indent, reg, sized_memory(src_operand, width));
                mov(src_loc_tk, indent, sized_memory(dst_operand, width), reg);

                rest -= width;
                if (rest != 0) {
                    src_operand.displacement += static_cast<int32_t>(width);
                    dst_operand.displacement += static_cast<int32_t>(width);
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

        lea(indent, machine_x86::reg("rsi"), address);
    }

    auto set_array_copy_destination(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rdi"), address);
    }

    auto end_array_copy(const token& src_loc_tk, const size_t indent,
                        const size_t element_size) -> void override {

        scale_by_element_size(src_loc_tk, indent, machine_x86::reg("rcx"),
                              element_size);

        rep_movs(indent, 'b');
        release_bulk_registers(src_loc_tk, indent);
    }

    auto begin_memory_equal(const token& src_loc_tk, const size_t indent)
        -> operand override {

        return alloc_bulk_registers(src_loc_tk, indent);
    }

    auto set_memory_equal_left(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rsi"), address);
    }

    auto set_memory_equal_right(const size_t indent, const operand& address)
        -> void override {

        lea(indent, machine_x86::reg("rdi"), address);
    }

    auto end_memory_equal(const token& src_loc_tk, const size_t indent,
                          const size_t bytes_count, const operand& dst)
        -> void override {

        char rep_size{'b'};
        size_t count{bytes_count};
        if ((count % operand::size_qword) == 0) {
            rep_size = 'q';
            count /= operand::size_qword;
        } else if ((count % operand::size_dword) == 0) {
            rep_size = 'd';
            count /= operand::size_dword;
        } else if ((count % operand::size_word) == 0) {
            rep_size = 'w';
            count /= operand::size_word;
        }
        mov(src_loc_tk, indent, machine_x86::reg("rcx"), immediate(count));
        repe_cmps(indent, rep_size);
        release_bulk_registers(src_loc_tk, indent);
        store_equal_result(indent, dst);
    }

    auto end_arrays_equal(const token& src_loc_tk, const size_t indent,
                          const size_t element_size, const operand& dst)
        -> void override {

        scale_by_element_size(src_loc_tk, indent, machine_x86::reg("rcx"),
                              element_size);

        repe_cmps(indent, 'b');
        release_bulk_registers(src_loc_tk, indent);
        store_equal_result(indent, dst);
    }

    auto zero(const token& src_loc_tk, const size_t indent, const operand& dst,
              const size_t bytes_count) -> void override {

        if (bytes_count > threshold_for_rep_stos) {
            reserve_named_register(src_loc_tk, indent, "rax");
            reserve_named_register(src_loc_tk, indent, "rdi");
            reserve_named_register(src_loc_tk, indent, "rcx");
            xor_op(indent, machine_x86::reg("al"), machine_x86::reg("al"));
            lea(indent, machine_x86::reg("rdi"), dst);
            mov(src_loc_tk, indent, machine_x86::reg("rcx"),
                immediate(bytes_count));
            rep_stos(indent, 'b');
            release_named_register(src_loc_tk, indent, "rcx");
            release_named_register(src_loc_tk, indent, "rdi");
            release_named_register(src_loc_tk, indent, "rax");

            return;
        }

        comment(src_loc_tk, indent, "size <= {} B, use mov",
                threshold_for_rep_stos);

        size_t rest{bytes_count};
        operand dst_operand{dst};
        for (size_t width{operand::size_qword}; width >= operand::size_byte;
             width /= 2) {

            while (rest >= width) {
                mov(src_loc_tk, indent, sized_memory(dst_operand, width),
                    immediate(0));

                rest -= width;
                if (rest != 0) {
                    dst_operand.displacement += static_cast<int32_t>(width);
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

        if (product.size == operand::size_byte) {
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
            src_loc_tk, indent, builtin_type_for_size(product.size))};

        mov(src_loc_tk, indent, reg, product);
        imul(src_loc_tk, indent, reg, factor);
        mov(src_loc_tk, indent, product, reg);
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto validate_shift_operand(const token& src_loc_tk,
                                const operand& count) const -> void override {

        if (count.is_register() and count.base_register == "rcx") {
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
        reserve_named_register(src_loc_tk, indent, "rcx");
        mov(src_loc_tk, indent, sized_register("rcx", dst.size), count);
        op(src_loc_tk, indent, operation == '<' ? "sal" : "sar", dst,
           sized_register("rcx", operand::size_byte));
        release_named_register(src_loc_tk, indent, "rcx");
    }

    auto validate_division_operand(const token& src_loc_tk,
                                   const operand& divisor) const
        -> void override {

        if (divisor.is_register() and (divisor.base_register == "rdx" or
                                       divisor.base_register == "rax")) {
            throw compiler_exception{
                src_loc_tk, "cannot use 'rdx' or 'rax' for division; they are "
                            "reserved"};
        }
    }

    auto divide(const token& src_loc_tk, const size_t indent,
                const char operation, const operand& dst,
                const operand& divisor) -> void override {

        assert(operation == '/' or operation == '%');

        reserve_named_register(src_loc_tk, indent, "rax");
        mov(src_loc_tk, indent, machine_x86::reg("rax"), dst);

        reserve_named_register(src_loc_tk, indent, "rdx");
        asm_line(indent, "cqo");

        if (divisor.is_immediate() or divisor.size != operand::size_qword) {
            const operand scratch_reg{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};

            mov(src_loc_tk, indent, scratch_reg, divisor);
            idiv(indent, scratch_reg);
            free_scratch_register(src_loc_tk, indent, scratch_reg);
        } else {
            idiv(indent, divisor);
        }

        mov(src_loc_tk, indent, dst,
            machine_x86::reg(operation == '/' ? "rax" : "rdx"));

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

    [[nodiscard]] auto can_encode_index_scale(const size_t size) const
        -> bool override {

        return size == operand::size_byte or size == operand::size_word or
               size == operand::size_dword or size == operand::size_qword;
    }

    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size)
        -> void override {

        scale_by_element_size(src_loc_tk, indent, index, element_size);
    }

    auto exit_process(const token& src_loc_tk, const size_t indent,
                      const int exit_code) -> void override {

        mov(src_loc_tk, indent, machine_x86::reg("rdi"), immediate(exit_code));
        mov(src_loc_tk, indent, machine_x86::reg("rax"),
            immediate(syscall_exit));
        syscall(indent);
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        return "rbp";
    }

    [[nodiscard]] auto is_variables_base(const operand& reg) const
        -> bool override {

        return not reg.is_indexed() and reg.base_register == "rbp";
    }

    auto address_of_variable(const token& src_loc_tk, const size_t indent,
                             const operand& dst, const int32_t offset)
        -> void override {

        const operand address{operand::mem("rbp", {}, 1, offset)};
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
        reserve_named_register(token{}, 0, "rbp");
    }

    auto release_variables_base() -> void override {
        release_named_register(token{}, 0, "rbp");
    }

    auto program_start() -> void override {
        println(";\n; generated by baz\n;\n\ndefault rel\n");
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
                      const operand& reg_to_check, const size_t array_size,
                      const bool allow_end, const operand& reg_size,
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
                cmovs(indent, machine_x86::reg("rbp"), reg_line_num);
            }
            jcc(indent, "s", "panic_bounds");
        }

        if (options.upper) {
            if (not reg_size.is_empty()) {
                const operand reg_top_idx{
                    alloc_scratch_register(src_loc_tk, indent, *default_type_)};

                mov(src_loc_tk, indent, reg_top_idx, reg_size);
                add(indent, reg_top_idx, reg_to_check);
                cmp(indent, reg_top_idx, immediate(array_size));
                free_scratch_register(src_loc_tk, indent, reg_top_idx);
            } else {
                cmp(indent, reg_to_check, immediate(array_size));
            }
            if (options.with_line) {
                op(src_loc_tk, indent, std::format("cmov{}", comparison),
                   machine_x86::reg("rbp"), reg_line_num);
            }
            jcc(indent, comparison, "panic_bounds");
        }

        if (options.with_line) {
            free_scratch_register(src_loc_tk, indent, reg_line_num);
        }
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        if (not with_line) {
            println();
            println("panic_bounds:");
            println("    ; system call: exit 255");
            println("    mov rax, 60");
            println("    mov rdi, 255");
            println("    syscall");
        } else {
            println("panic_bounds:");
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

    auto reserve_variables(const size_t alignment, const size_t bytes_count)
        -> void override {

        println("dat.end:\n\nsection .bss.vars nobits alloc write\nalign "
                "{}\nvars:\nvars "
                "resb {}",
                alignment, bytes_count);
    }

    auto emit_data(const size_t element_size, const data_initializer& value)
        -> void override {

        print("{} ", get_data_def(element_size));
        emit_data_value(value);
        println();
    }

    using machine::emit_data_array;

    auto emit_data_array(const size_t element_size,
                         const std::function_ref<bool(data_initializer&)> next)
        -> void override {

        print("{} ", get_data_def(element_size));
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
        size_t position{};
        while (position < value.size()) {
            const size_t next{value.find('`', position)};
            if (next == std::string_view::npos) {
                print("{}", value.substr(position));
                break;
            }
            print("{}\\`", value.substr(position, next - position));
            position = next + 1;
        }
        println("`");
    }

    auto emit_zero_data(const size_t bytes_count) const -> void override {
        emit_repeated_data(operand::size_byte, bytes_count, {});
    }

    auto emit_repeated_data(const size_t element_size, const size_t count,
                            const data_initializer& value) const
        -> void override {

        std::println(os_.get(), "times {} {} {}{}", count,
                     get_data_def(element_size), value.uops, value.value);
    }

    // returns 0 if operand is not a register
    [[nodiscard]] auto register_size(const std::string_view operand) const
        -> size_t override {

        for (const register_names& names : register_names_) {
            if (operand == names.qword) {
                return operand::size_qword;
            }
            if (operand == names.dword) {
                return operand::size_dword;
            }
            if (operand == names.word) {
                return operand::size_word;
            }
            if (operand == names.byte) {
                return operand::size_byte;
            }
        }
        if (operand == "ah" or operand == "bh" or operand == "ch" or
            operand == "dh") {

            return operand::size_byte;
        }

        return 0;
    }

    [[nodiscard]] auto reg(const std::string_view name) const
        -> operand override {
        return operand::reg(name, register_size(name));
    }

  private:
    [[nodiscard]] auto sized_register(const std::string_view reg,
                                      const size_t size) const -> operand {

        operand result{operand::reg(sized_register_operand(reg, size), size)};
        result.type_ptr = &builtin_type_for_size(size);

        return result;
    }

    [[nodiscard]] auto sized_register(const operand& reg,
                                      const size_t size) const -> operand {

        assert(reg.is_register());

        operand result{sized_register(reg.allocation_register.empty()
                                          ? reg.base_register
                                          : reg.allocation_register,
                                      size)};

        result.allocation_register = reg.allocation_register;
        if (reg.type_ptr and reg.size == size) {
            result.type_ptr = reg.type_ptr;
        }

        return result;
    }

    template <std::integral value_t>
    [[nodiscard]] auto immediate(const value_t value) const -> operand {
        return operand::imm(std::format("{}", value), *default_type_);
    }

    [[nodiscard]] static auto sized_memory(operand value, const size_t size)
        -> operand {

        assert(value.is_memory());

        value.size = size;

        return value;
    }

    [[nodiscard]] static auto same_operand(const operand& left,
                                           const operand& right) -> bool {

        if (left.kind() != right.kind()) {
            return false;
        }
        if (left.is_register()) {
            return left.base_register == right.base_register;
        }
        if (left.is_immediate()) {
            return left.immediate == right.immediate;
        }

        const bool left_index_as_base{left.base_register.empty() and
                                      left.scale <= 1};

        const bool right_index_as_base{right.base_register.empty() and
                                       right.scale <= 1};

        const std::string_view left_base{
            left_index_as_base ? left.index_register : left.base_register};

        const std::string_view right_base{
            right_index_as_base ? right.index_register : right.base_register};

        const std::string_view left_index{
            left_index_as_base ? std::string_view{} : left.index_register};

        const std::string_view right_index{
            right_index_as_base ? std::string_view{} : right.index_register};

        return left.size == right.size and left_base == right_base and
               left_index == right_index and
               left.displacement == right.displacement and
               (left_index.empty() or std::max(left.scale, uint8_t{1}) ==
                                          std::max(right.scale, uint8_t{1}));
    }

    [[nodiscard]] static auto size_specifier(const size_t size)
        -> std::string_view {

        switch (size) {
        case operand::size_qword:
            return "qword";

        case operand::size_dword:
            return "dword";

        case operand::size_word:
            return "word";

        case operand::size_byte:
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

        if (not value.base_register.empty()) {
            s += value.base_register;
        }

        if (not value.index_register.empty()) {
            if (not s.empty()) {
                s += " + ";
            }
            s += value.index_register;
            if (value.scale > 1) {
                s += " * " + std::format("{}", value.scale);
            }
        }

        if (explicit_displacement) {
            s += std::format(" + {}", value.displacement);
        } else if (value.displacement != 0) {
            if (not s.empty()) {
                if (value.displacement > 0) {
                    s += " + ";
                } else {
                    s += " - ";
                }
            }
            s += std::format("{}", value.displacement < 0 ? -value.displacement
                                                          : value.displacement);
        }

        return s;
    }

    [[nodiscard]] static auto format_operand(const operand& value)
        -> std::string {
        if (value.is_immediate()) {
            return value.immediate;
        }

        return value.is_register() ? value.base_register
                                   : format_operand(value, value.size);
    }

    [[nodiscard]] static auto format_operand(const operand& value,
                                             const size_t size) -> std::string {
        if (value.is_immediate()) {
            return value.immediate;
        }
        std::string s;
        if (size != 0) {
            s.append(size_specifier(size));
            s.append(" [");
        }
        s.append(format_address(value));
        if (size != 0) {
            s.append("]");
        }

        return s;
    }
    auto reserve_named_register(const token& src_loc_tk, const size_t indnt,
                                const std::string_view reg) -> void {

        comment(src_loc_tk, indnt, "allocate named register {}", reg);

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
                                          std::move(*reg_iter));

        named_registers_.erase(reg_iter);
    }

    auto release_named_register(const token& src_loc_tk, const size_t indnt,
                                const std::string_view reg) -> void {

        comment(src_loc_tk, indnt, "free named register {}", reg);

        assert(allocated_registers_.back().name == reg);

        named_registers_.emplace_back(
            std::move(allocated_registers_.back().name));

        allocated_registers_.pop_back();
    }

    [[nodiscard]] auto alloc_bulk_registers(const token& src_loc_tk,
                                            const size_t indent) -> operand {

        reserve_named_register(src_loc_tk, indent, "rsi");
        reserve_named_register(src_loc_tk, indent, "rdi");

        return alloc_named_register(src_loc_tk, indent, "rcx", *default_type_);
    }

    auto release_bulk_registers(const token& src_loc_tk, const size_t indent)
        -> void {

        release_named_register(src_loc_tk, indent, "rcx");
        release_named_register(src_loc_tk, indent, "rdi");
        release_named_register(src_loc_tk, indent, "rsi");
    }

    auto scale_by_element_size(const token& src_loc_tk, const size_t indent,
                               const operand& value, const size_t element_size)
        -> void {

        if (element_size <= 1) {
            return;
        }
        if (std::has_single_bit(element_size)) {
            shl(indent, value, immediate(std::countr_zero(element_size)));

            return;
        }
        imul(src_loc_tk, indent, value, immediate(element_size));
    }

    [[nodiscard]] static auto
    sized_register_operand(const std::string_view operand, const size_t size)
        -> std::string {

        // map canonical 64-bit register names to size-specific aliases
        for (const register_names& names : register_names_) {
            if (operand != names.qword) {
                continue;
            }
            switch (size) {
            case operand::size_qword:
                return std::string{names.qword};

            case operand::size_dword:
                return std::string{names.dword};

            case operand::size_word:
                return std::string{names.word};

            case operand::size_byte:
                return std::string{names.byte};

            default:
                std::unreachable();
            }
        }

        // numbered registers are accepted as rN/rNd/rNw/rNb
        if (operand.size() < 2 || operand[0] != 'r') {
            std::unreachable();
        }

        const size_t digits_start{1};
        size_t digits_end{digits_start};
        while (digits_end < operand.size() && operand[digits_end] >= '0' &&
               operand[digits_end] <= '9') {

            ++digits_end;
        }

        if (digits_end == digits_start) {
            std::unreachable();
        }

        const std::string_view rnbr{
            operand.substr(digits_start, digits_end - digits_start)};

        switch (size) {
        case operand::size_qword:
            return std::format("r{}", rnbr);

        case operand::size_dword:
            return std::format("r{}d", rnbr);

        case operand::size_word:
            return std::format("r{}w", rnbr);

        case operand::size_byte:
            return std::format("r{}b", rnbr);

        default:
            std::unreachable();
        }
    }

    auto emit_data_value(const data_initializer& value) const -> void {
        std::print(os_.get(), "{}{}", value.uops, value.value);
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
                  sized_memory(dst, operand::size_byte));

            return;
        }
        setcc(indent, asm_cc_for_op(comparison, inverted), dst);
    }

    auto store_equal_result(const size_t indent, const operand& dst) -> void {
        if (dst.is_register()) {
            setcc(indent, "e", sized_register(dst, operand::size_byte));

            return;
        }
        setcc(indent, "e", sized_memory(dst, operand::size_byte));
    }

    template <typename... args_t>
    auto print(const std::format_string<args_t...> format, args_t&&... args)
        -> void {

        std::print(os_.get(), format, std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto println(const std::format_string<args_t...> format, args_t&&... args)
        -> void {

        std::println(os_.get(), format, std::forward<args_t>(args)...);
    }

    auto comment_start(const token& source_location, const size_t indent)
        -> void {

        const auto [line, column]{line_and_col_num_for_char_index(
            source_location.at_line(), source_location.start_index(), source_)};

        comment_indent(indent);
        print("[{}:{}] ", line, column);
    }

    auto emit_buffer(const std::string_view text) const -> void {
        std::print(os_.get(), "{}", text);
    }

    [[nodiscard]] static auto count_instructions(std::string_view text)
        -> size_t {

        size_t count{};
        while (not text.empty()) {
            const size_t newline{text.find('\n')};
            if (not is_nasm_comment_or_empty_line(text.substr(0, newline))) {
                ++count;
            }
            if (newline == std::string_view::npos) {
                break;
            }
            text.remove_prefix(newline + 1);
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

        for (size_t index{}; index < indent; ++index) {
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

        const size_t dst_size{operand_size(dst_op)};
        const size_t src_size{operand_size(src_op)};

        if (dst_op.is_memory() and src_op.is_memory()) {
            const operand reg{alloc_scratch_register(
                src_loc_tk, indent, builtin_type_for_size(dst_size))};

            if (dst_size > src_size) {
                asm_line(indent, "movsx {}, {}", format_operand(reg),
                         format_operand(src_op));
            } else if (dst_size < src_size) {
                asm_line(indent, "mov {}, {}", format_operand(reg),
                         format_operand(src_op, dst_size));
            } else {
                asm_line(indent, "mov {}, {}", format_operand(reg),
                         format_operand(src_op));
            }

            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(reg));

            free_scratch_register(src_loc_tk, indent, reg);

            return;
        }

        if (dst_size == src_size) {
            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     format_operand(src_op));

            return;
        }

        if (dst_size > src_size) {
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
                     format_operand(sized_register(src_op, dst_size)));

            return;
        }
        if (dst_op.is_register()) {
            asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                     src_op.is_memory() ? format_operand(src_op, dst_size)
                                        : format_operand(src_op));

            return;
        }

        // memory destination, immediate source; dst_size < src_size

        asm_line(indent, "{} {}, {}", op, format_operand(dst_op),
                 format_operand(src_op));
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

    [[nodiscard]] static auto get_data_def(const size_t size)
        -> std::string_view {

        switch (size) {
        case operand::size_qword:
            return data_qword;

        case operand::size_dword:
            return data_dword;

        case operand::size_word:
            return data_word;

        case operand::size_byte:
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
        for (size_t index{1}; index < indent; ++index) {
            print("    ");
        }
    }

    [[nodiscard]] auto operand_size(const operand& value) const -> size_t {

        if (not value.is_immediate() and value.size != 0) {
            return value.size;
        }

        return default_type_->size();
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

    // returns the cached builtin type (i64/i32/i16/i8) matching 'size'
    [[nodiscard]] auto builtin_type_for_size(const size_t size) const
        -> const type& {

        switch (size) {
        case operand::size_qword:
            return *type_i64_;

        case operand::size_dword:
            return *type_i32_;

        case operand::size_word:
            return *type_i16_;

        case operand::size_byte:
            return *type_i8_;

        default:
            std::unreachable();
        }
    }
};
