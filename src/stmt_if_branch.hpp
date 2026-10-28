#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "compile_error.hpp"
#include "expr_bool.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "stmt_block.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "ub_assigned.hpp"

// what a branch found of its condition: its value where known and whether
// the condition returns, where the next branch and the 'else' start, e.g.
// not after 'if stop() == 1' with 'stop' an inlined 'exit'
struct branch_condition {
    condition_result known;
    bool returns{};
};

// one condition of an 'if' or 'else if' and the block it guards
class stmt_if_branch final : public statement {
    expr_bool condition_;
    // the value of the condition known where the 'if' is written, e.g. from
    // literals and constants, 'unknown' when it depends on the run or on a
    // call
    condition_result constant_at_parse_;
    stmt_block code_;

  public:
    // e.g. 'a == b {x = 1}' of 'if a == b {x = 1} else {y = 2}', the condition
    // and the block that runs when it holds
    stmt_if_branch(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()},
          condition_{parse_condition(tc, tz)},
          constant_at_parse_{constant_of(tc, condition_)}, code_{tc, tz} {}

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        condition_.source_to(os);
        code_.source_to(os);
    }

    // the condition is read before the branch code runs
    auto trace_assignment(assignment_flow& flow) const -> void override {
        condition_.assert_result_not_read(flow);
        code_.trace_assignment(flow);
    }

    //
    // class methods
    //

    // the condition at 'if_label' and the code; the jump from the end of the
    // code past the other branches is the caller's
    [[nodiscard]] auto
    compile_branch(toc& tc, const size_t indent,
                   const std::string_view if_label,
                   const std::string_view jmp_to_if_false_label) const
        -> branch_condition {

        const std::string jmp_to_if_true_label{
            std::format("{}.code", if_label),
        };

        machine& x{tc.machine()};

        x.label(indent, if_label);

        const condition_result const_eval{
            condition_.compile(tc, indent,
                               {
                                   .if_false{jmp_to_if_false_label},
                                   .if_true{jmp_to_if_true_label},
                               },
                               {}),
        };

        const bool returns{tc.is_reachable()};

        // note: code that never runs holds the compile rules too, see
        //       'toc::check_skipped'

        if (const_eval == condition_result::always_false) {
            tc.check_skipped(
                [&] -> void { code_.compile(tc, indent, destination{}); });

            return {
                .known{const_eval},
                .returns{returns},
            };
        }

        // an 'or' condition branches here as soon as one operand is true
        x.label(indent, jmp_to_if_true_label);
        code_.compile(tc, indent, destination{});

        return {
            .known{const_eval},
            .returns{returns},
        };
    }

    [[nodiscard]] auto constant_at_parse() const -> condition_result {
        return constant_at_parse_;
    }

    //
    // statics
    //

    // an empty token where the condition starts, the branch's token holds the
    // whitespace before it; a '{' there means the condition is missing, e.g.
    // 'if {x = 1}'
    [[nodiscard]] static auto condition_start(tokenizer& tz) -> token {
        const token ws_tk{tz.next_whitespace_token()};

        if (tz.peek_char() == '{') {
            throw compile_error{tz, "expected a condition before '{'"};
        }

        return ws_tk;
    }

    [[nodiscard]] static auto constant_of(const toc& tc,
                                          const expr_bool& condition)
        -> condition_result {

        const std::optional<bool> constant{condition.constant_value(tc)};

        if (not constant) {
            return condition_result::unknown;
        }

        return known_result(*constant);
    }

    [[nodiscard]] static auto parse_condition(toc& tc, tokenizer& tz)
        -> expr_bool {

        expr_bool condition{tc, condition_start(tz), tz};

        condition.assert_boolean_operands();

        return condition;
    }
};
