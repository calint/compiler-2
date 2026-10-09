#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <limits>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "assembler_rv32i.hpp"
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"

class machine_rv32i : public machine {
    using op = assembler_rv32i::op;

    using section = assembler_rv32i::section;

    static constexpr size_t s0_register_index{8};
    static constexpr std::string_view variables_base_register_{"s0"};
    static constexpr size_t variables_base_past_vars_bytes_{2032};

    // note: the largest multiple of 16 an 'addi' immediate holds, 'vars'
    //       starts 2032 below 's0' so 4080 bytes of variables are in reach
    //       of a load or store, and 's0' stays aligned

    static constexpr size_t data_alignment_{16};
    static constexpr size_t copy_unroll_threshold_bytes_{16};
    // stores run faster than the loop's 4 instructions per word, the cap only
    // bounds the code size
    static constexpr size_t zero_unroll_threshold_parts_{16};
    static constexpr int64_t immediate_min{-2048};
    static constexpr int64_t immediate_max{2047};
    static constexpr int syscall_read_{63};
    static constexpr int syscall_write_{64};
    static constexpr int syscall_exit_{93};
    // shifting a word right by this spreads its sign bit over all bits
    static constexpr int sign_shift_{31};
    // sp stays 16-byte aligned, the return address slot of a frame is one unit
    static constexpr size_t stack_alignment_{16};
    static constexpr int64_t frame_save_bytes_{stack_alignment_};
    static constexpr size_t byte_size_bytes_{1};
    static constexpr size_t half_size_bytes_{2};
    static constexpr size_t word_size_bytes_{4};
    static constexpr size_t register_bits_{
        std::numeric_limits<uint32_t>::digits,
    };
    static constexpr size_t address_space_bytes_{
        size_t{std::numeric_limits<uint32_t>::max()} + 1,
    };
    // note: +1 because the highest address is the maximum value

    static constexpr const decltype(assembler_rv32i::register_names)&
        register_names_{assembler_rv32i::register_names};

    static constexpr std::array<size_t, 30> scratch_registers_{
        5,  6,  7,  28, 29, 30, 31, 8,  9,  18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 4,  3,  1,  11, 12, 13, 14, 15, 16, 17, 10,
    };

    // note: ascending t and s names keep generated code readable while argument
    //       registers stay late to avoid builtin conflicts and a0 stays last
    //       because syscalls overwrite it with their result

    // the last of the saved registers, no built-in names it
    static constexpr std::string_view slot_register_{"s11"};

    // where unrolled accesses start: the address minus 'phase' is aligned to
    // 'alignment', which is at most a word
    struct access_start {
        size_t alignment{};
        size_t phase{};
    };

    // the addresses given to an active bulk operation, each start taken with
    // alignment 1 since the type alignment is given at the end
    struct bulk_addresses {
        std::array<access_start, 2> starts;
        // a missing address would leave the word alignment unproven
        size_t count{};
    };

    // the indexes are those of 'register_names_'
    register_pool registers_;

    // keeps the registers of its operands, and the base and index registers
    // of memory operands, from being picked for scratch while an operation is
    // lowered. on exit, also while an exception unwinds, it frees the
    // temporaries allocated within the scope and restores what was protected.
    // it restores allocator bookkeeping only, not register values, and what
    // was allocated before the scope must not be freed within it
    class address_scope final {
        machine_rv32i& backend_;
        uint32_t saved_mask_;
        size_t saved_count_;

      public:
        address_scope(machine_rv32i& backend, const operand& dst,
                      const operand& src)
            : backend_{backend},
              saved_mask_{backend.registers_.unavailable_mask()},
              saved_count_{backend.registers_.allocations().size()} {

            for (const operand* value : {&dst, &src}) {
                if (value->is_register() or value->is_memory()) {
                    backend_.registers_.protect(
                        register_mask(value->base_register()));
                }

                if (value->is_memory()) {
                    backend_.registers_.protect(
                        register_mask(value->index_register()));
                }
            }
        }

        // protects nothing, only frees what is allocated while it lives
        explicit address_scope(machine_rv32i& backend)
            : address_scope{backend, operand{}, operand{}} {}

        address_scope(const address_scope&) = delete;
        address_scope(address_scope&&) = delete;
        auto operator=(const address_scope&) -> address_scope& = delete;
        auto operator=(address_scope&&) -> address_scope& = delete;

        ~address_scope() {
            while (backend_.registers_.allocations().size() > saved_count_) {
                // implicit releases need the allocation context for a balanced
                // trace
                const register_pool::allocation entry{
                    backend_.registers_.top(),
                };

                backend_.comment(entry.src_loc_tk, entry.indent,
                                 "free {} register {}",
                                 entry.named ? "named" : "scratch",
                                 register_names_.at(entry.index));

                std::ignore = backend_.registers_.pop(entry.index);
            }

            backend_.registers_.restore_unavailable(saved_mask_);
        }
    };

    struct address_offset_parts {
        uint32_t upper;
        int32_t low;
    };

    // a 32-bit multiplier can need one digit above bit 31 in non-adjacent form
    static constexpr size_t multiplier_digit_count{33};

    // 'value' is modified in place then written back by
    // 'store_operation_result' through 'address'
    struct loaded_destination {
        operand address;
        operand value;
    };

    // a multiplier as signed digits: a factor of 2^i is added or subtracted per
    // nonzero digit
    struct digit_sequence {
        std::array<int, multiplier_digit_count> digits;
        // the digits build the negated multiplier
        bool negate;
        // the highest and the lowest nonzero digit
        size_t top;
        size_t lowest;
    };

    // a store of constant bytes at 'offset' with 'size_bytes' of 1, 2 or 4
    struct byte_part {
        size_t offset{};
        size_t size_bytes{};
        int64_t value{};
        // zero is stored from register 'zero' and a repeated value is reused
        bool needs_load{};
    };

    // note: zero, copy and compare each have their own functions for a known
    //       size ('zero_known_size', 'copy_known_size', 'compare_known_size');
    //       the code is repeated on purpose, so each function reads from top
    //       to bottom; only plain decisions are shared ('aligned_parts',
    //       'plan_loop_start', 'plan_known_loop'); copy and compare share the
    //       walk of a run-time count ('walk_runtime_count') and differ only in
    //       the access they emit
    //       * the variables base 's0' and a non-inline frame base 's1' are
    //         word aligned, so an unindexed address from them has a known
    //         position within a word; other addresses are only as aligned as
    //         their type
    //       * the pointers of a loop advance together, so the loop keeps one
    //         width that every address reaches after the same head of a byte
    //         and a halfword
    //       * a known size decides the head, loop and tail at compile time, a
    //         run-time count checks the head against the count and selects the
    //         tail by its low bits

    // a numeric local label as defined and as a branch refers to it
    struct local_label {
        std::string_view name;
        std::string_view reference;
    };

    // every local label of the bulk operations, listed together so none is
    // used twice
    static constexpr local_label chunk_loop{
        .name{"1"},
        .reference{"1b"},
    };
    static constexpr local_label after_chunks{
        .name{"2"},
        .reference{"2f"},
    };
    static constexpr local_label after_halfword{
        .name{"3"},
        .reference{"3f"},
    };
    static constexpr local_label walk_end{
        .name{"4"},
        .reference{"4f"},
    };
    static constexpr local_label false_exit{
        .name{"5"},
        .reference{"5f"},
    };
    static constexpr local_label result_end{
        .name{"6"},
        .reference{"6f"},
    };
    static constexpr local_label after_head{
        .name{"7"},
        .reference{"7f"},
    };

    // the labels of a bounds check: a failing check branches to 'bounds_fail'
    // where the line and the handler follow, a passing one to 'bounds_pass'
    static constexpr local_label bounds_fail{
        .name{"1"},
        .reference{"1f"},
    };
    static constexpr local_label bounds_pass{
        .name{"2"},
        .reference{"2f"},
    };

    // an access of 'width' bytes 'offset' bytes past an address
    struct access_part {
        size_t offset{};
        size_t width{};
    };

    // the loop runs 'width' accesses after 'head_size_bytes' of narrower ones
    struct loop_start {
        size_t head_size_bytes{};
        size_t width{};
    };

    // what a known size needs as a loop; e.g. 24 bytes with both addresses 1 B
    // past a word boundary and an access of 4 bytes is a 3 B head, 5 chunks of
    // 4 B and a 1 B tail
    struct known_loop {
        size_t head_size_bytes{};
        size_t width{};
        // the chunks of 'width' bytes after the head and the bytes after the
        // chunks
        size_t chunk_count{};
        size_t tail_size_bytes{};
    };

    // two pointers walked by a run-time byte count, 'access' emits one access
    // of 'width' bytes at the current pointers and 'verb' names it in comments
    struct runtime_walk {
        token src_loc_tk;
        size_t indent{};
        std::string_view verb;
        operand src;
        operand dst;
        operand count;
        std::function<void(size_t)> access;
    };

    // the registers a comparison loads both sides into
    struct compare_registers {
        operand left;
        operand right;
        // a register result holds the left value and saves the final copy
        bool left_is_result{};
    };

    // the two memory operands of a comparison of a known size and where each
    // access starts in its word
    struct compared_memory {
        operand left;
        operand right;
        std::array<access_start, 2> starts;
    };

    // the pointer registers of a comparison with a run-time byte count
    struct compared_range {
        operand left;
        operand right;
        operand count;
        std::array<access_start, 2> starts;
    };

    // empty when no binary image is written
    std::string binary_file_name_;
    assembler_rv32i assembler_;
    bool variables_base_reserved_{};
    bool frame_base_reserved_{};
    bool multiply_helper_used_{};
    bool divide_helper_used_{};
    // the shared tail of the handlers that report a line was emitted
    bool line_report_emitted_{};
    std::vector<std::array<operand, 3>> bulk_registers_;
    std::vector<bulk_addresses> bulk_addresses_;

  protected:
    //
    // overridden methods
    //

    [[nodiscard]] auto target_assembler() -> ::assembler& override {
        return assembler_;
    }

    [[nodiscard]] auto target_assembler() const -> const ::assembler& override {
        return assembler_;
    }

    //
    // virtual methods
    //

    // devices with a fixed memory size reject images that do not fit, an
    // operating system loads the program where it has room
    virtual auto
    check_memory_end([[maybe_unused]] const size_t memory_end_address) const
        -> void {}

    // a0 is the descriptor and receives the byte count, a1 is the address,
    // a2 the count and a7 is reserved, other registers keep their values
    virtual auto emit_read_call(const size_t indent) -> void {
        assembler_.li(indent, "a7", syscall_read_);
        assembler_.ecall(indent);
    }

    // same registers as 'emit_read_call'
    virtual auto emit_write_call(const size_t indent) -> void {
        assembler_.li(indent, "a7", syscall_write_);
        assembler_.ecall(indent);
    }

    //
    // class methods
    //

    [[nodiscard]] auto assembler() -> assembler_rv32i& { return assembler_; }

    // i/o routines replacing system calls return through a7 and change only
    // 'clobbered' besides a0, so the call keeps just the live ones like a
    // system call does
    auto call_io_routine(const size_t indent, const std::string_view label,
                         const std::span<const std::string_view> clobbered)
        -> void {

        // the clobbered registers follow a1 and a2, which the call needs, in
        // the allocation order, so they are free whenever the call is made
        assert(std::ranges::none_of(clobbered,
                                    [&](const std::string_view name) -> bool {
                                        return is_register_allocated(name);
                                    }));

        assembler_.call(indent, label, "a7");
    }

  public:
    // 'binary_file_name' receives the image of the resolved output, backend
    // tests without complete programs leave it empty
    explicit machine_rv32i(std::ostream* const direct_output,
                           const source_files* const files = nullptr,
                           const jump_mode jumps = jump_mode::resolved,
                           const std::string_view binary_file_name = {})
        : machine{direct_output, files, jumps},
          binary_file_name_{binary_file_name} {

        // output before 'start' is written as emitted
        assembler_.set_direct_output(direct_stream());
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

        binary_operation(src_loc_tk, indent,
                         operation == arithmetic_operator::add ? op::add
                                                               : op::sub,
                         dst, src);
    }

    auto address_of(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& address)
        -> void override {

        assert(dst.is_register() or dst.is_memory());
        assert(dst.type_ref().size_bytes() == word_size_bytes_);

        const address_scope scope{*this, dst, address};
        const operand value{working_register(src_loc_tk, indent, dst)};

        const operand lowered{
            lower_address(src_loc_tk, indent, address, value),
        };

        // a distinct base or nonzero residual offset still needs an add
        if (register_index(value.base_register()) !=
                register_index(lowered.base_register()) or
            lowered.displacement() != 0) {

            assembler_.addi(indent, value.base_register(),
                            lowered.base_register(), lowered.displacement());
        }

        if (dst.is_memory()) {
            copy_value(src_loc_tk, indent, dst, value);
        }
    }

