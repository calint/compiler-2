#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
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
class stmt_builtin_io final : public statement {
    // the buffer follows the descriptor or the sector
    static constexpr size_t buffer_index_{1};

  public:
    // what tells the functions apart, a sector function takes a sector in
    // place of the descriptor and has no count and no result
    struct io_function {
        std::string_view name;
        machine::target_call function{};
        bool fills_buffer{};
        bool is_sector{};

        [[nodiscard]] constexpr auto has_result() const -> bool {
            return not is_sector;
        }

        // also the count and the start of a transfer, the start of a sector
        // function
        [[nodiscard]] constexpr auto max_arg_count() const -> size_t {
            return is_sector ? buffer_index_ + 2 : buffer_index_ + 3;
        }

        // the count of a transfer, nothing for a sector function
        [[nodiscard]] auto result_type(const toc& tc) const -> const type& {
            return has_result() ? tc.builtins().default_type()
                                : tc.builtins().void_type();
        }

        //
        // statics
        //

        // the descriptor or the sector, and the buffer
        [[nodiscard]] static constexpr auto min_arg_count() -> size_t {
            return buffer_index_ + 1;
        }
    };

  private:
    static constexpr std::array<io_function, 4> functions_{
        io_function{
            .name{"read"},
            .function{machine::target_call::read},
            .fills_buffer{true},
            .is_sector{false},
        },
        io_function{
            .name{"write"},
            .function{machine::target_call::write},
            .fills_buffer{false},
            .is_sector{false},
        },
        io_function{
            .name{"sector_read"},
            .function{machine::target_call::sector_read},
            .fills_buffer{true},
            .is_sector{true},
        },
        io_function{
            .name{"sector_write"},
            .function{machine::target_call::sector_write},
            .fills_buffer{false},
            .is_sector{true},
        },
    };
    static constexpr std::array<size_t, 1> array_indexes_{buffer_index_};

    // where the optional arguments are, e.g. 2 and 3 of 'read(0, b, 4, 2)',
    // or the start 2 of 'sector_read(s, b, 2)', which has no count
    struct optional_positions {
        std::optional<size_t> count;
        std::optional<size_t> start;
    };

    // the entry of 'functions_' that the call names
    io_function io_{};
    builtin_arguments args_;
    // made once from the count of the arguments
    optional_positions at_{};

