#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "expr_bool.hpp"
#include "stmt_block.hpp"

class stmt_if_branch final : public statement {
    expr_bool bol_;
    stmt_block code_;

  public:
    stmt_if_branch(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()},
          bol_{tc, tz.next_whitespace_token(), tz}, code_{tc, tz} {

        set_type(tc.get_type_void());
    }

    stmt_if_branch() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        bol_.source_to(os);
        code_.source_to(os);
    }

    [[nodiscard]] auto
    compile_branch(toc& tc, const size_t indent,
                   const std::string_view jmp_to_if_false_label,
                   const std::string_view jmp_to_after_code_label) const
        -> std::optional<bool> {

        const std::string if_bgn_lbl{if_bgn_label(tc)};
        const std::string jmp_to_if_true_lbl{
            std::format("{}.code", if_bgn_lbl)};

        // the beginning of this branch

        machine& x{tc.machine()};

        x.label(indent, if_bgn_lbl);
        // compile the boolean expression
        const std::optional<bool> const_eval{bol_.compile(
            tc, indent, jmp_to_if_false_label, jmp_to_if_true_lbl, {})};

        if (const_eval == false) {
            return false;
        }
        // the label where to jump if evaluation of the condition is true
        x.label(indent, jmp_to_if_true_lbl);
        // the code of the branch
        code_.compile(tc, indent, ident_info::make_empty());
        if (const_eval == true) {
            return true;
        }
        // after the branch code executes, jump to the end of the
        // 'if ... else if ... else ...' block.
        // if the jump label is not provided, then there is no 'else' and this
        // is the last 'if' so just continue execution
        if (not jmp_to_after_code_label.empty()) {
            x.branch(indent, jmp_to_after_code_label);
        }

        return std::nullopt;
    }

    // Returns the label where the if branch begins evaluating the boolean
    // expression
    [[nodiscard]] auto if_bgn_label(const toc& tc) const -> std::string {
        return tc.create_unique_label(tok(), "if");
    }

    // the condition is read before the branch code runs
    auto trace_assignment(assignment_flow& flow) const -> void override {
        bol_.assert_var_not_used(flow.var, flow.assigned);
        code_.trace_assignment(flow);
    }
};
