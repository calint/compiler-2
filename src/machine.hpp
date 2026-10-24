#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <expected>
#include <format>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#include <ostream>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "compile_error.hpp"
#include "operand.hpp"
#include "register_pool.hpp"
#include "source_files.hpp"
#include "token.hpp"
#include "type.hpp"

// a check of an operation at run time: enabled, a failure jumps to the failure
// handler of the check, which reports the line when the machine
// 'reports_line'; the operation that takes it tells what fails
struct runtime_check {
    bool enabled{};
};

// what the front end emits through, and what every target shares: the
// registers with the variables and frame bases, the trace of their use and
// the routines, helpers and failure stubs the code uses; a backend supplies
// the instructions, the names of its registers and the order it picks scratch
// registers in; the public interface by group:
//
//   lifecycle      in the order 'program' calls it: 'start', 'end_main',
//                  'begin_noinline_body' and 'end_noinline_body' for each
//                  body, 'emit_runtime', 'emit_string_constants',
//                  'begin_data', 'reserve_variables', 'finish',
//                  'write_assembly'
//   data           'data_alignment', 'define_constant', 'emit_data',
//                  'emit_data_array', 'emit_repeated_data',
//                  'emit_string_data', 'emit_zero_data', 'zero_padding'
//   registers      'alloc_scratch_register', 'free_scratch_register(s)',
//                  'free_named_register(s)', 'reserve_frame_base',
//                  'release_frame_base', 'slot_register', 'register_view',
//                  'reserve_variables_base' (a backend, in 'start')
//   operations     'add_subtract', 'multiply', 'divide', 'shift', 'bitwise',
//                  'unary', 'zero', 'copy', 'copy_value', 'copy_bytes',
//                  'address_of', 'scale_index', 'store_boolean',
//                  'memory_equal'
//   control        'label', 'branch', 'compare_and_branch', 'call_function',
//                  'call_target', 'return_function', 'exit'
//   checks         'check_bounds', 'check_frame_capacity',
//                  'check_stack_capacity'
//   versions       'emit_most_efficient', 'emit_discarded'
//   comments       'comment', 'comment_binding', 'comment_variable',
//                  'comment_held_address'
//   target         'default_type', 'builtin_types', 'address_size_bytes',
//                  'has_indexed_addressing', 'is_stack_bounded',
//                  'max_storage_bytes', 'sector_size_bytes',
//                  'foo_counter_in_memory',
//                  'max_padding_gaps_zeroed_one_by_one', 'frame_base_register',
//                  'variables_base_register', 'variables_base_past_vars_bytes'
//   reports        'register_peak_report', 'separate_report', 'statistics';
//                  the queries of the register report are private, for
//                  'register_reporter'
//   backend tests  'assert_registers_free', 'emitted_line_count',
//                  'emitted_text', 'release_variables_base'
class machine {
  public:
    // which of two versions of the same code 'emit_most_efficient' keeps: the
    // smaller, or to test the other version too, the first or the last
    enum class kept_version : uint8_t { smaller, first, last };

    // a built-in the target provides as a system call or a routine
    enum class target_call : uint8_t {
        read,
        write,
        exit,
        sector_read,
        sector_write,
    };

    // receives an address while the registers that built it are allocated
    using address_use = std::function_ref<void(const operand& address)>;

    // emits one address of a bulk operation: 'count' is the register of the
    // element count, empty for a known size, 'preferred' a register the address
    // may be built in, empty when the backend has none to offer
    using address_emitter = std::function<void(
        const operand& count, const operand& preferred, address_use use)>;

    // at namespace scope, the front end computes with them too
    using arithmetic_operator = ::arithmetic_operator;

    using comparison_operator = ::comparison_operator;

    // whether an operation may overwrite a register operand, e.g. a scratch
    // register that holds a factor and is freed after the operation
    enum class factor_use : uint8_t { read_only, may_overwrite };

    using frame_kind = register_trace::frame_kind;

    // at namespace scope because a default argument '= {}' of a member
    // function cannot use a nested class with default member initializers
    using runtime_check = ::runtime_check;

    // what the command line chooses for the target, a target reads what it
    // has, e.g. only the bare-metal ones a stack
    struct target_options {
        assembler::jump_mode jumps{assembler::jump_mode::as_emitted};
        kept_version kept{kept_version::smaller};
        // the use of registers is kept for 'register_peak_report'
        bool register_report{};
        // the image of the sd card on targets with an operating system
        std::string_view sd_card_file_name;
        // the image that the rv32i targets write
        std::string_view binary_file_name;
        // the stack of the bare-metal targets
        size_t stack_size_bytes{};
        // the memory of 'rv32i-fpga'
        size_t memory_size_bytes{};
        // '--checks=line': a failed check reports the line of its source
        bool reports_line{};
    };

    struct output_statistics {
        assembler::optimization_counts optimizations;
        std::vector<assembler::function_summary> noinline_functions;
        size_t max_scratch_registers{};
        size_t instruction_count{};
    };

    struct comparison_action {
        comparison_operator operation{comparison_operator::equal};

        // negate the comparison before storing or branching
        bool inverted{};

        // receives 0 or 1, empty skips storing
        operand destination;

        // branch label, empty skips branching
        std::string_view target;

        // branch on true after 'inverted', otherwise on false
        bool branch_on_true{};
    };

    struct bounds_check_options {
        bool upper{};
        bool lower{};
    };

    // what a bounds check compares with 'array_count' elements: an index, a
    // count alone, or a range 'start + count'; a count and a range may end at
    // the end of the array, an index may not
    struct bounds_range {
        // the index, the count alone or the start of the range
        operand checked;
        // the count of a range, empty otherwise
        operand count;
        size_t array_count{};
        bool allow_end{};

        //
        // statics
        //

        [[nodiscard]] static auto count_alone(const operand& count,
                                              const size_t array_count)
            -> bounds_range {

            return {
                .checked{count},
                .count{},
                .array_count{array_count},
                .allow_end{true},
            };
        }

        [[nodiscard]] static auto index(const operand& index,
                                        const size_t array_count)
            -> bounds_range {

            return {
                .checked{index},
                .count{},
                .array_count{array_count},
                .allow_end{},
            };
        }

        [[nodiscard]] static auto range(const operand& start,
                                        const operand& count,
                                        const size_t array_count)
            -> bounds_range {

            return {
                .checked{start},
                .count{count},
                .array_count{array_count},
                .allow_end{true},
            };
        }
    };

    // read-only bytes at 'label'
    struct string_constant {
        std::string label;
        std::string bytes;
    };

    struct target_call_registers {
        // argument register names in parameter order
        std::span<const std::string_view> arguments;

        // result register name, empty for 'exit' and the sector functions
        std::string_view result;
    };

    // bytes of an instance that no field covers, from its start, e.g. the 3 B
    // at offset 1 of 'type r { a i8, b i32 }'; 'alignment' is known for the
    // first byte
    struct padding_gap {
        size_t offset{};
        size_t size_bytes{};
        size_t alignment{};
    };

    // the registers allocated while it lives belong to the call it names in
    // the register trace, see 'register_peak_report'; it ends the frame of
    // the trace when it goes out of scope, an error leaves it too
    class register_trace_scope final {
        machine& machine_;

      public:
        register_trace_scope(machine& backend, const token& call_site_tk,
                             std::string name, const frame_kind kind)
            : machine_{backend} {

            backend.begin_trace_frame(call_site_tk, std::move(name), kind);
        }

        register_trace_scope(const register_trace_scope&) = delete;
        register_trace_scope(register_trace_scope&&) = delete;
        auto operator=(const register_trace_scope&)
            -> register_trace_scope& = delete;
        auto operator=(register_trace_scope&&)
            -> register_trace_scope& = delete;

        ~register_trace_scope() { machine_.end_trace_frame(); }
    };

