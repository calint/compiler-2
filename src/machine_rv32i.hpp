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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "assembler_rv32i.hpp"
#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "register_pool.hpp"
#include "register_report.hpp"
#include "token.hpp"
#include "type.hpp"

// the rv32i backend, written as llvm assembler text and a binary image
// through 'assembler_rv32i'; on linux in qemu user mode, the bare-metal targets
// derive from it ('machine_rv32i_bare_metal'). the formatter sorts the
// functions by name, so by topic they are:
//
//   registers    'machine' allocates them, here the order
//                'scratch_registers_', the frame base 's1' and the variables
//                base 's0'
//   addresses    'lower_address', 'lower_address_offset': an operand into a
//                base register and a 12-bit offset
//   operations   'binary_operation', 'emit_binary_instruction',
//                'multiply_by_constant' (signed digits), 'shift_by_constant',
//                'shift_by_register'
//   helpers      '.Lbaz_multiply', '.Lbaz_multiply_checked' and
//                '.Lbaz_divide' for 'mul' and 'div',
//                'call_arithmetic_helper', 'emit_arithmetic_helpers'
//   memory       'zero_known_size', 'copy_known_size', 'compare_known_size'
//                for a known size, 'walk_runtime_count' for a run-time count
//                (see the note before 'local_label')
//   comparisons  'compare_and_branch', 'emit_comparison', 'compare_memory'
//   checks       'check_bounds', 'check_divisor', 'check_fits_width',
//                'check_shift_count', 'check_copy_overlap',
//                'check_frame_capacity', 'check_stack_capacity', the overflow
//                checks 'check_negate', 'check_multiply_result',
//                'emit_checked_add_subtract'
//   handlers     'emit_*_failure_handler', 'emit_named_failure_handler',
//                'emit_line_failure', 'emit_report_line_routine'
//   calls        'call_function', the system calls 'emit_read_call',
//                'emit_write_call' and 'exit', the sector routines
//   output       'write_assembly' writes the text and the image
class machine_rv32i : public machine {
    using op = assembler_rv32i::op;

    using section = assembler_rv32i::section;

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

    // note: a gap takes one to three stores, the whole instance a store per
    //       word up to 64 B and a loop of 5 instructions with its head and
    //       tail beyond; measured on 2500 random types with padding, from 9
    //       gaps the whole instance was larger in 1 of 533, at 7 and 8 gaps in
    //       10 of 146

    static constexpr size_t max_padding_gaps_zeroed_one_by_one_{8};

    static constexpr int64_t immediate_min_{-2048};
    static constexpr int64_t immediate_max_{2047};
    static constexpr int syscall_read_{63};
    static constexpr int syscall_write_{64};
    static constexpr int syscall_exit_{93};
    static constexpr int syscall_openat_{56};
    static constexpr int syscall_llseek_{62};
    static constexpr int syscall_pread_{67};
    static constexpr int syscall_pwrite_{68};
    static constexpr std::array<std::string_view, 4> sector_clobbered_{
        "a3",
        "a4",
        "a5",
        "a6",
    };
    // shifting a word right by this spreads its sign bit over all bits
    static constexpr int sign_shift_{31};
    // sp stays 16-byte aligned, the return address slot of a frame is one unit
    static constexpr size_t stack_alignment_{16};
    static constexpr int64_t frame_save_bytes_{stack_alignment_};
    // the line report of a failure writes the ten digits of a 32-bit line
    // number and a newline on the stack
    static constexpr int64_t line_digits_bytes_{16};
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

