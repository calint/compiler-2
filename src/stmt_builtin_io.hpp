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

    // the entry of 'functions_' that the call names
    io_function io_{};
    builtin_arguments args_;

  public:
    stmt_builtin_io(toc& tc, const io_function& io, unary_ops uops,
                    const token tk, tokenizer& tz)
        : statement{tk, std::move(uops)}, io_{io},
          args_{read_arguments(tc, io, tk, tz)} {

        set_type(io_.result_type(tc));

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

        // e.g. the 'i' of 'read(0, bs[i].b, f())' with 'f' writing 'i'; the
        // 'b' of 'read(0, b, n)' is written
        assert_not_written_later(tc, *this, "argument");

        const place buffer{assert_buffer(tc)};

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        x.call_target(
            tok(), indent, io_.function,
            [&](const std::span<const operand> args) -> void {
                compile_arguments(tc, indent, buffer, args);
            },
            [&](const operand& result) -> void {
                get_unary_ops().compile(x, indent, tok(), result,
                                        tc.overflow_options(dst));

                if (not dst.is_empty()) {
                    x.copy_value(tok(), indent, dst.operand(), result);
                }
            });
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
        args_.visit_operands(visitor, io_.fills_buffer
                                          ? std::optional{buffer_index_}
                                          : std::nullopt);
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

        const expr_any* const start_arg{start_argument()};
        const expr_any* const count_arg{count_argument()};

        // note: a sector function has no count argument, an error about its
        //       range points at the start, or at the buffer when there is none

        const expr_any* const count_or_start{
            count_arg != nullptr ? count_arg : start_arg,
        };

        const std::optional<int64_t> start{
            start_arg != nullptr ? start_arg->constant_value(tc) : int64_t{},
        };

        // a constant start alone, e.g. the -1 of 'write(1, a, n, -1)'; a count
        // of 0 may start at the end
        if (not count) {
            if (start_arg != nullptr and start) {
                assert_constant_index(buffer_info, start_arg->tok(),
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
                    start_arg != nullptr ? std::optional{start_arg->tok()}
                                         : std::nullopt,
                },
                .count_tk{
                    count_or_start != nullptr ? count_or_start->tok()
                                              : args_.at(buffer_index_).tok(),
                },
                .count_text{
                    count_arg != nullptr
                        ? std::format("count {}", *count)
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

    // the array the call transfers, a path since 'assert_buffer_path'
    [[nodiscard]] auto buffer_path() const -> const stmt_identifier& {
        return *args_.at(buffer_index_).bare_path();
    }

    // the range is checked and the buffer address moves to the start, which
    // is 0 without a start argument
    auto check_range_from_start(toc& tc, const size_t indent,
                                const operand& buffer_reg, const operand& count,
                                const std::optional<int64_t> known_count,
                                const ident_info& buffer_info) const -> void {

        const expr_any* const start_arg{start_argument()};

        const std::optional<int64_t> constant_start{
            start_arg != nullptr ? start_arg->constant_value(tc) : int64_t{},
        };

        // note: an unsized array parameter has no length in the check of a
        //       function without its calls, see 'ident_info::is_length_unknown'

        if (constant_start and
            (start_arg == nullptr or buffer_info.array_len())) {

            move_to_constant_start(tc, indent, buffer_reg, count, known_count,
                                   buffer_info,
                                   static_cast<size_t>(*constant_start));

            return;
        }

        move_to_run_time_start(tc, indent, buffer_reg, count, known_count,
                               buffer_info);
    }

    // every argument into the register of the call the target takes it in,
    // 'args', in the order of the arguments, then the range check and the
    // byte count; the count and start are in elements
    auto compile_arguments(toc& tc, const size_t indent, const place& buffer,
                           const std::span<const operand> args) const -> void {

        // the descriptor, or the sector of a sector function
        const operand& descriptor{args.at(0)};
        const operand& buffer_reg{args.at(1)};
        const operand& count{args.at(2)};

        args_.at(0).compile(tc, indent, destination::of_register(descriptor));

        // the range is checked after the count
        buffer.ident->load_address(tc, indent, tok(), buffer.info, buffer_reg);

        const ident_info& buffer_info{buffer.info};

        const std::optional<int64_t> known_count{
            constant_count(tc, buffer_info),
        };

        compile_count(tc, indent, count, known_count);

        check_range_from_start(tc, indent, buffer_reg, count, known_count,
                               buffer_info);

        machine& x{tc.machine()};

        // the count becomes the byte count
        x.scale_index(tok(), indent, count,
                      buffer_info.type_ref().size_bytes());
    }

    // without a count the whole array is transferred, a sector function
    // transfers the elements of one sector, see 'constant_count'
    auto compile_count(toc& tc, const size_t indent, const operand& count,
                       const std::optional<int64_t> known_count) const -> void {

        if (const expr_any* const count_arg{count_argument()}) {
            count_arg->compile(tc, indent, destination::of_register(count));
            return;
        }

        machine& x{tc.machine()};

        // note: an unknown length is the check of a function without its
        //       calls, its code is discarded

        x.copy_value(tok(), indent, count,
                     operand::imm(known_count.value_or(discarded_array_len),
                                  tc.builtins().default_type()));
    }

    // the count argument, else the elements of the array or of one sector,
    // empty when the count is computed at run time or the array length is
    // not known
    [[nodiscard]] auto constant_count(toc& tc,
                                      const ident_info& buffer_info) const
        -> std::optional<int64_t> {

        if (const expr_any* const count_arg{count_argument()}) {
            return count_arg->constant_value(tc);
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

    // the count, null when left out and for a sector function, which has none
    [[nodiscard]] auto count_argument() const -> const expr_any* {
        return io_.is_sector ? nullptr : args_.find(buffer_index_ + 1);
    }

    // a constant start in an array of known length is checked at compile
    // time and moves the address by a constant, a run-time count is then
    // checked against the elements after it
    auto move_to_constant_start(toc& tc, const size_t indent,
                                const operand& buffer_reg, const operand& count,
                                const std::optional<int64_t> known_count,
                                const ident_info& buffer_info,
                                const size_t start_count) const -> void {

        const size_t element_count{buffer_info.element_count()};

        // note: a constant start is at most the length, rejected at
        //       compile time otherwise

        assert(start_count <= element_count);

        const expr_any* const start_arg{start_argument()};
        const expr_any* const count_arg{count_argument()};

        machine& x{tc.machine()};

        if (count_arg != nullptr and not known_count) {
            // the start, or the count when there is none
            const expr_any& checked_arg{
                start_arg != nullptr ? *start_arg : *count_arg,
            };

            x.check_bounds(checked_arg.tok(), indent,
                           machine::bounds_range::up_to(count, element_count -
                                                                   start_count),
                           tc.checks().bounds_options());
        }

        const size_t offset_bytes{
            start_count * buffer_info.type_ref().size_bytes(),
        };

        if (offset_bytes != 0) {
            const type& type_default{tc.builtins().default_type()};

            x.add_subtract(tok(), indent, arithmetic_operator::add, buffer_reg,
                           operand::imm(offset_bytes, type_default));
        }
    }

    // a run-time start is checked with the count, against the last start a
    // constant count leaves, e.g. 'write(1, a, 2, s)'
    auto move_to_run_time_start(toc& tc, const size_t indent,
                                const operand& buffer_reg, const operand& count,
                                const std::optional<int64_t> known_count,
                                const ident_info& buffer_info) const -> void {

        const expr_any* const start_arg{start_argument()};

        // note: without a start argument the start is the constant 0
        assert(start_arg != nullptr);

        const std::optional<size_t> array_len{buffer_info.array_len()};
        const size_t element_count{buffer_info.element_count()};
        const type& type_default{tc.builtins().default_type()};

        machine& x{tc.machine()};

        const operand start{
            x.alloc_scratch_register(start_arg->tok(), indent, type_default),
        };

        start_arg->compile(tc, indent, destination::of_register(start));

        // note: a constant count past the end is rejected at compile time
        assert(not known_count or not array_len or
               (*known_count >= 0 and
                std::cmp_less_equal(*known_count, element_count)));

        x.check_bounds(
            start_arg->tok(), indent,
            known_count and array_len
                ? machine::bounds_range::up_to(
                      start, element_count - static_cast<size_t>(*known_count))
                : machine::bounds_range::range(start, count, element_count),
            tc.checks().bounds_options());

        x.scale_index(tok(), indent, start,
                      buffer_info.type_ref().size_bytes());

        x.add_subtract(tok(), indent, arithmetic_operator::add, buffer_reg,
                       start);

        x.free_scratch_register(tok(), indent, start);
    }

    // the start, the last argument a call can have, null when left out
    [[nodiscard]] auto start_argument() const -> const expr_any* {
        return args_.find(io_.max_arg_count() - 1);
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

    // the arguments after the checks of the call itself, made first as for a
    // call of a function, e.g. 'write' in the initializer of a global
    [[nodiscard]] static auto read_arguments(toc& tc, const io_function& io,
                                             const token& tk, tokenizer& tz)
        -> builtin_arguments {

        assert_call_in_func(tc, tk);

        const machine& x{tc.machine()};

        if (io.is_sector and not x.has_sd_card()) {
            throw compile_error{tk, "the target has no sd card"};
        }

        return {
            tc,
            tz,
            tk,
            array_indexes_,
            io_function::min_arg_count(),
            io.max_arg_count(),
        };
    }

    [[nodiscard]] static auto
    sector_element_count(toc& tc, const ident_info& buffer_info) -> size_t {

        const machine& x{tc.machine()};
        return x.sector_size_bytes() / buffer_info.type_ref().size_bytes();
    }
};
