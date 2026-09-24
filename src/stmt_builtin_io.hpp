#pragma once

#include <algorithm>
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
            compile_builtin_arguments(tc, indent, registers.arguments)};

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
};
