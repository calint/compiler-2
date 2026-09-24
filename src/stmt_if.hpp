#pragma once
// reviewed: 2025-09-28

#include <ranges>

#include "decouple.hpp"
#include "stmt_block.hpp"
#include "stmt_if_branch.hpp"

class stmt_if final : public statement {
    std::vector<stmt_if_branch> branches_;
    std::vector<token> else_if_tokens_;
    stmt_block else_code_;

  public:
    stmt_if(toc& tc, const token tk, tokenizer& tz) : statement{tk} {
        set_type(tc.get_type_void());
        // e.g. if a == b {x = 1} else if c == d {y = 2} else {z = 3}, broken
        // down into branches 'a == b {x = 1}', 'c == d {y = 2}', ending with
        // an optional 'else' block

        // note: 'if' token has been read

        while (true) {
            // read branch e.g. a == b {x = 1}
            branches_.emplace_back(tc, tz);

            // check if it is an 'else if' or 'else' or a new statement
            const token tkn{tz.next_token()};
            if (not tkn.is_text("else")) {
                // not 'else', push the token back in stream and exit
                tz.put_back_token(tkn);

                return;
            }
            // is 'else'
            // check if it is 'else if'
            const token tkn2{tz.next_token()};
            if (not tkn2.is_text("if")) {
                // not 'else if', push token back in stream
                tz.put_back_token(tkn2);
                // 'else' branch
                // save tokens to be able to reproduce the source
                else_if_tokens_.emplace_back(tkn);
                // read the 'else' code
                else_code_ = {tc, tz};

                return;
            }
            // 'else if': continue reading if branches
            // save tokens to be able to reproduce the source
            else_if_tokens_.emplace_back(tkn);
            else_if_tokens_.emplace_back(tkn2);
        }
    }

    stmt_if() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        // output first branch
        const stmt_if_branch& branch{branches_[0]};
        branch.source_to(os);
        // output the remaining 'else if' branches
        const auto else_if_branches{branches_ | std::views::drop(1)};
        const auto token_pairs{else_if_tokens_ | std::views::chunk(2)};
        for (const auto [b, t] :
             std::views::zip(else_if_branches, token_pairs)) {

            // 'else if' tokens as read from source
            t[0].source_to(os);
            t[1].source_to(os);
            b.source_to(os);
        }
        // the 'else' code
        if (not else_code_.is_empty()) {
            else_if_tokens_.back().source_to(os);
            else_code_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        // make unique labels considering inlined functions
        const std::string_view call_path{tc.get_call_path()};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string label_after_if{
            call_path.empty()
                ? std::format("if_{}_end", src_loc)
                : std::format("if_{}_{}_end", src_loc, call_path)};

        const std::string label_else_branch{stmt_if::create_label_else_branch(
            else_code_, call_path, src_loc, label_after_if)};

        const size_t branch_count{branches_.size()};

        bool branch_evaluated_to_true{};
        for (size_t branch_index{}; branch_index < branch_count;
             ++branch_index) {
            const stmt_if_branch& if_branch{branches_[branch_index]};
            std::string jmp_if_false{label_else_branch};
            std::string jmp_if_done{label_after_if};
            if (branch_index < branch_count - 1) {
                // if branch is false jump to next if
                jmp_if_false = branches_[branch_index + 1].if_bgn_label(tc);
            } else if (else_code_.is_empty()) {
                // if the last branch and no 'else', then no need to jump to
                // 'after_if' after the code of the branch has been executed.
                // just continue
                jmp_if_done = "";
            }
            // compile the condition which might return that the condition was a
            // constant evaluation
            if (const std::optional<bool> const_eval{if_branch.compile_branch(
                    tc, indent, jmp_if_false, jmp_if_done)};
                const_eval.value_or(false)) {

                branch_evaluated_to_true = true;
                break;
            }
        }
        // if it wasn't a constant evaluation that was true, generate the else
        // code

        machine& x{tc.machine()};

        if (not branch_evaluated_to_true and not else_code_.is_empty()) {
            x.label(indent, label_else_branch);
            else_code_.compile(tc, indent, dst_info);
        }

        x.label(indent, label_after_if);
    }

    [[nodiscard]] auto else_block() const -> const stmt_block& {
        return else_code_;
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        for (const stmt_if_branch& e : branches_) {
            e.assert_var_not_used(var);
        }
        else_code_.assert_var_not_used(var);
    }

    [[nodiscard]] auto is_var_set(const std::string_view var) const
        -> bool override {

        // without 'else' every branch may be skipped
        if (else_code_.is_empty()) {
            return false;
        }

        for (const stmt_if_branch& e : branches_) {
            if (not e.is_var_set(var)) {
                return false;
            }
        }

        return else_code_.is_var_set(var);
    }

    [[nodiscard]] auto may_return_unset(const std::string_view var) const
        -> bool override {

        return any_path_may_exit_unset(var, &statement::may_return_unset);
    }

    [[nodiscard]] auto may_break_unset(const std::string_view var) const
        -> bool override {

        return any_path_may_exit_unset(var, &statement::may_break_unset);
    }

  private:
    [[nodiscard]] auto
    any_path_may_exit_unset(const std::string_view var,
                            bool (statement::*may_exit)(std::string_view)
                                const) const -> bool {

        for (const stmt_if_branch& e : branches_) {
            if ((e.*may_exit)(var)) {
                return true;
            }
        }

        return (else_code_.*may_exit)(var);
    }

    [[nodiscard]] static auto create_label_else_branch(
        const stmt_block& else_code, const std::string_view call_path,
        const std::string_view src_loc, const std::string_view label_after_if)
        -> std::string {

        if (else_code.is_empty()) {
            return std::string{label_after_if};
        }

        return (call_path.empty()
                    ? std::format("if_else_{}", src_loc)
                    : std::format("if_else_{}_{}", src_loc, call_path));
    }
};
