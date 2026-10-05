#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "assembler.hpp"
#include "decouple.hpp"
#include "token.hpp"

class type;

class machine {
    std::reference_wrapper<std::ostream> os_;
    std::string_view source_;
    assembler::jump_mode jump_mode_;
    const type* type_i64_{};
    const type* type_i32_{};
    const type* type_i16_{};
    const type* type_i8_{};
    size_t usage_max_scratch_regs_{};

  protected:
    // buffered modes hold output from 'start' to 'finish' so jumps can be
    // optimized and grown to reach their targets
    using jump_mode = assembler::jump_mode;

  public:
    enum class builtin_function : uint8_t { read, write, exit };

    // receives an address while the registers that built it are allocated
    using address_use = std::function_ref<void(const operand& address)>;

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
    // emits one address of a bulk operation: 'count' is the register of the
    // element count, empty for a known size, 'preferred' a register the address
    // may be built in, empty when the backend has none to offer
    using address_emitter = std::function_ref<void(
        const operand& count, const operand& preferred, address_use use)>;

  public:
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

    // a failed check prints its message to this descriptor and exits with this
    // code
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
                              const char operation, const operand& dst,
                              const operand& src) -> void = 0;

    virtual auto address_of(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& address)
        -> void = 0;

    [[nodiscard]] virtual auto address_size_bytes() const -> size_t = 0;

    // 'limit' is the constant array size or a register holding a count
    virtual auto advance_array_iteration(
        const token& src_loc_tk, const size_t indent, const operand& iterator,
        const operand& counter, const size_t element_size_bytes,
        const operand& limit, const std::string_view loop_label) -> void = 0;

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
                 const size_t element_size_bytes, const size_t alignment,
                 const std::function_ref<void(const operand&)> emit_count,
                 const address_emitter emit_left,
                 const address_emitter emit_right, const operand& dst,
                 const bool inverted) -> void = 0;

    virtual auto begin_data(const size_t alignment) -> void = 0;

    virtual auto bitwise(const token& src_loc_tk, const size_t indent,
                         const char operation, const operand& dst,
                         const operand& src) -> void = 0;

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

    // copies the count of elements 'emit_count' compiles, 'emit_source' and
    // 'emit_destination' emit the addresses
    virtual auto
    copy_elements(const token& src_loc_tk, const size_t indent,
                  const size_t element_size_bytes, const size_t alignment,
                  const std::function_ref<void(const operand&)> emit_count,
                  const address_emitter emit_source,
                  const address_emitter emit_destination) -> void = 0;

    virtual auto copy_value(const token& src_loc_tk, const size_t indent,
                            const operand& dst, const operand& src) -> void = 0;

    [[nodiscard]] virtual auto data_alignment() const -> size_t = 0;

    [[nodiscard]] virtual auto default_type() const -> const type& = 0;

    virtual auto define_constant(const std::string_view name,
                                 const size_t value) -> void = 0;

    virtual auto divide(const token& src_loc_tk, const size_t indent,
                        const char operation, const operand& dst,
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

    // compares 'size_bytes' of two arrays and puts 1 into 'dst' when they are
    // equal, 'alignment' is the alignment known for both addresses
    virtual auto memory_equal(const token& src_loc_tk, const size_t indent,
                              const size_t size_bytes, const size_t alignment,
                              const address_emitter emit_left,
                              const address_emitter emit_right,
                              const operand& dst, const bool inverted)
        -> void = 0;

    virtual auto multiply(const token& src_loc_tk, const size_t indent,
                          const operand& product, const operand& factor,
                          const bool reuse_source = false) -> void = 0;

    virtual auto read(const token& src_loc_tk, const size_t indent,
                      const operand& dst, const operand& descriptor,
                      const operand& address, const operand& count) -> void = 0;

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

    virtual auto shift(const token& src_loc_tk, const size_t indent,
                       const char operation, const operand& dst,
                       const operand& count) -> void = 0;

    virtual auto start() -> void = 0;

    virtual auto store_boolean(const token& src_loc_tk, const size_t indent,
                               const operand& dst, const bool value)
        -> void = 0;

    virtual auto unary(const token& src_loc_tk, const size_t indent,
                       const char operation, const operand& dst) -> void = 0;

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

        // buffered because comments are otherwise written as emitted
        target_assembler().emit_buffered(
            [&] -> void { std::ignore = target_assembler().capture(emit); });

        usage_max_scratch_regs_ = max_scratch_regs;
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

    // the optional jump optimization of buffered output
    auto finish_output() -> void {
        assembler& output{target_assembler()};

        if (output.is_buffering() and jump_mode_ == jump_mode::optimized) {
            output.optimize_jumps();
        }
    }

    // keeps the greatest count of scratch registers in use at once
    auto record_scratch_register_count(const size_t count) -> void {
        usage_max_scratch_regs_ = std::max(usage_max_scratch_regs_, count);
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
};