  protected:
    // how a constant factor multiplies, decided at its width
    enum class factor_kind : uint8_t {
        zero,
        one,
        // all low bits set
        minus_one,
        power_of_two,
        other,
    };

    // the lines of the failures in each file, with '--checks=line'
    using failure_lines = std::map<size_t, std::set<size_t>>;

    // the registers of the calls the target provides, see
    // 'registers_for_target_call'
    struct target_call_names {
        // the descriptor or sector, the address and the count or size; 'exit'
        // takes the first
        std::array<std::string_view, 3> arguments;
        // the count that 'read' and 'write' return
        std::string_view result;
    };

    // the addresses and the result of a comparison of two arrays, 'alignment'
    // is the alignment known for both
    struct equality_request {
        size_t alignment{};
        address_emitter lhs;
        address_emitter rhs;
        operand dst;
        bool inverted{};
    };

    // the addresses of a copy of elements, 'alignment' is the alignment known
    // for both
    struct copy_request {
        size_t alignment{};
        address_emitter src;
        address_emitter dst;
        // an enabled check makes a copy where the destination starts inside
        // the source jump to the overlap failure handler
        runtime_check overlap;
    };

  private:
    using routine_uses = std::map<std::string, failure_lines, std::less<>>;

    // what compiling a version of code changes in the machine
    struct version_state {
        // the indexes are those of the names of the backend, see
        // 'register_index_of'
        register_pool registers;
        bool frame_base_reserved{};
        // the inlined calls being compiled and what the registers held
        register_trace trace;
        // the most scratch registers in use
        size_t max_scratch_regs{};
        // the routines, helpers and failure handlers the code uses, by label,
        // each with the locations its failures report; emitted after the
        // code, see 'mark_used'
        routine_uses used_routines;
    };

    // saved before a version and put back after it, see 'version_scope'
    version_state state_;

  protected:
    // keeps the registers of its operands, and the base and index registers
    // of memory operands, from being picked for scratch while an operation is
    // lowered. on exit, also while an exception unwinds, it frees the
    // registers allocated within the scope and restores what was protected.
    // it restores allocator bookkeeping only, not register values, and what
    // was allocated before the scope must not be freed within it
    class address_scope final {
        machine& machine_;
        uint32_t saved_mask_;
        size_t saved_count_;

      public:
        address_scope(machine& backend, const operand& dst, const operand& src)
            : machine_{backend},
              saved_mask_{backend.state_.registers.unavailable_mask()},
              saved_count_{backend.state_.registers.allocations().size()} {

            for (const operand* value : {&dst, &src}) {
                if (value->is_register() or value->is_memory()) {
                    backend.protect_register(value->base_register());
                }

                if (value->is_memory()) {
                    backend.protect_register(value->index_register());
                }
            }
        }

        // protects nothing, only frees what is allocated while it lives
        explicit address_scope(machine& backend)
            : address_scope{backend, operand{}, operand{}} {}

        address_scope(const address_scope&) = delete;
        address_scope(address_scope&&) = delete;
        auto operator=(const address_scope&) -> address_scope& = delete;
        auto operator=(address_scope&&) -> address_scope& = delete;

        ~address_scope() {
            register_pool& pool{machine_.state_.registers};

            while (pool.allocations().size() > saved_count_) {
                // implicit releases need the allocation context for a balanced
                // trace
                const register_pool::allocation entry{pool.top()};

                machine_.comment(entry.src_loc_tk, entry.indent,
                                 "free {} register {}",
                                 entry.named ? "named" : "scratch",
                                 machine_.register_display_name(entry.index));

                pool.pop(entry.index);
            }

            pool.restore_unavailable(saved_mask_);
        }
    };

    // what a bounds check does about the lower bound, decided from its
    // options and operands
    struct bounds_plan {
        // the unsigned upper comparison already fails a negative index or
        // count as long as the limit is below 2^(width - 1), only a sum
        // 'index + count' needs both signs checked
        bool upper_covers_lower{};

        // the second array of a copy or compare checks the same count again
        bool count_known{};
    };

    struct constant_factor {
        factor_kind kind{};
        // the factor with the bits above the width cleared
        uint64_t multiplier{};
        // the shift count of a power of two
        int shift{};
    };

    // the handler of a failed check prints 'panic: NAME' and exits; one that
    // 'takes_line' prints the line of the failure with '--checks=line'
    struct failure_handler {
        std::string_view label;
        std::string_view name;
        // the comment above it in the listing
        std::string_view comment;
        bool takes_line{};
        // the status the program exits with, one per handler, the same on
        // every target
        int exit_code{};
    };

  private:
    source_locations locations_;
    std::reference_wrapper<const builtin_type_set> builtins_;
    assembler::jump_mode jump_mode_;
    // the image of the sd card on targets with an operating system
    std::string sd_card_file_name_;

    bool variables_base_reserved_{};
    kept_version kept_version_{kept_version::smaller};
    // see 'target_options::reports_line'
    bool reports_line_{};

    // the state before a version, put back when the scope ends, also when an
    // error unwinds; a version that is kept is given its end state again
    // ('emit_most_efficient'), discarded code is not ('emit_discarded')
    class version_scope final {
        machine& machine_;
        version_state start_;

      public:
        explicit version_scope(machine& backend)
            : machine_{backend}, start_{backend.version_state_now()} {}

        version_scope(const version_scope&) = delete;
        version_scope(version_scope&&) = delete;
        auto operator=(const version_scope&) -> version_scope& = delete;
        auto operator=(version_scope&&) -> version_scope& = delete;

        ~version_scope() { machine_.restore_version_state(std::move(start_)); }
    };

    // the lines of a compiled version with the state it ended in
    struct compiled_version {
        assembler::captured_lines lines;
        version_state end_state;
    };

    // a version that runs out of registers is not available, its error is
    // kept to be thrown when the other version runs out too
    using version_result = std::expected<compiled_version, std::exception_ptr>;

    using jump_mode = assembler::jump_mode;

  public:
    // 'files' locate the tokens of comments
    machine(const builtin_type_set& builtins, const source_files& files,
            const target_options& options)
        : locations_{files}, builtins_{builtins}, jump_mode_{options.jumps},
          sd_card_file_name_{options.sd_card_file_name},
          kept_version_{options.kept}, reports_line_{options.reports_line} {

        if (options.register_report) {
            state_.trace.enable();
        }
    }

    machine(const machine&) = delete;
    machine(machine&&) = delete;
    auto operator=(const machine&) -> machine& = delete;
    auto operator=(machine&&) -> machine& = delete;

    virtual ~machine() = default;

    // a failed check prints its message, the 'panic: ...' of its handler, to
    // this descriptor and exits with the code of its handler
    static constexpr int stderr_descriptor{2};

    // the sector size of the sd card of every target that has one
    static constexpr size_t card_sector_size_bytes{512};

    // the unit of the shifts that take a byte out of a value
    static constexpr size_t bits_per_byte{
        std::numeric_limits<unsigned char>::digits,
    };

    // the labels of the sections of the program's data and variables
    static constexpr std::string_view data_label{"dat"};
    static constexpr std::string_view data_end_label{"dat.end"};
    static constexpr std::string_view variables_label{"vars"};
    static constexpr std::string_view variables_end_label{"vars.end"};

    // the start of the label of a noinline body, the report drops it
    static constexpr std::string_view body_label_prefix{"func."};

