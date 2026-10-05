#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "operand.hpp"
#include "token.hpp"

class type;

// the registers of a machine that hold a value: named ones are fixed by an
// instruction or the calling convention, scratch ones are picked by the
// machine, all are freed in the reverse order of allocation. a register is
// 'unavailable' while it is allocated and while an operation protects the
// registers of its operands. the machine names the registers, the pool counts
// them by their index in the names of the machine and by masks of 1 << index
class register_pool {
  public:
    struct allocation {
        size_t index{};
        // where it was allocated, 'indent' is the indent of that code
        token src_loc_tk;
        size_t indent{};
        const type* type_ptr{};
        bool named{};
        // the index of the inlined call that allocated it, see 'call_frame'
        size_t frame{};
    };

  private:
    uint32_t unavailable_{};
    // registers a lower bounds check found non-negative, allocating a register
    // forgets it since the new owner writes its own value
    uint32_t lower_checked_{};
    std::vector<allocation> allocations_;

  public:
    [[nodiscard]] auto allocations() const -> std::span<const allocation> {
        return allocations_;
    }

    [[nodiscard]] auto is_empty() const -> bool { return allocations_.empty(); }

    [[nodiscard]] auto is_lower_checked(const uint32_t mask) const -> bool {
        return (lower_checked_ & mask) != 0;
    }

    // 'mask' has a register that is allocated or protected
    [[nodiscard]] auto is_unavailable(const uint32_t mask) const -> bool {
        return (unavailable_ & mask) != 0;
    }

    auto mark_lower_checked(const uint32_t mask) -> void {
        lower_checked_ |= mask;
    }

    // the register of the last allocation, freeing is in reverse order
    auto pop(const size_t index) -> allocation {
        assert(top().index == index);

        const allocation entry{allocations_.back()};

        allocations_.pop_back();
        unavailable_ &= ~(uint32_t{1} << index);

        return entry;
    }

    auto protect(const uint32_t mask) -> void { unavailable_ |= mask; }

    auto push(const allocation& entry) -> void {
        const uint32_t mask{uint32_t{1} << entry.index};

        assert(not is_unavailable(mask));

        allocations_.push_back(entry);
        unavailable_ |= mask;
        lower_checked_ &= ~mask;
    }

    // forgets what 'protect' added since 'saved' was read by 'unavailable_mask'
    auto restore_unavailable(const uint32_t saved) -> void {
        unavailable_ = saved;
    }

    [[nodiscard]] auto scratch_count() const -> size_t {
        return static_cast<size_t>(
            std::ranges::count(allocations_, false, &allocation::named));
    }

    [[nodiscard]] auto top() const -> const allocation& {
        assert(not allocations_.empty());

        return allocations_.back();
    }

    // the registers that are allocated or protected, none when all is freed
    [[nodiscard]] auto unavailable_mask() const -> uint32_t {
        return unavailable_;
    }
};

class machine {
  public:
    enum class builtin_function : uint8_t { read, write, exit };

    // receives an address while the registers that built it are allocated
    using address_use = std::function_ref<void(const operand& address)>;

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

    struct data_initializer {
        int64_t value{};
        std::string_view uops; // unary operations
    };

    // read-only bytes at 'label', 'text' keeps its escapes such as '\n'
    struct string_constant {
        std::string label;
        std::string text;
    };

    struct builtin_function_registers {
        // argument register names in parameter order
        std::span<const std::string_view> arguments;

        // result register name, empty for 'exit'
        std::string_view result;
    };

    // ends the call frame when it goes out of scope, an error leaves it too
    class call_frame_scope final {
        std::reference_wrapper<machine> machine_;

      public:
        call_frame_scope(machine& backend, const token& call_site_tk,
                         std::string name)
            : machine_{backend} {

            backend.begin_call_frame(call_site_tk, std::move(name));
        }

        call_frame_scope(const call_frame_scope&) = delete;
        call_frame_scope(call_frame_scope&&) = delete;
        auto operator=(const call_frame_scope&) -> call_frame_scope& = delete;
        auto operator=(call_frame_scope&&) -> call_frame_scope& = delete;

