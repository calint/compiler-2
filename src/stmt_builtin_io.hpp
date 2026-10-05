#pragma once

#include <algorithm>
#include <format>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "decouple.hpp"
#include "stmt_call.hpp"

class stmt_builtin_io final : public stmt_call {
  public:
    stmt_builtin_io(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
        : stmt_call{tc, std::move(uops), tk, tz.is_next_char_token('('), tz} {

        if (argument_count() < 2 or argument_count() > 4) {
            throw compiler_exception{tok(), "expected 2 to 4 arguments"};
        }

        // 'read' fills the buffer, the second argument
        const statement& buffer{argument(1)};

        if (tok().is_text("read") and buffer.is_identifier()) {
            const ident_info info{tc.make_ident_info(buffer)};

            if (info.is_var()) {
                assert_not_read_only(buffer.tok(), "read into",
                                     buffer.identifier(), info);
            }
        }
    }

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const machine::builtin_function function{
            tok().is_text("read") ? machine::builtin_function::read
                                  : machine::builtin_function::write,
        };

        const machine::builtin_function_registers registers{
            x.registers_for_builtin_function(function),
        };

        const std::vector<operand> args{
            compile_array_arguments(tc, indent, registers.arguments),
        };

        const bool result_is_argument{
            std::ranges::contains(registers.arguments, registers.result),
        };

        const operand result{
            result_is_argument
                ? x.make_register_operand(registers.result,
                                          tc.get_type_default())
                : x.alloc_named_register(tok(), indent, registers.result,
                                         tc.get_type_default()),
        };

        emit_call(x, indent, result, args);

        get_unary_ops().compile(tc, indent, tok(), result);

        if (not dst_info.is_empty()) {
            x.copy_value(tok(), indent, dst_info.operand, result);
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

        // the start has one spelling, the 4th argument
        if (buffer.is_array_element()) {
            throw compiler_exception{
                buffer.tok(),
                std::format("pass the array and the start as 4th argument, "
                            "e.g. '{}(fd, buf, count, start)'",
                            tok().text())};
        }

        if (buffer.is_identifier()) {
            const ident_info info{tc.make_ident_info(buffer)};

            if (info.is_var() and info.is_array) {
                return;
            }
        }

        throw compiler_exception{buffer.tok(), "argument 2 must be an array"};
    }

    // the count and start are in elements, the byte count is computed last
    [[nodiscard]] auto compile_array_arguments(
        toc& tc, const size_t indent,
        const std::span<const std::string_view> registers) const
        -> std::vector<operand> {

        assert_array_buffer(tc);

        machine& x{tc.machine()};

        const type& type_default{tc.get_type_default()};

        std::vector<operand> args;
        args.reserve(registers.size());
        for (const std::string_view name : registers) {
            args.push_back(
                x.alloc_named_register(tok(), indent, name, type_default));
        }

        const operand& descriptor{args.at(0)};
        const operand& buffer_reg{args.at(1)};
        const operand& count{args.at(2)};

        argument(0).compile(tc, indent,
                            toc::make_ident_info_from_register(descriptor));

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
            operand::imm(std::format("{}", buffer_info.type_ref().size_bytes()),
                         type_default),
        };

        if (has_start()) {
            x.multiply(tok(), indent, start, element_size_bytes);

            x.add_subtract(tok(), indent, machine::arithmetic_operator::add,
                           buffer_reg, start);

            x.free_scratch_register(tok(), indent, start);
        }

        x.multiply(tok(), indent, count, element_size_bytes);

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
            buffer.compile_lea(tc, indent, buffer.tok(), lea_registers, range,
                               buffer_info.lea_path, {}),
        };

        x.address_of(tok(), indent, buffer_reg, buffer_lea);

        x.free_scratch_registers(tok(), indent, lea_registers);
    }

    // without a count the whole array is transferred
    auto compile_count(toc& tc, const size_t indent, const operand& count,
                       const ident_info& buffer_info) const -> void {

        if (has_count()) {
            argument(2).compile(tc, indent,
                                toc::make_ident_info_from_register(count));

            return;
        }

        machine& x{tc.machine()};

        x.copy_value(tok(), indent, count,
                     operand::imm(std::format("{}", buffer_info.array_len),
                                  tc.get_type_default()));
    }

    // the start register stays allocated for the caller
    [[nodiscard]] auto compile_start(toc& tc, const size_t indent,
                                     const operand& count,
                                     const ident_info& buffer_info) const
        -> operand {

        machine& x{tc.machine()};

        const statement& start_arg{argument(3)};

        const operand start{
            x.alloc_scratch_register(start_arg.tok(), indent,
                                     tc.get_type_default()),
        };

        start_arg.compile(tc, indent,
                          toc::make_ident_info_from_register(start));

        x.check_bounds(start_arg.tok(), indent, start, buffer_info.array_len,
                       true, count, tc.bounds_check_options());

        return start;
    }

    auto emit_call(machine& x, const size_t indent, const operand& result,
                   const std::vector<operand>& args) const -> void {

        if (tok().is_text("read")) {
            x.read(tok(), indent, result, args.at(0), args.at(1), args.at(2));
            return;
        }

        x.write(tok(), indent, result, args.at(0), args.at(1), args.at(2));
    }

    // 'read(fd, buf, count, start)': the count and the start are optional
    [[nodiscard]] auto has_count() const -> bool {
        return argument_count() >= 3;
    }

    [[nodiscard]] auto has_start() const -> bool {
        return argument_count() == 4;
    }
};
