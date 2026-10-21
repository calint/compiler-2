#pragma once

#include <algorithm>
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
// handler of the check, which reports the line when 'with_line'; the operation
// that takes it tells what fails
struct runtime_check {
    bool enabled{};
    bool with_line{};
};

// what the front end emits through, and what every target shares: the
// registers with the variables and frame bases, the trace of their use and
// the routines, helpers and failure stubs the code uses; a backend supplies
// the instructions, the names of its registers and the order it picks scratch
// registers in
class machine {
  public:
    // which of two versions of the same code 'emit_most_efficient' keeps: the
    // smaller, or to test the other version too, the first or the last
    enum class kept_version : uint8_t { smaller, first, last };

    enum class builtin_function : uint8_t {
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

    // what the methods 'add_subtract', 'bitwise', 'divide', 'shift' and
    // 'unary' take, each one a part of it
    enum class arithmetic_operator : uint8_t {
        add,
        subtract,
        multiply,
        divide,
        remainder,
        bit_and,
        bit_or,
        bit_xor,
        shift_left,
        shift_right,
        // the first element of a list that is neither added nor multiplied
        assign,
        negate,
        complement,
    };

    // what a comparison asks, e.g. 'a <= b' is 'less_equal'
    enum class comparison_operator : uint8_t {
        equal,
        not_equal,
        less,
        less_equal,
        greater,
        greater_equal,
    };

    // whether an operation may overwrite a register operand, e.g. a scratch
    // register that holds a factor and is freed after the operation
    enum class factor_use : uint8_t { read_only, may_overwrite };

    using frame_kind = register_trace::frame_kind;

    // note: at namespace scope because a default argument '= {}' of a
    //       member function cannot use a nested class with default member
    //       initializers
    using runtime_check = ::runtime_check;

    struct output_statistics {
        // false when written output was not kept to count
        bool is_counted{};
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
        bool with_line{};
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

    // read-only bytes at 'label', 'text' keeps its escapes such as '\n'
    struct string_constant {
        std::string label;
        std::string text;
    };

    struct builtin_function_registers {
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

    // ends the call frame when it goes out of scope, an error leaves it too
    class call_frame_scope final {
        std::reference_wrapper<machine> machine_;

      public:
        call_frame_scope(machine& backend, const token& call_site_tk,
                         std::string name, const frame_kind kind)
            : machine_{backend} {

            backend.begin_call_frame(call_site_tk, std::move(name), kind);
        }

        call_frame_scope(const call_frame_scope&) = delete;
        call_frame_scope(call_frame_scope&&) = delete;
        auto operator=(const call_frame_scope&) -> call_frame_scope& = delete;
        auto operator=(call_frame_scope&&) -> call_frame_scope& = delete;

        ~call_frame_scope() { machine_.get().end_call_frame(); }
    };

  private:
    // receives the lines emitted before 'start' as they are emitted, which
    // backend tests use, null for a build
    std::ostream* direct_output_;
    source_locations locations_;
    assembler::jump_mode jump_mode_;
    const type* type_i64_{};
    const type* type_i32_{};
    const type* type_i16_{};
    const type* type_i8_{};
    size_t usage_max_scratch_regs_{};
    // the image of the sd card on targets with an operating system
    std::string sd_card_file_name_;

    // the inlined calls being compiled and what the registers held
    register_trace trace_;

    // the indexes are those of the names of the backend, see
    // 'register_index_of'
    register_pool registers_;
    bool variables_base_reserved_{};
    bool frame_base_reserved_{};
    kept_version kept_version_{kept_version::smaller};

    // the routines, helpers and failure handlers that the code compiled so
    // far uses, emitted after the code, with the lines whose failure stubs it
    // uses, e.g. 'sector_read' or the lines of the bounds checks that report
    // them; see 'mark_used'
    std::map<std::string, std::set<size_t>, std::less<>> used_routines_;

    // what compiling a version of code changes in the machine: the registers
    // held, the trace of the calls, the most scratch registers in use and the
    // routines used
    struct version_state {
        register_pool registers;
        register_trace trace;
        size_t max_scratch_regs{};
        std::map<std::string, std::set<size_t>, std::less<>> used_routines;
    };

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

