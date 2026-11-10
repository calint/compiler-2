#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>

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
    // the operating system keeps the low 8 bits of the code
    static constexpr int64_t max_code_{255};

    builtin_arguments args_;

  public:
    stmt_builtin_exit(toc& tc, const token& src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, args_{tc, tz, src_loc_tk, {}, 1, 1} {

        assert_constant_code_fits(tc);
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

        comment_source(x, indent);

        const operand exit_code{
            x.alloc_named_register(tok(), indent, x.exit_code_register(),
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
        assert_result_not_read(flow);
        flow.end_path();
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        args_.visit_operands(visitor, std::nullopt);
    }

  private:
    // a constant code outside the low 8 bits would end with another code,
    // e.g. 'exit(-1)' with 255, that of a 'bounds' panic; a run-time code
    // keeps its low 8 bits (README chapter 8)
    auto assert_constant_code_fits(const toc& tc) const -> void {
        const expr_any& code_arg{args_.at(0)};

        const std::optional<folded_value> code{
            code_arg.fold(tc, tc.use_at(tc.builtins().default_type())),
        };

        if (not code or (code->value >= 0 and code->value <= max_code_)) {
            return;
        }

        throw compile_error{
            code_arg.tok(),
            std::format("exit code {} is outside 0 to {}, the operating "
                        "system keeps only the low 8 bits",
                        code->value, max_code_)};
    }
};