        ~call_frame_scope() { machine_.get().end_call_frame(); }
    };

  private:
    // emits one address of a bulk operation: 'count' is the register of the
    // element count, empty for a known size, 'preferred' a register the address
    // may be built in, empty when the backend has none to offer
    using address_emitter = std::function<void(
        const operand& count, const operand& preferred, address_use use)>;

    std::reference_wrapper<std::ostream> os_;
    std::string_view source_;
    assembler::jump_mode jump_mode_;
    const type* type_i64_{};
    const type* type_i32_{};
    const type* type_i16_{};
    const type* type_i8_{};
    size_t usage_max_scratch_regs_{};

    // an inlined call being compiled, the registers it allocates are its own
    struct call_frame_info {
        std::string name;
        token call_site_tk;
    };

    // the inlined calls being compiled, the outermost first
    std::vector<call_frame_info> call_frames_;

    // what a callee holds itself, over all its instances
    struct callee_use {
        size_t own_peak{};
        size_t instances{};
    };

    // the register report keeps the allocations at the busiest point of the
    // build and what each callee held
    bool register_report_enabled_{};
    size_t peak_held_{};
    std::vector<register_pool::allocation> peak_allocations_;
    std::vector<call_frame_info> peak_frames_;
    std::map<std::string, callee_use> callee_uses_;

    // what the calls of a function with a body of its own save
    struct noinline_calls {
        size_t calls{};
        size_t saved_peak{};
        token peak_site_tk;
    };

    std::map<std::string, noinline_calls> noinline_calls_;