    // the handlers of the failed checks, a check site passes its entry, so
    // the label, the name and whether it takes the line come together
    static constexpr failure_handler bounds_failure{
        .label{"baz_bounds_panic"},
        .name{"bounds"},
        .comment{"bounds failure handler (--checks=upper or --checks=lower)"},
        .takes_line{true},
        .exit_code{255},
    };
    static constexpr failure_handler division_failure{
        .label{"baz_division_panic"},
        .name{"division"},
        .comment{"division failure handler (--checks=division)"},
        .takes_line{true},
        .exit_code{253},
    };
    static constexpr failure_handler frame_overflow_failure{
        .label{"baz_frame_overflow"},
        .name{"frame overflow"},
        .comment{"frame overflow handler (--checks=frame)"},
        .takes_line{false},
        .exit_code{254},
    };
    static constexpr failure_handler overflow_failure{
        .label{"baz_overflow_panic"},
        .name{"overflow"},
        .comment{"overflow failure handler (--checks=overflow)"},
        .takes_line{true},
        .exit_code{249},
    };
    static constexpr failure_handler overlap_failure{
        .label{"baz_overlap_panic"},
        .name{"overlap"},
        .comment{"overlap failure handler (--checks=overlap)"},
        .takes_line{true},
        .exit_code{251},
    };
    static constexpr failure_handler shift_failure{
        .label{"baz_shift_panic"},
        .name{"shift"},
        .comment{"shift failure handler (--checks=shift)"},
        .takes_line{true},
        .exit_code{252},
    };
    static constexpr failure_handler stack_overflow_failure{
        .label{"baz_stack_overflow"},
        .name{"stack overflow"},
        .comment{"stack overflow handler (--checks=stack)"},
        .takes_line{false},
        .exit_code{250},
    };

    // in the order they are emitted after the code
    static constexpr std::array<failure_handler, 7> failure_handlers{
        frame_overflow_failure, division_failure,       overlap_failure,
        overflow_failure,       stack_overflow_failure, shift_failure,
        bounds_failure,
    };

    // a failed sector read or write jumps to it, the sector routines emit it,
    // see 'emit_sector_routines'
    static constexpr failure_handler sd_card_failure{
        .label{"baz_sd_card_panic"},
        .name{"sd card"},
        .comment{"sd card failure handler (sector_read or sector_write)"},
        .takes_line{false},
        .exit_code{248},
    };

    // a panic is told by its exit code alone, e.g. by a script
    static_assert([] -> bool {
        std::array<int, failure_handlers.size() + 1> codes{
            sd_card_failure.exit_code,
        };

        for (size_t i{}; i < failure_handlers.size(); ++i) {
            codes.at(i + 1) = failure_handlers.at(i).exit_code;
        }

        std::ranges::sort(codes);

        return std::ranges::adjacent_find(codes) == codes.end();
    }());

    //
    // virtual methods
    //

    // an enabled check makes a signed result that does not fit the width of
    // 'dst' jump to the overflow failure handler, also in 'multiply' and
    // 'unary'
    virtual auto add_subtract(const token& src_loc_tk, const size_t indent,
                              const arithmetic_operator operation,
                              const operand& dst, const operand& src,
                              const runtime_check& check = {}) -> void = 0;

    virtual auto address_of(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& address)
        -> void = 0;

    [[nodiscard]] virtual auto address_size_bytes() const -> size_t = 0;

    // 'emit_count' compiles the element count into the register it is given,
    // the elements of both arrays are compared at the width 'alignment' allows
    // and the result, 1 when they are equal, goes to 'dst'
    virtual auto
    arrays_equal(const token& src_loc_tk, const size_t indent,
                 const size_t element_size_bytes,
                 const std::function_ref<void(const operand&)> emit_count,
                 const equality_request& request) -> void = 0;

    // starts the data section, after the code and every routine it uses
    virtual auto begin_data(const size_t alignment) -> void = 0;

    virtual auto bitwise(const token& src_loc_tk, const size_t indent,
                         const arithmetic_operator operation,
                         const operand& dst, const operand& src) -> void = 0;

    virtual auto branch(const size_t indent, const std::string_view target)
        -> void = 0;

    [[nodiscard]] virtual auto
    can_lower_index_scale(const size_t size_bytes) const -> bool = 0;

    virtual auto check_bounds(const token& src_loc_tk, const size_t indent,
                              const bounds_range& range,
                              const bounds_check_options& options) -> void = 0;

    // jumps to the frame overflow handler when the callee frame at
    // 'frame_address' of 'frame_size_bytes' does not fit in the variables
    virtual auto check_frame_capacity(const token& src_loc_tk,
                                      const size_t indent,
                                      const operand& frame_address,
                                      const operand& frame_size_bytes)
        -> void = 0;

    // jumps to the stack overflow handler when a noinline call, made with the
    // registers allocated now, does not fit in the stack; only a target that
    // 'is_stack_bounded' is asked
    virtual auto check_stack_capacity([[maybe_unused]] const token& src_loc_tk,
                                      [[maybe_unused]] const size_t indent)
        -> void {

        std::unreachable();
    }

    virtual auto compare_and_branch(const token& src_loc_tk,
                                    const size_t indent, const operand& lhs,
                                    const operand& rhs,
                                    const comparison_action& action)
        -> void = 0;

    // 'alignment' is the alignment known for both addresses
    virtual auto copy(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const operand& src,
                      const size_t size_bytes, const size_t alignment)
        -> void = 0;

    // stores constant 'bytes', 'add_constant' returns the read-only data label
    // and is called only when the bytes are not stored with immediates
    virtual auto copy_bytes(const token& src_loc_tk, const size_t indent,
                            const std::string_view bytes, const operand& dst,
                            const size_t alignment,
                            const std::function_ref<std::string()> add_constant)
        -> void = 0;

    // copies the count of elements 'emit_count' compiles, the request emits
    // the addresses
    virtual auto
    copy_elements(const token& src_loc_tk, const size_t indent,
                  const size_t element_size_bytes,
                  const std::function_ref<void(const operand&)> emit_count,
                  const copy_request& request) -> void = 0;

    virtual auto copy_value(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& src) -> void = 0;

    [[nodiscard]] virtual auto data_alignment() const -> size_t = 0;

    [[nodiscard]] virtual auto default_type() const -> const type& = 0;

    virtual auto define_constant(const std::string_view name,
                                 const size_t value) -> void = 0;

    // an enabled check makes a zero divisor, and the minimum of the type of
    // 'dst' divided by -1, jump to the division failure handler
    virtual auto divide(const token& src_loc_tk, const size_t indent,
                        const arithmetic_operator operation, const operand& dst,
                        const operand& divisor, const runtime_check& check)
        -> void = 0;

    virtual auto emit_data(const size_t element_size_bytes, const int64_t value)
        -> void = 0;

    virtual auto emit_data_array(const size_t element_size_bytes,
                                 const std::span<const int64_t> values)
        -> void = 0;

    virtual auto emit_repeated_data(const size_t element_size_bytes,
                                    const size_t count, const int64_t value)
        -> void = 0;

    // leaves the read-only data section current
    virtual auto
    emit_string_constants(const std::span<const string_constant> strings)
        -> void = 0;

    // the backend writes the bytes in the escapes of its assembler
    virtual auto emit_string_data(const std::string_view bytes) -> void = 0;

    virtual auto emit_zero_data(const size_t size_bytes) -> void = 0;

    virtual auto end_main() -> void = 0;

    virtual auto exit(const token& src_loc_tk, const size_t indent,
                      const operand& exit_code) -> void = 0;

    virtual auto finish() -> void = 0;

    // 'limit' is the constant array size or a register holding a count,
    // 'counter' is a register or a memory operand
    virtual auto
    foo_advance_iteration(const token& src_loc_tk, const size_t indent,
                          const operand& iterator, const operand& counter,
                          const size_t element_size_bytes, const operand& limit,
                          const std::string_view loop_label) -> void = 0;

    // whether the counter of an array loop is better kept in memory than in a
    // register, decided when the loop starts
    [[nodiscard]] virtual auto foo_counter_in_memory() const -> bool = 0;

    [[nodiscard]] virtual auto frame_base_register() const
        -> std::string_view = 0;