  public:
    // 'files' locate the tokens of comments, backend tests may leave it null
    // and give a stream that receives the lines as they are emitted
    machine(std::ostream* const direct_output, const source_files* const files,
            const assembler::jump_mode jumps)
        : direct_output_{direct_output}, locations_{files}, jump_mode_{jumps} {}

    machine(const machine&) = delete;
    machine(machine&&) = delete;
    auto operator=(const machine&) -> machine& = delete;
    auto operator=(machine&&) -> machine& = delete;

    virtual ~machine() = default;

  protected:
    using jump_mode = assembler::jump_mode;

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
              saved_mask_{backend.registers_.unavailable_mask()},
              saved_count_{backend.registers_.allocations().size()} {

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
            register_pool& pool{machine_.registers_};

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

  public:
    // a failed check prints its message, the 'panic: ...' of its handler, to
    // this descriptor and exits with this code
    static constexpr int stderr_descriptor{2};
    static constexpr int panic_exit_code{255};

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

    // the labels of the code that a failed check jumps to, every backend
    // emits it
    static constexpr std::string_view bounds_failure_handler_label{
        "baz_bounds_panic",
    };
    static constexpr std::string_view division_failure_handler_label{
        "baz_division_panic",
    };
    static constexpr std::string_view overflow_failure_handler_label{
        "baz_overflow_panic",
    };
    static constexpr std::string_view overlap_failure_handler_label{
        "baz_overlap_panic",
    };
    static constexpr std::string_view stack_overflow_handler_label{
        "baz_stack_overflow",
    };
    static constexpr std::string_view shift_failure_handler_label{
        "baz_shift_panic",
    };
    static constexpr std::string_view frame_overflow_handler_label{
        "baz_frame_overflow",
    };

    // a failed sector read or write jumps to it and exits with a panic message
    static constexpr std::string_view sd_card_handler_label{
        "baz_sd_card_panic",
    };

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

    virtual auto begin_data(const size_t alignment) -> void = 0;

    virtual auto bitwise(const token& src_loc_tk, const size_t indent,
                         const arithmetic_operator operation,
                         const operand& dst, const operand& src) -> void = 0;

    virtual auto branch(const size_t indent, const std::string_view target)
        -> void = 0;

    // 'slot_address' is the address the callee finds in
    // 'slot_register', empty when the register is not used
    virtual auto call_function(const token& src_loc_tk, const size_t indent,
                               const std::string_view label,
                               const operand& frame_address,
                               const operand& slot_address) -> void = 0;

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
    // registers allocated now, does not fit in the stack; only where
    // 'is_stack_bounded'
    virtual auto check_stack_capacity(const token& src_loc_tk,
                                      const size_t indent) -> void = 0;

    virtual auto comment_alias(const token& src_loc_tk, const size_t indent,
                               const std::string_view from,
                               const std::string_view to,
                               const operand& address) -> void = 0;

    virtual auto comment_variable(const token& src_loc_tk, const size_t indent,
                                  const std::string_view text,
                                  const size_t size_bytes,
                                  const operand& address) -> void = 0;

    virtual auto compare_and_branch(
        const token& src_loc_tk, const size_t indent, const operand& lhs,
        const operand& rhs, const comparison_action& action,
        const std::span<const operand> scratch_registers_to_free) -> void = 0;

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

    // why a backend cannot emit data elements of this size, none by default
    [[nodiscard]] virtual auto data_element_size_error(
        [[maybe_unused]] const size_t element_size_bytes) const
        -> std::optional<std::string> {

        return std::nullopt;
    }

    [[nodiscard]] virtual auto default_type() const -> const type& = 0;

    virtual auto define_constant(const std::string_view name,
                                 const size_t value) -> void = 0;

    // an enabled check makes a zero divisor, and the minimum of the type of
    // 'dst' divided by -1, jump to the division failure handler
    virtual auto divide(const token& src_loc_tk, const size_t indent,
                        const arithmetic_operator operation, const operand& dst,
                        const operand& divisor, const runtime_check& check)
        -> void = 0;

    virtual auto emit_bounds_failure_handler(const bool with_line) -> void = 0;

    virtual auto emit_data(const size_t element_size_bytes, const int64_t value)
        -> void = 0;

    // 'next' gives the values one by one, false after the last
    virtual auto emit_data_array(const size_t element_size_bytes,
                                 const std::function_ref<bool(int64_t&)> next)
        -> void = 0;

    // prints 'panic: division', with the line when 'with_line'
    virtual auto emit_division_failure_handler(const bool with_line)
        -> void = 0;

    // prints 'panic: frame overflow'
    virtual auto emit_frame_overflow_handler() -> void = 0;

    // prints 'panic: overflow', with the line when 'with_line'
    virtual auto emit_overflow_failure_handler(const bool with_line)
        -> void = 0;

    // prints 'panic: overlap', with the line when 'with_line'
    virtual auto emit_overlap_failure_handler(const bool with_line) -> void = 0;

    virtual auto emit_repeated_data(const size_t element_size_bytes,
                                    const size_t count, const int64_t value)
        -> void = 0;

    // prints 'panic: shift', with the line when 'with_line'
    virtual auto emit_shift_failure_handler(const bool with_line) -> void = 0;

    // prints 'panic: stack overflow', only where 'is_stack_bounded'
    virtual auto emit_stack_overflow_handler() -> void = 0;

    // leaves the code section current
    virtual auto
    emit_string_constants(const std::span<const string_constant> strings)
        -> void = 0;

    virtual auto emit_string_data(const std::string_view value) -> void = 0;

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
    [[nodiscard]] virtual auto is_stack_bounded() const -> bool = 0;

    virtual auto label(const size_t indent, const std::string_view label)
        -> void = 0;

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

    virtual auto read(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const operand& descriptor,
                      const operand& address, const operand& count) -> void = 0;

    // the name of a register in the output, for the use of registers
    [[nodiscard]] virtual auto register_display_name(size_t index) const
        -> std::string = 0;

    [[nodiscard]] virtual auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers = 0;

    // whether a failure handler can report the line of the check, else
    // '--checks=line' is dropped and a check loads no line
    [[nodiscard]] virtual auto reports_line() const -> bool { return true; }

    virtual auto reserve_variables(const size_t alignment,
                                   const size_t size_bytes) -> void = 0;

    virtual auto return_function(const size_t indent) -> void = 0;

    virtual auto scale_index(const token& src_loc_tk, const size_t indent,
                             const operand& index,
                             const size_t element_size_bytes) -> void = 0;

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

    // the register that carries the address of one slot of a call, empty when
    // the backend passes every slot in the frame of the callee
    [[nodiscard]] virtual auto slot_register() const -> std::string_view = 0;

    virtual auto start() -> void = 0;

    virtual auto store_boolean(const token& src_loc_tk, const size_t indent,
                               const operand& dst, const bool value)
        -> void = 0;

    virtual auto unary(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst,
                       const runtime_check& check = {}) -> void = 0;

    // empty when the variables base register points at 'dat', else how far
    // past the 'vars' label it points
    [[nodiscard]] virtual auto variables_base_past_vars_bytes() const
        -> std::optional<size_t> = 0;

    [[nodiscard]] virtual auto variables_base_register() const
        -> std::string_view = 0;

    virtual auto write(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const operand& descriptor,
                       const operand& address, const operand& count)
        -> void = 0;

    // writes the lines buffered from 'start', output emitted before it was
    // already written
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
            if (registers_.is_unavailable(register_mask(register_name))) {
                continue;
            }

            comment(src_loc_tk, indent, "allocate scratch register -> {}",
                    register_name);

            push_allocation(src_loc_tk, indent, register_name, false);

            usage_max_scratch_regs_ =
                std::max(usage_max_scratch_regs_, registers_.scratch_count());

            return allocated_operand(register_name, type_ref);
        }