  protected:
    // buffered modes hold output from 'start' to 'finish' so jumps can be
    // optimized and grown to reach their targets
    using jump_mode = assembler::jump_mode;

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
    };

  public:
    // 'source' locates the tokens of comments, backend tests may leave it empty
    machine(std::ostream& os, const std::string_view source,
            const jump_mode jumps)
        : os_{os}, source_{source}, jump_mode_{jumps} {}

    machine(const machine&) = delete;
    machine(machine&&) = delete;
    auto operator=(const machine&) -> machine& = delete;
    auto operator=(machine&&) -> machine& = delete;

    virtual ~machine() = default;

  protected:
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

    // the call frame that allocations are made in
    [[nodiscard]] auto current_call_frame() const -> size_t {
        return call_frames_.empty() ? 0 : call_frames_.size() - 1;
    }

    [[nodiscard]] auto location_text(const token& src_loc_tk) const
        -> std::string {

        if (src_loc_tk.at_line() == 0 or source_.empty()) {
            return "-";
        }

        const auto [line, column]{
            line_and_col_num_for_char_index(src_loc_tk.at_line(),
                                            src_loc_tk.start_index(), source_),
        };

        return std::format("{}:{}", line, column);
    }

    // an error that the registers ran out or are held by a value, with how the
    // registers are used by each frame and what a 'noinline' frame would give
    [[nodiscard]] auto register_error(const token& src_loc_tk,
                                      std::string message,
                                      const register_pool& pool) const
        -> compiler_exception {

        compiler_exception error{src_loc_tk, std::move(message)};

        error.detail = register_use_report(pool.allocations(), call_frames_,
                                           "register use at the failure");

        return error;
    }

    [[nodiscard]] auto register_use_report(
        const std::span<const register_pool::allocation> allocations,
        const std::vector<call_frame_info>& frames,
        const std::string_view heading) const -> std::string {

        const size_t scratch_total{scratch_register_total()};

        // what each frame holds
        struct frame_use {
            // the registers listed, those named by instructions included
            size_t held{};
            std::vector<std::string> registers;
        };

        const size_t frame_count{std::max<size_t>(frames.size(), 1)};
        std::vector<frame_use> uses(frame_count);
        size_t live{};
        size_t named{};

        for (const register_pool::allocation& allocated : allocations) {
            frame_use& use{uses.at(std::min(allocated.frame, frame_count - 1))};

            std::string text{
                std::format("{} {}", register_display_name(allocated.index),
                            location_text(allocated.src_loc_tk)),
            };

            if (allocated.named) {
                // a register the backend holds itself has no place in the
                // source
                if (allocated.src_loc_tk.at_line() == 0) {
                    continue;
                }

                text += " (named)";
                ++named;
            }

            ++use.held;
            ++live;

            use.registers.push_back(std::move(text));
        }

        const auto frame_label{
            [&](const size_t frame) -> std::string {
                if (frames.empty()) {
                    return "code";
                }

                const call_frame_info& info{frames.at(frame)};

                if (frame == 0) {
                    return info.name;
                }

                return std::format("{} called at {}", info.name,
                                   location_text(info.call_site_tk));
            },
        };

        const std::string named_note{
            named == 0 ? std::string{}
                       : std::format(", {} named by instructions", named),
        };

        std::string report{
            std::format("{}: {} of {} registers live{}\n\n  held  frame, "
                        "registers (allocated at)",
                        heading, live, scratch_total, named_note),
        };

        for (const auto [frame, use] : std::views::enumerate(uses)) {
            report += std::format("\n{:>6}  {}", use.held,
                                  frame_label(static_cast<size_t>(frame)));

            for (const std::string& text : use.registers) {
                report += std::format("\n          {}", text);
            }
        }

        if (frame_count < 2) {
            return report;
        }

        // a frame helps when registers are held above it
        struct candidate {
            std::string name;
            size_t inside{};
            size_t saved{};
        };

        std::vector<candidate> candidates;
        size_t saved{};
        size_t inside{live};

        for (const auto [frame, use] : std::views::enumerate(uses)) {
            if (frame > 0 and saved > 0) {
                candidates.push_back({
                    .name{frames.at(static_cast<size_t>(frame)).name},
                    .inside{inside},
                    .saved{saved},
                });
            }

            saved += use.held;
            inside -= use.held;
        }

        if (candidates.empty()) {
            report += "\n\nno noinline frame helps, the registers are held by "
                      "the innermost frame, simplify its expression";

            return report;
        }

        constexpr size_t min_name_width{14};

        size_t name_width{min_name_width};

        for (const candidate& entry : candidates) {
            name_width = std::max(name_width, entry.name.size());
        }

        // the names are padded to the widest
        const auto padded{
            [&](const std::string& name) -> std::string {
                return name + std::string(name_width - name.size(), ' ');
            },
        };

        report += std::format(
            "\n\nmaking a frame noinline starts its body with all {} registers "
            "free and saves the\nregisters held above it around the "
            "call:\n\n  {}  held inside  saved at the call",
            scratch_total, padded("noinline frame"));

        for (const candidate& entry : candidates) {
            report += std::format("\n  {}{:>13}{:>19}", padded(entry.name),
                                  entry.inside, entry.saved);
        }

        return report;
    }

  public:
    // a failed check prints its message to this descriptor and exits with this
    // code
    // the name of the frame of a function with a body of its own ends with this
    static constexpr std::string_view noinline_body_suffix{" (noinline body)"};

    static constexpr int stderr_descriptor{2};
    static constexpr int panic_exit_code{255};

    // the unit of the shifts that take a byte out of a value
    static constexpr size_t bits_per_byte{
        std::numeric_limits<unsigned char>::digits,
    };

    // the labels of the sections of the program's data and variables
    static constexpr std::string_view data_label{"dat"};
    static constexpr std::string_view data_end_label{"dat.end"};
    static constexpr std::string_view variables_label{"vars"};
    static constexpr std::string_view variables_end_label{"vars.end"};

    // the labels of the code that a failed check jumps to, every backend
    // emits it
    static constexpr std::string_view bounds_failure_handler_label{
        "baz_bounds_panic",
    };
    static constexpr std::string_view frame_overflow_handler_label{
        "baz_frame_overflow",
    };

    //
    // virtual methods
    //

    virtual auto add_subtract(const token& src_loc_tk, const size_t indent,
                              const arithmetic_operator operation,
                              const operand& dst, const operand& src)
        -> void = 0;

    virtual auto address_of(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& address)
        -> void = 0;

    [[nodiscard]] virtual auto address_size_bytes() const -> size_t = 0;

    [[nodiscard]] virtual auto
    alloc_named_register(const token& src_loc_tk, const size_t indent,
                         const std::string_view register_name,
                         const type& type_ref) -> operand = 0;

    [[nodiscard]] virtual auto alloc_scratch_register(const token& src_loc_tk,
                                                      const size_t indent,
                                                      const type& type_ref)
        -> operand = 0;

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

    virtual auto call_function(const token& src_loc_tk, const size_t indent,
                               const std::string_view label,
                               const operand& frame_address) -> void = 0;

    [[nodiscard]] virtual auto
    can_lower_index_scale(const size_t size_bytes) const -> bool = 0;

    virtual auto check_bounds(const token& src_loc_tk, const size_t indent,
                              const operand& reg_to_check,
                              const size_t array_count, const bool allow_end,
                              const operand& reg_count,
                              const bounds_check_options& options) -> void = 0;

    virtual auto check_frame_capacity(const token& src_loc_tk,
                                      const size_t indent,
                                      const operand& frame_address,
                                      const operand& frame_size_bytes,
                                      const bool enabled = {}) -> void = 0;

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
                      const operand& src, const operand& dst,
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

    virtual auto divide(const token& src_loc_tk, const size_t indent,
                        const arithmetic_operator operation, const operand& dst,
                        const operand& divisor) -> void = 0;

    virtual auto emit_bounds_failure_handler(const bool with_line) -> void = 0;

    virtual auto emit_data(const size_t element_size_bytes,
                           const data_initializer& value) -> void = 0;

    virtual auto
    emit_data_array(const size_t element_size_bytes,
                    const std::function_ref<bool(data_initializer&)> next)
        -> void = 0;

    // prints 'panic: frame overflow' to stderr and exits with 255
    virtual auto emit_frame_overflow_handler() -> void = 0;

    // emits both versions and keeps the one with less code, the first on ties
    virtual auto
    emit_most_efficient(const token& src_loc_tk, const size_t indent,
                        const std::function_ref<void()> emit_without_scratch,
                        const std::function_ref<void()> emit_with_scratch)
        -> void = 0;

    virtual auto emit_repeated_data(const size_t element_size_bytes,
                                    const size_t count,
                                    const data_initializer& value) const
        -> void = 0;

    // leaves the code section current
    virtual auto
    emit_string_constants(const std::span<const string_constant> strings)
        -> void = 0;

    virtual auto emit_string_data(const std::string_view value) -> void = 0;

    virtual auto emit_zero_data(const size_t size_bytes) const -> void = 0;

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

    virtual auto free_named_register(const token& src_loc_tk,
                                     const size_t indent, const operand& reg)
        -> void = 0;

    virtual auto free_scratch_register(const token& src_loc_tk,
                                       const size_t indent, const operand& reg)
        -> void = 0;

    virtual auto label(const size_t indent, const std::string_view label)
        -> void = 0;

    [[nodiscard]] virtual auto
    make_register_operand(const std::string_view name,
                          const type& value_type) const -> operand = 0;

    // the most bytes of data and variables the target can address
    [[nodiscard]] virtual auto max_storage_bytes() const -> size_t = 0;

    // compares 'size_bytes' of two arrays and puts 1 into the destination of
    // the request when they are equal
    virtual auto memory_equal(const token& src_loc_tk, const size_t indent,
                              const size_t size_bytes,
                              const equality_request& request) -> void = 0;

    virtual auto multiply(const token& src_loc_tk, const size_t indent,
                          const operand& product, const operand& factor,
                          const bool reuse_source = false) -> void = 0;

    virtual auto read(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const operand& descriptor,
                      const operand& address, const operand& count) -> void = 0;

    // the name of a register in the output, for the use of registers
    [[nodiscard]] virtual auto register_display_name(size_t index) const
        -> std::string = 0;

    [[nodiscard]] virtual auto
    registers_for_builtin_function(const builtin_function function) const
        -> builtin_function_registers = 0;

    virtual auto release_frame_base() -> void = 0;

    virtual auto release_variables_base() -> void = 0;

    virtual auto reserve_frame_base() -> void = 0;

    virtual auto reserve_variables(const size_t alignment,
                                   const size_t size_bytes) -> void = 0;

    virtual auto reserve_variables_base() -> void = 0;

    virtual auto return_function(const size_t indent) -> void = 0;

    virtual auto scale_index(const token& src_loc_tk, const size_t indent,
                             const operand& index,
                             const size_t element_size_bytes) -> void = 0;

    // how many scratch registers the backend can hand out
    [[nodiscard]] virtual auto scratch_register_total() const -> size_t = 0;

    virtual auto shift(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst,
                       const operand& count) -> void = 0;

    virtual auto start() -> void = 0;

    virtual auto store_boolean(const token& src_loc_tk, const size_t indent,
                               const operand& dst, const bool value)
        -> void = 0;

    virtual auto unary(const token& src_loc_tk, const size_t indent,
                       const arithmetic_operator operation, const operand& dst)
        -> void = 0;

    virtual auto
    validate_data_element_size(const token& src_loc_tk,
                               const size_t element_size_bytes) const
        -> void = 0;

    virtual auto validate_division_operand(const token& src_loc_tk,
                                           const operand& divisor) const
        -> void = 0;

    virtual auto validate_shift_operand(const token& src_loc_tk,
                                        const operand& count) const -> void = 0;

    // 'dat' is where the variables base register points, unless it points past
    // the 'vars' label, then this is the distance
    [[nodiscard]] virtual auto variables_base_past_vars_bytes() const
        -> std::optional<size_t> = 0;

    [[nodiscard]] virtual auto variables_base_register() const
        -> std::string_view = 0;

    virtual auto write(const token& src_loc_tk, const size_t indent,
                       const operand& dst, const operand& descriptor,
                       const operand& address, const operand& count)
        -> void = 0;

    // 'as_emitted' output was already written, so nothing is buffered
    virtual auto write_assembly(std::ostream& os) -> void = 0;

    virtual auto zero(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const size_t size_bytes,
                      const size_t alignment) -> void = 0;

    //
    // class methods
    //

    // the registers allocated until 'end_call_frame' belong to the inlined call
    // of 'name' made at 'call_site_tk', the first frame is the function being
    // compiled and has no call site
    auto begin_call_frame(const token& call_site_tk, std::string name) -> void {
        if (register_report_enabled_) {
            ++callee_uses_[name].instances;
        }

        call_frames_.push_back({
            .name{std::move(name)},
            .call_site_tk{call_site_tk},
        });
    }

    // the code emitted up to 'end_noinline_body' is a body of 'function'
    auto begin_noinline_body(std::string function, std::string label) -> void {
        target_assembler().begin_body(std::move(function), std::move(label));
    }

    // synthetic tokens and standalone backend calls have no source location
    auto comment(const token& src_loc_tk, const size_t indent,
                 const std::string_view text) -> void {

        if (src_loc_tk.at_line() == 0 or source_.empty()) {
            target_assembler().comment(indent, text);
            return;
        }

        const auto [line, column]{
            line_and_col_num_for_char_index(src_loc_tk.at_line(),
                                            src_loc_tk.start_index(), source_),
        };

        target_assembler().comment(indent, line, column, text);
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

    // runs 'emit' for its checks only, nothing it emits is kept and the
    // registers it used are not counted
    auto discard_output(const std::function_ref<void()> emit) -> void {
        const size_t max_scratch_regs{usage_max_scratch_regs_};

        const size_t kept_held{peak_held_};

        const std::vector<register_pool::allocation> kept_allocations{
            peak_allocations_,
        };

        const std::vector<call_frame_info> kept_frames{peak_frames_};
        const std::map<std::string, callee_use> kept_callees{callee_uses_};
        const std::map<std::string, noinline_calls> kept_calls{noinline_calls_};

        // buffered because comments are otherwise written as emitted
        target_assembler().emit_buffered(
            [&] -> void { std::ignore = target_assembler().capture(emit); });

        usage_max_scratch_regs_ = max_scratch_regs;
        peak_held_ = kept_held;
        peak_allocations_ = kept_allocations;
        peak_frames_ = kept_frames;
        callee_uses_ = kept_callees;
        noinline_calls_ = kept_calls;
    }

    template <std::ranges::input_range values_t>
    auto emit_data_array(const size_t element_size_bytes, values_t&& values)
        -> void {

        auto&& range{std::forward<values_t>(values)};
        auto current{std::ranges::begin(range)};
        const auto end{std::ranges::end(range)};

        auto next{
            [&](data_initializer& value) -> bool {
                if (current == end) {
                    return false;
                }

                value = *current;
                ++current;

                return true;
            },
        };

        emit_data_array(element_size_bytes,
                        std::function_ref<bool(data_initializer&)>{next});
    }

    // the report of the use of registers is kept for 'register_peak_report'
    auto enable_register_report() -> void { register_report_enabled_ = true; }

    auto end_call_frame() -> void { call_frames_.pop_back(); }

    auto end_noinline_body() -> void { target_assembler().end_body(); }

    auto free_named_registers(const token& src_loc_tk, const size_t indent,
                              const std::span<const operand> registers)
        -> void {

        for (const operand& reg : registers | std::views::reverse) {
            free_named_register(src_loc_tk, indent, reg);
        }
    }

    auto free_scratch_registers(const token& src_loc_tk, const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& r : registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, indent, r);
        }
    }

    // a call of the function with the body 'label' saves the registers 'saved'
    auto record_noinline_call(const token& call_site_tk,
                              const std::string_view label, const size_t saved)
        -> void {

        if (not register_report_enabled_) {
            return;
        }

        noinline_calls& entry{noinline_calls_[std::string{label}]};

        ++entry.calls;

        if (saved > entry.saved_peak or entry.calls == 1) {
            entry.saved_peak = saved;
            entry.peak_site_tk = call_site_tk;
        }
    }

    [[nodiscard]] auto register_peak_lines() const -> std::vector<std::string> {

        if (not register_report_enabled_) {
            return {};
        }

        const std::string text{
            register_use_report(peak_allocations_, peak_frames_,
                                "register use at the peak"),
        };

        std::vector<std::string> lines;

        for (const auto line : std::views::split(text, '\n')) {
            lines.emplace_back(std::string_view{line});
        }

        if (not peak_frames_.empty() and
            peak_frames_.front().name.ends_with(noinline_body_suffix)) {

            lines.emplace_back();

            lines.emplace_back("the peak is in a function with a body of its "
                               "own, compiled with all registers free, its "
                               "callers are not on this stack");
        }

        lines.emplace_back();

        lines.emplace_back("per callee, the most registers one instance holds "
                           "itself");

        lines.emplace_back();
        lines.emplace_back("  own  instances  callee");

        std::vector<std::pair<std::string, callee_use>> callees{
            callee_uses_.begin(),
            callee_uses_.end(),
        };

        std::ranges::stable_sort(
            callees, [](const auto& lhs, const auto& rhs) -> bool {
                return lhs.second.own_peak > rhs.second.own_peak;
            });

        for (const auto& [name, use] : callees) {
            lines.push_back(std::format("{:>5}{:>11}  {}", use.own_peak,
                                        use.instances, name));
        }

        if (noinline_calls_.empty()) {
            return lines;
        }

        lines.emplace_back();

        lines.emplace_back(
            "calls of functions with a body of their own save the "
            "registers held at the call");

        lines.emplace_back();
        lines.emplace_back("  saved  calls  callee, most saved at");

        std::vector<std::pair<std::string, noinline_calls>> calls{
            noinline_calls_.begin(),
            noinline_calls_.end(),
        };

        std::ranges::stable_sort(
            calls, [](const auto& lhs, const auto& rhs) -> bool {
                return lhs.second.saved_peak > rhs.second.saved_peak;
            });

        for (const auto& [label, entry] : calls) {
            const std::string_view name{
                label.starts_with("func.")
                    ? std::string_view{label}.substr(
                          std::string_view{"func."}.size())
                    : std::string_view{label},
            };

            lines.push_back(std::format("{:>7}{:>7}  {} {}", entry.saved_peak,
                                        entry.calls, name,
                                        location_text(entry.peak_site_tk)));
        }

        return lines;
    }

    // the use of registers at the busiest point of the build, as lines after
    // the code when asked for, else none
    [[nodiscard]] auto register_peak_report() const
        -> std::vector<std::string> {

        // room for the comment marker and a space of the assembly output
        constexpr size_t max_line_width{78};

        std::vector<std::string> lines;

        for (const std::string& line : register_peak_lines()) {
            for (std::string& part : wrapped(line, max_line_width)) {
                lines.push_back(std::move(part));
            }
        }

        return lines;
    }

    // a blank line that separates the code from the report
    auto separate_report() -> void {
        target_assembler().add_separator_newline();
    }

    auto set_builtin_types(const type& t_i64, const type& t_i32,
                           const type& t_i16, const type& t_i8) -> void {

        type_i64_ = &t_i64;
        type_i32_ = &t_i32;
        type_i16_ = &t_i16;
        type_i8_ = &t_i8;
    }

    // what the output measured, for the report of the front end. the counts
    // are valid after 'finish' and only for buffered output
    [[nodiscard]] auto statistics() const -> output_statistics {
        const assembler& output{target_assembler()};

        output_statistics stats;
        stats.max_scratch_registers = usage_max_scratch_regs_;

        if (output.is_buffering()) {
            stats.is_counted = true;
            stats.optimizations = output.optimizations();
            stats.noinline_functions = output.noinline_summaries();
            stats.instruction_count = output.instruction_count();
        }

        return stats;
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

        assert(op == arithmetic_operator::complement);

        return "~";
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

    // breaks a line that is too long at spaces, the parts keep its indentation
    [[nodiscard]] static auto wrapped(const std::string& line,
                                      const size_t width)
        -> std::vector<std::string> {

        std::vector<std::string> parts;

        const std::string indentation(
            std::min(line.find_first_not_of(' '), line.size()), ' ');

        std::string rest{line};

        while (rest.size() > width) {
            const size_t space{rest.rfind(' ', width)};

            // note: a word longer than the width is left as it is
            if (space == std::string::npos or space <= indentation.size()) {
                break;
            }

            parts.push_back(rest.substr(0, space));

            std::string next{indentation};
            next += rest.substr(space + 1);
            // note: +1 because the space that the line broke at is dropped

            rest = std::move(next);
        }

        parts.push_back(std::move(rest));

        return parts;
    }

  protected:
    //
    // virtual methods
    //

    // the assembler the target writes its output with
    [[nodiscard]] virtual auto target_assembler() const -> assembler& = 0;

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

    // what the lower bound check is about, a bounds check that has none says
    // nothing
    auto comment_lower_bound(const token& src_loc_tk, const size_t indent,
                             const operand& reg_to_check,
                             const operand& reg_count, const bounds_plan& plan)
        -> void {

        comment(src_loc_tk, indent, "lower bound");

        if (plan.upper_covers_lower) {
            comment(src_loc_tk, indent,
                    "{} lower bound covered by the unsigned upper bound",
                    reg_to_check.base_register());

            return;
        }

        if (plan.count_known) {
            comment(src_loc_tk, indent, "count {} lower bound already checked",
                    reg_count.base_register());
        }
    }

    // the optional jump optimization of buffered output
    auto finish_output() -> void {
        assembler& output{target_assembler()};

        if (output.is_buffering() and jump_mode_ == jump_mode::optimized) {
            output.optimize_jumps();
        }
    }

    // 'pool' has just got a register, for the report of the use of registers:
    // what the frame being compiled holds itself and the busiest point
    auto record_register_use(const register_pool& pool) -> void {
        if (not register_report_enabled_) {
            return;
        }

        const size_t frame{current_call_frame()};

        callee_use& use{
            callee_uses_[call_frames_.empty() ? "code"
                                              : call_frames_.back().name],
        };

        use.own_peak = std::max(use.own_peak, held_count(pool, frame));

        size_t held{};

        for (size_t index{}; index <= frame; ++index) {
            held += held_count(pool, index);
        }

        if (held <= peak_held_) {
            return;
        }

        const std::span<const register_pool::allocation> live{
            pool.allocations(),
        };

        peak_held_ = held;
        peak_allocations_.assign(live.begin(), live.end());
        peak_frames_ = call_frames_;
    }

    // 'pool' has just got a scratch register
    auto record_scratch_registers(const register_pool& pool) -> void {
        usage_max_scratch_regs_ =
            std::max(usage_max_scratch_regs_, pool.scratch_count());
    }

    [[nodiscard]] auto source() const -> std::string_view { return source_; }

    // resolved and optimized jumps need every line before writing
    auto start_output() -> void {
        assembler& output{target_assembler()};

        output.set_direct_output(
            jump_mode_ == jump_mode::as_emitted ? &os_.get() : nullptr);

        usage_max_scratch_regs_ = 0;

        output.comment(0, "");
        output.comment(0, "generated by baz");
        output.comment(0, "");
        output.add_separator_newline();
    }

    [[nodiscard]] auto stream() const -> std::ostream& { return os_.get(); }

    //
    // statics
    //

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

    // keeps the greatest count of scratch registers in use at once
    // the registers listed in the report: those a source location allocated,
    // the register a backend holds itself is left out
    [[nodiscard]] static auto held_count(const register_pool& pool,
                                         const size_t frame) -> size_t {

        return static_cast<size_t>(std::ranges::count_if(
            pool.allocations(),
            [frame](const register_pool::allocation& allocated) -> bool {
                const bool held_by_backend{
                    allocated.named and allocated.src_loc_tk.at_line() == 0,
                };

                return allocated.frame == frame and not held_by_backend;
            }));
    }

    // immediates are decimal numbers prefixed by unary '-' and '~' operators
    // returns two's complement bits or empty for an operand that is not an
    // immediate
    [[nodiscard]] static auto immediate_bits(const operand& value)
        -> std::optional<uint64_t> {

        if (not value.is_immediate()) {
            return std::nullopt;
        }

        const std::string_view text{value.immediate()};
        const size_t digits{text.find_first_not_of("-~")};

        assert(digits != std::string_view::npos);

        const std::string_view number{text.substr(digits)};
        const char* const end{std::to_address(number.end())};
        uint64_t bits{};

        const std::from_chars_result parsed{
            std::from_chars(std::to_address(number.begin()), end, bits),
        };

        assert(parsed.ec == std::errc{} and parsed.ptr == end);

        for (const char operation :
             text.substr(0, digits) | std::views::reverse) {
            // unsigned negation wraps instead of overflowing
            if (operation == '-') {
                bits = uint64_t{} - bits;
                continue;
            }

            bits = ~bits;
        }

        return bits;
    }

    // a word or double word is sign extended so a value that fits a signed
    // 32-bit immediate is recognized
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
    [[nodiscard]] static auto
    lower_checked_register(const operand& reg_to_check,
                           const operand& reg_count) -> std::string_view {

        return reg_count.is_empty() ? reg_to_check.base_register()
                                    : reg_count.base_register();
    }

    // 'signed_max' is the largest value of the signed type of the compare,
    // 'count_known' tells that the lower bound of 'reg_count' was checked
    [[nodiscard]] static auto
    plan_bounds_check(const bounds_check_options& options,
                      const operand& reg_count, const size_t array_count,
                      const uint64_t signed_max, const bool count_known)
        -> bounds_plan {

        return {
            .upper_covers_lower{
                options.lower and options.upper and reg_count.is_empty() and
                    array_count <= signed_max,
            },
            .count_known{not reg_count.is_empty() and count_known},
        };
    }
};