    // whether an access can add an index register to a base register, else an
    // indexed address is computed into a register by separate instructions
    [[nodiscard]] virtual auto has_indexed_addressing() const -> bool = 0;

    // true where no operating system stops a program that overflows the
    // stack: calls check that the stack has room and a failure has a handler
    [[nodiscard]] virtual auto is_stack_bounded() const -> bool {
        return false;
    }

    [[nodiscard]] virtual auto
    make_register_operand(const std::string_view name,
                          const type& value_type) const -> operand = 0;

    // above this count 'zero_padding' zeroes the whole instance, which is then
    // smaller than a zero per gap
    [[nodiscard]] virtual auto max_padding_gaps_zeroed_one_by_one() const
        -> size_t = 0;

    // the most bytes of data and variables the target can address
    [[nodiscard]] virtual auto max_storage_bytes() const -> size_t = 0;

    // compares 'size_bytes' of two arrays and puts 1 into the destination of
    // the request when they are equal
    virtual auto memory_equal(const token& src_loc_tk, const size_t indent,
                              const size_t size_bytes,
                              const equality_request& request) -> void = 0;

    virtual auto multiply(const token& src_loc_tk, const size_t indent,
                          const operand& product, const operand& factor,
                          const factor_use use, const runtime_check& check = {})
        -> void = 0;

    virtual auto reserve_variables(const size_t alignment,
                                   const size_t size_bytes) -> void = 0;

    virtual auto return_function(const size_t indent) -> void = 0;

    // the size of a sector of the sd card, 0 when the target has no card
    [[nodiscard]] virtual auto sector_size_bytes() const -> size_t {
        return card_sector_size_bytes;
    }

    // an enabled check makes a count below zero or not below the width of the
    // type of 'dst' jump to the shift failure handler
    virtual auto shift(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst,
                       const operand& count, const runtime_check& check)
        -> void = 0;

    // the register that carries the address of the receiver of a method call
    [[nodiscard]] virtual auto slot_register() const -> std::string_view = 0;

    virtual auto start() -> void = 0;

    virtual auto unary(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst,
                       const runtime_check& check = {}) -> void = 0;

    // empty when the variables base register points at 'dat', else how far
    // past the 'vars' label it points
    [[nodiscard]] virtual auto variables_base_past_vars_bytes() const
        -> std::optional<size_t> = 0;

    [[nodiscard]] virtual auto variables_base_register() const
        -> std::string_view = 0;

    // writes the lines buffered from 'start'
    virtual auto write_assembly(std::ostream& os) -> void = 0;

    virtual auto zero(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const size_t size_bytes,
                      const size_t alignment) -> void = 0;

    //
    // class methods
    //

    // a register that an instruction or the calling convention fixes, held
    // until 'free_named_register'
    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand {

        reserve_named_register(src_loc_tk, indent, register_name, type_ref);
        return allocated_operand(register_name, type_ref);
    }

