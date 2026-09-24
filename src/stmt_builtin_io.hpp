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
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {
        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));
        const machine::builtin_function function{
            tok().is_text("read") ? machine::builtin_function::read
                                  : machine::builtin_function::write};

        const machine::builtin_function_registers registers{
            x.registers_for_builtin_function(function)};
        const std::vector<operand> args{
            compile_array_arguments(tc, indent, registers.arguments)};

        const bool result_is_argument{
            std::ranges::contains(registers.arguments, registers.result)};

        const operand result{
            result_is_argument
                ? x.make_register_operand(registers.result,
                                          tc.get_type_default())
                : x.alloc_named_register(tok(), indent, registers.result,
                                         tc.get_type_default())};

        if (tok().is_text("read")) {
            x.read(tok(), indent, result, args.at(0), args.at(1), args.at(2));
        } else {
            x.write(tok(), indent, result, args.at(0), args.at(1), args.at(2));
        }
        get_unary_ops().compile(tc, indent, result);
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

        const statement& buffer{argument(1)};
        const ident_info buffer_info{tc.make_ident_info(buffer)};

        const bool has_count{argument_count() >= 3};
        const bool has_start{argument_count() == 4};

        if (has_count) {
            argument(2).compile(tc, indent,
                                toc::make_ident_info_from_register(count));
        }

        // without a count the whole array is transferred
        if (not has_count) {
            x.copy_value(tok(), indent, count,
                         operand::imm(std::format("{}", buffer_info.array_len),
                                      type_default));
        }

        // a start is checked here, a bare count is checked by 'compile_lea'
        operand start;
        if (has_start) {
            const statement& start_arg{argument(3)};
            start =
                x.alloc_scratch_register(start_arg.tok(), indent, type_default);
            start_arg.compile(tc, indent,
                              toc::make_ident_info_from_register(start));
            x.check_bounds(start_arg.tok(), indent, start,
                           buffer_info.array_len, true, count,
                           {
                               .upper{tc.is_bounds_check_upper()},
                               .lower{tc.is_bounds_check_lower()},
                               .with_line{tc.is_bounds_check_with_line()},
                           });
        }

        const operand range{has_count and not has_start ? count : operand{}};

        std::vector<operand> lea_registers;
        const operand buffer_lea{buffer.compile_lea(tc, indent, buffer.tok(),
                                                    lea_registers, range,
                                                    buffer_info.lea_path, {})};
        x.address_of(tok(), indent, buffer_reg, buffer_lea);
        x.free_scratch_registers(tok(), indent, lea_registers);

        const operand element_size_bytes{
            operand::imm(std::format("{}", buffer_info.type_ref().size_bytes()),
                         type_default)};

        if (has_start) {
            x.multiply(tok(), indent, start, element_size_bytes);
            x.add_subtract(tok(), indent, '+', buffer_reg, start);
            x.free_scratch_register(tok(), indent, start);
        }

        x.multiply(tok(), indent, count, element_size_bytes);

        return args;
    }
};
