#pragma once

#include <cassert>
#include <cstddef>

#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

class stmt_builtin_exit final : public stmt_call {
  public:
    stmt_builtin_exit(toc& tc, const token tk, tokenizer& tz)
        : stmt_call{builtin_call_tag{},       tc, {}, tk, tz,
                    tc.builtins().void_type()} {

        if (argument_count() != 1) {
            throw compile_error{tok(), "expected 1 argument"};
        }
    }

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const machine::builtin_function_registers registers{
            x.registers_for_builtin_function(machine::builtin_function::exit),
        };

        // note: the exit code is the only argument
        assert(registers.arguments.size() == 1);

        const operand exit_code{
            x.alloc_named_register(tok(), indent, registers.arguments.front(),
                                   tc.builtins().default_type()),
        };

        argument(0).compile(tc, indent, destination::of_register(exit_code));

        x.exit(tok(), indent, exit_code);

        x.free_named_register(tok(), indent, exit_code);
    }

    // the program ends here, so this path never returns and cannot leave the
    // result unset
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow);
        flow.end_path();
    }
};
