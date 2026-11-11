#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "expr_bool.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "stmt_block.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "ub_assigned.hpp"

// 'if', 'else if' and 'else': the branches are tried in order and the first
// whose condition holds runs; a branch that is constant true ends the chain
// and one that is constant false is not emitted
class stmt_if final : public statement {
    // the 'else if' before a branch after the first
    struct else_if_tokens {
        token else_tk;
        token if_tk;
    };

    // one condition of an 'if' or 'else if' and the block it guards, e.g.
    // 'a == b {x = 1}' of 'if a == b {x = 1} else {y = 2}'
    struct branch {
        // 'lead_in' was read before
        branch(toc& tc, tokenizer& tz,
               const std::optional<else_if_tokens>& lead_in)
            : lead{lead_in}, condition{parse_condition(tc, tz)},
              constant_at_parse{condition.constant_value(tc)}, code{tc, tz} {}

        // none for the first branch, kept to reproduce the source
        std::optional<else_if_tokens> lead;
        expr_bool condition;
        // the value of the condition known where the 'if' is written, e.g.
        // from literals and constants, empty when it depends on the run or on
        // a call
        std::optional<bool> constant_at_parse;
        stmt_block code;

        auto source_to(std::ostream& os) const -> void {
            if (lead) {
                lead->else_tk.source_to(os);
                lead->if_tk.source_to(os);
            }

            condition.source_to(os);
            code.source_to(os);
        }

        // the condition is read before the branch code runs
        auto trace_assignment(assignment_flow& flow) const -> void {
            condition.assert_result_not_read(flow);
            code.trace_assignment(flow);
        }

        //
        // statics
        //

        // the condition holds the whitespace before it; a '{' there means
        // the condition is missing, e.g. 'if {x = 1}'
        [[nodiscard]] static auto parse_condition(toc& tc, tokenizer& tz)
            -> expr_bool {

            const token ws_tk{tz.next_whitespace_token()};

            if (tz.peek_char_after_whitespace() == '{') {
                throw compile_error{tz, "expected a condition before '{'"};
            }

            expr_bool condition{tc, ws_tk, tz};

            condition.assert_boolean_operands();

            return condition;
        }
    };

    // what a branch found of its condition: its value where known and
    // whether the code after the condition is reached, where the next branch
    // and the 'else' start, e.g. not after 'if stop() == 1' with 'stop' an
    // inlined 'exit'
    struct branch_condition {
        std::optional<bool> known;
        bool is_end_reachable{};
    };

    // e.g. 'else {z = 3}'
    struct else_part {
        token else_tk;
        stmt_block code;
    };

    std::vector<branch> branches_;
    // none without an 'else'
    std::optional<else_part> else_;

  public:
    stmt_if(toc& tc, const token& src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk} {

        // 'if a == b {x = 1} else if c == d {y = 2} else {z = 3}' breaks down
        // into the branches 'a == b {x = 1}' and 'c == d {y = 2}' and an
        // optional 'else' block, the 'if' token has been read
        branches_.emplace_back(tc, tz, std::nullopt);

        while (const std::optional<else_if_tokens> lead{parse_else(tc, tz)}) {
            branches_.emplace_back(tc, tz, lead);
        }
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        for (const branch& b : branches_) {
            b.source_to(os);
        }

        // the 'else' code, also when it is empty
        if (else_) {
            else_->else_tk.source_to(os);
            else_->code.source_to(os);
        }
    }

    auto compile(toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        // the 'if' keyword locates the labels shared by all branches
        const std::string if_label{tc.create_unique_label(tok(), "if")};
        const std::string label_after_if{toc::end_label(if_label)};

        toc::path_merge paths{tc};
        // a branch starts where the condition before it is false, the first
        // at the 'if'
        bool is_start_reachable{tc.is_reachable()};
        // the branches after one whose condition is true and the 'else' never
        // run; they hold the compile rules too, see 'toc::check_skipped'
        bool is_chain_ended{};
        size_t index{};

        for (; index < branches_.size() and not is_chain_ended; ++index) {
            paths.begin_path(is_start_reachable);

            const branch_condition condition{
                compile_branch_at(tc, index, if_label),
            };

            is_start_reachable = condition.is_end_reachable;

            const std::optional<bool> known{condition.known};

            // note: the compile knows at least what the parse knew, the
            //       paths of the definite assignment follow the parse

            assert(not branches_.at(index).constant_at_parse or
                   branches_.at(index).constant_at_parse == known);

            if (runs(known)) {
                paths.join_path();
            }

            is_chain_ended = ends_chain(known);
        }

        if (is_chain_ended) {
            check_skipped_from(tc, index, if_label);
        } else {
            compile_else(tc, if_label, paths, is_start_reachable);
        }

        machine& x{tc.machine()};

        x.label(tc.indent(), label_after_if);
    }

    // the paths that the conditions known where the 'if' is written leave,
    // e.g. 'if 1 == 1 { res = 2 }' sets 'res' on every path
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assignment_flow::path_merge paths{flow};