    static constexpr std::array<std::string_view, 30> scratch_registers_{
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "s0",  "s1",  "s2",
        "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "tp",
        "gp", "ra", "a1", "a2", "a3", "a4", "a5", "a6",  "a7",  "a0",
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

    // the registers of an active copy or comparison of arrays and where its
    // two addresses start, each start taken with alignment 1 since the type
    // alignment is given at the end
    struct bulk_operation {
        // the two pointers then the count, in the order of allocation
        std::array<operand, 3> registers;
        std::array<access_start, 2> starts;
        // a missing address would leave the word alignment unproven
        size_t address_count{};

        [[nodiscard]] auto count() const -> const operand& {
            return registers.back();
        }

        // 0 the first address and 1 the second
        [[nodiscard]] auto pointer(const size_t slot) const -> const operand& {
            assert(slot < starts.size());

            return registers.at(slot);
        }
    };

    struct address_offset_parts {
        uint32_t upper;
        int32_t low;
    };

    // what a call of '.Lbaz_multiply', '.Lbaz_multiply_checked' or
    // '.Lbaz_divide' computes
    enum class helper_result : uint8_t { product, quotient, remainder };

    // the run-time routines of 'call_arithmetic_helper'
    enum class helper_routine : uint8_t { multiply, multiply_checked, divide };

    // a 32-bit multiplier can need one digit above bit 31 in non-adjacent form
    static constexpr size_t multiplier_digit_count_{33};

    // 'value' is modified in place then written back by
    // 'store_operation_result' through 'address'
    struct loaded_destination {
        operand address;
        operand value;
    };

    // a multiplier as signed digits: a factor of 2^i is added or subtracted per
    // nonzero digit
    struct digit_sequence {
        std::array<int, multiplier_digit_count_> digits;
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
    //       * the variables base 's0' and a noinline frame base 's1' are
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
    static constexpr local_label chunk_loop_{
        .name{"1"},
        .reference{"1b"},
    };
    static constexpr local_label after_chunks_{
        .name{"2"},
        .reference{"2f"},
    };
    static constexpr local_label after_halfword_{
        .name{"3"},
        .reference{"3f"},
    };
    static constexpr local_label walk_end_{
        .name{"4"},
        .reference{"4f"},
    };
    static constexpr local_label false_exit_{
        .name{"5"},
        .reference{"5f"},
    };
    static constexpr local_label result_end_{
        .name{"6"},
        .reference{"6f"},
    };
    static constexpr local_label after_head_{
        .name{"7"},
        .reference{"7f"},
    };

    // the labels of a bounds check: a failing check branches to 'bounds_fail_'
    // where the line and the handler follow, a passing one to 'bounds_pass_'
    static constexpr local_label bounds_fail_{
        .name{"1"},
        .reference{"1f"},
    };
    static constexpr local_label bounds_pass_{
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

    // an error that unwinds an array operation ends it, so a version of code
    // that fails leaves no bulk registers behind, see 'emit_most_efficient'
    class bulk_depth_scope final {
        std::vector<bulk_operation>& operations_;
        size_t depth_;

      public:
        explicit bulk_depth_scope(std::vector<bulk_operation>& operations)
            : operations_{operations}, depth_{operations.size()} {}

        bulk_depth_scope(const bulk_depth_scope&) = delete;
        bulk_depth_scope(bulk_depth_scope&&) = delete;
        auto operator=(const bulk_depth_scope&) -> bulk_depth_scope& = delete;
        auto operator=(bulk_depth_scope&&) -> bulk_depth_scope& = delete;

        ~bulk_depth_scope() { operations_.resize(depth_); }
    };

    // the shared tail of the handlers that report a line, and its powers of
    // ten
    static constexpr std::string_view report_line_label_{".Lbaz_report_line"};
    static constexpr std::string_view decimal_places_label_{
        ".Lbaz_decimal_places",
    };
    // the part the system call sector routines share, and their data
    static constexpr std::string_view sector_transfer_label_{
        ".Lbaz_sector_transfer",
    };
    static constexpr std::string_view sd_card_label_{".Lbaz_sd_card"};
    static constexpr std::string_view sd_card_name_label_{".Lbaz_sd_card_name"};

    // empty when no binary image is written
    std::string binary_file_name_;
    assembler_rv32i assembler_;
    // the shared tail of the handlers that report a line was emitted
    bool line_report_emitted_{};
    std::vector<bulk_operation> bulk_operations_;

  protected:
    // the sector routines, see 'emit_sector_routines'
    static constexpr std::string_view sector_read_label_{".Lbaz_sector_read"};
    static constexpr std::string_view sector_write_label_{
        ".Lbaz_sector_write",
    };

    //
    // overridden methods
    //

    // the sector is in a0 where 'read' has the descriptor, there is no result
    auto call_sector_routine(const token& src_loc_tk, const size_t indent,
                             const operand& sector, const operand& address,
                             const operand& size,
                             const builtin_function function) -> void override {

        const operand call_register{
            reserve_call_register(src_loc_tk, indent, sector, address, size),
        };

        const std::string_view label{
            function == builtin_function::sector_read ? sector_read_label_
                                                      : sector_write_label_,
        };

        call_io_routine(indent, label, sector_clobbered_);
        free_named_register(src_loc_tk, indent, call_register);
    }

    // a 'noinline' body saves the return address below 'sp', the frame base
    // is commented like any named register
    auto emit_frame_entry() -> void override {
        comment(token{}, 0, "allocate named register {}",
                frame_base_register());

        assembler_.addi(1, "sp", "sp", -frame_save_bytes_);
        assembler_.sw(1, "ra", 0, "sp");
    }

    auto emit_frame_exit() -> void override {
        comment(token{}, 0, "free named register {}", frame_base_register());
    }

    // the routines take the sector in a0, the address in a1 and the size in
    // a2, return through a7 and may change a0 and a3 to a6; an operating
    // system transfers the sector of the image file
    auto emit_sector_routines(const bool read_used, const bool write_used)
        -> void override {

        emit_sector_system_call_routines(read_used, write_used);
    }

    [[nodiscard]] auto register_index_of(const std::string_view name) const
        -> std::optional<size_t> override {

        return assembler_rv32i::register_number(name);
    }

    [[nodiscard]] auto scratch_registers() const
        -> std::span<const std::string_view> override {

        return scratch_registers_;
    }

    [[nodiscard]] auto target_assembler() -> ::assembler& override {
        return assembler_;
    }

    [[nodiscard]] auto target_assembler() const -> const ::assembler& override {
        return assembler_;
    }

    auto validate_register_type(const token& src_loc_tk,
                                const type& type_ref) const -> void override {

        validate_scalar(src_loc_tk, type_ref);
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
    // a2 the count and a7 is reserved; an override may change the registers
    // after a2 in the allocation order, see 'call_io_routine'
    virtual auto emit_read_call(const size_t indent) -> void {
        assembler_.li(indent, "a7", syscall_read_);
        assembler_.ecall(indent);
    }

    // loads the lowest address the stack may reach into the register 'reg'
    // note: linux does not bound the stack, see 'is_stack_bounded'
    virtual auto
    emit_stack_limit_load([[maybe_unused]] const size_t indent,
                          [[maybe_unused]] const std::string_view reg) -> void {

        std::unreachable();
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
    // 'clobbered' besides a0, which are free at the call, so like a system
    // call it saves nothing
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

    //
    // statics
    //

    // what a callee pushes without a check of its own: a helper call saves
    // the registers the helper changes, and a failed check of the helper,
    // made while they are saved, writes the digits of its line
    [[nodiscard]] static auto leaf_stack_bytes() -> size_t {
        // note: '.Lbaz_divide' changes the most registers
        const size_t helper_save_bytes{
            align_storage_size(helper_clobbers(helper_routine::divide).size() *
                                   word_size_bytes_,
                               stack_alignment_),
        };

        return helper_save_bytes + line_digits_bytes_;
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
                      const operand& src, const runtime_check& check = {})
        -> void override {

        assert(operation == arithmetic_operator::add or
               operation == arithmetic_operator::subtract);

        binary_operation(src_loc_tk, indent,
                         operation == arithmetic_operator::add ? op::add
                                                               : op::sub,
                         dst, src, check);
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

    auto arrays_equal(const token& src_loc_tk, const size_t indent,
                      const size_t element_size_bytes,
                      const std::function_ref<void(const operand&)> emit_count,
                      const equality_request& request) -> void override {

        const bulk_depth_scope bulk_depth{bulk_operations_};

        const operand count{
            begin_array_operation(src_loc_tk, indent, "lhs", "rhs"),
        };

        emit_count(count);

        set_bulk_addresses(src_loc_tk, indent, count, request.lhs, request.rhs);

        const bulk_operation& bulk{bulk_operations_.back()};
        {
            // scaling the count must not pick the result registers
            const address_scope scope{*this, request.dst, operand{}};

            comment(src_loc_tk, indent,
                    "{}: elements to bytes ({} bytes/element)",
                    bulk.count().base_register(), element_size_bytes);

            scale_index(src_loc_tk, indent, bulk.count(), element_size_bytes);
        }

        compare_runtime_count(src_loc_tk, indent,
                              {
                                  .left{bulk.pointer(0)},
                                  .right{bulk.pointer(1)},
                                  .count{bulk.count()},
                                  .starts{bulk_starts()},
                              },
                              request);

        release_bulk(src_loc_tk, indent);
    }

    auto begin_data(const size_t alignment) -> void override {
        emit_arithmetic_helpers();

        emit_used_sector_routines();

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
        for (const size_t index : registers_saved_by_call()) {
            saved.push_back(register_names_.at(index));
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
                      const bounds_range& range,
                      const bounds_check_options& options) -> void override {

        if (not options.upper and not options.lower) {
            return;
        }

        // array lengths are limited by the variables that hold the arrays, the
        // line is loaded with 'li' into a 32-bit register
        assert(range.array_count <= std::numeric_limits<uint32_t>::max() and
               src_loc_tk.at_line() <= std::numeric_limits<uint32_t>::max());

        comment(src_loc_tk, indent, "bounds check begin");

        emit_bounds_check(src_loc_tk, indent, range, options);

        comment(src_loc_tk, indent, "bounds check end");
    }

    auto check_frame_capacity(const token& src_loc_tk, const size_t indent,
                              const operand& frame_address,
                              const operand& frame_size_bytes)
        -> void override {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(frame_size_bytes.is_symbol());

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

        // note: the start cannot wrap around the address space: from 's0' it
        //       follows the variables, which fit in 'vars' when compiled, and
        //       from 's1' it follows the caller's frame, which passed this
        //       check

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
                           frame_size_bytes.symbol(),
                           assembler_rv32i::immediate::part::high));

        assembler_.addi(indent, start.base_register(), start.base_register(),
                        assembler_rv32i::immediate::of_symbol(
                            frame_size_bytes.symbol(),
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

    // a call saves the registers allocated now, the registers of the arguments
    // included, so the bound is not below what the call pushes; the callee
    // pushes 'leaf_stack_bytes' more without a check of its own
    auto check_stack_capacity(const token& src_loc_tk, const size_t indent)
        -> void override {

        assert(is_stack_bounded());

        const size_t saved_count{registers_saved_by_call().size()};

        const size_t call_bytes{
            align_storage_size(saved_count * word_size_bytes_,
                               stack_alignment_) +
                frame_save_bytes_ + leaf_stack_bytes(),
        };

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        comment(src_loc_tk, indent, "stack capacity check begin");

        const operand limit{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const operand lowest{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        emit_stack_limit_load(indent, limit.base_register());

        comment(src_loc_tk, indent, "lowest stack pointer the call reaches");

        assembler_.addi(indent, lowest.base_register(), "sp",
                        -static_cast<int64_t>(call_bytes));

        assembler_.bgeu(indent, lowest.base_register(), limit.base_register(),
                        fits.reference);

        branch(indent, stack_overflow_handler_label);
        assembler_.label(indent, fits.name);
        free_scratch_register(src_loc_tk, indent, lowest);
        free_scratch_register(src_loc_tk, indent, limit);
        comment(src_loc_tk, indent, "stack capacity check end");
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

    auto copy(const token& src_loc_tk, const size_t indent, const operand& dst,
              const operand& src, const size_t size_bytes,
              const size_t alignment) -> void override {

        if (size_bytes == 0) {
            return;
        }

        // note: a constant count is within its array, and an array is within
        //       the 32-bit address range

        assert(size_bytes <= std::numeric_limits<uint32_t>::max());

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
        copy(src_loc_tk, indent, dst,
             operand::mem(pointer.base_register(), {}, 1, 0, dst.type_ref()),
             bytes.size(), width);
    }

    auto copy_elements(const token& src_loc_tk, const size_t indent,
                       const size_t element_size_bytes,
                       const std::function_ref<void(const operand&)> emit_count,
                       const copy_request& request) -> void override {

        const bulk_depth_scope bulk_depth{bulk_operations_};

        const operand count{
            begin_array_operation(src_loc_tk, indent, "source", "destination"),
        };

        emit_count(count);

        set_bulk_addresses(src_loc_tk, indent, count, request.src, request.dst);

        const bulk_operation& bulk{bulk_operations_.back()};

        comment(src_loc_tk, indent, "{}: elements to bytes ({} bytes/element)",
                bulk.count().base_register(), element_size_bytes);

        scale_index(src_loc_tk, indent, bulk.count(), element_size_bytes);

        if (request.overlap.enabled) {
            check_copy_overlap(src_loc_tk, indent, bulk.pointer(0),
                               bulk.pointer(1), bulk.count(), request.overlap);
        }

        copy_runtime_count(src_loc_tk, indent, bulk.pointer(0), bulk.pointer(1),
                           bulk.count(), bulk_starts(), request.alignment);

        release_bulk(src_loc_tk, indent);
    }

    auto copy_value(const token& src_loc_tk, const size_t indent,
                    const operand& dst, const operand& src) -> void override {

        validate_operands(src_loc_tk, dst, src);

        assert(not src.is_empty());

        // a copy onto itself, e.g. of an alias to its argument, changes
        // nothing
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

    [[nodiscard]] auto
    data_element_size_error(const size_t element_size_bytes) const
        -> std::optional<std::string> override {

        if (element_size_bytes != byte_size_bytes_ and
            element_size_bytes != half_size_bytes_ and
            element_size_bytes != word_size_bytes_) {

            return "RV32I data elements must be 1, 2, or 4 bytes";
        }

        return std::nullopt;
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
                const operand& divisor, const runtime_check& check)
        -> void override {

        assert(operation == arithmetic_operator::divide or
               operation == arithmetic_operator::remainder);

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_scalar(src_loc_tk, divisor.type_ref());

        assert(dst.is_register() or dst.is_memory());

        mark_used("divide_helper");

        call_arithmetic_helper(src_loc_tk, indent, dst, divisor,
                               operation == arithmetic_operator::remainder
                                   ? helper_result::remainder
                                   : helper_result::quotient,
                               check);
    }

    auto emit_bounds_failure_handler(const bool with_line) -> void override {
        emit_named_failure_handler(bounds_failure_handler_label, "bounds",
                                   with_line);
    }

    auto emit_data(const size_t element_size_bytes, const int64_t value)
        -> void override {

        assert_data_element_size(element_size_bytes);

        assembler_.data(element_size_bytes, std::array{value});
    }

    // the elements on one line, e.g. '.word 1, 2, 3'
    auto emit_data_array(const size_t element_size_bytes,
                         const std::function_ref<bool(int64_t&)> next)
        -> void override {

        assert_data_element_size(element_size_bytes);

        std::vector<int64_t> values;
        int64_t value{};
        while (next(value)) {
            values.push_back(value);
        }

        assembler_.data(element_size_bytes, values);
    }

    auto emit_division_failure_handler(const bool with_line) -> void override {
        emit_named_failure_handler(division_failure_handler_label, "division",
                                   with_line);
    }

    auto emit_frame_overflow_handler() -> void override {
        emit_named_failure_handler(frame_overflow_handler_label,
                                   "frame overflow", false);
    }

    auto emit_overflow_failure_handler(const bool with_line) -> void override {
        emit_named_failure_handler(overflow_failure_handler_label, "overflow",
                                   with_line);
    }

    auto emit_overlap_failure_handler(const bool with_line) -> void override {
        emit_named_failure_handler(overlap_failure_handler_label, "overlap",
                                   with_line);
    }

    auto emit_repeated_data(const size_t element_size_bytes, const size_t count,
                            const int64_t value) -> void override {

        assert_data_element_size(element_size_bytes);

        assembler_.repeated_data(element_size_bytes, count, value);
    }

    auto emit_shift_failure_handler(const bool with_line) -> void override {
        emit_named_failure_handler(shift_failure_handler_label, "shift",
                                   with_line);
    }

    auto emit_stack_overflow_handler() -> void override {
        assert(is_stack_bounded());

        emit_named_failure_handler(stack_overflow_handler_label,
                                   "stack overflow", false);
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

        // the routines that 'begin_data' emits follow and must stay in the code
        // section
        assembler_.switch_section(section::text);
    }

    auto emit_string_data(const std::string_view value) -> void override {
        assembler_.ascii(token::decode_string(value));
    }

    auto emit_zero_data(const size_t size_bytes) -> void override {
        assembler_.zero(size_bytes);
    }

    auto end_main() -> void override {
        exit(token{}, 1, operand::imm(0, default_type()));
    }

    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void override {

        copy_value(src_loc_tk, indent, operand::reg("a0", default_type()),
                   exit_code);

        assembler_.li(indent, "a7", syscall_exit_);
        assembler_.ecall(indent);
    }

    auto finish() -> void override {
        finish_registers();

        assert(bulk_operations_.empty());

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
                     operand::imm(element_size_bytes, default_type()));

        add_subtract(src_loc_tk, indent, arithmetic_operator::add, counter,
                     operand::imm(1, default_type()));

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

    // a load or store adds only an immediate to its base register
    [[nodiscard]] auto has_indexed_addressing() const -> bool override {
        return false;
    }

    // linux stops a program that overflows the stack
    [[nodiscard]] auto is_stack_bounded() const -> bool override {
        return false;
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

    [[nodiscard]] auto max_padding_gaps_zeroed_one_by_one() const
        -> size_t override {

        return max_padding_gaps_zeroed_one_by_one_;
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
                  [[maybe_unused]] const factor_use use,
                  const runtime_check& check = {}) -> void override {

        validate_scalar(src_loc_tk, product.type_ref());
        validate_scalar(src_loc_tk, factor.type_ref());
        validate_destination_storage(product);

        // validate memory operands even when a constant eliminates the
        // operation
        if (factor.is_memory()) {
            validate_address(factor);
        }

        const std::optional<int32_t> constant{immediate_value(factor)};

        // zero and one cannot overflow, shifts and adds do not report the
        // bits they lose
        const bool is_checked_constant{
            check.enabled and constant.has_value() and *constant != 0 and
                *constant != 1,
        };

        // variable factors use the shared run-time helper
        if (not constant.has_value() or is_checked_constant) {
            if (check.enabled) {
                mark_used("multiply_checked_helper");
            } else {
                mark_used("multiply_helper");
            }

            const runtime_check no_division_check;

            call_arithmetic_helper(src_loc_tk, indent, product, factor,
                                   helper_result::product, no_division_check,
                                   check);

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
            .result{"a0"},
        };
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
                 operand::imm(element_size_bytes, default_type()),
                 factor_use::read_only);
    }

    auto shift(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const operand& count, const runtime_check& check)
        -> void override {

        assert(operation == arithmetic_operator::shift_left or
               operation == arithmetic_operator::shift_right);

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_scalar(src_loc_tk, count.type_ref());
        validate_destination_storage(dst);

        const std::optional<int32_t> constant{immediate_value(count)};

        // immediate shifts must be resolved here rather than by the assembler
        assert(not count.is_immediate() or constant.has_value());

        const size_t bits{dst.type_ref().size_bits()};

        // note: the front end rejects a constant count outside the width
        assert(not constant.has_value() or
               (*constant >= 0 and std::cmp_less(*constant, bits)));

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

        shift_by_register(src_loc_tk, indent, operation, dst, count, loaded,
                          check);
    }

    [[nodiscard]] auto slot_register() const -> std::string_view override {
        return slot_register_;
    }

    auto start() -> void override {
        // note: the parse emits no failure
        assert(not line_report_emitted_);

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
                   operand::imm(value ? 1 : 0, default_type()));
    }

    auto unary(const token& src_loc_tk, const size_t indent,
               const arithmetic_operator operation, const operand& dst,
               const runtime_check& check = {}) -> void override {

        assert(operation == arithmetic_operator::negate or
               operation == arithmetic_operator::complement);

        // note: the front end rejects a 'bool' as an arithmetic operand
        assert(not dst.type_ref().is_bool());

        validate_scalar(src_loc_tk, dst.type_ref());

        assert(dst.is_register() or dst.is_memory());

        const address_scope scope{*this, dst, operand{}};

        const loaded_destination loaded{
            load_destination(src_loc_tk, indent, dst),
        };

        if (operation == arithmetic_operator::negate) {
            if (check.enabled) {
                check_negate(src_loc_tk, indent, loaded.value,
                             dst.type_ref().size_bytes(), check);
            }

            assembler_.sub(indent, loaded.value.base_register(), "zero",
                           loaded.value.base_register());

            store_operation_result(indent, dst, loaded.address, loaded.value,
                                   true);

            return;
        }

        assembler_.xori(indent, loaded.value.base_register(),
                        loaded.value.base_register(), -1);

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               false);
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
            throw unlocated_error{
                std::format("cannot write file '{}'", binary_file_name_)};
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

    // the bulk registers of an array copy or comparison, the roles of the
    // pointers name them in the comment, e.g. 'source' and 'destination'
    auto begin_array_operation(const token& src_loc_tk, const size_t indent,
                               const std::string_view first_role,
                               const std::string_view second_role) -> operand {

        const operand count{begin_bulk(src_loc_tk, indent)};

        const bulk_operation& bulk{bulk_operations_.back()};

        comment(src_loc_tk, indent, "{}: {}, {}: {}, {}: count",
                bulk.pointer(0).base_register(), first_role,
                bulk.pointer(1).base_register(), second_role,
                count.base_register());

        return count;
    }

    auto begin_bulk(const token& src_loc_tk, const size_t indent) -> operand {
        // argument-order allocation keeps pointer and count names easy to
        // follow
        bulk_operation bulk;
        for (operand& reg : bulk.registers) {
            reg = alloc_scratch_register(src_loc_tk, indent, default_type());
        }

        bulk_operations_.push_back(bulk);

        return bulk.count();
    }

    auto binary_operation(const token& src_loc_tk, const size_t indent,
                          const op instruction, const operand& dst,
                          const operand& src, const runtime_check& check = {})
        -> void {

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

        if (check.enabled) {
            emit_checked_add_subtract(src_loc_tk, indent, instruction, src,
                                      loaded.value, dst.type_ref().size_bytes(),
                                      constant, check);
        } else {
            emit_binary_instruction(src_loc_tk, indent, instruction, src,
                                    loaded.value, constant);
        }

        store_operation_result(
            indent, dst, loaded.address, loaded.value,
            needs_normalize(instruction, dst, src, constant));
    }

    // a missing address would leave the loop width unproven
    [[nodiscard]] auto bulk_starts() const -> std::array<access_start, 2> {
        const bulk_operation& bulk{bulk_operations_.back()};

        assert(bulk.address_count == bulk.starts.size());

        return bulk.starts;
    }

    auto call_arithmetic_helper(const token& src_loc_tk, const size_t indent,
                                const operand& dst, const operand& source,
                                const helper_result result_kind,
                                const runtime_check& division_check,
                                const runtime_check& overflow_check = {})
        -> void {

        // the first argument would overwrite a source in 'a0', which is
        // allocated last, when every other register is taken
        if (uses_register(source, "a0")) {
            throw out_of_registers_error(src_loc_tk);
        }

        const address_scope scope{*this, dst, source};

        const uint32_t live{registers().unavailable_mask()};

        const helper_routine routine{
            arithmetic_helper_routine(result_kind, overflow_check.enabled),
        };

        const std::span<const std::string_view> clobbered{
            helper_clobbers(routine),
        };

        const std::vector<std::string_view> saved{
            live_clobbered(live, clobbered, dst),
        };

        // staging registers must survive the helper call
        for (const std::string_view name : clobbered) {
            protect_register(name);
        }

        // variables and frames are based on 's0' and 's1', and the save area
        // moves sp
        assert(not uses_register(dst, "sp"));
        assert(not uses_register(source, "sp"));

        // save caller values before argument setup overwrites a0 or a1
        const size_t stack_bytes{save_registers(indent, saved)};

        load_helper_arguments(src_loc_tk, indent, dst, source);

        if (division_check.enabled) {
            check_divisor(src_loc_tk, indent, dst.type_ref().size_bytes(),
                          division_check.with_line);
        }

        assembler_.call(indent, helper_label(routine));

        if (routine == helper_routine::multiply_checked) {
            check_multiply_result(src_loc_tk, indent, dst, overflow_check);
        }

        const operand result{
            operand::reg(result_kind == helper_result::remainder ? "a1" : "a0",
                         default_type()),
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

    // a distance from the source to the destination that is not zero and below
    // the byte count jumps to the failure handler, the copy would overwrite
    // source bytes it has not read yet, the line is passed in 'a0'
    auto check_copy_overlap(const token& src_loc_tk, const size_t indent,
                            const operand& src, const operand& dst,
                            const operand& count_bytes,
                            const runtime_check& check) -> void {

        constexpr local_label passed{
            .name{"1"},
            .reference{"1f"},
        };

        comment(src_loc_tk, indent, "overlap check begin");

        const operand distance{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.sub(indent, distance.base_register(), dst.base_register(),
                       src.base_register());

        comment(src_loc_tk, indent, "the same range is not an overlap");

        assembler_.beqz(indent, distance.base_register(), passed.reference);

        comment(src_loc_tk, indent, "destination starts inside the source");

        assembler_.bgeu(indent, distance.base_register(),
                        count_bytes.base_register(), passed.reference);

        free_scratch_register(src_loc_tk, indent, distance);

        comment(src_loc_tk, indent, "failed: report and exit");

        if (check.with_line) {
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }

        branch(indent, overlap_failure_handler_label);
        assembler_.label(indent, passed.name);

        comment(src_loc_tk, indent, "overlap check end");
    }

    // jumps to the failure handler when the divisor in 'a1' is zero or the
    // dividend in 'a0' is the minimum of its width and the divisor is -1, the
    // line is passed in 'a0'
    auto check_divisor(const token& src_loc_tk, const size_t indent,
                       const size_t dividend_size_bytes, const bool with_line)
        -> void {

        const int64_t minimum{
            constant_arith::signed_minimum(dividend_size_bytes * bits_per_byte),
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

    // the register holds a sign extended result of a narrow operation, which
    // fits when sign extending its low bits gives it back
    auto check_fits_width(const token& src_loc_tk, const size_t indent,
                          const operand& value, const size_t width_bytes,
                          const runtime_check& check) -> void {

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        const size_t shift{
            register_bits_ - (width_bytes * bits_per_byte),
        };

        const operand extended{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.slli(indent, extended.base_register(), value.base_register(),
                        shift);

        assembler_.srai(indent, extended.base_register(),
                        extended.base_register(), shift);

        assembler_.beq(indent, extended.base_register(), value.base_register(),
                       fits.reference);

        free_scratch_register(src_loc_tk, indent, extended);
        fail_on_overflow(src_loc_tk, indent, check);
        assembler_.label(indent, fits.name);
    }

    // passing checks branch to 'bounds_pass_' and failing ones to
    // 'bounds_fail_', the last check branches past the handler on success so
    // failures fall through to it
    auto check_lower_bounds(const size_t indent, const std::string_view index,
                            const operand& reg_count, const bool is_last)
        -> void {

        const auto check_negative = [&](const std::string_view reg,
                                        const bool last) -> void {
            assembler_.branch_zero(indent, last ? op::bgez : op::bltz, reg,
                                   last ? bounds_pass_.reference
                                        : bounds_fail_.reference);
        };

        if (reg_count.is_empty()) {
            check_negative(index, is_last);
            return;
        }

        check_negative(index, false);

        // a negative count passes 'start + count' but spans the address space
        check_negative(reg_count.base_register(), is_last);
    }

    // after the checked multiply helper: the flag is in 'a1', the product in
    // 'a0' must fit the width of the destination
    auto check_multiply_result(const token& src_loc_tk, const size_t indent,
                               const operand& dst, const runtime_check& check)
        -> void {

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        // the product of a narrow width also fits the width
        constexpr local_label fits_width{
            .name{"1"},
            .reference{"1f"},
        };

        comment(src_loc_tk, indent, "overflow check begin");

        assembler_.beqz(indent, "a1", fits.reference);
        fail_on_overflow(src_loc_tk, indent, check);
        assembler_.label(indent, fits.name);

        const size_t width{dst.type_ref().size_bytes()};

        if (width < word_size_bytes_) {
            const size_t shift{register_bits_ - (width * bits_per_byte)};

            assembler_.slli(indent, "a1", "a0", shift);
            assembler_.srai(indent, "a1", "a1", shift);
            assembler_.beq(indent, "a1", "a0", fits_width.reference);
            fail_on_overflow(src_loc_tk, indent, check);
            assembler_.label(indent, fits_width.name);
        }

        comment(src_loc_tk, indent, "overflow check end");
    }

    // the minimum of the width has no positive counterpart
    auto check_negate(const token& src_loc_tk, const size_t indent,
                      const operand& value, const size_t width_bytes,
                      const runtime_check& check) -> void {

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        comment(src_loc_tk, indent, "overflow check begin");

        const operand minimum{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const int64_t minimum_value{
            constant_arith::signed_minimum(width_bytes * bits_per_byte),
        };

        assembler_.li(indent, minimum.base_register(), minimum_value);

        assembler_.bne(indent, value.base_register(), minimum.base_register(),
                       fits.reference);

        free_scratch_register(src_loc_tk, indent, minimum);
        fail_on_overflow(src_loc_tk, indent, check);
        assembler_.label(indent, fits.name);
        comment(src_loc_tk, indent, "overflow check end");
    }

    // a count below zero or not below the width jumps to the failure handler,
    // a negative count is a large unsigned one, the line is passed in 'a0'
    auto check_shift_count(const token& src_loc_tk, const size_t indent,
                           const operand& amount, const type& dst_type,
                           const runtime_check& check) -> void {

        constexpr local_label pass{
            .name{"1"},
            .reference{"1f"},
        };

        comment(src_loc_tk, indent, "shift check begin");

        const operand limit{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.li(indent, limit.base_register(), dst_type.size_bits());

        assembler_.bltu(indent, amount.base_register(), limit.base_register(),
                        pass.reference);

        free_scratch_register(src_loc_tk, indent, limit);

        comment(src_loc_tk, indent, "failed: report and exit");

        if (check.with_line) {
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }

        branch(indent, shift_failure_handler_label);
        assembler_.label(indent, pass.name);

        comment(src_loc_tk, indent, "shift check end");
    }

    auto check_upper_bound(const token& src_loc_tk, const size_t indent,
                           const bounds_range& range, const bool lower_checked)
        -> void {

        const operand limit{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        const std::string top{
            upper_bound_top(src_loc_tk, indent, range, limit, lower_checked),
        };

        assembler_.li(indent, limit.base_register(), range.array_count);

        if (range.allow_end) {
            assembler_.bgeu(indent, limit.base_register(), top,
                            bounds_pass_.reference);

            return;
        }

        assembler_.bltu(indent, top, limit.base_register(),
                        bounds_pass_.reference);
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
                       registers.right.base_register(), false_exit_.reference);
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

        const address_scope operand_scope{*this, right, left};

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

        assembler_.label(indent, chunk_loop_.name);

        compare_access(indent, registers, loop.width, memory_at(left_pointer),
                       memory_at(right_pointer));

        advance(indent, left_pointer, loop.width);
        advance(indent, right_pointer, loop.width);

        assembler_.bne(indent, left_pointer.base_register(),
                       end.base_register(), chunk_loop_.reference);

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

        const address_scope operand_scope{*this, right, left};

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

        assembler_.label(indent, chunk_loop_.name);

        copy_access(indent, value, loop.width, memory_at(src_pointer),
                    memory_at(dst_pointer));

        advance(indent, src_pointer, loop.width);
        advance(indent, dst_pointer, loop.width);

        assembler_.bne(indent, src_pointer.base_register(), end.base_register(),
                       chunk_loop_.reference);

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
        if (is_used("multiply_helper")) {
            assembler_.label(0, helper_label(helper_routine::multiply));
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

        // multiplies the magnitudes by shift and add, a carry out of the
        // sum, the multiplicand losing its top bit while multiplier bits
        // remain or a magnitude above the range of the sign leaves the flag
        // in a1 set, the sign is applied last
        if (is_used("multiply_checked_helper")) {
            // 'lui' of 2^31
            constexpr int sign_bit_upper{0x80000};

            assembler_.label(0, helper_label(helper_routine::multiply_checked));
            assembler_.xor_op(1, "a4", "a0", "a1");
            assembler_.srai(1, "a4", "a4", sign_shift_);
            assembler_.srai(1, "a3", "a0", sign_shift_);
            assembler_.xor_op(1, "a0", "a0", "a3");
            assembler_.sub(1, "a0", "a0", "a3");
            assembler_.srai(1, "a3", "a1", sign_shift_);
            assembler_.xor_op(1, "a1", "a1", "a3");
            assembler_.sub(1, "a1", "a1", "a3");
            assembler_.mv(1, "a2", "a0");
            assembler_.li(1, "a0", 0);
            assembler_.beqz(1, "a1", "3f");
            assembler_.label(0, "1");
            assembler_.andi(1, "a3", "a1", 1);
            assembler_.beqz(1, "a3", "2f");
            assembler_.add(1, "a0", "a0", "a2");
            assembler_.bltu(1, "a0", "a2", "5f");
            assembler_.label(0, "2");
            assembler_.srli(1, "a1", "a1", 1);
            assembler_.beqz(1, "a1", "3f");
            assembler_.bltz(1, "a2", "5f");
            assembler_.slli(1, "a2", "a2", 1);
            assembler_.j(1, "1b");
            assembler_.label(0, "3");
            assembler_.lui(1, "a3", sign_bit_upper);
            assembler_.bltz(1, "a4", "4f");
            assembler_.bgeu(1, "a0", "a3", "5f");
            assembler_.j(1, "6f");
            assembler_.label(0, "4");
            assembler_.bltu(1, "a3", "a0", "5f");
            assembler_.label(0, "6");
            assembler_.xor_op(1, "a0", "a0", "a4");
            assembler_.sub(1, "a0", "a0", "a4");
            assembler_.li(1, "a1", 0);
            assembler_.ret(1);
            assembler_.label(0, "5");
            assembler_.li(1, "a1", 1);
            assembler_.ret(1);
        }

        // divide and remainder share magnitude division and sign restoration
        if (is_used("divide_helper")) {
            // one quotient bit per step
            constexpr int divide_steps{32};

            assembler_.label(0, helper_label(helper_routine::divide));
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

        if (constant.has_value() and immediate >= immediate_min_ and
            immediate <= immediate_max_) {

            assembler_.immediate_op(indent, immediate_form(instruction),
                                    left.base_register(), left.base_register(),
                                    immediate);

            return;
        }

        // two addi are shorter than li and add
        if (constant.has_value() and arithmetic and
            immediate >= 2 * immediate_min_ and
            immediate <= 2 * immediate_max_) {

            const int64_t first{
                immediate < 0 ? immediate_min_ : immediate_max_,
            };

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
                           const bounds_range& range,
                           const bounds_check_options& options) -> void {

        const address_scope scope{*this, range.checked, range.count};

        const bounds_plan plan{
            plan_bounds_check(options, range,
                              std::numeric_limits<int32_t>::max()),
        };

        if (options.lower) {
            emit_lower_bound(src_loc_tk, indent, range, plan,
                             not options.upper);
        }

        if (options.upper) {
            comment(src_loc_tk, indent, "upper bound");

            check_upper_bound(src_loc_tk, indent, range, options.lower);
        }

        assembler_.label(indent, bounds_fail_.name);

        if (options.with_line) {
            comment(src_loc_tk, indent, "source line");
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }

        branch(indent, bounds_failure_handler_label);
        assembler_.label(indent, bounds_pass_.name);
    }

    // a sum or difference computed by 'emit_binary_instruction' must fit the
    // width of the destination, the register holds the sign extended result
    //   32 bit add: the sum is below the first operand exactly when the
    //               second operand is negative, the second is 'sum - first'
    //   32 bit sub: the difference is below the first operand exactly when
    //               the second operand is positive
    //   narrow:     the result is exact, it fits when sign extending its low
    //               bits gives it back
    auto emit_checked_add_subtract(const token& src_loc_tk, const size_t indent,
                                   const op instruction, const operand& src,
                                   const operand& value,
                                   const size_t width_bytes,
                                   const std::optional<int32_t> constant,
                                   const runtime_check& check) -> void {

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        comment(src_loc_tk, indent, "overflow check begin");

        if (width_bytes < word_size_bytes_) {
            emit_binary_instruction(src_loc_tk, indent, instruction, src, value,
                                    constant);

            check_fits_width(src_loc_tk, indent, value, width_bytes, check);

            comment(src_loc_tk, indent, "overflow check end");

            return;
        }

        // the first operand is 'result -/+ second', one register is enough
        // when the second operand is in a register of its own or small
        const bool is_register_source{
            src.is_register() and register_index(src.base_register()) !=
                                      register_index(value.base_register()),
        };

        // note: above the minimum because an add negates the constant into an
        //       'addi'

        const bool is_small_constant{
            constant.has_value() and *constant > immediate_min_ and
                *constant <= immediate_max_,
        };

        if (is_register_source or is_small_constant) {
            emit_checked_with_one_register(src_loc_tk, indent, instruction, src,
                                           value, constant, check);

            comment(src_loc_tk, indent, "overflow check end");

            return;
        }

        const operand first{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.mv(indent, first.base_register(), value.base_register());

        emit_binary_instruction(src_loc_tk, indent, instruction, src, value,
                                constant);

        const operand below{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        assembler_.slt(indent, below.base_register(), value.base_register(),
                       first.base_register());

        if (instruction == op::add) {
            assembler_.sub(indent, first.base_register(), value.base_register(),
                           first.base_register());

            assembler_.slti(indent, first.base_register(),
                            first.base_register(), 0);
        } else {
            assembler_.sub(indent, first.base_register(), first.base_register(),
                           value.base_register());

            assembler_.slt(indent, first.base_register(), "zero",
                           first.base_register());
        }

        assembler_.beq(indent, below.base_register(), first.base_register(),
                       fits.reference);

        // the scope of the operation frees the registers, the instruction
        // above may have left a temporary of its own above them
        fail_on_overflow(src_loc_tk, indent, check);
        assembler_.label(indent, fits.name);
        comment(src_loc_tk, indent, "overflow check end");
    }

    // a 32 bit sum or difference with the second operand in a register or an
    // immediate: the first operand is 'result - second' for an add and
    // 'result + second' for a subtract, the result is below it exactly when
    // the second operand is negative for an add or positive for a subtract
    auto emit_checked_with_one_register(const token& src_loc_tk,
                                        const size_t indent,
                                        const op instruction,
                                        const operand& src,
                                        const operand& value,
                                        const std::optional<int32_t> constant,
                                        const runtime_check& check) -> void {

        constexpr local_label fits{
            .name{"2"},
            .reference{"2f"},
        };

        constexpr local_label below{
            .name{"3"},
            .reference{"3f"},
        };

        constexpr local_label failed{
            .name{"4"},
            .reference{"4f"},
        };

        const bool is_add{instruction == op::add};

        emit_binary_instruction(src_loc_tk, indent, instruction, src, value,
                                constant);

        const operand first{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        if (constant.has_value()) {
            assembler_.addi(indent, first.base_register(),
                            value.base_register(),
                            is_add ? -*constant : *constant);
        } else if (is_add) {
            assembler_.sub(indent, first.base_register(), value.base_register(),
                           src.base_register());
        } else {
            assembler_.add(indent, first.base_register(), value.base_register(),
                           src.base_register());
        }

        if (constant.has_value()) {
            // the sign of the second operand is known
            const bool below_first{is_add ? *constant < 0 : *constant > 0};

            assembler_.branch(indent, below_first ? op::blt : op::bge,
                              value.base_register(), first.base_register(),
                              fits.reference);

            fail_on_overflow(src_loc_tk, indent, check);
            assembler_.label(indent, fits.name);

            return;
        }

        if (is_add) {
            assembler_.bltz(indent, src.base_register(), below.reference);
        } else {
            assembler_.bgtz(indent, src.base_register(), below.reference);
        }

        assembler_.branch(indent, op::bge, value.base_register(),
                          first.base_register(), fits.reference);

        assembler_.j(indent, failed.reference);
        assembler_.label(indent, below.name);

        assembler_.branch(indent, op::blt, value.base_register(),
                          first.base_register(), fits.reference);

        assembler_.label(indent, failed.name);
        fail_on_overflow(src_loc_tk, indent, check);
        assembler_.label(indent, fits.name);
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
                threshold >= immediate_min_ and threshold <= immediate_max_,
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

        assembler_.mv(1, "s2", "a0");
        assembler_.li(1, "a0", stderr_descriptor);
        assembler_.la(1, "a1", message_label);
        assembler_.li(1, "a2", message.size());
        emit_write_call(1);

        if (line_report_emitted_) {
            assembler_.j(1, report_line_label_);
        } else {
            line_report_emitted_ = true;
            emit_report_line_routine();
        }

        assembler_.switch_section(section::rodata);
        assembler_.label(0, message_label);
        assembler_.ascii(message);
        assembler_.switch_section(section::text);
    }

    // the lower bound, the last check of the bounds check branches past the
    // handler, see 'check_lower_bounds'
    auto emit_lower_bound(const token& src_loc_tk, const size_t indent,
                          const bounds_range& range, const bounds_plan& plan,
                          const bool is_last) -> void {

        comment_lower_bound(src_loc_tk, indent, range, plan);

        if (not plan.upper_covers_lower) {
            check_lower_bounds(indent, range.checked.base_register(),
                               plan.count_known ? operand{} : range.count,
                               is_last);
        }

        mark_lower_checked(range);
    }

    // the handler of a check that prints 'panic: NAME', with the line number
    // in a0 when 'with_line', then exits
    auto emit_named_failure_handler(const std::string_view handler_label,
                                    const std::string_view name,
                                    const bool with_line) -> void {

        constexpr std::array<int64_t, 1> newline{'\n'};

        std::string label_name{name};
        std::ranges::replace(label_name, ' ', '_');

        const std::string message_label{
            std::format(".Lbaz_{}_message", label_name),
        };

        label(0, handler_label);

        if (with_line) {
            emit_line_failure(message_label,
                              std::format("panic: {} at line ", name));

            return;
        }

        const std::string message{std::format("panic: {}", name)};

        assembler_.li(1, "a0", stderr_descriptor);
        assembler_.la(1, "a1", message_label);
        // the newline follows the message text
        assembler_.li(1, "a2", message.size() + 1);
        emit_write_call(1);

        exit(token{}, 1, operand::imm(panic_exit_code, default_type()));

        assembler_.switch_section(section::rodata);
        assembler_.label(0, message_label);
        assembler_.ascii(message);
        assembler_.data(1, newline);
        assembler_.switch_section(section::text);
    }

    // writes the line number of s2 in decimal, then exits: each power of ten
    // of '.Lbaz_decimal_places' gives a digit by repeated subtraction,
    // leading zeros are skipped and the last place always writes its digit
    auto emit_report_line_routine() -> void {
        constexpr int digit_zero{'0'};
        constexpr int newline{'\n'};

        assembler_.label(0, report_line_label_);
        assembler_.addi(1, "sp", "sp", -line_digits_bytes_);
        assembler_.mv(1, "a1", "sp");
        assembler_.li(1, "a2", 0);
        assembler_.la(1, "t0", decimal_places_label_);
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

        exit(token{}, 1, operand::imm(panic_exit_code, default_type()));

        constexpr std::array<int64_t, 10> decimal_places{
            1000000000, 100000000, 10000000, 1000000, 100000,
            10000,      1000,      100,      10,      1,
        };

        assembler_.switch_section(section::rodata);
        assembler_.align(word_size_bytes_);
        assembler_.label(0, decimal_places_label_);
        assembler_.data(word_size_bytes_, decimal_places);
        assembler_.switch_section(section::text);
    }

    // opens the image file at the first transfer and keeps its descriptor and
    // sector count in '.Lbaz_sd_card', a sector past the end, a failed open or
    // a short transfer panics
    auto emit_sector_system_call_routines(const bool read_used,
                                          const bool write_used) -> void {

        // 'AT_FDCWD', 'O_RDWR' and 'SEEK_END' of linux
        constexpr int current_directory{-100};
        constexpr int read_write{2};
        constexpr int seek_end{2};
        // the descriptor, the sector count and the 64 bit size of the file
        constexpr std::array<int64_t, 4> sd_card_words{-1, 0, 0, 0};
        constexpr int64_t count_offset{4};
        constexpr int64_t size_low_offset{8};
        constexpr int64_t size_high_offset{12};
        // the arguments and the system call are kept while the file is opened
        constexpr std::array<std::string_view, 4> saved{"a0", "a1", "a2", "a5"};
        // the file name ends with a zero byte for the system call
        constexpr std::array<int64_t, 1> name_end{0};
        // the byte offset of a sector is split into two words
        constexpr int sector_shift{std::countr_zero(card_sector_size_bytes)};

        constexpr int high_shift{
            static_cast<int>(register_bits_) - sector_shift,
        };

        assembler_rv32i& a{assembler_};

        if (read_used) {
            a.label(0, sector_read_label_);
            a.li(1, "a5", syscall_pread_);
            a.j(1, sector_transfer_label_);
        }

        if (write_used) {
            a.label(0, sector_write_label_);
            a.li(1, "a5", syscall_pwrite_);
        }

        // a5 holds the system call, a6 the return address
        a.label(0, sector_transfer_label_);
        a.mv(1, "a6", "a7");
        a.la(1, "a3", sd_card_label_);
        a.lw(1, "a4", 0, "a3");
        a.bgez(1, "a4", "2f");

        const size_t stack_bytes{save_registers(1, saved)};

        a.li(1, "a0", current_directory);
        a.la(1, "a1", sd_card_name_label_);
        a.li(1, "a2", read_write);
        a.li(1, "a3", 0);
        a.li(1, "a7", syscall_openat_);
        a.ecall(1);
        a.bltz(1, "a0", sd_card_handler_label);
        a.la(1, "a3", sd_card_label_);
        a.sw(1, "a0", 0, "a3");
        // 'llseek' writes the size of the file at a3
        a.li(1, "a1", 0);
        a.li(1, "a2", 0);
        a.addi(1, "a3", "a3", size_low_offset);
        a.li(1, "a4", seek_end);
        a.li(1, "a7", syscall_llseek_);
        a.ecall(1);
        a.bnez(1, "a0", sd_card_handler_label);
        a.la(1, "a3", sd_card_label_);
        a.lw(1, "a4", size_low_offset, "a3");
        a.lw(1, "a0", size_high_offset, "a3");
        a.srli(1, "a4", "a4", sector_shift);
        a.slli(1, "a1", "a0", high_shift);
        a.or_op(1, "a4", "a4", "a1");
        // an image of 2 TB or more has more sectors than a sector number
        // reaches
        a.srli(1, "a0", "a0", sector_shift);
        a.beqz(1, "a0", "1f");
        a.li(1, "a4", -1);
        a.label(0, "1");
        a.sw(1, "a4", count_offset, "a3");
        restore_saved_registers(1, saved, stack_bytes);
        a.label(0, "2");
        a.lw(1, "a4", count_offset, "a3");
        a.bgeu(1, "a0", "a4", sd_card_handler_label);
        // a0 the descriptor, a3 and a4 the low and high word of the offset
        a.lw(1, "a7", 0, "a3");
        a.slli(1, "a3", "a0", sector_shift);
        a.srli(1, "a4", "a0", high_shift);
        a.mv(1, "a0", "a7");
        a.mv(1, "a7", "a5");
        a.ecall(1);
        a.bne(1, "a0", "a2", sd_card_handler_label);
        a.jr(1, "a6");

        emit_named_failure_handler(sd_card_handler_label, "sd card", false);

        a.switch_section(section::data);
        a.align(word_size_bytes_);
        a.label(0, sd_card_label_);
        a.data(word_size_bytes_, sd_card_words);
        a.switch_section(section::rodata);
        a.label(0, sd_card_name_label_);
        a.ascii(sd_card_file_name());
        a.data(1, name_end);
        a.switch_section(section::text);
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

    auto extend_register(const size_t indent, const type& dst_type,
                         const operand& value) -> void {

        const size_t shift{register_bits_ - dst_type.size_bits()};

        // the register holds the value at the full width, as the comparisons
        // and the arithmetic of the narrow type read it
        assembler_.slli(indent, value.base_register(), value.base_register(),
                        shift);

        assembler_.immediate_op(indent, extend_shift_op(dst_type),
                                value.base_register(), value.base_register(),
                                shift);
    }

    // jumps to the overflow failure handler, the line is passed in 'a0'
    auto fail_on_overflow(const token& src_loc_tk, const size_t indent,
                          const runtime_check& check) -> void {

        if (check.with_line) {
            assembler_.li(indent, "a0", src_loc_tk.at_line());
        }

        branch(indent, overflow_failure_handler_label);
    }

    // a jump grown beyond 1 MiB needs a register without a live value
    [[nodiscard]] auto far_jump_register() const -> std::string_view {
        for (const std::string_view name : scratch_registers_) {
            if (not is_register_allocated(name)) {
                return name;
            }
        }

        return {};
    }

    [[nodiscard]] auto is_register_allocated(const std::string_view name) const
        -> bool {

        return registers().is_unavailable(register_mask(name));
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
            is_variables_base_reserved() and
                base == register_index(variables_base_register_),
        };

        const bool is_frame_base{
            is_frame_base_reserved() and
                base == register_index(frame_base_register()),
        };

        // other bases such as loaded pointers or bulk registers may hold any
        // address
        return is_variables_base or is_frame_base;
    }

    // the clobbered registers that hold a live value, 'live' is the mask of
    // the registers in use
    [[nodiscard]] auto
    live_clobbered(const uint32_t live,
                   const std::span<const std::string_view> clobbered,
                   const operand& dst) const -> std::vector<std::string_view> {

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

        // note: 'call_arithmetic_helper' rejects a source in a0
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

        validate_address(address);

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
                  operand::imm(factor_info.shift, default_type()),
                  runtime_check{});

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

        // restoring the saved registers would overwrite the result
        const operand kept{
            alloc_scratch_register(src_loc_tk, indent, default_type()),
        };

        copy_value(src_loc_tk, indent, kept, result);

        return kept;
    }

    auto release_bulk(const token& src_loc_tk, const size_t indent) -> void {
        free_scratch_registers(src_loc_tk, indent,
                               bulk_operations_.back().registers);

        bulk_operations_.pop_back();
    }

    // the arguments are in a0, a1 and a2
    auto reserve_call_register(const token& src_loc_tk, const size_t indent,
                               const operand& first, const operand& address,
                               const operand& count) -> operand {

        for (const operand* value : {&first, &address, &count}) {
            assert(value->is_register() and
                   value->type_ref().size_bytes() == word_size_bytes_);
        }

        assert(register_index(first.base_register()) == register_index("a0"));
        assert(register_index(address.base_register()) == register_index("a1"));
        assert(register_index(count.base_register()) == register_index("a2"));

        // selects the system call or receives the return address
        return alloc_named_register(src_loc_tk, indent, "a7", default_type());
    }

    // the byte count of 'read' and 'write' replaces the descriptor in a0
    auto reserve_io_call_register(const token& src_loc_tk, const size_t indent,
                                  const operand& dst, const operand& descriptor,
                                  const operand& address, const operand& count)
        -> operand {

        assert(dst.is_register() and
               dst.type_ref().size_bytes() == word_size_bytes_);

        assert(register_index(dst.base_register()) == register_index("a0"));

        return reserve_call_register(src_loc_tk, indent, descriptor, address,
                                     count);
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
    // address; the pointers advance together so both starts decide the width
    // of the loop
    auto set_bulk_address(const token& src_loc_tk, const size_t indent,
                          const size_t slot, const operand& address) -> void {

        bulk_operation& bulk{bulk_operations_.back()};

        bulk.starts.at(slot) = start_of(address, 1);
        ++bulk.address_count;

        address_of(src_loc_tk, indent, bulk.pointer(slot), address);
    }

    // borrowed pointers avoid a temporary address and final move for indexing
    auto set_bulk_addresses(const token& src_loc_tk, const size_t indent,
                            const operand& count, const address_emitter& first,
                            const address_emitter& second) -> void {

        first(count, bulk_operations_.back().pointer(0),
              [&](const operand& address) -> void {
                  set_bulk_address(src_loc_tk, indent, 0, address);
              });

        second(count, bulk_operations_.back().pointer(1),
               [&](const operand& address) -> void {
                   set_bulk_address(src_loc_tk, indent, 1, address);
               });
    }

    // the loop ends when its pointer reaches 'end', replacing a counter that
    // costs a decrement in every iteration, 'address_of' avoids 'li' whose
    // expansion for some values differs from the assembler's
    auto set_loop_end(const token& src_loc_tk, const size_t indent,
                      const operand& end, const operand& pointer,
                      const size_t size_bytes) -> void {

        address_of(src_loc_tk, indent, end,
                   operand::mem(pointer.base_register(), {}, 1,
                                address_offset(size_bytes), default_type()));
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
                           const loaded_destination& loaded,
                           const runtime_check& check) -> void {

        const operand amount{
            source_register(src_loc_tk, indent, count, std::nullopt),
        };

        if (check.enabled) {
            check_shift_count(src_loc_tk, indent, amount, dst.type_ref(),
                              check);
        }

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

    auto store_low_bits(const token& src_loc_tk, const size_t indent,
                        const operand& dst, const operand& value) -> void {

        const operand lowered{lower_address(src_loc_tk, indent, dst)};

        assembler_.store(indent, store_op(dst.type_ref().size_bytes()),
                         value.base_register(), lowered.displacement(),
                         lowered.base_register());
    }

    auto store_operation_result(const size_t indent, const operand& dst,
                                const operand& address, const operand& value,
                                const bool normalize) -> void {

        const size_t width{dst.type_ref().size_bytes()};

        if (dst.is_memory()) {
            assembler_.store(indent, store_op(width), value.base_register(),
                             address.displacement(), address.base_register());
        } else if (width < word_size_bytes_ and normalize) {
            extend_register(indent, dst.type_ref(), value);
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
            immediate_max_) {

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
                    const bounds_range& range, const operand& limit,
                    const bool lower_checked) -> std::string {

        const std::string_view index{range.checked.base_register()};
        const operand& reg_count{range.count};

        // a negative index is left to the lower check, which has already
        // failed it when enabled
        if (reg_count.is_empty() and not lower_checked) {
            assembler_.bltz(indent, index, bounds_pass_.reference);
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

        assembler_.bltz(indent, high.base_register(), bounds_pass_.reference);
        assembler_.bgtz(indent, high.base_register(), bounds_fail_.reference);

        return top;
    }

    auto walk_runtime_bytes(const runtime_walk& walk) -> void {
        const std::string_view count{walk.count.base_register()};

        comment(walk.src_loc_tk, walk.indent, "{}; skip if none",
                describe_loop(walk.verb, 1));

        assembler_.beqz(walk.indent, count, walk_end_.reference);

        // 'count' becomes the end of the walk, which saves a decrement in
        // every iteration
        assembler_.add(walk.indent, count, count, walk.src.base_register());

        assembler_.label(walk.indent, chunk_loop_.name);
        walk_runtime_step(walk, 1);

        assembler_.bne(walk.indent, walk.src.base_register(), count,
                       chunk_loop_.reference);

        assembler_.label(walk.indent, walk_end_.name);
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

        assembler_.beqz(walk.indent, chunks.base_register(),
                        after_chunks_.reference);

        assembler_.add(walk.indent, chunks.base_register(),
                       chunks.base_register(), walk.src.base_register());

        comment(walk.src_loc_tk, walk.indent, "{}",
                describe_loop(walk.verb, width));

        assembler_.label(walk.indent, chunk_loop_.name);
        walk_runtime_step(walk, width);

        assembler_.bne(walk.indent, walk.src.base_register(),
                       chunks.base_register(), chunk_loop_.reference);

        assembler_.label(walk.indent, after_chunks_.name);
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
            assembler_.beqz(walk.indent, count, after_head_.reference);

            walk_runtime_step(walk, 1);
            assembler_.addi(walk.indent, count, count, -1);
        }

        if ((head_size_bytes & size_t{2}) != 0) {
            assembler_.sltiu(walk.indent, chunks.base_register(), count, 2);

            assembler_.bnez(walk.indent, chunks.base_register(),
                            after_head_.reference);

            walk_runtime_step(walk, 2);
            assembler_.addi(walk.indent, count, count, -2);
        }

        assembler_.label(walk.indent, after_head_.name);
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
                            after_halfword_.reference);

            walk_runtime_step(walk, 2);
            assembler_.label(walk.indent, after_halfword_.name);
            assembler_.andi(walk.indent, count, count, 1);
        }

        comment(walk.src_loc_tk, walk.indent, "{} optional final byte",
                walk.verb);

        assembler_.beqz(walk.indent, count, walk_end_.reference);
        walk.access(1);
        assembler_.label(walk.indent, walk_end_.name);
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
        assembler_.j(indent, result_end_.reference);
        assembler_.label(indent, false_exit_.name);

        comment(src_loc_tk, indent, "mismatch: {}",
                inverted ? "true" : "false");

        assembler_.li(indent, registers.left.base_register(), inverted ? 1 : 0);
        assembler_.label(indent, result_end_.name);

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

        assembler_.label(indent, chunk_loop_.name);
        zero_access(indent, loop.width, memory_at(pointer));
        advance(indent, pointer, loop.width);

        assembler_.bne(indent, pointer.base_register(), end.base_register(),
                       chunk_loop_.reference);

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

    [[nodiscard]] static auto
    arithmetic_helper_routine(const helper_result result_kind,
                              const bool is_checked) -> helper_routine {

        if (result_kind != helper_result::product) {
            return helper_routine::divide;
        }

        if (is_checked) {
            return helper_routine::multiply_checked;
        }

        return helper_routine::multiply;
    }

    // note: 'data_element_size_error' rejects other sizes
    static auto
    assert_data_element_size([[maybe_unused]] const size_t element_size_bytes)
        -> void {

        assert(element_size_bytes == byte_size_bytes_ or
               element_size_bytes == half_size_bytes_ or
               element_size_bytes == word_size_bytes_);
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

        throw unlocated_error{std::format(
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
    [[nodiscard]] static auto helper_clobbers(const helper_routine routine)
        -> std::span<const std::string_view> {

        // argument registers are allocated last, so the helpers rarely
        // clobber a live scratch register that would need saving
        static constexpr std::array<std::string_view, 8> clobbers{
            "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6",
        };

        // '.Lbaz_multiply' changes the first five, '.Lbaz_multiply_checked'
        // also 'a4' for the sign of the product, '.Lbaz_divide' all
        constexpr size_t multiply_clobber_count{5};
        constexpr size_t checked_multiply_clobber_count{6};

        if (routine == helper_routine::divide) {
            return clobbers;
        }

        if (routine == helper_routine::multiply_checked) {
            return std::span{clobbers}.first(checked_multiply_clobber_count);
        }

        return std::span{clobbers}.first(multiply_clobber_count);
    }

    [[nodiscard]] static auto helper_label(const helper_routine routine)
        -> std::string_view {

        if (routine == helper_routine::divide) {
            return ".Lbaz_divide";
        }

        if (routine == helper_routine::multiply_checked) {
            return ".Lbaz_multiply_checked";
        }

        return ".Lbaz_multiply";
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

        std::array<int, multiplier_digit_count_> digits{
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
        -> std::array<int, multiplier_digit_count_> {

        std::array<int, multiplier_digit_count_> digits{};
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

        return *constant < -limit or *constant >= limit;
    }

    [[nodiscard]] static auto offset_by(const operand& address,
                                        const size_t offset) -> operand {

        operand moved{address};
        moved.increment_offset(address_offset(offset));
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

        return dst_size_bytes < word_size_bytes_ and
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

    static auto validate_address(const operand& address) -> void {

        assert(address.is_memory());
        assert(is_register(address.base_register()));

        assert(address.index_register().empty() or
               is_register(address.index_register()));

        // an element size beyond the address range cannot be allocated
        assert(address.index_register().empty() or
               address.scale() <= std::numeric_limits<uint32_t>::max());

        constexpr int64_t limit{std::numeric_limits<uint32_t>::max()};

        // note: '--vars' is bounded by 'max_storage_bytes' and a constant index
        //       by the array size, so an offset stays in the address range

        assert(address.displacement() >= -limit and
               address.displacement() <= limit);
    }

    // the result is written in place
    static auto validate_destination_storage(const operand& dst) -> void {

        assert(dst.is_register() or dst.is_memory());

        if (dst.is_memory()) {
            validate_address(dst);
        }
    }

    // scalar operands in a register or an address of the target
    static auto validate_operands(const token& src_loc_tk, const operand& dst,
                                  const operand& src) -> void {

        validate_scalar(src_loc_tk, dst.type_ref());
        validate_scalar(src_loc_tk, src.type_ref());
        validate_destination_storage(dst);

        if (src.is_memory()) {
            validate_address(src);
        }
    }

    static auto validate_scalar(const token& src_loc_tk, const type& value_type)
        -> void {

        if (not is_scalar(value_type)) {
            throw compile_error{src_loc_tk,
                                "RV32I requires an 8-, 16-, or 32-bit scalar"};
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