  public:
    stmt_builtin_io(toc& tc, const io_function& io, unary_ops uops,
                    const token tk, tokenizer& tz)
        : statement{tk, std::move(uops)}, io_{io},
          args_{tc,
                tz,
                tk,
                array_indexes_,
                io_function::min_arg_count(),
                io.max_arg_count()},
          at_{optional_positions_of(io_, args_.count())} {

        assert_call_in_func(tc, tok());

        set_type(io_.result_type(tc));

        if (io_.is_sector) {
            assert_target_has_sd_card(tc);
        }

        assert_buffer_path();

        if (not io_.fills_buffer) {
            return;
        }

        // 'read' and 'sector_read' fill the buffer
        buffer_path().record_write(tc, "read into");
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

        const std::vector<list_operand> operands{operands_of(*this)};

        // e.g. the 'i' of 'read(0, a[i], f())' with 'f' writing 'i'; the 'b'
        // of 'read(0, b, n)' is written
        assert_not_written_later(tc, operands, "argument");

        const place buffer{assert_buffer(tc)};

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const target_call_names& registers{x.target_call_registers()};

        const std::vector<operand> args{
            compile_arguments(tc, indent, registers.arguments, buffer),
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

    // 'read' and 'sector_read' write their buffer, e.g. 'b' of 'read(0, b)'
    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        statement::visit_call_effects(tc, visitor);

        if (not io_.fills_buffer) {
            return;
        }

        const std::optional<storage_path> written{buffer_path().storage(tc)};

        // note: the buffer is an array path ('assert_buffer_path'), a
        //       variable, which has storage

        assert(written);

        visitor({
            .call_tk{tok()},
            .name{buffer_path().dotted_path()},
            .storage{*written},
            .is_write{true},
        });
    }

    // the arguments in their order, the buffer that 'read' and
    // 'sector_read' fill is written
    auto visit_operands(const operand_visitor visitor) const -> void override {
        for (size_t i{}; i < args_.count(); ++i) {
            const bool is_written{i == buffer_index_ and io_.fills_buffer};

            visitor({
                .node{&args_.at(i)},
                .written_place{is_written ? &buffer_path() : nullptr},
            });
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

                // note: a call with a count may also stand alone
                return fn != nullptr and
                       io.has_result() ==
                           (fn->value ==
                            reserved_names::builtin_value::count) and
                       io.has_result() ==
                           (fn->use == reserved_names::builtin_use::either);
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
    // the buffer an array, of whole sectors for a sector function, and a
    // constant range in its bounds: every verdict that needs no code, made
    // before the arguments are compiled
    [[nodiscard]] auto assert_buffer(toc& tc) const -> place {
        // note: the parse took an array path ('assert_buffer_path'), an
        //       address would bypass the bounds check

        std::optional<place> array{as_place(tc, buffer_path())};

        assert(array and array->info.is_array());

        const place buffer{std::move(*array)};

        if (io_.is_sector) {
            assert_sector_buffer(tc, buffer.info);
        }

        assert_constant_range_in_bounds(tc, buffer);

        return buffer;
    }

    // the buffer is an array, not an element ('builtin_arguments' took an
    // array path), the sector size and the constant range need the types at
    // compile time
    auto assert_buffer_path() const -> void {
        const stmt_identifier& buffer{buffer_path()};

        // the start has one spelling, the last argument
        if (buffer.is_array_element()) {
            throw compile_error{
                args_.at(buffer_index_).tok(),
                std::format("pass the array and the start as {} argument, "
                            "e.g. {}",
                            io_.is_sector ? "3rd" : "4th",
                            in_quotes(usage_text()))};
        }
    }

    // a constant range past the end of the buffer is rejected like the range
    // of 'array_copy', e.g. 'write(1, a, 5)' of 'a = i8[4]', a run-time one is
    // left to the bounds check at run time
    auto assert_constant_range_in_bounds(toc& tc, const place& buffer) const
        -> void {

        const ident_info& buffer_info{buffer.info};

        const std::optional<int64_t> count{constant_count(tc, buffer_info)};

        const std::optional<int64_t> start{
            at_.start ? args_.constant(tc, *at_.start) : int64_t{},
        };

        // a constant start alone, e.g. the -1 of 'write(1, a, n, -1)'; a count
        // of 0 may start at the end
        if (not count) {
            if (at_.start and start) {
                assert_constant_index(buffer_info, args_.at(*at_.start).tok(),
                                      buffer.ident->dotted_path(), *start,
                                      true);
            }

            return;
        }

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
                .array_name{buffer.ident->dotted_path()},
            });
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

        if (not x.has_sd_card()) {
            throw compile_error{tok(), "the target has no sd card"};
        }
    }

    // the array the call transfers, a path since 'assert_buffer_path'
    [[nodiscard]] auto buffer_path() const -> const stmt_identifier& {
        return *args_.at(buffer_index_).bare_path();
    }

    // every argument into the register of the call the target takes it in,
    // in the order of the arguments, then the range check and the byte
    // count; the count and start are in elements
    [[nodiscard]] auto
    compile_arguments(toc& tc, const size_t indent,
                      const std::span<const std::string_view> registers,
                      const place& buffer) const -> std::vector<operand> {

        const type& type_default{tc.builtins().default_type()};

        machine& x{tc.machine()};

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

        // the range is checked after the count
        buffer.ident->load_address(tc, indent, tok(), buffer.info, buffer_reg);

        const ident_info& buffer_info{buffer.info};

        compile_count(tc, indent, count, buffer_info);

        const std::optional<int64_t> known_count{
            constant_count(tc, buffer_info),
        };

        if (at_.start) {
            move_to_start(tc, indent, buffer_reg, count, known_count,
                          buffer_info);
        } else if (at_.count and not known_count) {
            x.check_bounds(args_.at(*at_.count).tok(), indent,
                           machine::bounds_range::count_alone(
                               count, buffer_info.element_count()),
                           tc.checks().bounds_options());
        }

        // the count becomes the byte count
        x.scale_index(tok(), indent, count,
                      buffer_info.type_ref().size_bytes());

        return args;
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

    // the buffer address moves to the start; a constant start in an array of
    // known length is checked at compile time and moves it by a constant, a
    // run-time count is then checked against the elements after the start; a
    // run-time start is checked against the last start a constant count
    // leaves, e.g. 'write(1, a, 2, s)'
    auto move_to_start(toc& tc, const size_t indent, const operand& buffer_reg,
                       const operand& count,
                       const std::optional<int64_t> known_count,
                       const ident_info& buffer_info) const -> void {

        // note: only a call with a start asks
        assert(at_.start);

        const size_t start_index{*at_.start};
        const statement& start_arg{args_.at(start_index)};
        const size_t element_count{buffer_info.element_count()};
        const size_t element_size_bytes{buffer_info.type_ref().size_bytes()};
        const type& type_default{tc.builtins().default_type()};

        const std::optional<int64_t> constant_start{
            args_.constant(tc, start_index),
        };

        machine& x{tc.machine()};

        // note: an unsized array parameter has no length in the check of a
        //       function without its calls, see 'ident_info::is_length_unknown'

        if (constant_start and buffer_info.array_len()) {
            // note: a constant start is at most the length, rejected at
            //       compile time otherwise

            assert(*constant_start >= 0 and
                   std::cmp_less_equal(*constant_start, element_count));

            const size_t start_count{static_cast<size_t>(*constant_start)};

            if (not known_count) {
                x.check_bounds(start_arg.tok(), indent,
                               machine::bounds_range::count_alone(
                                   count, element_count - start_count),
                               tc.checks().bounds_options());
            }

            const size_t offset_bytes{start_count * element_size_bytes};

            if (offset_bytes != 0) {
                x.add_subtract(tok(), indent, arithmetic_operator::add,
                               buffer_reg,
                               operand::imm(offset_bytes, type_default));
            }

            return;
        }

        const operand start{
            x.alloc_scratch_register(start_arg.tok(), indent, type_default),
        };

        start_arg.compile(tc, indent, destination::of_register(start));

        x.check_bounds(start_arg.tok(), indent,
                       range_of_start(start, count, known_count, buffer_info),
                       tc.checks().bounds_options());

        x.scale_index(tok(), indent, start, element_size_bytes);

        x.add_subtract(tok(), indent, arithmetic_operator::add, buffer_reg,
                       start);

        x.free_scratch_register(tok(), indent, start);
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

        const bool has_start{arg_count == io.max_arg_count()};

        return {
            .count{has_count ? std::optional{buffer_index_ + 1} : std::nullopt},
            .start{has_start ? std::optional{arg_count - 1} : std::nullopt},
        };
    }

    // a constant count in an array of known length leaves a last start, else
    // the start and the count are checked together
    [[nodiscard]] static auto
    range_of_start(const operand& start, const operand& count,
                   const std::optional<int64_t> known_count,
                   const ident_info& buffer_info) -> machine::bounds_range {

        const size_t element_count{buffer_info.element_count()};

        if (not known_count or not buffer_info.array_len()) {
            return machine::bounds_range::range(start, count, element_count);
        }

        // note: a constant count past the end is rejected at compile time
        assert(*known_count >= 0 and
               std::cmp_less_equal(*known_count, element_count));

        return machine::bounds_range::range_start(
            start, element_count - static_cast<size_t>(*known_count));
    }

    [[nodiscard]] static auto
    sector_element_count(toc& tc, const ident_info& buffer_info) -> size_t {

        const machine& x{tc.machine()};
        return x.sector_size_bytes() / buffer_info.type_ref().size_bytes();
    }
};
