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

#include "assembler_rv32i.hpp"
#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"

// the rv32i backend, written as llvm assembler text and a binary image
// through 'assembler_rv32i'; the targets derive from it, 'machine_rv32i_linux'
// for linux in qemu user mode and 'machine_rv32i_bare_metal'. the
// formatter sorts the functions by name, so by topic they are:
//
//   registers    'machine' allocates them, here the order
//                'scratch_registers_' and the roles in 'rv32i_target'
//   addresses    'lower_address': an operand into a base register and a
//                12-bit offset
//   operations   'binary_operation', 'emit_binary_instruction',
//                'emit_multiply' through 'multiply_by_shifts_and_adds'
//                (signed digits), 'shift_by_constant', 'shift_by_register'
//   helpers      '.Lbaz_multiply', '.Lbaz_multiply_checked' and
//                '.Lbaz_divide' for 'mul' and 'div',
//                'helper_specs_', 'call_arithmetic_helper',
//                'emit_target_routines'
//   memory       'zero', 'copy_block', 'memory_equal'
//                through 'walk_known_size' for a known size,
//                'walk_runtime_count' for a run-time count
//                (see the note before 'local_label'); 'store_bytes' as
//                immediates or a copy of a constant ('copy_constant')
//   comparisons  'compare_and_branch', 'emit_comparison_branch'
//   checks       'emit_bounds_check' with 'check_upper_bound',
//                'check_divisor', 'check_fits_width', 'check_shift_count',
//                'check_copy_overlap', 'emit_frame_capacity_check',
//                'check_stack_capacity', the overflow checks 'check_negate',
//                'check_multiply_result', 'emit_checked_add_subtract'; a
//                failed test branches to its handler, all but the frame and
//                stack checks with a stub per line for '--checks=line'
//                ('branch_to_failure'), tests of one check share a jump
//                ('failure_tests', 'fail_if', 'pass_if')
//   handlers     'emit_panic_message', 'emit_report_line_routine'
//   calls        'emit_call', 'emit_target_call' through 'call_io_routine'
//                to the routines of 'emit_io_routines' and the sector routines
//   target hooks 'emit_read_transfer', 'emit_write_transfer',
//                'emit_stderr_write' and the machine
//                virtuals 'emit_exit_routine', 'emit_sector_routines',
//                'emit_target_setup' and 'finish_target';
//                the protected section is what the targets build on
//   output       'write_assembly' writes the text and the image
class machine_rv32i : public machine {
    using op = assembler_rv32i::op;

    static constexpr size_t variables_base_past_vars_bytes_{2032};

    // note: the largest multiple of 16 an 'addi' immediate holds, 'vars'
    //       starts 2032 below 's0' so 4080 bytes of variables are in reach
    //       of a load or store, and 's0' stays aligned

    static constexpr size_t data_alignment_{16};

    //
    // code-size thresholds, each measured, see its note
    //

    // a copy or a compare of a known size up to this is unrolled, up to twice
    // this when the registers of a loop are not free ('is_copy_unrolled'), a
    // larger one loops
    static constexpr size_t unroll_threshold_bytes_{12};

    // note: measured on the tests, 'roome' and copies and compares of 1 to
    //       128 B, 12 was smaller than 16 by 229, 20 and 319 words, a compare
    //       gains most (two loads and a branch per word); below 10 a loop of a
    //       start 3 B before a word boundary can have one chunk
    //       ('plan_known_loop')

    // a constant up to this is stored as immediates when they are smaller
    // than its copy ('are_immediates_smaller'), a larger one is copied
    static constexpr size_t immediates_threshold_bytes_{16};

    // note: measured on the tests and stores of 1 to 128 B, 24 and 32 were
    //       smaller by 16 and 20 words on the tests but larger by 34 and 132 on
    //       the stores, 12 larger on both; it stays above the unroll threshold
    //       of copies, which 12 and 16 together measured smallest

    // stores run faster than the loop's 3 instructions per word, the cap only
    // bounds the code size
    static constexpr size_t zero_unroll_threshold_parts_{16};
    // see 'target_description::max_padding_gaps_zeroed_one_by_one'
    static constexpr size_t max_padding_gaps_zeroed_one_by_one_{8};

    // note: a gap takes one to three stores, the whole instance a store per
    //       word up to 64 B and a loop of 5 instructions with its head and
    //       tail beyond; measured on 2500 random types with padding, from 9
    //       gaps the whole instance was larger in 1 of 533, at 7 and 8 gaps in
    //       10 of 146

    // what an i/o or sector routine may change besides 'a0' and the 'a7' it
    // returns through, free at the call, see 'call_io_routine'
    static constexpr std::array<std::string_view, 4> io_routine_clobbered_{
        "a3",
        "a4",
        "a5",
        "a6",
    };
    // shifting a word right by this moves its sign bit to bit 0, an
    // arithmetic shift spreads it over all bits
    static constexpr int sign_shift_{31};
    // 'sp' stays 16-byte aligned, the return address slot of a frame is one
    // unit
    static constexpr size_t stack_alignment_{16};
    static constexpr int64_t frame_save_bytes_{stack_alignment_};
    // the line report of a failure writes the ten digits of a 32-bit line
    // number and a newline on the stack
    static constexpr int64_t line_digits_bytes_{16};
    // note: 11 B rounded up to 'stack_alignment_'
    static constexpr size_t byte_size_bytes_{1};
    static constexpr size_t half_size_bytes_{2};

    static constexpr std::array<std::string_view, 30> scratch_registers_{
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "s0",  "s1",  "s2",
        "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "tp",
        "gp", "ra", "a1", "a2", "a3", "a4", "a5", "a6",  "a7",  "a0",
    };

    // note: ascending t and s names keep generated code readable while argument
    //       registers stay late to avoid builtin conflicts and 'a0' stays last
    //       because syscalls overwrite it with their result

    // where unrolled accesses start: the address minus 'phase' is aligned to
    // 'alignment', which is at most a word
    struct access_start {
        size_t alignment{};
        size_t phase{};
    };

    // the registers of an active copy or comparison of arrays and where its
    // two addresses start
    struct bulk_operation {
        // the two pointers then the count, in the order of allocation
        std::array<operand, 3> registers{};
        // the type alignment of the elements
        size_t alignment{};
        std::array<access_start, 2> starts{};
        // a missing address would leave the loop width unproven
        size_t address_count{};

        [[nodiscard]] auto checked_starts() const
            -> const std::array<access_start, 2>& {

            assert(address_count == starts.size());

            return starts;
        }

        [[nodiscard]] auto count() const -> const operand& {
            return registers.back();
        }

        // 0 the first address and 1 the second
        [[nodiscard]] auto pointer(const size_t slot) const -> const operand& {
            assert(slot < starts.size());

            return registers.at(slot);
        }
    };

    // the run-time routines of 'call_arithmetic_helper': they take 'a0' and
    // 'a1' and return the product or the quotient in 'a0', the division also
    // the remainder in 'a1'
    enum class helper_routine : uint8_t { multiply, multiply_checked, divide };

    // a run-time routine of 'call_arithmetic_helper', 'clobbers' are the
    // registers a call of it changes
    struct helper_spec {
        helper_routine routine{};
        std::string_view label;
        std::span<const std::string_view> clobbers;
    };

    // note: argument registers are allocated last, so the helpers rarely
    //       clobber a live scratch register that would need saving

    static constexpr std::array<std::string_view, 5> multiply_clobbers_{
        "ra", "a0", "a1", "a2", "a3",
    };

    // note: 'a4' holds the sign of the product
    static constexpr std::array<std::string_view, 6> multiply_checked_clobbers_{
        "ra", "a0", "a1", "a2", "a3", "a4",
    };

    static constexpr std::array<std::string_view, 8> divide_clobbers_{
        "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6",
    };

