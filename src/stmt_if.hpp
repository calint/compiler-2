#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "ident_info.hpp"
#include "machine.hpp"
#include "stmt_block.hpp"
#include "stmt_if_branch.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

// 'if', 'else if' and 'else': the branches are tried in order and the first
// whose condition holds runs; a branch that is constant true ends the chain
// and one that is constant false is not compiled
class stmt_if final : public statement {
    // e.g. 'else if' of 'else if c == d {y = 2}'
    struct else_if_tokens {
        token else_tk;
        token if_tk;
    };

    // e.g. 'else {z = 3}'
    struct else_part {
        token else_tk;
        stmt_block code;
    };

    std::vector<stmt_if_branch> branches_;
    // one for each branch after the first, kept to reproduce the source
    std::vector<else_if_tokens> else_if_tokens_;
    // none without an 'else'
    std::optional<else_part> else_;

  public:
    stmt_if(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk} {

        // 'if a == b {x = 1} else if c == d {y = 2} else {z = 3}' breaks down
        // into the branches 'a == b {x = 1}' and 'c == d {y = 2}' and an
        // optional 'else' block, the 'if' token has been read
        branches_.emplace_back(tc, tz);

        while (parse_else(tc, tz)) {
            branches_.emplace_back(tc, tz);
        }
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        const stmt_if_branch& branch{branches_.at(0)};
        branch.source_to(os);
        const auto else_if_branches{branches_ | std::views::drop(1)};
        for (const auto [b, t] :
             std::views::zip(else_if_branches, else_if_tokens_)) {

            t.else_tk.source_to(os);
            t.if_tk.source_to(os);
            b.source_to(os);
        }

        // the 'else' code, also when it is empty
        if (else_) {
            else_->else_tk.source_to(os);
            else_->code.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        // the 'if' keyword locates the labels shared by all branches
        const std::string if_label{tc.create_unique_label(tok(), "if")};
        const std::string label_after_if{toc::end_label(if_label)};

        const std::string label_else_branch{
            create_label_else_branch(has_else_code(), if_label, label_after_if),
        };

        toc::path_merge paths{tc};
        bool reaches_else{true};

        for (size_t i{}; i < branches_.size(); ++i) {
            paths.begin_path();

            const condition_result known{
                compile_branch_at(tc, indent, i, if_label, label_else_branch,
                                  label_after_if),
            };

            // note: the compile knows at least what the parse knew, the
            //       paths of the definite assignment follow the parse

            assert(not is_known(branches_.at(i).constant_at_parse()) or
                   branches_.at(i).constant_at_parse() == known);

            if (runs(known)) {
                paths.join_path();
            }

            if (ends_chain(known)) {
                reaches_else = false;

                check_skipped_after(tc, indent, i, if_label, label_else_branch,
                                    label_after_if);

                break;
            }
        }

        machine& x{tc.machine()};

        // an empty 'else' is the path that skips every branch
        if (reaches_else) {
            paths.begin_path();

            // note: spelled out because clang-tidy does not see that
            // 'has_else_code'
            //       checks 'else_'

            if (else_ and not else_->code.is_empty()) {
                x.label(indent, label_else_branch);
                else_->code.compile(tc, indent, destination{});
            }

            paths.join_path();
        }

        x.label(indent, label_after_if);
    }

    // the paths that the conditions known where the 'if' is written leave,
    // e.g. 'if 1 == 1 { res = 2 }' sets 'res' on every path
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assignment_flow::path_merge paths{flow};

        for (const stmt_if_branch& branch : branches_) {
            const condition_result known{branch.constant_at_parse()};

            if (runs(known)) {
                paths.trace(branch);
            }

            if (ends_chain(known)) {
                return;
            }
        }

        // an empty 'else' is the path that skips every branch
        if (else_) {
            paths.trace(else_->code);
        } else {
            paths.trace_empty();
        }
    }

