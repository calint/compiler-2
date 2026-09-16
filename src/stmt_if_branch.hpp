#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "expr_bool_ops_list.hpp"
#include "stmt_block.hpp"

class stmt_if_branch final : public statement {
    expr_bool_ops_list bol_;
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
    compile_branch(toc& tc, std::ostream& os, const size_t indent,
                   const std::string_view jmp_to_if_false_label,
                   const std::string_view jmp_to_after_code_label) const
        -> std::optional<bool> {

        const std::string if_bgn_lbl{if_bgn_label(tc)};
        const std::string jmp_to_if_true_lbl{
            std::format("{}_code", if_bgn_lbl)};
        // the beginning of this branch
        x86::label(tc, os, indent, if_bgn_lbl);
        // compile the boolean ops list
        if (const std::optional<bool> const_eval{
                bol_.compile(tc, os, indent, jmp_to_if_false_label,
                             jmp_to_if_true_lbl, false, "")};
            const_eval) {

            // yes, was the constant evaluation result true?
            if (*const_eval) {
                // yes, this branch code will execute

                x86::label(tc, os, indent, jmp_to_if_true_lbl);
                // note: label is necessary because of a 'jmp' that gets
                //       optimized away
                code_.compile(tc, os, indent, toc::make_ident_info_empty());
            }
            return *const_eval;
        }
        // the label where to jump if evaluation of the condition is true
        x86::label(tc, os, indent, jmp_to_if_true_lbl);
        // the code of the branch
        code_.compile(tc, os, indent, toc::make_ident_info_empty());
        // after the code of the branch is executed, jump to the end of the 'if
        // ... else if ... else ...' block.
        // if the jump label is not provided, then there is no 'else' and this
        // is the last 'if' so just continue execution
        if (not jmp_to_after_code_label.empty()) {
            x86::jmp(tc, os, indent, jmp_to_after_code_label);
        }
        return std::nullopt;
    }

    // returns the label where the if branch begins evaluating the boolean
    // expression
    [[nodiscard]] auto if_bgn_label(const toc& tc) const -> std::string {
        // construct a unique label considering in-lined functions
        const std::string_view call_path{tc.get_call_path()};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        return call_path.empty() ? std::format("if_{}", src_loc)
                                 : std::format("if_{}_{}", src_loc, call_path);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {
        bol_.assert_var_not_used(var);
    }
};
