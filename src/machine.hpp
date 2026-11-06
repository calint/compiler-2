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
// handler of the check, which reports the line when
// 'target_options::reports_line'; the operation that takes it tells what fails
struct runtime_check {
    bool enabled{};
};

// the registers of the calls the target provides, see 'call_target'
struct target_call_names {
    // the descriptor or sector, the address and the count or size; 'exit'
    // takes the first
    std::array<std::string_view, 3> arguments{};
    // the count that 'read' and 'write' return
    std::string_view result;
};

// the routines of every target, emitted after the code when it uses them, see
// 'emit_used_routines'
struct routine_labels {
    // ends the program with the code in 'exit_code_register'
    std::string_view exit;
    // prints the line in 'line' and exits with the code in 'exit_code', see
    // 'emit_failure_handler'
    std::string_view report_line;
    std::string_view sector_read;
    std::string_view sector_write;
};

// the facts of a target that the shared code and the front end read, one
// table per backend; a target that differs passes a copy with its change,
// e.g. the qemu machine has no sd card
struct target_description {
    size_t address_size_bytes{};
    size_t data_alignment{};
    // the most bytes of data and variables the target can address
    size_t max_storage_bytes{};
    // whether an access can add an index register to a base register, else
    // an indexed address is computed into a register by separate instructions
    bool has_indexed_addressing{};
    // the largest element size an indexed address scales by, a power of two
    size_t max_index_scale_bytes{};
    // above this count 'zero_padding' zeroes the whole instance, which is
    // then smaller than a zero per gap
    size_t max_padding_gaps_zeroed_one_by_one{};
    // the live scratch values from which the counter of an array loop is
    // kept in memory, none where it stays in a register
    std::optional<size_t> memory_counter_pressure_count;
    // the sector functions need it, a sector is 'card_sector_size_bytes_'
    bool has_sd_card{};
    // the alignment of a string constant, e.g. a word lets copies to aligned
    // destinations use words
    size_t string_alignment{};
    // true where no operating system stops a program that overflows the
    // stack: calls check that the stack has room and a failure has a handler
    bool is_stack_bounded{};
    std::string_view frame_base_register;
    std::string_view variables_base_register;
    // the register that carries the address of the receiver of a method call
    std::string_view slot_register;
    // empty when the variables base register points at 'dat', else how far
    // past the 'vars' label it points
    std::optional<size_t> variables_base_past_vars_bytes;
    // the registers 'alloc_scratch_register' picks from, in the order it
    // tries them
    std::span<const std::string_view> scratch_registers;
    // the registers of the arguments and the result of 'read', 'write', the
    // sector functions and 'exit'
    target_call_names target_calls{};
    routine_labels routines{};
    // the registers a failure that reports its line passes the line and the
    // exit code to 'routine_labels::report_line' in
    std::string_view report_line_register;
    std::string_view report_exit_code_register;
};

// what the front end emits through, and what every target shares: the
// registers with the variables and frame bases, the trace of their use and
// the routines, helpers and failure stubs the code uses; a backend supplies
// the instructions, the names of its registers and the order it picks scratch
// registers in; the public interface by group:
//
//   lifecycle      in the order of a compile: 'start', 'begin_noinline_body'
//                  and 'end_noinline_body' for each body
//                  ('stmt_def_func'), 'emit_runtime',
//                  'emit_read_only_constants', 'begin_data',
//                  'reserve_variables', 'finish', 'write_assembly'
//   data           'data_alignment', 'define_constant', 'emit_data_array',
//                  'emit_string_data', 'emit_zero_data'
//   registers      'alloc_scratch_register', 'free_scratch_register',
//                  'held_registers' (what a front-end step holds),
//                  'free_named_register', 'slot_register', 'register_view',
//                  'alloc_named_register'
//   operations     'add_subtract', 'multiply', 'divide', 'shift', 'bitwise',
//                  'unary', 'zero', 'zero_padding', 'copy_value',
//                  'store_bytes', 'address_of', 'scale_index', 'store_boolean';
//                  bulk by a constant size
//                  'copy_block', 'memory_equal', by a run-time count
//                  'copy_elements', 'equal_elements'
//   control        'label', 'branch', 'compare_and_branch', 'call_function',
//                  'call_target', 'return_function', 'exit'
//   checks         'check_bounds', 'check_frame_capacity',
//                  'check_stack_capacity'
//   versions       'emit_most_efficient', 'emit_discarded'
//   comments       'comment', 'comment_binding', 'comment_variable',
//                  'comment_held_address'
//   target         'default_type', 'address_size_bytes',
//                  'has_indexed_addressing', 'can_lower_index_scale',
//                  'is_stack_bounded',
//                  'max_storage_bytes', 'has_sd_card', 'sector_size_bytes',
//                  'foo_counter_in_memory', 'frame_base_register',
//                  'variables_base_register', 'variables_base_past_vars_bytes',
//                  'exit_code_register'
//   reports        'register_peak_report', 'register_trace_scope',
//                  'separate_report', 'statistics';
//                  the queries of the register report are private, for
//                  'register_reporter'
class machine {
  public:
    // which of two versions of the same code 'emit_most_efficient' keeps: the
    // smaller, or to test the other version too, the first or the last
    enum class kept_version : uint8_t { smaller, first, last };

    // a built-in the target provides as a system call or a routine
    enum class target_call : uint8_t {
        read,
        write,
        sector_read,
        sector_write,
    };

    // receives an address while the registers that built it are allocated
    using address_use = std::function_ref<void(const operand& address)>;

    // emits one address of a bulk operation: 'preferred' a register the
    // address may be built in, none when the backend has none to offer
    using address_emitter = std::function<void(
        const std::optional<operand>& preferred, address_use use)>;

    // whether an operation may overwrite a register operand, e.g. a scratch
    // register that holds a factor and is freed after the operation
    enum class factor_use : uint8_t { read_only, may_overwrite };

