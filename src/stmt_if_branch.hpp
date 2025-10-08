#pragma once
// reviewed: 2025-09-28

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

    ~stmt_if_branch() override = default;

    stmt_if_branch() = default;
    stmt_if_branch(const stmt_if_branch&) = default;
    stmt_if_branch(stmt_if_branch&&) = default;
    auto operator=(const stmt_if_branch&) -> stmt_if_branch& = default;
    auto operator=(stmt_if_branch&&) -> stmt_if_branch& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        bol_.source_to(os);
        code_.source_to(os);
    }

    // returns the label where the if branch begins evaluating the boolean
    // expression
    [[nodiscard]] auto if_bgn_label(const toc& tc) const -> std::string {
        // construct a unique label considering in-lined functions
        const std::string_view call_path{tc.get_call_path(tok())};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        return call_path.empty() ? std::format("if_{}", src_loc)
                                 : std::format("if_{}_{}", src_loc, call_path);
    }

    [[noreturn]] void
    compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
            [[maybe_unused]] size_t indent,
            [[maybe_unused]] std::string_view dst) const override {

        throw panic_exception("unexpected code path");
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 std::string_view jmp_to_if_false_label,
                 std::string_view jmp_to_after_code_label) const
        -> std::optional<bool> {

        const std::string if_bgn_lbl{if_bgn_label(tc)};
        const std::string jmp_to_if_true_lbl{
            std::format("{}_code", if_bgn_lbl)};
        // the beginning of this branch
        toc::asm_label(tok(), os, indent, if_bgn_lbl);
        // compile boolean ops list
        std::optional<bool> const_eval{bol_.compile(
            tc, os, indent, jmp_to_if_false_label, jmp_to_if_true_lbl, false)};
        // did boolean evaluation result in a constant?
        if (const_eval) {
            // yes, was the constant evaluation result true?
            if (*const_eval) {
                // yes, this branch code will execute
                code_.compile(tc, os, indent);
            }
            return *const_eval;
        }
        // the label where to jump if evaluation of condition is true
        toc::asm_label(tok(), os, indent, jmp_to_if_true_lbl);
        // the code of the branch
        code_.compile(tc, os, indent);
        // after the code of the branch is executed jump to the end of the 'if
        // ... else if ... else ...' block.
        // if jump label not provided then there is no 'else' and this is the
        // last 'if' so just continue execution
        if (not jmp_to_after_code_label.empty()) {
            toc::asm_jmp(tok(), os, indent, jmp_to_after_code_label);
        }
        return std::nullopt;
    }
};