    [[nodiscard]] auto address_size_bytes() const -> size_t override {
        return word_size_bytes_;
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        const size_t index{register_index(register_name)};
        const uint32_t mask{register_mask(register_name)};

        // the zero register and the stack pointer are never allocated
        const bool is_fixed{
            index == register_index("zero") or index == register_index("sp"),
        };

        if (mask == 0 or is_fixed or registers_.is_unavailable(mask)) {
            throw register_error(
                src_loc_tk,
                std::format("cannot allocate register {}", register_name),
                registers_);
        }

        operand result{make_register_operand(register_name, type_ref)};
        result.set_allocation_register(register_names_.at(index));
        record_allocation(src_loc_tk, indent, index, type_ref, true);

        comment(src_loc_tk, indent, "allocate named register {}",
                register_names_.at(index));

        return result;
    }

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref)
        -> operand override {

        validate_scalar(src_loc_tk, type_ref);
        for (const size_t index : scratch_registers_) {
            if (registers_.is_unavailable(uint32_t{1} << index)) {
                continue;
            }

            record_allocation(src_loc_tk, indent, index, type_ref, false);

            record_scratch_registers(registers_);

            comment(src_loc_tk, indent, "allocate scratch register -> {}",
                    register_names_.at(index));

            operand result{
                make_register_operand(register_names_.at(index), type_ref),
            };

            result.set_allocation_register(register_names_.at(index));

            return result;
        }

        throw register_error(src_loc_tk, "out of RV32I scratch registers",
                             registers_);
    }

    auto arrays_equal(const token& src_loc_tk, const size_t indent,
                      const size_t element_size_bytes,
                      const std::function_ref<void(const operand&)> emit_count,
                      const equality_request& request) -> void override {

        const operand count{begin_array_operation(src_loc_tk, indent)};

        emit_count(count);

        // borrowed pointers avoid a temporary address and final move for
        // indexing
        request.lhs(count, bulk_registers_.back().at(0),
                    [&](const operand& address) -> void {
                        set_bulk_address(src_loc_tk, indent, 0, address);
                    });

        request.rhs(count, bulk_registers_.back().at(1),
                    [&](const operand& address) -> void {
                        set_bulk_address(src_loc_tk, indent, 1, address);
                    });

        const std::array<operand, 3>& registers{bulk_registers_.back()};
        {
            // scaling the count must not pick the result registers
            const address_scope scope{*this, request.dst, operand{}};

            comment(src_loc_tk, indent,
                    "{}: elements to bytes ({} bytes/element)",
                    registers.at(2).base_register(), element_size_bytes);

            scale_index(src_loc_tk, indent, registers.at(2),
                        element_size_bytes);
        }

        compare_runtime_count(src_loc_tk, indent,
                              {
                                  .left{registers.at(0)},
                                  .right{registers.at(1)},
                                  .count{registers.at(2)},
                                  .starts{bulk_starts()},
                              },
                              request);

        release_bulk(src_loc_tk, indent);
    }

    auto begin_data(const size_t alignment) -> void override {
        emit_arithmetic_helpers();
        assembler_.switch_section(section::data);
        assembler_.align(alignment);
        label(0, data_label);
    }

    auto bitwise(const token& src_loc_tk, const size_t indent,
                 const arithmetic_operator operation, const operand& dst,
                 const operand& src) -> void override {

        assert(operation == arithmetic_operator::bit_and or
               operation == arithmetic_operator::bit_or or
               operation == arithmetic_operator::bit_xor);

        op instruction{op::xor_op};

        if (operation == arithmetic_operator::bit_and) {
            instruction = op::and_op;
        } else if (operation == arithmetic_operator::bit_or) {
            instruction = op::or_op;
        }

        binary_operation(src_loc_tk, indent, instruction, dst, src);
    }

    auto branch(const size_t indent, const std::string_view target)
        -> void override {

        emit_jump(indent, op::j, {}, {}, target);
    }

    auto call_function(const token& src_loc_tk, const size_t indent,
                       const std::string_view label,
                       const operand& frame_address,
                       const operand& slot_address) -> void override {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());

        assert(register_index(frame_address.base_register()) !=
               register_index("sp"));

        // the callee may use every register but the variables base, so only
        // values live at the call need saving
        std::vector<std::string_view> saved;
        for (const register_pool::allocation& allocated :
             registers_.allocations()) {

            if (allocated.index == register_index(variables_base_register_)) {
                continue;
            }

            saved.push_back(register_names_.at(allocated.index));
        }

        record_noinline_call(src_loc_tk, label, saved.size());

        if (not saved.empty()) {
            comment(src_loc_tk, indent,
                    "before call: save allocated registers");
        }

        const size_t stack_bytes{save_registers(indent, saved)};

        load_slot_address(src_loc_tk, indent, slot_address);

        comment(src_loc_tk, indent, "set function frame base");

        address_of(src_loc_tk, indent,
                   make_register_operand(frame_base_register(), default_type()),
                   frame_address);

        assembler_.call(indent, label);

        if (stack_bytes != 0) {
            comment(src_loc_tk, indent, "after call: restore saved registers");
        }

        restore_saved_registers(indent, saved, stack_bytes);
    }

    [[nodiscard]] auto can_lower_index_scale(const size_t size_bytes) const
        -> bool override {

        return std::has_single_bit(size_bytes) and
               size_bytes <= std::numeric_limits<uint32_t>::max();
    }

    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const operand& reg_to_check, const size_t array_count,
                      const bool allow_end, const operand& reg_count,
                      const bounds_check_options& options) -> void override {

        if (not options.upper and not options.lower) {
            return;
        }

        // array lengths are limited by the variables that hold the arrays
        assert(array_count <= std::numeric_limits<uint32_t>::max() and
               src_loc_tk.at_line() <= std::numeric_limits<uint32_t>::max());

        comment(src_loc_tk, indent, "bounds check begin");

        emit_bounds_check(src_loc_tk, indent, reg_to_check, array_count,
                          allow_end, reg_count, options);

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
        assert(frame_address.index_register().empty());
        assert(frame_size_bytes.is_immediate());

        const address_scope scope{*this, frame_address, frame_size_bytes};

        // 'overflow' is where a frame outside of 'vars' and one too large for
        // it end up, 'fits' is after the jump to the handler
        constexpr local_label overflow{
            .name{"1"},
            .reference{"1f"},
        };

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        comment(src_loc_tk, indent, "frame capacity check begin");

        comment(src_loc_tk, indent,
                "callee storage starts after the caller's storage ({})",
                format_address(frame_address));

        const operand start{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const operand remaining{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        address_of(src_loc_tk, indent, start, frame_address);

        if (register_mask(frame_address.base_register()) != 0 and
            frame_address.displacement() != 0) {

            assembler_.branch(
                indent, frame_address.displacement() > 0 ? op::bltu : op::bgtu,
                start.base_register(), frame_address.base_register(),
                overflow.reference);
        }

        assembler_.la(indent, remaining.base_register(), variables_label);

        assembler_.bltu(indent, start.base_register(),
                        remaining.base_register(), overflow.reference);

        assembler_.la(indent, remaining.base_register(), variables_end_label);

        assembler_.bltu(indent, remaining.base_register(),
                        start.base_register(), overflow.reference);

        assembler_.sub(indent, remaining.base_register(),
                       remaining.base_register(), start.base_register());

        assembler_.lui(indent, start.base_register(),
                       assembler_rv32i::immediate::of_symbol(
                           frame_size_bytes.immediate(),
                           assembler_rv32i::immediate::part::high));

        assembler_.addi(indent, start.base_register(), start.base_register(),
                        assembler_rv32i::immediate::of_symbol(
                            frame_size_bytes.immediate(),
                            assembler_rv32i::immediate::part::low));

        assembler_.bgeu(indent, remaining.base_register(),
                        start.base_register(), fits.reference);

        assembler_.label(indent, overflow.name);
        branch(indent, frame_overflow_handler_label);
        assembler_.label(indent, fits.name);
        free_scratch_register(src_loc_tk, indent, remaining);
        free_scratch_register(src_loc_tk, indent, start);
        comment(src_loc_tk, indent, "frame capacity check end");
    }

    auto comment_alias(const token& src_loc_tk, const size_t indent,
                       const std::string_view from, const std::string_view to,
                       [[maybe_unused]] const operand& address)
        -> void override {

        comment(src_loc_tk, indent, "alias {} -> {}", from, to);
    }

    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t size_bytes,
                          const operand& address) -> void override {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, size_bytes,
                format_address(address));
    }

    auto
    compare_and_branch(const token& src_loc_tk, const size_t indent,
                       const operand& lhs, const operand& rhs,
                       const comparison_action& action,
                       const std::span<const operand> scratch_registers_to_free)
        -> void override {

        // the address scopes in 'emit_comparison' end before these are freed
        emit_comparison(src_loc_tk, indent, lhs, rhs, action);
        free_scratch_registers(src_loc_tk, indent, scratch_registers_to_free);
    }

    auto copy(const token& src_loc_tk, const size_t indent, const operand& src,
              const operand& dst, const size_t size_bytes,
              const size_t alignment) -> void override {

        if (size_bytes == 0) {
            return;
        }

        if (size_bytes > std::numeric_limits<uint32_t>::max()) {
            throw compiler_exception{src_loc_tk,
                                     "copy size exceeds RV32I address range"};
        }

        // the pointers of a loop advance together, unrolled accesses can
        // follow where each address is within its word
        const std::array<access_start, 2> starts{
            start_of(src, alignment),
            start_of(dst, alignment),
        };

        copy_known_size(src_loc_tk, indent, src, dst, size_bytes, starts);
    }

    // few bytes are stored with immediates when that takes no more code than
    // loading them from read-only data
    auto copy_bytes(const token& src_loc_tk, const size_t indent,
                    const std::string_view bytes, const operand& dst,
                    const size_t alignment,
                    const std::function_ref<std::string()> add_constant)
        -> void override {

        // the scope keeps 'dst' registers from being picked for scratch
        const address_scope scope{*this, dst, operand{}};

        const size_t width{bulk_width(access_alignment(dst, alignment))};

        const access_start dst_start{start_of(dst, alignment)};

        const std::vector<byte_part> parts{split_bytes(bytes, dst_start)};

        // the word aligned constant is copied with the alignment 'width', so
        // its parts can be narrower than the stored immediates
        const std::array<access_start, 2> copy_starts{
            access_start{
                .alignment{width},
                .phase{},
            },
            dst_start,
        };

        const size_t copy_part_count{
            aligned_part_count(bytes.size(), copy_starts),
        };

        if (are_immediates_smaller(parts, bytes.size(), copy_part_count)) {
            comment_aligned_parts(src_loc_tk, indent, "store", bytes.size(),
                                  std::span{&dst_start, 1});

            store_byte_parts(src_loc_tk, indent, parts, dst, bytes.size());

            return;
        }

        const operand pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.la(indent, pointer.base_register(), add_constant());

        // read-only constants are word aligned so 'dst' limits the width
        copy(src_loc_tk, indent,
             operand::mem(pointer.base_register(), {}, 1, 0, dst.type_ref()),
             dst, bytes.size(), width);
    }

    auto copy_elements(const token& src_loc_tk, const size_t indent,
                       const size_t element_size_bytes,
                       const std::function_ref<void(const operand&)> emit_count,
                       const copy_request& request) -> void override {

        const operand count{begin_array_operation(src_loc_tk, indent)};

        emit_count(count);

        // borrowed pointers avoid a temporary address and final move for
        // indexing
        request.src(count, bulk_registers_.back().at(0),
                    [&](const operand& address) -> void {
                        set_bulk_address(src_loc_tk, indent, 0, address);
                    });

        request.dst(count, bulk_registers_.back().at(1),
                    [&](const operand& address) -> void {
                        set_bulk_address(src_loc_tk, indent, 1, address);
                    });

        const std::array<operand, 3>& registers{bulk_registers_.back()};

        comment(src_loc_tk, indent, "{}: elements to bytes ({} bytes/element)",
                registers.at(2).base_register(), element_size_bytes);

        scale_index(src_loc_tk, indent, registers.at(2), element_size_bytes);

        copy_runtime_count(src_loc_tk, indent, registers.at(0), registers.at(1),
                           registers.at(2), bulk_starts(), request.alignment);

        release_bulk(src_loc_tk, indent);
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        validate_operands(src_loc_tk, dst, src);

        assert(not src.is_empty());

        // skip self assignment
        if (same_memory(dst, src)) {
            return;
        }

        const address_scope scope{*this, dst, src};

        const std::optional<int32_t> constant{
            narrowed_immediate(src, dst.type_ref()),
        };

        // known constants are truncated and extended before emission
        if (constant.has_value()) {
            store_constant_result(src_loc_tk, indent, dst, *constant);
            return;
        }

        if (shares_address_base(dst, src)) {
            copy_through_shared_base(src_loc_tk, indent, dst, src);
            return;
        }

        operand value{dst};

        if (not dst.is_register()) {
            value = src.is_register() ? src
                                      : alloc_scratch_register(
                                            src_loc_tk, indent, default_type());
        }

        load_into(src_loc_tk, indent, value, src);

        if (dst.is_memory()) {
            store_low_bits(src_loc_tk, indent, dst, value);
            return;
        }

        if (register_needs_extension(dst.type_ref(), src)) {
            extend_register(indent, dst.type_ref(), value);
        }
    }

    [[nodiscard]] auto data_alignment() const -> size_t override {
        return data_alignment_;
    }

    [[nodiscard]] auto default_type() const -> const type& override {
        return builtin_type_i32();
    }

    auto define_constant(const std::string_view name, const size_t value)
        -> void override {

        // frame sizes are limited by the variables that hold the frames
        assert(value <= std::numeric_limits<uint32_t>::max());

        assembler_.define_constant(name, static_cast<int64_t>(value));
    }

    auto divide(const token& src_loc_tk, const size_t indent,
                const arithmetic_operator operation, const operand& dst,
                const operand& divisor, const division_check_options& check)
        -> void override {

        assert(operation == arithmetic_operator::divide or
               operation == arithmetic_operator::remainder);

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_division_operand(src_loc_tk, divisor);

        assert(dst.is_register() or dst.is_memory());

        divide_helper_used_ = true;

        call_arithmetic_helper(src_loc_tk, indent, dst, divisor, true,
                               operation == arithmetic_operator::remainder,
                               check);
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        label(0, bounds_failure_handler_label);

        if (with_line) {
            emit_line_failure(".Lbaz_bounds_message", "panic: bounds at line ");
        }

        exit(token{}, 1,
             operand::imm(std::format("{}", panic_exit_code), default_type()));
    }

    auto emit_data(const size_t element_size_bytes,
                   const data_initializer& value) -> void override {

        emit_repeated_data(element_size_bytes, 1, value);
    }

    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(data_initializer&)> next)
        -> void override {

        data_initializer value;
        while (next(value)) {
            emit_data(element_size_bytes, value);
        }
    }

    auto emit_division_failure_handler(const bool with_line) -> void override {

        constexpr std::string_view message{"panic: division"};
        constexpr std::array<int64_t, 1> newline{'\n'};

        label(0, division_failure_handler_label);

        if (with_line) {
            emit_line_failure(".Lbaz_division_message",
                              "panic: division at line ");

            return;
        }

        assembler_.li(1, "a0", stderr_descriptor);
        assembler_.la(1, "a1", ".Lbaz_division_message");
        // the newline follows the message text
        assembler_.li(1, "a2", message.size() + 1);
        emit_write_call(1);

        exit(token{}, 1,
             operand::imm(std::format("{}", panic_exit_code), default_type()));

        assembler_.switch_section(section::rodata);
        assembler_.label(0, ".Lbaz_division_message");
        assembler_.ascii(message);
        assembler_.data(1, newline);
        // the next handler may follow and must stay in the code section
        assembler_.switch_section(section::text);
    }

    auto emit_frame_overflow_handler() -> void override {
        constexpr std::string_view message{"panic: frame overflow"};
        constexpr std::array<int64_t, 1> newline{'\n'};

        label(0, frame_overflow_handler_label);
        assembler_.li(1, "a0", stderr_descriptor);
        assembler_.la(1, "a1", ".Lbaz_frame_message");
        // the newline follows the message text
        assembler_.li(1, "a2", message.size() + 1);
        emit_write_call(1);

        exit(token{}, 1,
             operand::imm(std::format("{}", panic_exit_code), default_type()));

        assembler_.switch_section(section::rodata);
        assembler_.label(0, ".Lbaz_frame_message");
        assembler_.ascii(message);
        assembler_.data(1, newline);
        // the bounds handler may follow and must stay in the code section
        assembler_.switch_section(section::text);
    }

    auto
    emit_most_efficient([[maybe_unused]] const token& src_loc_tk,
                        [[maybe_unused]] const size_t indent,
                        const std::function_ref<void()> emit_without_scratch,
                        const std::function_ref<void()> emit_with_scratch)
        -> void override {

        // both versions are buffered to compare sizes, even when output is
        // otherwise written as emitted
        assembler_.emit_buffered([&] -> void {
            assembler_.emit_smaller(emit_without_scratch, emit_with_scratch);
        });
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const data_initializer& value) -> void override {

        // 'validate_data_element_size' rejects other sizes
        assert(element_size_bytes == byte_size_bytes_ or
               element_size_bytes == half_size_bytes_ or
               element_size_bytes == word_size_bytes_);

        assembler_.repeated_data(element_size_bytes, count, value.uops,
                                 value.value);
    }

    auto emit_string_constants(const std::span<const string_constant> strings)
        -> void override {

        assembler_.switch_section(section::rodata);
        for (const string_constant& s : strings) {
            // word alignment lets copies to aligned destinations use words
            assembler_.align(word_size_bytes_);
            assembler_.label(0, s.label);
            emit_string_data(s.text);
        }

        // the arithmetic helpers follow and must stay in the code section
        assembler_.switch_section(section::text);
    }

    auto emit_string_data(const std::string_view value) -> void override {
        assembler_.ascii(token::decode_string(value));
    }

    auto emit_zero_data(const size_t size_bytes) -> void override {
        assembler_.zero(size_bytes);
    }

    auto end_main() -> void override {
        exit(token{}, 1, operand::imm("0", default_type()));
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);

        assembler_.li(indent, "a7", syscall_exit_);
        assembler_.ecall(indent);
    }

    auto finish() -> void override {
        // reserved by 'start', not set in backend testing mode
        if (variables_base_reserved_) {
            release_variables_base();
        }

        assert(bulk_registers_.empty());
        assert(registers_.is_empty());
        assert(registers_.unavailable_mask() == 0);
        assert(not variables_base_reserved_);
        assert(not frame_base_reserved_);

        finish_output();

        // direct output was written as emitted, there is nothing to resolve
        if (not assembler_.is_buffering()) {
            return;
        }

        // the report counts after resolving because grown jumps take more
        // instructions, a failing build writes nothing
        assembler_.resolve_jumps();
        check_address_range(assembler_.memory_end_address());
        check_memory_end(assembler_.memory_end_address());
    }

    auto foo_advance_iteration(const token& src_loc_tk, const size_t indent,
                               const operand& iterator, const operand& counter,
                               const size_t element_size_bytes,
                               const operand& limit,
                               const std::string_view loop_label)
        -> void override {

        // element sizes are limited by the variables that hold the arrays
        assert(element_size_bytes <= std::numeric_limits<uint32_t>::max());

        const address_scope scope{*this, iterator, counter};

        add_subtract(src_loc_tk, indent, arithmetic_operator::add, iterator,
                     operand::imm(std::format("{}", element_size_bytes),
                                  default_type()));

        add_subtract(src_loc_tk, indent, arithmetic_operator::add, counter,
                     operand::imm("1", default_type()));

        emit_comparison(src_loc_tk, indent, counter, limit,
                        {
                            .operation{comparison_operator::not_equal},
                            .inverted{},
                            .destination{},
                            .target{loop_label},
                            .branch_on_true{true},
                        });
    }

    // a load and store machine counts in a register
    [[nodiscard]] auto foo_counter_in_memory() const -> bool override {
        return false;
    }

    [[nodiscard]] auto frame_base_register() const
        -> std::string_view override {

        return "s1";
    }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void override {

        free_scratch_register(src_loc_tk, indent, reg);
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indent,
                               const operand& reg) -> void override {

        const size_t index{register_index(reg.allocation_register())};

        // named and scratch allocations share the same lifo pool
        comment(src_loc_tk, indent, "free {} register {}",
                registers_.top().named ? "named" : "scratch",
                register_names_.at(index));

        std::ignore = registers_.pop(index);
    }

    auto label(const size_t indent, const std::string_view label)
        -> void override {

        assembler_.label(indent, label);
    }

    [[nodiscard]] auto make_register_operand(const std::string_view name,
                                             const type& value_type) const
        -> operand override {

        // the allocating callers validated the type with their token
        assert(is_scalar(value_type));

        const size_t index{register_index(name)};

        assert(index != register_names_.size());

        return operand::reg(register_names_.at(index), value_type);
    }

    [[nodiscard]] auto max_storage_bytes() const -> size_t override {
        return address_space_bytes_;
    }

    // every known size is compared from the address operands, small ones
    // unrolled and larger ones in a loop, both addresses stay in their
    // registers until it has compared
    auto memory_equal(const token& src_loc_tk, const size_t indent,
                      const size_t size_bytes, const equality_request& request)
        -> void override {

        request.lhs({}, {}, [&](const operand& left) -> void {
            request.rhs({}, {}, [&](const operand& right) -> void {
                compare_memory(src_loc_tk, indent, size_bytes, left, right,
                               request);
            });
        });
    }

    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  [[maybe_unused]] const bool reuse_source = {})
        -> void override {

        validate_scalar(src_loc_tk, product.type_ref());
        validate_scalar(src_loc_tk, factor.type_ref());
        validate_destination_storage(src_loc_tk, product);

        // validate memory operands even when a constant eliminates the
        // operation

        if (factor.is_memory()) {
            validate_address(src_loc_tk, factor);
        }

        const std::optional<int32_t> constant{immediate_value(factor)};

        // variable factors use the shared runtime helper
        if (not constant.has_value()) {
            multiply_helper_used_ = true;

            call_arithmetic_helper(src_loc_tk, indent, product, factor, false,
                                   false, {});

            return;
        }

        multiply_by_constant(src_loc_tk, indent, product, factor, *constant);
    }

    auto read(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& descriptor, const operand& address,
              const operand& count) -> void override {

        const operand call_register{
            reserve_io_call_register(src_loc_tk, indent, dst, descriptor,
                                     address, count),
        };

        emit_read_call(indent);
        free_named_register(src_loc_tk, indent, call_register);
    }

    [[nodiscard]] auto register_display_name(const size_t index) const
        -> std::string override {

        return std::string{register_names_.at(index)};
    }

    [[nodiscard]] auto register_pool_ref() -> register_pool& override {
        return registers_;
    }

    [[nodiscard]] auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers override {

        static constexpr std::array<std::string_view, 3> io_args{
            "a0",
            "a1",
            "a2",
        };

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

    auto release_frame_base() -> void override {
        assert(frame_base_reserved_);

        operand base{
            make_register_operand(frame_base_register(), default_type()),
        };

        base.set_allocation_register(frame_base_register());
        free_named_register(token{}, 0, base);
        frame_base_reserved_ = false;
    }

    auto reserve_frame_base() -> void override {
        assert(not frame_base_reserved_);

        std::ignore = alloc_named_register(token{}, 0, frame_base_register(),
                                           default_type());

        frame_base_reserved_ = true;

        assembler_.addi(1, "sp", "sp", -frame_save_bytes_);
        assembler_.sw(1, "ra", 0, "sp");
    }

    auto reserve_variables(const size_t alignment, const size_t size_bytes)
        -> void override {

        label(0, data_end_label);
        // variables are zeroed when defined, so the image does not hold them
        assembler_.switch_section(section::bss);
        assembler_.align(alignment);
        label(0, variables_label);
        assembler_.zero(size_bytes);
        label(0, variables_end_label);
    }

    auto return_function(const size_t indent) -> void override {
        assembler_.lw(indent, "ra", 0, "sp");
        assembler_.addi(indent, "sp", "sp", frame_save_bytes_);
        assembler_.ret(indent);
    }

    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size_bytes)
        -> void override {

        // index scaling uses the target's address width, element sizes are
        // limited by the variables that hold the arrays
        assert(index.type_ref().size_bytes() == address_size_bytes());
        assert(element_size_bytes <= std::numeric_limits<uint32_t>::max());

        multiply(src_loc_tk, indent, index,
                 operand::imm(std::format("{}", element_size_bytes),
                              default_type()));
    }

    [[nodiscard]] auto scratch_register_total() const -> size_t override {
        return scratch_registers_.size();
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const operand& count) -> void override {

        assert(operation == arithmetic_operator::shift_left or
               operation == arithmetic_operator::shift_right);

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_shift_operand(src_loc_tk, count);
        validate_destination_storage(src_loc_tk, dst);

        const std::optional<int32_t> constant{immediate_value(count)};

        // immediate shifts must be resolved here rather than by the assembler
        assert(not count.is_immediate() or constant.has_value());

        const size_t bits{dst.type_ref().size_bits()};

        if (constant.has_value() and
            (*constant < 0 or std::cmp_greater_equal(*constant, bits))) {

            throw compiler_exception{
                src_loc_tk,
                std::format("RV32I shift count must be 0 to {} for {}-bit "
                            "values",
                            bits - 1, bits)};
            // note: bits - 1 because the count is below the width
        }

        const uint32_t shift_count{
            static_cast<uint32_t>(constant.value_or(0)),
        };

        if (constant.has_value() and shift_count == 0) {
            return;
        }

        const address_scope scope{*this, dst, count};

        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, dst),
        };

        if (constant.has_value()) {
            shift_by_constant(indent, operation, dst, loaded, shift_count,
                              bits);

            return;
        }

        shift_by_register(src_loc_tk, indent, operation, dst, count, loaded);
    }

    [[nodiscard]] auto slot_register() const -> std::string_view override {
        return slot_register_;
    }

    auto start() -> void override {
        multiply_helper_used_ = false;
        divide_helper_used_ = false;
        line_report_emitted_ = false;

        start_output();

        assembler_.option_norvc();
        assembler_.option_norelax();
        assembler_.add_separator_newline();
        assembler_.switch_section(section::text);
        assembler_.globl("_start");
        label(0, "_start");
        assembler_.add_separator_newline();
        reserve_variables_base();
        assembler_.la(0, variables_base_register_, variables_label);

        assembler_.addi(0, variables_base_register_, variables_base_register_,
                        static_cast<int64_t>(variables_base_past_vars_bytes_));

        assembler_.add_separator_newline();
    }

    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void override {

        copy_value(src_loc_tk, indent, dst,
                   operand::imm(value ? "1" : "0", default_type()));
    }

    auto unary(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst)
        -> void override {

        assert(operation == arithmetic_operator::negate or
               operation == arithmetic_operator::complement);

        validate_scalar(src_loc_tk, dst.type_ref());

        assert(dst.is_register() or dst.is_memory());

        const address_scope scope{*this, dst, operand{}};

        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, dst),
        };

        if (operation == arithmetic_operator::negate) {
            assembler_.sub(indent, loaded.value.base_register(), "zero",
                           loaded.value.base_register());

            store_operation_result(indent, dst, loaded.address, loaded.value,
                                   true);

            return;
        }

        // 'not' of a bool flips the stored byte only
        const int mask{
            dst.type_ref().is_bool() ? std::numeric_limits<uint8_t>::max() : -1,
        };

        assembler_.xori(indent, loaded.value.base_register(),
                        loaded.value.base_register(), mask);

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               false);
    }

    auto validate_data_element_size(const token& src_loc_tk,
                                    const size_t element_size_bytes) const
        -> void override {

        if (element_size_bytes != byte_size_bytes_ and
            element_size_bytes != half_size_bytes_ and
            element_size_bytes != word_size_bytes_) {

            throw compiler_exception{
                src_loc_tk, "RV32I data elements must be 1, 2, or 4 bytes"};
        }
    }

    auto validate_division_operand(const token& src_loc_tk,
                                   const operand& divisor) const
        -> void override {

        validate_scalar(src_loc_tk, divisor.type_ref());
    }

    auto validate_shift_operand(const token& src_loc_tk,
                                const operand& count) const -> void override {

        validate_scalar(src_loc_tk, count.type_ref());
    }

    [[nodiscard]] auto variables_base_past_vars_bytes() const
        -> std::optional<size_t> override {

        return variables_base_past_vars_bytes_;
    }

    [[nodiscard]] auto variables_base_register() const
        -> std::string_view override {

        return variables_base_register_;
    }

    auto write(const token& src_loc_tk, const size_t indent, const operand& dst,
               const operand& descriptor, const operand& address,
               const operand& count) -> void override {

        const operand call_register{
            reserve_io_call_register(src_loc_tk, indent, dst, descriptor,
                                     address, count),
        };

        emit_write_call(indent);
        free_named_register(src_loc_tk, indent, call_register);
    }

    // a named binary image is written together with the assembly source
    auto write_assembly(std::ostream& os) -> void override {
        // a build buffers its output
        assert(assembler_.is_buffering());

        assembler_.set_direct_output(direct_stream());

        if (binary_file_name_.empty()) {
            assembler_.write_resolved(os);
            return;
        }

        std::ofstream binary{binary_file_name_, std::ios::binary};

        if (not binary) {
            throw std::runtime_error{
                std::format("cannot write '{}'", binary_file_name_)};
        }

        assembler_.write_resolved(os, binary);
    }

    auto zero(const token& src_loc_tk, const size_t indent, const operand& dst,
              const size_t size_bytes, const size_t alignment)
        -> void override {

        assert(size_bytes != 0);

        const access_start start{start_of(dst, alignment)};

        zero_known_size(src_loc_tk, indent, dst, size_bytes, start);
    }

    //
    // class methods
    //

    auto extend_register(const size_t indent, const type& dst_type,
                         const operand& value) -> void {

        const size_t shift{register_bits_ - dst_type.size_bits()};

        // discard high bits, then sign-extend integers or zero-extend bool
        assembler_.slli(indent, value.base_register(), value.base_register(),
                        shift);

        assembler_.immediate_op(indent, extend_shift_op(dst_type),
                                value.base_register(), value.base_register(),
                                shift);
    }

    auto release_variables_base() -> void {
        assert(variables_base_reserved_);

        operand base{
            make_register_operand(variables_base_register(), default_type()),
        };

        base.set_allocation_register(variables_base_register());

        free_named_register(token{}, 0, base);

        variables_base_reserved_ = false;
    }

    auto reserve_variables_base() -> void {
        assert(not variables_base_reserved_);

        std::ignore = alloc_named_register(
            token{}, 0, variables_base_register(), default_type());

        variables_base_reserved_ = true;
    }

    auto store_low_bits(const token& src_loc_tk, const size_t indent,
                        const operand& dst, const operand& value) -> void {

        const operand lowered{lower_address(src_loc_tk, indent, dst)};

        // store the low 32, 16, or 8 bits at base + displacement
        assembler_.store(indent, store_op(dst.type_ref().size_bytes()),
                         value.base_register(), lowered.displacement(),
                         lowered.base_register());
    }

  private:
    // a direct offset from a word aligned base can prove more alignment than
    // the type does
    [[nodiscard]] auto access_alignment(const operand& address,
                                        const size_t alignment) const
        -> size_t {

        if (not is_word_based(address)) {
            return alignment;
        }

        // the lowest set bit of the displacement, capped at a word; the low
        // bits of a negative displacement give the same alignment
        const size_t displacement_alignment{
            offset_alignment(static_cast<size_t>(address.displacement()),
                             word_size_bytes_),
        };

        return std::max(alignment, displacement_alignment);
    }

    // the destination holds the computed address when that keeps its inputs
    [[nodiscard]] auto address_result_register(const token& src_loc_tk,
                                               const size_t indent,
                                               const operand& address,
                                               const operand& dst) -> operand {

        if (can_reuse_address_destination(address, dst)) {
            return dst;
        }

        return alloc_scratch_register(src_loc_tk, indent, default_type());
    }

    auto advance(const size_t indent, const operand& pointer,
                 const size_t size_bytes) -> void {

        assembler_.addi(indent, pointer.base_register(),
                        pointer.base_register(), size_bytes);
    }

    [[nodiscard]] auto alloc_compare_registers(
        const token& src_loc_tk, const size_t indent, const operand& result,
        const std::span<const operand> in_use) -> compare_registers {

        const bool left_is_result{can_hold_left_value(result, in_use)};

        const operand left{
            left_is_result
                ? result
                : alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const operand right{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        comment(src_loc_tk, indent, "{}: left value/result, {}: right value",
                left.base_register(), right.base_register());

        comment(src_loc_tk, indent, "stop at first mismatch");

        return {
            .left{left},
            .right{right},
            .left_is_result{left_is_result},
        };
    }

    // the bulk registers of an array copy or comparison
    auto begin_array_operation(const token& src_loc_tk, const size_t indent)
        -> operand {

        const operand count{begin_bulk(src_loc_tk, indent)};

        const std::array<operand, 3>& registers{bulk_registers_.back()};

        comment(src_loc_tk, indent, "{}: source, {}: destination, {}: count",
                registers.at(0).base_register(),
                registers.at(1).base_register(), count.base_register());

        return count;
    }

    auto begin_bulk(const token& src_loc_tk, const size_t indent) -> operand {
        // argument-order allocation keeps pointer and count names easy to
        // follow
        std::array<operand, 3> registers;
        for (operand& reg : registers) {
            reg = alloc_scratch_register(src_loc_tk, indent, default_type());
        }

        bulk_registers_.push_back(registers);
        bulk_addresses_.emplace_back();

        return registers.back();
    }

    auto binary_operation(const token& src_loc_tk, const size_t indent,
                          const op instruction, const operand& dst,
                          const operand& src) -> void {

        validate_operands(src_loc_tk, dst, src);

        const std::optional<int32_t> constant{
            narrowed_immediate(src, dst.type_ref()),
        };

        assert_folded(instruction, dst, constant);

        if (constant.has_value() and
            yields_constant(instruction, *constant,
                            dst.type_ref().size_bytes())) {

            store_constant_result(src_loc_tk, indent, dst, *constant);
            return;
        }

        const address_scope scope{*this, dst, src};

        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, dst),
        };

        emit_binary_instruction(src_loc_tk, indent, instruction, src,
                                loaded.value, constant);

        store_operation_result(
            indent, dst, loaded.address, loaded.value,
            needs_normalize(instruction, dst, src, constant));
    }

    // a missing address would leave the loop width unproven
    [[nodiscard]] auto bulk_starts() const -> std::array<access_start, 2> {
        const bulk_addresses& addresses{bulk_addresses_.back()};

        assert(addresses.count == 2);

        return addresses.starts;
    }

    auto call_arithmetic_helper(const token& src_loc_tk, const size_t indent,
                                const operand& dst, const operand& source,
                                const bool division, const bool remainder,
                                const division_check_options& check) -> void {

        const address_scope scope{*this, dst, source};

        const uint32_t live{registers_.unavailable_mask()};

        const std::span<const std::string_view> clobbered{
            helper_clobbers(division),
        };

        const std::vector<std::string_view> saved{
            live_clobbered(live, clobbered, dst),
        };

        // staging registers must survive the helper call
        for (const std::string_view name : clobbered) {
            registers_.protect(register_mask(name));
        }

        // variables and frames are based on 's0' and 's1', and the save area
        // moves sp
        assert(not uses_register(dst, "sp"));
        assert(not uses_register(source, "sp"));

        // save caller values before argument setup overwrites a0 or a1
        const size_t stack_bytes{save_registers(indent, saved)};

        load_helper_arguments(src_loc_tk, indent, dst, source);

        if (check.enabled) {
            check_divisor(src_loc_tk, indent, dst.type_ref().size_bytes(),
                          check.with_line);
        }

        assembler_.call(indent, division ? ".Lbaz_divide" : ".Lbaz_multiply");

        const operand result{
            operand::reg(remainder ? "a1" : "a0", default_type()),
        };

        // the destination is not restored so it can retain the result
        if (dst.is_register()) {
            copy_value(src_loc_tk, indent, dst, result);
            restore_saved_registers(indent, saved, stack_bytes);
            return;
        }

        const operand kept{
            preserved_helper_result(src_loc_tk, indent, result, saved),
        };

        restore_saved_registers(indent, saved, stack_bytes);

        // memory destinations need their original address registers back
        copy_value(src_loc_tk, indent, dst, kept);
    }

    // jumps to the failure handler when the divisor in 'a1' is zero or the
    // dividend in 'a0' is the minimum of its width and the divisor is -1, the
    // line is passed in 'a0'
    auto check_divisor(const token& src_loc_tk, const size_t indent,
                       const size_t dividend_size_bytes, const bool with_line)
        -> void {

        constexpr size_t bits_per_byte{8};

        // note: setting all bits above the sign bit of the width gives its
        //       minimum, -1 because the sign bit is the highest bit
        const int64_t minimum{
            static_cast<int32_t>(
                ~uint32_t{} << ((dividend_size_bytes * bits_per_byte) - 1)),
        };

        constexpr local_label fail{
            .name{"1"},
            .reference{"1f"},
        };

        constexpr local_label pass{
            .name{"2"},
            .reference{"2f"},
        };

        comment(src_loc_tk, indent, "division check begin");

        comment(src_loc_tk, indent, "zero divisor");

        assembler_.beqz(indent, "a1", fail.reference);

        comment(src_loc_tk, indent, "minimum divided by -1 overflows");

        assembler_.li(indent, "a2", -1);
        assembler_.bne(indent, "a1", "a2", pass.reference);
        assembler_.li(indent, "a2", minimum);
        assembler_.bne(indent, "a0", "a2", pass.reference);
        assembler_.label(indent, fail.name);

        comment(src_loc_tk, indent, "failed: report and exit");

        if (with_line) {
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }

        branch(indent, division_failure_handler_label);
        assembler_.label(indent, pass.name);

        comment(src_loc_tk, indent, "division check end");
    }

    // passing checks branch to 'bounds_pass' and failing ones to 'bounds_fail',
    // the last check branches past the handler on success so failures fall
    // through to it
    auto check_lower_bounds(const size_t indent, const std::string_view index,
                            const operand& reg_count, const bool is_last)
        -> void {

        const auto check_negative = [&](const std::string_view reg,
                                        const bool last) -> void {
            assembler_.branch_zero(indent, last ? op::bgez : op::bltz, reg,
                                   last ? bounds_pass.reference
                                        : bounds_fail.reference);
        };

        if (reg_count.is_empty()) {
            check_negative(index, is_last);
            return;
        }

        check_negative(index, false);

        // a negative count passes 'start + count' but spans the address space
        check_negative(reg_count.base_register(), is_last);
    }

    auto check_upper_bound(const token& src_loc_tk, const size_t indent,
                           const std::string_view index,
                           const size_t array_count, const bool allow_end,
                           const operand& reg_count, const bool lower_checked)
        -> void {

        const operand limit{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const std::string top{
            upper_bound_top(src_loc_tk, indent, index, reg_count, limit,
                            lower_checked),
        };

        assembler_.li(indent, limit.base_register(), array_count);

        if (allow_end) {
            assembler_.bgeu(indent, limit.base_register(), top,
                            bounds_pass.reference);

            return;
        }

        assembler_.bltu(indent, top, limit.base_register(),
                        bounds_pass.reference);
    }

    // accesses widen after the first only when a start inside a word was
    // peeled, other sequences keep the uncommented widths of the alignment
    auto comment_aligned_parts(const token& src_loc_tk, const size_t indent,
                               const std::string_view verb,
                               const size_t size_bytes,
                               const std::span<const access_start> starts)
        -> void {

        size_t first_width{};
        size_t widest{};
        for (const access_part& part : aligned_parts(size_bytes, starts)) {
            if (first_width == 0) {
                first_width = part.width;
            }

            widest = std::max(widest, part.width);
        }

        if (widest == first_width) {
            return;
        }

        const std::string parts{describe_parts(size_bytes, starts)};

        if (starts.size() == 1) {
            comment(src_loc_tk, indent,
                    "{} {}: start {}, widest aligned access at each offset",
                    verb, parts, describe_start(starts.front()));

            return;
        }

        comment(src_loc_tk, indent,
                "{} {}: source {}, destination {}, widest access aligned for "
                "both at each offset",
                verb, parts, describe_start(starts.front()),
                describe_start(starts.back()));
    }

    // names the parts of a known-size loop
    auto comment_known_loop(const token& src_loc_tk, const size_t indent,
                            const std::string_view verb, const known_loop& loop,
                            const std::span<const access_start> starts)
        -> void {

        const std::string head{
            loop.head_size_bytes == 0
                ? std::string{}
                : std::format(" after a {} B head", loop.head_size_bytes),
        };

        comment(src_loc_tk, indent, "{} loop of {}-byte accesses{}: {}", verb,
                loop.width, head, describe_starts(starts));
    }

    // names what decided the width of a pointer loop
    auto comment_pointer_loop(const token& src_loc_tk, const size_t indent,
                              const loop_start& start,
                              const std::array<access_start, 2>& typed,
                              const size_t alignment) -> void {

        if (start.head_size_bytes != 0) {
            comment(src_loc_tk, indent,
                    "{}-byte accesses after a {} B head: addresses {} and {}",
                    start.width, start.head_size_bytes,
                    describe_start(typed.front()),
                    describe_start(typed.back()));

            return;
        }

        if (start.width > alignment) {
            comment(src_loc_tk, indent,
                    "{}-byte accesses: both addresses {}-byte aligned, type "
                    "{}-byte aligned",
                    start.width, start.width, alignment);

            return;
        }

        comment(src_loc_tk, indent,
                "{}-byte accesses: type {}-byte aligned, addresses not proven "
                "more aligned",
                start.width, alignment);
    }

    // loads both values and leaves for the false exit on a mismatch
    auto compare_access(const size_t indent, const compare_registers& registers,
                        const size_t width, const operand& left,
                        const operand& right) -> void {

        assembler_.load(indent, unsigned_load_op(width),
                        registers.left.base_register(), left.displacement(),
                        left.base_register());

        assembler_.load(indent, unsigned_load_op(width),
                        registers.right.base_register(), right.displacement(),
                        right.base_register());

        assembler_.bne(indent, registers.left.base_register(),
                       registers.right.base_register(), false_exit.reference);
    }

    // known size: the addresses are memory operands and the result of the
    // request is 1 when every access matched, 0 at the first mismatch; its
    // 'inverted' swaps them
    auto compare_known_size(const token& src_loc_tk, const size_t indent,
                            const size_t size_bytes,
                            const compared_memory& memory,
                            const equality_request& request) -> void {

        const operand& left{memory.left};
        const operand& right{memory.right};
        const operand& result{request.dst};

        // keeps the registers of both addresses from being picked for scratch
        const address_scope operand_scope{*this, right, left};

        // keeps the result registers from being picked for scratch
        const address_scope result_scope{*this, result, operand{}};

        const std::array<operand, 2> in_use{left, right};

        const compare_registers registers{
            alloc_compare_registers(src_loc_tk, indent, result, in_use),
        };

        compare_known_walk(src_loc_tk, indent, size_bytes, memory, registers);

        write_compare_result(src_loc_tk, indent, result, registers,
                             request.inverted);
    }

    // the walk's own registers are freed before the result is written
    auto compare_known_walk(const token& src_loc_tk, const size_t indent,
                            const size_t size_bytes,
                            const compared_memory& memory,
                            const compare_registers& registers) -> void {

        const operand& left{memory.left};
        const operand& right{memory.right};
        const std::array<access_start, 2>& starts{memory.starts};

        const address_scope walk_scope{*this};

        // a load pair and a branch per access beat the pointer setup of a loop
        // for small sizes
        if (size_bytes <= copy_unroll_threshold_bytes_) {
            comment_aligned_parts(src_loc_tk, indent, "compare", size_bytes,
                                  starts);

            const operand left_at{
                unrolled_address(src_loc_tk, indent, left, size_bytes),
            };

            const operand right_at{
                unrolled_address(src_loc_tk, indent, right, size_bytes),
            };

            compare_parts(indent, registers, size_bytes, starts, left_at,
                          right_at);

            return;
        }

        const known_loop loop{plan_known_loop(size_bytes, starts)};

        comment_known_loop(src_loc_tk, indent, "compare", loop, starts);

        if (loop.head_size_bytes != 0) {
            // a head pointer for a far offset is not needed by the loop
            const address_scope head_scope{*this};

            const operand left_at{
                unrolled_address(src_loc_tk, indent, left,
                                 loop.head_size_bytes),
            };

            const operand right_at{
                unrolled_address(src_loc_tk, indent, right,
                                 loop.head_size_bytes),
            };

            compare_parts(indent, registers, loop.head_size_bytes, starts,
                          left_at, right_at);
        }

        const operand left_pointer{
            load_pointer(src_loc_tk, indent,
                         offset_by(left, loop.head_size_bytes)),
        };

        const operand right_pointer{
            load_pointer(src_loc_tk, indent,
                         offset_by(right, loop.head_size_bytes)),
        };

        const operand end{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        comment(src_loc_tk, indent, "{}", describe_loop("compare", loop.width));

        set_loop_end(src_loc_tk, indent, end, left_pointer,
                     loop.chunk_count * loop.width);

        assembler_.label(indent, chunk_loop.name);

        compare_access(indent, registers, loop.width, memory_at(left_pointer),
                       memory_at(right_pointer));

        advance(indent, left_pointer, loop.width);
        advance(indent, right_pointer, loop.width);

        assembler_.bne(indent, left_pointer.base_register(),
                       end.base_register(), chunk_loop.reference);

        if (loop.tail_size_bytes == 0) {
            return;
        }

        comment(src_loc_tk, indent, "compare {} B tail", loop.tail_size_bytes);

        // the pointers are at a boundary of the loop width
        const access_start boundary{
            .alignment{loop.width},
            .phase{},
        };

        compare_parts(indent, registers, loop.tail_size_bytes,
                      std::span{&boundary, 1}, memory_at(left_pointer),
                      memory_at(right_pointer));
    }

    auto compare_memory(const token& src_loc_tk, const size_t indent,
                        const size_t size_bytes, const operand& left,
                        const operand& right, const equality_request& request)
        -> void {

        // compared sizes are limited by the variables that hold the data
        assert(size_bytes <= std::numeric_limits<uint32_t>::max());

        // unrolled accesses can follow where each address is within its word
        const std::array<access_start, 2> starts{
            start_of(left, request.alignment),
            start_of(right, request.alignment),
        };

        compare_known_size(src_loc_tk, indent, size_bytes,
                           {
                               .left{left},
                               .right{right},
                               .starts{starts},
                           },
                           request);
    }

    auto compare_parts(const size_t indent, const compare_registers& registers,
                       const size_t size_bytes,
                       const std::span<const access_start> starts,
                       const operand& left, const operand& right) -> void {

        for (const access_part& part : aligned_parts(size_bytes, starts)) {
            compare_access(indent, registers, part.width,
                           offset_by(left, part.offset),
                           offset_by(right, part.offset));
        }
    }

    // run-time count: the addresses are pointer registers and 'count' is a
    // byte count, the result is as for 'compare_known_size'
    auto compare_runtime_count(const token& src_loc_tk, const size_t indent,
                               const compared_range& range,
                               const equality_request& request) -> void {

        const operand& left{range.left};
        const operand& right{range.right};
        const operand& count{range.count};
        const operand& result{request.dst};

        // keeps the registers of both addresses from being picked for scratch
        const address_scope operand_scope{*this, right, left};

        // keeps the result registers from being picked for scratch
        const address_scope result_scope{*this, result, operand{}};

        const std::array<operand, 3> in_use{left, right, count};

        const compare_registers registers{
            alloc_compare_registers(src_loc_tk, indent, result, in_use),
        };

        compare_runtime_walk(src_loc_tk, indent, range, request.alignment,
                             registers);

        write_compare_result(src_loc_tk, indent, result, registers,
                             request.inverted);
    }

    // the walk's own registers are freed before the result is written
    auto compare_runtime_walk(const token& src_loc_tk, const size_t indent,
                              const compared_range& range,
                              const size_t alignment,
                              const compare_registers& registers) -> void {

        const operand& left{range.left};
        const operand& right{range.right};
        const operand& count{range.count};
        const std::array<access_start, 2>& starts{range.starts};

        const address_scope walk_scope{*this};

        const operand left_at{memory_at(left)};
        const operand right_at{memory_at(right)};

        const std::function<void(size_t)> access{
            [&](const size_t width) -> void {
                compare_access(indent, registers, width, left_at, right_at);
            },
        };

        walk_runtime_count(
            {
                .src_loc_tk{src_loc_tk},
                .indent{indent},
                .verb{"compare"},
                .src{left},
                .dst{right},
                .count{count},
                .access{access},
            },
            starts, alignment);
    }

    // 'source' in a register of the comparison width
    auto comparison_operand(const token& src_loc_tk, const size_t indent,
                            const operand& source, const operand& other,
                            const type& width_type, const operand& dst)
        -> operand {

        // matching register representations need no conversion
        if (source.is_register() and source.type_ref().is_same(width_type)) {
            return source;
        }

        // zero has the same representation at every supported width
        if (immediate_value(source) == 0) {
            return operand::reg("zero", width_type);
        }

        // the output can hold an input unless doing so destroys the other
        // value or an address still needed to load it
        const size_t output_register{
            register_index(dst.base_register()),
        };

        const bool reuse_destination{
            dst.is_register() and output_register != 0 and
                (not(other.is_register() or other.is_memory()) or
                 output_register != register_index(other.base_register())) and
                (not other.is_memory() or
                 output_register != register_index(other.index_register())),
        };

        // preserve the comparison width rather than narrowing to bool
        const operand value{
            reuse_destination
                ? operand::reg(dst.base_register(), width_type)
                : alloc_scratch_register(src_loc_tk, indent, width_type),
        };

        copy_value(src_loc_tk, indent, value, source);

        return value;
    }

    // both sides in registers of the comparison width, the right one is a zero
    // register when 'use_immediate' compares against an immediate
    [[nodiscard]] auto
    comparison_operands(const token& src_loc_tk, const size_t indent,
                        const operand& lhs, const operand& rhs,
                        const bool use_immediate, const operand& dst)
        -> std::array<operand, 2> {

        if (shares_address_base(lhs, rhs) and
            not register_needs_extension(lhs.type_ref(), rhs)) {

            return comparison_operands_through_shared_base(src_loc_tk, indent,
                                                           lhs, rhs);
        }

        const operand left{
            comparison_operand(src_loc_tk, indent, lhs, rhs, lhs.type_ref(),
                               dst),
        };

        // the immediate is encoded in the instruction, so the register is
        // never read
        const operand right{
            use_immediate ? operand::reg("zero", lhs.type_ref())
                          : comparison_operand(src_loc_tk, indent, rhs, left,
                                               lhs.type_ref(), dst),
        };

        return {left, right};
    }

    // both addresses are 'base + upper + low', so one 'lui' and 'add' serve
    // both loads and the base register ends up holding the left value
    [[nodiscard]] auto comparison_operands_through_shared_base(
        const token& src_loc_tk, const size_t indent, const operand& lhs,
        const operand& rhs) -> std::array<operand, 2> {

        const address_offset_parts lhs_parts{
            split_address_offset(lhs.displacement()),
        };

        const address_offset_parts rhs_parts{
            split_address_offset(rhs.displacement()),
        };

        const operand left{
            alloc_scratch_register(src_loc_tk, indent, lhs.type_ref()),
        };

        const operand right{
            alloc_scratch_register(src_loc_tk, indent, lhs.type_ref()),
        };

        comment(src_loc_tk, indent, "operands share base {}",
                left.base_register());

        assembler_.lui(indent, left.base_register(), lhs_parts.upper);

        assembler_.add(indent, left.base_register(), left.base_register(),
                       lhs.base_register());

        assembler_.load(indent, load_op(rhs.type_ref()), right.base_register(),
                        rhs_parts.low, left.base_register());

        assembler_.load(indent, load_op(lhs.type_ref()), left.base_register(),
                        lhs_parts.low, left.base_register());

        return {left, right};
    }

    // prefer the output register or a temporary the comparison owns
    auto comparison_result_register(const token& src_loc_tk,
                                    const size_t indent, const operand& dst,
                                    const operand& lhs, const operand& rhs,
                                    const operand& left, const operand& right)
        -> operand {

        if (dst.is_register()) {
            return dst;
        }

        // materialized operands are ours to overwrite after the comparison
        if (not left.allocation_register().empty() and not lhs.is_register()) {
            return left;
        }

        if (not right.allocation_register().empty() and not rhs.is_register()) {
            return right;
        }

        // both operands are live inputs or zero
        return alloc_scratch_register(src_loc_tk, indent, default_type());
    }

    auto copy_access(const size_t indent, const operand& value,
                     const size_t width, const operand& from, const operand& to)
        -> void {

        assembler_.load(indent, unsigned_load_op(width), value.base_register(),
                        from.displacement(), from.base_register());

        assembler_.store(indent, store_op(width), value.base_register(),
                         to.displacement(), to.base_register());
    }

    // known size: the addresses are memory operands
    auto copy_known_size(const token& src_loc_tk, const size_t indent,
                         const operand& src, const operand& dst,
                         const size_t size_bytes,
                         const std::array<access_start, 2>& starts) -> void {

        // keeps the registers of both addresses from being picked for scratch
        // and frees the registers allocated below
        const address_scope scope{*this, dst, src};

        const operand value{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        // direct offsets avoid two pointer temporaries for ordinary small
        // copies
        if (size_bytes <= copy_unroll_threshold_bytes_) {
            comment_aligned_parts(src_loc_tk, indent, "copy", size_bytes,
                                  starts);

            const operand from{
                unrolled_address(src_loc_tk, indent, src, size_bytes),
            };

            const operand to{
                unrolled_address(src_loc_tk, indent, dst, size_bytes),
            };

            copy_parts(indent, value, size_bytes, starts, from, to);

            return;
        }

        const known_loop loop{plan_known_loop(size_bytes, starts)};

        comment_known_loop(src_loc_tk, indent, "copy", loop, starts);

        if (loop.head_size_bytes != 0) {
            // a head pointer for a far offset is not needed by the loop
            const address_scope head_scope{*this};

            const operand from{
                unrolled_address(src_loc_tk, indent, src, loop.head_size_bytes),
            };

            const operand to{
                unrolled_address(src_loc_tk, indent, dst, loop.head_size_bytes),
            };

            copy_parts(indent, value, loop.head_size_bytes, starts, from, to);
        }

        const operand src_pointer{
            load_pointer(src_loc_tk, indent,
                         offset_by(src, loop.head_size_bytes)),
        };

        const operand dst_pointer{
            load_pointer(src_loc_tk, indent,
                         offset_by(dst, loop.head_size_bytes)),
        };

        const operand end{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        comment(src_loc_tk, indent, "{}", describe_loop("copy", loop.width));

        set_loop_end(src_loc_tk, indent, end, src_pointer,
                     loop.chunk_count * loop.width);

        assembler_.label(indent, chunk_loop.name);

        copy_access(indent, value, loop.width, memory_at(src_pointer),
                    memory_at(dst_pointer));

        advance(indent, src_pointer, loop.width);
        advance(indent, dst_pointer, loop.width);

        assembler_.bne(indent, src_pointer.base_register(), end.base_register(),
                       chunk_loop.reference);

        if (loop.tail_size_bytes == 0) {
            return;
        }

        comment(src_loc_tk, indent, "copy {} B tail", loop.tail_size_bytes);

        // the pointers are at a boundary of the loop width
        const access_start boundary{
            .alignment{loop.width},
            .phase{},
        };

        copy_parts(indent, value, loop.tail_size_bytes, std::span{&boundary, 1},
                   memory_at(src_pointer), memory_at(dst_pointer));
    }

    auto copy_parts(const size_t indent, const operand& value,
                    const size_t size_bytes,
                    const std::span<const access_start> starts,
                    const operand& from, const operand& to) -> void {

        for (const access_part& part : aligned_parts(size_bytes, starts)) {
            copy_access(indent, value, part.width, offset_by(from, part.offset),
                        offset_by(to, part.offset));
        }
    }

    // run-time count: the addresses are pointer registers and 'count' is a
    // byte count
    auto copy_runtime_count(const token& src_loc_tk, const size_t indent,
                            const operand& src, const operand& dst,
                            const operand& count,
                            const std::array<access_start, 2>& starts,
                            const size_t alignment) -> void {

        // keeps the registers of both addresses from being picked for scratch
        // and frees the registers allocated below
        const address_scope scope{*this, dst, src};

        const operand value{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const operand from{memory_at(src)};
        const operand to{memory_at(dst)};

        const std::function<void(size_t)> access{
            [&](const size_t width) -> void {
                copy_access(indent, value, width, from, to);
            },
        };

        walk_runtime_count(
            {
                .src_loc_tk{src_loc_tk},
                .indent{indent},
                .verb{"copy"},
                .src{src},
                .dst{dst},
                .count{count},
                .access{access},
            },
            starts, alignment);
    }

    // both addresses are 'base + upper + low', so one 'lui' and 'add' serve
    // the load and the store
    auto copy_through_shared_base(const token& src_loc_tk, const size_t indent,
                                  const operand& dst, const operand& src)
        -> void {

        const address_offset_parts src_parts{
            split_address_offset(src.displacement()),
        };

        const address_offset_parts dst_parts{
            split_address_offset(dst.displacement()),
        };

        const operand base{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const operand value{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        comment(src_loc_tk, indent, "source and destination share base {}",
                base.base_register());

        assembler_.lui(indent, base.base_register(), src_parts.upper);

        assembler_.add(indent, base.base_register(), base.base_register(),
                       src.base_register());

        assembler_.load(indent, load_op(src.type_ref()), value.base_register(),
                        src_parts.low, base.base_register());

        assembler_.store(indent, store_op(dst.type_ref().size_bytes()),
                         value.base_register(), dst_parts.low,
                         base.base_register());
    }

    auto emit_arithmetic_helpers() -> void {
        // unused helpers contribute no code
        if (multiply_helper_used_) {
            assembler_.label(0, ".Lbaz_multiply");
            assembler_.mv(1, "a2", "a0");
            assembler_.li(1, "a0", 0);
            assembler_.beqz(1, "a1", "3f");
            assembler_.label(0, "1");
            assembler_.andi(1, "a3", "a1", 1);
            assembler_.beqz(1, "a3", "2f");
            assembler_.add(1, "a0", "a0", "a2");
            assembler_.label(0, "2");
            assembler_.slli(1, "a2", "a2", 1);
            assembler_.srli(1, "a1", "a1", 1);
            assembler_.bnez(1, "a1", "1b");
            assembler_.label(0, "3");
            assembler_.ret(1);
        }

        // divide and remainder share magnitude division and sign restoration
        if (divide_helper_used_) {
            // one quotient bit per step
            constexpr int divide_steps{32};

            assembler_.label(0, ".Lbaz_divide");
            assembler_.beqz(1, "a1", "5f");
            assembler_.srai(1, "a4", "a0", sign_shift_);
            assembler_.srai(1, "a3", "a1", sign_shift_);
            assembler_.xor_op(1, "a0", "a0", "a4");
            assembler_.sub(1, "a0", "a0", "a4");
            assembler_.xor_op(1, "a1", "a1", "a3");
            assembler_.sub(1, "a1", "a1", "a3");
            assembler_.xor_op(1, "a3", "a3", "a4");
            assembler_.li(1, "a2", 0);
            assembler_.li(1, "a5", divide_steps);
            assembler_.label(0, "1");
            assembler_.srli(1, "a6", "a0", sign_shift_);
            assembler_.slli(1, "a2", "a2", 1);
            assembler_.or_op(1, "a2", "a2", "a6");
            assembler_.slli(1, "a0", "a0", 1);
            assembler_.bltu(1, "a2", "a1", "2f");
            assembler_.sub(1, "a2", "a2", "a1");
            assembler_.ori(1, "a0", "a0", 1);
            assembler_.label(0, "2");
            assembler_.addi(1, "a5", "a5", -1);
            assembler_.bnez(1, "a5", "1b");
            assembler_.xor_op(1, "a0", "a0", "a3");
            assembler_.sub(1, "a0", "a0", "a3");
            assembler_.xor_op(1, "a1", "a2", "a4");
            assembler_.sub(1, "a1", "a1", "a4");
            assembler_.ret(1);
            assembler_.label(0, "5");

            assembler_.ebreak(1);
            assembler_.j(1, "5b");
        }
    }

    // applies 'instruction' to 'left' with an immediate when it fits
    auto emit_binary_instruction(const token& src_loc_tk, const size_t indent,
                                 const op instruction, const operand& src,
                                 const operand& left,
                                 const std::optional<int32_t> constant)
        -> void {

        const bool arithmetic{instruction == op::add or instruction == op::sub};

        const int64_t immediate{
            instruction == op::sub ? -int64_t{constant.value_or(0)}
                                   : int64_t{constant.value_or(0)},
        };

        if (constant.has_value() and immediate >= immediate_min and
            immediate <= immediate_max) {

            assembler_.immediate_op(indent, immediate_form(instruction),
                                    left.base_register(), left.base_register(),
                                    immediate);

            return;
        }

        // two addi are shorter than li and add
        if (constant.has_value() and arithmetic and
            immediate >= 2 * immediate_min and immediate <= 2 * immediate_max) {

            const int64_t first{immediate < 0 ? immediate_min : immediate_max};

            assembler_.addi(indent, left.base_register(), left.base_register(),
                            first);

            assembler_.addi(indent, left.base_register(), left.base_register(),
                            immediate - first);

            return;
        }

        const operand right{
            source_register(src_loc_tk, indent, src, constant),
        };

        assembler_.register_op(indent, instruction, left.base_register(),
                               left.base_register(), right.base_register());
    }

    // sets 'result' to 1 when the comparison holds and to 0 otherwise
    auto emit_boolean_result(const size_t indent,
                             const comparison_operator operation,
                             const std::string_view result, const operand& left,
                             const operand& right,
                             const std::optional<int64_t> immediate,
                             const bool inverted) -> void {

        if (operation == comparison_operator::equal or
            operation == comparison_operator::not_equal) {

            emit_equality_result(
                indent, result, left, right, immediate,
                inverted != (operation == comparison_operator::not_equal));

            return;
        }

        emit_less_than(indent, operation, result, left, right, immediate);

        // 'slt' answers '<', against a threshold '>' and '>=' are its
        // complement, with swapped registers '>=' and '<=' are
        const bool complement{
            immediate.has_value()
                ? (operation == comparison_operator::greater_equal or
                   operation == comparison_operator::greater)
                : (operation == comparison_operator::greater_equal or
                   operation == comparison_operator::less_equal),
        };

        if (inverted != complement) {
            assembler_.xori(indent, result, result, 1);
        }
    }

    // the address scope frees its temporaries before the end comment
    auto emit_bounds_check(const token& src_loc_tk, const size_t indent,
                           const operand& reg_to_check,
                           const size_t array_count, const bool allow_end,
                           const operand& reg_count,
                           const bounds_check_options& options) -> void {

        const address_scope scope{*this, reg_to_check, reg_count};

        const std::string_view index{reg_to_check.base_register()};

        const bounds_plan plan{
            plan_bounds_check(options, reg_count, array_count,
                              std::numeric_limits<int32_t>::max(),
                              registers_.is_lower_checked(
                                  register_mask(reg_count.base_register()))),
        };

        if (options.lower) {
            emit_lower_bound(src_loc_tk, indent, reg_to_check, reg_count, plan,
                             not options.upper);
        }

        if (options.upper) {
            comment(src_loc_tk, indent, "upper bound");

            check_upper_bound(src_loc_tk, indent, index, array_count, allow_end,
                              reg_count, options.lower);
        }

        assembler_.label(indent, bounds_fail.name);

        if (options.with_line) {
            comment(src_loc_tk, indent, "source line");
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }

        branch(indent, bounds_failure_handler_label);
        assembler_.label(indent, bounds_pass.name);
    }

    // the address scopes end on return, before the caller frees its registers
    auto emit_comparison(const token& src_loc_tk, const size_t indent,
                         const operand& lhs, const operand& rhs,
                         const comparison_action& action) -> void {

        const address_scope destination_scope{*this, action.destination,
                                              operand{}};

        const address_scope scope{*this, lhs, rhs};

        validate_scalar(src_loc_tk, lhs.type_ref());
        validate_scalar(src_loc_tk, rhs.type_ref());

        const comparison_operator operation{action.operation};

        const std::optional<int32_t> constant{
            narrowed_immediate(rhs, lhs.type_ref()),
        };

        // x > c and x <= c use the signed threshold c + 1
        const bool inclusive_threshold{swaps_operands(operation)};

        const int64_t threshold{
            int64_t{constant.value_or(0)} + (inclusive_threshold ? 1 : 0),
        };

        // a branch compares registers, a boolean result can use an immediate
        const bool use_immediate{
            not action.destination.is_empty() and constant.has_value() and
                threshold >= immediate_min and threshold <= immediate_max,
        };

        const std::optional<int64_t> immediate{
            use_immediate ? std::optional<int64_t>{threshold} : std::nullopt,
        };

        const std::array<operand, 2> sides{
            comparison_operands(src_loc_tk, indent, lhs, rhs, use_immediate,
                                action.destination),
        };

        const operand& left{sides.at(0)};
        const operand& right{sides.at(1)};

        // branch-only comparisons do not need a materialized boolean
        if (action.destination.is_empty()) {
            emit_comparison_branch(indent, operation, left, right, action);
            return;
        }

        const operand value{
            comparison_result_register(src_loc_tk, indent, action.destination,
                                       lhs, rhs, left, right),
        };

        const std::string_view result{value.base_register()};

        emit_boolean_result(indent, operation, result, left, right, immediate,
                            action.inverted);

        // memory results require a store, register results are in place
        if (action.destination.is_memory()) {
            copy_value(src_loc_tk, indent, action.destination, value);
        }

        // some callers request both a stored boolean and a branch
        if (not action.target.empty()) {
            emit_jump(indent, action.branch_on_true ? op::bne : op::beq, result,
                      "zero", action.target);
        }
    }

    auto emit_comparison_branch(const size_t indent,
                                const comparison_operator operation,
                                const operand& left, const operand& right,
                                const comparison_action& action) -> void {

        // a comparison without a boolean result is a branch
        assert(not action.target.empty());

        const bool inverted{action.inverted != not action.branch_on_true};

        // equality and inequality share one branch pair
        if (operation == comparison_operator::equal or
            operation == comparison_operator::not_equal) {

            const bool branch_on_unequal{
                inverted != (operation == comparison_operator::not_equal),
            };

            emit_jump(indent, branch_on_unequal ? op::bne : op::beq,
                      left.base_register(), right.base_register(),
                      action.target);

            return;
        }

        // ordered comparisons use signed blt/bge, swapping for > and <=
        const bool swapped{swaps_operands(operation)};

        const bool branch_on_greater_equal{
            inverted != (operation == comparison_operator::greater_equal or
                         operation == comparison_operator::less_equal),
        };

        emit_jump(indent, branch_on_greater_equal ? op::bge : op::blt,
                  swapped ? right.base_register() : left.base_register(),
                  swapped ? left.base_register() : right.base_register(),
                  action.target);
    }

    auto emit_equality_result(const size_t indent,
                              const std::string_view result,
                              const operand& left, const operand& right,
                              const std::optional<int64_t> immediate,
                              const bool unequal) -> void {

        const std::string_view tested{
            equality_tested_register(indent, result, left, right, immediate),
        };

        // inequality is a nonzero test, not a second boolean inversion
        if (unequal) {
            assembler_.sltu(indent, result, "zero", tested);
            return;
        }

        assembler_.sltiu(indent, result, tested, 1);
    }

    // 'j' or a conditional branch comparing 'first' and 'second'
    auto emit_jump(const size_t indent, const op code,
                   const std::string_view first, const std::string_view second,
                   const std::string_view target) -> void {

        if (code == op::j) {
            assembler_.resolved_jump(indent, target, far_jump_register());
            return;
        }

        assembler_.resolved_branch(indent, code, first, second, target,
                                   far_jump_register());
    }

    auto emit_less_than(const size_t indent,
                        const comparison_operator operation,
                        const std::string_view result, const operand& left,
                        const operand& right,
                        const std::optional<int64_t> immediate) -> void {

        // encodable thresholds avoid materializing a constant register
        if (immediate.has_value()) {
            assembler_.slti(indent, result, left.base_register(), *immediate);
            return;
        }

        // 'x > y' is 'y < x' and 'x <= y' is the complement of 'y < x'
        const bool swapped{swaps_operands(operation)};

        assembler_.slt(indent, result,
                       swapped ? right.base_register() : left.base_register(),
                       swapped ? left.base_register() : right.base_register());
    }

    // prints 'message' and the line number in a0, then exits, the digits are
    // emitted once and shared by the handlers
    auto emit_line_failure(const std::string_view message_label,
                           const std::string_view message) -> void {

        // room for the ten digits of a 32-bit line number and a newline
        constexpr int64_t digits_bytes{16};
        constexpr int digit_zero{'0'};
        constexpr int newline{'\n'};

        assembler_.mv(1, "s2", "a0");
        assembler_.li(1, "a0", stderr_descriptor);
        assembler_.la(1, "a1", message_label);
        assembler_.li(1, "a2", message.size());
        emit_write_call(1);

        const bool is_first{not line_report_emitted_};
        line_report_emitted_ = true;

        if (not is_first) {
            assembler_.j(1, ".Lbaz_report_line");
        } else {
            // writes the line number of s2 in decimal: each power of ten of
            // '.Lbaz_decimal_places' gives a digit by repeated subtraction,
            // leading zeros are skipped and the last place always writes its
            // digit
            assembler_.label(0, ".Lbaz_report_line");
            assembler_.addi(1, "sp", "sp", -digits_bytes);
            assembler_.mv(1, "a1", "sp");
            assembler_.li(1, "a2", 0);
            assembler_.la(1, "t0", ".Lbaz_decimal_places");
            assembler_.label(0, "1");
            assembler_.lw(1, "t1", 0, "t0");
            assembler_.li(1, "t2", 0);
            assembler_.label(0, "2");
            assembler_.bltu(1, "s2", "t1", "3f");
            assembler_.sub(1, "s2", "s2", "t1");
            assembler_.addi(1, "t2", "t2", 1);
            assembler_.j(1, "2b");
            assembler_.label(0, "3");
            assembler_.or_op(1, "t3", "a2", "t2");
            assembler_.bnez(1, "t3", "4f");
            assembler_.li(1, "t3", 1);
            assembler_.bne(1, "t1", "t3", "5f");
            assembler_.label(0, "4");
            assembler_.addi(1, "t2", "t2", digit_zero);
            assembler_.sb(1, "t2", 0, "a1");
            assembler_.addi(1, "a1", "a1", 1);
            assembler_.addi(1, "a2", "a2", 1);
            assembler_.label(0, "5");
            assembler_.addi(1, "t0", "t0", word_size_bytes_);
            assembler_.li(1, "t3", 1);
            assembler_.bne(1, "t1", "t3", "1b");
            assembler_.li(1, "t2", newline);
            assembler_.sb(1, "t2", 0, "a1");
            assembler_.addi(1, "a2", "a2", 1);
            assembler_.mv(1, "a1", "sp");
            assembler_.li(1, "a0", stderr_descriptor);
            emit_write_call(1);

            exit(token{}, 1,
                 operand::imm(std::format("{}", panic_exit_code),
                              default_type()));

            constexpr std::array<int64_t, 10> decimal_places{
                1000000000, 100000000, 10000000, 1000000, 100000,
                10000,      1000,      100,      10,      1,
            };

            assembler_.switch_section(section::rodata);
            assembler_.align(4);
            assembler_.label(0, ".Lbaz_decimal_places");
            assembler_.data(4, decimal_places);
            assembler_.switch_section(section::text);
        }

        assembler_.switch_section(section::rodata);
        assembler_.label(0, message_label);
        assembler_.ascii(message);
        assembler_.switch_section(section::text);
    }

    // the lower bound, the last check of the bounds check branches past the
    // handler, see 'check_lower_bounds'
    auto emit_lower_bound(const token& src_loc_tk, const size_t indent,
                          const operand& reg_to_check, const operand& reg_count,
                          const bounds_plan& plan, const bool is_last) -> void {

        comment_lower_bound(src_loc_tk, indent, reg_to_check, reg_count, plan);

        if (not plan.upper_covers_lower) {
            check_lower_bounds(indent, reg_to_check.base_register(),
                               plan.count_known ? operand{} : reg_count,
                               is_last);
        }

        registers_.mark_lower_checked(
            register_mask(lower_checked_register(reg_to_check, reg_count)));
    }

    // the result replaces the value in 'loaded', a partial sum goes through a
    // scratch register
    //   x * 10  =>  t = x << 2; x = t + x; x = x << 1
    auto emit_shift_add_sequence(const token& src_loc_tk, const size_t indent,
                                 const loaded_destination& loaded,
                                 const digit_sequence& sequence) -> void {

        // a single digit needs only the final shift
        const operand partial{
            sequence.lowest == sequence.top
                ? operand{}
                : alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        std::string_view shifted{loaded.value.base_register()};
        int pending_shift{};

        for (size_t bit{sequence.top}; bit != sequence.lowest;) {
            --bit;
            ++pending_shift;

            if (sequence.digits.at(bit) == 0) {
                continue;
            }

            assembler_.slli(indent, partial.base_register(), shifted,
                            pending_shift);

            const std::string_view sum{
                bit == sequence.lowest ? loaded.value.base_register()
                                       : partial.base_register(),
            };

            assembler_.register_op(
                indent, sequence.digits.at(bit) < 0 ? op::sub : op::add, sum,
                partial.base_register(), loaded.value.base_register());

            shifted = partial.base_register();
            pending_shift = 0;
        }

        // the zero bits below the lowest digit
        if (sequence.lowest != 0) {
            assembler_.slli(indent, loaded.value.base_register(),
                            loaded.value.base_register(), sequence.lowest);
        }

        if (sequence.negate) {
            assembler_.sub(indent, loaded.value.base_register(), "zero",
                           loaded.value.base_register());
        }
    }

    // a register that is zero exactly when the operands are equal
    auto equality_tested_register(const size_t indent,
                                  const std::string_view result,
                                  const operand& left, const operand& right,
                                  const std::optional<int64_t> immediate)
        -> std::string_view {

        // an immediate zero can be tested without transforming the input
        if (immediate.has_value() and *immediate == 0) {
            return left.base_register();
        }

        // nonzero small constants fit directly in xori
        if (immediate.has_value()) {
            assembler_.xori(indent, result, left.base_register(), *immediate);
            return result;
        }

        // an operand that is not tested directly is in a register other than
        // 'zero'
        assert(register_index(left.base_register()) != 0 and
               register_index(right.base_register()) != 0);

        // neither operand is zero and the constant did not fit
        assembler_.xor_op(indent, result, left.base_register(),
                          right.base_register());

        return result;
    }

    // a jump grown beyond 1 MiB needs a register without a live value
    [[nodiscard]] auto far_jump_register() const -> std::string_view {
        for (const size_t index : scratch_registers_) {
            if (not registers_.is_unavailable(uint32_t{1} << index)) {
                return register_names_.at(index);
            }
        }

        return {};
    }

    [[nodiscard]] auto is_register_allocated(const std::string_view name) const
        -> bool {

        return registers_.is_unavailable(register_mask(name));
    }

    // variables and frames start word aligned so a direct offset from their
    // base has a known position within a word
    [[nodiscard]] auto is_word_based(const operand& address) const -> bool {
        // an index register holds a value unknown at compile time
        if (not address.is_memory() or not address.index_register().empty()) {
            return false;
        }

        const size_t base{register_index(address.base_register())};

        const bool is_variables_base{
            variables_base_reserved_ and base == s0_register_index,
        };

        const bool is_frame_base{
            frame_base_reserved_ and
                base == register_index(frame_base_register()),
        };

        // other bases such as loaded pointers or bulk registers may hold any
        // address
        return is_variables_base or is_frame_base;
    }

    [[nodiscard]] auto load_destination(const token& src_loc_tk,
                                        const size_t indent, const operand& dst)
        -> loaded_destination {

        if (dst.is_register()) {
            return {
                .address{},
                .value{dst},
            };
        }

        // lowering before allocating keeps the scratch register order
        const operand address{lower_address(src_loc_tk, indent, dst)};

        const operand value{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        copy_value(src_loc_tk, indent, value, address);

        return {
            .address{address},
            .value{value},
        };
    }

    // writes 'first_value' to a0 and 'source' to a1
    auto load_helper_arguments(const token& src_loc_tk, const size_t indent,
                               const operand& first_value,
                               const operand& source) -> void {

        const operand first_argument{operand::reg("a0", default_type())};
        const operand second_argument{operand::reg("a1", default_type())};

        // argument registers are allocated last, so no operand lives in a0
        assert(not uses_register(source, "a0"));

        copy_value(src_loc_tk, indent, first_argument, first_value);
        copy_value(src_loc_tk, indent, second_argument, source);
    }

    auto load_into(const token& src_loc_tk, const size_t indent,
                   const operand& value, const operand& src) -> void {

        if (src.is_memory()) {
            // a load may build its address in the register it will overwrite
            const operand lowered{
                lower_address(src_loc_tk, indent, src, value),
            };

            assembler_.load(indent, load_op(src.type_ref()),
                            value.base_register(), lowered.displacement(),
                            lowered.base_register());

            return;
        }

        if (src.is_register() and register_index(value.base_register()) ==
                                      register_index(src.base_register())) {

            return;
        }

        // adding zero copies a register without changing its bits
        if (src.is_register()) {
            assembler_.addi(indent, value.base_register(), src.base_register(),
                            0);

            return;
        }

        // constants are stored by the caller
        std::unreachable();
    }

    // a register holding 'address'
    [[nodiscard]] auto load_pointer(const token& src_loc_tk,
                                    const size_t indent, const operand& address)
        -> operand {

        const operand pointer{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        address_of(src_loc_tk, indent, pointer, address);

        return pointer;
    }

    // lower base + index * scale + displacement to register + signed 12-bit
    // offset
    [[nodiscard]] auto lower_address(const token& src_loc_tk,
                                     const size_t indent,
                                     const operand& address,
                                     const operand& dst = {}) -> operand {

        validate_address(src_loc_tk, address);

        // no index: encode the offset directly or materialize base + offset
        if (address.index_register().empty()) {
            return lower_address_without_index(src_loc_tk, indent, address,
                                               dst);
        }

        // indexed memory operand: [base + index * scale + displacement];
        // combine the register terms before applying the displacement
        const operand result{
            address_result_register(src_loc_tk, indent, address, dst),
        };

        // unit scale: combine the base and index without multiplication
        if (address.scale() == 1) {
            return lower_address_unscaled_index(src_loc_tk, indent, address,
                                                result);
        }

        // scaled index: form the product before adding the base and offset
        return lower_address_scaled_index(src_loc_tk, indent, address, result);
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
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.lui(indent, displacement.base_register(), parts.upper);

        assembler_.add(indent, result_name, result_name,
                       displacement.base_register());

        free_scratch_register(src_loc_tk, indent, displacement);

        return operand::mem(result_name, {}, 1, parts.low, address.type_ref());
    }

    [[nodiscard]] auto
    lower_address_scaled_index(const token& src_loc_tk, const size_t indent,
                               const operand& address, const operand& result)
        -> operand {

        const std::string& base{address.base_register()};
        const std::string& result_name{result.base_register()};

        assembler_.slli(indent, result_name, address.index_register(),
                        std::countr_zero(address.scale()));

        // the preserved base can now be added to the scaled index
        assembler_.add(indent, result_name, result_name, base);

        return lower_address_offset(src_loc_tk, indent, address, result);
    }

    [[nodiscard]] auto
    lower_address_unscaled_index(const token& src_loc_tk, const size_t indent,
                                 const operand& address, const operand& result)
        -> operand {

        const std::string& base{address.base_register()};
        const std::string& index{address.index_register()};
        const std::string& result_name{result.base_register()};

        // one add combines register inputs, even when result aliases either
        assembler_.add(indent, result_name, base, index);

        return lower_address_offset(src_loc_tk, indent, address, result);
    }

    [[nodiscard]] auto
    lower_address_without_index(const token& src_loc_tk, const size_t indent,
                                const operand& address, const operand& dst)
        -> operand {

        const std::string& base{address.base_register()};
        const int64_t offset{address.displacement()};

        const address_offset_parts parts{split_address_offset(offset)};

        // no upper part: the memory instruction handles the entire offset
        if (parts.upper == 0) {
            return operand::mem(base, {}, 1, parts.low, address.type_ref());
        }

        const operand result{
            address_result_register(src_loc_tk, indent, address, dst),
        };

        const std::string& result_name{result.base_register()};

        // only the upper part; the memory instruction adds the low

        assembler_.lui(indent, result_name, parts.upper);

        assembler_.add(indent, result_name, result_name, base);

        return operand::mem(result_name, {}, 1, parts.low, address.type_ref());
    }

    // what an access through the register 'pointer' reads or writes
    [[nodiscard]] auto memory_at(const operand& pointer) const -> operand {
        return operand::mem(pointer.base_register(), {}, 1, 0, default_type());
    }

    // the factor is a known constant, keep only the bits that fit in the
    // product's type before choosing how to multiply
    auto multiply_by_constant(const token& src_loc_tk, const size_t indent,
                              const operand& product, const operand& factor,
                              const int32_t constant) -> void {

        const size_t bits{product.type_ref().size_bits()};

        const constant_factor factor_info{
            classify_factor(static_cast<uint32_t>(constant), bits),
        };

        if (factor_info.kind == factor_kind::zero) {
            store_constant_result(src_loc_tk, indent, product, 0);
            return;
        }

        if (factor_info.kind == factor_kind::one) {
            return;
        }

        if (factor_info.kind == factor_kind::minus_one) {
            unary(src_loc_tk, indent, arithmetic_operator::negate, product);
            return;
        }

        if (factor_info.kind == factor_kind::power_of_two) {
            shift(src_loc_tk, indent, arithmetic_operator::shift_left, product,
                  operand::imm(std::format("{}", factor_info.shift),
                               default_type()));

            return;
        }

        multiply_by_shifts_and_adds(
            src_loc_tk, indent, product, factor,
            static_cast<uint32_t>(factor_info.multiplier), bits);
    }

    // the remaining constant needs shifts and adds; keep the original value
    // for the additions while the result changes
    auto
    multiply_by_shifts_and_adds(const token& src_loc_tk, const size_t indent,
                                const operand& product, const operand& factor,
                                const uint32_t multiplier, const size_t bits)
        -> void {

        const address_scope scope{*this, product, factor};

        // the original value stays here until the last add or sub, which
        // writes the result in its place so no copy is needed
        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, product),
        };

        const digit_sequence sequence{make_digit_sequence(multiplier, bits)};

        emit_shift_add_sequence(src_loc_tk, indent, loaded, sequence);

        store_operation_result(indent, product, loaded.address, loaded.value,
                               true);
    }

    // a register holding the result after the saved registers are restored
    auto preserved_helper_result(const token& src_loc_tk, const size_t indent,
                                 const operand& result,
                                 const std::span<const std::string_view> saved)
        -> operand {

        const bool restored{
            std::ranges::find(saved, result.base_register()) != saved.end(),
        };

        if (not restored) {
            return result;
        }

        // protect a result register being restored
        const operand kept{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        copy_value(src_loc_tk, indent, kept, result);

        return kept;
    }

    // named and scratch allocations share one lifo stack
    auto record_allocation(const token& src_loc_tk, const size_t indent,
                           const size_t index, const type& type_ref,
                           const bool named) -> void {

        registers_.push({
            .index{index},
            .src_loc_tk{src_loc_tk},
            .indent{indent},
            .type_ptr{&type_ref},
            .named{named},
            .frame{current_call_frame()},
        });

        record_register_use(registers_);
    }

    // the pointers advance together so both starts decide the loop width
    auto record_bulk_address_start(const operand& address) -> void {
        bulk_addresses& addresses{bulk_addresses_.back()};
        addresses.starts.at(addresses.count) = start_of(address, 1);
        ++addresses.count;
    }

    auto release_bulk(const token& src_loc_tk, const size_t indent) -> void {
        free_scratch_registers(src_loc_tk, indent, bulk_registers_.back());

        bulk_registers_.pop_back();
        bulk_addresses_.pop_back();
    }

    auto reserve_io_call_register(const token& src_loc_tk, const size_t indent,
                                  const operand& dst, const operand& descriptor,
                                  const operand& address, const operand& count)
        -> operand {

        for (const operand* value : {&dst, &descriptor, &address, &count}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == word_size_bytes_);
        }

        assert(register_index(dst.base_register()) == register_index("a0"));

        assert(register_index(descriptor.base_register()) ==
               register_index("a0"));

        assert(register_index(address.base_register()) == register_index("a1"));
        assert(register_index(count.base_register()) == register_index("a2"));

        // selects the system call or receives the return address
        return alloc_named_register(src_loc_tk, indent, "a7", default_type());
    }

    auto restore_saved_registers(const size_t indent,
                                 const std::span<const std::string_view> saved,
                                 const size_t stack_bytes) -> void {

        for (const auto [index, name] : std::views::enumerate(saved)) {
            assembler_.lw(indent, name,
                          static_cast<size_t>(index) * word_size_bytes_, "sp");
        }

        // restore sp before writing a possibly stack-relative destination
        if (stack_bytes != 0) {
            assembler_.addi(indent, "sp", "sp", stack_bytes);
        }
    }

    // an aligned stack area is allocated only when a register is saved, the
    // returned size lets 'restore_saved_registers' free it
    auto save_registers(const size_t indent,
                        const std::span<const std::string_view> saved)
        -> size_t {

        const size_t stack_bytes{
            align_storage_size(saved.size() * word_size_bytes_,
                               stack_alignment_),
        };

        if (stack_bytes != 0) {
            assembler_.addi(indent, "sp", "sp",
                            -static_cast<int64_t>(stack_bytes));
        }

        for (const auto [index, name] : std::views::enumerate(saved)) {
            assembler_.sw(indent, name,
                          static_cast<size_t>(index) * word_size_bytes_, "sp");
        }

        return stack_bytes;
    }

    // the pointer of 'slot', 0 the first address and 1 the second, holds the
    // address, its start decides the width of the loop
    auto set_bulk_address(const token& src_loc_tk, const size_t indent,
                          const size_t slot, const operand& address) -> void {

        record_bulk_address_start(address);

        address_of(src_loc_tk, indent, bulk_registers_.back().at(slot),
                   address);
    }

    // the loop ends when its pointer reaches 'end', replacing a counter that
    // costs a decrement in every iteration, 'address_of' avoids 'li' whose
    // expansion for some values differs from the assembler's
    auto set_loop_end(const token& src_loc_tk, const size_t indent,
                      const operand& end, const operand& pointer,
                      const size_t size_bytes) -> void {

        address_of(src_loc_tk, indent, end,
                   operand::mem(pointer.base_register(), {}, 1,
                                static_cast<int64_t>(size_bytes),
                                default_type()));
    }

    // a narrow register shifts to the top and back, extending in one pair
    auto shift_by_constant(const size_t indent,
                           const arithmetic_operator operation,
                           const operand& dst, const loaded_destination& loaded,
                           const uint32_t shift_count, const size_t bits)
        -> void {

        if (operation == arithmetic_operator::shift_left and
            bits < register_bits_ and dst.is_register()) {

            assembler_.slli(indent, loaded.value.base_register(),
                            loaded.value.base_register(),
                            register_bits_ - bits + shift_count);

            assembler_.immediate_op(indent, extend_shift_op(dst.type_ref()),
                                    loaded.value.base_register(),
                                    loaded.value.base_register(),
                                    register_bits_ - bits);

            store_operation_result(indent, dst, loaded.address, loaded.value,
                                   false);

            return;
        }

        // known counts are below the width of the value
        const bool is_left{operation == arithmetic_operator::shift_left};

        assembler_.immediate_op(indent, is_left ? op::slli : op::srai,
                                loaded.value.base_register(),
                                loaded.value.base_register(), shift_count);

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               is_left);
    }

    auto shift_by_register(const token& src_loc_tk, const size_t indent,
                           const arithmetic_operator operation,
                           const operand& dst, const operand& count,
                           const loaded_destination& loaded) -> void {

        const operand amount{
            source_register(src_loc_tk, indent, count, std::nullopt),
        };

        const bool is_left{operation == arithmetic_operator::shift_left};

        assembler_.register_op(
            indent, is_left ? op::sll : op::sra, loaded.value.base_register(),
            loaded.value.base_register(), amount.base_register());

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               is_left);
    }

    auto source_register(const token& src_loc_tk, const size_t indent,
                         const operand& src,
                         const std::optional<int32_t> constant) -> operand {

        if (src.is_register()) {
            return src;
        }

        const operand right{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        if (constant.has_value()) {
            assembler_.li(indent, right.base_register(), *constant);
            return right;
        }

        copy_value(src_loc_tk, indent, right, src);

        return right;
    }

    [[nodiscard]] auto start_of(const operand& address,
                                const size_t alignment) const -> access_start {

        // without a known base only the type alignment holds, from offset 0
        if (not is_word_based(address)) {
            return {
                .alignment{bulk_width(alignment)},
                .phase{},
            };
        }

        // the low bits of a negative displacement give the same phase
        return {
            .alignment{word_size_bytes_},
            .phase{
                static_cast<size_t>(address.displacement()) % word_size_bytes_,
            },
        };
    }

    auto store_byte_parts(const token& src_loc_tk, const size_t indent,
                          const std::span<const byte_part> parts,
                          const operand& dst, const size_t size_bytes) -> void {

        const operand address{
            unrolled_address(src_loc_tk, indent, dst, size_bytes),
        };

        const bool has_load{
            std::ranges::any_of(
                parts, [](const byte_part& p) -> bool { return p.needs_load; }),
        };

        const operand value{
            has_load
                ? alloc_scratch_register(src_loc_tk, indent, default_type())
                : operand{},
        };

        for (const byte_part& p : parts) {
            const int64_t displacement{
                address.displacement() + static_cast<int64_t>(p.offset),
            };

            if (p.value == 0) {
                assembler_.store(indent, store_op(p.size_bytes), "zero",
                                 displacement, address.base_register());

                continue;
            }

            if (p.needs_load) {
                assembler_.li(indent, value.base_register(), p.value);
            }

            assembler_.store(indent, store_op(p.size_bytes),
                             value.base_register(), displacement,
                             address.base_register());
        }
    }

    auto store_constant_result(const token& src_loc_tk, const size_t indent,
                               const operand& dst, const int32_t constant)
        -> void {

        const address_scope scope{*this, dst, operand{}};

        const operand address{
            dst.is_memory() ? lower_address(src_loc_tk, indent, dst)
                            : operand{},
        };

        if (dst.is_memory() and constant == 0) {
            store_operation_result(indent, dst, address,
                                   operand::reg("zero", default_type()), false);

            return;
        }

        const operand value{working_register(src_loc_tk, indent, dst)};

        assembler_.li(indent, value.base_register(), constant);
        store_operation_result(indent, dst, address, value, false);
    }

    auto store_operation_result(const size_t indent, const operand& dst,
                                const operand& address, const operand& value,
                                const bool normalize) -> void {

        const size_t width{dst.type_ref().size_bytes()};

        if (dst.is_memory()) {
            assembler_.store(indent, store_op(width), value.base_register(),
                             address.displacement(), address.base_register());
        } else if (width < 4 and normalize) {
            const size_t shift{register_bits_ - (width * bits_per_byte)};

            assembler_.slli(indent, value.base_register(),
                            value.base_register(), shift);

            assembler_.immediate_op(indent, extend_shift_op(dst.type_ref()),
                                    value.base_register(),
                                    value.base_register(), shift);
        }
    }

    // keeps an unrolled access of 'size_bytes' within the load/store offset
    // range
    [[nodiscard]] auto unrolled_address(const token& src_loc_tk,
                                        const size_t indent,
                                        const operand& address,
                                        const size_t size_bytes) -> operand {

        operand lowered{lower_address(src_loc_tk, indent, address)};

        if (lowered.displacement() + static_cast<int64_t>(size_bytes) - 1 >
            immediate_max) {
            // note: -1 because the last byte accessed is at 'size_bytes - 1'

            const operand pointer{
                alloc_scratch_register(src_loc_tk, indent, default_type()),
            };

            address_of(src_loc_tk, indent, pointer, lowered);

            lowered = operand::mem(pointer.base_register(), {}, 1, 0,
                                   address.type_ref());
        }

        // one return keeps the copy elided
        return lowered;
    }

    // the value compared with the limit, the index or the end 'index + count'
    [[nodiscard]] auto
    upper_bound_top(const token& src_loc_tk, const size_t indent,
                    const std::string_view index, const operand& reg_count,
                    const operand& limit, const bool lower_checked)
        -> std::string {

        // a negative index is left to the lower check, which has already
        // failed it when enabled
        if (reg_count.is_empty() and not lower_checked) {
            assembler_.bltz(indent, index, bounds_pass.reference);
            return std::string{index};
        }

        if (reg_count.is_empty()) {
            return std::string{index};
        }

        const operand sum{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const std::string& top{sum.base_register()};

        // after the lower checks both are below 2^31, so the sum cannot wrap
        if (lower_checked) {
            assembler_.add(indent, top, index, reg_count.base_register());
            return top;
        }

        // the sign and carry bits form the high word of the widened sum, which
        // decides ends outside the 32-bit range
        const operand high{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.srai(indent, high.base_register(), index, sign_shift_);

        assembler_.srai(indent, limit.base_register(),
                        reg_count.base_register(), sign_shift_);

        assembler_.add(indent, high.base_register(), high.base_register(),
                       limit.base_register());

        assembler_.add(indent, top, index, reg_count.base_register());
        assembler_.sltu(indent, limit.base_register(), top, index);

        assembler_.add(indent, high.base_register(), high.base_register(),
                       limit.base_register());

        assembler_.bltz(indent, high.base_register(), bounds_pass.reference);
        assembler_.bgtz(indent, high.base_register(), bounds_fail.reference);

        return top;
    }

    auto walk_runtime_bytes(const runtime_walk& walk) -> void {
        const std::string_view count{walk.count.base_register()};

        comment(walk.src_loc_tk, walk.indent, "{}; skip if none",
                describe_loop(walk.verb, 1));

        assembler_.beqz(walk.indent, count, walk_end.reference);

        // 'count' becomes the end of the walk, which saves a decrement in
        // every iteration
        assembler_.add(walk.indent, count, count, walk.src.base_register());

        assembler_.label(walk.indent, chunk_loop.name);
        walk_runtime_step(walk, 1);

        assembler_.bne(walk.indent, walk.src.base_register(), count,
                       chunk_loop.reference);

        assembler_.label(walk.indent, walk_end.name);
    }

    // the loop takes 'count' rounded down to whole chunks and ends when 'src'
    // reaches their end, 'count' keeps the tail bytes
    auto walk_runtime_chunks(const runtime_walk& walk, const operand& chunks,
                             const size_t width) -> void {

        const std::string_view count{walk.count.base_register()};

        comment(walk.src_loc_tk, walk.indent,
                "split bytes into chunks and tail; skip loop if none");

        assembler_.andi(walk.indent, chunks.base_register(), count,
                        -static_cast<int64_t>(width));

        assembler_.andi(walk.indent, count, count, width - 1);
        // note: -1 turns the power of two 'width' into a mask of the tail bytes

        assembler_.beqz(walk.indent, chunks.base_register(),
                        after_chunks.reference);

        assembler_.add(walk.indent, chunks.base_register(),
                       chunks.base_register(), walk.src.base_register());

        comment(walk.src_loc_tk, walk.indent, "{}",
                describe_loop(walk.verb, width));

        assembler_.label(walk.indent, chunk_loop.name);
        walk_runtime_step(walk, width);

        assembler_.bne(walk.indent, walk.src.base_register(),
                       chunks.base_register(), chunk_loop.reference);

        assembler_.label(walk.indent, after_chunks.name);
    }

    // a head up to the first aligned chunk, whole chunks, then a tail
    auto walk_runtime_count(const runtime_walk& walk,
                            const std::array<access_start, 2>& starts,
                            const size_t alignment) -> void {

        // the run-time count may be smaller than any head
        const std::array<access_start, 2> typed{
            typed_starts(starts, alignment),
        };

        const loop_start start{
            plan_loop_start(typed, std::numeric_limits<size_t>::max()),
        };

        comment_pointer_loop(walk.src_loc_tk, walk.indent, start, typed,
                             alignment);

        // without a known alignment every access is a byte
        if (start.width == byte_size_bytes_) {
            walk_runtime_bytes(walk);
            return;
        }

        // the end of the chunks, also the scratch of the head check and the
        // halfword tail test
        const operand chunks{
            alloc_scratch_register(walk.src_loc_tk, walk.indent,
                                   default_type()),
        };

        comment(walk.src_loc_tk, walk.indent, "{}: end of {}, {}: tail bytes",
                chunks.base_register(),
                start.width == word_size_bytes_ ? "words" : "halfwords",
                walk.count.base_register());

        if (start.head_size_bytes != 0) {
            walk_runtime_head(walk, chunks, start.head_size_bytes);
        }

        walk_runtime_chunks(walk, chunks, start.width);
        walk_runtime_tail(walk, chunks, start.width);
    }

    // each head access first checks that the count still covers it, otherwise
    // the fewer bytes are left to the tail, which needs no more than the
    // current pointer alignment
    auto walk_runtime_head(const runtime_walk& walk, const operand& chunks,
                           const size_t head_size_bytes) -> void {

        const std::string_view count{walk.count.base_register()};

        comment(walk.src_loc_tk, walk.indent, "{} {} B head", walk.verb,
                head_size_bytes);

        if ((head_size_bytes & size_t{1}) != 0) {
            assembler_.beqz(walk.indent, count, after_head.reference);

            walk_runtime_step(walk, 1);
            assembler_.addi(walk.indent, count, count, -1);
        }

        if ((head_size_bytes & size_t{2}) != 0) {
            assembler_.sltiu(walk.indent, chunks.base_register(), count, 2);

            assembler_.bnez(walk.indent, chunks.base_register(),
                            after_head.reference);

            walk_runtime_step(walk, 2);
            assembler_.addi(walk.indent, count, count, -2);
        }

        assembler_.label(walk.indent, after_head.name);
    }

    // one access at the current pointers, then both move past it
    auto walk_runtime_step(const runtime_walk& walk, const size_t width)
        -> void {

        walk.access(width);
        advance(walk.indent, walk.src, width);
        advance(walk.indent, walk.dst, width);
    }

    // after words at most 3 bytes remain, bit 1 selects a halfword and bit 0
    // the final byte
    auto walk_runtime_tail(const runtime_walk& walk, const operand& chunks,
                           const size_t width) -> void {

        const std::string_view count{walk.count.base_register()};

        if (width == word_size_bytes_) {
            comment(walk.src_loc_tk, walk.indent, "{} optional 2-byte tail",
                    walk.verb);

            assembler_.andi(walk.indent, chunks.base_register(), count, 2);

            assembler_.beqz(walk.indent, chunks.base_register(),
                            after_halfword.reference);

            walk_runtime_step(walk, 2);
            assembler_.label(walk.indent, after_halfword.name);
            assembler_.andi(walk.indent, count, count, 1);
        }

        comment(walk.src_loc_tk, walk.indent, "{} optional final byte",
                walk.verb);

        assembler_.beqz(walk.indent, count, walk_end.reference);
        walk.access(1);
        assembler_.label(walk.indent, walk_end.name);
    }

    // a register destination holds the result directly, memory needs a scratch
    [[nodiscard]] auto working_register(const token& src_loc_tk,
                                        const size_t indent, const operand& dst)
        -> operand {

        if (dst.is_register()) {
            return dst;
        }

        return alloc_scratch_register(src_loc_tk, indent, default_type());
    }

    // every access matched or the range was empty, unless a mismatch branched
    // to the false exit
    auto write_compare_result(const token& src_loc_tk, const size_t indent,
                              const operand& result,
                              const compare_registers& registers,
                              const bool inverted) -> void {

        comment(src_loc_tk, indent, "all matched or empty: {}",
                inverted ? "false" : "true");

        assembler_.li(indent, registers.left.base_register(), inverted ? 0 : 1);
        assembler_.j(indent, result_end.reference);
        assembler_.label(indent, false_exit.name);

        comment(src_loc_tk, indent, "mismatch: {}",
                inverted ? "true" : "false");

        assembler_.li(indent, registers.left.base_register(), inverted ? 1 : 0);
        assembler_.label(indent, result_end.name);

        if (not registers.left_is_result) {
            copy_value(src_loc_tk, indent, result, registers.left);
        }
    }

    auto zero_access(const size_t indent, const size_t width,
                     const operand& address) -> void {

        assembler_.store(indent, store_op(width), "zero",
                         address.displacement(), address.base_register());
    }

    // known size: the address is a memory operand
    auto zero_known_size(const token& src_loc_tk, const size_t indent,
                         const operand& dst, const size_t size_bytes,
                         const access_start& start) -> void {

        // keeps the registers of the address from being picked for scratch and
        // frees the registers allocated below
        const address_scope scope{*this, dst, operand{}};

        const std::span<const access_start> starts{&start, 1};

        if (aligned_part_count(size_bytes, starts) <=
            zero_unroll_threshold_parts_) {

            comment_aligned_parts(src_loc_tk, indent, "zero", size_bytes,
                                  starts);

            const operand at{
                unrolled_address(src_loc_tk, indent, dst, size_bytes),
            };

            zero_parts(indent, size_bytes, starts, at);

            return;
        }

        const known_loop loop{plan_known_loop(size_bytes, starts)};

        comment_known_loop(src_loc_tk, indent, "zero", loop, starts);

        if (loop.head_size_bytes != 0) {
            // a head pointer for a far offset is not needed by the loop
            const address_scope head_scope{*this};

            const operand at{
                unrolled_address(src_loc_tk, indent, dst, loop.head_size_bytes),
            };

            zero_parts(indent, loop.head_size_bytes, starts, at);
        }

        const operand pointer{
            load_pointer(src_loc_tk, indent,
                         offset_by(dst, loop.head_size_bytes)),
        };

        const operand end{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        comment(src_loc_tk, indent, "{}", describe_loop("zero", loop.width));

        set_loop_end(src_loc_tk, indent, end, pointer,
                     loop.chunk_count * loop.width);

        assembler_.label(indent, chunk_loop.name);
        zero_access(indent, loop.width, memory_at(pointer));
        advance(indent, pointer, loop.width);

        assembler_.bne(indent, pointer.base_register(), end.base_register(),
                       chunk_loop.reference);

        if (loop.tail_size_bytes == 0) {
            return;
        }

        comment(src_loc_tk, indent, "zero {} B tail", loop.tail_size_bytes);

        // the pointer is at a boundary of the loop width
        const access_start boundary{
            .alignment{loop.width},
            .phase{},
        };

        zero_parts(indent, loop.tail_size_bytes, std::span{&boundary, 1},
                   memory_at(pointer));
    }

    auto zero_parts(const size_t indent, const size_t size_bytes,
                    const std::span<const access_start> starts,
                    const operand& address) -> void {

        for (const access_part& part : aligned_parts(size_bytes, starts)) {
            zero_access(indent, part.width, offset_by(address, part.offset));
        }
    }

    //
    // statics
    //

    // the same count as 'aligned_parts' without keeping the parts, which a
    // large size would not allow
    [[nodiscard]] static auto
    aligned_part_count(const size_t size_bytes,
                       const std::span<const access_start> starts) -> size_t {

        size_t count{};
        size_t offset{};
        while (offset < size_bytes) {
            const size_t width{aligned_width_at(size_bytes, offset, starts)};

            // a word access keeps every start word aligned, so whole words
            // follow until fewer than a word remain
            if (width == word_size_bytes_) {
                const size_t word_count{
                    (size_bytes - offset) / word_size_bytes_,
                };

                count += word_count;
                offset += word_count * word_size_bytes_;
                continue;
            }

            offset += width;
            ++count;
        }

        return count;
    }

    // each access is the widest that fits the remaining bytes and is aligned
    // for every address, so an unaligned start takes a byte and a halfword up
    // to the word boundary, then words, then a halfword and byte tail
    [[nodiscard]] static auto
    aligned_parts(const size_t size_bytes,
                  const std::span<const access_start> starts)
        -> std::vector<access_part> {

        std::vector<access_part> parts;
        size_t offset{};
        while (offset < size_bytes) {
            const size_t width{aligned_width_at(size_bytes, offset, starts)};

            parts.push_back({
                .offset{offset},
                .width{width},
            });

            offset += width;
        }

        return parts;
    }

    [[nodiscard]] static auto
    aligned_width_at(const size_t size_bytes, const size_t offset,
                     const std::span<const access_start> starts) -> size_t {

        size_t width{word_size_bytes_};

        while (width > 1 and (width > size_bytes - offset or
                              not is_aligned_at(starts, offset, width))) {

            width /= 2;
        }

        return width;
    }

    // 'copy' of a constant takes 'la' and a load and a store for each of its
    // 'copy_part_count' parts, above the unroll threshold it loops
    [[nodiscard]] static auto
    are_immediates_smaller(const std::span<const byte_part> parts,
                           const size_t size_bytes,
                           const size_t copy_part_count) -> bool {

        if (size_bytes > copy_unroll_threshold_bytes_) {
            return false;
        }

        const size_t copy_size_bytes{
            assembler_rv32i::two_instructions_bytes +
                (copy_part_count * assembler_rv32i::two_instructions_bytes),
        };

        size_t immediates_size_bytes{};
        for (const byte_part& p : parts) {
            immediates_size_bytes += assembler_rv32i::one_instruction_bytes;

            if (p.needs_load) {
                immediates_size_bytes +=
                    assembler_rv32i::li_value_size_bytes(p.value);
            }
        }

        return immediates_size_bytes <= copy_size_bytes;
    }

    // folding removes the operations that keep the destination, an operation
    // of a location with itself is a valid one that an aliased result reaches
    static auto
    assert_folded([[maybe_unused]] const op instruction,
                  [[maybe_unused]] const operand& dst,
                  [[maybe_unused]] const std::optional<int32_t> constant)
        -> void {

        const size_t width{dst.type_ref().size_bytes()};

        assert(not constant.has_value() or
               not keeps_destination(instruction, *constant, width));
    }

    // aligned hardware requires every access to be within the known alignment:
    // 1 selects 'lbu'/'sb', 2 'lhu'/'sh', 4 and above 'lw'/'sw'
    [[nodiscard]] static auto bulk_width(const size_t alignment) -> size_t {
        return std::min(alignment, word_size_bytes_);
    }

    // a register result holds the left value when no operand needs the register
    [[nodiscard]] static auto
    can_hold_left_value(const operand& result,
                        const std::span<const operand> in_use) -> bool {

        if (not result.is_register()) {
            return false;
        }

        const size_t index{register_index(result.base_register())};

        // 'zero' discards writes and 'sp' holds the stack
        assert(index != register_index("zero") and
               index != register_index("sp"));

        // writing the left value into one of the operands' registers would
        // destroy an address or the count before the walk is done with it; an
        // operand without an index register has an empty name that matches none
        return std::ranges::none_of(in_use, [index](const operand& o) -> bool {
            return register_index(o.base_register()) == index or
                   register_index(o.index_register()) == index;
        });
    }

    [[nodiscard]] static auto
    can_reuse_address_destination(const operand& address, const operand& dst)
        -> bool {

        // no destination register is available to reuse
        if (not dst.is_register()) {
            return false;
        }

        const size_t dst_index{register_index(dst.base_register())};

        // zero discards writes, so it cannot hold the computed address
        assert(dst_index != 0);

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

    // the code and data of the image cannot be addressed beyond 32 bits
    static auto check_address_range(const size_t memory_end_address) -> void {
        if (memory_end_address <= address_space_bytes_) {
            return;
        }

        throw std::runtime_error{std::format(
            "code, data and variables use {} B, which exceeds the RV32I "
            "address range",
            memory_end_address)};
    }

    // what a loop of 'width' byte accesses does, e.g. 'copy 4-byte words'
    [[nodiscard]] static auto describe_loop(const std::string_view verb,
                                            const size_t width) -> std::string {

        if (width == byte_size_bytes_) {
            return std::format("{} bytes", verb);
        }

        return std::format("{} {}-byte {}", verb, width,
                           width == word_size_bytes_ ? "words" : "halfwords");
    }

    // runs of equal widths, e.g. '1 + 2 + 4 x 4 + 1 B'
    [[nodiscard]] static auto
    describe_parts(const size_t size_bytes,
                   const std::span<const access_start> starts) -> std::string {

        std::vector<std::pair<size_t, size_t>> runs;
        for (const access_part& part : aligned_parts(size_bytes, starts)) {
            if (not runs.empty() and runs.back().first == part.width) {
                ++runs.back().second;
                continue;
            }

            runs.emplace_back(part.width, 1);
        }

        std::string text;
        for (const auto& [width, count] : runs) {
            if (not text.empty()) {
                text += " + ";
            }

            text += count == 1 ? std::format("{}", width)
                               : std::format("{} x {}", width, count);
        }

        return text + " B";
    }

    [[nodiscard]] static auto describe_start(const access_start& start)
        -> std::string {

        if (start.phase != 0) {
            return std::format("{} B past a word boundary", start.phase);
        }

        if (start.alignment == word_size_bytes_) {
            return "word aligned";
        }

        return std::format("{}-byte aligned", start.alignment);
    }

    [[nodiscard]] static auto
    describe_starts(const std::span<const access_start> starts) -> std::string {

        if (starts.size() == 1) {
            return std::format("start {}", describe_start(starts.front()));
        }

        return std::format("source {}, destination {}",
                           describe_start(starts.front()),
                           describe_start(starts.back()));
    }

    // the shift that extends the high bits of a narrow value, zero for bool
    [[nodiscard]] static auto extend_shift_op(const type& value_type) -> op {
        return value_type.is_bool() ? op::srli : op::srai;
    }

    [[nodiscard]] static auto format_address(const operand& address)
        -> std::string {

        // variables and frames are addressed from a base register
        assert(not address.base_register().empty());
        assert(address.index_register().empty());

        std::string text{address.base_register()};

        if (address.displacement() < 0) {
            const uint64_t magnitude{
                uint64_t{} - static_cast<uint64_t>(address.displacement()),
            };

            text += std::format(" - {}", magnitude);
        } else if (address.displacement() > 0) {
            text += std::format(" + {}", address.displacement());
        }

        return text;
    }

    [[nodiscard]] static auto has_all_bits(const int32_t constant,
                                           const size_t width) -> bool {

        const uint32_t mask{
            std::numeric_limits<uint32_t>::max() >>
                ((word_size_bytes_ - width) * bits_per_byte),
        };

        return (static_cast<uint32_t>(constant) & mask) == mask;
    }

    // the registers that a call of the helper changes
    [[nodiscard]] static auto helper_clobbers(const bool division)
        -> std::span<const std::string_view> {

        // argument registers are allocated last, so the helpers rarely
        // clobber a live scratch register that would need saving
        static constexpr std::array<std::string_view, 8> clobbers{
            "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6",
        };

        // the multiply helper changes the first five, the divide helper all
        constexpr size_t multiply_clobber_count{5};

        return std::span{clobbers}.first(division ? clobbers.size()
                                                  : multiply_clobber_count);
    }

    // the register-immediate form of 'add', 'and', 'or' and 'xor', 'sub' adds
    // the negated immediate
    [[nodiscard]] static auto immediate_form(const op operation) -> op {
        if (operation == op::and_op) {
            return op::andi;
        }

        if (operation == op::or_op) {
            return op::ori;
        }

        if (operation == op::xor_op) {
            return op::xori;
        }

        return op::addi;
    }

    [[nodiscard]] static auto immediate_value(const operand& value)
        -> std::optional<int32_t> {

        const std::optional<uint64_t> bits{immediate_bits(value)};

        if (not bits) {
            return std::nullopt;
        }

        return std::bit_cast<int32_t>(static_cast<uint32_t>(*bits));
    }

    [[nodiscard]] static auto
    is_aligned_at(const std::span<const access_start> starts,
                  const size_t offset, const size_t width) -> bool {

        return std::ranges::all_of(
            starts, [offset, width](const access_start& s) -> bool {
                return width <= s.alignment and (s.phase + offset) % width == 0;
            });
    }

    [[nodiscard]] static auto is_register(const std::string_view name) -> bool {
        return register_index(name) != register_names_.size();
    }

    [[nodiscard]] static auto is_scalar(const type& value_type) -> bool {
        return value_type.is_builtin() and
               (value_type.size_bytes() == byte_size_bytes_ or
                value_type.size_bytes() == half_size_bytes_ or
                value_type.size_bytes() == word_size_bytes_);
    }

    // 'x op constant' is 'x': all bits for 'and', zero for the others
    [[nodiscard]] static auto keeps_destination(const op instruction,
                                                const int32_t constant,
                                                const size_t width) -> bool {

        if (instruction == op::and_op) {
            return has_all_bits(constant, width);
        }

        return constant == 0;
    }

    // the clobbered registers that hold a live value, 'live' is the mask of
    // the registers in use
    [[nodiscard]] static auto
    live_clobbered(const uint32_t live,
                   const std::span<const std::string_view> clobbered,
                   const operand& dst) -> std::vector<std::string_view> {

        std::vector<std::string_view> saved;

        for (const std::string_view name : clobbered) {
            // restoring a register destination would discard the result
            const bool is_destination{
                dst.is_register() and
                    register_index(dst.base_register()) == register_index(name),
            };

            if ((live & register_mask(name)) != 0 and not is_destination) {
                saved.push_back(name);
            }
        }

        return saved;
    }

    // lb and lh sign-extend, a bool is zero-extended
    [[nodiscard]] static auto load_op(const type& value_type) -> op {
        if (value_type.size_bytes() == word_size_bytes_) {
            return op::lw;
        }

        if (value_type.size_bytes() == half_size_bytes_) {
            return op::lh;
        }

        if (value_type.is_bool()) {
            return op::lbu;
        }

        return op::lb;
    }

    [[nodiscard]] static auto make_digit_sequence(const uint32_t multiplier,
                                                  const size_t bits)
        -> digit_sequence {

        std::array<int, multiplier_digit_count> digits{
            multiplier_digits(multiplier),
        };

        // a digit at the product width vanishes modulo the width, leaving a
        // negative multiplier that is cheaper to build positive then negate
        const bool negate{digits.at(bits) != 0};

        if (negate) {
            digits.at(bits) = 0;
            for (int& digit : digits) {
                digit = -digit;
            }
        }

        // the leading nonzero digit is now plus one and starts the result
        size_t top{digits.size() - 1};
        // note: -1 is the index of the top digit

        while (digits.at(top) == 0) {
            --top;
        }

        size_t lowest{};
        while (digits.at(lowest) == 0) {
            ++lowest;
        }

        return {
            .digits{digits},
            .negate{negate},
            .top{top},
            .lowest{lowest},
        };
    }

    // non-adjacent form turns a run of set bits into one subtraction, e.g. 7
    // as 8 - 1, so each run costs one shift and add instead of one per bit
    [[nodiscard]] static auto multiplier_digits(const uint32_t multiplier)
        -> std::array<int, multiplier_digit_count> {

        std::array<int, multiplier_digit_count> digits{};
        uint64_t remaining{multiplier};
        for (size_t i{}; remaining != 0; ++i) {
            if ((remaining & 1U) == 0) {
                remaining >>= 1U;
                continue;
            }

            // remainder 3 modulo 4 is inside a run of set bits
            const bool in_run{(remaining & 3U) == 3};
            digits.at(i) = in_run ? -1 : 1;
            remaining = in_run ? remaining + 1 : remaining - 1;
            // note: +1 carries a run of set bits, -1 clears a lone set bit

            remaining >>= 1U;
        }

        return digits;
    }

    // the value as a register of 'value_type' holds it: truncated to the width,
    // then sign-extended unless it is a bool
    [[nodiscard]] static auto narrow_constant(const int32_t constant,
                                              const type& value_type)
        -> int32_t {

        const size_t bits{value_type.size_bits()};

        const uint32_t mask{
            std::numeric_limits<uint32_t>::max() >> (register_bits_ - bits),
        };

        uint32_t value{static_cast<uint32_t>(constant) & mask};

        if (not value_type.is_bool() and
            (value & (uint32_t{1} << (bits - 1))) != 0) {
            // note: -1 because the sign bit is the highest bit

            value |= ~mask;
        }

        return std::bit_cast<int32_t>(value);
    }

    // comparisons convert the right operand to the left operand's width
    [[nodiscard]] static auto narrowed_immediate(const operand& value,
                                                 const type& width_type)
        -> std::optional<int32_t> {

        const std::optional<int32_t> constant{immediate_value(value)};

        if (not constant.has_value()) {
            return std::nullopt;
        }

        return narrow_constant(*constant, width_type);
    }

    // whether the result may have bits outside the destination width
    [[nodiscard]] static auto
    needs_normalize(const op instruction, const operand& dst,
                    const operand& src, const std::optional<int32_t> constant)
        -> bool {

        // sums can carry out of the width
        if (instruction == op::add or instruction == op::sub) {
            return true;
        }

        const type& dst_type{dst.type_ref()};

        // bitwise results stay in range when the source representation does
        if (not constant.has_value()) {
            return src.type_ref().size_bytes() > dst_type.size_bytes() or
                   src.type_ref().is_bool() != dst_type.is_bool();
        }

        // a constant is narrowed to the width of a destination, a bool
        // destination has no constant operand
        assert(not dst_type.is_bool());

        const int64_t limit{
            static_cast<int64_t>(uint64_t{1} << (dst_type.size_bits() - 1)),
        };
        // note: -1 because the sign bit is the highest bit of the type

        return *constant < -limit or *constant >= limit;
    }

    [[nodiscard]] static auto offset_by(const operand& address,
                                        const size_t offset) -> operand {

        operand moved{address};
        moved.increment_offset(static_cast<int64_t>(offset));
        return moved;
    }

    // the loop starts at the widest boundary every start reaches after a head
    // of at most 'size_bytes'
    [[nodiscard]] static auto
    plan_known_loop(const size_t size_bytes,
                    const std::span<const access_start> starts) -> known_loop {

        const loop_start start{plan_loop_start(starts, size_bytes)};

        const size_t after_head_size_bytes{size_bytes - start.head_size_bytes};

        // a loop is only chosen above 16 bytes, so there is more than one chunk
        assert(after_head_size_bytes / start.width > 1);

        return {
            .head_size_bytes{start.head_size_bytes},
            .width{start.width},
            .chunk_count{after_head_size_bytes / start.width},
            .tail_size_bytes{after_head_size_bytes % start.width},
        };
    }

    // the widest width that every start reaches after the same head of at most
    // 'max_head_size_bytes'
    [[nodiscard]] static auto
    plan_loop_start(const std::span<const access_start> starts,
                    const size_t max_head_size_bytes) -> loop_start {

        for (size_t width{word_size_bytes_}; width > 1; width /= 2) {
            const size_t phase{starts.front().phase % width};
            const size_t head_size_bytes{(width - phase) % width};

            const bool reaches_width{
                std::ranges::all_of(
                    starts,
                    [width, phase](const access_start& s) -> bool {
                        return width <= s.alignment and
                               s.phase % width == phase;
                    }),
            };

            if (reaches_width and head_size_bytes <= max_head_size_bytes) {
                return {
                    .head_size_bytes{head_size_bytes},
                    .width{width},
                };
            }
        }

        return {
            .head_size_bytes{},
            .width{1},
        };
    }

    [[nodiscard]] static auto register_index(const std::string_view name)
        -> size_t {

        return assembler_rv32i::register_number(name).value_or(
            register_names_.size());
    }

    [[nodiscard]] static auto register_mask(const std::string_view name)
        -> uint32_t {

        const size_t index{register_index(name)};
        return index == register_names_.size() ? 0 : uint32_t{1} << index;
    }

    // a narrow register gets high bits from a wider, immediate or
    // differently extended source
    [[nodiscard]] static auto register_needs_extension(const type& dst_type,
                                                       const operand& src)
        -> bool {

        const size_t dst_size_bytes{dst_type.size_bytes()};
        const size_t src_size_bytes{src.type_ref().size_bytes()};

        const bool extension_differs{
            src.type_ref().is_bool() != dst_type.is_bool(),
        };

        return dst_size_bytes < 4 and
               (src.is_immediate() or src_size_bytes > dst_size_bytes or
                (extension_differs and src_size_bytes == dst_size_bytes));
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
               left.type_ref().is_same(right.type_ref()) and
               same_register(left.base_register(), right.base_register()) and
               same_register(left.index_register(), right.index_register()) and
               left.scale() == right.scale() and
               left.displacement() == right.displacement();
    }

    // a memory copy between two addresses of one base register without index
    // whose offsets differ only in the low 12 bits
    [[nodiscard]] static auto shares_address_base(const operand& dst,
                                                  const operand& src) -> bool {

        if (not dst.is_memory() or not src.is_memory()) {
            return false;
        }

        if (not dst.index_register().empty() or
            not src.index_register().empty()) {

            return false;
        }

        if (register_index(dst.base_register()) !=
            register_index(src.base_register())) {

            return false;
        }

        const uint32_t src_upper{
            split_address_offset(src.displacement()).upper,
        };

        // without an upper part the access needs no base register of its own
        return src_upper != 0 and
               src_upper == split_address_offset(dst.displacement()).upper;
    }

    [[nodiscard]] static auto split_address_offset(const int64_t offset)
        -> address_offset_parts {

        // the signed low 12 bits stay in the memory operand, the upper part
        // is loaded with lui
        return {
            .upper{assembler_rv32i::upper_part(offset)},
            .low{assembler_rv32i::lower_part(offset)},
        };
    }

    // the widest aligned parts like the unrolled 'copy'
    [[nodiscard]] static auto split_bytes(const std::string_view bytes,
                                          const access_start& start)
        -> std::vector<byte_part> {

        std::vector<byte_part> parts;
        int64_t loaded_value{};
        for (const access_part& part :
             aligned_parts(bytes.size(), std::span{&start, 1})) {

            const int64_t value{
                little_endian_value(bytes.substr(part.offset, part.width)),
            };

            const bool needs_load{value != 0 and value != loaded_value};

            if (needs_load) {
                loaded_value = value;
            }

            parts.push_back({
                .offset{part.offset},
                .size_bytes{part.width},
                .value{value},
                .needs_load{needs_load},
            });
        }

        return parts;
    }

    // stores the low 8, 16 or 32 bits
    [[nodiscard]] static auto store_op(const size_t width) -> op {
        if (width == word_size_bytes_) {
            return op::sw;
        }

        if (width == half_size_bytes_) {
            return op::sh;
        }

        return op::sb;
    }

    // only 'slt' and 'blt' exist: 'x > y' is 'y < x' and 'x <= y' is the
    // complement of 'y < x'
    [[nodiscard]] static auto
    swaps_operands(const comparison_operator operation) -> bool {

        return operation == comparison_operator::greater or
               operation == comparison_operator::less_equal;
    }

    // pointers may hold any address, so their starts from 'start_of' with
    // alignment 1 gain the type 'alignment' here
    [[nodiscard]] static auto
    typed_starts(const std::array<access_start, 2>& starts,
                 const size_t alignment) -> std::array<access_start, 2> {

        std::array<access_start, 2> typed{starts};
        for (access_start& s : typed) {
            s.alignment = std::max(s.alignment, bulk_width(alignment));
        }

        return typed;
    }

    // loads without sign extension, for copying bytes unchanged
    [[nodiscard]] static auto unsigned_load_op(const size_t width) -> op {
        if (width == word_size_bytes_) {
            return op::lw;
        }

        if (width == half_size_bytes_) {
            return op::lhu;
        }

        return op::lbu;
    }

    // aliases and address registers carry the same argument dependencies
    [[nodiscard]] static auto uses_register(const operand& value,
                                            const std::string_view name)
        -> bool {

        return (value.is_register() or value.is_memory()) and
               (register_index(value.base_register()) == register_index(name) or
                (value.is_memory() and register_index(value.index_register()) ==
                                           register_index(name)));
    }

    static auto validate_address(const token& src_loc_tk,
                                 const operand& address) -> void {

        assert(address.is_memory());
        assert(is_register(address.base_register()));

        assert(address.index_register().empty() or
               is_register(address.index_register()));

        // an element size beyond the address range cannot be allocated
        assert(address.index_register().empty() or
               address.scale() <= std::numeric_limits<uint32_t>::max());

        constexpr int64_t limit{std::numeric_limits<uint32_t>::max()};

        if (address.displacement() < -limit or address.displacement() > limit) {
            throw compiler_exception{
                src_loc_tk, "address offset exceeds RV32I address range"};
        }
    }

    // the result is written in place
    static auto validate_destination_storage(const token& src_loc_tk,
                                             const operand& dst) -> void {

        assert(dst.is_register() or dst.is_memory());

        if (dst.is_memory()) {
            validate_address(src_loc_tk, dst);
        }
    }

    // scalar operands in a register or an address of the target
    static auto validate_operands(const token& src_loc_tk, const operand& dst,
                                  const operand& src) -> void {

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_scalar(src_loc_tk, src.type_ref());

        assert(dst.is_register() or dst.is_memory());

        if (dst.is_memory()) {
            validate_address(src_loc_tk, dst);
        }

        if (src.is_memory()) {
            validate_address(src_loc_tk, src);
        }
    }

    static auto validate_scalar(const token& src_loc_tk, const type& value_type)
        -> void {

        if (not is_scalar(value_type)) {
            throw compiler_exception{
                src_loc_tk, "RV32I requires an 8-, 16-, or 32-bit scalar"};
        }
    }

    // 'x op constant' is 'constant': and-ing zero or or-ing all bits
    [[nodiscard]] static auto yields_constant(const op instruction,
                                              const int32_t constant,
                                              const size_t width) -> bool {

        return (instruction == op::and_op and constant == 0) or
               (instruction == op::or_op and has_all_bits(constant, width));
    }
};
