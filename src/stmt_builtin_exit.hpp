#pragma once

#include <cassert>
#include <cstddef>
#include <format>

#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

// 'exit(code)' ends the program with 'code', the path after it is not
// reached
class stmt_builtin_exit final : public statement {
    builtin_arguments args_;

  public:
    stmt_builtin_exit(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, args_{tc, tz} {

        if (args_.count() != 1) {
            throw compile_error{tok(), std::format("{} takes {}",
                                                   in_quotes(tok().text()),
                                                   counted(1, "argument"))};
        }
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        args_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const machine::target_call_registers registers{
            x.registers_for_target_call(machine::target_call::exit),
        };

        // note: the exit code is the only argument
        assert(registers.arguments.size() == 1);

        const operand exit_code{
            x.alloc_named_register(tok(), indent, registers.arguments.front(),
                                   tc.builtins().default_type()),
        };

        args_.at(0).compile(tc, indent, destination::of_register(exit_code));

        x.exit(tok(), indent, exit_code);

        x.free_named_register(tok(), indent, exit_code);

        tc.end_path();
    }

    // the program ends here, so this path never returns and cannot leave the
    // result unset
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow);
        flow.end_path();
    }

    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        args_.visit_call_effects(tc, visitor);
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        args_.visit_reads(var, reader);
    }
};
