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
        if (argument_count() != 3) {
            throw compiler_exception{tok(), "expected 3 arguments"};
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
            is_array_buffer(tc)
                ? compile_array_arguments(tc, indent, registers.arguments)
                : compile_builtin_arguments(tc, indent, registers.arguments)};

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
    // 'buf' or 'buf[start]' names elements, any other value is an address
    [[nodiscard]] auto is_array_buffer(const toc& tc) const -> bool {
        const statement& buffer{argument(1)};

        if (not buffer.is_identifier()) {
            return false;
        }

        if (buffer.is_array_element()) {
            return true;
        }

        const ident_info info{tc.make_ident_info(buffer)};

        return info.is_var() and info.is_array;
    }

    // the count is in elements and is compiled before the address so the
    // address computation can check that 'start + count' fits the array
    [[nodiscard]] auto compile_array_arguments(
        toc& tc, const size_t indent,
        const std::span<const std::string_view> registers) const
        -> std::vector<operand> {

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
        argument(2).compile(tc, indent,
                            toc::make_ident_info_from_register(count));

        const statement& buffer{argument(1)};
        const ident_info buffer_info{tc.make_ident_info(buffer)};

        std::vector<operand> lea_registers;
        const operand buffer_lea{buffer.compile_lea(tc, indent, buffer.tok(),
                                                    lea_registers, count,
                                                    buffer_info.lea_path, {})};
        x.address_of(tok(), indent, buffer_reg, buffer_lea);
        x.free_scratch_registers(tok(), indent, lea_registers);

        const size_t element_size_bytes{buffer_info.type_ref().size_bytes()};
        x.multiply(
            tok(), indent, count,
            operand::imm(std::format("{}", element_size_bytes), type_default));

        return args;
    }
};
