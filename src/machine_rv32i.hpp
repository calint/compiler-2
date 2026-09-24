#pragma once

#include <array>
#include <bit>
#include <charconv>
#include <iostream>
#include <limits>
#include <optional>
#include <print>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "machine.hpp"
#include "type.hpp"

class machine_rv32i final : public machine {
    static constexpr size_t s0_register_index{8};
    static constexpr size_t data_alignment_{16};
    static constexpr size_t copy_unroll_threshold_bytes_{16};
    static constexpr int64_t immediate_min{-2048};
    static constexpr int64_t immediate_max{2047};

    static constexpr std::array<std::string_view, 32> register_names_{
        "zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
        "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
        "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
    };

    static constexpr std::array<size_t, 30> scratch_registers_{
        5,  6,  7,  28, 29, 30, 31, 8,  9,  18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 4,  3,  1,  11, 12, 13, 14, 15, 16, 17, 10,
    };
    // note: ascending t and s names keep generated code readable while argument
    //       registers stay late to avoid builtin conflicts and a0 stays last
    //       because syscalls overwrite it with their result

    struct allocation {
        size_t register_index;
        const type* type_ptr;
        token source_location;
        size_t indent;
        bool named;
    };

    std::reference_wrapper<std::ostream> os_{std::cout};
    std::string_view source_;
    const type* type_i32_{};
    uint32_t unavailable_registers_{};
    bool variables_base_reserved_{};
    bool frame_base_reserved_{};
    bool multiply_helper_used_{};
    bool divide_helper_used_{};
    std::vector<allocation> allocations_;
    std::vector<std::array<operand, 3>> bulk_registers_;

    [[nodiscard]] static auto format_address(const operand& address)
        -> std::string {

        std::string text{address.base_register()};
        if (not address.index_register().empty()) {
            if (not text.empty()) {
                text += " + ";
            }
            text += address.index_register();
            if (address.scale() > 1) {
                text += std::format(" * {}", address.scale());
            }
        }

        if (text.empty()) {
            text = std::format("{}", address.displacement());
        } else if (address.displacement() < 0) {
            const uint64_t magnitude{
                uint64_t{} - static_cast<uint64_t>(address.displacement())};

            text += std::format(" - {}", magnitude);
        } else if (address.displacement() > 0) {
            text += std::format(" + {}", address.displacement());
        }

        return text;
    }

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

    [[nodiscard]] static auto is_register(const std::string_view name) -> bool {
        return register_index(name) != register_names_.size();
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

        if (not is_register(address.base_register())) {
            throw compiler_exception{src_loc_tk, "invalid RV32I base register"};
        }

        if (not address.index_register().empty() and
            not is_register(address.index_register())) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I index register"};
        }

