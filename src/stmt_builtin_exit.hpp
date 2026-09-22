#pragma once

#include <vector>

#include "decouple.hpp"
#include "stmt_call.hpp"

class stmt_builtin_exit final : public stmt_call {
  public:
    stmt_builtin_exit(toc& tc, const token tk, tokenizer& tz)
        : stmt_call{tc, {}, tk, tz.is_next_char_token('('), tz} {
        if (argument_count() != 1) {
            throw compiler_exception{tok(), "expected 1 argument"};
        }
        set_type(tc.get_type_void());
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {
        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));
        const machine::builtin_registers registers{
            x.registers_for_builtin(machine::builtin_function::exit)};

        const std::vector<operand> args{
            compile_builtin_arguments(tc, indent, registers.arguments)};

        x.exit(tok(), indent, args.at(0));
        x.free_named_registers(tok(), indent, args);
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
