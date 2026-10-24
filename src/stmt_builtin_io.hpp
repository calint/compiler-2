#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "ub_order.hpp"
#include "unary_ops.hpp"

// 'read(fd, buf, count, start)' and 'write(fd, buf, count, start)', and the
// sd card functions 'sector_read(sector, buf, start)' and
// 'sector_write(sector, buf, start)' that transfer one sector
class stmt_builtin_io final : public expression {
  public:
    // what tells the functions apart, a sector function takes a sector in
    // place of the descriptor and has no count and no result
    struct io_function {
        std::string_view name;
        machine::target_call function{};
        size_t min_arg_count{};
        size_t max_arg_count{};
        bool fills_buffer{};
        bool is_sector{};

        [[nodiscard]] constexpr auto has_result() const -> bool {
            return not is_sector;
        }

        // the count of a transfer, nothing for a sector function
        [[nodiscard]] auto result_type(const toc& tc) const -> const type& {
            return has_result() ? tc.builtins().default_type()
                                : tc.builtins().void_type();
        }
    };

  private:
    static constexpr std::array<io_function, 4> functions_{
        io_function{
            .name{"read"},
            .function{machine::target_call::read},
            .min_arg_count{2},
            .max_arg_count{4},
            .fills_buffer{true},
            .is_sector{false},
        },
        io_function{
            .name{"write"},
            .function{machine::target_call::write},
            .min_arg_count{2},
            .max_arg_count{4},
            .fills_buffer{false},
            .is_sector{false},
        },
        io_function{
            .name{"sector_read"},
            .function{machine::target_call::sector_read},
            .min_arg_count{2},
            .max_arg_count{3},
            .fills_buffer{true},
            .is_sector{true},
        },
        io_function{
            .name{"sector_write"},
            .function{machine::target_call::sector_write},
            .min_arg_count{2},
            .max_arg_count{3},
            .fills_buffer{false},
            .is_sector{true},
        },
    };

    // the buffer follows the descriptor or the sector
    static constexpr size_t buffer_index_{1};

    // where the optional arguments are, e.g. 2 and 3 of 'read(0, b, 4, 2)',
    // or the start 2 of 'sector_read(s, b, 2)', which has no count
    struct optional_positions {
        std::optional<size_t> count;
        std::optional<size_t> start;
    };

    // the entry of 'functions_' that the call names
    io_function io_;
    builtin_arguments args_;
    // made once from the count of the arguments
    optional_positions at_;

  public:
    stmt_builtin_io(toc& tc, const io_function& io, unary_ops uops,
                    const token tk, tokenizer& tz)
        : expression{tk, std::move(uops)}, io_{io}, args_{tc, tz} {

        set_type(io_.result_type(tc));

        if (io_.is_sector) {
            assert_target_has_sd_card(tc);
        }

        assert_argument_count();
        at_ = optional_positions_of(io_, args_.count());
        assert_buffer_path();

        if (not io_.fills_buffer) {
            return;
        }

        // 'read' and 'sector_read' fill the buffer
        if (const std::optional<place> filled{as_place(tc, buffer_path())};
            filled) {

            buffer_path().record_write(tc, buffer_path().tok(), "read into",
                                       filled->info);
        }
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        args_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        assert_call_in_func(tc, tok());

        // e.g. the 'b' of 'read(0, b, n)' is written
        args_.assert_not_written_later(
            tc, io_.fills_buffer ? std::optional<size_t>{buffer_index_}
                                 : std::nullopt);

        assert_count_not_written_by_buffer(tc);

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const machine::target_call_registers registers{
            x.registers_for_target_call(io_.function),
        };

        const std::vector<operand> args{
            compile_array_arguments(tc, indent, registers.arguments),
        };

        if (io_.is_sector) {
            x.call_target(tok(), indent, io_.function, args, operand{});
            x.free_named_registers(tok(), indent, args);
            return;
        }

        const bool result_is_argument{
            std::ranges::contains(registers.arguments, registers.result),
        };

        const operand result{
            result_is_argument
                ? x.make_register_operand(registers.result,
                                          tc.builtins().default_type())
                : x.alloc_named_register(tok(), indent, registers.result,
                                         tc.builtins().default_type()),
        };

        x.call_target(tok(), indent, io_.function, args, result);

        get_unary_ops().compile(x, indent, tok(), result,
                                tc.overflow_options(dst));

        if (not dst.is_empty()) {
            x.copy_value(tok(), indent, dst.operand(), result);
        }

        if (not result_is_argument) {
            x.free_named_register(tok(), indent, result);
        }

        x.free_named_registers(tok(), indent, args);
    }