    // the first free register in the scratch order of the backend
    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref) -> operand {

        validate_register_type(src_loc_tk, type_ref);

        for (const std::string_view register_name : scratch_registers()) {
            if (state_.registers.is_unavailable(register_mask(register_name))) {
                continue;
            }

            comment(src_loc_tk, indent, "allocate scratch register -> {}",
                    register_name);

            push_allocation(src_loc_tk, indent, register_name, false);

            state_.max_scratch_regs = std::max(
                state_.max_scratch_regs, state_.registers.scratch_count());

            return allocated_operand(register_name, type_ref);
        }

        throw out_of_registers_error(src_loc_tk);
    }

    // every operation freed what it allocated, also checked by backend tests
    // between their cases
    auto assert_registers_free() const -> void {
        assert(state_.registers.is_empty());
        assert(state_.registers.unavailable_mask() == 0);
        assert(not state_.frame_base_reserved);
    }

    // the code emitted up to 'end_noinline_body' is a body of 'function'
    auto begin_noinline_body(std::string function, std::string label) -> void {
        target_assembler().begin_body(std::move(function), std::move(label));
    }

    // the set the front end shares, made before the machine
    [[nodiscard]] auto builtin_types() const -> const builtin_type_set& {
        return builtins_.get();
    }

    // 'slot_address' is the address the callee finds in
    // 'slot_register', empty when the register is not used
    auto call_function(const token& src_loc_tk, const size_t indent,
                       const std::string_view label,
                       const operand& frame_address,
                       const operand& slot_address) -> void {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());
        assert(not is_stack_pointer(frame_address.base_register()));

        // the callee may use every register but the variables base, so only
        // values live at the call need saving
        const std::vector<size_t> saved{registers_saved_by_call()};

        record_noinline_call(src_loc_tk, label, saved.size());

        if (not saved.empty()) {
            comment(src_loc_tk, indent,
                    "before call: save allocated registers");
        }

        save_call_registers(indent, saved);
        load_slot_address(src_loc_tk, indent, slot_address);

        comment(src_loc_tk, indent, "set function frame base");

        // a far frame address is lowered through a scratch register, which
        // must not be the loaded slot register
        const address_scope slot_kept{
            *this,
            make_register_operand(slot_register(), default_type()),
            operand{},
        };

        address_of(src_loc_tk, indent,
                   make_register_operand(frame_base_register(), default_type()),
                   frame_address);

        emit_call(indent, label);

        if (not saved.empty()) {
            comment(src_loc_tk, indent, "after call: restore saved registers");
        }

        restore_call_registers(indent, saved);
    }

    // a call of 'read', 'write', 'sector_read' or 'sector_write' with its
    // arguments and its result in the registers of
    // 'registers_for_target_call', 'result' empty for a sector function; a
    // sector is an unsigned 32 bit number, the size of a sector transfer is
    // 'sector_size_bytes', a sector past the end of the card panics where the
    // target can tell
    auto call_target(const token& src_loc_tk, const size_t indent,
                     const target_call function,
                     const std::span<const operand> args, const operand& result)
        -> void {

        // note: 'exit' is reached through 'exit', which takes any operand
        assert(function != target_call::exit);

        assert_target_call_registers(function, args, result);
        emit_target_call(src_loc_tk, indent, function, args, result);
    }

    // synthetic tokens and standalone backend calls have no source location;
    // the text may quote the source, e.g. a raw NUL that ends a line for nasm
    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void {

        const std::string line{printable(text)};

        if (src_loc_tk.at_line() == 0) {
            target_assembler().comment(indent, line);
            return;
        }

        const std::string location{locations_.for_comment(src_loc_tk)};

        target_assembler().comment(indent, location, line);
    }

    template <typename... args_t>
    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::format_string<args_t...> format, args_t&&... args)
        -> void {

        const std::string text{
            std::format(format, std::forward<args_t>(args)...),
        };

        comment(src_loc_tk, indent, std::string_view{text});
    }

    // a parameter bound to what 'to' names, with the address the call
    // computed when it did
    auto comment_binding(const token& src_loc_tk, const size_t indent,
                         const std::string_view from, const std::string_view to,
                         const operand& address) -> void {

        if (address.is_empty()) {
            comment(src_loc_tk, indent, "bind {} -> {}", from, to);
            return;
        }

        comment(src_loc_tk, indent, "bind {} -> {} (lea: {})", from, to,
                address_text(address));
    }

    // e.g. 'b: bag (address @ [rbx])', the slot holds the address of the
    // variable, e.g. a parameter of a noinline body
    auto comment_held_address(const token& src_loc_tk, const size_t indent,
                              const std::string_view text,
                              const operand& address) -> void {

        comment(src_loc_tk, indent, "{} (address @ [{}])", text,
                address_text(address));
    }

    // e.g. 'x: i32 (4 B @ [rbp + 8])'
    auto comment_variable(const token& src_loc_tk, const size_t indent,
                          const std::string_view text, const size_t size_bytes,
                          const operand& address) -> void {

        comment(src_loc_tk, indent, "{} ({} B @ [{}])", text, size_bytes,
                address_text(address));
    }

    // runs 'emit' for its checks, nothing it emits is kept and the registers
    // it used are not counted. it starts with the registers that the body of
    // a 'noinline' function holds, the body it stands for, so the registers
    // that the caller holds do not make it run out
    auto emit_discarded(const std::function_ref<void()> emit) -> void {
        const version_scope discarded{*this};

        state_.registers = noinline_body_pool();
        state_.frame_base_reserved = true;

        std::ignore = target_assembler().capture(emit);
    }

    // emits both versions and keeps the one with less code, the first on ties,
    // or the one that '--versions' names; a version that runs out of
    // registers is not kept, when both do the error of the first is thrown.
    // each version starts from the state before it, the state of the kept
    // one remains
    auto emit_most_efficient(const token& src_loc_tk, const size_t indent,
                             const std::function_ref<void()> emit_first,
                             const std::function_ref<void()> emit_second)
        -> void {

        assembler& output{target_assembler()};

        version_result first{compile_version(emit_first)};
        version_result second{compile_version(emit_second)};

        if (not first and not second) {
            std::rethrow_exception(first.error());
        }

        const bool keeps_first{keeps_first_version(first, second)};

        comment(src_loc_tk, indent,
                version_comment(first, second, keeps_first));

        compiled_version& kept{keeps_first ? *first : *second};

        restore_version_state(std::move(kept.end_state));

        output.append(std::move(kept.lines));
    }

    // the code after the program: the handlers its checks branch to, then the
    // routines that the code and the handlers call
    auto emit_runtime() -> void {
        emit_used_failure_handlers();
        emit_used_routines();
        assert_used_routines_emitted();
    }

    // the lines buffered so far, for backend tests that read what an emit
    // adds with 'emitted_text'
    [[nodiscard]] auto emitted_line_count() const -> size_t {
        return target_assembler().line_count();
    }

    // the text of the lines buffered from 'first_line' on
    [[nodiscard]] auto emitted_text(const size_t first_line) const
        -> std::string {

        return target_assembler().text_from(first_line);
    }

    auto end_noinline_body() -> void { target_assembler().end_body(); }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void {

        assert(reg.is_register() and not reg.allocation_register().empty());

        release_named_register(src_loc_tk, indent, reg.allocation_register());
    }

    auto free_named_registers(const token& src_loc_tk, const size_t indent,
                              const std::span<const operand> registers)
        -> void {

        for (const operand& reg : registers | std::views::reverse) {
            free_named_register(src_loc_tk, indent, reg);
        }
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indent,
                               const operand& reg) -> void {

        assert(reg.is_register() and not reg.allocation_register().empty());

        comment(src_loc_tk, indent, "free scratch register {}",
                reg.allocation_register());

        assert(not state_.registers.top().named);

        pop_allocation(reg.allocation_register());
    }

    auto free_scratch_registers(const token& src_loc_tk, const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& reg : registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, reg);
        }
    }

    auto label(const size_t indent, const std::string_view name) -> void {
        target_assembler().label(indent, name);
    }

    // the use of registers at the busiest point of the build, as lines after
    // the code when asked for, else none; defined in 'register_report.hpp'
    [[nodiscard]] auto register_peak_report() const -> std::vector<std::string>;

    // the register of 'reg' at the width of 'type_ref', freed by the same
    // allocation name, e.g. 'al' of 'rax' on x86_64
    [[nodiscard]] auto register_view(const operand& reg,
                                     const type& type_ref) const -> operand {

        assert(reg.is_register());

        operand result{make_register_operand(reg.base_register(), type_ref)};

        result.set_allocation_register(reg.allocation_register());

        return result;
    }

    // the registers of the arguments and the result of a call of 'function'
    [[nodiscard]] auto
    registers_for_target_call(const target_call function) const
        -> target_call_registers {

        const target_call_names& names{target_call_register_names()};

        if (function == target_call::exit) {
            return {
                .arguments{std::span{names.arguments}.first(1)},
                .result{},
            };
        }

        if (function == target_call::sector_read or
            function == target_call::sector_write) {

            return {
                .arguments{names.arguments},
                .result{},
            };
        }

        return {
            .arguments{names.arguments},
            .result{names.result},
        };
    }

    // the frame base is held while a 'noinline' body is compiled, which starts
    // and ends with no scratch register held
    auto release_frame_base() -> void {
        assert(state_.frame_base_reserved);
        assert(state_.registers.scratch_count() == 0);

        comment(token{}, 0, "free named register {}", frame_base_register());
        pop_allocation(frame_base_register());

        state_.frame_base_reserved = false;
    }

    auto release_variables_base() -> void {
        assert(variables_base_reserved_);

        release_named_register(token{}, 0, variables_base_register());

        variables_base_reserved_ = false;
    }

    auto reserve_frame_base() -> void {
        assert(not state_.frame_base_reserved);
        assert(state_.registers.scratch_count() == 0);

        push_allocation(token{}, 0, frame_base_register(), true);

        state_.frame_base_reserved = true;

        comment(token{}, 0, "allocate named register {}",
                frame_base_register());

        emit_frame_entry();
    }

    // held from 'start' to 'finish'
    auto reserve_variables_base() -> void {
        assert(not variables_base_reserved_);

        reserve_named_register(token{}, 0, variables_base_register(),
                               default_type());

        variables_base_reserved_ = true;
    }

    // an index or a count of elements becomes a byte offset
    auto scale_index(const token& src_loc_tk, const size_t indent,
                     const operand& index, const size_t element_size_bytes)
        -> void {

        // note: element sizes are limited by the variables that hold the
        //       arrays

        assert(index.type_ref().size_bytes() == address_size_bytes());
        assert(element_size_bytes <= max_storage_bytes());

        multiply(src_loc_tk, indent, index,
                 operand::imm(element_size_bytes, default_type()),
                 factor_use::read_only);
    }

    // a blank line that separates the code from the report
    auto separate_report() -> void {
        target_assembler().add_separator_newline();
    }

    // what the output measured, for the report of the front end, valid after
    // 'finish'
    [[nodiscard]] auto statistics() const -> output_statistics {
        const assembler& output{target_assembler()};

        return {
            .optimizations{output.optimizations()},
            .noinline_functions{output.noinline_summaries()},
            .max_scratch_registers{state_.max_scratch_regs},
            .instruction_count{output.instruction_count()},
        };
    }

    // a 'bool' is stored as 0 or 1
    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void {

        copy_value(src_loc_tk, indent, dst,
                   operand::imm(value ? 1 : 0, default_type()));
    }

    // the padding of the instance of 'size_bytes' at 'dst', zeroed gap by gap
    // or as the whole instance, whichever the target emits smaller; the fields
    // are written over the zeroes after
    auto zero_padding(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const size_t size_bytes,
                      const size_t alignment,
                      const std::span<const padding_gap> gaps) -> void {

        // note: a measured threshold, not 'emit_most_efficient': both forms
        //       would multiply the versions of the inlined calls around it,
        //       and an instance with thousands of gaps would cost compile time

        if (gaps.size() > max_padding_gaps_zeroed_one_by_one()) {
            comment(src_loc_tk, indent, "zero instance for its padding: {} B",
                    size_bytes);

            zero(src_loc_tk, indent, dst, size_bytes, alignment);

            return;
        }

        for (const padding_gap& gap : gaps) {
            operand gap_dst{dst};
            gap_dst.increment_offset(address_offset(gap.offset));

            comment(src_loc_tk, indent, "zero padding: {} B", gap.size_bytes);
            zero(src_loc_tk, indent, gap_dst, gap.size_bytes, gap.alignment);
        }
    }

  protected:
    //
    // virtual methods
    //

    // a memory operand as the comments show it, e.g. 'rbp + 8'
    [[nodiscard]] virtual auto address_text(const operand& address) const
        -> std::string = 0;

    // the call instruction of 'call_function'
    virtual auto emit_call(const size_t indent, const std::string_view label)
        -> void = 0;

    // prints 'panic: NAME' of 'handler', with the line when 'with_line'
    virtual auto emit_failure_handler(const failure_handler& handler,
                                      const bool with_line) -> void = 0;

    // the code at the start of a 'noinline' body after the frame base is
    // reserved, none by default
    virtual auto emit_frame_entry() -> void {}

    // the call of 'call_target', its registers checked
    virtual auto emit_target_call(const token& src_loc_tk, const size_t indent,
                                  const target_call function,
                                  const std::span<const operand> args,
                                  const operand& result) -> void = 0;

    // the helpers and routines that the code marked used; a routine may mark
    // another one it calls, see 'emit_runtime'
    virtual auto emit_used_routines() -> void = 0;

    // the asserts of 'call_function': a frame is never addressed from the
    // stack pointer
    [[nodiscard]] virtual auto
    is_stack_pointer(const std::string_view name) const -> bool = 0;

    // the index of the register 'name' in any of its sizes, empty for text
    // that names no register, e.g. a label
    [[nodiscard]] virtual auto
    register_index_of(const std::string_view name) const
        -> std::optional<size_t> = 0;

    // puts back the registers 'save_call_registers' saved
    virtual auto restore_call_registers(const size_t indent,
                                        const std::span<const size_t> saved)
        -> void = 0;

    // saves the registers of the indexes 'saved' before a call
    virtual auto save_call_registers(const size_t indent,
                                     const std::span<const size_t> saved)
        -> void = 0;

    // the registers 'alloc_scratch_register' picks from, in the order it tries
    // them
    [[nodiscard]] virtual auto scratch_registers() const
        -> std::span<const std::string_view> = 0;

    // the assembler the target writes its output with
    [[nodiscard]] virtual auto target_assembler() -> assembler& = 0;

    [[nodiscard]] virtual auto target_assembler() const -> const assembler& = 0;

    // the registers 'registers_for_target_call' takes its answers from
    [[nodiscard]] virtual auto target_call_register_names() const
        -> const target_call_names& = 0;

    // throws when a register of the target cannot hold a value of 'type_ref',
    // any type fits by default
    virtual auto
    validate_register_type([[maybe_unused]] const token& src_loc_tk,
                           [[maybe_unused]] const type& type_ref) const
        -> void {}

    //
    // class methods
    //

    // the arguments of a call of 'function' and its result, empty when it has
    // none, are in the registers 'registers_for_target_call' gives it
    auto assert_target_call_registers(const target_call function,
                                      const std::span<const operand> arguments,
                                      const operand& result) const -> void {

        const target_call_registers registers{
            registers_for_target_call(function),
        };

        assert(arguments.size() == registers.arguments.size());

        for (const auto [arg, name] :
             std::views::zip(arguments, registers.arguments)) {

            assert(arg.is_register() and
                   register_index_of(arg.base_register()) ==
                       register_index_of(name));
        }

        assert(result.is_empty() == registers.result.empty());

        assert(result.is_empty() or register_index_of(result.base_register()) ==
                                        register_index_of(registers.result));
    }

    // a routine marked while another one is emitted is emitted only if a
    // later step of 'emit_runtime' asks for it
    auto assert_used_routines_emitted() const -> void {
        for (const auto& [routine, uses] : state_.used_routines) {
            // note: the code marks the handlers, they are emitted first and a
            //       handler that reports lines is under the labels of its lines

            const bool is_handler{
                std::ranges::any_of(
                    failure_handlers,
                    [&](const failure_handler& handler) -> bool {
                        return handler.label == routine;
                    }),
            };

            assert(is_handler or target_assembler().defines_label(routine));
        }
    }

    [[nodiscard]] auto builtin_type_i16() const -> const type& {
        return builtins_.get().i16;
    }

    [[nodiscard]] auto builtin_type_i32() const -> const type& {
        return builtins_.get().i32;
    }

    [[nodiscard]] auto builtin_type_i64() const -> const type& {
        return builtins_.get().i64;
    }

    [[nodiscard]] auto builtin_type_i8() const -> const type& {
        return builtins_.get().i8;
    }

    // the comments of a lower bound check, which the upper comparison or an
    // earlier check may cover
    auto comment_lower_bound(const token& src_loc_tk, const size_t indent,
                             const bounds_range& range, const bounds_plan& plan)
        -> void {

        comment(src_loc_tk, indent, "lower bound");

        if (plan.upper_covers_lower) {
            comment(src_loc_tk, indent,
                    "{} lower bound covered by the unsigned upper bound",
                    range.checked.base_register());

            return;
        }

        if (plan.count_known) {
            comment(src_loc_tk, indent, "count {} lower bound already checked",
                    range.count.base_register());
        }
    }

    // the handlers that the code branches to, see 'mark_used'; before
    // 'emit_used_routines', a handler marks the routines it calls
    auto emit_used_failure_handlers() -> void {
        for (const failure_handler& handler : failure_handlers) {
            if (not is_used(handler.label)) {
                continue;
            }

            comment({}, 0, handler.comment);
            emit_failure_handler(handler, reports_line_ and handler.takes_line);
        }
    }

    // the lines of the failures of 'routine' by the file they are in
    [[nodiscard]] auto failure_lines_of(const std::string_view routine) const
        -> failure_lines {

        const auto it{state_.used_routines.find(routine)};
        return it == state_.used_routines.end() ? failure_lines{} : it->second;
    }

    // e.g. 'panic: bounds at prog.baz:', the line follows
    [[nodiscard]] auto failure_message(const std::string_view name,
                                       const size_t file_ix) const
        -> std::string {

        return std::format("{} at {}:", panic_message(name),
                           locations_.file_name(file_ix));
    }

    // the optional jump optimization
    auto finish_output() -> void {
        assembler& output{target_assembler()};

        if (jump_mode_ == jump_mode::optimized) {
            output.optimize_jumps();
        }
    }

    // releases the variables base that 'start' reserved, backend tests that
    // do not start hold none, and asserts that every register is free
    auto finish_registers() -> void {
        if (variables_base_reserved_) {
            release_variables_base();
        }

        assert_registers_free();
    }

    [[nodiscard]] auto is_frame_base_reserved() const -> bool {
        return state_.frame_base_reserved;
    }

    [[nodiscard]] auto is_used(const std::string_view routine) const -> bool {
        return state_.used_routines.contains(routine);
    }

    [[nodiscard]] auto is_variables_base_reserved() const -> bool {
        return variables_base_reserved_;
    }

    // loaded before the frame base is set, the address may be relative to the
    // caller's; the receiver already in the register is passed as is
    auto load_slot_address(const token& src_loc_tk, const size_t indent,
                           const operand& slot_address) -> void {

        if (slot_address.is_empty() or
            slot_address.is_address_in_register(slot_register())) {

            return;
        }

        comment(src_loc_tk, indent, "address of the slot in a register");

        address_of(src_loc_tk, indent,
                   make_register_operand(slot_register(), default_type()),
                   slot_address);
    }

    // a passed lower bounds check leaves the checked register non-negative
    // until it is allocated again
    auto mark_lower_checked(const bounds_range& range) -> void {
        state_.registers.mark_lower_checked(
            register_mask(lower_checked_register(range)));
    }

    // the routine is emitted after the code; the use is version state, so a
    // version that is not kept and discarded code use nothing
    auto mark_used(const std::string_view routine) -> void {
        state_.used_routines.try_emplace(std::string{routine});
    }

    // the failure at 'src_loc_tk' is reported by the handler of 'routine',
    // which names its file and line, see 'mark_used'
    auto mark_used(const std::string_view routine, const token& src_loc_tk)
        -> void {

        state_.used_routines[std::string{routine}][src_loc_tk.file_index()]
            .insert(src_loc_tk.at_line());
    }

    [[nodiscard]] auto out_of_registers_error(const token& src_loc_tk) const
        -> compile_error {

        compile_error error{
            register_error(src_loc_tk,
                           "out of scratch registers, simplify the expression"),
        };

        error.kind = compile_error::error_kind::out_of_registers;

        return error;
    }

    // 'signed_max' is the largest value of the signed type of the compare
    [[nodiscard]] auto plan_bounds_check(const bounds_check_options& options,
                                         const bounds_range& range,
                                         const uint64_t signed_max) const
        -> bounds_plan {

        const bool count_known{
            not range.count.is_empty() and
                state_.registers.is_lower_checked(
                    register_mask(range.count.base_register())),
        };

        return {
            .upper_covers_lower{
                options.lower and options.upper and range.count.is_empty() and
                    range.array_count <= signed_max,
            },
            .count_known{count_known},
        };
    }

    // keeps the register 'name' from being picked for scratch until the
    // enclosing 'address_scope' ends
    auto protect_register(const std::string_view name) -> void {
        state_.registers.protect(register_mask(name));
    }

    // a call of the function with the body 'label' saves 'saved' registers
    auto record_noinline_call(const token& call_site_tk,
                              const std::string_view label, const size_t saved)
        -> void {

        state_.trace.record_noinline_call(call_site_tk, label, saved);
    }

    // the mask of the register 'name' in the pool, 0 for text that names no
    // register
    [[nodiscard]] auto register_mask(const std::string_view name) const
        -> uint32_t {

        const std::optional<size_t> index{register_index_of(name)};
        return index ? uint32_t{1} << *index : 0;
    }

    [[nodiscard]] auto registers() const -> const register_pool& {
        return state_.registers;
    }

    // the registers a 'noinline' call saves: the allocated ones but the
    // variables base, which the callee uses too
    [[nodiscard]] auto registers_saved_by_call() const -> std::vector<size_t> {
        const std::optional<size_t> variables_base{
            register_index_of(variables_base_register()),
        };

        std::vector<size_t> saved;
        for (const register_pool::allocation& allocated :
             state_.registers.allocations()) {

            if (allocated.index != variables_base) {
                saved.push_back(allocated.index);
            }
        }

        return saved;
    }

    auto release_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view register_name) -> void {

        comment(src_loc_tk, indent, "free named register {}", register_name);

        assert(state_.registers.top().named);

        pop_allocation(register_name);
    }

    // see 'target_options::reports_line'
    [[nodiscard]] auto reports_line() const -> bool { return reports_line_; }

    // 'alloc_named_register' without an operand, for a register that the
    // backend uses by name
    auto reserve_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view register_name,
                                const type& type_ref) -> void {

        validate_register_type(src_loc_tk, type_ref);

        comment(src_loc_tk, indent, "allocate named register {}",
                register_name);

        if (state_.registers.is_unavailable(register_mask(register_name))) {
            throw register_in_use_error(src_loc_tk, register_name);
        }

        push_allocation(src_loc_tk, indent, register_name, true);
    }

    [[nodiscard]] auto sd_card_file_name() const -> std::string_view {
        return sd_card_file_name_;
    }

    auto start_output() -> void {
        assembler& output{target_assembler()};

        // note: the parse declares names without emitting
        assert(output.is_empty());

        // note: the parse emits no code, so it allocates no register and uses
        //       no routine

        assert(state_.max_scratch_regs == 0);
        assert(state_.used_routines.empty());

        output.comment(0, "");
        output.comment(0, "generated by baz");
        output.comment(0, "");
        output.add_separator_newline();
    }

    //
    // statics
    //

    // zero clears, one needs no code, minus one negates and a power of two is a
    // shift, only the other factors need a multiplication
    [[nodiscard]] static auto classify_factor(const uint64_t bits,
                                              const size_t width_bits)
        -> constant_factor {

        const uint64_t mask{
            width_bits >= std::numeric_limits<uint64_t>::digits
                ? std::numeric_limits<uint64_t>::max()
                : (uint64_t{1} << width_bits) - 1,
        };

        const uint64_t multiplier{bits & mask};

        factor_kind kind{factor_kind::other};

        if (multiplier == 0) {
            kind = factor_kind::zero;
        } else if (multiplier == 1) {
            kind = factor_kind::one;
        } else if (multiplier == mask) {
            kind = factor_kind::minus_one;
        } else if (std::has_single_bit(multiplier)) {
            kind = factor_kind::power_of_two;
        }

        return {
            .kind{kind},
            .multiplier{multiplier},
            .shift{
                kind == factor_kind::power_of_two ? std::countr_zero(multiplier)
                                                  : 0,
            },
        };
    }

    // the entry of the handler 'handler_label' for the failures in a file,
    // which names the file before the line
    [[nodiscard]] static auto
    failure_entry_label(const std::string_view handler_label,
                        const size_t file_ix) -> std::string {

        return std::format("{}_file_{}", handler_label, file_ix);
    }

    // unrolled accesses take the widest parts first so each narrower width
    // covers at most one remaining part
    static auto for_each_part(
        const size_t size_bytes, const size_t widest_size_bytes,
        const std::function_ref<void(size_t part_size_bytes, size_t offset)>
            emit_part) -> void {

        size_t offset{};
        for (size_t w{widest_size_bytes}; w != 0; w /= 2) {
            while (size_bytes - offset >= w) {
                emit_part(w, offset);
                offset += w;
            }
        }
    }

    // the two's complement bits of an immediate, empty for an operand that is
    // not one
    [[nodiscard]] static auto immediate_bits(const operand& value)
        -> std::optional<uint64_t> {

        if (not value.is_immediate()) {
            return std::nullopt;
        }

        return static_cast<uint64_t>(value.immediate());
    }

    // 4 and 8 bytes are read as signed so a value that fits a signed 32-bit
    // immediate is recognized
    [[nodiscard]] static auto little_endian_value(const std::string_view bytes)
        -> int64_t {

        uint64_t bits{};
        for (size_t i{}; i < bytes.size(); ++i) {
            bits |= uint64_t{static_cast<unsigned char>(bytes.at(i))}
                    << (bits_per_byte * i);
        }

        if (bytes.size() == sizeof(int32_t)) {
            return std::bit_cast<int32_t>(static_cast<uint32_t>(bits));
        }

        return std::bit_cast<int64_t>(bits);
    }

    // the register whose lower bound a check settles, a count alone is checked
    // as the index of its own range
    [[nodiscard]] static auto lower_checked_register(const bounds_range& range)
        -> std::string_view {

        return range.count.is_empty() ? range.checked.base_register()
                                      : range.count.base_register();
    }

    // the operator that is true when 'op' is false
    [[nodiscard]] static auto negated(const comparison_operator op)
        -> comparison_operator {

        if (op == comparison_operator::equal) {
            return comparison_operator::not_equal;
        }

        if (op == comparison_operator::not_equal) {
            return comparison_operator::equal;
        }

        if (op == comparison_operator::less) {
            return comparison_operator::greater_equal;
        }

        if (op == comparison_operator::less_equal) {
            return comparison_operator::greater;
        }

        if (op == comparison_operator::greater) {
            return comparison_operator::less_equal;
        }

        assert(op == comparison_operator::greater_equal);

        return comparison_operator::less;
    }

    // e.g. 'panic: bounds'; the newline in front separates it from the output
    // of the program, which may not end with one
    [[nodiscard]] static auto panic_message(const std::string_view name)
        -> std::string {

        return std::format("\npanic: {}", name);
    }

  public:
    // the queries of the register report
    friend class register_reporter;

  private:
    //
    // virtual methods
    //

    // the name of a register in the output, for the use of registers
    [[nodiscard]] virtual auto register_display_name(size_t index) const
        -> std::string = 0;

    //
    // class methods
    //

    // the operand of a register handed out by 'alloc_named_register' or
    // 'alloc_scratch_register', which frees it by its allocation name
    [[nodiscard]] auto allocated_operand(const std::string_view register_name,
                                         const type& type_ref) const
        -> operand {

        operand result{make_register_operand(register_name, type_ref)};

        result.set_allocation_register(register_name);

        return result;
    }

    // the registers allocated until 'end_trace_frame' belong to the inlined
    // call of 'name' made at 'call_site_tk', the first frame is the function
    // being compiled and has no call site
    auto begin_trace_frame(const token& call_site_tk, std::string name,
                           const frame_kind kind) -> void {

        state_.trace.begin_frame(call_site_tk, std::move(name), kind);
    }

    // the lines of 'emit' with the state they end in, compiled from the
    // current state, which is put back
    [[nodiscard]] auto compile_version(const std::function_ref<void()> emit)
        -> version_result {

        const version_scope version{*this};

        try {
            assembler::captured_lines lines{target_assembler().capture(emit)};

            return compiled_version{
                .lines{std::move(lines)},
                .end_state{version_state_now()},
            };
        } catch (const compile_error& e) {
            if (e.kind != compile_error::error_kind::out_of_registers) {
                throw;
            }

            return std::unexpected{std::current_exception()};
        }
    }

    // the call frame that allocations are made in
    [[nodiscard]] auto current_trace_frame() const -> size_t {
        return state_.trace.current_frame();
    }

    auto end_trace_frame() -> void { state_.trace.end_frame(); }

    [[nodiscard]] auto is_scratch_register(const size_t index) const -> bool {
        return std::ranges::any_of(scratch_registers(),
                                   [&](const std::string_view name) -> bool {
                                       return register_index_of(name) == index;
                                   });
    }

    // of two versions, at least one available, the one to keep
    [[nodiscard]] auto keeps_first_version(const version_result& first,
                                           const version_result& second) const
        -> bool {

        if (not first or not second) {
            return first.has_value();
        }

        if (kept_version_ == kept_version::first) {
            return true;
        }

        if (kept_version_ == kept_version::last) {
            return false;
        }

        const assembler& output{target_assembler()};

        return output.instruction_count_of(first->lines) <=
               output.instruction_count_of(second->lines);
    }

    [[nodiscard]] auto location_text(const token& src_loc_tk) const
        -> std::string {

        if (src_loc_tk.at_line() == 0) {
            return "-";
        }

        return locations_.human_readable(src_loc_tk);
    }

    // the backend's own registers, e.g. the variables base, and the frame
    // base that a 'noinline' body holds from its start
    [[nodiscard]] auto noinline_body_pool() const -> register_pool {
        register_pool pool;

        for (const register_pool::allocation& allocated :
             state_.registers.allocations()) {

            if (not register_trace::is_listed(allocated)) {
                pool.push(allocated);
            }
        }

        const size_t frame_base_index{pool_index(frame_base_register())};

        if (pool.is_unavailable(uint32_t{1} << frame_base_index)) {
            return pool;
        }

        pool.push({
            .index{frame_base_index},
            .src_loc_tk{},
            .indent{},
            .named{true},
            .frame{current_trace_frame()},
        });

        return pool;
    }

    // a 'noinline' body holds the variables base and the frame base from its
    // start
    [[nodiscard]] auto noinline_body_register_count() const -> size_t {
        size_t count{};

        for (const std::string_view name : scratch_registers()) {
            if (name != variables_base_register() and
                name != frame_base_register()) {

                ++count;
            }
        }

        return count;
    }

    // the index of 'register_name', which names a register
    [[nodiscard]] auto pool_index(const std::string_view register_name) const
        -> size_t {

        const std::optional<size_t> index{register_index_of(register_name)};

        assert(index.has_value());

        return *index;
    }

    // registers are released in reverse order of allocation
    auto pop_allocation(const std::string_view register_name) -> void {
        state_.registers.pop(pool_index(register_name));
    }

    // named and scratch allocations share one stack, each is recorded for
    // the report of the use of registers
    auto push_allocation(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name, const bool named)
        -> void {

        state_.registers.push({
            .index{pool_index(register_name)},
            .src_loc_tk{src_loc_tk},
            .indent{indent},
            .named{named},
            .frame{current_trace_frame()},
        });

        state_.trace.record_allocation(state_.registers);
    }

    // an error that the registers ran out or are held by a value, with how the
    // registers are used by each frame and what a 'noinline' frame would give;
    // defined in 'register_report.hpp'
    [[nodiscard]] auto register_error(const token& src_loc_tk,
                                      std::string message) const
        -> compile_error;

    // an error that the register 'name', which an instruction or a built-in
    // needs, is held: by a scratch value of the expression, or by an operation
    // whose operands are being computed, which a value computed with 'let'
    // before the operation does not clash with
    [[nodiscard]] auto register_in_use_error(const token& src_loc_tk,
                                             const std::string_view name) const
        -> compile_error {

        const register_pool::allocation& holder{
            state_.registers.holder_of(pool_index(name)),
        };

        // note: the registers that the backend holds without a place in the
        //       source, e.g. the variables base, are needed by no instruction

        assert(holder.src_loc_tk.at_line() != 0);

        const std::string holder_location{location_text(holder.src_loc_tk)};

        if (not holder.named) {
            compile_error error{
                register_error(
                    src_loc_tk,
                    std::format("cannot allocate register {} because it holds "
                                "a scratch value allocated at {}, compute a "
                                "part of the expression with 'let' before it",
                                name, holder_location)),
            };

            // note: which register a scratch value takes depends on how many
            //       are live, as running out of them does

            error.kind = compile_error::error_kind::out_of_registers;

            return error;
        }

        return register_error(
            src_loc_tk,
            std::format("register {} is always used by the operation at {} and "
                        "is needed here too, compute the operand that needs "
                        "it with 'let' before that operation",
                        name, holder_location));
    }

    auto restore_version_state(version_state state) -> void {
        state_ = std::move(state);
    }

    // the scratch registers that can hold values beside the ones the backend
    // holds itself in 'allocations', e.g. the variables base 's0' on rv32i
    [[nodiscard]] auto scratch_register_count(
        const std::span<const register_pool::allocation> allocations) const
        -> size_t {

        size_t count{scratch_registers().size()};

        for (const register_pool::allocation& allocated : allocations) {
            if (not register_trace::is_listed(allocated) and
                is_scratch_register(allocated.index)) {

                --count;
            }
        }

        return count;
    }

    // the version kept and why, at least one available, e.g. 'kept the second
    // of two versions of 4 and 3 instructions, the smaller'
    [[nodiscard]] auto version_comment(const version_result& first,
                                       const version_result& second,
                                       const bool keeps_first) const
        -> std::string {

        if (not second) {
            return "kept the first of two versions, the second runs out of "
                   "registers";
        }

        if (not first) {
            return "kept the second of two versions, the first runs out of "
                   "registers";
        }

        const assembler& output{target_assembler()};

        const size_t first_count{output.instruction_count_of(first->lines)};
        const size_t second_count{output.instruction_count_of(second->lines)};

        std::string_view reason{"the smaller"};

        if (kept_version_ == kept_version::first) {
            reason = "as '--versions=first' asks";
        } else if (kept_version_ == kept_version::last) {
            reason = "as '--versions=last' asks";
        } else if (first_count == second_count) {
            reason = "the first of equal sizes";
        }

        return std::format("kept the {} of two versions of {} and {} "
                           "instructions, {}",
                           keeps_first ? "first" : "second", first_count,
                           second_count, reason);
    }

    [[nodiscard]] auto version_state_now() const -> version_state {
        return state_;
    }
};