        throw out_of_registers_error(src_loc_tk);
    }

    // the code emitted up to 'end_noinline_body' is a body of 'function'
    auto begin_noinline_body(std::string function, std::string label) -> void {
        target_assembler().begin_body(std::move(function), std::move(label));
    }

    // synthetic tokens and standalone backend calls have no source location
    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void {

        if (src_loc_tk.at_line() == 0 or not locations_.has_source()) {
            target_assembler().comment(indent, text);
            return;
        }

        const std::string location{locations_.for_comment(src_loc_tk)};

        target_assembler().comment(indent, location, text);
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

    template <std::ranges::input_range values_t>
    auto emit_data_array(const size_t element_size_bytes, values_t&& values)
        -> void {

        auto&& range{std::forward<values_t>(values)};
        auto current{std::ranges::begin(range)};
        const auto end{std::ranges::end(range)};

        auto next{
            [&](int64_t& value) -> bool {
                if (current == end) {
                    return false;
                }

                value = *current;
                ++current;

                return true;
            },
        };

        emit_data_array(element_size_bytes,
                        std::function_ref<bool(int64_t&)>{next});
    }

    // runs 'emit' for its checks, nothing it emits is kept and the registers
    // it used are not counted. it starts with the registers that the body of
    // a 'noinline' function holds, the body it stands for, so the registers
    // that the caller holds do not make it run out
    auto emit_discarded(const std::function_ref<void()> emit) -> void {
        const version_scope discarded{*this};

        registers_ = noinline_body_pool();

        try {
            // buffered because comments are otherwise written as emitted
            target_assembler().emit_buffered([&] -> void {
                std::ignore = target_assembler().capture(emit);
            });
        } catch (compile_error& e) {
            // note: the registers it starts with do not depend on the code
            //       around it, so the error is not one of a version there;
            //       discarded code stands for a noinline body, compiled for
            //       real when it is called, so the error does not depend on
            //       the arguments of a call
            e.kind = compile_error::error_kind::source;
            throw;
        }
    }

    // emits both versions and keeps the one with less code, the first on ties,
    // or the one that '--versions' names; a version that runs out of
    // registers is not kept, when both do the error of the first is thrown.
    // each version starts from the state before it, the state of the kept
    // one remains. both are buffered to be compared, even when output is
    // otherwise written as emitted
    auto emit_most_efficient(const token& src_loc_tk, const size_t indent,
                             const std::function_ref<void()> emit_first,
                             const std::function_ref<void()> emit_second)
        -> void {

        assembler& output{target_assembler()};

        output.emit_buffered([&] -> void {
            version_result first{compile_version(emit_first)};
            version_result second{compile_version(emit_second)};

            if (not first and not second) {
                std::rethrow_exception(first.error());
            }

            const bool keeps_first{keeps_first_version(first, second)};

            comment(src_loc_tk, indent, version_comment(first, second));

            compiled_version& kept{keeps_first ? *first : *second};

            restore_version_state(std::move(kept.end_state));

            output.append(std::move(kept.lines));
        });
    }

    // the report of the use of registers is kept for 'register_peak_report'
    auto enable_register_report() -> void { trace_.enable(); }

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

        assert(not registers_.top().named);

        pop_allocation(reg.allocation_register());
    }