    // the count has the default type, like the result of a call
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    // 'read' and 'sector_read' write their buffer, e.g. 'b' of 'read(0, b)'
    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        args_.visit_call_effects(tc, visitor);

        if (not io_.fills_buffer) {
            return;
        }

        if (not as_place(tc, buffer_path())) {
            return;
        }

        const std::optional<storage_path> written{buffer_path().storage(tc)};

        // note: a place is a variable, which has storage
        assert(written);

        visitor({
            .call_tk{tok()},
            .name{buffer_path().dotted_path()},
            .storage{*written},
            .is_write{true},
        });
    }

    // the buffer that 'read' and 'sector_read' fill is written, only the
    // indexes in its path are read
    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        for (size_t i{}; i < args_.count(); ++i) {
            if (i == buffer_index_ and io_.fills_buffer) {
                buffer_path().visit_index_reads(var, reader);
            } else {
                args_.at(i).visit_reads(var, reader);
            }
        }
    }

    //
    // statics
    //

    // the io function named 'name', null for any other name
    [[nodiscard]] static auto find(const std::string_view name)
        -> const io_function* {

        // note: a user definition with the name of an io function is
        //       rejected because the name is in the language-level list,
        //       which also tells whether the call has a value

        constexpr auto is_cataloged{
            [](const io_function& io) -> bool {
                const reserved_names::builtin_function* const fn{
                    reserved_names::find_builtin_function(io.name),
                };

                return fn != nullptr and
                       io.has_result() ==
                           (fn->value == reserved_names::builtin_value::count);
            },
        };

        static_assert(std::ranges::all_of(functions_, is_cataloged));

        for (const io_function& info : functions_) {
            if (info.name == name) {
                return &info;
            }
        }

        return nullptr;
    }

  private:
    // e.g. "'write' takes 2 to 4 arguments" or "'sector_read' takes 2 or 3
    // arguments"
    auto assert_argument_count() const -> void {
        const size_t min_count{io_.min_arg_count};
        const size_t max_count{io_.max_arg_count};

        if (args_.count() >= min_count and args_.count() <= max_count) {
            return;
        }

        const std::string_view conjunction{
            max_count == min_count + 1 ? "or" : "to",
        };

        throw compile_error{tok(),
                            std::format("{} takes {} {} {} arguments",
                                        in_quotes(tok().text()), min_count,
                                        conjunction, max_count)};
    }

    // an address would bypass the bounds check, so only arrays are accepted
    [[nodiscard]] auto assert_array_buffer(const toc& tc) const -> place {
        if (std::optional<place> array{as_place(tc, buffer_path())};
            array and array->info.is_array()) {

            return std::move(*array);
        }

        throw_not_an_array();
    }

    // the buffer is named by a path, the rest of 'assert_array_buffer' needs
    // the types at compile time
    auto assert_buffer_path() const -> void {
        const statement& arg{args_.at(buffer_index_)};
        const stmt_identifier* const path{arg.operand_path()};

        // the start has one spelling, the last argument
        if (path != nullptr and path->is_array_element()) {
            throw compile_error{
                arg.tok(),
                std::format("pass the array and the start as {} argument, "
                            "e.g. {}",
                            io_.is_sector ? "3rd" : "4th",
                            in_quotes(usage_text()))};
        }

        // e.g. not '-b', a unary operator makes a value
        if (arg.place_path() == nullptr) {
            throw_not_an_array();
        }
    }

    // a constant range past the end of the buffer is rejected like the range
    // of 'array_copy', e.g. 'write(1, a, 5)' of 'a = i8[4]', a run-time one is
    // left to the bounds check at run time
    auto assert_constant_range_in_bounds(toc& tc, const place& buffer) const
        -> void {

        const ident_info& buffer_info{buffer.info};

        const std::optional<int64_t> count{constant_count(tc, buffer_info)};

        if (not count) {
            return;
        }

        const std::optional<int64_t> start{
            at_.start ? args_.constant(tc, *at_.start) : int64_t{},
        };

        assert_constant_range(
            buffer_info,
            {
                .start{start},
                .count{*count},
                .start_tk{
                    at_.start ? std::optional{args_.at(*at_.start).tok()}
                              : std::nullopt,
                },

                // note: a sector function has no count argument, an error
                //       about its range points at the start, or at the
                //       buffer when there is none

                .count_tk{
                    at_.count
                        ? args_.at(*at_.count).tok()
                        : args_.at(at_.start.value_or(buffer_index_)).tok(),
                },
                .count_text{
                    at_.count ? std::format("count {}", *count)
                              : std::format("a sector of {} elements", *count),
                },
                .array_name{buffer.path->dotted_path()},
            });
    }

    // the count and the start are computed before the buffer, its range check
    // needs them; the call is accepted only when that order and the source
    // order give the same result, e.g. 'read(0, a[g()].buf, n)' with 'g'
    // writing 'n' is rejected
    auto assert_count_not_written_by_buffer(const toc& tc) const -> void {
        std::vector<list_operand> computed_first;
        computed_first.reserve(args_.count());

        for (size_t i{buffer_index_ + 1}; i < args_.count(); ++i) {
            computed_first.push_back({
                .node{&args_.at(i)},
                .written_place{},
            });
        }

        const std::array<list_operand, 1> buffer{
            list_operand{
                .node{&args_.at(buffer_index_)},
                .written_place{},
            },
        };

        assert_not_written_after(tc, computed_first, buffer,
                                 "the buffer argument, the count and the start "
                                 "are computed before the buffer");
    }

    // a sector is a whole number of elements and fits in the array
    auto assert_sector_buffer(toc& tc, const ident_info& buffer_info) const
        -> void {

        const machine& x{tc.machine()};

        const size_t sector_size_bytes{x.sector_size_bytes()};

        const size_t element_size_bytes{buffer_info.type_ref().size_bytes()};

        if (sector_size_bytes % element_size_bytes != 0) {
            throw compile_error{
                buffer_path().tok(),
                std::format("the element size {} B does not divide the "
                            "sector size {} B",
                            element_size_bytes, sector_size_bytes)};
        }

        const std::optional<size_t> array_len{buffer_info.array_len()};

        // e.g. a parameter checked without a call
        if (not array_len) {
            return;
        }

        const size_t buffer_size_bytes{*array_len * element_size_bytes};

        if (buffer_size_bytes < sector_size_bytes) {
            throw compile_error{
                buffer_path().tok(),
                std::format("the array of {} B is smaller than a sector of "
                            "{} B",
                            buffer_size_bytes, sector_size_bytes)};
        }
    }

    auto assert_target_has_sd_card(toc& tc) const -> void {
        const machine& x{tc.machine()};

        if (x.sector_size_bytes() == 0) {
            throw compile_error{tok(), "the target has no sd card"};
        }
    }

    // the array the call transfers, a path since 'assert_buffer_path'
    [[nodiscard]] auto buffer_path() const -> const stmt_identifier& {
        return *args_.at(buffer_index_).place_path();
    }

    // the count and start are in elements, the byte count is computed last
    [[nodiscard]] auto compile_array_arguments(
        toc& tc, const size_t indent,
        const std::span<const std::string_view> registers) const
        -> std::vector<operand> {

        const place buffer{assert_array_buffer(tc)};

        machine& x{tc.machine()};

        const type& type_default{tc.builtins().default_type()};

        std::vector<operand> args;
        args.reserve(registers.size());
        for (const std::string_view name : registers) {
            args.push_back(
                x.alloc_named_register(tok(), indent, name, type_default));
        }

        // the descriptor, or the sector of a sector function
        const operand& descriptor{args.at(0)};
        const operand& buffer_reg{args.at(1)};
        const operand& count{args.at(2)};

        args_.at(0).compile(tc, indent, destination::of_register(descriptor));

        const ident_info& buffer_info{buffer.info};

        if (io_.is_sector) {
            assert_sector_buffer(tc, buffer_info);
        }

        compile_count(tc, indent, count, buffer_info);

        const range_check check{range_check_of(tc, buffer_info)};

        operand start;

        if (at_.start) {
            start = compile_start(tc, indent, *at_.start, count, buffer_info,
                                  check);
        }

        assert_constant_range_in_bounds(tc, buffer);

        // a count with a start is checked by 'compile_start', a bare count by
        // 'compile_lea'
        const bool is_count_checked_by_lea{
            at_.count and not at_.start and check == range_check::run_time,
        };

        compile_buffer_address(tc, indent, buffer_reg, buffer,
                               is_count_checked_by_lea ? count : operand{});

        const operand element_size_bytes{
            operand::imm(buffer_info.type_ref().size_bytes(), type_default),
        };

        convert_to_bytes(tc, indent, buffer_reg, count, start,
                         element_size_bytes);

        return args;
    }

    // the buffer register receives the address of the array, 'range' is the
    // count that a bare count check covers
    auto compile_buffer_address(toc& tc, const size_t indent,
                                const operand& buffer_reg, const place& buffer,
                                const operand& range) const -> void {

        machine& x{tc.machine()};

        const stmt_identifier& path{*buffer.path};

        std::vector<operand> lea_registers;

        const operand buffer_lea{
            path.compile_lea(
                tc, indent, path.tok(), lea_registers,
                {
                    .reg_count{range},
                    .address_by_call{buffer.info.address_by_call()},
                    .address_register{},
                }),
        };

        x.address_of(tok(), indent, buffer_reg, buffer_lea);

        x.free_scratch_registers(tok(), indent, lea_registers);
    }

    // without a count the whole array is transferred, a sector function
    // transfers the elements of one sector
    auto compile_count(toc& tc, const size_t indent, const operand& count,
                       const ident_info& buffer_info) const -> void {

        if (at_.count) {
            args_.at(*at_.count)
                .compile(tc, indent, destination::of_register(count));

            return;
        }

        const size_t element_count{
            io_.is_sector ? sector_element_count(tc, buffer_info)
                          : buffer_info.element_count(),
        };

        machine& x{tc.machine()};

        x.copy_value(tok(), indent, count,
                     operand::imm(element_count, tc.builtins().default_type()));
    }

    // the start register stays allocated for the caller
    [[nodiscard]] auto compile_start(toc& tc, const size_t indent,
                                     const size_t start_index,
                                     const operand& count,
                                     const ident_info& buffer_info,
                                     const range_check check) const -> operand {

        machine& x{tc.machine()};

        const statement& start_arg{args_.at(start_index)};

        const operand start{
            x.alloc_scratch_register(start_arg.tok(), indent,
                                     tc.builtins().default_type()),
        };

        start_arg.compile(tc, indent, destination::of_register(start));

        if (check == range_check::compile_time) {
            return start;
        }

        x.check_bounds(start_arg.tok(), indent,
                       machine::bounds_range::range(
                           start, count, buffer_info.element_count()),
                       tc.checks().bounds_options());

        return start;
    }

    // the count argument, else the elements of the array or of one sector,
    // empty when the count is computed at run time or the array length is
    // not known
    [[nodiscard]] auto constant_count(toc& tc,
                                      const ident_info& buffer_info) const
        -> std::optional<int64_t> {

        if (at_.count) {
            return args_.constant(tc, *at_.count);
        }

        if (io_.is_sector) {
            return static_cast<int64_t>(sector_element_count(tc, buffer_info));
        }

        const std::optional<size_t> array_len{buffer_info.array_len()};

        if (not array_len) {
            return std::nullopt;
        }

        return static_cast<int64_t>(*array_len);
    }

    // the start moves the buffer address and the count becomes the byte count
    auto convert_to_bytes(toc& tc, const size_t indent,
                          const operand& buffer_reg, const operand& count,
                          const operand& start,
                          const operand& element_size_bytes) const -> void {

        machine& x{tc.machine()};

        if (at_.start) {
            x.multiply(tok(), indent, start, element_size_bytes,
                       machine::factor_use::read_only);

            x.add_subtract(tok(), indent, machine::arithmetic_operator::add,
                           buffer_reg, start);

            x.free_scratch_register(tok(), indent, start);
        }

        x.multiply(tok(), indent, count, element_size_bytes,
                   machine::factor_use::read_only);
    }

    // see 'range_check', a sector function has the constant count of a sector
    [[nodiscard]] auto range_check_of(toc& tc,
                                      const ident_info& buffer_info) const
        -> range_check {

        if (not constant_count(tc, buffer_info)) {
            return range_check::run_time;
        }

        if (at_.start and not args_.constant(tc, *at_.start)) {
            return range_check::run_time;
        }

        return range_check::compile_time;
    }

    [[noreturn]] auto throw_not_an_array() const -> void {
        const statement& arg{args_.at(buffer_index_)};

        throw compile_error{
            arg.tok(),
            std::format("{} is not an array", in_quotes(trimmed_source(arg)))};
    }

    // the call with all its arguments
    [[nodiscard]] auto usage_text() const -> std::string {
        if (io_.is_sector) {
            return std::format("{}(sector, buf, start)", tok().text());
        }

        return std::format("{}(fd, buf, count, start)", tok().text());
    }

    //
    // statics
    //

    // 'read(fd, buf, count, start)': the count and the start are optional,
    // a sector function has no count
    [[nodiscard]] static auto optional_positions_of(const io_function& io,
                                                    const size_t arg_count)
        -> optional_positions {

        const bool has_count{
            not io.is_sector and arg_count > buffer_index_ + 1,
        };

        const bool has_start{arg_count == io.max_arg_count};

        return {
            .count{has_count ? std::optional{buffer_index_ + 1} : std::nullopt},
            .start{has_start ? std::optional{arg_count - 1} : std::nullopt},
        };
    }

    [[nodiscard]] static auto
    sector_element_count(toc& tc, const ident_info& buffer_info) -> size_t {

        const machine& x{tc.machine()};
        return x.sector_size_bytes() / buffer_info.type_ref().size_bytes();
    }
};
