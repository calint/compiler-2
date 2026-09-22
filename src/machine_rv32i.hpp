#pragma once

#include <array>
#include <bit>
#include <cstdio>
#include <iostream>
#include <limits>
#include <print>
#include <utility>

#include "compiler_exception.hpp"
#include "machine.hpp"
#include "panic_exception.hpp"
#include "type.hpp"

class machine_rv32i final : public machine {
    static constexpr size_t s0_register_index{8};
    static constexpr size_t data_alignment_{16};
    static constexpr int64_t immediate_min{-2048};
    static constexpr int64_t immediate_max{2047};

    static constexpr std::array<std::string_view, 32> register_names_{
        "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    };

    static constexpr std::array<size_t, 30> scratch_registers_{
        10, 11, 12, 13, 14, 15, 16, 17, 1, 3, 4, 8,  9,  18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 5, 6, 7, 28, 29, 30, 31,
    };
    // note: reverse allocation keeps a0-a7 last to reduce register conflicts
    //       with builtins that require argument, result, or syscall registers.

    struct allocation {
        size_t register_index;
        const type* type_ptr;
    };

    std::reference_wrapper<std::ostream> os_{std::cout};
    const type* type_i32_{};
    uint32_t unavailable_registers_{};
    bool variables_base_reserved_{};
    bool frame_base_reserved_{};
    std::vector<allocation> allocations_;

    [[nodiscard]] static auto register_index(const std::string_view name)
        -> size_t {
        for (const auto [index, alias] :
             std::views::enumerate(register_names_)) {
            if (name == alias or name == std::format("x{}", index) or
                (std::cmp_equal(index, s0_register_index) and name == "fp")) {
                return static_cast<size_t>(index);
            }
        }

        return register_names_.size();
    }

    [[nodiscard]] static auto register_mask(const std::string_view name)
        -> uint32_t {
        const size_t index{register_index(name)};

        return index == register_names_.size() ? 0 : uint32_t{1} << index;
    }

    static auto validate_scalar(const token& src_loc_tk, const type& value_type)
        -> void {
        if (not value_type.is_builtin() or
            (value_type.size_bytes() != 1 and value_type.size_bytes() != 2 and
             value_type.size_bytes() != 4)) {
            throw compiler_exception{
                src_loc_tk, "RV32I requires an 8-, 16-, or 32-bit scalar"};
        }
    }

    static auto validate_address(const token& src_loc_tk,
                                 const operand& address) -> void {
        if (not address.is_memory()) {
            throw compiler_exception{src_loc_tk,
                                     "RV32I requires a memory address"};
        }
        constexpr int64_t limit{std::numeric_limits<uint32_t>::max()};
        if (address.displacement() < -limit or address.displacement() > limit) {
            throw compiler_exception{
                src_loc_tk, "address offset exceeds RV32I address range"};
        }
        if (not address.index_register().empty() and
            register_index(address.index_register()) ==
                register_names_.size()) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I index register"};
        }
        if (not address.index_register().empty() and
            not std::has_single_bit(address.scale())) {
            throw compiler_exception{src_loc_tk,
                                     "index scale must be a power of two"};
        }
    }

    template <typename... args_t>
    auto asm_line(const size_t indent,
                  const std::format_string<args_t...> format,
                  args_t&&... args) const -> void {
        std::print(os_.get(), "{}", std::string(indent * 4, ' '));
        std::println(os_.get(), format, std::forward<args_t>(args)...);
    }

    class address_scope {
        machine_rv32i& backend_;
        uint32_t saved_mask_;
        size_t saved_count_;

      public:
        address_scope(machine_rv32i& backend, const operand& dst,
                      const operand& src)
            : backend_{backend}, saved_mask_{backend.unavailable_registers_},
              saved_count_{backend.allocations_.size()} {
            for (const operand* value : {&dst, &src}) {
                if (value->is_register() or value->is_memory()) {
                    backend_.unavailable_registers_ |=
                        register_mask(value->base_register());
                }
                if (value->is_memory()) {
                    backend_.unavailable_registers_ |=
                        register_mask(value->index_register());
                }
            }
        }

