#pragma once

#include <array>
#include <bit>
#include <charconv>
#include <cstdio>
#include <iostream>
#include <limits>
#include <optional>
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
    bool multiply_helper_used_{};
    bool divide_helper_used_{};
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
    [[nodiscard]] auto
    lower_address(const token& src_loc_tk, const size_t indent,
                  const operand& address, const operand& destination = {})
        -> operand {

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

        // an unscaled index alone is already a usable base register
        if (base.empty() and not index.empty() and address.scale() == 1 and
            offset >= immediate_min and offset <= immediate_max) {

            return operand::mem(index, {}, 1, offset, address.type_ref());
        }

        // reuse the output only when address inputs survive its construction
        const bool reuse_destination{
            destination.is_register() and
            register_index(destination.base_register()) != 0 and
            register_index(destination.base_register()) !=
                register_index(base) and
            register_index(destination.base_register()) !=
                register_index(index)};

        const operand result{
            reuse_destination
                ? destination
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        const std::string& result_name{result.base_register()};

        const bool base_is_register{register_index(base) !=
                                    register_names_.size()};

        if (index.empty()) {
            if (not base.empty() and not base_is_register) {
                asm_line(indent, "la {}, {}", result_name, base);
            } else {

                const int32_t bits{
                    std::bit_cast<int32_t>(static_cast<uint32_t>(offset))};

                asm_line(indent, "li {}, {}", result_name, bits);
                if (not base.empty()) {

                    asm_line(indent, "add {}, {}, {}", result_name, result_name,
                             base);
                }

                return operand::mem(result_name, {}, 1, 0, address.type_ref());
            }
        } else if (address.scale() == 1) {
            if (base.empty()) {
                asm_line(indent, "addi {}, {}, 0", result_name, index);
            } else if (base_is_register) {
                asm_line(indent, "add {}, {}, {}", result_name, base, index);
            } else {
                asm_line(indent, "la {}, {}", result_name, base);

                asm_line(indent, "add {}, {}, {}", result_name, result_name,
                         index);
            }
        } else {

            asm_line(indent, "slli {}, {}, {}", result_name, index,
                     std::countr_zero(address.scale()));

            if (base_is_register) {

                asm_line(indent, "add {}, {}, {}", result_name, result_name,
                         base);

            } else if (not base.empty()) {

                const operand symbol{
                    alloc_scratch_register(src_loc_tk, indent, default_type())};

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

    [[nodiscard]] static auto immediate_value(const operand& value)
        -> std::optional<int32_t> {

        if (not value.is_immediate()) {
            return {};
        }
        const std::string_view text{value.immediate()};
        const size_t digits{text.find_first_not_of("-~")};
        if (digits == std::string_view::npos) {
            return {};
        }
        const std::string_view number{text.substr(digits)};
        uint64_t magnitude{};
        const char* const end{std::to_address(number.end())};

        const std::from_chars_result parsed{
            std::from_chars(std::to_address(number.begin()), end, magnitude)};

        if (parsed.ec != std::errc{} or parsed.ptr != end) {
            return {};
        }
        uint32_t bits{static_cast<uint32_t>(magnitude)};
        for (const char operation :
             text.substr(0, digits) | std::views::reverse) {
            bits = operation == '-' ? uint32_t{} - bits : ~bits;
        }

        return std::bit_cast<int32_t>(bits);
    }

    [[nodiscard]] static auto same_memory(const operand& left,
                                          const operand& right) -> bool {

        const auto same_register = [](const std::string_view first,
                                      const std::string_view second) -> bool {
            return first == second or
                   (register_index(first) != register_names_.size() and
                    register_index(first) == register_index(second));
        };

        return left.is_memory() and right.is_memory() and
               left.type_ref().name() == right.type_ref().name() and
               same_register(left.base_register(), right.base_register()) and
               same_register(left.index_register(), right.index_register()) and
               left.scale() == right.scale() and
               left.displacement() == right.displacement();
    }

    auto store_operation_result(const size_t indent, const operand& destination,
                                const operand& address, const operand& value,
                                const bool normalize) const -> void {

        const size_t width{destination.type_ref().size_bytes()};
        if (destination.is_memory()) {
            std::string_view instruction{"sb"};
            if (width == 4) {
                instruction = "sw";
            } else if (width == 2) {
                instruction = "sh";
            }

            asm_line(indent, "{} {}, {}({})", instruction,
                     value.base_register(), address.displacement(),
                     address.base_register());

        } else if (width < 4 and normalize) {
            const size_t shift{32 - (width * 8)};

            asm_line(indent, "slli {}, {}, {}", value.base_register(),
                     value.base_register(), shift);

            asm_line(indent, "{} {}, {}, {}",
                     destination.type_ref().name() == "bool" ? "srli" : "srai",
                     value.base_register(), value.base_register(), shift);
        }
    }

    auto store_constant_result(const token& src_loc_tk, const size_t indent,
                               const operand& destination,
                               const int32_t constant) -> void {

        const address_scope scope{*this, destination, operand{}};

        const operand address{
            destination.is_memory()
                ? lower_address(src_loc_tk, indent, destination)
                : operand{}};

        if (destination.is_memory() and constant == 0) {

            store_operation_result(indent, destination, address,
                                   operand::reg("zero", default_type()), false);

            return;
        }

        const operand value{
            destination.is_register()
                ? destination
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        asm_line(indent, "li {}, {}", value.base_register(), constant);
        store_operation_result(indent, destination, address, value, false);
    }

    auto binary_operation(const token& src_loc_tk, const size_t indent,
                          const std::string_view instruction,
                          const operand& destination, const operand& src)
        -> void {

        validate_scalar(src_loc_tk, destination.type_ref());
        validate_scalar(src_loc_tk, src.type_ref());
        if (not(destination.is_register() or destination.is_memory())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I operation destination"};
        }
        if (destination.is_memory()) {
            validate_address(src_loc_tk, destination);
        }
        if (src.is_memory()) {
            validate_address(src_loc_tk, src);
        }
        const bool arithmetic{instruction == "add" or instruction == "sub"};
        const size_t width{destination.type_ref().size_bytes()};
        std::optional<int32_t> constant{immediate_value(src)};
        const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                            ((4 - width) * 8)};
        if (constant.has_value()) {
            uint32_t bits{static_cast<uint32_t>(*constant) & mask};
            const uint32_t sign{uint32_t{1} << ((width * 8) - 1)};
            if (destination.type_ref().name() != "bool" and
                (bits & sign) != 0) {
                bits |= ~mask;
            }
            constant = std::bit_cast<int32_t>(bits);
            const bool all_bits{(bits & mask) == mask};
            if ((*constant == 0 and instruction != "and") or
                (all_bits and instruction == "and")) {

                return;
            }
            if ((*constant == 0 and instruction == "and") or
                (all_bits and instruction == "or")) {

                store_constant_result(src_loc_tk, indent, destination,
                                      *constant);

                return;
            }
        }

        const bool identical{
            same_memory(destination, src) or
            (destination.is_register() and src.is_register() and
             register_index(destination.base_register()) ==
                 register_index(src.base_register()) and
             destination.type_ref().name() == src.type_ref().name())};

        if (identical) {
            if (instruction == "and" or instruction == "or") {
                return;
            }
            if (instruction == "sub" or instruction == "xor") {
                store_constant_result(src_loc_tk, indent, destination, 0);

                return;
            }
        }
        const address_scope scope{*this, destination, src};

        const operand address{
            destination.is_memory()
                ? lower_address(src_loc_tk, indent, destination)
                : operand{}};

        const operand left{
            destination.is_register()
                ? destination
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        if (destination.is_memory()) {
            copy_value(src_loc_tk, indent, left, address);
        }
        bool normalize{true};
        if (not arithmetic) {
            if (constant.has_value()) {
                const int64_t limit{
                    static_cast<int64_t>(uint64_t{1} << ((width * 8) - 1))};
                normalize = *constant < -limit or *constant >= limit;
                if (destination.type_ref().name() == "bool") {

                    normalize =
                        *constant < 0 or
                        std::cmp_greater(*constant,
                                         std::numeric_limits<uint8_t>::max());
                }
            } else {
                normalize = src.type_ref().size_bytes() > width or
                            (src.type_ref().name() == "bool") !=
                                (destination.type_ref().name() == "bool");
            }
        }
        int64_t immediate{constant.value_or(0)};
        if (instruction == "sub") {
            immediate = -immediate;
        }

        if (constant.has_value() and immediate >= immediate_min and
            immediate <= immediate_max) {

            asm_line(indent, "{}i {}, {}, {}", arithmetic ? "add" : instruction,
                     left.base_register(), left.base_register(), immediate);

        } else if (constant.has_value() and arithmetic and
                   immediate >= 2 * immediate_min and
                   immediate <= 2 * immediate_max) {

            const int64_t first{immediate < 0 ? immediate_min : immediate_max};

            asm_line(indent, "addi {}, {}, {}", left.base_register(),
                     left.base_register(), first);

            asm_line(indent, "addi {}, {}, {}", left.base_register(),
                     left.base_register(), immediate - first);

        } else {
            const bool reuse_left{same_memory(destination, src)};
            operand right{left};
            if (not reuse_left) {

                right = src.is_register()
                            ? src
                            : alloc_scratch_register(src_loc_tk, indent,
                                                     default_type());

                if (not src.is_register()) {
                    if (constant.has_value()) {
                        asm_line(indent, "li {}, {}", right.base_register(),
                                 *constant);
                    } else {
                        copy_value(src_loc_tk, indent, right, src);
                    }
                }
            }

            asm_line(indent, "{} {}, {}, {}", instruction, left.base_register(),
                     left.base_register(), right.base_register());
        }

        store_operation_result(indent, destination, address, left, normalize);
    }

    auto call_arithmetic_helper(const token& src_loc_tk, const size_t indent,
                                const operand& destination,
                                const operand& source, const bool division,
                                const bool remainder = {}) -> void {

        const address_scope scope{*this, destination, source};
        const uint32_t live{unavailable_registers_};
        constexpr std::array<std::string_view, 8> clobbers{
            "ra", "a0", "a1", "t0", "t1", "t2", "t3", "t4"};
        const size_t clobber_count{division ? clobbers.size() : 5};
        std::vector<std::string_view> saved;
        for (const std::string_view name :
             std::span{clobbers}.first(clobber_count)) {
            // protect live values and keep operand staging outside the helper
            // registers
            if ((live & register_mask(name)) != 0 and
                (not destination.is_register() or
                 register_index(destination.base_register()) !=
                     register_index(name))) {
                saved.push_back(name);
            }
            unavailable_registers_ |= register_mask(name);
        }

        const operand left{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand right{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        copy_value(src_loc_tk, indent, left, destination);
        copy_value(src_loc_tk, indent, right, source);
        constexpr size_t stack_alignment{16};
        constexpr size_t word_size{4};
        const size_t stack_bytes{
            (((saved.size() * word_size) + stack_alignment - 1) /
             stack_alignment) *
            stack_alignment};
        // allocate an aligned save area only when a clobbered register is live
        if (stack_bytes != 0) {
            asm_line(indent, "addi sp, sp, -{}", stack_bytes);
        }
        for (const auto [index, name] : std::views::enumerate(saved)) {
            asm_line(indent, "sw {}, {}(sp)", name,
                     static_cast<size_t>(index) * word_size);
        }
        asm_line(indent, "mv a0, {}", left.base_register());
        asm_line(indent, "mv a1, {}", right.base_register());
        asm_line(indent, "call {}",
                 division ? ".Lbaz_divide" : ".Lbaz_multiply");
        asm_line(indent, "mv {}, {}", left.base_register(),
                 remainder ? "a1" : "a0");
        for (const auto [index, name] : std::views::enumerate(saved)) {
            asm_line(indent, "lw {}, {}(sp)", name,
                     static_cast<size_t>(index) * word_size);
        }
        // restore sp before writing a possibly stack-relative destination
        if (stack_bytes != 0) {
            asm_line(indent, "addi sp, sp, {}", stack_bytes);
        }
        copy_value(src_loc_tk, indent, destination, left);
    }

    auto emit_arithmetic_helpers() const -> void {
        // unused helpers contribute no code
        if (multiply_helper_used_) {
            asm_line(0, ".Lbaz_multiply:");
            asm_line(1, "mv t0, a0");
            asm_line(1, "li a0, 0");
            asm_line(1, "beqz a1, 3f");
            asm_line(0, "1:");
            asm_line(1, "andi t1, a1, 1");
            asm_line(1, "beqz t1, 2f");
            asm_line(1, "add a0, a0, t0");
            asm_line(0, "2:");
            asm_line(1, "slli t0, t0, 1");
            asm_line(1, "srli a1, a1, 1");
            asm_line(1, "bnez a1, 1b");
            asm_line(0, "3:");
            asm_line(1, "ret");
        }
        // divide and remainder share magnitude division and sign restoration
        if (divide_helper_used_) {
            asm_line(0, ".Lbaz_divide:");
            asm_line(1, "beqz a1, 5f");
            asm_line(1, "srai t2, a0, 31");
            asm_line(1, "srai t1, a1, 31");
            asm_line(1, "xor a0, a0, t2");
            asm_line(1, "sub a0, a0, t2");
            asm_line(1, "xor a1, a1, t1");
            asm_line(1, "sub a1, a1, t1");
            asm_line(1, "xor t1, t1, t2");
            asm_line(1, "li t0, 0");
            asm_line(1, "li t3, 32");
            asm_line(0, "1:");
            asm_line(1, "srli t4, a0, 31");
            asm_line(1, "slli t0, t0, 1");
            asm_line(1, "or t0, t0, t4");
            asm_line(1, "slli a0, a0, 1");
            asm_line(1, "bltu t0, a1, 2f");
            asm_line(1, "sub t0, t0, a1");
            asm_line(1, "ori a0, a0, 1");
            asm_line(0, "2:");
            asm_line(1, "addi t3, t3, -1");
            asm_line(1, "bnez t3, 1b");
            asm_line(1, "xor a0, a0, t1");
            asm_line(1, "sub a0, a0, t1");
            asm_line(1, "xor a1, t0, t2");
            asm_line(1, "sub a1, a1, t2");
            asm_line(1, "ret");
            asm_line(0, "5:");
            asm_line(1, "ebreak");
            asm_line(1, "j 5b");
        }
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

        const std::optional<int32_t> constant{immediate_value(src)};
        // known constants can be truncated and extended before emission
        if (constant.has_value()) {
            const size_t bits{dst.type_ref().size_bytes() * 8};

            constexpr size_t register_bits{
                std::numeric_limits<uint32_t>::digits};

            const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                                (register_bits - bits)};

            uint32_t value{static_cast<uint32_t>(*constant) & mask};
            // signed destinations need the stored sign bit extended
            if (dst.type_ref().name() != "bool" and
                (value & (uint32_t{1} << (bits - 1))) != 0) {

                value |= ~mask;
            }

            store_constant_result(src_loc_tk, indent, dst,
                                  std::bit_cast<int32_t>(value));

            return;
        }

        operand value{dst};
        if (not dst.is_register()) {

            value = src.is_register() ? src
                                      : alloc_scratch_register(
                                            src_loc_tk, indent, default_type());
        }

        if (src.is_memory()) {
            // a load may build its address in the register it will overwrite
            const operand lowered{
                lower_address(src_loc_tk, indent, src, value)};

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
            if (register_index(value.base_register()) !=
                register_index(src.base_register())) {

                asm_line(indent, "addi {}, {}, 0", value.base_register(),
                         src.base_register());
            }
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
        } else if (dst.type_ref().size_bytes() < 4 and
                   (src.is_immediate() or
                    src.type_ref().size_bytes() > dst.type_ref().size_bytes() or
                    ((src.type_ref().name() == "bool") !=
                         (dst.type_ref().name() == "bool") and
                     src.type_ref().size_bytes() ==
                         dst.type_ref().size_bytes()))) {

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

            validate_scalar(src_loc_tk, lhs.type_ref());
            validate_scalar(src_loc_tk, rhs.type_ref());

            const auto prepare = [&](const operand& source) -> operand {
                // matching register representations need no conversion
                if (source.is_register() and
                    source.type_ref().name() == lhs.type_ref().name()) {
                    return source;
                }
                // zero has the same representation at every supported width
                if (immediate_value(source) == 0) {
                    return operand::reg("zero", lhs.type_ref());
                }

                const operand value{
                    alloc_scratch_register(src_loc_tk, indent, lhs.type_ref())};

                copy_value(src_loc_tk, indent, value, source);

                return value;
            };

            const std::string_view operation{action.operation};
            std::optional<int32_t> constant{immediate_value(rhs)};
            // comparisons convert the right operand to the left operand's width
            if (constant.has_value()) {
                constexpr size_t register_bits{
                    std::numeric_limits<uint32_t>::digits};
                const size_t bits{lhs.type_ref().size_bytes() * 8};

                const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                                    (register_bits - bits)};

                uint32_t value{static_cast<uint32_t>(*constant) & mask};
                // signed operands extend the narrowed sign bit
                if (lhs.type_ref().name() != "bool" and
                    (value & (uint32_t{1} << (bits - 1))) != 0) {

                    value |= ~mask;
                }
                constant = std::bit_cast<int32_t>(value);
            }
            const bool equality{operation == "==" or operation == "!="};

            const bool inclusive_threshold{operation == ">" or
                                           operation == "<="};

            int64_t immediate{constant.value_or(0)};
            // x > c and x <= c use the signed threshold c + 1
            if (inclusive_threshold) {
                ++immediate;
            }

            const bool use_immediate{
                not action.destination.is_empty() and constant.has_value() and
                immediate >= immediate_min and immediate <= immediate_max};

            const operand left{prepare(lhs)};

            const operand right{use_immediate
                                    ? operand::reg("zero", lhs.type_ref())
                                    : prepare(rhs)};

            // branch-only comparisons do not need a materialized boolean
            if (action.destination.is_empty()) {
                // no target means the comparison result is discarded
                if (not action.target.empty()) {
                    std::string_view instruction;
                    std::string_view first{left.base_register()};
                    std::string_view second{right.base_register()};
                    bool inverted{action.inverted != not action.branch_on_true};
                    // equality and inequality share one branch pair
                    if (operation == "==" or operation == "!=") {
                        inverted = inverted != (operation == "!=");
                        instruction = inverted ? "beq" : "bne";
                    } else {
                        // ordered comparisons use signed blt/bge, swapping for
                        // > and <=
                        if (operation == ">" or operation == "<=") {
                            std::swap(first, second);
                        }

                        inverted = inverted !=
                                   (operation == ">=" or operation == "<=");

                        instruction = inverted ? "blt" : "bge";
                    }

                    asm_line(indent, "{} {}, {}, 1f", instruction, first,
                             second);

                    branch(indent, action.target);
                    asm_line(indent, "1:");
                }
            } else {
                // a boolean is required; prefer its output register or an owned
                // temporary
                operand value{action.destination};
                if (not value.is_register()) {
                    // materialized operands are ours to overwrite after the
                    // comparison
                    if (not left.allocation_register().empty() and
                        not lhs.is_register()) {
                        value = left;
                    } else if (not right.allocation_register().empty() and
                               not rhs.is_register()) {
                        // only the right operand supplied a reusable temporary
                        value = right;
                    } else {
                        // both operands are live inputs or zero, so reserve a
                        // result

                        value = alloc_scratch_register(src_loc_tk, indent,
                                                       default_type());
                    }
                }
                const std::string& result{value.base_register()};
                bool inverted{action.inverted};
                // equality needs a zero test, with xor only for a nonzero
                // operand
                if (equality) {
                    std::string_view tested{left.base_register()};
                    // an immediate zero can be tested without transforming the
                    // input
                    if (use_immediate) {
                        // nonzero small constants fit directly in xori
                        if (immediate != 0) {

                            asm_line(indent, "xori {}, {}, {}", result,
                                     left.base_register(), immediate);

                            tested = result;
                        }
                    } else if (register_index(right.base_register()) == 0) {
                        // the right operand is zero, so test the left operand
                        tested = left.base_register();
                    } else if (register_index(left.base_register()) == 0) {
                        // the left operand is zero, so test the right operand
                        tested = right.base_register();
                    } else {
                        // neither operand is zero and the constant did not fit

                        asm_line(indent, "xor {}, {}, {}", result,
                                 left.base_register(), right.base_register());

                        tested = result;
                    }

                    inverted = inverted != (operation == "!=");
                    // inverted equality is a nonzero test, not a second boolean
                    // inversion
                    if (inverted) {
                        asm_line(indent, "sltu {}, zero, {}", result, tested);
                    } else {
                        // plain equality tests whether the xor is zero
                        asm_line(indent, "sltiu {}, {}, 1", result, tested);
                    }
                } else {
                    // encodable thresholds avoid materializing a constant
                    // register
                    if (use_immediate) {

                        asm_line(indent, "slti {}, {}, {}", result,
                                 left.base_register(), immediate);

                        inverted =
                            inverted != (operation == ">=" or operation == ">");
                    } else {
                        // other thresholds use register comparison and operand
                        // order

                        asm_line(indent, "slt {}, {}, {}", result,
                                 inclusive_threshold ? right.base_register()
                                                     : left.base_register(),
                                 inclusive_threshold ? left.base_register()
                                                     : right.base_register());

                        inverted = inverted !=
                                   (operation == ">=" or operation == "<=");
                    }

                    // inclusive comparisons or explicit inversion complement
                    // the result
                    if (inverted) {
                        asm_line(indent, "xori {}, {}, 1", result, result);
                    }
                }
                // memory results require a store; register results are already
                // in place
                if (action.destination.is_memory()) {
                    copy_value(src_loc_tk, indent, action.destination, value);
                }
                // some callers request both a stored boolean and a branch
                if (not action.target.empty()) {

                    asm_line(indent, "{} {}, zero, 1f",
                             action.branch_on_true ? "beq" : "bne", result);

                    branch(indent, action.target);
                    asm_line(indent, "1:");
                }
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
        constexpr size_t direct_store_limit{4};
        // tiny fills need neither a loop counter nor a moving pointer
        if (size_bytes <= direct_store_limit) {
            for (size_t offset{}; offset < size_bytes; ++offset) {
                operand address{destination};
                address.increment_offset(static_cast<int64_t>(offset));
                const operand lowered{
                    lower_address(src_loc_tk, indent, address)};

                asm_line(indent, "sb zero, {}({})", lowered.displacement(),
                         lowered.base_register());
            }

            return;
        }

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

    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  [[maybe_unused]] const bool reuse_source = false)
        -> void override {

        validate_scalar(src_loc_tk, product.type_ref());
        validate_scalar(src_loc_tk, factor.type_ref());
        // the product must be writable storage
        if (not(product.is_register() or product.is_memory())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I multiply destination"};
        }
        // validate memory operands even when a constant eliminates the
        // operation
        if (product.is_memory()) {
            validate_address(src_loc_tk, product);
        }
        if (factor.is_memory()) {
            validate_address(src_loc_tk, factor);
        }
        const std::optional<int32_t> constant{immediate_value(factor)};
        constexpr size_t register_bits{std::numeric_limits<uint32_t>::digits};
        const size_t bits{product.type_ref().size_bytes() * 8};

        const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                            (register_bits - bits)};

        const uint32_t multiplier{static_cast<uint32_t>(constant.value_or(0)) &
                                  mask};
        // variable factors use the shared runtime helper
        if (not constant.has_value()) {
            multiply_helper_used_ = true;
            call_arithmetic_helper(src_loc_tk, indent, product, factor, false);

            return;
        }
        // constant zero and one need no multiplication machinery
        if (constant.has_value()) {
            if (multiplier == 0) {
                store_constant_result(src_loc_tk, indent, product, 0);

                return;
            }
            if (multiplier == 1) {
                return;
            }
            // all low bits set is multiplication by minus one at this width
            if (multiplier == mask) {
                unary(indent, '-', product);

                return;
            }
            // a power of two requires only a shift
            if (std::has_single_bit(multiplier)) {

                shift(src_loc_tk, indent, '<', product,
                      operand::imm(
                          std::format("{}", std::countr_zero(multiplier)),
                          default_type()));

                return;
            }
        }
        const address_scope scope{*this, product, factor};

        const operand address{product.is_memory()
                                  ? lower_address(src_loc_tk, indent, product)
                                  : operand{}};

        const operand result{
            product.is_register()
                ? product
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand left{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        copy_value(src_loc_tk, indent, left,
                   product.is_memory() ? address : product);
        // known multipliers use an unrolled sequence of shifts and adds
        bool initialized{};
        int pending_shift{};
        for (unsigned bit{static_cast<unsigned>(std::bit_width(multiplier)) -
                          1};
             bit != 0;) {
            --bit;
            ++pending_shift;
            // emit a shift when the next set bit needs an addition
            if ((multiplier & (uint32_t{1} << bit)) != 0) {

                asm_line(indent, "slli {}, {}, {}", result.base_register(),
                         initialized ? result.base_register()
                                     : left.base_register(),
                         pending_shift);

                asm_line(indent, "add {}, {}, {}", result.base_register(),
                         result.base_register(), left.base_register());

                pending_shift = 0;
                initialized = true;
            }
        }
        // trailing zero bits require only a final shift
        if (pending_shift != 0) {

            asm_line(indent, "slli {}, {}, {}", result.base_register(),
                     result.base_register(), pending_shift);
        }
        store_operation_result(indent, product, address, result, true);
    }

    auto validate_shift_operand(const token& src_loc_tk,
                                const operand& count) const -> void override {

        validate_scalar(src_loc_tk, count.type_ref());
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const char operation, const operand& dst, const operand& count)
        -> void override {

        assert(operation == '<' or operation == '>');
        validate_scalar(src_loc_tk, dst.type_ref());
        validate_shift_operand(src_loc_tk, count);
        if (not(dst.is_register() or dst.is_memory())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I shift destination"};
        }
        if (dst.is_memory()) {
            validate_address(src_loc_tk, dst);
        }
        const std::optional<int32_t> constant{immediate_value(count)};
        const uint32_t shift_count{static_cast<uint32_t>(constant.value_or(0)) &
                                   31U};
        const size_t bits{dst.type_ref().size_bytes() * 8};
        if (constant.has_value()) {
            if (shift_count == 0) {
                return;
            }
            if (shift_count >= bits and
                (operation == '<' or dst.type_ref().name() == "bool")) {
                store_constant_result(src_loc_tk, indent, dst, 0);

                return;
            }
        }
        const address_scope scope{*this, dst, count};
        const operand address{dst.is_memory()
                                  ? lower_address(src_loc_tk, indent, dst)
                                  : operand{}};

        const operand value{
            dst.is_register()
                ? dst
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        if (dst.is_memory()) {
            copy_value(src_loc_tk, indent, value, address);
        }
        bool normalize{operation == '<'};
        constexpr size_t register_bits{std::numeric_limits<uint32_t>::digits};
        if (constant.has_value() and operation == '<' and
            bits < register_bits and dst.is_register()) {

            asm_line(indent, "slli {}, {}, {}", value.base_register(),
                     value.base_register(), register_bits - bits + shift_count);

            asm_line(indent, "{} {}, {}, {}",
                     dst.type_ref().name() == "bool" ? "srli" : "srai",
                     value.base_register(), value.base_register(),
                     register_bits - bits);

            normalize = false;
        } else if (count.is_immediate()) {

            asm_line(indent, "{} {}, {}, (({}) & 31)",
                     operation == '<' ? "slli" : "srai", value.base_register(),
                     value.base_register(), count.immediate());

        } else {

            operand amount{value};
            if (not same_memory(dst, count)) {

                amount = count.is_register()
                             ? count
                             : alloc_scratch_register(src_loc_tk, indent,
                                                      default_type());

                if (not count.is_register()) {
                    copy_value(src_loc_tk, indent, amount, count);
                }
            }

            asm_line(indent, "{} {}, {}, {}", operation == '<' ? "sll" : "sra",
                     value.base_register(), value.base_register(),
                     amount.base_register());
        }
        store_operation_result(indent, dst, address, value, normalize);
    }

    auto validate_division_operand(const token& src_loc_tk,
                                   const operand& divisor) const
        -> void override {

        validate_scalar(src_loc_tk, divisor.type_ref());
    }

    auto divide(const token& src_loc_tk, const size_t indent,
                const char operation, const operand& dst,
                const operand& divisor) -> void override {

        assert(operation == '/' or operation == '%');
        validate_scalar(src_loc_tk, dst.type_ref());
        validate_division_operand(src_loc_tk, divisor);
        // division requires a writable quotient or remainder destination
        if (not(dst.is_register() or dst.is_memory())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I division destination"};
        }
        divide_helper_used_ = true;
        call_arithmetic_helper(src_loc_tk, indent, dst, divisor, true,
                               operation == '%');
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

        const operand lowered{
            lower_address(src_loc_tk, indent, address, value)};
        // a distinct base or nonzero residual offset still needs an add
        if (register_index(value.base_register()) !=
                register_index(lowered.base_register()) or
            lowered.displacement() != 0) {

            asm_line(indent, "addi {}, {}, {}", value.base_register(),
                     lowered.base_register(), lowered.displacement());
        }
        if (dst.is_memory()) {
            copy_value(src_loc_tk, indent, dst, value);
        }
    }

    auto unary(const size_t indent, const char operation,
               const operand& destination) -> void override {

        assert(operation == '-' or operation == '~');
        validate_scalar(token{}, destination.type_ref());
        if (not(destination.is_register() or destination.is_memory())) {
            throw compiler_exception{token{},
                                     "invalid RV32I unary destination"};
        }
        const address_scope scope{*this, destination, operand{}};

        const operand address{destination.is_memory()
                                  ? lower_address(token{}, indent, destination)
                                  : operand{}};

        const operand value{
            destination.is_register()
                ? destination
                : alloc_scratch_register(token{}, indent, default_type())};

        if (destination.is_memory()) {
            copy_value(token{}, indent, value, address);
        }
        if (operation == '-') {

            asm_line(indent, "sub {}, zero, {}", value.base_register(),
                     value.base_register());

        } else {
            const int mask{destination.type_ref().name() == "bool"
                               ? std::numeric_limits<uint8_t>::max()
                               : -1};

            asm_line(indent, "xori {}, {}, {}", value.base_register(),
                     value.base_register(), mask);
        }
        store_operation_result(indent, destination, address, value,
                               operation == '-');
    }

    [[nodiscard]] auto
    can_encode_index_scale([[maybe_unused]] const size_t size_bytes) const
        -> bool override {
        return false;
    }

    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size_bytes)
        -> void override {

        // index scaling uses the target's address width
        if (index.type_ref().size_bytes() != address_size_bytes() or
            element_size_bytes > std::numeric_limits<uint32_t>::max()) {

            throw compiler_exception{src_loc_tk,
                                     "index scale exceeds RV32I address range"};
        }

        multiply(src_loc_tk, indent, index,
                 operand::imm(std::format("{}", element_size_bytes),
                              default_type()));
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
        multiply_helper_used_ = false;
        divide_helper_used_ = false;
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

    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const operand& reg_to_check, const size_t array_count,
                      const bool allow_end, const operand& reg_count,
                      const bounds_check_options& options) -> void override {
        if (not options.upper and not options.lower) {
            return;
        }
        if (array_count > std::numeric_limits<uint32_t>::max() or
            src_loc_tk.at_line() > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{src_loc_tk,
                                     "bounds check exceeds RV32I range"};
        }
        const address_scope scope{*this, reg_to_check, reg_count};
        const std::string_view index{reg_to_check.base_register()};
        comment(src_loc_tk, indent, "bounds check");
        if (options.lower) {
            asm_line(indent, "bltz {}, 1f", index);
        }
        if (options.upper) {
            const operand limit{
                alloc_scratch_register(src_loc_tk, indent, default_type())};

            std::string_view top{index};
            if (not reg_count.is_empty()) {
                const operand sum{
                    alloc_scratch_register(src_loc_tk, indent, default_type())};

                const operand high{
                    alloc_scratch_register(src_loc_tk, indent, default_type())};

                top = sum.base_register();
                asm_line(indent, "srai {}, {}, 31", high.base_register(),
                         index);
                asm_line(indent, "srai {}, {}, 31", limit.base_register(),
                         reg_count.base_register());
                asm_line(indent, "add {}, {}, {}", high.base_register(),
                         high.base_register(), limit.base_register());
                asm_line(indent, "add {}, {}, {}", top, index,
                         reg_count.base_register());
                asm_line(indent, "sltu {}, {}, {}", limit.base_register(), top,
                         index);
                asm_line(indent, "add {}, {}, {}", high.base_register(),
                         high.base_register(), limit.base_register());
                asm_line(indent, "bltz {}, 2f", high.base_register());
                asm_line(indent, "bgtz {}, 1f", high.base_register());
            } else {
                asm_line(indent, "bltz {}, 2f", index);
            }
            asm_line(indent, "li {}, {}", limit.base_register(), array_count);
            if (allow_end) {
                asm_line(indent, "bltu {}, {}, 1f", limit.base_register(), top);
            } else {
                asm_line(indent, "bgeu {}, {}, 1f", top, limit.base_register());
            }
        }
        asm_line(indent, "j 2f");
        asm_line(indent, "1:");
        if (options.with_line) {
            asm_line(indent, "li a0, {}", src_loc_tk.at_line());
        }
        asm_line(indent, "j baz_bounds_panic");
        asm_line(indent, "2:");
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        constexpr std::string_view message{"panic: bounds at line "};
        asm_line(0, "baz_bounds_panic:");
        if (with_line) {
            asm_line(1, "mv s2, a0");
            asm_line(1, "li a0, 2");
            asm_line(1, "la a1, .Lbaz_bounds_message");
            asm_line(1, "li a2, {}", message.size());
            asm_line(1, "li a7, 64");
            asm_line(1, "ecall");
            asm_line(1, "addi sp, sp, -16");
            asm_line(1, "mv a1, sp");
            asm_line(1, "li a2, 0");
            asm_line(1, "la t0, .Lbaz_decimal_places");
            asm_line(0, "1:");
            asm_line(1, "lw t1, 0(t0)");
            asm_line(1, "li t2, 0");
            asm_line(0, "2:");
            asm_line(1, "bltu s2, t1, 3f");
            asm_line(1, "sub s2, s2, t1");
            asm_line(1, "addi t2, t2, 1");
            asm_line(1, "j 2b");
            asm_line(0, "3:");
            asm_line(1, "or t3, a2, t2");
            asm_line(1, "bnez t3, 4f");
            asm_line(1, "li t3, 1");
            asm_line(1, "bne t1, t3, 5f");
            asm_line(0, "4:");
            asm_line(1, "addi t2, t2, 48");
            asm_line(1, "sb t2, 0(a1)");
            asm_line(1, "addi a1, a1, 1");
            asm_line(1, "addi a2, a2, 1");
            asm_line(0, "5:");
            asm_line(1, "addi t0, t0, 4");
            asm_line(1, "li t3, 1");
            asm_line(1, "bne t1, t3, 1b");
            asm_line(1, "li t2, 10");
            asm_line(1, "sb t2, 0(a1)");
            asm_line(1, "addi a2, a2, 1");
            asm_line(1, "mv a1, sp");
            asm_line(1, "li a0, 2");
            asm_line(1, "li a7, 64");
            asm_line(1, "ecall");
        }
        exit(token{}, 1, operand::imm("255", default_type()));
        if (with_line) {
            asm_line(0, ".section .rodata");
            asm_line(0, ".Lbaz_bounds_message:");
            asm_line(0, ".ascii \"{}\"", message);
            asm_line(0, ".balign 4");
            asm_line(0, ".Lbaz_decimal_places:");
            asm_line(0, ".word 1000000000, 100000000, 10000000, 1000000, "
                        "100000, 10000, 1000, 100, 10, 1");
            asm_line(0, ".text");
        }
    }

    [[nodiscard]] auto data_alignment() const -> size_t override {
        return data_alignment_;
    }

    auto begin_data(const size_t alignment) -> void override {
        emit_arithmetic_helpers();
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