    // what discarded code stands for, which decides the registers it starts
    // with ('emit_discarded'): the body of a function checked without its
    // calls, or code that a compile skips where it stands
    enum class discarded_code : uint8_t { body, here };

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
        std::string sd_card_file_name;
        // the image that the rv32i targets write
        std::string binary_file_name;
        // the stack of the bare-metal targets
        size_t stack_size_bytes{};
        // the memory of 'rv32i-fpga'
        size_t memory_size_bytes{};
        // '--checks=line': a failed check reports the line of its source
        bool reports_line{};
    };

    struct output_statistics {
        assembler::optimization_counts optimizations{};
        std::vector<assembler::function_summary> noinline_functions;
        size_t max_scratch_registers{};
        size_t instruction_count{};
    };

    // what 'compare_and_branch' does with 'lhs operation rhs'; a negated
    // condition passes the negated operator, e.g. 'not a < b' is 'a >= b'
    struct comparison_action {
        comparison_operator operation{comparison_operator::equal};

        // receives 0 or 1, empty skips storing
        operand destination;

        // branch label, empty skips branching
        std::string_view target;

        // branch when the comparison holds, otherwise when it does not
        bool branch_on_true{};
    };

    struct bounds_check_options {
        bool upper{};
        bool lower{};
    };

    // what a bounds check compares with 'array_count' elements: an index, a
    // range 'start + count', or one value of a range whose other part is
    // constant; a range and its value may end at the end of the array, an
    // index may not
    struct bounds_range {
        // the index, the count alone or the start of the range
        operand checked;
        // the count of a range, none otherwise
        std::optional<operand> count;
        // the start is known not to be negative, e.g. checked by the walk of
        // its address
        bool start_lower_checked{};
        // the count is known not to be negative, e.g. checked for the first
        // array of a copy
        bool count_lower_checked{};
        size_t array_count{};
        bool allow_end{};

        //
        // statics
        //

        [[nodiscard]] static auto index(const operand& index,
                                        const size_t array_count)
            -> bounds_range {

            return {
                .checked{index},
                .count{},
                .start_lower_checked{},
                .count_lower_checked{},
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
                .start_lower_checked{},
                .count_lower_checked{},
                .array_count{array_count},
                .allow_end{true},
            };
        }

        // a count or a start that may reach 'limit', the array count less
        // the constant other part of its range, e.g. a count alone
        [[nodiscard]] static auto up_to(const operand& value,
                                        const size_t limit) -> bounds_range {

            return {
                .checked{value},
                .count{},
                .start_lower_checked{},
                .count_lower_checked{},
                .array_count{limit},
                .allow_end{true},
            };
        }
    };

    // read-only bytes at 'label'
    struct read_only_constant {
        std::string label;
        std::string bytes;
    };

  private:
    // what compiling a version of code changes in the machine
    struct version_state {
        // the indexes are those of the names of the backend, see
        // 'register_index_of'
        register_pool registers;
        // the inlined calls being compiled and what the registers held
        register_trace trace;
        // the most scratch registers in use
        size_t max_scratch_registers{};
        // the routines, helpers and failure handlers the code uses, by label;
        // emitted after the code, see 'mark_used'
        std::set<std::string, std::less<>> used_routines;
    };

    // saved before a version and put back after it, see 'version_scope'
    version_state state_{};

  public:
    // the registers allocated while it lives belong to the call it names in
    // the register trace, see 'register_peak_report'; it ends the frame of
    // the trace when it goes out of scope, an error leaves it too
    class register_trace_scope final {
        machine& machine_;

      public:
        register_trace_scope(machine& backend, const token& call_site_tk,
                             std::string name, const frame_kind kind)
            : machine_{backend} {

            backend.state_.trace.begin_frame(call_site_tk, std::move(name),
                                             kind);
        }

        register_trace_scope(const register_trace_scope&) = delete;
        register_trace_scope(register_trace_scope&&) = delete;
        auto operator=(const register_trace_scope&)
            -> register_trace_scope& = delete;
        auto operator=(register_trace_scope&&)
            -> register_trace_scope& = delete;

        ~register_trace_scope() { machine_.state_.trace.end_frame(); }
    };

    // the scratch registers a front-end step allocates for its operands and
    // holds until it ends, e.g. the address registers of a path; it frees
    // them in reverse when it goes out of scope, with the token and indent it
    // was made with. like 'operation_scope' for the backend, but it frees only
    // what was added to it
    //
    // note: an error leaves the registers allocated: a version or a dry run
    //       that catches it puts the state back, any other error stops the
    //       compile
    class held_registers final {
        machine& machine_;
        token src_loc_tk_;
        size_t indent_{};
        int uncaught_at_start_{};
        std::vector<operand> registers_;

      public:
        held_registers(machine& backend, const token& src_loc_tk,
                       const size_t indent)
            : machine_{backend}, src_loc_tk_{src_loc_tk}, indent_{indent},
              uncaught_at_start_{std::uncaught_exceptions()} {}

        held_registers(const held_registers&) = delete;
        held_registers(held_registers&&) = delete;
        auto operator=(const held_registers&) -> held_registers& = delete;
        auto operator=(held_registers&&) -> held_registers& = delete;

        ~held_registers() {
            if (std::uncaught_exceptions() > uncaught_at_start_) {
                return;
            }

            machine_.free_scratch_registers(src_loc_tk_, indent_, registers_);
        }

        auto add(const operand& reg) -> void { registers_.push_back(reg); }

        [[nodiscard]] auto registers() const -> std::span<const operand> {
            return registers_;
        }

        // frees the registers before the scope ends, e.g. before a call
        // that would save them as live
        auto release() -> void {
            machine_.free_scratch_registers(src_loc_tk_, indent_, registers_);
            registers_.clear();
        }
    };

  private:
    // a failure that reports its line, see 'failure_target'
    struct failure_location {
        std::string_view routine;
        size_t file_ix{};
        size_t line{};
    };

    // in the order they are marked; not in 'version_state', which each version
    // copies: 'version_scope' drops those its version added
    std::vector<failure_location> failure_locations_;

    target_description target_{};
    source_locations locations_;
    std::reference_wrapper<const builtin_type_set> builtins_;
    // the assembler member of the backend
    std::reference_wrapper<assembler> output_;
    target_options options_;

    // the state before a version, put back when the scope ends, also when an
    // error unwinds; a version that is kept is given its end state again
    // ('emit_most_efficient'), discarded code is not ('emit_discarded')
    class version_scope final {
        machine& machine_;
        version_state start_{};
        size_t failures_start_{};

      public:
        explicit version_scope(machine& backend)
            : machine_{backend}, start_{backend.state_},
              failures_start_{backend.failure_locations_.size()} {}

        version_scope(const version_scope&) = delete;
        version_scope(version_scope&&) = delete;
        auto operator=(const version_scope&) -> version_scope& = delete;
        auto operator=(version_scope&&) -> version_scope& = delete;

        ~version_scope() {
            machine_.state_ = std::move(start_);
            machine_.failure_locations_.resize(failures_start_);
        }

        // the failures marked since the scope began, given back by a kept
        // version
        [[nodiscard]] auto added_failures() const
            -> std::vector<failure_location> {

            const std::span<const failure_location> added{
                std::span{machine_.failure_locations_}.subspan(failures_start_),
            };

            return {added.begin(), added.end()};
        }
    };

    // the lines of a compiled version with the state it ended in
    struct compiled_version {
        assembler::captured_lines lines;
        version_state end_state{};
        std::vector<failure_location> failures;
    };

    // a version that fails with an error that depends on the code compiled,
    // e.g. running out of registers, is not available, its error is kept to
    // be thrown when the other version fails too
    struct unavailable_version {
        std::exception_ptr error;
        // what it ran out of, for the comment of the choice
        std::string_view resource;
    };

    using version_result = std::expected<compiled_version, unavailable_version>;

    // the version 'emit_most_efficient' keeps and the comment of why
    struct version_choice {
        bool keeps_first{};
        std::string comment;
    };

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

  protected:
    using section = assembler::section;

    // the result of a comparison of two arrays, 'alignment' is the alignment
    // known for both
    struct equality_request {
        size_t alignment{};
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
        runtime_check overlap{};
    };

    // keeps the registers of its operands, and the base and index registers
    // of memory operands, from being picked for scratch while an operation is
    // lowered. on exit, also while an exception unwinds, it frees the
    // registers allocated within the scope and restores what was protected.
    // it restores allocator bookkeeping only, not register values, and what
    // was allocated before the scope must not be freed within it
    class operation_scope final {
        machine& machine_;
        uint32_t saved_mask_{};
        size_t saved_count_{};

      public:
        // protects nothing, only frees what is allocated while it lives
        explicit operation_scope(machine& backend)
            : machine_{backend},
              saved_mask_{backend.state_.registers.unavailable_mask()},
              saved_count_{backend.state_.registers.allocations().size()} {}

        operation_scope(machine& backend, const operand& value)
            : operation_scope{backend} {

            protect(value);
        }

        operation_scope(machine& backend, const operand& dst,
                        const operand& src)
            : operation_scope{backend} {

            protect(dst);
            protect(src);
        }

        operation_scope(const operation_scope&) = delete;
        operation_scope(operation_scope&&) = delete;
        auto operator=(const operation_scope&) -> operation_scope& = delete;
        auto operator=(operation_scope&&) -> operation_scope& = delete;

        ~operation_scope() {
            register_pool& pool{machine_.state_.registers};

            assert(pool.allocations().size() >= saved_count_);

            while (pool.allocations().size() > saved_count_) {
                // implicit releases need the allocation context for a balanced
                // trace
                const register_pool::allocation entry{pool.top()};

                machine_.pop_allocation(entry.src_loc_tk, entry.indent,
                                        entry.index);
            }

            pool.restore_unavailable(saved_mask_);
        }

        // the registers of the address 'src' that were free when the scope
        // began and that 'dst' does not use: the caller has given them up, so
        // they are dead once 'src' is read, e.g. the index of 'b[j]' in
        // 'a[i] = b[j]'
        [[nodiscard]] auto consumed_registers(const operand& dst,
                                              const operand& src) const
            -> uint32_t {

            const uint32_t address{machine_.registers_mask(src)};
            const uint32_t kept{saved_mask_ | machine_.registers_mask(dst)};
            return src.is_memory() ? address & ~kept : 0;
        }

        // the registers of 'value' are not picked for scratch until the scope
        // ends
        auto protect(const operand& value) -> void {
            if (value.is_register()) {
                machine_.protect_register(value.register_name());
            } else if (value.is_memory()) {
                machine_.protect_register(value.base_register());
                machine_.protect_register(value.index_register());
            }
        }
    };

    // what a backend emits for a bounds check, decided by 'check_bounds' from
    // its options and operands
    struct bounds_plan {
        // the values tested for a negative sign, in order
        std::vector<operand> sign_tested;
        // the range is compared with the array count
        bool upper{};
        // the lower bound is checked or covered, so the upper compare is
        // unsigned
        bool lower_checked{};
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

        // the name as a part of a label, which has no space, e.g.
        // 'frame_overflow'
        [[nodiscard]] auto symbol() const -> std::string {
            std::string text{name};
            std::ranges::replace(text, ' ', '_');
            return text;
        }
    };

  public:
    // 'files' locate the tokens of comments; 'output' is not used here, the
    // backend constructs it after the base
    machine(const builtin_type_set& builtins, const source_files& files,
            const target_options& options, const target_description& target,
            assembler& output)
        : target_{target}, locations_{files}, builtins_{builtins},
          output_{output}, options_{options} {

        if (options.register_report) {
            state_.trace.enable();
        }
    }

    machine(const machine&) = delete;
    machine(machine&&) = delete;
    auto operator=(const machine&) -> machine& = delete;
    auto operator=(machine&&) -> machine& = delete;

    virtual ~machine() = default;

    // the unit of the shifts that take a byte out of a value
    static constexpr size_t bits_per_byte{
        std::numeric_limits<unsigned char>::digits,
    };

    // the start of the label of a noinline body, the report drops it
    static constexpr std::string_view body_label_prefix{"func."};

  protected:
    // a failed check prints its message, the 'panic: ...' of its handler, to
    // this descriptor and exits with the code of its handler
    static constexpr int stderr_descriptor_{2};

    // the sector size of the sd card of every target that has one
    static constexpr size_t card_sector_size_bytes_{512};

    // the labels of the sections of the program's data and variables
    static constexpr std::string_view data_label_{"dat"};
    static constexpr std::string_view data_end_label_{"dat.end"};
    static constexpr std::string_view variables_label_{"vars"};
    static constexpr std::string_view variables_end_label_{"vars.end"};

    // the handlers of the failed checks, a check site passes its entry, so
    // the label, the name and whether it takes the line come together
    static constexpr failure_handler bounds_failure_{
        .label{"baz_bounds_panic"},
        .name{"bounds"},
        .comment{"bounds failure handler (--checks=upper or --checks=lower)"},
        .takes_line{true},
        .exit_code{255},
    };
    static constexpr failure_handler division_failure_{
        .label{"baz_division_panic"},
        .name{"division"},
        .comment{"division failure handler (--checks=division)"},
        .takes_line{true},
        .exit_code{253},
    };
    static constexpr failure_handler frame_overflow_failure_{
        .label{"baz_frame_overflow"},
        .name{"frame overflow"},
        .comment{"frame overflow handler (--checks=frame)"},
        .takes_line{false},
        .exit_code{254},
    };
    static constexpr failure_handler overflow_failure_{
        .label{"baz_overflow_panic"},
        .name{"overflow"},
        .comment{"overflow failure handler (--checks=overflow)"},
        .takes_line{true},
        .exit_code{249},
    };
    static constexpr failure_handler overlap_failure_{
        .label{"baz_overlap_panic"},
        .name{"overlap"},
        .comment{"overlap failure handler (--checks=overlap)"},
        .takes_line{true},
        .exit_code{251},
    };
    static constexpr failure_handler shift_failure_{
        .label{"baz_shift_panic"},
        .name{"shift"},
        .comment{"shift failure handler (--checks=shift)"},
        .takes_line{true},
        .exit_code{252},
    };
    static constexpr failure_handler stack_overflow_failure_{
        .label{"baz_stack_overflow"},
        .name{"stack overflow"},
        .comment{"stack overflow handler (--checks=stack)"},
        .takes_line{false},
        .exit_code{250},
    };

    // in the order they are emitted after the code
    static constexpr std::array<failure_handler, 7> failure_handlers_{
        frame_overflow_failure_, division_failure_,       overlap_failure_,
        overflow_failure_,       stack_overflow_failure_, shift_failure_,
        bounds_failure_,
    };

    // a failed sector read or write jumps to it, the sector routines emit it,
    // see 'emit_sector_routines'
    static constexpr failure_handler sd_card_failure_{
        .label{"baz_sd_card_panic"},
        .name{"sd card"},
        .comment{"sd card failure handler (sector_read or sector_write)"},
        .takes_line{false},
        .exit_code{248},
    };

    // a panic is told by its exit code alone, e.g. by a script
    static_assert([] -> bool {
        std::array<int, failure_handlers_.size() + 1> codes{
            sd_card_failure_.exit_code,
        };

        for (size_t i{}; i < failure_handlers_.size(); ++i) {
            codes.at(i + 1) = failure_handlers_.at(i).exit_code;
        }

        std::ranges::sort(codes);

        return std::ranges::adjacent_find(codes) == codes.end();
    }());

  public:
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

    virtual auto bitwise(const token& src_loc_tk, const size_t indent,
                         const arithmetic_operator operation,
                         const operand& dst, const operand& src) -> void = 0;

    virtual auto branch(const size_t indent, const std::string_view target)
        -> void = 0;

    // jumps to the frame overflow handler when the callee frame at
    // 'frame_address' does not fit in the variables; 'frame_size_label' is the
    // assembler constant of its size, defined after the body
    virtual auto check_frame_capacity(const token& src_loc_tk,
                                      const size_t indent,
                                      const operand& frame_address,
                                      const std::string_view frame_size_label)
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
    virtual auto copy_block(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& src,
                            const size_t size_bytes, const size_t alignment)
        -> void = 0;

    // copies the count of elements 'emit_count' compiles after the request
    // emits the addresses, in the order of the arguments
    virtual auto
    copy_elements(const token& src_loc_tk, const size_t indent,
                  const size_t element_size_bytes,
                  const std::function_ref<void(const operand&)> emit_count,
                  const copy_request& request) -> void = 0;

    virtual auto copy_value(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& src) -> void = 0;

    // an enabled check makes a zero divisor, and the minimum of the type of
    // 'dst' divided by -1, jump to the division failure handler
    virtual auto divide(const token& src_loc_tk, const size_t indent,
                        const arithmetic_operator operation, const operand& dst,
                        const operand& divisor, const runtime_check& check)
        -> void = 0;

    // 'emit_count' compiles the element count into the register it is given
    // after 'lhs' and 'rhs' emit the addresses, in the order of the
    // arguments, the elements of both arrays are compared at the width
    // 'alignment' allows and the result, 1 when they are equal, goes to 'dst'
    virtual auto
    equal_elements(const token& src_loc_tk, const size_t indent,
                   const size_t element_size_bytes,
                   const std::function_ref<void(const operand&)> emit_count,
                   const address_emitter& lhs, const address_emitter& rhs,
                   const equality_request& request) -> void = 0;

    // compares 'size_bytes' at the addresses 'lhs' and 'rhs', whose registers
    // the caller holds, and puts 1 into the destination of the request when
    // they are equal, when they differ if it is 'inverted'
    virtual auto memory_equal(const token& src_loc_tk, const size_t indent,
                              const size_t size_bytes, const operand& lhs,
                              const operand& rhs,
                              const equality_request& request) -> void = 0;

    virtual auto return_function(const size_t indent) -> void = 0;

    // an enabled check makes a count below zero or not below the width of the
    // type of 'dst' jump to the shift failure handler
    virtual auto shift(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst,
                       const operand& count, const runtime_check& check)
        -> void = 0;

    // stores constant 'bytes', 'add_constant' adds the read-only data the
    // backend lays them out in and returns its label; it is called only when
    // the bytes are not stored with immediates
    virtual auto store_bytes(
        const token& src_loc_tk, const size_t indent,
        const std::string_view bytes, const operand& dst,
        const size_t alignment,
        const std::function_ref<std::string(std::string_view)> add_constant)
        -> void = 0;

    virtual auto unary(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst,
                       const runtime_check& check = {}) -> void = 0;

    // writes the lines buffered from 'start'
    virtual auto write_assembly(std::ostream& os) -> void = 0;

    virtual auto zero(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const size_t size_bytes,
                      const size_t alignment) -> void = 0;

    //
    // class methods
    //

    [[nodiscard]] auto address_size_bytes() const -> size_t {
        return target_.address_size_bytes;
    }

    // a register for a value loaded from an address: the first in the
    // scratch order that is free or one of the address registers 'consumed',
    // see 'operation_scope::consumed_registers'
    [[nodiscard]] auto alloc_load_register(const token& src_loc_tk,
                                           const size_t indent,
                                           const type& type_ref,
                                           const uint32_t consumed) -> operand {

        return alloc_scratch_register_from(src_loc_tk, indent, type_ref,
                                           consumed);
    }

    // a register that an instruction or the calling convention fixes, held
    // until 'free_named_register'
    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand {

        reserve_named_register(src_loc_tk, indent, register_name);
        return make_register_operand(register_name, type_ref);
    }

    // the first free register in the scratch order of the backend
    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indent,
                                              const type& type_ref) -> operand {

        return alloc_scratch_register_from(src_loc_tk, indent, type_ref, 0);
    }

    // starts the data section, after the code and every routine it uses
    auto begin_data() -> void {
        assembler& output{target_assembler()};

        output.add_separator_newline();
        output.switch_section(section::data);
        output.align(target_.data_alignment);
        output.label(0, data_label_);
    }

    // the code emitted up to 'end_noinline_body' is a body of 'function':
    // its 'body_label', the frame base held and the entry code
    // ('emit_frame_entry'), whose exit is 'return_function'
    auto begin_noinline_body(std::string function,
                             const std::string& body_label) -> void {

        target_assembler().begin_body(std::move(function), body_label);
        label(0, body_label);

        // note: a body starts and ends with no scratch register held
        assert(state_.registers.scratch_count() == 0);

        reserve_backend_register(frame_base_register());
        emit_frame_entry();
    }

    // 'slot_address' is the address the callee finds in
    // 'slot_register', none when the register is not used
    auto call_function(const token& src_loc_tk, const size_t indent,
                       const std::string_view label,
                       const operand& frame_address,
                       const std::optional<operand>& slot_address) -> void {

        assert(frame_address.is_memory());
        assert(frame_address.index_register().empty());

        assert(frame_address.base_register() == frame_base_register() or
               frame_address.base_register() == variables_base_register());

        // the callee may use every register but the variables base, so only
        // values live at the call need saving
        const std::vector<size_t> saved{registers_saved_by_call()};

        state_.trace.record_noinline_call(src_loc_tk, label, saved.size());

        if (not saved.empty()) {
            comment(src_loc_tk, indent,
                    "before call: save allocated registers");
        }

        save_call_registers(indent, saved);
        load_slot_address(src_loc_tk, indent, slot_address);

        comment(src_loc_tk, indent, "set function frame base");

        // a far frame address is lowered through a scratch register, which
        // must not be the loaded slot register
        const operation_scope slot_kept{
            *this,
            make_register_operand(slot_register(), default_type()),
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

    // a call of 'read', 'write', 'sector_read' or 'sector_write':
    // 'emit_arguments' compiles the arguments into the registers it is given,
    // in their order, and 'use_result' takes the count that 'read' and
    // 'write' return, it is not called for a sector function; the registers
    // are held for the call only; a sector is an unsigned 32 bit number, the
    // size of a sector transfer is 'sector_size_bytes', a sector past the end
    // of the card panics where the target can tell
    auto
    call_target(const token& src_loc_tk, const size_t indent,
                const target_call function,
                const std::function_ref<void(std::span<const operand> args)>
                    emit_arguments,
                const std::function_ref<void(const operand& result)> use_result)
        -> void {

        const operation_scope call{*this};

        std::vector<operand> args;
        args.reserve(target_.target_calls.arguments.size());
        for (const std::string_view name : target_.target_calls.arguments) {
            args.push_back(
                alloc_named_register(src_loc_tk, indent, name, default_type()));
        }

        emit_arguments(args);

        const bool is_sector{
            function == target_call::sector_read or
                function == target_call::sector_write,
        };

        if (is_sector) {
            emit_target_call(src_loc_tk, indent, function, operand{});
            return;
        }

        const operand result{alloc_target_call_result(src_loc_tk, indent)};

        emit_target_call(src_loc_tk, indent, function, result);

        use_result(result);
    }

    [[nodiscard]] auto can_lower_index_scale(const size_t size_bytes) const
        -> bool {

        return std::has_single_bit(size_bytes) and
               size_bytes <= target_.max_index_scale_bytes;
    }

    // jumps to the bounds failure handler when 'range' is outside its array,
    // by the bounds that 'options' turns on
    auto check_bounds(const token& src_loc_tk, const size_t indent,
                      const bounds_range& range,
                      const bounds_check_options& options) -> void {

        if (not options.upper and not options.lower) {
            return;
        }

        // note: an index and a count are values of the default type in
        //       registers

        assert(range.checked.type_ref().size_bytes() ==
               default_type().size_bytes());

        assert(range.checked.is_register() and
               (not range.count or range.count->is_register()));

        assert(not range.count_lower_checked or range.count);

        comment(src_loc_tk, indent, "bounds check begin");

        const bounds_plan plan{
            .sign_tested{lower_bound_tests(src_loc_tk, indent, range, options)},
            .upper{options.upper},
            .lower_checked{options.lower},
        };

        // e.g. a start that the walk of its address checked, under
        // '--checks=lower' alone
        if (plan.sign_tested.empty() and not plan.upper) {
            comment(src_loc_tk, indent, "bounds check end");
            return;
        }

        emit_bounds_check(src_loc_tk, indent, range, plan);

        comment(src_loc_tk, indent, "bounds check end");
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
                         const std::optional<operand>& address) -> void {

        if (not address) {
            comment(src_loc_tk, indent, "bind {} -> {}", from, to);
            return;
        }

        comment(src_loc_tk, indent, "bind {} -> {} (address: {})", from, to,
                address_text(*address));
    }

    // the slot holds the address of the variable, e.g. a parameter of a
    // noinline body
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

    [[nodiscard]] auto data_alignment() const -> size_t {
        return target_.data_alignment;
    }

    // the integer of the width of an address, also the type of an address in
    // a register
    [[nodiscard]] auto default_type() const -> const type& {
        // note: the targets have 4 or 8 byte addresses ('target_description')
        assert(address_size_bytes() == sizeof(int32_t) or
               address_size_bytes() == sizeof(int64_t));

        return address_size_bytes() == sizeof(int32_t) ? builtins_.get().i32
                                                       : builtins_.get().i64;
    }

    // a frame size, limited by the variables that hold the frames
    auto define_constant(const std::string_view name, const size_t value)
        -> void {

        assert(value <= target_.max_storage_bytes);

        target_assembler().define_constant(name, static_cast<int64_t>(value));
    }

    // the elements on one line, e.g. '.word 1, 2, 3'
    auto emit_data_array(const size_t element_size_bytes,
                         const std::span<const int64_t> values) -> void {

        target_assembler().data(element_size_bytes, values);
    }

    // runs 'emit' for its checks, nothing it emits is kept and the registers
    // it used are not counted. it starts with the registers of the code it
    // stands for: the body of a 'noinline' function holds its own, so the
    // registers that the caller holds do not make it run out; code skipped
    // where it stands holds those held there
    auto emit_discarded(const std::function_ref<void()> emit,
                        const discarded_code code) -> void {

        const version_scope discarded{*this};

        if (code == discarded_code::body) {
            state_.registers = noinline_body_pool();
        }

        std::ignore = target_assembler().capture(emit);
    }

    // emits both versions and keeps the one with less code, the first on ties,
    // or the one that '--versions' names; a version that fails with an error
    // other than 'source', e.g. running out of registers, is not kept, when
    // the first does its error is thrown. each version starts from the
    // state before it, the state of the kept one remains; true when the first
    // is kept, 'toc' keeps its peaks ('toc::emit_most_efficient')
    [[nodiscard]] auto
    emit_most_efficient(const token& src_loc_tk, const size_t indent,
                        const std::function_ref<void()> emit_first,
                        const std::function_ref<void()> emit_second) -> bool {

        assembler& output{target_assembler()};

        version_result first{compile_version(emit_first)};
        version_result second{compile_version(emit_second)};

        // note: the second version of each caller holds the registers of the
        //       first and one more, with the same storage and inlining depth
        //       ('expr_arith' through a scratch register, 'stmt_call' with
        //       address registers), so it fails where the first fails

        if (not first) {
            std::rethrow_exception(first.error().error);
        }

        const version_choice choice{choose_version(first, second)};

        comment(src_loc_tk, indent, choice.comment);

        compiled_version& kept{choice.keeps_first ? *first : *second};

        state_ = std::move(kept.end_state);
        failure_locations_.append_range(kept.failures);

        output.append(std::move(kept.lines));

        return choice.keeps_first;
    }

    // leaves the read-only data section current
    auto emit_read_only_constants(
        const std::span<const read_only_constant> constants) -> void {

        assembler& output{target_assembler()};

        output.switch_section(section::rodata);
        for (const read_only_constant& s : constants) {
            // note: a byte alignment needs no directive
            if (target_.string_alignment > 1) {
                output.align(target_.string_alignment);
            }

            output.label(0, s.label);
            output.string_data(s.bytes);
        }
    }

    // the code after the program: the handlers its checks branch to, then the
    // routines that the code and the handlers call
    auto emit_runtime() -> void {
        emit_used_failure_handlers();
        emit_used_routines();
        assert_used_routines_emitted();
    }

    auto emit_string_data(const std::string_view bytes) -> void {
        target_assembler().string_data(bytes);
    }

    auto emit_zero_data(const size_t size_bytes) -> void {
        target_assembler().zero_data(size_bytes);
    }

    // releases the frame base and ends the body
    auto end_noinline_body() -> void {
        assert(state_.registers.scratch_count() == 0);

        release_named_register(token{}, 0, frame_base_register());
        target_assembler().end_body();
    }

    // the exit routine is shared, an exit is a jump to it
    auto exit(const token& src_loc_tk, const size_t indent,
              const operand& exit_code) -> void {

        const operand code{
            make_register_operand(exit_code_register(), default_type()),
        };

        copy_value(src_loc_tk, indent, code, exit_code);

        emit_no_return_jump(indent, target_.routines.exit);
        mark_used(target_.routines.exit);
    }

    // the register 'exit' takes its code in when the code is computed
    [[nodiscard]] auto exit_code_register() const -> std::string_view {
        return target_.target_calls.arguments.front();
    }

    // releases the variables base and asserts that the registers are
    // balanced, optimizes the jumps, then the target's last steps; called once
    // at the end of the compile pass
    auto finish() -> void {
        release_named_register(token{}, 0, variables_base_register());

        // every operation freed what it allocated
        assert(state_.registers.is_empty());
        assert(state_.registers.unavailable_mask() == 0);

        assembler& output{target_assembler()};

        if (options_.jumps == assembler::jump_mode::optimized) {
            output.optimize_jumps();
        }

        finish_target();
    }

    // whether the counter of an array loop is better kept in memory than in a
    // register, decided when the loop starts
    [[nodiscard]] auto foo_counter_in_memory() const -> bool {
        return target_.memory_counter_pressure_count and
               state_.registers.scratch_count() >=
                   *target_.memory_counter_pressure_count;
    }

    [[nodiscard]] auto frame_base_register() const -> std::string_view {
        return target_.frame_base_register;
    }

    auto free_named_register(const token& src_loc_tk, const size_t indent,
                             const operand& reg) -> void {

        assert(reg.is_register());

        release_named_register(src_loc_tk, indent, reg.register_name());
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indent,
                               const operand& reg) -> void {

        assert(reg.is_register());

        assert(state_.registers.top().holder ==
               register_pool::holder_kind::scratch);

        pop_allocation(src_loc_tk, indent, pool_index(reg.register_name()));
    }

    [[nodiscard]] auto has_indexed_addressing() const -> bool {
        return target_.has_indexed_addressing;
    }

    [[nodiscard]] auto has_sd_card() const -> bool {
        return target_.has_sd_card;
    }

    [[nodiscard]] auto is_stack_bounded() const -> bool {
        return target_.is_stack_bounded;
    }

    auto label(const size_t indent, const std::string_view name) -> void {
        target_assembler().label(indent, name);
    }

    [[nodiscard]] auto max_storage_bytes() const -> size_t {
        return target_.max_storage_bytes;
    }

    // a constant factor is resolved at the width of 'product': zero clears,
    // one needs no code, minus one negates and a power of two is a shift, the
    // other factors go to 'emit_multiply'
    auto multiply(const token& src_loc_tk, const size_t indent,
                  const operand& product, const operand& factor,
                  const factor_use use, const runtime_check& check = {})
        -> void {

        const std::optional<uint64_t> bits{immediate_bits(factor)};

        if (not bits) {
            emit_multiply(src_loc_tk, indent, product, factor, use, check);
            return;
        }

        const constant_factor factor_info{
            classify_factor(*bits, product.type_ref().size_bits()),
        };

        if (factor_info.kind == factor_kind::zero) {
            copy_value(src_loc_tk, indent, product,
                       operand::imm(0, product.type_ref()));

            return;
        }

        if (factor_info.kind == factor_kind::one) {
            return;
        }

        if (factor_info.kind == factor_kind::minus_one) {
            unary(src_loc_tk, indent, arithmetic_operator::negate, product,
                  check);

            return;
        }

        // a shift does not report the bits it loses, a multiplication does
        if (factor_info.kind == factor_kind::other or check.enabled) {
            emit_multiply(src_loc_tk, indent, product, factor, use, check);
            return;
        }

        shift(src_loc_tk, indent, arithmetic_operator::shift_left, product,
              operand::imm(factor_info.shift, default_type()), runtime_check{});
    }

    // the use of registers at the busiest point of the build, as lines after
    // the code when asked for, else none; defined in 'register_report.hpp'
    [[nodiscard]] auto register_peak_report() const -> std::vector<std::string>;

    // the register of 'reg' at the width of 'type_ref', freed as 'reg' is,
    // e.g. 'al' of 'rax' on x86_64
    [[nodiscard]] auto register_view(const operand& reg,
                                     const type& type_ref) const -> operand {

        assert(reg.is_register());

        return make_register_operand(reg.register_name(), type_ref);
    }

    // ends the data at 'dat.end', then the variables from 'vars' to
    // 'vars.end'
    auto reserve_variables(const size_t size_bytes) -> void {
        assembler& output{target_assembler()};

        output.label(0, data_end_label_);
        output.add_separator_newline();
        output.switch_section(section::variables);
        output.align(target_.data_alignment);
        output.label(0, variables_label_);
        output.reserve(size_bytes);
        output.label(0, variables_end_label_);
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

    // asked only of a target that has a card
    [[nodiscard]] auto sector_size_bytes() const -> size_t {
        assert(target_.has_sd_card);

        return card_sector_size_bytes_;
    }

    // a blank line that separates the code from the report
    auto separate_report() -> void {
        target_assembler().add_separator_newline();
    }

    [[nodiscard]] auto slot_register() const -> std::string_view {
        return target_.slot_register;
    }

    // the header, the entry of the program and the variables base, held until
    // 'finish', then the target's own setup
    auto start() -> void {
        assembler& output{target_assembler()};

        // note: the parse declares names without emitting
        assert(output.is_empty());

        // note: the parse emits no code, so it allocates no register and uses
        //       no routine

        assert(state_.max_scratch_registers == 0);
        assert(state_.used_routines.empty());

        output.comment(0, "");
        output.comment(0, "generated by baz");
        output.comment(0, "");
        output.add_separator_newline();

        emit_entry();
        reserve_backend_register(variables_base_register());
        emit_variables_base_load();
        output.add_separator_newline();

        emit_target_setup();
    }

    // what the output measured, for the report of the front end, valid after
    // 'finish'
    [[nodiscard]] auto statistics() const -> output_statistics {
        const assembler& output{target_assembler()};

        return {
            .optimizations{output.optimizations()},
            .noinline_functions{output.noinline_summaries()},
            .max_scratch_registers{state_.max_scratch_registers},
            .instruction_count{output.instruction_count()},
        };
    }

    // a 'bool' is stored as 0 or 1
    auto store_boolean(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const bool value) -> void {

        copy_value(src_loc_tk, indent, dst,
                   operand::imm(value ? 1 : 0, default_type()));
    }

    [[nodiscard]] auto variables_base_past_vars_bytes() const
        -> std::optional<size_t> {

        return target_.variables_base_past_vars_bytes;
    }

    [[nodiscard]] auto variables_base_register() const -> std::string_view {
        return target_.variables_base_register;
    }

    // the padding of the instance 'dst' of 'dst_type', zeroed gap by gap or as
    // the whole instance, whichever the target emits smaller; the fields are
    // written over the zeroes after
    auto zero_padding(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const type& dst_type) -> void {

        const size_t max_gap_count{target_.max_padding_gaps_zeroed_one_by_one};

        const std::vector<padding_gap> gaps{
            dst_type.padding_gaps(max_gap_count),
        };

        // note: a measured threshold, not 'emit_most_efficient': both forms
        //       would multiply the versions of the inlined calls around it,
        //       and an instance with thousands of gaps would cost compile time

        if (gaps.size() > max_gap_count) {
            comment(src_loc_tk, indent, "zero instance for its padding: {} B",
                    dst_type.size_bytes());

            zero(src_loc_tk, indent, dst, dst_type.size_bytes(),
                 dst_type.alignment());

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

    // the compares and branches of 'check_bounds' after its plan, between the
    // begin and end comments: the sign tests, then the upper compare
    virtual auto emit_bounds_check(const token& src_loc_tk, const size_t indent,
                                   const bounds_range& range,
                                   const bounds_plan& plan) -> void = 0;

    // the call instruction of 'call_function'
    virtual auto emit_call(const size_t indent, const std::string_view label)
        -> void = 0;

    // the entry of the program, e.g. its section and '_start'
    virtual auto emit_entry() -> void = 0;

    // the routine at 'routine_labels::exit'
    virtual auto emit_exit_routine() -> void = 0;

    // the code at the start of a 'noinline' body after the frame base is
    // reserved, none by default; 'return_function' is its exit
    virtual auto emit_frame_entry() -> void {}

    // a multiplication that 'multiply' did not resolve: a run-time factor, a
    // constant one that is not zero, one, minus one or a power of two, or a
    // checked power of two
    virtual auto emit_multiply(const token& src_loc_tk, const size_t indent,
                               const operand& product, const operand& factor,
                               const factor_use use, const runtime_check& check)
        -> void = 0;

    // a jump to a handler or a routine that does not return, so the registers
    // are dead at the target
    virtual auto emit_no_return_jump(const size_t indent,
                                     const std::string_view target) -> void = 0;

    // writes 'message' at 'message_label' to stderr, for the failure handlers,
    // which run after all allocations
    virtual auto emit_panic_message(const std::string_view message_label,
                                    const std::string_view message) -> void = 0;

    // the routine at 'routine_labels::report_line', which a handler that
    // reports the line jumps to
    virtual auto emit_report_line_routine() -> void = 0;

    // the routines at 'routine_labels::sector_read' and 'sector_write', each
    // only when it is used; only a target that 'has_sd_card' is asked
    virtual auto emit_sector_routines([[maybe_unused]] const bool read_used,
                                      [[maybe_unused]] const bool write_used)
        -> void {

        std::unreachable();
    }

    // the call of 'call_target', its arguments in their registers
    virtual auto emit_target_call(const token& src_loc_tk, const size_t indent,
                                  const target_call function,
                                  const operand& result) -> void = 0;

    // the routines of the target besides 'routine_labels' that the code
    // marked used, before the sector routines and 'exit', which they may use,
    // none by default
    virtual auto emit_target_routines() -> void {}

    // the setup a target needs before 'main', none by default
    virtual auto emit_target_setup() -> void {}

    // the routines that the code marked used; a routine may mark another one
    // it calls, see 'emit_runtime'
    virtual auto emit_used_routines() -> void {
        const routine_labels& routines{target_.routines};

        if (is_used(routines.report_line)) {
            emit_report_line_routine();
        }

        emit_target_routines();

        const bool is_read_used{is_used(routines.sector_read)};
        const bool is_write_used{is_used(routines.sector_write)};

        if (is_read_used or is_write_used) {
            emit_sector_routines(is_read_used, is_write_used);
        }

        // note: after the routines that may use 'exit', e.g. the report of a
        //       line

        if (is_used(routines.exit)) {
            emit_exit_routine();
        }
    }

    // the variables base register gets its address, after it is reserved
    virtual auto emit_variables_base_load() -> void = 0;

    // the checks and fixups of the output after the jumps are optimized, none
    // by default
    virtual auto finish_target() -> void {}

    // the register 'name' as an operand of the scalar 'value_type', e.g.
    // 'eax' for 'rax' and an 'i32' on x86_64
    [[nodiscard]] virtual auto
    make_register_operand(const std::string_view name,
                          const type& value_type) const -> operand = 0;

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

    //
    // class methods
    //

    [[nodiscard]] auto builtins() const -> const builtin_type_set& {
        return builtins_.get();
    }

    // the handler of a check: without the line it prints 'panic: NAME' and
    // exits; with it, one stub per failure location loads the line and enters
    // the entry of its file, which prints 'panic: NAME at FILE:', loads the
    // exit code and enters the shared line report
    auto emit_failure_handler(const failure_handler& handler) -> void {
        assembler& output{target_assembler()};

        if (not reports_line(handler)) {
            const std::string message_label{
                std::format("{}_message", handler.label),
            };

            const std::string message{
                std::format("{}\n", panic_message(handler.name)),
            };

            output.label(0, handler.label);
            emit_panic_message(message_label, message);
            exit(token{}, 1, operand::imm(handler.exit_code, default_type()));

            emit_message_data(message_label, message);

            return;
        }

        const std::string label_name{handler.symbol()};

        const operand line_register{
            make_register_operand(target_.report_line_register, default_type()),
        };

        const operand code_register{
            make_register_operand(target_.report_exit_code_register,
                                  default_type()),
        };

        const operand exit_code{
            operand::imm(handler.exit_code, default_type()),
        };

        for (const auto& [file_ix, lines] : failure_lines_of(handler.label)) {
            const std::string entry_label{
                failure_entry_label(handler.label, file_ix),
            };

            for (const size_t line : lines) {
                output.label(0, line_failure_label(label_name, file_ix, line));

                const operand line_value{
                    operand::imm(static_cast<int64_t>(line), default_type()),
                };

                copy_value(token{}, 1, line_register, line_value);
                emit_no_return_jump(1, entry_label);
            }

            const std::string message_label{
                std::format("{}_message", entry_label),
            };

            const std::string message{failure_message(handler.name, file_ix)};

            output.label(0, entry_label);
            emit_panic_message(message_label, message);
            copy_value(token{}, 1, code_register, exit_code);
            emit_no_return_jump(1, target_.routines.report_line);
            mark_used(target_.routines.report_line);

            emit_message_data(message_label, message);
        }
    }

    // the lines of the failures of 'routine' by the file they are in
    [[nodiscard]] auto failure_lines_of(const std::string_view routine) const
        -> failure_lines {

        failure_lines lines;

        for (const failure_location& failure : failure_locations_) {
            if (failure.routine == routine) {
                lines[failure.file_ix].insert(failure.line);
            }
        }

        return lines;
    }

    // e.g. 'panic: bounds at prog.baz:', the line follows; the name is
    // escaped as in the compiler's messages, so the message stays one line
    [[nodiscard]] auto failure_message(const std::string_view name,
                                       const size_t file_ix) const
        -> std::string {

        return std::format("{} at {}:", panic_message(name),
                           printable(locations_.relative_name(file_ix)));
    }

    // the label a failed check at 'src_loc_tk' jumps to, 'handler' marked
    // used: the handler, or with the line the stub of the line, which loads
    // the line for the entry of the handler for the file
    [[nodiscard]] auto failure_target(const failure_handler& handler,
                                      const token& src_loc_tk) -> std::string {

        mark_used(handler.label);

        if (not reports_line(handler)) {
            return std::string{handler.label};
        }

        failure_locations_.push_back({
            .routine{handler.label},
            .file_ix{src_loc_tk.file_index()},
            .line{src_loc_tk.at_line()},
        });

        return line_failure_label(handler.symbol(), src_loc_tk.file_index(),
                                  src_loc_tk.at_line());
    }

    // the first register in the scratch order of the backend that is free or
    // in 'consumed', none when all are held
    [[nodiscard]] auto
    first_unused_scratch_register(const uint32_t consumed = 0) const
        -> std::optional<std::string_view> {

        for (const std::string_view register_name : scratch_registers()) {
            const uint32_t mask{register_mask(register_name)};

            if ((consumed & mask) != 0 or
                not state_.registers.is_unavailable(mask)) {

                return register_name;
            }
        }

        return std::nullopt;
    }

    // the scratch registers of the backend that are free
    [[nodiscard]] auto free_scratch_register_count() const -> size_t {
        const std::ptrdiff_t count{
            std::ranges::count_if(
                scratch_registers(),
                [&](const std::string_view name) -> bool {
                    return not state_.registers.is_unavailable(
                        register_mask(name));
                }),
        };

        return static_cast<size_t>(count);
    }

    auto free_scratch_registers(const token& src_loc_tk, const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& reg : registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, reg);
        }
    }

    // the backend holds 'register_name' itself, e.g. the frame base while a
    // noinline body is compiled
    [[nodiscard]] auto
    is_backend_register(const std::string_view register_name) const -> bool {

        const size_t index{pool_index(register_name)};

        return std::ranges::any_of(
            state_.registers.allocations(),
            [&](const register_pool::allocation& allocated) -> bool {
                return allocated.index == index and
                       allocated.holder == register_pool::holder_kind::backend;
            });
    }

    [[nodiscard]] auto is_used(const std::string_view routine) const -> bool {
        return state_.used_routines.contains(routine);
    }

    // the routine is emitted after the code; the use is version state, so a
    // version that is not kept and discarded code use nothing
    auto mark_used(const std::string_view routine) -> void {
        state_.used_routines.emplace(routine);
    }

    [[nodiscard]] auto options() const -> const target_options& {
        return options_;
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

    // keeps the register 'name' from being picked for scratch until the
    // enclosing 'operation_scope' ends
    auto protect_register(const std::string_view name) -> void {
        state_.registers.protect(register_mask(name));
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

    // the registers 'value' names, its base and index when it is in memory
    [[nodiscard]] auto registers_mask(const operand& value) const -> uint32_t {
        if (value.is_register()) {
            return register_mask(value.register_name());
        }

        if (value.is_memory()) {
            return register_mask(value.base_register()) |
                   register_mask(value.index_register());
        }

        return 0;
    }

    // the registers a 'noinline' call saves: the allocated ones but the
    // variables base, which the callee uses too
    [[nodiscard]] auto registers_saved_by_call() const -> std::vector<size_t> {
        const size_t variables_base{pool_index(variables_base_register())};

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

        assert(state_.registers.top().holder !=
               register_pool::holder_kind::scratch);

        pop_allocation(src_loc_tk, indent, pool_index(register_name));
    }

    // the failure of 'handler' reports its line, e.g. not a frame overflow,
    // which is not at a statement
    [[nodiscard]] auto reports_line(const failure_handler& handler) const
        -> bool {

        return options_.reports_line and handler.takes_line;
    }

    // 'alloc_named_register' without an operand, for a register that the
    // backend uses by name
    auto reserve_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view register_name) -> void {

        comment(src_loc_tk, indent, "allocate named register {}",
                register_name);

        if (state_.registers.is_unavailable(register_mask(register_name))) {
            throw register_in_use_error(src_loc_tk, register_name);
        }

        push_allocation(src_loc_tk, indent, register_name,
                        register_pool::holder_kind::named);
    }

    // the assembler the target writes its output with
    [[nodiscard]] auto target_assembler() -> assembler& {
        return output_.get();
    }

    [[nodiscard]] auto target_assembler() const -> const assembler& {
        return output_.get();
    }

    // 'emit' changes the registers 'clobbered', the allocated ones among them
    // are saved before and restored after, in the order of 'clobbered', e.g.
    // around a system call
    auto with_saved_registers(const size_t indent,
                              const std::span<const std::string_view> clobbered,
                              const std::function_ref<void()> emit) -> void {

        std::vector<size_t> saved;

        for (const std::string_view name : clobbered) {
            const std::optional<size_t> index{register_index_of(name)};

            // note: the callers name registers of the target, e.g. 'rcx'
            assert(index);

            if (std::ranges::contains(state_.registers.allocations(), *index,
                                      &register_pool::allocation::index)) {

                saved.push_back(*index);
            }
        }

        save_call_registers(indent, saved);
        emit();
        restore_call_registers(indent, saved);
    }

    //
    // statics
    //

    // a memory operand as the comments show it, e.g. 'rbp + 8' or
    // 's0 + t0 * 4 + 28' of a variable at a run-time index
    [[nodiscard]] static auto address_text(const operand& address)
        -> std::string {

        // note: variables and frames are addressed from a base register
        assert(not address.base_register().empty());

        std::string text{address.base_register()};

        if (not address.index_register().empty()) {
            text += std::format(" + {}", address.index_register());

            if (address.scale() != 1) {
                text += std::format(" * {}", address.scale());
            }
        }

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

    // the two's complement bits of an immediate, empty for an operand that is
    // not one
    [[nodiscard]] static auto immediate_bits(const operand& value)
        -> std::optional<uint64_t> {

        if (not value.is_immediate()) {
            return std::nullopt;
        }

        return static_cast<uint64_t>(value.immediate());
    }

    // a location in the main file has no file index, so the label stays the
    // same for a program of one file
    [[nodiscard]] static auto line_failure_label(const std::string_view name,
                                                 const size_t file_ix,
                                                 const size_t line)
        -> std::string {

        if (file_ix == 0) {
            return std::format("baz_{}_line_{}", name, line);
        }

        return std::format("baz_{}_f{}_line_{}", name, file_ix, line);
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

    // e.g. 'panic: bounds'; the newline in front separates it from the output
    // of the program, which may not end with one
    [[nodiscard]] static auto panic_message(const std::string_view name)
        -> std::string {

        return std::format("\npanic: {}", name);
    }

  public:
    // the queries and the register state that the register report reads
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

    // the first register in the scratch order that is free or in 'consumed',
    // registers the operation protects but no one holds
    [[nodiscard]] auto
    alloc_scratch_register_from(const token& src_loc_tk, const size_t indent,
                                const type& type_ref, const uint32_t consumed)
        -> operand {

        const std::optional<std::string_view> register_name{
            first_unused_scratch_register(consumed),
        };

        if (not register_name) {
            throw out_of_registers_error(src_loc_tk);
        }

        comment(src_loc_tk, indent, "allocate scratch register -> {}",
                *register_name);

        state_.registers.unprotect(consumed & register_mask(*register_name));

        push_allocation(src_loc_tk, indent, *register_name,
                        register_pool::holder_kind::scratch);

        state_.max_scratch_registers = std::max(
            state_.max_scratch_registers, state_.registers.scratch_count());

        return make_register_operand(*register_name, type_ref);
    }

    // the register of the count that 'read' and 'write' return, not allocated
    // again when an argument register holds it
    [[nodiscard]] auto alloc_target_call_result(const token& src_loc_tk,
                                                const size_t indent)
        -> operand {

        const target_call_names& registers{target_.target_calls};

        if (std::ranges::contains(registers.arguments, registers.result)) {
            return make_register_operand(registers.result, default_type());
        }

        return alloc_named_register(src_loc_tk, indent, registers.result,
                                    default_type());
    }

    // a routine marked while another one is emitted is emitted only if a
    // later step of 'emit_runtime' asks for it
    auto assert_used_routines_emitted() const -> void {
        for (const std::string& routine : state_.used_routines) {
            // note: the code marks the handlers, they are emitted first and a
            //       handler that reports lines is under the labels of its lines

            const bool is_handler{
                std::ranges::any_of(
                    failure_handlers_,
                    [&](const failure_handler& handler) -> bool {
                        return handler.label == routine;
                    }),
            };

            assert(is_handler or target_assembler().defines_label(routine));
        }
    }

    // of two versions, the first available, the one to keep and why, e.g.
    // 'kept the second of two versions of 4 and 3 instructions, the smaller'
    [[nodiscard]] auto choose_version(const version_result& first,
                                      const version_result& second) const
        -> version_choice {

        assert(first);

        if (not second) {
            return {
                .keeps_first{true},
                .comment{
                    std::format("kept the first of two versions, the second "
                                "runs out of {}",
                                second.error().resource),
                },
            };
        }

        const assembler& output{target_assembler()};

        const size_t first_count{output.instruction_count_of(first->lines)};
        const size_t second_count{output.instruction_count_of(second->lines)};

        bool keeps_first{first_count <= second_count};
        std::string_view reason{"the smaller"};

        if (options_.kept == kept_version::first) {
            keeps_first = true;
            reason = "as '--versions=first' asks";
        } else if (options_.kept == kept_version::last) {
            keeps_first = false;
            reason = "as '--versions=last' asks";
        } else if (first_count == second_count) {
            reason = "the first of equal sizes";
        }

        return {
            .keeps_first{keeps_first},
            .comment{
                std::format("kept the {} of two versions of {} and {} "
                            "instructions, {}",
                            keeps_first ? "first" : "second", first_count,
                            second_count, reason),
            },
        };
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
                .end_state{state_},
                .failures{version.added_failures()},
            };
        } catch (const compile_error& e) {
            // note: every kind but 'source' depends on the code compiled, the
            //       other version may not reach it

            if (not e.is_limit()) {
                throw;
            }

            const unavailable_version unavailable{
                .error{std::current_exception()},
                .resource{exhausted_resource(e.kind)},
            };

            return std::unexpected{unavailable};
        }
    }

    // the text of a message in read-only data, then back to the code
    auto emit_message_data(const std::string_view message_label,
                           const std::string_view message) -> void {

        assembler& output{target_assembler()};

        output.switch_section(section::rodata);
        output.label(0, message_label);

        // note: a backslash or a control byte of a path is written as an
        //       escape of the target's string directive

        output.string_data(message);
        output.switch_section(section::text);
    }

    // the handlers that the code branches to, see 'mark_used'; before
    // 'emit_used_routines', a handler marks the routines it calls
    auto emit_used_failure_handlers() -> void {
        for (const failure_handler& handler : failure_handlers_) {
            if (not is_used(handler.label)) {
                continue;
            }

            // note: only a bounded stack is checked, see 'is_stack_bounded'
            assert(handler.label != stack_overflow_failure_.label or
                   is_stack_bounded());

            comment({}, 0, handler.comment);

            emit_failure_handler(handler);
        }
    }

    [[nodiscard]] auto is_scratch_register(const size_t index) const -> bool {
        return std::ranges::any_of(scratch_registers(),
                                   [&](const std::string_view name) -> bool {
                                       return register_index_of(name) == index;
                                   });
    }

    // loaded before the frame base is set, the address may be relative to the
    // caller's; the receiver already in the register is passed as is
    auto load_slot_address(const token& src_loc_tk, const size_t indent,
                           const std::optional<operand>& slot_address) -> void {

        if (not slot_address or
            slot_address->is_address_in_register(slot_register())) {

            return;
        }

        comment(src_loc_tk, indent, "address of the slot in a register");

        address_of(src_loc_tk, indent,
                   make_register_operand(slot_register(), default_type()),
                   *slot_address);
    }

    [[nodiscard]] auto location_text(const token& src_loc_tk) const
        -> std::string {

        // note: only the registers the backend holds, which the report does
        //       not list, and the first frame, which it names without a call
        //       site, have no source location

        assert(src_loc_tk.at_line() != 0);

        return locations_.for_listing(src_loc_tk);
    }

    // the values of 'range' that a lower bound check tests for a sign, with
    // the comments of what it leaves out
    [[nodiscard]] auto lower_bound_tests(const token& src_loc_tk,
                                         const size_t indent,
                                         const bounds_range& range,
                                         const bounds_check_options& options)
        -> std::vector<operand> {

        if (not options.lower) {
            return {};
        }

        comment(src_loc_tk, indent, "lower bound");

        // the compare is at the default type, 'signed_max' its largest value
        const uint64_t signed_max{
            (uint64_t{1} << (default_type().size_bits() - 1)) - 1,
        };

        // note: the unsigned upper compare already fails a negative index or
        //       count as long as the limit is below 2^(width - 1), only a sum
        //       'index + count' needs both signs checked

        if (options.upper and not range.count and
            range.array_count <= signed_max) {

            comment(src_loc_tk, indent,
                    "{} lower bound covered by the unsigned upper bound",
                    range.checked.register_name());

            return {};
        }

        std::vector<operand> tested;

        if (range.start_lower_checked) {
            comment(src_loc_tk, indent, "start {} lower bound already checked",
                    range.checked.register_name());
        } else {
            tested.push_back(range.checked);
        }

        if (not range.count) {
            return tested;
        }

        if (range.count_lower_checked) {
            comment(src_loc_tk, indent, "count {} lower bound already checked",
                    range.count->register_name());

            return tested;
        }

        // a negative count passes 'start + count' but spans the address space
        tested.push_back(*range.count);

        return tested;
    }

    // the registers the backend holds itself in a 'noinline' body: the
    // variables base and the frame base
    [[nodiscard]] auto noinline_body_pool() const -> register_pool {
        register_pool pool;

        for (const std::string_view name :
             {variables_base_register(), frame_base_register()}) {

            pool.push({
                .index{pool_index(name)},
                .src_loc_tk{},
                .indent{},
                .holder{register_pool::holder_kind::backend},
                .frame{},
            });
        }

        return pool;
    }

    // the scratch registers a 'noinline' body can use, which holds the
    // variables base and the frame base from its start
    [[nodiscard]] auto noinline_body_register_count() const -> size_t {
        return scratch_register_count(noinline_body_pool().allocations());
    }

    // the index of 'register_name', which names a register
    [[nodiscard]] auto pool_index(const std::string_view register_name) const
        -> size_t {

        const std::optional<size_t> index{register_index_of(register_name)};

        assert(index.has_value());

        return *index;
    }

    // registers are released in reverse order of allocation, the comment
    // names the register as the report does
    auto pop_allocation(const token& src_loc_tk, const size_t indent,
                        const size_t index) -> void {

        const bool is_scratch{
            state_.registers.top().holder ==
                register_pool::holder_kind::scratch,
        };

        comment(src_loc_tk, indent, "free {} register {}",
                is_scratch ? "scratch" : "named", register_display_name(index));

        state_.registers.pop(index);
    }

    // named and scratch allocations share one stack, each is recorded for
    // the report of the use of registers
    auto push_allocation(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const register_pool::holder_kind holder) -> void {

        state_.registers.push({
            .index{pool_index(register_name)},
            .src_loc_tk{src_loc_tk},
            .indent{indent},
            .holder{holder},
            .frame{state_.trace.current_frame()},
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

        const register_pool::allocation& allocated{
            state_.registers.holder_of(pool_index(name)),
        };

        // note: the registers that the backend holds itself, e.g. the
        //       variables base, are needed by no instruction

        assert(allocated.holder != register_pool::holder_kind::backend);

        // note: a message names the file as the error header does, the
        //       register report as the listing does ('location_text'); only
        //       a register the backend holds has no source location

        assert(allocated.src_loc_tk.at_line() != 0);

        const std::string holder_location{
            locations_.for_message(allocated.src_loc_tk),
        };

        if (allocated.holder == register_pool::holder_kind::scratch) {
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

    // a register the backend holds itself, which no source location asked
    // for, e.g. the variables base from 'start' to 'finish'
    auto reserve_backend_register(const std::string_view register_name)
        -> void {

        comment(token{}, 0, "allocate named register {}", register_name);

        push_allocation(token{}, 0, register_name,
                        register_pool::holder_kind::backend);
    }

    // the scratch registers that can hold values beside the ones the backend
    // holds itself in 'allocations', e.g. the variables base 's0' on rv32i
    [[nodiscard]] auto scratch_register_count(
        const std::span<const register_pool::allocation> allocations) const
        -> size_t {

        size_t count{scratch_registers().size()};

        for (const register_pool::allocation& allocated : allocations) {
            if (allocated.holder == register_pool::holder_kind::backend and
                is_scratch_register(allocated.index)) {

                --count;
            }
        }

        return count;
    }

    // see 'target_description::scratch_registers'
    [[nodiscard]] auto scratch_registers() const
        -> std::span<const std::string_view> {

        return target_.scratch_registers;
    }

    //
    // statics
    //

    // what a version that fails with 'kind' ran out of, for the comment of
    // the choice
    [[nodiscard]] static auto
    exhausted_resource(const compile_error::error_kind kind)
        -> std::string_view {

        if (kind == compile_error::error_kind::out_of_registers) {
            return "registers";
        }

        if (kind == compile_error::error_kind::out_of_storage) {
            return "variable storage";
        }

        // note: 'compile_version' passes only a limit
        assert(kind == compile_error::error_kind::inlining_limit);

        return "inlined call levels";
    }
};