        address_scope(const address_scope&) = delete;
        address_scope(address_scope&&) = delete;
        auto operator=(const address_scope&) -> address_scope& = delete;
        auto operator=(address_scope&&) -> address_scope& = delete;

        ~address_scope() {
            while (backend_.allocations_.size() > saved_count_) {
                backend_.allocations_.pop_back();
            }
            backend_.unavailable_registers_ = saved_mask_;
        }
    };

    // lower base + index * scale + displacement to register + signed 12-bit
    // offset
    [[nodiscard]] auto lower_address(const token& src_loc_tk,
                                     const size_t indent,
                                     const operand& address) -> operand {

        validate_address(src_loc_tk, address);

        const std::string& base{address.base_register()};
        const std::string& index{address.index_register()};
        const int64_t offset{address.displacement()};

        // without an index, a small displacement already fits the memory
        // instruction
        if (index.empty() and offset >= immediate_min and
            offset <= immediate_max and
            (base.empty() or register_index(base) != register_names_.size())) {

            return operand::mem(base.empty() ? "zero" : std::string_view{base},
                                {}, 1, offset, address.type_ref());
        }

        const operand result{
            alloc_scratch_register(src_loc_tk, indent, default_type())};
        const std::string& result_name{result.base_register()};

        if (not index.empty()) {
            if (address.scale() == 1) {
                // scale 1 only needs a copy of the index
                asm_line(indent, "addi {}, {}, 0", result_name, index);
            } else {
                // multiply by the power-of-two scale with one left shift
                asm_line(indent, "slli {}, {}, {}", result_name, index,
                         std::countr_zero(address.scale()));
            }
        } else {
            // start with no index contribution; zero is the constant-zero
            // register
            asm_line(indent, "addi {}, zero, 0", result_name);
        }

        // result now holds index * scale; add the base without changing the
        // inputs
        if (not base.empty()) {
            if (register_index(base) != register_names_.size()) {
                // add the base address to the scaled index
                asm_line(indent, "add {}, {}, {}", result_name, result_name,
                         base);
            } else {
                const operand symbol{
                    alloc_scratch_register(src_loc_tk, indent, default_type())};
                // la materializes a symbol's address, not its contents
                asm_line(indent, "la {}, {}", symbol.base_register(), base);
                asm_line(indent, "add {}, {}, {}", result_name, result_name,
                         symbol.base_register());
                free_scratch_register(src_loc_tk, indent, symbol);
            }
        }

        // keep a small displacement in the final load/store instead of adding
        // it here
        if (offset >= immediate_min and offset <= immediate_max) {
            return operand::mem(result_name, {}, 1, offset, address.type_ref());
        }

        const operand displacement{
            alloc_scratch_register(src_loc_tk, indent, default_type())};
        // preserve the validated offset's low 32 bits for RV32 address
        // arithmetic
        const int32_t bits{
            std::bit_cast<int32_t>(static_cast<uint32_t>(offset))};
        // li expands to instructions that load the full 32-bit offset
        asm_line(indent, "li {}, {}", displacement.base_register(), bits);
        // fold the offset into the base so the memory instruction can use zero
        asm_line(indent, "add {}, {}, {}", result_name, result_name,
                 displacement.base_register());
        free_scratch_register(src_loc_tk, indent, displacement);

        return operand::mem(result_name, {}, 1, 0, address.type_ref());
    }

    auto io_syscall(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& descriptor,
                    const operand& address, const operand& count,
                    const int syscall_number) -> void {
        constexpr size_t word_size{4};
        for (const operand* value : {&dst, &descriptor, &address, &count}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == word_size);
        }
        assert(register_index(dst.base_register()) == register_index("a0"));
        assert(register_index(descriptor.base_register()) ==
               register_index("a0"));
        assert(register_index(address.base_register()) == register_index("a1"));
        assert(register_index(count.base_register()) == register_index("a2"));