  private:
    // the branches after 'branch_index', whose condition is true, and the
    // 'else' never run; they hold the compile rules too, see
    // 'toc::check_skipped'
    auto check_skipped_after(toc& tc, const size_t indent,
                             const size_t branch_index,
                             const std::string_view if_label,
                             const std::string_view label_else_branch,
                             const std::string_view label_after_if) const
        -> void {

        tc.check_skipped([&] -> void {
            for (size_t i{branch_index + 1}; i < branches_.size(); ++i) {
                std::ignore = compile_branch_at(
                    tc, indent, i, if_label, label_else_branch, label_after_if);
            }

            if (else_) {
                else_->code.compile(tc, indent, destination{});
            }
        });
    }

    // the branch at 'branch_index', what it found of its condition
    [[nodiscard]] auto
    compile_branch_at(toc& tc, const size_t indent, const size_t branch_index,
                      const std::string_view if_label,
                      const std::string_view label_else_branch,
                      const std::string_view label_after_if) const
        -> condition_result {

        const size_t branch_count{branches_.size()};

        const stmt_if_branch& if_branch{branches_.at(branch_index)};
        const bool is_last_branch{branch_index == branch_count - 1};

        // a false condition continues at the next branch or the 'else'
        const std::string jmp_if_false{
            is_last_branch ? std::string{label_else_branch}
                           : branch_label(if_label, branch_index + 1),
        };

        // a condition that is constant true ends the chain at this branch
        const condition_result const_eval{
            if_branch.compile_branch(
                tc, indent, branch_label(if_label, branch_index), jmp_if_false),
        };

        // the last branch without an 'else' continues after the 'if'
        // without a jump
        const bool falls_through{is_last_branch and not has_else_code()};

        if (const_eval == condition_result::unknown and not falls_through) {
            machine& x{tc.machine()};

            x.branch(indent, label_after_if);
        }

        return const_eval;
    }

    // an empty 'else' compiles as none
    [[nodiscard]] auto has_else_code() const -> bool {
        return else_ and not else_->code.is_empty();
    }

    // reads what follows a branch: 'else if' continues the chain, 'else' ends
    // it with the else code, anything else is a new statement
    auto parse_else(toc& tc, tokenizer& tz) -> bool {
        const token else_tk{tz.next_token()};

        if (not else_tk.is_text("else")) {
            tz.put_back_token(else_tk);
            return false;
        }

        const token if_tk{tz.next_token()};

        if (if_tk.is_text("if")) {
            else_if_tokens_.push_back({
                .else_tk{else_tk},
                .if_tk{if_tk},
            });

            return true;
        }

        tz.put_back_token(if_tk);

        else_ = {
            .else_tk{else_tk},
            .code{tc, tz},
        };

        return false;
    }

    //
    // statics
    //

    // the label of a branch of the 'if' at 'if_label', the first branch is at
    // it, e.g. 'if.9.9' and the 'else if' after it 'if.9.9.1'
    [[nodiscard]] static auto branch_label(const std::string_view if_label,
                                           const size_t branch_index)
        -> std::string {

        if (branch_index == 0) {
            return std::string{if_label};
        }

        return std::format("{}.{}", if_label, branch_index);
    }

    [[nodiscard]] static auto create_label_else_branch(
        const bool has_else_code, const std::string_view if_label,
        const std::string_view label_after_if) -> std::string {

        if (not has_else_code) {
            return std::string{label_after_if};
        }

        return std::format("{}.else", if_label);
    }

    [[nodiscard]] static auto ends_chain(const condition_result known) -> bool {
        return known == condition_result::always_true;
    }

    // the chain rule, the same for the compile and for the paths of the
    // definite assignment: a branch whose condition is false does not run, one
    // whose condition is true ends the chain, the later branches and the
    // 'else' do not run
    [[nodiscard]] static auto runs(const condition_result known) -> bool {
        return known != condition_result::always_false;
    }
};
