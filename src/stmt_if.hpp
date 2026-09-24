#pragma once
// reviewed: 2025-09-28

#include <ranges>
#include <utility>

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

        // the 'if' keyword locates the labels shared by all branches
        const std::string if_label{tc.create_unique_label(tok(), "if")};
        const std::string label_after_if{std::format("{}.end", if_label)};

        const std::string label_else_branch{stmt_if::create_label_else_branch(
            else_code_, if_label, label_after_if)};

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

    // every path starts at the 'if' and afterwards only what all paths
    // assigned remains
    auto trace_assignment(assignment_flow& flow) const -> void override {
        const field_coverage entry{flow.assigned};
        field_coverage merged{field_coverage::full(entry.size_bytes())};
        bool is_reachable{};

        for (const stmt_if_branch& e : branches_) {
            trace_path(e, entry, flow, merged, is_reachable);
        }

        // an empty 'else' is the path that skips every branch
        trace_path(else_code_, entry, flow, merged, is_reachable);

        flow.assigned = std::move(merged);
        flow.is_reachable = is_reachable;
    }

  private:
    static auto trace_path(const statement& path, const field_coverage& entry,
                           assignment_flow& flow, field_coverage& merged,
                           bool& is_reachable) -> void {

        flow.assigned = entry;
        flow.is_reachable = true;
        path.trace_assignment(flow);
        merged.intersect(flow.assigned);
        is_reachable = is_reachable or flow.is_reachable;
    }

    [[nodiscard]] static auto create_label_else_branch(
        const stmt_block& else_code, const std::string_view if_label,
        const std::string_view label_after_if) -> std::string {

        if (else_code.is_empty()) {
            return std::string{label_after_if};
        }

        return std::format("{}.else", if_label);
    }
};