        const operand syscall_register{
            alloc_named_register(src_loc_tk, indent, "a7", default_type())};

        asm_line(indent, "li a7, {}", syscall_number);
        asm_line(indent, "ecall");
        free_named_register(src_loc_tk, indent, syscall_register);
    }

    auto binary_operation(const token& src_loc_tk, const size_t indent,
                          const std::string_view instruction,
                          const operand& destination, const operand& src,
                          const bool shift_count = {}) -> void {

        const address_scope scope{*this, destination, src};

        const operand left{
            alloc_scratch_register(src_loc_tk, indent, destination.type_ref())};

        const operand right{alloc_scratch_register(
            src_loc_tk, indent,
            shift_count ? default_type() : destination.type_ref())};

        copy_value(src_loc_tk, indent, left, destination);
        copy_value(src_loc_tk, indent, right, src);

        asm_line(indent, "{} {}, {}, {}", instruction, left.base_register(),
                 left.base_register(), right.base_register());

        copy_value(src_loc_tk, indent, destination, left);
    }

    [[noreturn]] static auto todo() -> void {
        std::println(stderr, "todo");
        throw panic_exception{"RV32I backend not implemented"};
    }

  public:
    using machine::comment;
    using machine::emit_data_array;

    [[nodiscard]] auto
    registers_for_builtin(const builtin_function function) const
        -> builtin_registers override {
        static constexpr std::array<std::string_view, 3> io_args{"a0", "a1",
                                                                 "a2"};
        static constexpr std::array<std::string_view, 1> exit_args{"a0"};
        if (function == builtin_function::exit) {
            return {
                .arguments{exit_args},
                .result{},
            };
        }

        return {
            .arguments{io_args},
            .result{"a0"},
        };
    }

    [[nodiscard]] auto default_type() const -> const type& override {
        assert(type_i32_ != nullptr);

        return *type_i32_;
    }

    auto set_builtin_types([[maybe_unused]] const type& t_i64,
                           const type& t_i32,
                           [[maybe_unused]] const type& t_i16,
                           [[maybe_unused]] const type& t_i8,
                           [[maybe_unused]] const type& t_bool,
                           [[maybe_unused]] const type& t_void)
        -> void override {

        type_i32_ = &t_i32;
    }

    auto use_stream(std::ostream& new_stream) -> std::ostream& override {
        std::ostream& previous{os_.get()};
        os_ = new_stream;
        return previous;
    }

    auto comment([[maybe_unused]] const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void override {
        asm_line(indent, "# {}", text);
    }

    auto emit_most_efficient([[maybe_unused]] const token& src_loc_tk,
                             [[maybe_unused]] const size_t indent,
                             const std::string_view without_scratch,
                             const std::string_view with_scratch)
        -> void override {

        const auto count_instructions{
            [](const std::string_view text) -> size_t {
                size_t count{};
                for (const auto line : text | std::views::split('\n')) {
                    const std::string_view value{line};
                    const size_t start{value.find_first_not_of(" \t\r")};
                    if (start != std::string_view::npos and
                        value[start] != '#' and value.back() != ':') {
                        ++count;
                    }
                }

                return count;
            }};

        std::print(os_.get(), "{}",
                   count_instructions(without_scratch) <=
                           count_instructions(with_scratch)
                       ? without_scratch
                       : with_scratch);
    }

    [[nodiscard]] auto
    alloc_scratch_register(const token& src_loc_tk,
                           [[maybe_unused]] const size_t indent,
                           const type& type_ref) -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        for (const size_t index : scratch_registers_ | std::views::reverse) {
            const uint32_t mask{uint32_t{1} << index};
            if ((unavailable_registers_ & mask) == 0) {
                unavailable_registers_ |= mask;
                allocations_.push_back({
                    .register_index{index},
                    .type_ptr{&type_ref},
                });

                operand result{
                    make_register_operand(register_names_.at(index), type_ref)};

                result.set_allocation_register(register_names_.at(index));

                return result;
            }
        }

        throw compiler_exception{src_loc_tk, "out of RV32I scratch registers"};
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk,
                         [[maybe_unused]] const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        const size_t index{register_index(register_name)};
        const uint32_t mask{register_mask(register_name)};
        if (mask == 0 or index == 0 or index == 2 or
            (unavailable_registers_ & mask) != 0) {
            throw compiler_exception{
                src_loc_tk,
                std::format("cannot allocate register {}", register_name)};
        }
        operand result{make_register_operand(register_name, type_ref)};
        result.set_allocation_register(register_names_.at(index));
        allocations_.push_back({
            .register_index{index},
            .type_ptr{&type_ref},
        });

        unavailable_registers_ |= mask;

        return result;
    }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void override {
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto free_scratch_register([[maybe_unused]] const token& src_loc_tk,
                               [[maybe_unused]] const size_t indent,
                               const operand& reg) -> void override {

        assert(not allocations_.empty());
        const size_t index{register_index(reg.allocation_register())};
        assert(allocations_.back().register_index == index);
        unavailable_registers_ &= ~(uint32_t{1} << index);
        allocations_.pop_back();
    }

    auto finish() -> void override {
        assert(allocations_.empty());
        assert(unavailable_registers_ == 0);
        assert(not variables_base_reserved_);
        assert(not frame_base_reserved_);
    }

    [[nodiscard]] auto address_size_bytes() const -> size_t override {
        return 4;
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_scalar(src_loc_tk, src.type_ref());

        if (not(dst.is_register() or dst.is_memory()) or src.is_empty()) {
            throw compiler_exception{src_loc_tk, "invalid RV32I copy operands"};
        }

        if (dst.is_memory()) {
            validate_address(src_loc_tk, dst);
        }

        if (src.is_memory()) {
            validate_address(src_loc_tk, src);
        }

        const address_scope scope{*this, dst, src};

        const operand value{
            dst.is_register()
                ? dst
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        if (src.is_memory()) {
            const operand lowered{lower_address(src_loc_tk, indent, src)};
            const size_t width{src.type_ref().size_bytes()};
            std::string_view instruction{"lb"};
            if (width == 4) {
                instruction = "lw";
            } else if (width == 2) {
                instruction = "lh";
            } else if (src.type_ref().name() == "bool") {
                instruction = "lbu";
            }

            // load from base + displacement; lb/lh sign-extend, lbu
            // zero-extends
            asm_line(indent, "{} {}, {}({})", instruction,
                     value.base_register(), lowered.displacement(),
                     lowered.base_register());
        } else if (src.is_register()) {
            // adding zero copies a register without changing its bits
            asm_line(indent, "addi {}, {}, 0", value.base_register(),
                     src.base_register());
        } else if (src.is_immediate()) {
            // li materializes a constant using one or more RV32I instructions
            asm_line(indent, "li {}, {}", value.base_register(),
                     src.immediate());
        } else {
            throw compiler_exception{src_loc_tk, "invalid RV32I copy source"};
        }

        if (dst.is_memory()) {
            const operand lowered{lower_address(src_loc_tk, indent, dst)};
            const size_t width{dst.type_ref().size_bytes()};
            std::string_view instruction{"sb"};
            if (width == 4) {
                instruction = "sw";
            } else if (width == 2) {
                instruction = "sh";
            }

            // store the low 32, 16, or 8 bits at base + displacement
            asm_line(indent, "{} {}, {}({})", instruction,
                     value.base_register(), lowered.displacement(),
                     lowered.base_register());
        } else if (dst.type_ref().size_bytes() < 4) {
            const size_t shift{32 - (dst.type_ref().size_bytes() * 8)};
            // discard high bits, then sign-extend integers or zero-extend bool
            asm_line(indent, "slli {}, {}, {}", value.base_register(),
                     value.base_register(), shift);
            asm_line(indent, "{} {}, {}, {}",
                     dst.type_ref().name() == "bool" ? "srli" : "srai",
                     value.base_register(), value.base_register(), shift);
        }
    }

    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t size_bytes,
                          [[maybe_unused]] const operand& address)
        -> void override {
        comment(src_loc_tk, indent, "{} ({} B)", text, size_bytes);
    }

    auto comment_alias(const token& src_loc_tk, const size_t indent,
                       const std::string_view from, const std::string_view to,
                       [[maybe_unused]] const operand& address)
        -> void override {
        comment(src_loc_tk, indent, "alias {} -> {}", from, to);
    }

    auto
    compare_and_branch(const token& src_loc_tk, const size_t indent,
                       const operand& lhs, const operand& rhs,
                       const comparison_action& action,
                       const std::span<const operand> scratch_registers_to_free)
        -> void override {

        {
            const address_scope destination_scope{*this, action.destination,
                                                  operand{}};

            const address_scope scope{*this, lhs, rhs};

            const operand left{
                alloc_scratch_register(src_loc_tk, indent, lhs.type_ref())};

            const operand right{
                alloc_scratch_register(src_loc_tk, indent, lhs.type_ref())};

            copy_value(src_loc_tk, indent, left, lhs);
            copy_value(src_loc_tk, indent, right, rhs);
            const std::string& result{left.base_register()};
            const std::string& other{right.base_register()};
            const std::string_view operation{action.operation};
            bool inverted{action.inverted};
            if (operation == "==" or operation == "!=") {
                asm_line(indent, "xor {}, {}, {}", result, result, other);
                asm_line(indent, "sltiu {}, {}, 1", result, result);
                inverted = inverted != (operation == "!=");
            } else if (operation == "<" or operation == ">=") {
                asm_line(indent, "slt {}, {}, {}", result, result, other);
                inverted = inverted != (operation == ">=");
            } else {
                assert(operation == ">" or operation == "<=");
                asm_line(indent, "slt {}, {}, {}", result, other, result);
                inverted = inverted != (operation == "<=");
            }
            if (inverted) {
                asm_line(indent, "xori {}, {}, 1", result, result);
            }
            if (not action.destination.is_empty()) {
                copy_value(src_loc_tk, indent, action.destination, left);
            }
            if (not action.target.empty()) {
                asm_line(indent, "{} {}, zero, 1f",
                         action.branch_on_true ? "beq" : "bne", result);

                branch(indent, action.target);
                asm_line(indent, "1:");
            }
        }
        free_scratch_registers(src_loc_tk, indent, scratch_registers_to_free);
    }

    auto branch(const size_t indent, const std::string_view target)
        -> void override {

        asm_line(indent, "j {}", target);
    }

    auto read(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& descriptor, const operand& address,
              const operand& count) -> void override {
        constexpr int syscall_read{63};
        io_syscall(src_loc_tk, indent, dst, descriptor, address, count,
                   syscall_read);
    }

    auto write(const token& src_loc_tk, const size_t indent, const operand& dst,
               const operand& descriptor, const operand& address,
               const operand& count) -> void override {
        constexpr int syscall_write{64};
        io_syscall(src_loc_tk, indent, dst, descriptor, address, count,
                   syscall_write);
    }

    auto invoke_syscall([[maybe_unused]] const size_t indent) -> void override {
        todo();
    }

    auto
    advance_array_iteration([[maybe_unused]] const size_t indent,
                            [[maybe_unused]] const operand& iterator,
                            [[maybe_unused]] const operand& counter,
                            [[maybe_unused]] const size_t element_size_bytes,
                            [[maybe_unused]] const size_t array_count,
                            [[maybe_unused]] const std::string_view loop_label)
        -> void override {
        todo();
    }

    auto copy([[maybe_unused]] const token& src_loc_tk,
              [[maybe_unused]] const size_t indent,
              [[maybe_unused]] const operand& src,
              [[maybe_unused]] const operand& dst,
              [[maybe_unused]] const size_t size_bytes) -> void override {
        todo();
    }

    [[nodiscard]] auto
    begin_array_copy([[maybe_unused]] const token& src_loc_tk,
                     [[maybe_unused]] const size_t indent) -> operand override {
        todo();
    }

    auto set_array_copy_source([[maybe_unused]] const size_t indent,
                               [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto set_array_copy_destination([[maybe_unused]] const size_t indent,
                                    [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto end_array_copy([[maybe_unused]] const token& src_loc_tk,
                        [[maybe_unused]] const size_t indent,
                        [[maybe_unused]] const size_t element_size_bytes)
        -> void override {
        todo();
    }

    auto begin_memory_equal([[maybe_unused]] const token& src_loc_tk,
                            [[maybe_unused]] const size_t indent)
        -> operand override {
        todo();
    }

    auto set_memory_equal_left([[maybe_unused]] const size_t indent,
                               [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto set_memory_equal_right([[maybe_unused]] const size_t indent,
                                [[maybe_unused]] const operand& address)
        -> void override {
        todo();
    }

    auto end_memory_equal([[maybe_unused]] const token& src_loc_tk,
                          [[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const size_t size_bytes,
                          [[maybe_unused]] const operand& dst)
        -> void override {
        todo();
    }

    auto end_arrays_equal([[maybe_unused]] const token& src_loc_tk,
                          [[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const size_t element_size_bytes,
                          [[maybe_unused]] const operand& dst)
        -> void override {
        todo();
    }

    auto zero(const token& src_loc_tk, const size_t indent,
              const operand& destination, const size_t size_bytes)
        -> void override {

        if (size_bytes == 0) {
            return;
        }
        const address_scope scope{*this, destination, operand{}};

        const operand dst_pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        address_of(src_loc_tk, indent, dst_pointer, destination);

        copy_value(src_loc_tk, indent, remaining,
                   operand::imm(std::format("{}", size_bytes), default_type()));

        asm_line(indent, "1:");
        asm_line(indent, "sb zero, 0({})", dst_pointer.base_register());

        asm_line(indent, "addi {}, {}, 1", dst_pointer.base_register(),
                 dst_pointer.base_register());

        asm_line(indent, "addi {}, {}, -1", remaining.base_register(),
                 remaining.base_register());

        asm_line(indent, "bnez {}, 1b", remaining.base_register());
    }

    auto add_subtract(const token& src_loc_tk, const size_t indent,
                      const char operation, const operand& dst,
                      const operand& src) -> void override {

        assert(operation == '+' or operation == '-');

        binary_operation(src_loc_tk, indent, operation == '+' ? "add" : "sub",
                         dst, src);
    }

    auto bitwise(const token& src_loc_tk, const size_t indent,
                 const char operation, const operand& dst, const operand& src)
        -> void override {

        assert(operation == '&' or operation == '|' or operation == '^');
        std::string_view instruction{"xor"};
        if (operation == '&') {
            instruction = "and";
        } else if (operation == '|') {
            instruction = "or";
        }
        binary_operation(src_loc_tk, indent, instruction, dst, src);
    }

    auto multiply([[maybe_unused]] const token& src_loc_tk,
                  [[maybe_unused]] const size_t indent,
                  [[maybe_unused]] const operand& product,
                  [[maybe_unused]] const operand& factor,
                  [[maybe_unused]] const bool reuse_source = false)
        -> void override {
        todo();
    }

    auto validate_shift_operand(const token& src_loc_tk,
                                const operand& count) const -> void override {

        validate_scalar(src_loc_tk, count.type_ref());
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const char operation, const operand& dst, const operand& count)
        -> void override {

        assert(operation == '<' or operation == '>');
        validate_shift_operand(src_loc_tk, count);

        binary_operation(src_loc_tk, indent, operation == '<' ? "sll" : "sra",
                         dst, count, true);
    }

    auto
    validate_division_operand([[maybe_unused]] const token& src_loc_tk,
                              [[maybe_unused]] const operand& divisor) const
        -> void override {
        todo();
    }

    auto divide([[maybe_unused]] const token& src_loc_tk,
                [[maybe_unused]] const size_t indent,
                [[maybe_unused]] const char operation,
                [[maybe_unused]] const operand& dst,
                [[maybe_unused]] const operand& divisor) -> void override {
        todo();
    }

    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void override {

        copy_value(src_loc_tk, indent, dst,
                   operand::imm(value ? "1" : "0", default_type()));
    }

    auto label(const size_t indent, const std::string_view label)
        -> void override {
        asm_line(indent, "{}:", label);
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {
        if (not(dst.is_register() or dst.is_memory()) or
            dst.type_ref().size_bytes() != 4) {
            throw compiler_exception{
                src_loc_tk, "RV32I address destination must be 32-bit storage"};
        }
        const address_scope scope{*this, dst, address};
        const operand value{
            dst.is_register()
                ? dst
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand lowered{lower_address(src_loc_tk, indent, address)};
        // compute base + the remaining 12-bit displacement without reading
        // memory
        asm_line(indent, "addi {}, {}, {}", value.base_register(),
                 lowered.base_register(), lowered.displacement());
        if (dst.is_memory()) {
            copy_value(src_loc_tk, indent, dst, value);
        }
    }

    auto unary(const size_t indent, const char operation,
               const operand& destination) -> void override {

        assert(operation == '-' or operation == '~');
        const address_scope scope{*this, destination, operand{}};

        const operand value{
            alloc_scratch_register(token{}, indent, destination.type_ref())};

        copy_value(token{}, indent, value, destination);
        if (operation == '-') {

            asm_line(indent, "sub {}, zero, {}", value.base_register(),
                     value.base_register());

        } else {

            asm_line(indent, "xori {}, {}, -1", value.base_register(),
                     value.base_register());
        }
        copy_value(token{}, indent, destination, value);
    }

    [[nodiscard]] auto
    can_encode_index_scale([[maybe_unused]] const size_t size_bytes) const
        -> bool override {
        return false;
    }

    auto scale_index([[maybe_unused]] const token& src_loc_tk,
                     [[maybe_unused]] const size_t indent,
                     [[maybe_unused]] const operand& index,
                     [[maybe_unused]] const size_t element_size_bytes)
        -> void override {
        todo();
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {
        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);
        asm_line(indent, "li a7, 93");
        asm_line(indent, "ecall");
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        return "s0";
    }

    [[nodiscard]] auto is_variables_base(const operand& reg) const
        -> bool override {
        return not reg.is_indexed() and
               register_index(reg.base_register()) == s0_register_index;
    }

    auto address_of_variable(const token& src_loc_tk, const size_t indent,
                             const operand& dst, const int64_t offset,
                             const type& value_type) -> void override {
        address_of(
            src_loc_tk, indent, dst,
            operand::mem(variables_base_register(), {}, 1, offset, value_type));
    }

    auto reserve_variables_base() -> void override {
        assert(not variables_base_reserved_);
        static_cast<void>(alloc_named_register(
            token{}, 0, variables_base_register(), default_type()));
        variables_base_reserved_ = true;
    }

    auto release_variables_base() -> void override {
        assert(variables_base_reserved_);
        operand base{
            make_register_operand(variables_base_register(), default_type())};
        base.set_allocation_register(variables_base_register());
        free_named_register(token{}, 0, base);
        variables_base_reserved_ = false;
    }

    [[nodiscard]] auto frame_base_register() const
        -> std::string_view override {
        return "s1";
    }

    auto reserve_frame_base() -> void override {
        assert(not frame_base_reserved_);
        static_cast<void>(alloc_named_register(
            token{}, 0, frame_base_register(), default_type()));
        frame_base_reserved_ = true;
    }

    auto release_frame_base() -> void override {
        assert(frame_base_reserved_);
        operand base{
            make_register_operand(frame_base_register(), default_type())};
        base.set_allocation_register(frame_base_register());
        free_named_register(token{}, 0, base);
        frame_base_reserved_ = false;
    }

    auto call_function([[maybe_unused]] const size_t indent,
                       [[maybe_unused]] const std::string_view label,
                       [[maybe_unused]] const operand& frame_address)
        -> void override {
        todo();
    }

    auto return_function([[maybe_unused]] const size_t indent)
        -> void override {
        todo();
    }

    auto
    check_frame_capacity([[maybe_unused]] const token& src_loc_tk,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const operand& frame_address,
                         [[maybe_unused]] const operand& frame_size_bytes,
                         [[maybe_unused]] const std::string_view failure_label,
                         [[maybe_unused]] const bool enabled = {})
        -> void override {
        todo();
    }

    auto define_constant([[maybe_unused]] const std::string_view name,
                         [[maybe_unused]] const size_t value) -> void override {
        todo();
    }

    auto program_start() -> void override {
        asm_line(0, ".option norvc");
        asm_line(0, ".option norelax");
        asm_line(0, ".text");
        asm_line(0, ".globl _start");
        label(0, "_start");
        asm_line(1, "la s0, dat");
    }

    auto program_end() -> void override {
        exit(token{}, 1, operand::imm("0", default_type()));
    }

    auto check_bounds([[maybe_unused]] const token& src_loc_tk,
                      [[maybe_unused]] const size_t indent,
                      [[maybe_unused]] const operand& reg_to_check,
                      [[maybe_unused]] const size_t array_count,
                      [[maybe_unused]] const bool allow_end,
                      [[maybe_unused]] const operand& reg_count,
                      [[maybe_unused]] const bounds_check_options& options)
        -> void override {
        todo();
    }

    auto emit_bounds_failure_handler([[maybe_unused]] const bool with_line)
        -> void override {
        todo();
    }

    [[nodiscard]] auto data_alignment() const -> size_t override {
        return data_alignment_;
    }

    auto begin_data(const size_t alignment) -> void override {
        asm_line(0, ".data");
        asm_line(0, ".balign {}", alignment);
        label(0, "dat");
    }

    auto reserve_variables(const size_t alignment, const size_t size_bytes)
        -> void override {
        label(0, "dat.end");
        asm_line(0, ".balign {}", alignment);
        label(0, "vars");
        asm_line(0, ".zero {}", size_bytes);
        label(0, "vars.end");
    }

    auto emit_data(const size_t element_size_bytes,
                   const data_initializer& value) -> void override {
        emit_repeated_data(element_size_bytes, 1, value);
    }

    auto emit_string_data([[maybe_unused]] const std::string_view value)
        -> void override {
        todo();
    }

    auto emit_zero_data(const size_t size_bytes) const -> void override {
        asm_line(0, ".zero {}", size_bytes);
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const data_initializer& value) const
        -> void override {
        std::string_view directive;
        switch (element_size_bytes) {
        case 1:
            directive = ".byte";
            break;

        case 2:
            directive = ".half";
            break;

        case 4:
            directive = ".word";
            break;

        default:
            throw compiler_exception{
                token{}, "RV32I data elements must be 1, 2, or 4 bytes"};
        }
        asm_line(0, ".rept {}", count);
        asm_line(0, "{} {}{}", directive, value.uops, value.value);
        asm_line(0, ".endr");
    }

    [[nodiscard]] auto register_size_bytes(const std::string_view name) const
        -> size_t override {
        return register_index(name) == register_names_.size() ? 0 : 4;
    }

    [[nodiscard]] auto
    allocated_register_type(const std::string_view name) const
        -> const type* override {
        const size_t index{register_index(name)};
        for (const allocation& entry : allocations_) {
            if (entry.register_index == index) {
                return entry.type_ptr;
            }
        }

        return nullptr;
    }

    [[nodiscard]] auto make_register_operand(const std::string_view name,
                                             const type& value_type) const
        -> operand override {
        validate_scalar(token{}, value_type);
        const size_t index{register_index(name)};
        if (index == register_names_.size()) {
            throw compiler_exception{token{}, "invalid RV32I register"};
        }

        return operand::reg(register_names_.at(index), value_type);
    }

    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(data_initializer&)> next)
        -> void override {
        data_initializer value;
        while (next(value)) {
            emit_data(element_size_bytes, value);
        }
    }
};