    // in the order 'emit_target_routines' emits them
    static constexpr auto helper_specs_{
        std::to_array<helper_spec>({
            {
                .routine{helper_routine::multiply},
                .label{".Lbaz_multiply"},
                .clobbers{multiply_clobbers_},
            },
            {
                .routine{helper_routine::multiply_checked},
                .label{".Lbaz_multiply_checked"},
                .clobbers{multiply_checked_clobbers_},
            },
            {
                .routine{helper_routine::divide},
                .label{".Lbaz_divide"},
                .clobbers{divide_clobbers_},
            },
        }),
    };

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
        std::array<int, multiplier_digit_count_> digits{};
        // the digits build the negated multiplier
        bool negate{};
        // the highest and the lowest nonzero digit
        size_t top{};
        size_t lowest{};
    };

    // a store of constant bytes at 'offset' with 'size_bytes' of 1, 2 or 4
    struct byte_part {
        size_t offset{};
        size_t size_bytes{};
        int64_t value{};
        // zero is stored from register 'zero' and a repeated value is reused
        bool needs_load{};
    };

    // note: zero, copy and compare share the walk of a known size
    //       ('walk_known_size'), copy and compare that of a run-time count
    //       ('walk_runtime_count'); each operation gives only the access it
    //       emits, its registers and, for a known size, when it unrolls
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

    // a numeric local label as defined and as a branch refers to it; '1f'
    // means the next '1:' in the emitted text, so the numbers repeat across
    // operations under one rule: a numeric label is referenced only inside
    // the operation that defines it, and no other operation emitted between
    // a reference and its label defines one, e.g. no run-time check is
    // emitted inside a walk
    struct local_label {
        std::string_view name;
        std::string_view reference;
    };

    // the local labels of the bulk operations, the run-time checks have
    // theirs below, other operations define theirs where they use them
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

    // the labels of a run-time check of more than one test
    // ('failure_tests'): the failing tests branch to the jump to the handler
    // at 'check_failed_', or back to it at 'check_failed_before_', the
    // passing ones past it to 'check_passed_'
    static constexpr local_label check_failed_{
        .name{"1"},
        .reference{"1f"},
    };
    static constexpr local_label check_failed_before_{
        .name{"1"},
        .reference{"1b"},
    };
    static constexpr local_label check_passed_{
        .name{"2"},
        .reference{"2f"},
    };

    // the tests of a run-time check that fail to 'handler' or pass it, see
    // 'fail_if' and 'pass_if': a check of one test branches to the handler,
    // more share one jump to it, which a far handler grows once
    struct failure_tests {
        failure_tests(const failure_handler& handler_in, const size_t count)
            : handler{handler_in}, remaining{count}, shared{count > 1} {}

        std::reference_wrapper<const failure_handler> handler;
        size_t remaining{};
        bool shared{};
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
        std::function_ref<void(size_t)> access;
    };

    // the addresses of a known size walked together, 'access' emits one access
    // of 'width' bytes at an operand per address and 'verb' names it in
    // comments
    struct known_walk {
        token src_loc_tk;
        size_t indent{};
        std::string_view verb;
        std::span<const operand> addresses;
        std::span<const access_start> starts;
        std::function_ref<void(size_t, std::span<const operand>)> access;
    };

    // the registers a comparison loads both sides into
    struct compare_registers {
        operand left;
        operand right;
        // a register result holds the left value and saves the final copy
        bool left_is_result{};
    };

    // the routine a program calls to exit, the target emits its body
    static constexpr std::string_view exit_label_{".Lbaz_exit"};
    // the shared tail of the handlers that report a line, and its powers of
    // ten
    static constexpr std::string_view report_line_label_{".Lbaz_report_line"};
    static constexpr std::string_view decimal_places_label_{
        ".Lbaz_decimal_places",
    };
    // the line and the exit code a handler passes to 'report_line_label_'
    static constexpr std::string_view report_line_register_{"s2"};
    static constexpr std::string_view report_exit_code_register_{"s3"};

    // the scratch of a far jump to code that does not return, a failure
    // handler or the exit routine: it reads no scratch register, so the
    // register is dead there
    static constexpr std::string_view no_return_jump_register_{"t0"};

    assembler_rv32i assembler_;

  public:
    // 'options.binary_file_name' receives the image of the resolved output
    machine_rv32i(const builtin_type_set& builtins, const source_files& files,
                  const target_options& options)
        : machine_rv32i{builtins, files, options, rv32i_target()} {}

    //
    // overridden methods
    //

    auto add_subtract(const emit_site site, const arithmetic_operator operation,
                      const operand& dst, const operand& src,
                      const runtime_check& check) -> void override {

        assert(operation == arithmetic_operator::add or
               operation == arithmetic_operator::subtract);

        binary_operation(
            site, operation == arithmetic_operator::add ? op::add : op::sub,
            dst, src, check);
    }

    auto address_of(const emit_site site, const operand& dst,
                    const operand& address) -> void override {

        assert(dst.is_register() or dst.is_memory());
        assert(dst.type_ref().size_bytes() == word_size_bytes_);

        const operation_scope scope{*this, dst, address};
        const operand value{working_register(site, dst)};

        const operand lowered{
            lower_address(site, address, value),
        };

        // a nonzero residual offset still needs an add, a distinct base a
        // copy
        if (lowered.displacement() != 0) {
            assembler_.addi(site.indent, value.register_name(),
                            lowered.base_register(), lowered.displacement());
        } else if (value.register_name() != lowered.base_register()) {
            assembler_.mv(site.indent, value.register_name(),
                          lowered.base_register());
        }

        if (dst.is_memory()) {
            copy_value(site, dst, value);
        }
    }

    auto bitwise(const emit_site site, const arithmetic_operator operation,
                 const operand& dst, const operand& src) -> void override {

        assert(operation == arithmetic_operator::bit_and or
               operation == arithmetic_operator::bit_or or
               operation == arithmetic_operator::bit_xor);

        // note: the front end rejects a 'bool' as an arithmetic operand; a
        //       source wider than the destination, from the argument of a
        //       conversion, is normalized by 'binary_operation'

        assert(not dst.type_ref().is_bool() and not src.type_ref().is_bool());

        op instruction{op::xor_op};

        if (operation == arithmetic_operator::bit_and) {
            instruction = op::and_op;
        } else if (operation == arithmetic_operator::bit_or) {
            instruction = op::or_op;
        }

        binary_operation(site, instruction, dst, src);
    }

    auto branch(const size_t indent, const std::string_view target)
        -> void override {

        assembler_.resolved_jump(indent, target, far_jump_register());
    }

    // a call saves the registers allocated now, the registers of the arguments
    // included, so the bound is not below what the call pushes; the callee
    // pushes 'leaf_stack_bytes' more without a check of its own
    auto check_stack_capacity(const emit_site site) -> void override {
        assert(is_stack_bounded());

        const size_t saved_count{registers_saved_by_call().size()};

        const size_t call_bytes{
            saved_registers_size_bytes(saved_count) + frame_save_bytes_ +
                leaf_stack_bytes(),
        };

        comment(site, "stack capacity check begin");

        const operation_scope scope{*this};

        const operand limit{
            alloc_scratch_register(site, default_type()),
        };

        const operand lowest{
            alloc_scratch_register(site, default_type()),
        };

        assembler_.li(site.indent, limit.register_name(),
                      stack_limit_address());

        comment(site, "lowest stack pointer the call reaches");

        assembler_.addi(site.indent, lowest.register_name(), "sp",
                        -static_cast<int64_t>(call_bytes));

        branch_to_failure(site, op::bltu, lowest.register_name(),
                          limit.register_name(), stack_overflow_failure_);

        comment(site, "stack capacity check end");
    }

    // the operation scope ends on return, before the caller frees its registers
    auto compare_and_branch(const emit_site site, const operand& lhs,
                            const operand& rhs, const comparison_action& action)
        -> void override {

        operation_scope scope{*this, lhs, rhs};
        scope.protect(action.destination);

        assert_scalar(lhs.type_ref());
        assert_scalar(rhs.type_ref());

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
                assembler_rv32i::fits_immediate(threshold),
        };

        const std::optional<int64_t> immediate{
            use_immediate ? std::optional<int64_t>{threshold} : std::nullopt,
        };

        const std::array<operand, 2> sides{
            comparison_operands(site, lhs, rhs, use_immediate,
                                action.destination),
        };

        const operand& left{sides.at(0)};
        const operand& right{sides.at(1)};

        // branch-only comparisons do not need a materialized boolean
        if (action.destination.is_empty()) {
            emit_comparison_branch(site.indent, operation, left, right, action);
            return;
        }

        const operand value{
            comparison_result_register(site, action.destination, lhs, rhs, left,
                                       right),
        };

        const std::string_view result{value.register_name()};

        emit_boolean_result(site.indent, operation, result, left, right,
                            immediate);

        // memory results require a store, register results are in place
        if (action.destination.is_memory()) {
            copy_value(site, action.destination, value);
        }

        // some callers request both a stored boolean and a branch
        if (not action.target.empty()) {
            emit_branch(site.indent, action.branch_on_true ? op::bne : op::beq,
                        result, "zero", action.target);
        }
    }

    auto copy_block(const emit_site site, const operand& dst,
                    const operand& src, const size_t size_bytes,
                    const size_t alignment) -> void override {

        // the pointers of a loop advance together, unrolled accesses can
        // follow where each address is within its word
        const std::array<access_start, 2> starts{
            start_of(src, alignment),
            start_of(dst, alignment),
        };

        copy_from_starts(site, dst, src, size_bytes, starts);
    }

    auto copy_elements(const emit_site site, const size_t element_size_bytes,
                       const std::function_ref<void(const operand&)> emit_count,
                       const copy_request& request) -> void override {

        const operation_scope bulk_scope{*this};

        bulk_operation bulk{
            begin_array_operation(site, "source", "destination",
                                  request.alignment),
        };

        load_bulk_addresses(site, bulk, request.src, request.dst);

        emit_count(bulk.count());
        count_to_bytes(site, bulk, element_size_bytes);

        if (request.overlap.enabled) {
            check_copy_overlap(site, bulk.pointer(0), bulk.pointer(1),
                               bulk.count());
        }

        copy_runtime_count(site, bulk);
    }

    auto copy_value(const emit_site site, const operand& dst,
                    const operand& src) -> void override {

        validate_operands(dst, src);

        assert(not src.is_empty());

        // a copy onto itself, e.g. of a parameter to its argument, changes
        // nothing
        if (dst.is_same_address(src) and
            dst.type_ref().is_same(src.type_ref())) {

            return;
        }

        const operation_scope scope{*this, dst, src};

        const std::optional<int32_t> constant{
            narrowed_immediate(src, dst.type_ref()),
        };

        // known constants are truncated and extended before emission
        if (constant.has_value()) {
            store_constant_result(site, dst, *constant);
            return;
        }

        if (shares_address_base(dst, src)) {
            copy_through_shared_base(site, dst, src);
            return;
        }

        operand value{dst};

        if (not dst.is_register()) {
            value =
                src.is_register()
                    ? src
                    : scope.alloc_load_register(site, default_type(), dst, src);
        }

        load_into(site, value, src);

        if (dst.is_memory()) {
            store_low_bits(site, dst, value);
            return;
        }

        if (register_needs_extension(dst.type_ref(), src)) {
            extend_register(site.indent, dst.type_ref(), value);
        }
    }

    auto divide(const emit_site site, const arithmetic_operator operation,
                const operand& dst, const operand& divisor,
                const runtime_check& check) -> void override {

        assert(operation == arithmetic_operator::divide or
               operation == arithmetic_operator::remainder);

        validate_operands(dst, divisor);

        const operand result{
            operand::reg(operation == arithmetic_operator::remainder ? "a1"
                                                                     : "a0",
                         default_type()),
        };

        call_arithmetic_helper(site, dst, divisor, helper_routine::divide,
                               result, check);
    }

    auto
    equal_elements(const emit_site site, const size_t element_size_bytes,
                   const std::function_ref<void(const operand&)> emit_count,
                   const address_emitter& lhs, const address_emitter& rhs,
                   const size_t alignment, const operand& dst)
        -> void override {

        const operation_scope bulk_scope{*this};

        bulk_operation bulk{
            begin_array_operation(site, "lhs", "rhs", alignment),
        };

        load_bulk_addresses(site, bulk, lhs, rhs);

        emit_count(bulk.count());

        {
            // scaling the count must not pick the result registers
            const operation_scope scope{*this, dst};

            count_to_bytes(site, bulk, element_size_bytes);
        }

        compare_runtime_count(site, bulk, dst);
    }

    // known size: the addresses are memory operands, small sizes unrolled and
    // larger ones in a loop; the result of the request is 1 when every access
    // matched, 0 at the first mismatch, its 'inverted' swaps them
    auto memory_equal(const emit_site site, const size_t size_bytes,
                      const operand& lhs, const operand& rhs,
                      const equality_request& request) -> void override {

        // note: compared sizes are limited by the variables that hold the data
        assert(size_bytes <= std::numeric_limits<uint32_t>::max());

        // unrolled accesses can follow where each address is within its word
        const std::array<access_start, 2> starts{
            start_of(lhs, request.alignment),
            start_of(rhs, request.alignment),
        };

        const operand& result{request.dst};

        operation_scope scope{*this, rhs, lhs};
        scope.protect(result);

        const std::array<operand, 2> in_use{lhs, rhs};

        const compare_registers registers{
            alloc_compare_registers(site, result, in_use),
        };

        const auto access{
            [&](const size_t width, const std::span<const operand> at) -> void {
                compare_access(site.indent, registers, width, at.front(),
                               at.back());
            },
        };

        // a load pair and a branch per access beat the pointer setup of a loop
        // for small sizes
        walk_known_size(
            {
                .src_loc_tk{site.src_loc_tk},
                .indent{site.indent},
                .verb{"compare"},
                .addresses{in_use},
                .starts{starts},
                .access{access},
            },
            size_bytes, is_copy_unrolled(size_bytes));

        write_compare_result(site, result, registers, request.inverted);
    }

    auto return_function(const size_t indent) -> void override {
        assembler_.lw(indent, "ra", 0, "sp");
        assembler_.addi(indent, "sp", "sp", frame_save_bytes_);
        assembler_.ret(indent);
    }

    auto shift(const emit_site site, const arithmetic_operator operation,
               const operand& dst, const operand& count,
               const runtime_check& check) -> void override {

        assert(operation == arithmetic_operator::shift_left or
               operation == arithmetic_operator::shift_right);

        validate_operands(dst, count);

        const std::optional<int32_t> constant{immediate_value(count)};

        const size_t bits{dst.type_ref().size_bits()};

        // note: the front end rejects a constant count outside the width and
        //       drops a count of 0

        assert(not constant.has_value() or
               (*constant > 0 and std::cmp_less(*constant, bits)));

        const uint32_t shift_count{
            static_cast<uint32_t>(constant.value_or(0)),
        };

        const operation_scope scope{*this, dst, count};

        const loaded_destination loaded{
            load_destination(site, dst),
        };

        if (constant.has_value()) {
            shift_by_constant(site.indent, operation, dst, loaded, shift_count,
                              bits);

            return;
        }

        shift_by_register(site, operation, dst, count, loaded, check);
    }

    // few bytes are stored with immediates when that takes no more code than
    // their copy from read-only data ('copy_constant')
    auto store_bytes(
        const emit_site site, const std::string_view bytes, const operand& dst,
        const size_t alignment,
        const std::function_ref<std::string(std::string_view)> add_constant)
        -> void override {

        const operation_scope scope{*this, dst};

        const access_start dst_start{start_of(dst, alignment)};

        // the widest access the start of 'dst' allows
        const size_t width{
            aligned_width_at(word_size_bytes_, 0, std::span{&dst_start, 1}),
        };

        const std::vector<byte_part> parts{split_bytes(bytes, dst_start)};

        // the copy is estimated with the alignment 'width', so its parts can
        // be narrower than the stored immediates
        const std::array<access_start, 2> estimated_starts{
            access_start{
                .alignment{width},
                .phase{},
            },
            dst_start,
        };

        // note: estimated with the word parts of the copy below, a copy inside
        //       a word won where it was a longer loop or saved less code than
        //       its read-only data took ('are_immediates_smaller' counts code
        //       and an unrolled copy)

        const size_t copy_part_count{
            aligned_parts(bytes.size(), estimated_starts).size(),
        };

        if (are_immediates_smaller(parts, bytes.size(), copy_part_count)) {
            comment_aligned_parts(site, "store", bytes.size(),
                                  std::span{&dst_start, 1});

            store_byte_parts(site, parts, dst, bytes.size());

            return;
        }

        copy_constant(site, bytes, dst, dst_start, add_constant);
    }

    auto unary(const emit_site site, const arithmetic_operator operation,
               const operand& dst, const runtime_check& check)
        -> void override {

        assert(operation == arithmetic_operator::negate or
               operation == arithmetic_operator::complement);

        // note: the front end rejects a 'bool' as an arithmetic operand
        assert(not dst.type_ref().is_bool());

        assert_scalar(dst.type_ref());

        assert(dst.is_register() or dst.is_memory());

        const operation_scope scope{*this, dst};

        const loaded_destination loaded{
            load_destination(site, dst),
        };

        if (operation == arithmetic_operator::negate) {
            if (check.enabled) {
                check_negate(site, loaded.value, dst.type_ref().size_bytes());
            }

            assembler_.sub(site.indent, loaded.value.register_name(), "zero",
                           loaded.value.register_name());

            // note: a checked negation is not of the minimum, the negated
            //       value of an extended register is extended

            store_operation_result(site.indent, dst, loaded.address,
                                   loaded.value, not check.enabled);

            return;
        }

        assembler_.xori(site.indent, loaded.value.register_name(),
                        loaded.value.register_name(), -1);

        store_operation_result(site.indent, dst, loaded.address, loaded.value,
                               false);
    }

    // the binary image is written together with the assembly source
    auto write_assembly(std::ostream& os) -> void override {
        // note: 'main' names the image of every rv32i target
        const std::string& file_name{options().binary_file_name};

        assert(not file_name.empty());

        std::ofstream binary{file_name, std::ios::binary};

        // the file did not open, or e.g. a full disk left the image incomplete
        if (binary) {
            assembler_.write_resolved(os, binary);
            binary.close();
        }

        if (not binary) {
            throw unlocated_error{
                std::format("cannot write file {}", in_quotes(file_name))};
        }
    }

    auto zero(const emit_site site, const operand& dst, const size_t size_bytes,
              const size_t alignment) -> void override {

        assert(size_bytes != 0);

        const access_start start{start_of(dst, alignment)};

        const operation_scope scope{*this, dst};

        const std::span<const access_start> starts{&start, 1};

        const auto access{
            [&](const size_t width, const std::span<const operand> at) -> void {
                zero_access(site.indent, width, at.front());
            },
        };

        // note: a part is at most a word, so a larger size has more parts and
        //       its parts are not listed

        const bool is_unrolled{
            size_bytes <= zero_unroll_threshold_parts_ * word_size_bytes_ and
                aligned_parts(size_bytes, starts).size() <=
                    zero_unroll_threshold_parts_,
        };

        walk_known_size(
            {
                .src_loc_tk{site.src_loc_tk},
                .indent{site.indent},
                .verb{"zero"},
                .addresses{std::span{&dst, 1}},
                .starts{starts},
                .access{access},
            },
            size_bytes, is_unrolled);
    }

    //
    // statics
    //

    // what a callee pushes without a check of its own: a helper call saves
    // the registers the helper changes, and a failed check of the helper,
    // made while they are saved, writes the digits of its line
    [[nodiscard]] static auto leaf_stack_bytes() -> size_t {
        size_t most_clobbers{};
        for (const helper_spec& spec : helper_specs_) {
            most_clobbers = std::max(most_clobbers, spec.clobbers.size());
        }

        const size_t helper_save_bytes{
            align_storage_size(most_clobbers * word_size_bytes_,
                               stack_alignment_),
        };

        return helper_save_bytes + line_digits_bytes_;
    }

  protected:
    // a target that differs from 'rv32i_target' passes its own table
    machine_rv32i(const builtin_type_set& builtins, const source_files& files,
                  const target_options& options,
                  const target_description& target)
        : machine{builtins, files, options, target, assembler_} {}

    static constexpr size_t word_size_bytes_{4};
    static constexpr size_t register_bits_{
        std::numeric_limits<uint32_t>::digits,
    };
    // the routines of 'read' and 'write', see 'emit_io_routines'
    static constexpr std::string_view read_label_{".Lbaz_read"};
    static constexpr std::string_view write_label_{".Lbaz_write"};
    // the sector routines, see 'emit_sector_routines': a routine takes the
    // sector in 'a0', the address in 'a1' and the size in 'a2', returns
    // through 'a7', which it may change, and changes only 'a0' and
    // 'io_routine_clobbered_' else
    static constexpr std::string_view sector_read_label_{".Lbaz_sector_read"};
    static constexpr std::string_view sector_write_label_{
        ".Lbaz_sector_write",
    };

    //
    // overridden methods
    //

    // the operation scope frees its temporaries before the end comment
    auto emit_bounds_check(const emit_site site, const bounds_range& range,
                           const bounds_plan& plan) -> void override {

        // note: array lengths are limited by the variables that hold the
        //       arrays, the line is loaded with 'li' into a 32-bit register

        assert(range.array_count <= std::numeric_limits<uint32_t>::max() and
               site.src_loc_tk.get().at_line() <=
                   std::numeric_limits<uint32_t>::max());

        operation_scope scope{*this, range.checked};

        if (range.count) {
            scope.protect(*range.count);
        }

        // an index of 32 bits is below 2^31 and never reaches a count of 2^31
        // or more, a negative one is left to the lower check
        const bool upper_compared{
            plan.upper and
                (range.count or
                 range.array_count <=
                     static_cast<size_t>(std::numeric_limits<int32_t>::max())),
        };

        size_t test_count{plan.sign_tested.size()};

        if (upper_compared) {
            test_count += upper_bound_test_count(range, plan.lower_checked);
        }

        failure_tests tests{bounds_failure_, test_count};

        for (const operand& value : plan.sign_tested) {
            fail_if(site, tests, op::blt, value.register_name(), "zero");
        }

        if (not plan.upper) {
            return;
        }

        if (not upper_compared) {
            comment(site, "upper bound: a 32-bit index is below the count");
            return;
        }

        comment(site, "upper bound");

        check_upper_bound(site, range, plan.lower_checked, tests);
    }

    auto emit_call(const size_t indent, const std::string_view label)
        -> void override {

        assembler_.call(indent, label);
    }

    auto emit_entry() -> void override {
        assembler_.option_norvc();
        assembler_.option_norelax();
        assembler_.add_separator_newline();
        assembler_.switch_section(section::text);
        assembler_.globl("_start");
        assembler_.label(0, "_start");
        assembler_.add_separator_newline();
    }

    auto emit_frame_capacity_check(const emit_site site,
                                   const operand& frame_address,
                                   const std::string_view frame_size_label)
        -> void override {

        const operand start{
            alloc_scratch_register(site, default_type()),
        };

        const operand last_start{
            alloc_scratch_register(site, default_type()),
        };

        address_of(site, start, frame_address);

        // the highest start at which the frame fits, compared unsigned since
        // an address can be 2 GiB or more
        // note: a frame fits in '--vars' when compiled, so the end of 'vars'
        //       less the frame size does not wrap
        assembler_.la(site.indent, last_start.register_name(),
                      variables_end_label_, frame_size_label);

        branch_to_failure(site, op::bltu, last_start.register_name(),
                          start.register_name(), frame_overflow_failure_);
    }

    // a 'noinline' body saves the return address below 'sp'
    auto emit_frame_entry() -> void override {
        assembler_.addi(1, "sp", "sp", -frame_save_bytes_);
        assembler_.sw(1, "ra", 0, "sp");
    }

    // 'a0' is the descriptor and receives the result, 'a1' is the address and
    // 'a2' the count; a routine returns through 'a7' and changes only
    // 'io_routine_clobbered_' else, see 'call_io_routine'
    // note: the label '5' of each routine returns 'bad_descriptor_', a
    //       transfer may use the labels '1' to '4'
    auto emit_io_routines(const bool read_used, const bool write_used)
        -> void override {

        if (read_used) {
            // note: 'bnez' compares the descriptor with 'stdin_descriptor_'

            static_assert(stdin_descriptor_ == 0);

            assembler_.label(0, read_label_);
            assembler_.bnez(1, "a0", "5f");
            emit_read_transfer();
            emit_bad_descriptor_return();
        }

        if (not write_used) {
            return;
        }

        assembler_.label(0, write_label_);
        // 1 and 2 less 1 are below 2 unsigned, 0 and a negative descriptor
        // are not
        assembler_.addi(1, "a5", "a0", -stdout_descriptor_);
        assembler_.li(1, "a4", write_served_count_);
        assembler_.bgeu(1, "a5", "a4", "5f");
        emit_write_transfer();
        emit_bad_descriptor_return();
    }

    // a constant factor that 'multiply' did not resolve shifts and adds
    // unless checked, else the helper multiplies
    auto emit_multiply(const emit_site site, const operand& product,
                       const operand& factor,
                       [[maybe_unused]] const factor_use use,
                       const runtime_check& check) -> void override {

        validate_operands(product, factor);

        const std::optional<uint64_t> bits{immediate_bits(factor)};

        // shifts and adds do not report the bits they lose, the helper does
        if (bits and not check.enabled) {
            const size_t width_bits{product.type_ref().size_bits()};

            const constant_factor factor_info{
                classify_factor(*bits, width_bits),
            };

            multiply_by_shifts_and_adds(
                site, product, factor,
                static_cast<uint32_t>(factor_info.multiplier), width_bits);

            return;
        }

        const helper_routine routine{
            check.enabled ? helper_routine::multiply_checked
                          : helper_routine::multiply,
        };

        call_arithmetic_helper(site, product, factor, routine,
                               operand::reg("a0", default_type()), check);
    }

    auto emit_no_return_jump(const size_t indent, const std::string_view target)
        -> void override {

        assembler_.resolved_jump(indent, target, no_return_jump_register_);
    }

    auto emit_panic_message(const std::string_view message_label,
                            const std::string_view message) -> void override {

        assembler_.li(1, "a0", stderr_descriptor_);
        assembler_.la(1, "a1", message_label);
        assembler_.li(1, "a2", message.size());
        emit_stderr_write(1);
    }

    // writes the line number of 'report_line_register_' in decimal, then exits
    // with the code in 'report_exit_code_register_': each power of ten of
    // '.Lbaz_decimal_places' gives a digit by repeated subtraction, leading
    // zeros are skipped and the last place always writes its digit
    auto emit_report_line_routine() -> void override {
        constexpr int digit_zero{'0'};
        constexpr int newline{'\n'};

        assembler_.addi(1, "sp", "sp", -line_digits_bytes_);
        assembler_.mv(1, "a1", "sp");
        assembler_.li(1, "a2", 0);
        assembler_.la(1, "t0", decimal_places_label_);
        assembler_.label(0, "1");
        assembler_.lw(1, "t1", 0, "t0");
        assembler_.li(1, "t2", 0);
        assembler_.label(0, "2");
        assembler_.bltu(1, report_line_register_, "t1", "3f");
        assembler_.sub(1, report_line_register_, report_line_register_, "t1");
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
        assembler_.li(1, "a0", stderr_descriptor_);
        emit_stderr_write(1);

        exit({token{}, 1},
             make_register_operand(report_exit_code_register_, default_type()));

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

    // the functions are routines, see 'emit_io_routines'; the sector functions
    // take the sector in 'a0' where 'read' has the descriptor, there is no
    // result
    auto emit_target_call(const emit_site site, const target_call function)
        -> void override {

        const operation_scope scope{*this};

        // the arguments are in the registers of 'target_calls', allocated by
        // 'call_target'; 'a7' receives the return address
        reserve_named_register(site, "a7");

        call_io_routine(site.indent, routine_label(function));
    }

    // the arithmetic helpers
    auto emit_target_routines() -> void override {
        for (const helper_spec& spec : helper_specs_) {
            if (not is_used(spec.label)) {
                continue;
            }

            assembler_.label(0, spec.label);
            emit_helper_body(spec.routine);
        }
    }

    auto emit_variables_base_load() -> void override {
        const std::string_view base{variables_base_register()};

        assembler_.la(0, base, variables_label_);

        assembler_.addi(0, base, base,
                        static_cast<int64_t>(variables_base_past_vars_bytes_));
    }

    auto finish_target() -> void override {
        // the report counts after resolving because grown jumps take more
        // instructions, a failing build writes nothing
        assembler_.resolve_jumps();

        check_address_range(assembler_.memory_end_address());
    }

    [[nodiscard]] auto make_register_operand(const std::string_view name,
                                             const type& value_type) const
        -> operand override {

        assert_scalar(value_type);

        assert(is_register(name));

        return operand::reg(name, value_type);
    }

    [[nodiscard]] auto register_index_of(const std::string_view name) const
        -> std::optional<size_t> override {

        return assembler_rv32i::register_number(name);
    }

    // 'saved' as given to 'save_call_registers'
    auto restore_call_registers(const size_t indent,
                                const std::span<const std::string_view> saved)
        -> void override {

        const size_t stack_bytes{saved_registers_size_bytes(saved.size())};

        for (const auto [index, name] : std::views::enumerate(saved)) {
            assembler_.lw(indent, name,
                          static_cast<size_t>(index) * word_size_bytes_, "sp");
        }

        if (stack_bytes != 0) {
            assembler_.addi(indent, "sp", "sp", stack_bytes);
        }
    }

    // an aligned stack area is allocated only when a register is saved
    auto save_call_registers(const size_t indent,
                             const std::span<const std::string_view> saved)
        -> void override {

        const size_t stack_bytes{saved_registers_size_bytes(saved.size())};

        if (stack_bytes != 0) {
            assembler_.addi(indent, "sp", "sp",
                            -static_cast<int64_t>(stack_bytes));
        }

        for (const auto [index, name] : std::views::enumerate(saved)) {
            assembler_.sw(indent, name,
                          static_cast<size_t>(index) * word_size_bytes_, "sp");
        }
    }

    //
    // virtual methods
    //

    // the transfer of 'read' from the checked descriptor in 'a0', which
    // receives the count, see 'emit_io_routines'
    virtual auto emit_read_transfer() -> void = 0;

    // writes the count in 'a2' at 'a1' to 'stderr_descriptor_' in 'a0' for a
    // failure, which needs no check of the descriptor; changes what the
    // routines of 'emit_io_routines' change, 'a7' included
    virtual auto emit_stderr_write(const size_t indent) -> void = 0;

    // the transfer of 'write' to the checked descriptor in 'a0', which
    // receives the count, see 'emit_io_routines'
    virtual auto emit_write_transfer() -> void = 0;

    //
    // class methods
    //

    [[nodiscard]] auto assembler() -> assembler_rv32i& { return assembler_; }

    // the i/o routines return through 'a7' and change only
    // 'io_routine_clobbered_' besides 'a0', which are free at the call, so
    // like a system call it saves nothing
    auto call_io_routine(const size_t indent, const std::string_view label)
        -> void {

        // note: the clobbered registers follow 'a1' and 'a2', which the call
        //       needs, in the allocation order, so they are free whenever the
        //       call is made

        assert(not is_any_held(io_routine_clobbered_));

        assembler_.call(indent, label, "a7");
        mark_used(label);
    }

    //
    // statics
    //

    // see 'target_description'
    [[nodiscard]] static constexpr auto rv32i_target() -> target_description {
        return {
            .address_size_bytes{word_size_bytes_},
            .data_alignment{data_alignment_},
            .max_storage_bytes{assembler_rv32i::address_space_bytes},
            .has_indexed_addressing{false},

            // note: 'scale_index' shifts by any power of two, a type stays
            //       below the 4 GiB address space

            .max_index_scale_bytes{assembler_rv32i::address_space_bytes},
            .max_padding_gaps_zeroed_one_by_one{
                max_padding_gaps_zeroed_one_by_one_,
            },
            // note: a load and store machine counts in a register
            .memory_counter_pressure_count{},
            .string_alignment{word_size_bytes_},
            .stack_limit_address{},
            .frame_base_register{"s1"},
            .variables_base_register{"s0"},
            // the last of the saved registers, no built-in names it
            .slot_register{"s11"},
            .variables_base_past_vars_bytes{variables_base_past_vars_bytes_},
            .scratch_registers{scratch_registers_},
            .target_calls{
                .arguments{"a0", "a1", "a2"},
                .result{"a0"},
            },
            .routines{
                .exit{exit_label_},
                .report_line{report_line_label_},
                .read{read_label_},
                .write{write_label_},
                .sector_read{sector_read_label_},
                .sector_write{sector_write_label_},
            },
            .report_line_register{report_line_register_},
            .report_exit_code_register{report_exit_code_register_},
        };
    }

  private:
    //
    // overridden methods
    //

    [[nodiscard]] auto register_display_name(const size_t index) const
        -> std::string_view override {

        return assembler_rv32i::register_names.at(index);
    }

    //
    // class methods
    //

    auto advance(const size_t indent, const operand& pointer,
                 const size_t size_bytes) -> void {

        assembler_.addi(indent, pointer.register_name(),
                        pointer.register_name(), size_bytes);
    }

    [[nodiscard]] auto
    alloc_compare_registers(const emit_site site, const operand& result,
                            const std::span<const operand> in_use)
        -> compare_registers {

        const bool left_is_result{can_hold_left_value(result, in_use)};

        const operand left{
            left_is_result ? result
                           : alloc_scratch_register(site, default_type()),
        };

        const operand right{
            alloc_scratch_register(site, default_type()),
        };

        comment(site, "{}: left value/result, {}: right value",
                left.register_name(), right.register_name());

        comment(site, "stop at first mismatch");

        return {
            .left{left},
            .right{right},
            .left_is_result{left_is_result},
        };
    }

    // in a helper body: 'dst' is 'src' negated when 'sign' is -1 and 'src'
    // when it is 0, e.g. a magnitude or a result with its sign restored
    auto apply_sign(const std::string_view dst, const std::string_view src,
                    const std::string_view sign) -> void {

        assembler_.xor_op(1, dst, src, sign);
        assembler_.sub(1, dst, dst, sign);
    }

    // the bulk registers of an array copy or comparison of elements aligned
    // to 'alignment', the roles of the pointers name them in the comment,
    // e.g. 'source' and 'destination'
    auto begin_array_operation(const emit_site site,
                               const std::string_view first_role,
                               const std::string_view second_role,
                               const size_t alignment) -> bulk_operation {

        bulk_operation bulk{};
        bulk.alignment = alignment;

        // argument-order allocation keeps pointer and count names easy to
        // follow
        for (operand& reg : bulk.registers) {
            reg = alloc_scratch_register(site, default_type());
        }

        comment(site, "{}: {}, {}: {}, {}: count",
                bulk.pointer(0).register_name(), first_role,
                bulk.pointer(1).register_name(), second_role,
                bulk.count().register_name());

        return bulk;
    }

    auto binary_operation(const emit_site site, const op instruction,
                          const operand& dst, const operand& src,
                          const runtime_check& check = {}) -> void {

        validate_operands(dst, src);

        const std::optional<int32_t> constant{
            narrowed_immediate(src, dst.type_ref()),
        };

        assert_folded(instruction, dst, constant);

        if (constant.has_value() and
            yields_constant(instruction, *constant,
                            dst.type_ref().size_bytes())) {

            store_constant_result(site, dst, *constant);
            return;
        }

        const operation_scope scope{*this, dst, src};

        const loaded_destination loaded{
            load_destination(site, dst),
        };

        if (check.enabled) {
            emit_checked_add_subtract(site, instruction, src, loaded.value,
                                      dst.type_ref().size_bytes(), constant);
        } else {
            emit_binary_instruction(site, instruction, src, loaded.value,
                                    constant);
        }

        // note: a checked narrow result passed 'check_fits_width', so the
        //       register already holds it extended

        // note: a source wider than the destination, from the wrapping
        //       argument of a conversion, leaves bits outside its width

        const bool wider_source{
            src.type_ref().size_bytes() > dst.type_ref().size_bytes(),
        };

        store_operation_result(
            site.indent, dst, loaded.address, loaded.value,
            not check.enabled and
                (needs_normalize(instruction) or wider_source));
    }

    // a failed check branches to the handler, or with the line to the stub
    // of its line ('failure_target'); grown when out of reach, a far jump
    // takes 'no_return_jump_register_', the handler does not return
    auto branch_to_failure(const emit_site site, const op failed,
                           const std::string_view rs1,
                           const std::string_view rs2,
                           const failure_handler& handler) -> void {

        const std::string target{failure_target(handler, site.src_loc_tk)};

        assembler_.resolved_branch(site.indent, failed, rs1, rs2, target,
                                   no_return_jump_register_);
    }

    // 'result' is the register of 'routine' that 'dst' receives, 'check' the
    // overflow check of 'multiply_checked' or the division check of 'divide'
    auto call_arithmetic_helper(const emit_site site, const operand& dst,
                                const operand& source,
                                const helper_routine routine,
                                const operand& result,
                                const runtime_check& check) -> void {

        // the first argument would overwrite a source in 'a0', which is
        // allocated last, when every other register is taken
        if (source.uses_register("a0")) {
            throw out_of_registers_error(site.src_loc_tk);
        }

        operation_scope scope{*this, dst, source};

        const helper_spec& spec{helper_spec_of(routine)};

        mark_used(spec.label);

        // note: not 'machine::with_saved_registers': the live registers
        //       include the operands the scope protects, and the saved ones
        //       are restored before the result reaches a memory destination

        const std::vector<std::string_view> saved{
            live_clobbered(spec.clobbers, dst),
        };

        // the arguments, the checks and the restore use these by name, so no
        // scratch register is one of them
        for (const std::string_view name : spec.clobbers) {
            scope.protect(make_register_operand(name, default_type()));
        }

        // note: variables and frames are based on 's0' and 's1', and the save
        //       area moves 'sp'

        assert(not dst.uses_register("sp"));
        assert(not source.uses_register("sp"));

        // save caller values before argument setup overwrites 'a0' or 'a1'
        save_call_registers(site.indent, saved);

        load_helper_arguments(site, dst, source);

        if (check.enabled and routine == helper_routine::divide) {
            check_divisor(site, dst.type_ref().size_bytes());
        }

        assembler_.call(site.indent, spec.label);

        if (routine == helper_routine::multiply_checked) {
            check_multiply_result(site, dst);
        }

        // the destination is not restored so it can retain the result
        if (dst.is_register()) {
            copy_value(site, dst, result);
            restore_call_registers(site.indent, saved);
            return;
        }

        const operand kept{
            preserved_helper_result(site, result, saved),
        };

        restore_call_registers(site.indent, saved);

        // memory destinations need their original address registers back
        copy_value(site, dst, kept);
    }

    // a check named 'name' that fails to the handler when 'failed' holds for
    // 'value' and 'constant', loaded into a scratch register
    auto check_against_constant(const emit_site site,
                                const std::string_view name, const op failed,
                                const std::string_view value,
                                const int64_t constant,
                                const failure_handler& handler) -> void {

        comment(site, "{} check begin", name);

        const operation_scope scope{*this};

        const operand constant_register{
            alloc_scratch_register(site, default_type()),
        };

        assembler_.li(site.indent, constant_register.register_name(), constant);

        branch_to_failure(site, failed, value,
                          constant_register.register_name(), handler);

        comment(site, "{} check end", name);
    }

    // a distance from the source to the destination that is not zero and below
    // the byte count jumps to the failure handler, the copy would overwrite
    // source bytes it has not read yet
    auto check_copy_overlap(const emit_site site, const operand& src,
                            const operand& dst, const operand& count_bytes)
        -> void {

        comment(site, "overlap check begin");

        const operation_scope scope{*this};

        const operand distance{
            alloc_scratch_register(site, default_type()),
        };

        assembler_.sub(site.indent, distance.register_name(),
                       dst.register_name(), src.register_name());

        comment(site, "the same range is not an overlap");

        assembler_.beqz(site.indent, distance.register_name(),
                        check_passed_.reference);

        comment(site, "destination starts inside the source");

        branch_to_failure(site, op::bltu, distance.register_name(),
                          count_bytes.register_name(), overlap_failure_);

        assembler_.label(site.indent, check_passed_.name);

        comment(site, "overlap check end");
    }

    // jumps to the failure handler when the divisor in 'a1' is zero or the
    // dividend in 'a0' is the minimum of its width and the divisor is -1
    auto check_divisor(const emit_site site, const size_t dividend_size_bytes)
        -> void {

        const int64_t minimum{
            constant_arith::signed_minimum(dividend_size_bytes * bits_per_byte),
        };

        comment(site, "division check begin");

        comment(site, "zero divisor");

        failure_tests tests{division_failure_, 3};
        // note: 3 for the two 'fail_if' and the 'pass_if' below

        fail_if(site, tests, op::beq, "a1", "zero");

        comment(site, "minimum divided by -1 overflows");

        assembler_.li(site.indent, "a2", -1);
        pass_if(site.indent, tests, op::bne, "a1", "a2");
        assembler_.li(site.indent, "a2", minimum);

        fail_if(site, tests, op::beq, "a0", "a2");

        comment(site, "division check end");
    }

    // the register holds a sign extended result of a narrow operation, which
    // fits when sign extending its low bits gives it back; 'extended' is a
    // register the check may change
    auto check_fits_width(const emit_site site, const std::string_view value,
                          const size_t width_bytes,
                          const std::string_view extended) -> void {

        sign_extend(site.indent, extended, value, width_bytes * bits_per_byte);

        branch_to_failure(site, op::bne, extended, value, overflow_failure_);
    }

    // after the checked multiply helper: the flag is in 'a1', the product in
    // 'a0' must fit the width of the destination
    auto check_multiply_result(const emit_site site, const operand& dst)
        -> void {

        comment(site, "overflow check begin");

        branch_to_failure(site, op::bne, "a1", "zero", overflow_failure_);

        const size_t width{dst.type_ref().size_bytes()};

        // the product of a narrow width also fits the width, 'a1' is free
        if (width < word_size_bytes_) {
            check_fits_width(site, "a0", width, "a1");
        }

        comment(site, "overflow check end");
    }

    // the minimum of the width has no positive counterpart
    auto check_negate(const emit_site site, const operand& value,
                      const size_t width_bytes) -> void {

        const int64_t minimum{
            constant_arith::signed_minimum(width_bytes * bits_per_byte),
        };

        check_against_constant(site, "overflow", op::beq, value.register_name(),
                               minimum, overflow_failure_);
    }

    // a count below zero or not below the width jumps to the failure handler,
    // a negative count is a large unsigned one
    auto check_shift_count(const emit_site site, const operand& amount,
                           const type& dst_type) -> void {

        const int64_t width_bits{static_cast<int64_t>(dst_type.size_bits())};

        check_against_constant(site, "shift", op::bgeu, amount.register_name(),
                               width_bits, shift_failure_);
    }

    // fails when the checked value or the end of a range reaches the array
    // count, with 'allow_end' when it passes it
    auto check_upper_bound(const emit_site site, const bounds_range& range,
                           const bool lower_checked, failure_tests& tests)
        -> void {

        const operand limit{
            alloc_scratch_register(site, default_type()),
        };

        const std::string top{
            upper_bound_top(site, range, limit, lower_checked, tests),
        };

        assembler_.li(site.indent, limit.register_name(), range.array_count);

        // unchecked below, a negative index is compared signed and passes, it
        // is left to the lower check as on x86_64; the end of a range is
        // compared unsigned, 'upper_bound_top' passed a negative one
        const bool compared_unsigned{lower_checked or range.count};

        if (range.allow_end) {
            fail_if(site, tests, compared_unsigned ? op::bltu : op::blt,
                    limit.register_name(), top);

            return;
        }

        fail_if(site, tests, compared_unsigned ? op::bgeu : op::bge, top,
                limit.register_name());
    }

    // accesses widen after the first only when a start inside a word was
    // peeled, other sequences keep the uncommented widths of the alignment
    auto comment_aligned_parts(const emit_site site,
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

        comment(site, "{} {}: {}, widest aligned access at each offset", verb,
                describe_parts(size_bytes, starts), describe_starts(starts));
    }

    // names the parts of a known-size loop
    auto comment_known_loop(const emit_site site, const std::string_view verb,
                            const known_loop& loop,
                            const std::span<const access_start> starts)
        -> void {

        const std::string head{
            loop.head_size_bytes == 0
                ? std::string{}
                : std::format(" after a {} B head", loop.head_size_bytes),
        };

        comment(site, "{} loop of {}-byte accesses{}: {}", verb, loop.width,
                head, describe_starts(starts));
    }

    // names what decided the width of a pointer loop
    auto comment_pointer_loop(const emit_site site, const loop_start& start,
                              const std::array<access_start, 2>& starts,
                              const size_t alignment) -> void {

        if (start.head_size_bytes != 0) {
            comment(site, "{}-byte accesses after a {} B head: {}", start.width,
                    start.head_size_bytes, describe_starts(starts));

            return;
        }

        if (start.width > alignment) {
            comment(site,
                    "{}-byte accesses: both addresses {}-byte aligned, type "
                    "{}-byte aligned",
                    start.width, start.width, alignment);

            return;
        }

        comment(site,
                "{}-byte accesses: type {}-byte aligned, addresses not proven "
                "more aligned",
                start.width, alignment);
    }

    // loads both values and leaves for the false exit on a mismatch
    auto compare_access(const size_t indent, const compare_registers& registers,
                        const size_t width, const operand& left,
                        const operand& right) -> void {

        assembler_.load(indent, unsigned_load_op(width),
                        registers.left.register_name(), left.displacement(),
                        left.base_register());

        assembler_.load(indent, unsigned_load_op(width),
                        registers.right.register_name(), right.displacement(),
                        right.base_register());

        assembler_.bne(indent, registers.left.register_name(),
                       registers.right.register_name(), false_exit_.reference);
    }

    // run-time count: the addresses are pointer registers and 'count' is a
    // byte count, 'result' is 1 when every access matched, else 0
    auto compare_runtime_count(const emit_site site, const bulk_operation& bulk,
                               const operand& result) -> void {

        const operand& left{bulk.pointer(0)};
        const operand& right{bulk.pointer(1)};
        const operand& count{bulk.count()};

        operation_scope scope{*this, right, left};
        scope.protect(result);

        const std::array<operand, 3> in_use{left, right, count};

        const compare_registers registers{
            alloc_compare_registers(site, result, in_use),
        };

        const operand left_at{memory_at(left)};
        const operand right_at{memory_at(right)};

        const auto access{
            [&](const size_t width) -> void {
                compare_access(site.indent, registers, width, left_at,
                               right_at);
            },
        };

        {
            // the walk's own registers are freed before the result is written
            const operation_scope walk_scope{*this};

            walk_runtime_count(
                {
                    .src_loc_tk{site.src_loc_tk},
                    .indent{site.indent},
                    .verb{"compare"},
                    .src{left},
                    .dst{right},
                    .count{count},
                    .access{access},
                },
                bulk);
        }

        write_compare_result(site, result, registers, false);
    }

    // 'source' in a register of the comparison width
    auto comparison_operand(const emit_site site, const operand& source,
                            const operand& other, const type& width_type,
                            const operand& dst) -> operand {

        // matching register representations need no conversion
        if (source.is_register() and source.type_ref().is_same(width_type)) {
            return source;
        }

        // zero has the same representation at every supported width
        if (immediate_value(source) == 0) {
            return operand::reg("zero", width_type);
        }

        // note: a destination register is allocated, never 'zero'
        assert(not dst.is_register() or dst.register_name() != "zero");

        // the output can hold an input unless doing so destroys the other
        // value or an address still needed to load it
        const bool reuse_destination{
            dst.is_register() and not other.uses_register(dst.register_name()),
        };

        // preserve the comparison width rather than narrowing to bool
        const operand value{
            reuse_destination ? operand::reg(dst.register_name(), width_type)
                              : alloc_scratch_register(site, width_type),
        };

        copy_value(site, value, source);

        return value;
    }

    // both sides in registers of the comparison width, the right one is a zero
    // register when 'use_immediate' compares against an immediate
    [[nodiscard]] auto
    comparison_operands(const emit_site site, const operand& lhs,
                        const operand& rhs, const bool use_immediate,
                        const operand& dst) -> std::array<operand, 2> {

        if (shares_address_base(lhs, rhs)) {
            return comparison_operands_through_shared_base(site, lhs, rhs);
        }

        const operand left{
            comparison_operand(site, lhs, rhs, lhs.type_ref(), dst),
        };

        // the immediate is encoded in the instruction, so the register is
        // never read
        const operand right{
            use_immediate
                ? operand::reg("zero", lhs.type_ref())
                : comparison_operand(site, rhs, left, lhs.type_ref(), dst),
        };

        return {left, right};
    }

    // both addresses are 'base + upper + low', so one 'lui' and 'add' serve
    // both loads and the base register ends up holding the left value
    [[nodiscard]] auto comparison_operands_through_shared_base(
        const emit_site site, const operand& lhs, const operand& rhs)
        -> std::array<operand, 2> {

        // note: the front end compares a 'bool' only with a 'bool' and
        //       rejects a right side wider than the left, so the load of the
        //       right side extends it as the left side reads it

        assert(not register_needs_extension(lhs.type_ref(), rhs));

        const operand left{
            alloc_scratch_register(site, lhs.type_ref()),
        };

        const operand right{
            alloc_scratch_register(site, lhs.type_ref()),
        };

        comment(site, "operands share base {}", left.register_name());

        const operand left_at{lower_address(site, lhs, left)};

        load_into(site, right, rebased(rhs, left_at));
        load_into(site, left, left_at);

        return {left, right};
    }

    // prefer the output register or a temporary the comparison owns
    auto comparison_result_register(const emit_site site, const operand& dst,
                                    const operand& lhs, const operand& rhs,
                                    const operand& left, const operand& right)
        -> operand {

        if (dst.is_register()) {
            return dst;
        }

        // an operand the comparison loaded into a scratch register is ours to
        // overwrite after it, 'zero' stands for a zero or an immediate
        if (not lhs.is_register() and left.register_name() != "zero") {
            return left;
        }

        if (not rhs.is_register() and right.register_name() != "zero") {
            return right;
        }

        // both operands are live inputs or zero
        return alloc_scratch_register(site, default_type());
    }

    auto copy_access(const size_t indent, const operand& value,
                     const size_t width, const operand& from, const operand& to)
        -> void {

        assembler_.load(indent, unsigned_load_op(width), value.register_name(),
                        from.displacement(), from.base_register());

        assembler_.store(indent, store_op(width), value.register_name(),
                         to.displacement(), to.base_register());
    }

    // 'bytes' from a read-only constant that starts as far into a word as
    // 'dst', so the copy takes words also at an address inside a word
    auto copy_constant(
        const emit_site site, const std::string_view bytes, const operand& dst,
        const access_start& dst_start,
        const std::function_ref<std::string(std::string_view)> add_constant)
        -> void {

        const operand pointer{
            alloc_scratch_register(site, default_type()),
        };

        // note: read-only constants are word aligned, the zero bytes before
        //       the constant bring its first byte to the phase of 'dst'

        const std::string laid_out{
            std::string(dst_start.phase, '\0') + std::string{bytes},
        };

        assembler_.la(site.indent, pointer.register_name(),
                      add_constant(laid_out));

        const std::array<access_start, 2> copy_starts{
            access_start{
                .alignment{word_size_bytes_},
                .phase{dst_start.phase},
            },
            dst_start,
        };

        const operand constant{offset_by(memory_at(pointer), dst_start.phase)};

        copy_from_starts(site, dst, constant, bytes.size(), copy_starts);
    }

    // 'copy_block' where 'starts' tells where 'src' and then 'dst' are within
    // a word
    auto copy_from_starts(const emit_site site, const operand& dst,
                          const operand& src, const size_t size_bytes,
                          const std::array<access_start, 2>& starts) -> void {

        if (size_bytes == 0) {
            return;
        }

        // note: a constant count is within its array, and an array is within
        //       the 32-bit address range

        assert(size_bytes <= std::numeric_limits<uint32_t>::max());

        const operation_scope scope{*this, dst, src};

        const operand value{
            alloc_scratch_register(site, default_type()),
        };

        const std::array<operand, 2> addresses{src, dst};

        const auto access{
            [&](const size_t width, const std::span<const operand> at) -> void {
                copy_access(site.indent, value, width, at.front(), at.back());
            },
        };

        // direct offsets avoid two pointer temporaries for ordinary small
        // copies
        walk_known_size(
            {
                .src_loc_tk{site.src_loc_tk},
                .indent{site.indent},
                .verb{"copy"},
                .addresses{addresses},
                .starts{starts},
                .access{access},
            },
            size_bytes, is_copy_unrolled(size_bytes));
    }

    // run-time count: the addresses are pointer registers and 'count' is a
    // byte count
    auto copy_runtime_count(const emit_site site, const bulk_operation& bulk)
        -> void {

        const operand& src{bulk.pointer(0)};
        const operand& dst{bulk.pointer(1)};
        const operand& count{bulk.count()};

        const operation_scope scope{*this, dst, src};

        const operand value{
            alloc_scratch_register(site, default_type()),
        };

        const operand from{memory_at(src)};
        const operand to{memory_at(dst)};

        const auto access{
            [&](const size_t width) -> void {
                copy_access(site.indent, value, width, from, to);
            },
        };

        walk_runtime_count(
            {
                .src_loc_tk{site.src_loc_tk},
                .indent{site.indent},
                .verb{"copy"},
                .src{src},
                .dst{dst},
                .count{count},
                .access{access},
            },
            bulk);
    }

    // both addresses are 'base + upper + low', so one 'lui' and 'add' serve
    // the load and the store
    auto copy_through_shared_base(const emit_site site, const operand& dst,
                                  const operand& src) -> void {

        const operand base{
            alloc_scratch_register(site, default_type()),
        };

        const operand value{
            alloc_scratch_register(site, default_type()),
        };

        comment(site, "source and destination share base {}",
                base.register_name());

        const operand src_at{lower_address(site, src, base)};

        load_into(site, value, src_at);
        store_low_bits(site, rebased(dst, src_at), value);
    }

    // the element count of 'bulk' becomes its byte count
    auto count_to_bytes(const emit_site site, const bulk_operation& bulk,
                        const size_t element_size_bytes) -> void {

        comment(site, "{}: elements to bytes ({} bytes/element)",
                bulk.count().register_name(), element_size_bytes);

        scale_index(site, bulk.count(), element_size_bytes);
    }

    // the label '5' of an i/o routine returns 'bad_descriptor_'
    auto emit_bad_descriptor_return() -> void {
        assembler_.label(0, "5");
        assembler_.li(1, "a0", bad_descriptor_);
        assembler_.jr(1, "a7");
    }

    // applies 'instruction' to 'left' with an immediate when it fits
    auto emit_binary_instruction(const emit_site site, const op instruction,
                                 const operand& src, const operand& left,
                                 const std::optional<int32_t> constant)
        -> void {

        const bool arithmetic{instruction == op::add or instruction == op::sub};

        const int64_t immediate{
            instruction == op::sub ? -int64_t{constant.value_or(0)}
                                   : int64_t{constant.value_or(0)},
        };

        if (constant.has_value() and
            assembler_rv32i::fits_immediate(immediate)) {

            assembler_.immediate_op(site.indent, immediate_form(instruction),
                                    left.register_name(), left.register_name(),
                                    immediate);

            return;
        }

        // two addi are shorter than li and add
        if (constant.has_value() and arithmetic and
            immediate >= 2 * assembler_rv32i::immediate_min and
            immediate <= 2 * assembler_rv32i::immediate_max) {

            const int64_t first{
                immediate < 0 ? assembler_rv32i::immediate_min
                              : assembler_rv32i::immediate_max,
            };

            assembler_.addi(site.indent, left.register_name(),
                            left.register_name(), first);

            assembler_.addi(site.indent, left.register_name(),
                            left.register_name(), immediate - first);

            return;
        }

        const operand right{
            source_register(site, src, constant),
        };

        assembler_.register_op(site.indent, instruction, left.register_name(),
                               left.register_name(), right.register_name());
    }

    // sets 'result' to 1 when the comparison holds and to 0 otherwise
    auto emit_boolean_result(const size_t indent,
                             const comparison_operator operation,
                             const std::string_view result, const operand& left,
                             const operand& right,
                             const std::optional<int64_t> immediate) -> void {

        if (is_equality(operation)) {
            emit_equality_result(indent, result, left, right, immediate,
                                 operation == comparison_operator::not_equal);

            return;
        }

        emit_less_than(indent, operation, result, left, right, immediate);

        // 'slt' answers '<', against a threshold '>' and '>=' are its
        // complement
        const bool complement{
            immediate.has_value()
                ? (operation == comparison_operator::greater_equal or
                   operation == comparison_operator::greater)
                : complements_less_than(operation),
        };

        if (complement) {
            assembler_.xori(indent, result, result, 1);
        }
    }

    // a conditional branch comparing 'first' and 'second'
    auto emit_branch(const size_t indent, const op code,
                     const std::string_view first,
                     const std::string_view second,
                     const std::string_view target) -> void {

        assembler_.resolved_branch(indent, code, first, second, target,
                                   far_jump_register());
    }

    // a sum or difference computed by 'emit_binary_instruction' must fit the
    // width of the destination, the register holds the sign extended result
    //   32 bit: 'emit_checked_with_one_register', a second operand that is not
    //           in a register of its own or a small immediate is loaded into
    //           one first
    //   narrow: the result is exact, it fits when sign extending its low bits
    //           gives it back
    auto emit_checked_add_subtract(const emit_site site, const op instruction,
                                   const operand& src, const operand& value,
                                   const size_t width_bytes,
                                   const std::optional<int32_t> constant)
        -> void {

        // the first operand is 'result -/+ second', one register is enough
        // when the second operand is in a register of its own or small
        const bool is_register_source{
            src.is_register() and src.register_name() != value.register_name(),
        };

        // note: the negation fits too, one of the two 'addi' takes it

        const bool is_small_constant{
            constant.has_value() and
                assembler_rv32i::fits_immediate(*constant) and
                assembler_rv32i::fits_immediate(-int64_t{*constant}),
        };

        comment(site, "overflow check begin");

        if (width_bytes < word_size_bytes_) {
            emit_binary_instruction(site, instruction, src, value, constant);

            const operand extended{
                alloc_scratch_register(site, default_type()),
            };

            check_fits_width(site, value.register_name(), width_bytes,
                             extended.register_name());
        } else if (is_register_source or is_small_constant) {
            emit_checked_with_one_register(site, instruction, src, value,
                                           constant);
        } else {
            // a memory source, a constant past the immediate or the value's
            // own register gets a register of its own
            const operand second{
                alloc_scratch_register(site, default_type()),
            };

            load_operand(site, second, src, constant);

            emit_checked_with_one_register(site, instruction, second, value,
                                           constant);
        }

        comment(site, "overflow check end");
    }

    // a 32 bit sum or difference with the second operand in a register or an
    // immediate: the first operand is 'result - second' for an add and
    // 'result + second' for a subtract, the result is below it exactly when
    // the second operand is negative for an add or positive for a subtract
    auto emit_checked_with_one_register(
        const emit_site site, const op instruction, const operand& src,
        const operand& value, const std::optional<int32_t> constant) -> void {

        constexpr local_label below{
            .name{"3"},
            .reference{"3f"},
        };

        const bool is_add{instruction == op::add};

        // a constant in a register is added as a register
        const std::optional<int32_t> immediate{
            src.is_register() ? std::nullopt : constant,
        };

        emit_binary_instruction(site, instruction, src, value, immediate);

        const operand first{
            alloc_scratch_register(site, default_type()),
        };

        if (immediate) {
            assembler_.addi(site.indent, first.register_name(),
                            value.register_name(),
                            is_add ? -*immediate : *immediate);
        } else if (is_add) {
            assembler_.sub(site.indent, first.register_name(),
                           value.register_name(), src.register_name());
        } else {
            assembler_.add(site.indent, first.register_name(),
                           value.register_name(), src.register_name());
        }

        if (constant.has_value()) {
            // the sign of the second operand is known
            const bool below_first{is_add ? *constant < 0 : *constant > 0};

            branch_to_failure(site, below_first ? op::bge : op::blt,
                              value.register_name(), first.register_name(),
                              overflow_failure_);

            return;
        }

        if (is_add) {
            assembler_.bltz(site.indent, src.register_name(), below.reference);
        } else {
            assembler_.bgtz(site.indent, src.register_name(), below.reference);
        }

        assembler_.branch(site.indent, op::bge, value.register_name(),
                          first.register_name(), check_passed_.reference);

        // note: one jump to the handler for both signs, which a far handler
        //       grows once

        assembler_.label(site.indent, check_failed_before_.name);
        jump_to_failure(site, overflow_failure_);
        assembler_.label(site.indent, below.name);

        assembler_.branch(site.indent, op::bge, value.register_name(),
                          first.register_name(),
                          check_failed_before_.reference);

        assembler_.label(site.indent, check_passed_.name);
    }

    auto emit_comparison_branch(const size_t indent,
                                const comparison_operator operation,
                                const operand& left, const operand& right,
                                const comparison_action& action) -> void {

        // note: a comparison without a boolean result is a branch
        assert(not action.target.empty());

        const bool inverted{not action.branch_on_true};

        // equality and inequality share one branch pair
        if (is_equality(operation)) {
            const bool branch_on_unequal{
                inverted != (operation == comparison_operator::not_equal),
            };

            emit_branch(indent, branch_on_unequal ? op::bne : op::beq,
                        left.register_name(), right.register_name(),
                        action.target);

            return;
        }

        // ordered comparisons use signed blt/bge, swapping for > and <=
        const bool swapped{swaps_operands(operation)};

        const bool branch_on_greater_equal{
            inverted != complements_less_than(operation),
        };

        emit_branch(indent, branch_on_greater_equal ? op::bge : op::blt,
                    swapped ? right.register_name() : left.register_name(),
                    swapped ? left.register_name() : right.register_name(),
                    action.target);
    }

    // divide and remainder share magnitude division and sign restoration
    auto emit_divide_body() -> void {
        assembler_.beqz(1, "a1", "5f");
        assembler_.srai(1, "a4", "a0", sign_shift_);
        assembler_.srai(1, "a3", "a1", sign_shift_);
        apply_sign("a0", "a0", "a4");
        apply_sign("a1", "a1", "a3");
        assembler_.xor_op(1, "a3", "a3", "a4");
        assembler_.li(1, "a2", 0);
        assembler_.li(1, "a5", register_bits_);
        // note: one quotient bit per step
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
        apply_sign("a0", "a0", "a3");
        apply_sign("a1", "a2", "a4");
        assembler_.ret(1);
        assembler_.label(0, "5");

        // without '--checks=division' a zero divisor exits like its panic
        exit({token{}, 1},
             operand::imm(division_failure_.exit_code, default_type()));
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

    auto emit_helper_body(const helper_routine routine) -> void {
        if (routine == helper_routine::multiply) {
            emit_multiply_body();
            return;
        }

        if (routine == helper_routine::multiply_checked) {
            emit_multiply_checked_body();
            return;
        }

        emit_divide_body();
    }

    auto emit_less_than(const size_t indent,
                        const comparison_operator operation,
                        const std::string_view result, const operand& left,
                        const operand& right,
                        const std::optional<int64_t> immediate) -> void {

        // encodable thresholds avoid materializing a constant register
        if (immediate.has_value()) {
            assembler_.slti(indent, result, left.register_name(), *immediate);
            return;
        }

        const bool swapped{swaps_operands(operation)};

        assembler_.slt(indent, result,
                       swapped ? right.register_name() : left.register_name(),
                       swapped ? left.register_name() : right.register_name());
    }

    auto emit_multiply_body() -> void {
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

    // multiplies the magnitudes by shift and add, a carry out of the sum, the
    // multiplicand losing its top bit while multiplier bits remain or a
    // magnitude above the range of the sign leaves the flag in 'a1' set, the
    // sign is applied last
    auto emit_multiply_checked_body() -> void {
        // 'lui' of 2^31
        constexpr int sign_bit_upper{0x80000};

        assembler_.xor_op(1, "a4", "a0", "a1");
        assembler_.srai(1, "a4", "a4", sign_shift_);
        assembler_.srai(1, "a3", "a0", sign_shift_);
        apply_sign("a0", "a0", "a3");
        assembler_.srai(1, "a3", "a1", sign_shift_);
        apply_sign("a1", "a1", "a3");
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
        apply_sign("a0", "a0", "a4");
        assembler_.li(1, "a1", 0);
        assembler_.ret(1);
        assembler_.label(0, "5");
        assembler_.li(1, "a1", 1);
        assembler_.ret(1);
    }

    // the result replaces the value in 'loaded', a partial sum goes through a
    // scratch register
    //   x * 10  =>  t = x << 2; x = t + x; x = x << 1
    auto emit_shift_add_sequence(const emit_site site,
                                 const loaded_destination& loaded,
                                 const digit_sequence& sequence) -> void {

        // a single digit needs only the final shift
        const operand partial{
            sequence.lowest == sequence.top
                ? operand{}
                : alloc_scratch_register(site, default_type()),
        };

        std::string_view shifted{loaded.value.register_name()};
        int pending_shift{};

        for (size_t bit{sequence.top}; bit != sequence.lowest;) {
            --bit;
            ++pending_shift;

            if (sequence.digits.at(bit) == 0) {
                continue;
            }

            assembler_.slli(site.indent, partial.register_name(), shifted,
                            pending_shift);

            const std::string_view sum{
                bit == sequence.lowest ? loaded.value.register_name()
                                       : partial.register_name(),
            };

            assembler_.register_op(
                site.indent, sequence.digits.at(bit) < 0 ? op::sub : op::add,
                sum, partial.register_name(), loaded.value.register_name());

            shifted = partial.register_name();
            pending_shift = 0;
        }

        // the zero bits below the lowest digit
        if (sequence.lowest != 0) {
            assembler_.slli(site.indent, loaded.value.register_name(),
                            loaded.value.register_name(), sequence.lowest);
        }

        if (sequence.negate) {
            assembler_.sub(site.indent, loaded.value.register_name(), "zero",
                           loaded.value.register_name());
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
            return left.register_name();
        }

        // nonzero small constants fit directly in xori
        if (immediate.has_value()) {
            assembler_.xori(indent, result, left.register_name(), *immediate);
            return result;
        }

        // note: an operand that is not tested directly is in a register other
        //       than 'zero'

        assert(left.register_name() != "zero" and
               right.register_name() != "zero");

        assembler_.xor_op(indent, result, left.register_name(),
                          right.register_name());

        return result;
    }

    auto extend_register(const size_t indent, const type& dst_type,
                         const operand& value) -> void {

        // note: no conversion makes a 'bool', a 'bool' register is only
        //       loaded from a 'bool' with 'lbu'

        assert(not dst_type.is_bool());

        // the register holds the value at the full width, as the comparisons
        // and the arithmetic of the narrow type read it
        sign_extend(indent, value.register_name(), value.register_name(),
                    dst_type.size_bits());
    }

    // a check of one test branches to the handler; of more, the last passing
    // test branches past the shared jump to the handler, the others to it
    auto fail_if(const emit_site site, failure_tests& tests, const op failed,
                 const std::string_view rs1, const std::string_view rs2)
        -> void {

        assert(tests.remaining > 0);

        --tests.remaining;

        if (not tests.shared) {
            branch_to_failure(site, failed, rs1, rs2, tests.handler.get());
            return;
        }

        if (tests.remaining > 0) {
            assembler_.branch(site.indent, failed, rs1, rs2,
                              check_failed_.reference);

            return;
        }

        assembler_.branch(site.indent, assembler_rv32i::inverse_branch(failed),
                          rs1, rs2, check_passed_.reference);

        assembler_.label(site.indent, check_failed_.name);
        jump_to_failure(site, tests.handler.get());
        assembler_.label(site.indent, check_passed_.name);
    }

    // a jump grown beyond 1 MiB needs a register without a live value
    [[nodiscard]] auto far_jump_register() const -> std::string_view {
        return first_unused_scratch_register().value_or(std::string_view{});
    }

    // a copy or a compare of two arrays is unrolled up to the threshold, and
    // up to twice it when the loop's two pointers and its end are not free
    [[nodiscard]] auto is_copy_unrolled(const size_t size_bytes) const -> bool {
        if (size_bytes <= unroll_threshold_bytes_) {
            return true;
        }

        // note: 3 for a pointer per array and the end of the loop
        return size_bytes <= 2 * unroll_threshold_bytes_ and
               free_scratch_register_count() < 3;
    }

    // variables and frames start word aligned so a direct offset from their
    // base has a known position within a word
    [[nodiscard]] auto is_word_based(const operand& address) const -> bool {
        // an index register holds a value unknown at compile time
        if (not address.is_memory() or not address.index_register().empty()) {
            return false;
        }

        const std::string& base{address.base_register()};

        // note: the variables base is held while code is emitted
        const bool is_variables_base{base == variables_base_register()};

        const bool is_frame_base{
            is_backend_register(frame_base_register()) and
                base == frame_base_register(),
        };

        // other bases such as loaded pointers or bulk registers may hold any
        // address
        return is_variables_base or is_frame_base;
    }

    // the target as 'branch_to_failure' chooses it
    auto jump_to_failure(const emit_site site, const failure_handler& handler)
        -> void {

        const std::string target{failure_target(handler, site.src_loc_tk)};

        emit_no_return_jump(site.indent, target);
    }

    // the clobbered registers that hold a live value, an allocated or a
    // protected one
    [[nodiscard]] auto
    live_clobbered(const std::span<const std::string_view> clobbered,
                   const operand& dst) const -> std::vector<std::string_view> {

        std::vector<std::string_view> saved;

        for (const std::string_view name : clobbered) {
            // restoring a register destination would discard the result
            const bool is_destination{
                dst.is_register() and dst.register_name() == name,
            };

            const bool is_live{
                is_register_unavailable(name),
            };

            if (is_live and not is_destination) {
                saved.push_back(name);
            }
        }

        return saved;
    }

    // the pointer of 'slot', 0 the first address and 1 the second, holds the
    // address; the pointers advance together so both starts decide the width
    // of the loop
    auto load_bulk_address(const emit_site site, bulk_operation& bulk,
                           const size_t slot, const operand& address) -> void {

        bulk.starts.at(slot) = start_of(address, bulk.alignment);
        ++bulk.address_count;

        address_of(site, bulk.pointer(slot), address);
    }

    // borrowed pointers avoid a temporary address and final move for indexing
    auto load_bulk_addresses(const emit_site site, bulk_operation& bulk,
                             const address_emitter& first,
                             const address_emitter& second) -> void {

        first(bulk.pointer(0), [&](const operand& address) -> void {
            load_bulk_address(site, bulk, 0, address);
        });

        second(bulk.pointer(1), [&](const operand& address) -> void {
            load_bulk_address(site, bulk, 1, address);
        });
    }

    [[nodiscard]] auto load_destination(const emit_site site,
                                        const operand& dst)
        -> loaded_destination {

        if (dst.is_register()) {
            return {
                .address{},
                .value{dst},
            };
        }

        // lowering before allocating keeps the scratch register order
        const operand address{lower_address(site, dst)};

        const operand value{
            alloc_scratch_register(site, default_type()),
        };

        copy_value(site, value, address);

        return {
            .address{address},
            .value{value},
        };
    }

    // writes 'first_value' to 'a0' and 'source' to 'a1'
    auto load_helper_arguments(const emit_site site, const operand& first_value,
                               const operand& source) -> void {

        const operand first_argument{operand::reg("a0", default_type())};
        const operand second_argument{operand::reg("a1", default_type())};

        // note: 'call_arithmetic_helper' rejects a source in 'a0'
        assert(not source.uses_register("a0"));

        // note: a register 'first_value' is the destination of the helper,
        //       which receives the result after the call

        const bool consumed{
            first_value.is_register() and
                not source.uses_register(first_value.register_name()),
        };

        if (consumed) {
            assembler_.move_consumed(site.indent, "a0",
                                     first_value.register_name());
        } else {
            copy_value(site, first_argument, first_value);
        }

        copy_value(site, second_argument, source);
    }

    auto load_into(const emit_site site, const operand& value,
                   const operand& src) -> void {

        if (src.is_memory()) {
            // a load may build its address in the register it will overwrite
            const operand lowered{
                lower_address(site, src, value),
            };

            assembler_.load(site.indent, load_op(src.type_ref()),
                            value.register_name(), lowered.displacement(),
                            lowered.base_register());

            return;
        }

        // note: constants are stored by the caller
        assert(src.is_register());

        if (value.register_name() != src.register_name()) {
            assembler_.mv(site.indent, value.register_name(),
                          src.register_name());
        }
    }

    // 'src' into the register 'dst', the immediate 'constant' when it is
    // known
    auto load_operand(const emit_site site, const operand& dst,
                      const operand& src, const std::optional<int32_t> constant)
        -> void {

        if (constant.has_value()) {
            assembler_.li(site.indent, dst.register_name(), *constant);
            return;
        }

        copy_value(site, dst, src);
    }

    // a register holding 'address'
    [[nodiscard]] auto load_pointer(const emit_site site,
                                    const operand& address) -> operand {

        const operand pointer{
            alloc_scratch_register(site, default_type()),
        };

        address_of(site, pointer, address);

        return pointer;
    }

    // lower base + index * scale + displacement to register + signed 12-bit
    // offset
    [[nodiscard]] auto lower_address(const emit_site site,
                                     const operand& address,
                                     const operand& dst = {}) -> operand {

        validate_address(address);

        const std::string& base{address.base_register()};
        const std::string& index{address.index_register()};

        const int64_t offset{address.displacement()};
        const uint32_t upper{assembler_rv32i::upper_part(offset)};
        const int32_t low{assembler_rv32i::lower_part(offset)};

        // no index and no upper part: the memory instruction handles the
        // entire offset
        if (index.empty() and upper == 0) {
            return operand::mem(base, {}, 1, low, address.type_ref());
        }

        // the destination holds the computed address when that keeps its
        // inputs
        const operand result{
            can_reuse_address_destination(address, dst)
                ? dst
                : alloc_scratch_register(site, default_type()),
        };

        const std::string& result_name{result.register_name()};

        // only the upper part; the memory instruction adds the low
        if (index.empty()) {
            assembler_.lui(site.indent, result_name, upper);
            assembler_.add(site.indent, result_name, result_name, base);
            return operand::mem(result_name, {}, 1, low, address.type_ref());
        }

        // a result apart from the register inputs gathers the terms itself,
        // upper part first, so the upper part takes no register of its own
        if (upper != 0 and address.scale() == 1 and result_name != base and
            result_name != index) {

            assembler_.lui(site.indent, result_name, upper);
            assembler_.add(site.indent, result_name, result_name, base);
            assembler_.add(site.indent, result_name, result_name, index);
            return operand::mem(result_name, {}, 1, low, address.type_ref());
        }

        // indexed memory operand: [base + index * scale + displacement];
        // combine the register terms before applying the displacement

        // unit scale: one add combines the register inputs, even when the
        // result aliases either
        if (address.scale() == 1) {
            assembler_.add(site.indent, result_name, base, index);
        } else {
            // scaled index: form the product before adding the base
            assembler_.slli(site.indent, result_name, index,
                            std::countr_zero(address.scale()));

            assembler_.add(site.indent, result_name, result_name, base);
        }

        if (upper != 0) {
            const operation_scope scope{*this};

            const operand displacement{
                alloc_scratch_register(site, default_type()),
            };

            assembler_.lui(site.indent, displacement.register_name(), upper);

            assembler_.add(site.indent, result_name, result_name,
                           displacement.register_name());
        }

        return operand::mem(result_name, {}, 1, low, address.type_ref());
    }

    // what an access through the register 'pointer' reads or writes
    [[nodiscard]] auto memory_at(const operand& pointer) const -> operand {
        return operand::mem(pointer.register_name(), {}, 1, 0, default_type());
    }

    // the remaining constant needs shifts and adds; keep the original value
    // for the additions while the result changes
    auto multiply_by_shifts_and_adds(const emit_site site,
                                     const operand& product,
                                     const operand& factor,
                                     const uint32_t multiplier,
                                     const size_t bits) -> void {

        const operation_scope scope{*this, product, factor};

        // the original value stays here until the last add or sub, which
        // writes the result in its place so no copy is needed
        const loaded_destination loaded{
            load_destination(site, product),
        };

        const digit_sequence sequence{make_digit_sequence(multiplier, bits)};

        emit_shift_add_sequence(site, loaded, sequence);

        store_operation_result(site.indent, product, loaded.address,
                               loaded.value, true);
    }

    // a test that passes the check branches past the shared jump to the
    // handler, a later test ends the check
    auto pass_if(const size_t indent, failure_tests& tests, const op passed,
                 const std::string_view rs1, const std::string_view rs2)
        -> void {

        assert(tests.shared and tests.remaining > 1);

        --tests.remaining;

        assembler_.branch(indent, passed, rs1, rs2, check_passed_.reference);
    }

    // a register holding the result after the saved registers are restored
    auto preserved_helper_result(const emit_site site, const operand& result,
                                 const std::span<const std::string_view> saved)
        -> operand {

        const bool restored{
            std::ranges::find(saved, result.register_name()) != saved.end(),
        };

        if (not restored) {
            return result;
        }

        // restoring the saved registers would overwrite the result
        const operand kept{
            alloc_scratch_register(site, default_type()),
        };

        copy_value(site, kept, result);

        return kept;
    }

    // the loop ends when its pointer reaches 'end', replacing a counter that
    // costs a decrement in every iteration, a size below 2 KiB takes one
    // 'addi' through 'address_of'
    auto set_loop_end(const emit_site site, const operand& end,
                      const operand& pointer, const size_t size_bytes) -> void {

        const operand at{memory_at(pointer)};
        const operand past_end{offset_by(at, size_bytes)};

        address_of(site, end, past_end);
    }

    // a narrow register shifts to the top and back, extending in one pair
    auto shift_by_constant(const size_t indent,
                           const arithmetic_operator operation,
                           const operand& dst, const loaded_destination& loaded,
                           const uint32_t shift_count, const size_t bits)
        -> void {

        const bool is_left{operation == arithmetic_operator::shift_left};

        if (is_left and bits < register_bits_ and dst.is_register()) {

            assembler_.slli(indent, loaded.value.register_name(),
                            loaded.value.register_name(),
                            register_bits_ - bits + shift_count);

            assembler_.srai(indent, loaded.value.register_name(),
                            loaded.value.register_name(),
                            register_bits_ - bits);

            store_operation_result(indent, dst, loaded.address, loaded.value,
                                   false);

            return;
        }

        assembler_.immediate_op(indent, is_left ? op::slli : op::srai,
                                loaded.value.register_name(),
                                loaded.value.register_name(), shift_count);

        store_operation_result(indent, dst, loaded.address, loaded.value,
                               is_left);
    }

    auto shift_by_register(const emit_site site,
                           const arithmetic_operator operation,
                           const operand& dst, const operand& count,
                           const loaded_destination& loaded,
                           const runtime_check& check) -> void {

        const operand amount{
            source_register(site, count, std::nullopt),
        };

        if (check.enabled) {
            check_shift_count(site, amount, dst.type_ref());
        }

        const bool is_left{operation == arithmetic_operator::shift_left};

        assembler_.register_op(site.indent, is_left ? op::sll : op::sra,
                               loaded.value.register_name(),
                               loaded.value.register_name(),
                               amount.register_name());

        store_operation_result(site.indent, dst, loaded.address, loaded.value,
                               is_left);
    }

    // 'dst' is the low 'width_bits' of 'src' sign extended
    auto sign_extend(const size_t indent, const std::string_view dst,
                     const std::string_view src, const size_t width_bits)
        -> void {

        const size_t shift{register_bits_ - width_bits};

        assembler_.slli(indent, dst, src, shift);
        assembler_.srai(indent, dst, dst, shift);
    }

    auto source_register(const emit_site site, const operand& src,
                         const std::optional<int32_t> constant) -> operand {

        if (src.is_register()) {
            return src;
        }

        const operand right{
            alloc_scratch_register(site, default_type()),
        };

        load_operand(site, right, src, constant);

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

    auto store_byte_parts(const emit_site site,
                          const std::span<const byte_part> parts,
                          const operand& dst, const size_t size_bytes) -> void {

        const operand address{
            unrolled_address(site, dst, size_bytes),
        };

        const bool has_load{
            std::ranges::any_of(
                parts, [](const byte_part& p) -> bool { return p.needs_load; }),
        };

        const operand value{
            has_load ? alloc_scratch_register(site, default_type()) : operand{},
        };

        for (const byte_part& p : parts) {
            const int64_t displacement{
                address.displacement() + static_cast<int64_t>(p.offset),
            };

            if (p.value == 0) {
                assembler_.store(site.indent, store_op(p.size_bytes), "zero",
                                 displacement, address.base_register());

                continue;
            }

            if (p.needs_load) {
                assembler_.li(site.indent, value.register_name(), p.value);
            }

            assembler_.store(site.indent, store_op(p.size_bytes),
                             value.register_name(), displacement,
                             address.base_register());
        }
    }

    auto store_constant_result(const emit_site site, const operand& dst,
                               const int32_t constant) -> void {

        const operation_scope scope{*this, dst};

        const operand address{
            dst.is_memory() ? lower_address(site, dst) : operand{},
        };

        if (dst.is_memory() and constant == 0) {
            store_operation_result(site.indent, dst, address,
                                   operand::reg("zero", default_type()), false);

            return;
        }

        const operand value{working_register(site, dst)};

        assembler_.li(site.indent, value.register_name(), constant);
        store_operation_result(site.indent, dst, address, value, false);
    }

    auto store_low_bits(const emit_site site, const operand& dst,
                        const operand& value) -> void {

        const operand lowered{lower_address(site, dst)};

        assembler_.store(site.indent, store_op(dst.type_ref().size_bytes()),
                         value.register_name(), lowered.displacement(),
                         lowered.base_register());
    }

    auto store_operation_result(const size_t indent, const operand& dst,
                                const operand& address, const operand& value,
                                const bool normalize) -> void {

        const size_t width{dst.type_ref().size_bytes()};

        if (dst.is_memory()) {
            assembler_.store(indent, store_op(width), value.register_name(),
                             address.displacement(), address.base_register());
        } else if (width < word_size_bytes_ and normalize) {
            extend_register(indent, dst.type_ref(), value);
        }
    }

    // keeps an unrolled access of 'size_bytes' within the load/store offset
    // range
    [[nodiscard]] auto unrolled_address(const emit_site site,
                                        const operand& address,
                                        const size_t size_bytes) -> operand {

        operand lowered{lower_address(site, address)};

        if (lowered.displacement() + static_cast<int64_t>(size_bytes) - 1 >
            assembler_rv32i::immediate_max) {

            const operand pointer{
                alloc_scratch_register(site, default_type()),
            };

            address_of(site, pointer, lowered);

            lowered = memory_at(pointer);
        }

        // one return keeps the copy elided
        return lowered;
    }

    // the value compared with the limit, the index or the end 'index + count'
    [[nodiscard]] auto upper_bound_top(const emit_site site,
                                       const bounds_range& range,
                                       const operand& limit,
                                       const bool lower_checked,
                                       failure_tests& tests) -> std::string {

        const std::string_view index{range.checked.register_name()};

        if (not range.count) {
            return std::string{index};
        }

        const operand& reg_count{*range.count};

        const operand sum{
            alloc_scratch_register(site, default_type()),
        };

        const std::string& top{sum.register_name()};

        // after the lower checks both are below 2^31, so the sum cannot wrap
        if (lower_checked) {
            assembler_.add(site.indent, top, index, reg_count.register_name());
            return top;
        }

        // the sign and carry bits form the high word of the widened sum, the
        // exact end is 'high * 2^32 + top' with 'top' unsigned: an end below
        // the minimum of 32 bits fails, as the overflow check of x86_64 does,
        // a negative end that fits passes and is left to the lower check, a
        // high word of zero leaves the unsigned compare of 'top' with the
        // limit, which may be 2^31 or more
        const operand high{
            alloc_scratch_register(site, default_type()),
        };

        assembler_.srai(site.indent, high.register_name(), index, sign_shift_);

        assembler_.srai(site.indent, limit.register_name(),
                        reg_count.register_name(), sign_shift_);

        assembler_.add(site.indent, high.register_name(), high.register_name(),
                       limit.register_name());

        assembler_.add(site.indent, top, index, reg_count.register_name());
        assembler_.sltu(site.indent, limit.register_name(), top, index);

        assembler_.add(site.indent, high.register_name(), high.register_name(),
                       limit.register_name());

        // note: a carry needs a negative operand, so the high word is not
        //       positive and the end below 2^32; below the minimum the high
        //       word is less than the sign of 'top'

        assembler_.srai(site.indent, limit.register_name(), top, sign_shift_);

        fail_if(site, tests, op::blt, high.register_name(),
                limit.register_name());

        pass_if(site.indent, tests, op::blt, high.register_name(), "zero");

        return top;
    }

    // the chunks of 'loop' through a pointer per address past the head, then
    // the tail unrolled from the pointers
    auto walk_known_loop(const known_walk& walk, const known_loop& loop)
        -> void {

        std::vector<operand> pointers;
        std::vector<operand> at;
        for (const operand& address : walk.addresses) {
            const operand after_head{offset_by(address, loop.head_size_bytes)};

            const operand pointer{
                load_pointer({walk.src_loc_tk, walk.indent}, after_head),
            };

            pointers.push_back(pointer);
            at.push_back(memory_at(pointer));
        }

        const operand end{
            alloc_scratch_register({walk.src_loc_tk, walk.indent},
                                   default_type()),
        };

        comment({walk.src_loc_tk, walk.indent}, "{}",
                describe_loop(walk.verb, loop.width));

        set_loop_end({walk.src_loc_tk, walk.indent}, end, pointers.front(),
                     loop.chunk_count * loop.width);

        assembler_.label(walk.indent, chunk_loop_.name);
        walk.access(loop.width, at);

        for (const operand& pointer : pointers) {
            advance(walk.indent, pointer, loop.width);
        }

        assembler_.bne(walk.indent, pointers.front().register_name(),
                       end.register_name(), chunk_loop_.reference);

        if (loop.tail_size_bytes == 0) {
            return;
        }

        comment({walk.src_loc_tk, walk.indent}, "{} {} B tail", walk.verb,
                loop.tail_size_bytes);

        // the pointers are at a boundary of the loop width
        const access_start boundary{
            .alignment{loop.width},
            .phase{},
        };

        walk_parts(walk, loop.tail_size_bytes, std::span{&boundary, 1}, at);
    }

    // unrolled accesses from the direct offsets, or a loop of the widest
    // width every start reaches after a head unrolled from them, then a tail;
    // the walk's own registers are freed before a comparison writes its result
    auto walk_known_size(const known_walk& walk, const size_t size_bytes,
                         const bool is_unrolled) -> void {

        const operation_scope walk_scope{*this};

        if (is_unrolled) {
            comment_aligned_parts({walk.src_loc_tk, walk.indent}, walk.verb,
                                  size_bytes, walk.starts);

            walk_unrolled(walk, size_bytes);

            return;
        }

        const known_loop loop{plan_known_loop(size_bytes, walk.starts)};

        comment_known_loop({walk.src_loc_tk, walk.indent}, walk.verb, loop,
                           walk.starts);

        if (loop.head_size_bytes != 0) {
            // a head pointer for a far offset is not needed by the loop
            const operation_scope head_scope{*this};

            walk_unrolled(walk, loop.head_size_bytes);
        }

        walk_known_loop(walk, loop);
    }

    auto walk_runtime_bytes(const runtime_walk& walk) -> void {
        const std::string_view count{walk.count.register_name()};

        comment({walk.src_loc_tk, walk.indent}, "{}; skip if none",
                describe_loop(walk.verb, 1));

        assembler_.beqz(walk.indent, count, walk_end_.reference);

        // 'count' becomes the end of the walk, which saves a decrement in
        // every iteration
        assembler_.add(walk.indent, count, count, walk.src.register_name());

        walk_runtime_loop(walk, count, 1);
        assembler_.label(walk.indent, walk_end_.name);
    }

    // the loop takes 'count' rounded down to whole chunks and ends when 'src'
    // reaches their end, 'count' keeps the tail bytes
    auto walk_runtime_chunks(const runtime_walk& walk, const operand& chunks,
                             const size_t width) -> void {

        const std::string_view count{walk.count.register_name()};

        comment({walk.src_loc_tk, walk.indent},
                "split bytes into chunks and tail; skip loop if none");

        assembler_.andi(walk.indent, chunks.register_name(), count,
                        -static_cast<int64_t>(width));

        assembler_.andi(walk.indent, count, count, width - 1);

        assembler_.beqz(walk.indent, chunks.register_name(),
                        after_chunks_.reference);

        assembler_.add(walk.indent, chunks.register_name(),
                       chunks.register_name(), walk.src.register_name());

        comment({walk.src_loc_tk, walk.indent}, "{}",
                describe_loop(walk.verb, width));

        walk_runtime_loop(walk, chunks.register_name(), width);
        assembler_.label(walk.indent, after_chunks_.name);
    }

    // a head up to the first aligned chunk, whole chunks, then a tail
    auto walk_runtime_count(const runtime_walk& walk,
                            const bulk_operation& bulk) -> void {

        const std::array<access_start, 2>& starts{bulk.checked_starts()};

        const loop_start start{plan_loop_start(starts)};

        comment_pointer_loop({walk.src_loc_tk, walk.indent}, start, starts,
                             bulk.alignment);

        // without a known alignment every access is a byte
        if (start.width == byte_size_bytes_) {
            walk_runtime_bytes(walk);
            return;
        }

        // the end of the chunks, also the scratch of the head check and the
        // halfword tail test
        const operand chunks{
            alloc_scratch_register({walk.src_loc_tk, walk.indent},
                                   default_type()),
        };

        comment({walk.src_loc_tk, walk.indent}, "{}: end of {}, {}: tail bytes",
                chunks.register_name(),
                start.width == word_size_bytes_ ? "words" : "halfwords",
                walk.count.register_name());

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

        const std::string_view count{walk.count.register_name()};

        comment({walk.src_loc_tk, walk.indent}, "{} {} B head", walk.verb,
                head_size_bytes);

        if ((head_size_bytes & size_t{1}) != 0) {
            assembler_.beqz(walk.indent, count, after_head_.reference);

            walk_runtime_step(walk, 1);
            assembler_.addi(walk.indent, count, count, -1);
        }

        if ((head_size_bytes & size_t{2}) != 0) {
            assembler_.sltiu(walk.indent, chunks.register_name(), count, 2);

            assembler_.bnez(walk.indent, chunks.register_name(),
                            after_head_.reference);

            walk_runtime_step(walk, 2);
            assembler_.addi(walk.indent, count, count, -2);
        }

        assembler_.label(walk.indent, after_head_.name);
    }

    // steps of 'width' bytes until 'src' reaches 'end'
    auto walk_runtime_loop(const runtime_walk& walk, const std::string_view end,
                           const size_t width) -> void {

        assembler_.label(walk.indent, chunk_loop_.name);
        walk_runtime_step(walk, width);

        assembler_.bne(walk.indent, walk.src.register_name(), end,
                       chunk_loop_.reference);
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

        const std::string_view count{walk.count.register_name()};

        if (width == word_size_bytes_) {
            comment({walk.src_loc_tk, walk.indent}, "{} optional 2-byte tail",
                    walk.verb);

            assembler_.andi(walk.indent, chunks.register_name(), count, 2);

            assembler_.beqz(walk.indent, chunks.register_name(),
                            after_halfword_.reference);

            walk_runtime_step(walk, 2);
            assembler_.label(walk.indent, after_halfword_.name);
            assembler_.andi(walk.indent, count, count, 1);
        }

        comment({walk.src_loc_tk, walk.indent}, "{} optional final byte",
                walk.verb);

        assembler_.beqz(walk.indent, count, walk_end_.reference);
        walk.access(1);
        assembler_.label(walk.indent, walk_end_.name);
    }

    // the first 'size_bytes' from the direct offsets of the addresses
    auto walk_unrolled(const known_walk& walk, const size_t size_bytes)
        -> void {

        std::vector<operand> at;
        for (const operand& address : walk.addresses) {
            at.push_back(unrolled_address({walk.src_loc_tk, walk.indent},
                                          address, size_bytes));
        }

        walk_parts(walk, size_bytes, walk.starts, at);
    }

    // a register destination holds the result directly, memory needs a scratch
    [[nodiscard]] auto working_register(const emit_site site,
                                        const operand& dst) -> operand {

        if (dst.is_register()) {
            return dst;
        }

        return alloc_scratch_register(site, default_type());
    }

    // every access matched or the range was empty, unless a mismatch branched
    // to the false exit
    auto write_compare_result(const emit_site site, const operand& result,
                              const compare_registers& registers,
                              const bool inverted) -> void {

        comment(site, "all matched or empty: {}", inverted ? "false" : "true");

        assembler_.li(site.indent, registers.left.register_name(),
                      inverted ? 0 : 1);

        assembler_.j(site.indent, result_end_.reference);
        assembler_.label(site.indent, false_exit_.name);

        comment(site, "mismatch: {}", inverted ? "true" : "false");

        assembler_.li(site.indent, registers.left.register_name(),
                      inverted ? 1 : 0);

        assembler_.label(site.indent, result_end_.name);

        if (not registers.left_is_result) {
            copy_value(site, result, registers.left);
        }
    }

    auto zero_access(const size_t indent, const size_t width,
                     const operand& address) -> void {

        assembler_.store(indent, store_op(width), "zero",
                         address.displacement(), address.base_register());
    }

    //
    // statics
    //

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

    // the copy of a constant ('copy_constant') takes 'la' and a load and a
    // store for each of its 'copy_part_count' parts, counted as unrolled up to
    // 'immediates_threshold_bytes_'
    [[nodiscard]] static auto
    are_immediates_smaller(const std::span<const byte_part> parts,
                           const size_t size_bytes,
                           const size_t copy_part_count) -> bool {

        if (size_bytes > immediates_threshold_bytes_) {
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
    assert_folded([[maybe_unused]] const op instruction, const operand& dst,
                  [[maybe_unused]] const std::optional<int32_t> constant)
        -> void {

        const size_t width{dst.type_ref().size_bytes()};

        assert(not constant.has_value() or
               not keeps_destination(instruction, *constant, width));
    }

    // note: the front end rejects 'i64' and an instance first, e.g. "'i64' is
    //       not available on this target"

    static auto assert_scalar([[maybe_unused]] const type& value_type) -> void {
        assert(value_type.is_builtin() and
               (value_type.size_bytes() == byte_size_bytes_ or
                value_type.size_bytes() == half_size_bytes_ or
                value_type.size_bytes() == word_size_bytes_));
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

        const std::string& name{result.register_name()};

        // note: 'zero' discards writes and 'sp' holds the stack
        assert(name != "zero" and name != "sp");

        // writing the left value into one of the operands' registers would
        // destroy an address or the count before the walk is done with it
        return std::ranges::none_of(in_use, [&name](const operand& o) -> bool {
            return o.uses_register(name);
        });
    }

    [[nodiscard]] static auto
    can_reuse_address_destination(const operand& address, const operand& dst)
        -> bool {

        // no destination register is available to reuse
        if (not dst.is_register()) {
            return false;
        }

        const std::string& dst_name{dst.register_name()};

        // note: zero discards writes, so it cannot hold the computed address
        assert(dst_name != "zero");

        const bool is_base{dst_name == address.base_register()};

        // if the destination is the base register, loading the offset would
        // erase the base value before the add
        if (address.index_register().empty()) {
            return not is_base;
        }

        // if the destination is the base register, shifting the index into it
        // would erase the base value before the add
        if (is_base and address.scale() != 1) {
            return false;
        }

        return true;
    }

    // the code and data of the image cannot be addressed beyond 32 bits
    static auto check_address_range(const size_t memory_end_address) -> void {
        // note: an end of the address space would wrap to 0

        if (memory_end_address < assembler_rv32i::address_space_bytes) {
            return;
        }

        throw unlocated_error{std::format(
            "code, data and variables use {} B, which reaches the {} B that "
            "the target addresses",
            memory_end_address, assembler_rv32i::address_space_bytes)};
    }

    // only 'slt' and 'blt' exist: 'x >= y' is the complement of 'x < y' and,
    // with 'swaps_operands', 'x <= y' of 'y < x'
    [[nodiscard]] static auto
    complements_less_than(const comparison_operator operation) -> bool {

        return operation == comparison_operator::greater_equal or
               operation == comparison_operator::less_equal;
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

        return std::format("addresses {} and {}",
                           describe_start(starts.front()),
                           describe_start(starts.back()));
    }

    [[nodiscard]] static auto has_all_bits(const int32_t constant,
                                           const size_t width) -> bool {

        const uint32_t mask{
            std::numeric_limits<uint32_t>::max() >>
                ((word_size_bytes_ - width) * bits_per_byte),
        };

        return (static_cast<uint32_t>(constant) & mask) == mask;
    }

    [[nodiscard]] static auto helper_spec_of(const helper_routine routine)
        -> const helper_spec& {

        for (const helper_spec& spec : helper_specs_) {
            if (spec.routine == routine) {
                return spec;
            }
        }

        std::unreachable();
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
        return assembler_rv32i::register_number(name).has_value();
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

    // a constant 'value' as a register of 'width_type' holds it, truncated to
    // the width and sign extended, e.g. the right side of a comparison at the
    // width of the left side
    [[nodiscard]] static auto narrowed_immediate(const operand& value,
                                                 const type& width_type)
        -> std::optional<int32_t> {

        const std::optional<int32_t> constant{immediate_value(value)};

        if (not constant.has_value()) {
            return std::nullopt;
        }

        const int64_t wrapped{
            constant_arith::wrap_to_width(*constant, width_type),
        };

        // note: no conversion makes a 'bool', so a 'bool' constant is 0 or 1,
        //       which the sign extension keeps

        assert(not width_type.is_bool() or wrapped == 0 or wrapped == 1);

        return static_cast<int32_t>(wrapped);
    }

    // whether the result may have bits outside the destination width: a sum
    // can carry out of it, a bitwise result of extended operands, a constant
    // extended by 'narrowed_immediate' included, stays extended
    [[nodiscard]] static auto needs_normalize(const op instruction) -> bool {
        return instruction == op::add or instruction == op::sub;
    }

    // the loop starts at the widest boundary every start reaches after a head
    [[nodiscard]] static auto
    plan_known_loop(const size_t size_bytes,
                    const std::span<const access_start> starts) -> known_loop {

        const loop_start start{plan_loop_start(starts)};

        const size_t after_head_size_bytes{size_bytes - start.head_size_bytes};

        // note: a loop is only chosen above 'unroll_threshold_bytes_' or
        //       'zero_unroll_threshold_parts_', so there is more than one chunk

        assert(after_head_size_bytes / start.width > 1);

        return {
            .head_size_bytes{start.head_size_bytes},
            .width{start.width},
            .chunk_count{after_head_size_bytes / start.width},
            .tail_size_bytes{after_head_size_bytes % start.width},
        };
    }

    // the widest width that every start reaches after the same head
    [[nodiscard]] static auto
    plan_loop_start(const std::span<const access_start> starts) -> loop_start {

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

            if (reaches_width) {
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

    // 'address' from the register of 'lowered', which holds the upper part
    // of their shared base, see 'shares_address_base'
    [[nodiscard]] static auto rebased(const operand& address,
                                      const operand& lowered) -> operand {

        return operand::mem(lowered.base_register(), {}, 1,
                            assembler_rv32i::lower_part(address.displacement()),
                            address.type_ref());
    }

    // a narrow register gets high bits from a wider or differently extended
    // source
    [[nodiscard]] static auto register_needs_extension(const type& dst_type,
                                                       const operand& src)
        -> bool {

        // note: the callers handled an immediate before, 'copy_value' as a
        //       constant and 'comparison_operands_through_shared_base' takes
        //       two memory operands

        assert(not src.is_immediate());

        const size_t dst_size_bytes{dst_type.size_bytes()};
        const size_t src_size_bytes{src.type_ref().size_bytes()};

        const bool extension_differs{
            src.type_ref().is_bool() != dst_type.is_bool(),
        };

        return dst_size_bytes < word_size_bytes_ and
               (src_size_bytes > dst_size_bytes or
                (extension_differs and src_size_bytes == dst_size_bytes));
    }

    // the aligned stack area 'save_call_registers' allocates for 'count'
    // registers
    [[nodiscard]] static auto saved_registers_size_bytes(const size_t count)
        -> size_t {

        return align_storage_size(count * word_size_bytes_, stack_alignment_);
    }

    // two memory operands of one base register without index whose offsets
    // have the same nonzero 'upper_part'
    [[nodiscard]] static auto shares_address_base(const operand& dst,
                                                  const operand& src) -> bool {

        if (not dst.is_memory() or not src.is_memory()) {
            return false;
        }

        if (not dst.index_register().empty() or
            not src.index_register().empty()) {

            return false;
        }

        if (dst.base_register() != src.base_register()) {
            return false;
        }

        const uint32_t src_upper{
            assembler_rv32i::upper_part(src.displacement()),
        };

        // without an upper part the access needs no base register of its own
        return src_upper != 0 and
               src_upper == assembler_rv32i::upper_part(dst.displacement());
    }

    // the widest aligned parts like the unrolled 'copy_block'
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

    // the 'fail_if' and 'pass_if' of 'check_upper_bound'
    [[nodiscard]] static auto upper_bound_test_count(const bounds_range& range,
                                                     const bool lower_checked)
        -> size_t {

        // before the compare, an end of a range not checked below fails below
        // the minimum and passes when negative ('upper_bound_top')
        if (range.count and not lower_checked) {
            return 3;
        }

        return 1;
    }

    static auto validate_address(const operand& address) -> void {
        assert(address.is_memory());
        assert(is_register(address.base_register()));

        assert(address.index_register().empty() or
               is_register(address.index_register()));

        // note: an element size beyond the address range cannot be allocated
        assert(address.index_register().empty() or
               address.scale() <= std::numeric_limits<uint32_t>::max());

        // note: any displacement is valid, addresses wrap at 32 bits and
        //       'upper_part' and 'lower_part' keep its low 32 bits, e.g.
        //       the addend of 'a[i + K]' may pass the address range
    }

    // scalar operands in a register or an address of the target, the result
    // is written in place
    static auto validate_operands(const operand& dst, const operand& src)
        -> void {

        assert_scalar(dst.type_ref());
        assert_scalar(src.type_ref());

        assert(dst.is_register() or dst.is_memory());

        if (dst.is_memory()) {
            validate_address(dst);
        }

        if (src.is_memory()) {
            validate_address(src);
        }
    }

    // the accesses of 'size_bytes' past the operands 'at'
    static auto walk_parts(const known_walk& walk, const size_t size_bytes,
                           const std::span<const access_start> starts,
                           const std::span<const operand> at) -> void {

        for (const access_part& part : aligned_parts(size_bytes, starts)) {
            std::vector<operand> moved;
            for (const operand& address : at) {
                moved.push_back(offset_by(address, part.offset));
            }

            walk.access(part.width, moved);
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