        // the branches that never run hold the rules too, their conditions
        // and bodies, e.g. 'if false { a = res }'
        bool is_chain_ended{};

        for (const branch& b : branches_) {
            const std::optional<bool> known{b.constant_at_parse};

            if (is_chain_ended or not runs(known)) {
                paths.trace_skipped(b);
            } else {
                paths.trace(b);
            }

            is_chain_ended = is_chain_ended or ends_chain(known);
        }

        if (is_chain_ended) {
            if (else_) {
                paths.trace_skipped(else_->code);
            }

            return;
        }

        // an empty 'else' is the path that skips every branch
        if (else_) {
            paths.trace(else_->code);
        } else {
            paths.trace_empty();
        }
    }

  private:
    // the branches from 'first_index' and the 'else' after a branch whose
    // condition is true never run, they hold the compile rules too
    auto check_skipped_from(toc& tc, const size_t first_index,
                            const std::string_view if_label) const -> void {

        for (size_t i{first_index}; i < branches_.size(); ++i) {
            tc.check_skipped([&] -> void {
                std::ignore = compile_branch_at(tc, i, if_label);
            });
        }

        if (not else_) {
            return;
        }

        tc.check_skipped(
            [&] -> void { else_->code.compile(tc, destination{}); });
    }

    // the branch at 'branch_index': its condition at its label, its code and
    // the jump past the other branches; what it found of its condition
    [[nodiscard]] auto compile_branch_at(toc& tc, const size_t branch_index,
                                         const std::string_view if_label) const
        -> branch_condition {

        const branch& b{branches_.at(branch_index)};
        const bool is_last_branch{branch_index == branches_.size() - 1};

        const std::string label{branch_label(if_label, branch_index)};

        // an 'or' condition branches to the code as soon as one operand is
        // true
        const std::string code_label{std::format("{}.code", label)};

        // a false condition continues at the next branch or the 'else'
        const std::string jmp_if_false{
            is_last_branch ? else_label(if_label)
                           : branch_label(if_label, branch_index + 1),
        };

        machine& x{tc.machine()};

        x.label(tc.indent(), label);

        const std::optional<bool> known{
            b.condition.compile_branches(tc,
                                         {
                                             .if_false{jmp_if_false},
                                             .if_true{code_label},
                                         },
                                         {}),
        };

        const branch_condition condition{
            .known{known},
            .is_end_reachable{tc.is_reachable()},
        };

        // note: code that never runs holds the compile rules too, see
        //       'toc::check_skipped'

        if (known == false) {
            tc.check_skipped(
                [&] -> void { b.code.compile(tc, destination{}); });

            return condition;
        }

        x.label(tc.indent(), code_label);
        b.code.compile(tc, destination{});

        // the last branch without an 'else' continues after the 'if'
        // without a jump
        const bool falls_through{is_last_branch and else_code() == nullptr};

        if (not known and not falls_through) {
            x.branch(tc.indent(), toc::end_label(if_label));
        }

        return condition;
    }

    // an empty 'else' is the path that skips every branch, it starts where
    // the condition of the last branch is false
    auto compile_else(toc& tc, const std::string_view if_label,
                      toc::path_merge& paths,
                      const bool is_start_reachable) const -> void {

        paths.begin_path(is_start_reachable);

        if (const stmt_block* const code{else_code()}) {
            machine& x{tc.machine()};

            x.label(tc.indent(), else_label(if_label));
            code->compile(tc, destination{});
        }

        paths.join_path();
    }

    // an empty 'else' compiles as none, null then
    [[nodiscard]] auto else_code() const -> const stmt_block* {
        if (not else_ or else_->code.is_empty()) {
            return nullptr;
        }

        return &else_->code;
    }

    // an empty 'else' continues after the 'if'
    [[nodiscard]] auto else_label(const std::string_view if_label) const
        -> std::string {

        if (else_code() == nullptr) {
            return toc::end_label(if_label);
        }

        return std::format("{}.else", if_label);
    }

    // reads what follows a branch: 'else if' continues the chain and is
    // returned, 'else' ends it with the else code, anything else is a new
    // statement
    auto parse_else(toc& tc, tokenizer& tz) -> std::optional<else_if_tokens> {
        const std::optional<token> else_tk{tz.is_next_word_token("else")};

        if (not else_tk) {
            return std::nullopt;
        }

        if (const std::optional<token> if_tk{tz.is_next_word_token("if")}) {
            return else_if_tokens{
                .else_tk{*else_tk},
                .if_tk{*if_tk},
            };
        }

        else_ = {
            .else_tk{*else_tk},
            .code{tc, tz},
        };

        return std::nullopt;
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

    [[nodiscard]] static auto ends_chain(const std::optional<bool> known)
        -> bool {

        return known.value_or(false);
    }

    // the chain rule, the same for the compile and for the paths of the
    // definite assignment: a branch whose condition is false does not run, one
    // whose condition is true ends the chain, the later branches and the
    // 'else' do not run
    [[nodiscard]] static auto runs(const std::optional<bool> known) -> bool {
        return known.value_or(true);
    }
};