        if (not address.index_register().empty() and
            address.scale() > UINT32_MAX) {
            throw compiler_exception{src_loc_tk,
                                     "index scale exceeds RV32I address range"};
        }
    }

    template <typename... args_t>
    auto asm_line(const size_t indent,
                  const std::format_string<args_t...> format,
                  args_t&&... args) const -> void {
        std::print(os_.get(), "{}", std::string(indent * 4, ' '));
        std::println(os_.get(), format, std::forward<args_t>(args)...);
    }

    // 'address_scope' protects operand registers and memory base/index
    // registers from scratch allocation while lowering an operation, including
    // raw register operands on scope exit, drops temporaries allocated within
    // the scope and restores the previous unavailable mask, including during
    // exception unwinding this restores allocator bookkeeping only, not runtime
    // register values allocations that existed on entry must not be freed
    // within the scope

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
                // implicit releases need the allocation context for a balanced
                // trace
                const allocation& entry{backend_.allocations_.back()};
                backend_.comment(entry.source_location, entry.indent,
                                 "free {} register {}",
                                 entry.named ? "named" : "scratch",
                                 register_names_.at(entry.register_index));
                backend_.allocations_.pop_back();
            }
            backend_.unavailable_registers_ = saved_mask_;
        }
    };

    struct address_offset_parts {
        uint32_t upper;
        int32_t low;
    };

    [[nodiscard]] static auto split_address_offset(const int64_t offset)
        -> address_offset_parts {

        constexpr unsigned low_bits{12};
        constexpr uint32_t low_mask{0xfff};
        constexpr int32_t low_range{4096};

        // keep the signed low 12 bits in the memory operand; subtracting
        // them from the offset leaves the upper part to load with lui
        int32_t low{
            static_cast<int32_t>(static_cast<uint32_t>(offset) & low_mask)};

        if (low > immediate_max) {
            low -= low_range;
        }

        return {
            .upper{static_cast<uint32_t>(offset - low) >> low_bits},
            .low{low},
        };
    }

    [[nodiscard]] auto lower_address_offset(const token& src_loc_tk,
                                            const size_t indent,
                                            const operand& address,
                                            const operand& result) -> operand {

        const int64_t offset{address.displacement()};

        const std::string& result_name{result.base_register()};

        const address_offset_parts parts{split_address_offset(offset)};

        // no upper part: the memory instruction handles the entire offset
        if (parts.upper == 0) {
            return operand::mem(result_name, {}, 1, parts.low,
                                address.type_ref());
        }

        const operand displacement{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        asm_line(indent, "lui {}, {}", displacement.base_register(),
                 parts.upper);

        asm_line(indent, "add {}, {}, {}", result_name, result_name,
                 displacement.base_register());

        free_scratch_register(src_loc_tk, indent, displacement);

        return operand::mem(result_name, {}, 1, parts.low, address.type_ref());
    }

    [[nodiscard]] static auto
    can_reuse_address_destination(const operand& address,
                                  const operand& destination) -> bool {

        // no destination register is available to reuse
        if (not destination.is_register()) {
            return false;
        }

        const size_t dst_index{register_index(destination.base_register())};

        // zero discards writes, so it cannot hold the computed address
        if (dst_index == 0) {
            return false;
        }

        const size_t base_index{register_index(address.base_register())};

        // if the destination is the base register, loading the offset would
        // erase the base value before the add
        if (address.index_register().empty()) {
            return dst_index != base_index;
        }

        // if the destination is the base register, shifting the index into it
        // would erase the base value before the add
        if (dst_index == base_index and address.scale() != 1) {
            return false;
        }

        return true;
    }

    [[nodiscard]] auto lower_address_without_index(const token& src_loc_tk,
                                                   const size_t indent,
                                                   const operand& address,
                                                   const operand& destination)
        -> operand {

        const std::string& base{address.base_register()};
        const int64_t offset{address.displacement()};

        const address_offset_parts parts{split_address_offset(offset)};

        // no upper part: the memory instruction handles the entire offset
        if (parts.upper == 0) {
            return operand::mem(base, {}, 1, parts.low, address.type_ref());
        }

        const operand result{
            can_reuse_address_destination(address, destination)
                ? destination
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        const std::string& result_name{result.base_register()};

        // only the upper part; the memory instruction adds the low

        asm_line(indent, "lui {}, {}", result_name, parts.upper);

        asm_line(indent, "add {}, {}, {}", result_name, result_name, base);

        return operand::mem(result_name, {}, 1, parts.low, address.type_ref());
    }

    [[nodiscard]] auto
    lower_address_unscaled_index(const token& src_loc_tk, const size_t indent,
                                 const operand& address, const operand& result)
        -> operand {

        const std::string& base{address.base_register()};
        const std::string& index{address.index_register()};
        const std::string& result_name{result.base_register()};

        // one add combines register inputs, even when result aliases either
        asm_line(indent, "add {}, {}, {}", result_name, base, index);

        return lower_address_offset(src_loc_tk, indent, address, result);
    }

    [[nodiscard]] auto
    lower_address_scaled_index(const token& src_loc_tk, const size_t indent,
                               const operand& address, const operand& result)
        -> operand {
        const std::string& base{address.base_register()};
        const std::string& result_name{result.base_register()};

        asm_line(indent, "slli {}, {}, {}", result_name,
                 address.index_register(), std::countr_zero(address.scale()));

        // the preserved base can now be added to the scaled index
        asm_line(indent, "add {}, {}, {}", result_name, result_name, base);

        return lower_address_offset(src_loc_tk, indent, address, result);
    }

    // lower base + index * scale + displacement to register + signed 12-bit
    // offset
    [[nodiscard]] auto
    lower_address(const token& src_loc_tk, const size_t indent,
                  const operand& address, const operand& destination = {})
        -> operand {

        validate_address(src_loc_tk, address);

        // no index: encode the offset directly or materialize base + offset
        if (address.index_register().empty()) {
            return lower_address_without_index(src_loc_tk, indent, address,
                                               destination);
        }

        // indexed memory operand: [base + index * scale + displacement];
        // combine the register terms before applying the displacement
        const operand result{
            can_reuse_address_destination(address, destination)
                ? destination
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        // unit scale: combine the base and index without multiplication
        if (address.scale() == 1) {
            return lower_address_unscaled_index(src_loc_tk, indent, address,
                                                result);
        }

        // scaled index: form the product before adding the base and offset
        return lower_address_scaled_index(src_loc_tk, indent, address, result);
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

        const std::optional<uint64_t> bits{immediate_bits(value)};
        if (not bits) {
            return std::nullopt;
        }

        return std::bit_cast<int32_t>(static_cast<uint32_t>(*bits));
    }

    [[nodiscard]] static auto same_memory(const operand& left,
                                          const operand& right) -> bool {

        const auto same_register = [](const std::string_view first,
                                      const std::string_view second) -> bool {
            return first == second or
                   (is_register(first) and
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

            // restoring a register destination would discard the result
            if ((live & register_mask(name)) != 0 and
                (not destination.is_register() or
                 register_index(destination.base_register()) !=
                     register_index(name))) {

                saved.push_back(name);
            }

            // staging registers must survive the helper call
            unavailable_registers_ |= register_mask(name);
        }

        // aliases and address registers carry the same argument dependencies
        const auto uses_register = [](const operand& value,
                                      const std::string_view name) -> bool {
            return (value.is_register() or value.is_memory()) and
                   (register_index(value.base_register()) ==
                        register_index(name) or
                    (value.is_memory() and
                     register_index(value.index_register()) ==
                         register_index(name)));
        };

        // stack operands must be read before the save area changes sp
        const bool stack_operands{uses_register(destination, "sp") or
                                  uses_register(source, "sp")};
        operand left;
        operand right;
        if (stack_operands) {
            left = alloc_scratch_register(src_loc_tk, indent, default_type());
            right = alloc_scratch_register(src_loc_tk, indent, default_type());
            copy_value(src_loc_tk, indent, left, destination);
            copy_value(src_loc_tk, indent, right, source);
        }
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
        // save caller values before argument setup overwrites a0 or a1
        for (const auto [index, name] : std::views::enumerate(saved)) {
            asm_line(indent, "sw {}, {}(sp)", name,
                     static_cast<size_t>(index) * word_size);
        }
        const operand first_argument{operand::reg("a0", default_type())};
        const operand second_argument{operand::reg("a1", default_type())};
        if (stack_operands) {
            copy_value(src_loc_tk, indent, first_argument, left);
            copy_value(src_loc_tk, indent, second_argument, right);
        } else if (uses_register(source, "a0")) {
            // neither argument can be written first without losing an input
            if (uses_register(destination, "a1")) {
                right =
                    alloc_scratch_register(src_loc_tk, indent, default_type());
                copy_value(src_loc_tk, indent, right, source);
                copy_value(src_loc_tk, indent, first_argument, destination);
                copy_value(src_loc_tk, indent, second_argument, right);
                free_scratch_register(src_loc_tk, indent, right);
            } else {
                // consume the source before loading the destination into a0
                copy_value(src_loc_tk, indent, second_argument, source);
                copy_value(src_loc_tk, indent, first_argument, destination);
            }
        } else {
            // the source does not need the old a0 so no staging is necessary
            copy_value(src_loc_tk, indent, first_argument, destination);
            copy_value(src_loc_tk, indent, second_argument, source);
        }
        asm_line(indent, "call {}",
                 division ? ".Lbaz_divide" : ".Lbaz_multiply");
        operand result{operand::reg(remainder ? "a1" : "a0", default_type())};
        const bool store_before_restore{destination.is_register() and
                                        not stack_operands};
        if (store_before_restore) {
            // the destination is not restored so it can retain the result
            copy_value(src_loc_tk, indent, destination, result);
        } else if (stack_operands or
                   std::ranges::find(saved, result.base_register()) !=
                       saved.end()) {
            // reuse stack staging or protect a result register being restored
            if (left.is_empty()) {
                left =
                    alloc_scratch_register(src_loc_tk, indent, default_type());
            }
            copy_value(src_loc_tk, indent, left, result);
            result = left;
        }
        for (const auto [index, name] : std::views::enumerate(saved)) {
            asm_line(indent, "lw {}, {}(sp)", name,
                     static_cast<size_t>(index) * word_size);
        }
        // restore sp before writing a possibly stack-relative destination
        if (stack_bytes != 0) {
            asm_line(indent, "addi sp, sp, {}", stack_bytes);
        }
        if (not store_before_restore) {
            // memory destinations need their original address registers back
            copy_value(src_loc_tk, indent, destination, result);
        }
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

    auto begin_bulk(const token& src_loc_tk, const size_t indent) -> operand {
        // argument-order allocation keeps pointer and count names easy to
        // follow
        std::array<operand, 3> registers;
        for (operand& reg : registers) {
            reg = alloc_scratch_register(src_loc_tk, indent, default_type());
        }
        bulk_registers_.push_back(registers);

        return registers.back();
    }

    auto release_bulk(const token& src_loc_tk, const size_t indent) -> void {
        for (const operand& reg :
             bulk_registers_.back() | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, reg);
        }
        bulk_registers_.pop_back();
    }

    auto emit_bulk_loop(const token& src_loc_tk, const size_t indent,
                        const operand& count, const operand& source,
                        const operand& destination, const operand& result = {},
                        const bool inverted = false) -> void {

        const address_scope scope{*this, result, operand{}};

        const bool reuse_result{
            result.is_register() and
            register_index(result.base_register()) != register_index("zero") and
            register_index(result.base_register()) != register_index("sp") and
            register_index(result.base_register()) !=
                register_index(count.base_register()) and
            register_index(result.base_register()) !=
                register_index(source.base_register()) and
            register_index(result.base_register()) !=
                register_index(destination.base_register())};

        const operand left{
            reuse_result
                ? result
                : alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand right{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const bool compare{not result.is_empty()};

        const operand compared{
            compare ? alloc_scratch_register(src_loc_tk, indent, default_type())
                    : operand{}};

        // right holds count / 4 words and count retains count % 4 tail bytes
        // alignment handling is deferred so wide accesses may be unaligned
        if (not compare) {
            comment(src_loc_tk, indent,
                    "{}: copy value, {}: words, {}: tail bytes",
                    left.base_register(), right.base_register(),
                    count.base_register());
        } else {
            comment(src_loc_tk, indent,
                    "{}: left value/result, {}: right value, {}: words, {}: "
                    "tail bytes",
                    left.base_register(), compared.base_register(),
                    right.base_register(), count.base_register());
            comment(src_loc_tk, indent, "stop at first mismatch");
        }

        comment(src_loc_tk, indent,
                "split bytes into words and tail; skip word loop if none");
        asm_line(indent, "srli {}, {}, 2", right.base_register(),
                 count.base_register());
        asm_line(indent, "andi {}, {}, 3", count.base_register(),
                 count.base_register());
        asm_line(indent, "beqz {}, 2f", right.base_register());
        // process four bytes per iteration and stop comparing at the first
        // mismatch
        comment(src_loc_tk, indent, "{} 4-byte words",
                compare ? "compare" : "copy");
        asm_line(indent, "1:");
        asm_line(indent, "lw {}, 0({})", left.base_register(),
                 source.base_register());
        if (compare) {
            asm_line(indent, "lw {}, 0({})", compared.base_register(),
                     destination.base_register());
            asm_line(indent, "bne {}, {}, 5f", left.base_register(),
                     compared.base_register());
        } else {
            asm_line(indent, "sw {}, 0({})", left.base_register(),
                     destination.base_register());
        }
        asm_line(indent, "addi {}, {}, 4", source.base_register(),
                 source.base_register());
        asm_line(indent, "addi {}, {}, 4", destination.base_register(),
                 destination.base_register());
        asm_line(indent, "addi {}, {}, -1", right.base_register(),
                 right.base_register());
        asm_line(indent, "bnez {}, 1b", right.base_register());
        asm_line(indent, "2:");
        // remainder bit 1 selects a halfword for tails of two or three bytes
        comment(src_loc_tk, indent, "{} optional 2-byte tail",
                compare ? "compare" : "copy");
        asm_line(indent, "andi {}, {}, 2", left.base_register(),
                 count.base_register());
        asm_line(indent, "beqz {}, 3f", left.base_register());
        asm_line(indent, "lhu {}, 0({})", left.base_register(),
                 source.base_register());
        if (compare) {
            asm_line(indent, "lhu {}, 0({})", compared.base_register(),
                     destination.base_register());
            asm_line(indent, "bne {}, {}, 5f", left.base_register(),
                     compared.base_register());
        } else {
            asm_line(indent, "sh {}, 0({})", left.base_register(),
                     destination.base_register());
        }
        asm_line(indent, "addi {}, {}, 2", source.base_register(),
                 source.base_register());
        asm_line(indent, "addi {}, {}, 2", destination.base_register(),
                 destination.base_register());
        asm_line(indent, "3:");
        // remainder bit 0 selects the final byte for tails of one or three
        comment(src_loc_tk, indent, "{} optional final byte",
                compare ? "compare" : "copy");
        asm_line(indent, "andi {}, {}, 1", count.base_register(),
                 count.base_register());
        asm_line(indent, "beqz {}, 4f", count.base_register());
        asm_line(indent, "lbu {}, 0({})", left.base_register(),
                 source.base_register());
        if (compare) {
            asm_line(indent, "lbu {}, 0({})", compared.base_register(),
                     destination.base_register());
            asm_line(indent, "bne {}, {}, 5f", left.base_register(),
                     compared.base_register());
        } else {
            asm_line(indent, "sb {}, 0({})", left.base_register(),
                     destination.base_register());
        }
        asm_line(indent, "4:");
        // every chunk matched or the range was empty unless a mismatch branched
        // here
        if (compare) {
            comment(src_loc_tk, indent, "all matched or empty: {}",
                    inverted ? "false" : "true");
            asm_line(indent, "li {}, {}", left.base_register(),
                     inverted ? 0 : 1);
            asm_line(indent, "j 6f");
            asm_line(indent, "5:");
            comment(src_loc_tk, indent, "mismatch: {}",
                    inverted ? "true" : "false");
            asm_line(indent, "li {}, {}", left.base_register(),
                     inverted ? 1 : 0);
            asm_line(indent, "6:");
            if (not reuse_result) {
                copy_value(src_loc_tk, indent, result, left);
            }
        }
    }

  public:
    explicit machine_rv32i(const std::string_view source = {})
        : source_{source} {}

    using machine::comment;
    using machine::emit_data_array;

    [[nodiscard]] auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers override {

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

    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void override {
        // synthetic tokens and standalone backend calls have no source location
        if (src_loc_tk.at_line() == 0 or source_.empty()) {
            asm_line(indent, "# {}", text);
        } else {
            const auto [line, column]{line_and_col_num_for_char_index(
                src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

            asm_line(indent, "# [{}:{}] {}", line, column, text);
        }
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
                    std::string_view value{line};
                    value = value.substr(0, value.find('#'));
                    const size_t start{value.find_first_not_of(" \t\r")};
                    if (start == std::string_view::npos) {
                        continue;
                    }
                    value = value.substr(
                        start, value.find_last_not_of(" \t\r") - start + 1);
                    // emitted labels and directives occupy no instruction slots
                    if (value.front() == '.' or value.back() == ':') {
                        continue;
                    }
                    const std::string_view instruction{
                        value.substr(0, value.find_first_of(" \t"))};
                    // norelax keeps address and call sequences at two
                    // instructions
                    if (instruction == "la" or instruction == "call") {
                        count += 2;
                    } else if (instruction == "li") {
                        size_t cost{2};
                        const size_t comma{value.find(',')};
                        if (comma != std::string_view::npos) {
                            std::string_view literal{value.substr(comma + 1)};
                            const size_t digits{
                                literal.find_first_not_of(" \t")};
                            if (digits != std::string_view::npos) {
                                literal.remove_prefix(digits);
                                int64_t parsed{};
                                const char* const end{
                                    std::to_address(literal.end())};
                                const std::from_chars_result conversion{
                                    std::from_chars(
                                        std::to_address(literal.begin()), end,
                                        parsed)};

                                // unresolved expressions retain the
                                // conservative cost
                                if (conversion.ec == std::errc{} and
                                    conversion.ptr == end and
                                    parsed >=
                                        std::numeric_limits<int32_t>::min() and
                                    std::cmp_less_equal(
                                        parsed,
                                        std::numeric_limits<uint32_t>::max())) {
                                    const uint32_t bits{
                                        static_cast<uint32_t>(parsed)};
                                    const int32_t immediate{
                                        std::bit_cast<int32_t>(bits)};
                                    constexpr uint32_t low_mask{0xfff};
                                    // addi handles signed 12 bits and lui needs
                                    // no low-part add
                                    if ((immediate >= immediate_min and
                                         immediate <= immediate_max) or
                                        (bits & low_mask) == 0) {
                                        cost = 1;
                                    }
                                }
                            }
                        }
                        count += cost;
                    } else {
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

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref)
        -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        for (const size_t index : scratch_registers_) {
            const uint32_t mask{uint32_t{1} << index};
            if ((unavailable_registers_ & mask) == 0) {
                unavailable_registers_ |= mask;
                allocations_.push_back({
                    .register_index{index},
                    .type_ptr{&type_ref},
                    .source_location{src_loc_tk},
                    .indent{indent},
                    .named{},
                });

                comment(src_loc_tk, indent, "allocate scratch register -> {}",
                        register_names_.at(index));

                operand result{
                    make_register_operand(register_names_.at(index), type_ref)};

                result.set_allocation_register(register_names_.at(index));

                return result;
            }
        }

        throw compiler_exception{src_loc_tk, "out of RV32I scratch registers"};
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
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
            .source_location{src_loc_tk},
            .indent{indent},
            .named{true},
        });

        unavailable_registers_ |= mask;
        comment(src_loc_tk, indent, "allocate named register {}",
                register_names_.at(index));

        return result;
    }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void override {
        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indent,
                               const operand& reg) -> void override {

        assert(not allocations_.empty());
        const size_t index{register_index(reg.allocation_register())};
        assert(allocations_.back().register_index == index);
        // named and scratch allocations share the same lifo pool
        comment(src_loc_tk, indent, "free {} register {}",
                allocations_.back().named ? "named" : "scratch",
                register_names_.at(index));
        unavailable_registers_ &= ~(uint32_t{1} << index);
        allocations_.pop_back();
    }

    auto finish() -> void override {
        assert(bulk_registers_.empty());
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

        if (dst.is_memory() and src.is_memory() and
            dst.type_ref().name() == src.type_ref().name() and
            dst.base_register() == src.base_register() and
            dst.index_register() == src.index_register() and
            dst.scale() == src.scale() and
            dst.displacement() == src.displacement()) {
            return;
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
                          const operand& address) -> void override {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, size_bytes,
                format_address(address));
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
            // note: open code scope for 'address_scop' to trigger delete before
            //       freeing scratch registers

            const address_scope destination_scope{*this, action.destination,
                                                  operand{}};

            const address_scope scope{*this, lhs, rhs};

            validate_scalar(src_loc_tk, lhs.type_ref());
            validate_scalar(src_loc_tk, rhs.type_ref());

            const auto prepare = [&](const operand& source,
                                     const operand& other) -> operand {
                // matching register representations need no conversion
                if (source.is_register() and
                    source.type_ref().name() == lhs.type_ref().name()) {
                    return source;
                }
                // zero has the same representation at every supported width
                if (immediate_value(source) == 0) {
                    return operand::reg("zero", lhs.type_ref());
                }

                // the output can hold an input unless doing so destroys the
                // other value or an address still needed to load it
                const size_t output_register{
                    register_index(action.destination.base_register())};
                const bool reuse_destination{
                    action.destination.is_register() and
                    output_register != 0 and
                    (not(other.is_register() or other.is_memory()) or
                     output_register !=
                         register_index(other.base_register())) and
                    (not other.is_memory() or
                     output_register !=
                         register_index(other.index_register()))};

                // preserve the comparison width rather than narrowing to bool
                const operand value{
                    reuse_destination
                        ? operand::reg(action.destination.base_register(),
                                       lhs.type_ref())
                        : alloc_scratch_register(src_loc_tk, indent,
                                                 lhs.type_ref())};

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

            const operand left{prepare(lhs, rhs)};

            const operand right{use_immediate
                                    ? operand::reg("zero", lhs.type_ref())
                                    : prepare(rhs, left)};

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

    auto invoke_syscall(const size_t indent) -> void override {
        asm_line(indent, "ecall");
    }

    auto advance_array_iteration(const size_t indent, const operand& iterator,
                                 const operand& counter,
                                 const size_t element_size_bytes,
                                 const size_t array_count,
                                 const std::string_view loop_label)
        -> void override {

        if (element_size_bytes > std::numeric_limits<uint32_t>::max() or
            array_count > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{token{},
                                     "array iteration exceeds RV32I range"};
        }

        const address_scope scope{*this, iterator, counter};
        add_subtract(token{}, indent, '+', iterator,
                     operand::imm(std::format("{}", element_size_bytes),
                                  default_type()));

        const operand value{
            counter.is_register()
                ? counter
                : alloc_scratch_register(token{}, indent, default_type())};

        copy_value(token{}, indent, value, counter);
        add_subtract(token{}, indent, '+', value,
                     operand::imm("1", default_type()));
        copy_value(token{}, indent, counter, value);

        const operand limit{
            alloc_scratch_register(token{}, indent, default_type())};

        asm_line(indent, "li {}, {}", limit.base_register(), array_count);
        asm_line(indent, "beq {}, {}, 1f", value.base_register(),
                 limit.base_register());
        branch(indent, loop_label);
        asm_line(indent, "1:");
    }

    auto copy(const token& src_loc_tk, const size_t indent, const operand& src,
              const operand& dst, const size_t size_bytes) -> void override {
        if (size_bytes == 0) {
            return;
        }
        if (size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{src_loc_tk,
                                     "copy size exceeds RV32I address range"};
        }
        const address_scope scope{*this, dst, src};
        if (size_bytes <= copy_unroll_threshold_bytes_) {
            const auto prepare_address =
                [&](const operand& address) -> operand {
                operand lowered{lower_address(src_loc_tk, indent, address)};
                // keep the entire unrolled copy within the load/store offset
                // range
                if (lowered.displacement() + static_cast<int64_t>(size_bytes) -
                        1 >
                    immediate_max) {
                    const operand pointer{alloc_scratch_register(
                        src_loc_tk, indent, default_type())};

                    address_of(src_loc_tk, indent, pointer, lowered);
                    lowered = operand::mem(pointer.base_register(), {}, 1, 0,
                                           address.type_ref());
                }

                return lowered;
            };

            // direct offsets avoid two pointer temporaries for ordinary small
            // copies
            const operand src_address{prepare_address(src)};
            const operand dst_address{prepare_address(dst)};
            const operand value{
                alloc_scratch_register(src_loc_tk, indent, default_type())};

            size_t offset{};
            for (const size_t width : {size_t{4}, size_t{2}, size_t{1}}) {
                std::string_view load{"lbu"};
                std::string_view store{"sb"};
                if (width == 4) {
                    load = "lw";
                    store = "sw";
                } else if (width == 2) {
                    load = "lhu";
                    store = "sh";
                }
                while (size_bytes - offset >= width) {
                    asm_line(indent, "{} {}, {}({})", load,
                             value.base_register(),
                             src_address.displacement() +
                                 static_cast<int64_t>(offset),
                             src_address.base_register());
                    asm_line(indent, "{} {}, {}({})", store,
                             value.base_register(),
                             dst_address.displacement() +
                                 static_cast<int64_t>(offset),
                             dst_address.base_register());
                    offset += width;
                }
            }

            return;
        }
        const operand src_pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand dst_pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        address_of(src_loc_tk, indent, src_pointer, src);
        const operand& dst_address{dst};
        address_of(src_loc_tk, indent, dst_pointer, dst_address);
        const operand count{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        asm_line(indent, "li {}, {}", count.base_register(), size_bytes);
        emit_bulk_loop(src_loc_tk, indent, count, src_pointer, dst_pointer);
    }

    [[nodiscard]] auto begin_array_copy(const token& src_loc_tk,
                                        const size_t indent)
        -> operand override {

        const operand count{begin_bulk(src_loc_tk, indent)};

        const std::array<operand, 3>& registers{bulk_registers_.back()};

        comment(src_loc_tk, indent, "{}: source, {}: destination, {}: count",
                registers.at(0).base_register(),
                registers.at(1).base_register(), count.base_register());

        return count;
    }

    [[nodiscard]] auto array_copy_source_register() const -> operand override {
        return bulk_registers_.back().at(0);
    }

    [[nodiscard]] auto array_copy_destination_register() const
        -> operand override {
        return bulk_registers_.back().at(1);
    }

    auto set_array_copy_source(const size_t indent, const operand& address)
        -> void override {
        address_of(token{}, indent, bulk_registers_.back().at(0), address);
    }

    auto set_array_copy_destination(const size_t indent, const operand& address)
        -> void override {
        address_of(token{}, indent, bulk_registers_.back().at(1), address);
    }

    auto end_array_copy(const token& src_loc_tk, const size_t indent,
                        const size_t element_size_bytes) -> void override {
        const std::array<operand, 3>& registers{bulk_registers_.back()};
        comment(src_loc_tk, indent, "{}: elements to bytes ({} bytes/element)",
                registers.at(2).base_register(), element_size_bytes);
        scale_index(src_loc_tk, indent, registers.at(2), element_size_bytes);
        emit_bulk_loop(src_loc_tk, indent, registers.at(2), registers.at(0),
                       registers.at(1));
        release_bulk(src_loc_tk, indent);
    }

    auto begin_memory_equal(const token& src_loc_tk, const size_t indent)
        -> operand override {

        const operand count{begin_bulk(src_loc_tk, indent)};

        const std::array<operand, 3>& registers{bulk_registers_.back()};

        comment(src_loc_tk, indent, "{}: source, {}: destination, {}: count",
                registers.at(0).base_register(),
                registers.at(1).base_register(), count.base_register());

        return count;
    }

    // borrowed pointers avoid a temporary address and final move for indexing
    [[nodiscard]] auto memory_equal_left_register() const -> operand override {
        return bulk_registers_.back().at(0);
    }

    [[nodiscard]] auto memory_equal_right_register() const -> operand override {
        return bulk_registers_.back().at(1);
    }

    auto set_memory_equal_left(const size_t indent, const operand& address)
        -> void override {
        set_array_copy_source(indent, address);
    }

    auto set_memory_equal_right(const size_t indent, const operand& address)
        -> void override {
        set_array_copy_destination(indent, address);
    }

    auto end_memory_equal(const token& src_loc_tk, const size_t indent,
                          const size_t size_bytes, const operand& dst,
                          const bool inverted = false) -> void override {
        if (size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{
                src_loc_tk, "comparison size exceeds RV32I address range"};
        }
        const std::array<operand, 3>& registers{bulk_registers_.back()};
        asm_line(indent, "li {}, {}", registers.at(2).base_register(),
                 size_bytes);
        emit_bulk_loop(src_loc_tk, indent, registers.at(2), registers.at(0),
                       registers.at(1), dst, inverted);
        release_bulk(src_loc_tk, indent);
    }

    auto end_arrays_equal(const token& src_loc_tk, const size_t indent,
                          const size_t element_size_bytes, const operand& dst,
                          const bool inverted = false) -> void override {
        const std::array<operand, 3>& registers{bulk_registers_.back()};
        {
            const address_scope scope{*this, dst, operand{}};
            comment(src_loc_tk, indent,
                    "{}: elements to bytes ({} bytes/element)",
                    registers.at(2).base_register(), element_size_bytes);
            scale_index(src_loc_tk, indent, registers.at(2),
                        element_size_bytes);
            emit_bulk_loop(src_loc_tk, indent, registers.at(2), registers.at(0),
                           registers.at(1), dst, inverted);
        }
        release_bulk(src_loc_tk, indent);
    }

    auto zero(const token& src_loc_tk, const size_t indent,
              const operand& destination, const size_t size_bytes)
        -> void override {

        if (size_bytes == 0) {
            return;
        }
        const address_scope scope{*this, destination, operand{}};
        constexpr size_t direct_store_limit{16};
        // match bulk copies: wide stores may be unaligned until alignment is
        // handled
        if (size_bytes <= direct_store_limit) {
            operand address{lower_address(src_loc_tk, indent, destination)};
            // keep every unrolled store inside the signed 12-bit offset range
            if (address.displacement() + static_cast<int64_t>(size_bytes) - 1 >
                immediate_max) {
                const operand pointer{
                    alloc_scratch_register(src_loc_tk, indent, default_type())};

                address_of(src_loc_tk, indent, pointer, address);
                address = operand::mem(pointer.base_register(), {}, 1, 0,
                                       destination.type_ref());
            }
            size_t offset{};
            for (const size_t width : {size_t{4}, size_t{2}, size_t{1}}) {
                std::string_view instruction{"sb"};
                if (width == 4) {
                    instruction = "sw";
                } else if (width == 2) {
                    instruction = "sh";
                }

                while (size_bytes - offset >= width) {
                    asm_line(indent, "{} zero, {}({})", instruction,
                             address.displacement() +
                                 static_cast<int64_t>(offset),
                             address.base_register());
                    offset += width;
                }
            }

            return;
        }

        const operand dst_pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        address_of(src_loc_tk, indent, dst_pointer, destination);

        copy_value(
            src_loc_tk, indent, remaining,
            operand::imm(std::format("{}", size_bytes / 4), default_type()));

        asm_line(indent, "1:");
        asm_line(indent, "sw zero, 0({})", dst_pointer.base_register());

        asm_line(indent, "addi {}, {}, 4", dst_pointer.base_register(),
                 dst_pointer.base_register());

        asm_line(indent, "addi {}, {}, -1", remaining.base_register(),
                 remaining.base_register());

        asm_line(indent, "bnez {}, 1b", remaining.base_register());
        // the known tail needs no runtime tests or additional scratch
        // registers
        if ((size_bytes & 2U) != 0) {
            asm_line(indent, "sh zero, 0({})", dst_pointer.base_register());
        }
        if ((size_bytes & 1U) != 0) {
            asm_line(indent, "sb zero, {}({})", size_bytes & 2U,
                     dst_pointer.base_register());
        }
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

        // variable factors use the shared runtime helper
        if (not constant.has_value()) {
            multiply_helper_used_ = true;
            call_arithmetic_helper(src_loc_tk, indent, product, factor, false);
            return;
        }

        // the factor is now a known constant; keep only the bits that fit
        // in the product's type before choosing how to multiply

        constexpr size_t register_bits{std::numeric_limits<uint32_t>::digits};

        const size_t bits{product.type_ref().size_bytes() * 8};

        const uint32_t mask{std::numeric_limits<uint32_t>::max() >>
                            (register_bits - bits)};

        const uint32_t multiplier{static_cast<uint32_t>(*constant) & mask};

        // constant zero and one need no multiplication machinery
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
                  operand::imm(std::format("{}", std::countr_zero(multiplier)),
                               default_type()));

            return;
        }

        // the remaining constant needs shifts and adds; keep the original
        // value for the additions while the result changes

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
        // immediate shifts must be resolved here rather than by the assembler
        if (count.is_immediate() and not constant.has_value()) {
            throw compiler_exception{src_loc_tk,
                                     "invalid RV32I immediate shift count"};
        }
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
        } else if (constant.has_value()) {
            // known counts already have rv32's five-bit shift semantics applied
            asm_line(indent, "{} {}, {}, {}",
                     operation == '<' ? "slli" : "srai", value.base_register(),
                     value.base_register(), shift_count);
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

    [[nodiscard]] auto can_lower_index_scale(const size_t size_bytes) const
        -> bool override {
        return std::has_single_bit(size_bytes) and size_bytes <= UINT32_MAX;
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
        asm_line(1, "addi sp, sp, -16");
        asm_line(1, "sw ra, 0(sp)");
    }

    auto release_frame_base() -> void override {
        assert(frame_base_reserved_);
        operand base{
            make_register_operand(frame_base_register(), default_type())};
        base.set_allocation_register(frame_base_register());
        free_named_register(token{}, 0, base);
        frame_base_reserved_ = false;
    }

    auto call_function(const size_t indent, const std::string_view label,
                       const operand& frame_address) -> void override {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(register_index(frame_address.base_register()) !=
               register_index("sp"));

        asm_line(indent, "addi sp, sp, -128");

        for (const size_t index : scratch_registers_) {
            asm_line(indent, "sw {}, {}(sp)", register_names_.at(index),
                     (index - 1) * 4);
        }

        address_of(token{}, indent,
                   make_register_operand(frame_base_register(), default_type()),
                   frame_address);

        asm_line(indent, "call {}", label);
        for (const size_t index : scratch_registers_) {
            asm_line(indent, "lw {}, {}(sp)", register_names_.at(index),
                     (index - 1) * 4);
        }

        asm_line(indent, "addi sp, sp, 128");
    }

    auto return_function(const size_t indent) -> void override {
        asm_line(indent, "lw ra, 0(sp)");
        asm_line(indent, "addi sp, sp, 16");
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
        assert(frame_address.index_register().empty());
        assert(frame_size_bytes.is_immediate());

        const address_scope scope{*this, frame_address, frame_size_bytes};
        const operand start{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, default_type())};

        address_of(src_loc_tk, indent, start, frame_address);
        if (register_mask(frame_address.base_register()) != 0 and
            frame_address.displacement() != 0) {
            asm_line(indent, "{} {}, {}, 1f",
                     frame_address.displacement() > 0 ? "bltu" : "bgtu",
                     start.base_register(), frame_address.base_register());
        }
        asm_line(indent, "la {}, vars", remaining.base_register());
        asm_line(indent, "bltu {}, {}, 1f", start.base_register(),
                 remaining.base_register());
        asm_line(indent, "la {}, vars.end", remaining.base_register());
        asm_line(indent, "bltu {}, {}, 1f", remaining.base_register(),
                 start.base_register());
        asm_line(indent, "sub {}, {}, {}", remaining.base_register(),
                 remaining.base_register(), start.base_register());
        asm_line(indent, "lui {}, %hi({})", start.base_register(),
                 frame_size_bytes.immediate());
        asm_line(indent, "addi {}, {}, %lo({})", start.base_register(),
                 start.base_register(), frame_size_bytes.immediate());
        asm_line(indent, "bgeu {}, {}, 2f", remaining.base_register(),
                 start.base_register());
        asm_line(indent, "1:");
        branch(indent, failure_label);
        asm_line(indent, "2:");
    }

    auto define_constant(const std::string_view name, const size_t value)
        -> void override {
        if (value > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{token{}, "constant exceeds RV32I range"};
        }
        asm_line(0, ".equ {}, {}", name, value);
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

            std::string top{index};
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

    auto emit_frame_overflow_handler() -> void override {
        constexpr std::string_view message{"panic: frame overflow"};
        asm_line(0, "baz_frame_overflow:");
        asm_line(1, "li a0, 2");
        asm_line(1, "la a1, .Lbaz_frame_message");
        // the newline follows the message text
        asm_line(1, "li a2, {}", message.size() + 1);
        asm_line(1, "li a7, 64");
        asm_line(1, "ecall");
        exit(token{}, 1, operand::imm("255", default_type()));
        asm_line(0, ".section .rodata");
        asm_line(0, ".Lbaz_frame_message:");
        asm_line(0, ".ascii \"{}\"", message);
        asm_line(0, ".byte 10");
        // the bounds handler may follow and must stay in the code section
        asm_line(0, ".text");
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

    auto emit_string_data(const std::string_view value) -> void override {
        std::string encoded;
        for (size_t offset{}; offset < value.size(); ++offset) {
            unsigned char byte{static_cast<unsigned char>(value[offset])};
            if (byte == '\\') {
                ++offset;
                if (offset == value.size()) {
                    throw compiler_exception{token{},
                                             "incomplete string escape"};
                }
                switch (value[offset]) {
                case '0':
                    byte = 0;
                    break;

                case 'a':
                    byte = '\a';
                    break;

                case 'b':
                    byte = '\b';
                    break;

                case 't':
                    byte = '\t';
                    break;

                case 'n':
                    byte = '\n';
                    break;

                case 'v':
                    byte = '\v';
                    break;

                case 'f':
                    byte = '\f';
                    break;

                case 'r':
                    byte = '\r';
                    break;

                case 'e':
                    byte = '\x1b';
                    break;

                case '\\':
                case '\'':
                case '"':
                case '`':
                    byte = static_cast<unsigned char>(value[offset]);
                    break;

                case 'x': {
                    const std::string_view digits{value.substr(offset + 1, 2)};
                    unsigned int decoded{};
                    const std::from_chars_result parsed{std::from_chars(
                        std::to_address(digits.begin()),
                        std::to_address(digits.end()), decoded, 16)};

                    if (digits.size() != 2 or parsed.ec != std::errc{} or
                        parsed.ptr != std::to_address(digits.end())) {
                        throw compiler_exception{token{},
                                                 "string hex escape requires "
                                                 "two hexadecimal digits"};
                    }
                    byte = static_cast<unsigned char>(decoded);
                    offset += 2;
                    break;
                }

                default:
                    throw compiler_exception{token{},
                                             "unsupported RV32I string escape"};
                }
            }
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
                encoded += static_cast<char>(byte);
                break;

            default:
                if (byte >= ' ' and byte <= '~') {
                    encoded += static_cast<char>(byte);
                } else {
                    encoded += std::format("\\{:03o}",
                                           static_cast<unsigned int>(byte));
                }
                break;
            }
        }
        asm_line(0, ".ascii \"{}\"", encoded);
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

    [[nodiscard]] auto
    allocated_register_type(const std::string_view name) const -> const type* {
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
