#pragma once

#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <string_view>

#include "expr_bool.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "stmt_block.hpp"
#include "ub_assigned.hpp"

// one condition of an 'if' or 'else if' and the block it guards
class stmt_if_branch final : public statement {
    expr_bool condition_;
    stmt_block code_;

  public:
    // e.g. 'a == b {x = 1}' of 'if a == b {x = 1} else {y = 2}', the condition
    // and the block that runs when it holds
    stmt_if_branch(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()},
          condition_{tc, tz.next_whitespace_token(), tz} {

        set_type(tc.builtins().void_type());
        condition_.assert_boolean_operands();
        code_ = {tc, tz};
    }

    stmt_if_branch() = default;

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
        condition_.assert_var_not_used(flow);
        code_.trace_assignment(flow);
    }

    //
    // class methods
    //

    // the label where the if branch begins evaluating the boolean
    // expression
    [[nodiscard]] auto begin_label(const toc& tc) const -> std::string {
        return tc.create_unique_label(tok(), "if");
    }

    [[nodiscard]] auto
    compile_branch(toc& tc, const size_t indent,
                   const std::string_view jmp_to_if_false_label,
                   const std::string_view jmp_to_after_code_label) const
        -> condition_result {

        const std::string if_label{begin_label(tc)};

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

        if (const_eval == condition_result::always_false) {
            return const_eval;
        }

        // an 'or' condition branches here as soon as one operand is true
        x.label(indent, jmp_to_if_true_label);
        code_.compile(tc, indent, destination{});

        if (const_eval == condition_result::always_true) {
            return const_eval;
        }

        // the last branch without an 'else' falls through to the code after
        // the 'if'
        if (not jmp_to_after_code_label.empty()) {
            x.branch(indent, jmp_to_after_code_label);
        }

        return condition_result::unknown;
    }
};