    auto free_scratch_registers(const token& src_loc_tk, const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& reg : registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, reg);
        }
    }

    [[nodiscard]] auto location_text(const token& src_loc_tk) const
        -> std::string {

        if (src_loc_tk.at_line() == 0 or not locations_.has_source()) {
            return "-";
        }

        return locations_.human_readable(src_loc_tk);
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

    // the use of registers at the busiest point of the build, as lines after
    // the code when asked for, else none
    [[nodiscard]] auto register_peak_report() const -> std::vector<std::string>;

    // the frame base is held while a 'noinline' body is compiled, which starts
    // and ends with no scratch register held
    auto release_frame_base() -> void {
        assert(frame_base_reserved_);
        assert(registers_.scratch_count() == 0);

        emit_frame_exit();
        pop_allocation(frame_base_register());

        frame_base_reserved_ = false;
    }

    // held from 'start' to 'finish'
    auto release_variables_base() -> void {
        assert(variables_base_reserved_);

        release_named_register(token{}, 0, variables_base_register());

        variables_base_reserved_ = false;
    }

    auto reserve_frame_base() -> void {
        assert(not frame_base_reserved_);
        assert(registers_.scratch_count() == 0);

        push_allocation(token{}, 0, frame_base_register(), true);

        frame_base_reserved_ = true;

        emit_frame_entry();
    }

    auto reserve_variables_base() -> void {
        assert(not variables_base_reserved_);

        reserve_named_register(token{}, 0, variables_base_register(),
                               default_type());

        variables_base_reserved_ = true;
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

    // reads the sd card sector, an unsigned 32 bit number, into the 'size'
    // bytes at 'address', 'size' holds 'sector_size_bytes'; a sector past the
    // end of the card panics where the target can tell
    auto sector_read(const token& src_loc_tk, const size_t indent,
                     const operand& sector, const operand& address,
                     const operand& size) -> void {

        call_sector_routine(src_loc_tk, indent, sector, address, size,
                            builtin_function::sector_read);

        mark_used("sector_read");
    }

    // writes the 'size' bytes at 'address' to the sd card sector, like
    // 'sector_read'
    auto sector_write(const token& src_loc_tk, const size_t indent,
                      const operand& sector, const operand& address,
                      const operand& size) -> void {

        call_sector_routine(src_loc_tk, indent, sector, address, size,
                            builtin_function::sector_write);

        mark_used("sector_write");
    }

    // a blank line that separates the code from the report
    auto separate_report() -> void {
        target_assembler().add_separator_newline();
    }

    // the machine is made before the program that owns the types
    auto set_builtin_types(const builtin_type_set& types) -> void {
        type_i64_ = types.i64;
        type_i32_ = types.i32;
        type_i16_ = types.i16;
        type_i8_ = types.i8;
    }

    auto set_kept_version(const kept_version kept) -> void {
        kept_version_ = kept;
    }

    // the image file that the sector functions read and write on targets with
    // an operating system
    auto set_sd_card_file_name(const std::string_view file_name) -> void {
        sd_card_file_name_ = file_name;
    }

    // what the output measured, for the report of the front end. the counts
    // are valid after 'finish' and only for buffered output
    [[nodiscard]] auto statistics() const -> output_statistics {
        const assembler& output{target_assembler()};

        output_statistics stats{
            .is_counted{},
            .optimizations{assembler::optimization_counts{}},
            .noinline_functions{},
            .max_scratch_registers{usage_max_scratch_regs_},
            .instruction_count{},
        };

        if (output.is_buffering()) {
            stats.is_counted = true;
            stats.optimizations = output.optimizations();
            stats.noinline_functions = output.noinline_summaries();
            stats.instruction_count = output.instruction_count();
        }

        return stats;
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

    //
    // statics
    //

    // the operator that gives the same result with the operands swapped
    [[nodiscard]] static auto mirrored(const comparison_operator op)
        -> comparison_operator {

        if (op == comparison_operator::less) {
            return comparison_operator::greater;
        }

        if (op == comparison_operator::less_equal) {
            return comparison_operator::greater_equal;
        }

        if (op == comparison_operator::greater) {
            return comparison_operator::less;
        }

        if (op == comparison_operator::greater_equal) {
            return comparison_operator::less_equal;
        }

        return op;
    }

    // the operator as written in the source, 'assign' as the '=' of a copy
    [[nodiscard]] static auto source_text(const arithmetic_operator op)
        -> std::string_view {

        if (op == arithmetic_operator::add) {
            return "+";
        }

        if (op == arithmetic_operator::subtract or
            op == arithmetic_operator::negate) {

            return "-";
        }

        if (op == arithmetic_operator::multiply) {
            return "*";
        }

        if (op == arithmetic_operator::divide) {
            return "/";
        }

        if (op == arithmetic_operator::remainder) {
            return "%";
        }

        if (op == arithmetic_operator::bit_and) {
            return "&";
        }

        if (op == arithmetic_operator::bit_or) {
            return "|";
        }

        if (op == arithmetic_operator::bit_xor) {
            return "^";
        }

        if (op == arithmetic_operator::shift_left) {
            return "<<";
        }

        if (op == arithmetic_operator::shift_right) {
            return ">>";
        }

        if (op == arithmetic_operator::assign) {
            return "=";
        }

        // note: 'complement' is a unary operator, 'unary_ops' writes it
        std::unreachable();
    }

    // the operator as written in the source
    [[nodiscard]] static auto source_text(const comparison_operator op)
        -> std::string_view {

        if (op == comparison_operator::equal) {
            return "==";
        }

        if (op == comparison_operator::not_equal) {
            return "!=";
        }

        if (op == comparison_operator::less) {
            return "<";
        }

        if (op == comparison_operator::less_equal) {
            return "<=";
        }

        if (op == comparison_operator::greater) {
            return ">";
        }

        assert(op == comparison_operator::greater_equal);

        return ">=";
    }

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

    struct constant_factor {
        factor_kind kind{};
        // the factor with the bits above the width cleared
        uint64_t multiplier{};
        // the shift count of a power of two
        int shift{};
    };

    //
    // virtual methods
    //

    // calls the routine of 'function', 'sector_read' or 'sector_write', with
    // the arguments in the registers of 'registers_for_builtin_function'
    virtual auto
    call_sector_routine(const token& src_loc_tk, const size_t indent,
                        const operand& sector, const operand& address,
                        const operand& size, const builtin_function function)
        -> void = 0;

    // the code at the start of a 'noinline' body after the frame base is
    // reserved, none by default
    virtual auto emit_frame_entry() -> void {}

    // the code before the frame base is released, none by default
    virtual auto emit_frame_exit() -> void {}

    // the routines that the calls of 'sector_read' and 'sector_write' jump
    // to, each only when it is used
    virtual auto emit_sector_routines(const bool read_used,
                                      const bool write_used) -> void = 0;

    // the index of the register 'name' in any of its sizes, empty for text
    // that names no register, e.g. a label
    [[nodiscard]] virtual auto
    register_index_of(const std::string_view name) const
        -> std::optional<size_t> = 0;

    // the registers 'alloc_scratch_register' picks from, in the order it tries
    // them
    [[nodiscard]] virtual auto scratch_registers() const
        -> std::span<const std::string_view> = 0;

    // the assembler the target writes its output with
    [[nodiscard]] virtual auto target_assembler() -> assembler& = 0;

    [[nodiscard]] virtual auto target_assembler() const -> const assembler& = 0;

    // throws when a register of the target cannot hold a value of 'type_ref',
    // any type fits by default
    virtual auto
    validate_register_type([[maybe_unused]] const token& src_loc_tk,
                           [[maybe_unused]] const type& type_ref) const
        -> void {}

    //
    // class methods
    //

    [[nodiscard]] auto builtin_type_i16() const -> const type& {
        assert(type_i16_ != nullptr);

        return *type_i16_;
    }

    [[nodiscard]] auto builtin_type_i32() const -> const type& {
        assert(type_i32_ != nullptr);

        return *type_i32_;
    }

    [[nodiscard]] auto builtin_type_i64() const -> const type& {
        assert(type_i64_ != nullptr);

        return *type_i64_;
    }

    [[nodiscard]] auto builtin_type_i8() const -> const type& {
        assert(type_i8_ != nullptr);

        return *type_i8_;
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

    [[nodiscard]] auto direct_stream() const -> std::ostream* {
        return direct_output_;
    }

    // unused routines contribute no code
    auto emit_used_sector_routines() -> void {
        const bool is_read_used{is_used("sector_read")};
        const bool is_write_used{is_used("sector_write")};

        if (is_read_used or is_write_used) {
            emit_sector_routines(is_read_used, is_write_used);
        }
    }

    // the optional jump optimization of buffered output
    auto finish_output() -> void {
        assembler& output{target_assembler()};

        if (output.is_buffering() and jump_mode_ == jump_mode::optimized) {
            output.optimize_jumps();
        }
    }

    // releases the variables base that 'start' reserved, backend tests that
    // do not start hold none, and asserts that every register is free
    auto finish_registers() -> void {
        if (variables_base_reserved_) {
            release_variables_base();
        }

        assert(registers_.is_empty());
        assert(registers_.unavailable_mask() == 0);
        assert(not frame_base_reserved_);
    }

    [[nodiscard]] auto is_frame_base_reserved() const -> bool {
        return frame_base_reserved_;
    }

    [[nodiscard]] auto is_used(const std::string_view routine) const -> bool {
        return used_routines_.contains(routine);
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
        registers_.mark_lower_checked(
            register_mask(lower_checked_register(range)));
    }

    // the routine is emitted after the code; the use is version state, so a
    // version that is not kept and discarded code use nothing
    auto mark_used(const std::string_view routine) -> void {
        used_routines_.try_emplace(std::string{routine});
    }

    // the failure stub of 'line' is emitted with the handler of 'routine',
    // see 'mark_used'
    auto mark_used(const std::string_view routine, const size_t line) -> void {
        used_routines_[std::string{routine}].insert(line);
    }

    // 'signed_max' is the largest value of the signed type of the compare
    [[nodiscard]] auto plan_bounds_check(const bounds_check_options& options,
                                         const bounds_range& range,
                                         const uint64_t signed_max) const
        -> bounds_plan {

        const bool count_known{
            not range.count.is_empty() and
                registers_.is_lower_checked(
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
        registers_.protect(register_mask(name));
    }

    // a call of the function with the body 'label' saves 'saved' registers
    auto record_noinline_call(const token& call_site_tk,
                              const std::string_view label, const size_t saved)
        -> void {

        trace_.record_noinline_call(call_site_tk, label, saved);
    }

    // the mask of the register 'name' in the pool, 0 for text that names no
    // register
    [[nodiscard]] auto register_mask(const std::string_view name) const
        -> uint32_t {

        const std::optional<size_t> index{register_index_of(name)};
        return index ? uint32_t{1} << *index : 0;
    }

    [[nodiscard]] auto registers() const -> const register_pool& {
        return registers_;
    }

    // the registers a 'noinline' call saves: the allocated ones but the
    // variables base, which the callee uses too
    [[nodiscard]] auto registers_saved_by_call() const -> std::vector<size_t> {
        const std::optional<size_t> variables_base{
            register_index_of(variables_base_register()),
        };

        std::vector<size_t> saved;
        for (const register_pool::allocation& allocated :
             registers_.allocations()) {

            if (allocated.index != variables_base) {
                saved.push_back(allocated.index);
            }
        }

        return saved;
    }

    auto release_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view register_name) -> void {

        comment(src_loc_tk, indent, "free named register {}", register_name);

        assert(registers_.top().named);

        pop_allocation(register_name);
    }

    // 'alloc_named_register' without an operand, for a register that the
    // backend uses by name
    auto reserve_named_register(const token& src_loc_tk, const size_t indent,
                                const std::string_view register_name,
                                const type& type_ref) -> void {

        validate_register_type(src_loc_tk, type_ref);

        comment(src_loc_tk, indent, "allocate named register {}",
                register_name);

        if (registers_.is_unavailable(register_mask(register_name))) {
            throw register_in_use_error(src_loc_tk, register_name);
        }

        push_allocation(src_loc_tk, indent, register_name, true);
    }

    [[nodiscard]] auto sd_card_file_name() const -> std::string_view {
        return sd_card_file_name_;
    }

    // resolved and optimized jumps need every line before writing
    auto start_output() -> void {
        assembler& output{target_assembler()};

        // note: the parse declares names without emitting
        assert(output.is_empty());

        output.set_direct_output(nullptr);

        // note: the parse emits no code, so it allocates no register and uses
        // no routine
        assert(usage_max_scratch_regs_ == 0);
        assert(used_routines_.empty());

        output.comment(0, "");
        output.comment(0, "generated by baz");
        output.comment(0, "");
        output.add_separator_newline();
    }

    // the lines of the failure stubs of 'routine'
    [[nodiscard]] auto used_lines(const std::string_view routine) const
        -> std::set<size_t> {

        const auto it{used_routines_.find(routine)};
        return it == used_routines_.end() ? std::set<size_t>{} : it->second;
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

  private:
    // the operand of a register handed out by 'alloc_named_register' or
    // 'alloc_scratch_register', which frees it by its allocation name
    [[nodiscard]] auto allocated_operand(const std::string_view register_name,
                                         const type& type_ref) const
        -> operand {

        operand result{make_register_operand(register_name, type_ref)};

        result.set_allocation_register(register_name);

        return result;
    }

    // the registers allocated until 'end_call_frame' belong to the inlined call
    // of 'name' made at 'call_site_tk', the first frame is the function being
    // compiled and has no call site
    auto begin_call_frame(const token& call_site_tk, std::string name,
                          const frame_kind kind) -> void {

        trace_.begin_frame(call_site_tk, std::move(name), kind);
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
    [[nodiscard]] auto current_call_frame() const -> size_t {
        return trace_.current_frame();
    }

    auto end_call_frame() -> void { trace_.end_frame(); }

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

    // the backend's own registers, e.g. the variables base, and the frame
    // base that a 'noinline' body holds from its start
    [[nodiscard]] auto noinline_body_pool() const -> register_pool {
        register_pool pool;

        for (const register_pool::allocation& allocated :
             registers_.allocations()) {

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
            .frame{current_call_frame()},
        });

        return pool;
    }

    // another version of the code may not run out
    [[nodiscard]] auto out_of_registers_error(const token& src_loc_tk) const
        -> compile_error {

        compile_error error{
            register_error(src_loc_tk,
                           "out of scratch registers, simplify the expression"),
        };

        error.kind = compile_error::error_kind::out_of_registers;

        return error;
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
        registers_.pop(pool_index(register_name));
    }

    // named and scratch allocations share one stack, each is recorded for
    // the report of the use of registers
    auto push_allocation(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name, const bool named)
        -> void {

        registers_.push({
            .index{pool_index(register_name)},
            .src_loc_tk{src_loc_tk},
            .indent{indent},
            .named{named},
            .frame{current_call_frame()},
        });

        trace_.record_allocation(registers_);
    }

    // an error that the registers ran out or are held by a value, with how the
    // registers are used by each frame and what a 'noinline' frame would give
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
            registers_.holder_of(pool_index(name)),
        };

        // note: the registers that the backend holds without a place in the
        //       source, e.g. the variables base, are needed by no instruction
        assert(holder.src_loc_tk.at_line() != 0);

        const std::string holder_location{location_text(holder.src_loc_tk)};

        if (not holder.named) {
            return register_error(
                src_loc_tk,
                std::format("cannot allocate register {} because it holds a "
                            "scratch value allocated at {}, simplify the "
                            "expression",
                            name, holder_location));
        }

        return register_error(
            src_loc_tk,
            std::format("register {} is always used by the operation at {} and "
                        "is needed here too, compute the operand that needs "
                        "it with 'let' before that operation",
                        name, holder_location));
    }

    auto restore_version_state(version_state state) -> void {
        registers_ = std::move(state.registers);
        trace_ = std::move(state.trace);
        usage_max_scratch_regs_ = state.max_scratch_regs;
        used_routines_ = std::move(state.used_routines);
    }

    // the choice between two versions, at least one available, e.g. 'smaller
    // of two versions of 3 and 4 instructions'
    [[nodiscard]] auto version_comment(const version_result& first,
                                       const version_result& second) const
        -> std::string {

        if (not second) {
            return "first of two versions, the second runs out of registers";
        }

        if (not first) {
            return "second of two versions, the first runs out of registers";
        }

        std::string_view kept{"smaller"};

        if (kept_version_ == kept_version::first) {
            kept = "first";
        } else if (kept_version_ == kept_version::last) {
            kept = "last";
        }

        const assembler& output{target_assembler()};

        return std::format("{} of two versions of {} and {} instructions", kept,
                           output.instruction_count_of(first->lines),
                           output.instruction_count_of(second->lines));
    }

    [[nodiscard]] auto version_state_now() const -> version_state {
        return {
            .registers{registers_},
            .trace{trace_},
            .max_scratch_regs{usage_max_scratch_regs_},
            .used_routines{used_routines_},
        };
    }
};
