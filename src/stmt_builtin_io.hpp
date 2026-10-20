#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
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
#include "unary_ops.hpp"

// 'read(fd, buf, count, start)' and 'write(fd, buf, count, start)', and the
// sd card functions 'sector_read(sector, buf, start)' and
// 'sector_write(sector, buf, start)' that transfer one sector
class stmt_builtin_io final : public stmt_call {
    // what tells the functions apart, a sector function takes a sector in
    // place of the descriptor and has no count and no result
    struct io_function {
        std::string_view name;
        machine::builtin_function function{};
        bool fills_buffer{};
        bool is_sector{};
    };

    static constexpr std::array<io_function, 4> functions_{
        io_function{
            .name{"read"},
            .function{machine::builtin_function::read},
            .fills_buffer{true},
            .is_sector{false},
        },
        io_function{
            .name{"write"},
            .function{machine::builtin_function::write},
            .fills_buffer{false},
            .is_sector{false},
        },
        io_function{
            .name{"sector_read"},
            .function{machine::builtin_function::sector_read},
            .fills_buffer{true},
            .is_sector{true},
        },
        io_function{
            .name{"sector_write"},
            .function{machine::builtin_function::sector_write},
            .fills_buffer{false},
            .is_sector{true},
        },
    };

  public:
    stmt_builtin_io(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
        : stmt_call{tc, std::move(uops), tk, tz, nullptr} {

        if (is_sector_function()) {
            assert_sector_function_call(tc);
            set_type(tc.builtins().void_type());
        } else if (argument_count() < 2 or argument_count() > 4) {
            throw compiler_exception{tok(), "expected 2 to 4 arguments"};
        }

        // 'read' and 'sector_read' fill the buffer, the second argument
        const statement& buffer{argument(1)};

        if (fills_buffer() and buffer.is_name_or_literal()) {
            const ident_info info{tc.make_ident_info(buffer)};

            if (info.is_var()) {
                assert_not_read_only(buffer.tok(), "read into",
                                     buffer.name_or_literal(), info);
            }
        }
    }

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const machine::builtin_function_registers registers{
            x.registers_for_builtin_function(builtin_function()),
        };

        const std::vector<operand> args{
            compile_array_arguments(tc, indent, registers.arguments),
        };

        if (is_sector_function()) {
            emit_sector_call(x, indent, args);
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

        emit_call(x, indent, result, args);

        get_unary_ops().compile(x, indent, tok(), result,
                                tc.checks().overflow_options());

        if (not dst.is_empty()) {
            x.copy_value(tok(), indent, dst.operand(), result);
        }

        if (not result_is_argument) {
            x.free_named_register(tok(), indent, result);
        }

        x.free_named_registers(tok(), indent, args);
    }

  private:
    // an address would bypass the bounds check, so only arrays are accepted
    auto assert_array_buffer(const toc& tc) const -> void {
        const statement& buffer{argument(1)};

        // the start has one spelling, the last argument
        if (buffer.is_array_element()) {
            throw compiler_exception{
                buffer.tok(),
                std::format("pass the array and the start as {} argument, "
                            "e.g. '{}'",
                            is_sector_function() ? "3rd" : "4th",
                            usage_text())};
        }

        if (buffer.is_name_or_literal()) {
            const ident_info info{tc.make_ident_info(buffer)};

            if (info.is_var() and info.is_array()) {
                return;
            }
        }

        throw compiler_exception{buffer.tok(), "argument 2 must be an array"};
    }

    // a sector is a whole number of elements and fits in the array
    auto assert_sector_buffer(toc& tc, const ident_info& buffer_info) const
        -> void {

        const machine& x{tc.machine()};

        const size_t sector_size_bytes{x.sector_size_bytes()};

        const size_t element_size_bytes{buffer_info.type_ref().size_bytes()};

        const statement& buffer{argument(1)};

        if (sector_size_bytes % element_size_bytes != 0) {
            throw compiler_exception{
                buffer.tok(),
                std::format("the element size {} B does not divide the "
                            "sector size {} B",
                            element_size_bytes, sector_size_bytes)};
        }

        const size_t buffer_size_bytes{
            buffer_info.array_len() * element_size_bytes,
        };

        if (buffer_size_bytes < sector_size_bytes) {
            throw compiler_exception{
                buffer.tok(),
                std::format("the array of {} B is smaller than a sector of "
                            "{} B",
                            buffer_size_bytes, sector_size_bytes)};
        }
    }

    // the target must have a card, the buffer is checked when it is compiled
    auto assert_sector_function_call(toc& tc) const -> void {
        const machine& x{tc.machine()};

        if (x.sector_size_bytes() == 0) {
            throw compiler_exception{tok(), "the target has no sd card"};
        }

        if (argument_count() < 2 or argument_count() > 3) {
            throw compiler_exception{tok(), "expected 2 or 3 arguments"};
        }
    }

    [[nodiscard]] auto builtin_function() const -> machine::builtin_function {
        return function_info().function;
    }

    // the count and start are in elements, the byte count is computed last
    [[nodiscard]] auto compile_array_arguments(
        toc& tc, const size_t indent,
        const std::span<const std::string_view> registers) const
        -> std::vector<operand> {

        assert_array_buffer(tc);

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

        argument(0).compile(tc, indent, destination::of_register(descriptor));

        const ident_info buffer_info{tc.make_ident_info(argument(1))};

        compile_count(tc, indent, count, buffer_info);

        operand start;

        if (has_start()) {
            start = compile_start(tc, indent, count, buffer_info);
        }

        // a start is checked with the start, a bare count by 'compile_lea'
        compile_buffer_address(tc, indent, buffer_reg, buffer_info,
                               has_count() and not has_start() ? count
                                                               : operand{});

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
                                const operand& buffer_reg,
                                const ident_info& buffer_info,
                                const operand& range) const -> void {

        machine& x{tc.machine()};

        const statement& buffer{argument(1)};

        std::vector<operand> lea_registers;

        const operand buffer_lea{
            buffer.compile_lea(tc, indent, buffer.tok(), lea_registers,
                               {
                                   .reg_count{range},
                                   .known{buffer_info.known()},
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

        if (has_count()) {
            argument(2).compile(tc, indent, destination::of_register(count));
            return;
        }

        machine& x{tc.machine()};

        size_t element_count{buffer_info.array_len()};

        if (is_sector_function()) {
            assert_sector_buffer(tc, buffer_info);

            element_count =
                x.sector_size_bytes() / buffer_info.type_ref().size_bytes();
        }

        x.copy_value(tok(), indent, count,
                     operand::imm(element_count, tc.builtins().default_type()));
    }

    // the start register stays allocated for the caller
    [[nodiscard]] auto compile_start(toc& tc, const size_t indent,
                                     const operand& count,
                                     const ident_info& buffer_info) const
        -> operand {

        machine& x{tc.machine()};

        const statement& start_arg{argument(argument_count() - 1)};

        const operand start{
            x.alloc_scratch_register(start_arg.tok(), indent,
                                     tc.builtins().default_type()),
        };

        start_arg.compile(tc, indent, destination::of_register(start));

        x.check_bounds(
            start_arg.tok(), indent,
            machine::bounds_range::range(start, count, buffer_info.array_len()),
            tc.checks().bounds_options());

        return start;
    }

    // the start moves the buffer address and the count becomes the byte count
    auto convert_to_bytes(toc& tc, const size_t indent,
                          const operand& buffer_reg, const operand& count,
                          const operand& start,
                          const operand& element_size_bytes) const -> void {

        machine& x{tc.machine()};

        if (has_start()) {
            x.multiply(tok(), indent, start, element_size_bytes);

            x.add_subtract(tok(), indent, machine::arithmetic_operator::add,
                           buffer_reg, start);

            x.free_scratch_register(tok(), indent, start);
        }

        x.multiply(tok(), indent, count, element_size_bytes);
    }

    auto emit_call(machine& x, const size_t indent, const operand& result,
                   const std::vector<operand>& args) const -> void {

        if (builtin_function() == machine::builtin_function::read) {
            x.read(tok(), indent, result, args.at(0), args.at(1), args.at(2));
            return;
        }

        x.write(tok(), indent, result, args.at(0), args.at(1), args.at(2));
    }

    auto emit_sector_call(machine& x, const size_t indent,
                          const std::vector<operand>& args) const -> void {

        if (builtin_function() == machine::builtin_function::sector_read) {
            x.sector_read(tok(), indent, args.at(0), args.at(1), args.at(2));
            return;
        }

        x.sector_write(tok(), indent, args.at(0), args.at(1), args.at(2));
    }

    [[nodiscard]] auto fills_buffer() const -> bool {
        return function_info().fills_buffer;
    }

    // the parser creates the statement only for the names of 'functions_'
    [[nodiscard]] auto function_info() const -> const io_function& {
        for (const io_function& info : functions_) {
            if (tok().is_text(info.name)) {
                return info;
            }
        }

        std::unreachable();
    }

    // 'read(fd, buf, count, start)': the count and the start are optional,
    // a sector function has no count
    [[nodiscard]] auto has_count() const -> bool {
        return not is_sector_function() and argument_count() >= 3;
    }

    [[nodiscard]] auto has_start() const -> bool {
        return argument_count() == (is_sector_function() ? 3 : 4);
    }

    [[nodiscard]] auto is_sector_function() const -> bool {
        return function_info().is_sector;
    }

    // the call with all its arguments
    [[nodiscard]] auto usage_text() const -> std::string {
        if (is_sector_function()) {
            return std::format("{}(sector, buf, start)", tok().text());
        }

        return std::format("{}(fd, buf, count, start)", tok().text());
    }
};
