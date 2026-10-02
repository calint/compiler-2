#pragma once
// reviewed: 2025-09-28

#include <ranges>
#include <utility>

#include "decouple.hpp"
#include "stmt_block.hpp"
#include "stmt_if_branch.hpp"

class stmt_if final : public statement {
    // e.g. 'else if' of 'else if c == d {y = 2}'
    struct else_if_tokens {
        token else_tk;
        token if_tk;
    };

    std::vector<stmt_if_branch> branches_;
    // one for each branch after the first, kept to reproduce the source
    std::vector<else_if_tokens> else_if_tokens_;
    token else_tk_;
    stmt_block else_code_;

  public:
    stmt_if(toc& tc, const token tk, tokenizer& tz) : statement{tk} {
        set_type(tc.get_type_void());
        // e.g. if a == b {x = 1} else if c == d {y = 2} else {z = 3}, broken
        // down into branches 'a == b {x = 1}', 'c == d {y = 2}', ending with
        // an optional 'else' block

        // note: 'if' token has been read

        // read branch e.g. a == b {x = 1}
        branches_.emplace_back(tc, tz);

        while (parse_else(tc, tz)) {
            branches_.emplace_back(tc, tz);
        }
    }

    stmt_if() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        // output first branch
        const stmt_if_branch& branch{branches_.at(0)};
        branch.source_to(os);
        // output the remaining 'else if' branches
        const auto else_if_branches{branches_ | std::views::drop(1)};
        for (const auto [b, t] :
             std::views::zip(else_if_branches, else_if_tokens_)) {

            t.else_tk.source_to(os);
            t.if_tk.source_to(os);
            b.source_to(os);
        }
        // the 'else' code
        if (not else_code_.is_empty()) {
            else_tk_.source_to(os);
            else_code_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        // the 'if' keyword locates the labels shared by all branches
        const std::string if_label{tc.create_unique_label(tok(), "if")};
        const std::string label_after_if{std::format("{}.end", if_label)};

        const std::string label_else_branch{
            stmt_if::create_label_else_branch(else_code_, if_label,
                                              label_after_if),
        };

        const size_t branch_count{branches_.size()};

        bool branch_evaluated_to_true{};
        for (size_t branch_index{}; branch_index < branch_count;
             ++branch_index) {
            const stmt_if_branch& if_branch{branches_.at(branch_index)};
            const bool is_last_branch{branch_index == branch_count - 1};

            // a false condition continues at the next branch or the 'else'
            const std::string jmp_if_false{
                is_last_branch
                    ? label_else_branch
                    : branches_.at(branch_index + 1).if_bgn_label(tc),
            };

            // the last branch without an 'else' continues after the 'if'
            // without a jump
            const std::string jmp_if_done{
                is_last_branch and else_code_.is_empty() ? "" : label_after_if,
            };

            // compile the condition which might return that the condition was a
            // constant evaluation
            if (const std::optional<bool> const_eval{
                    if_branch.compile_branch(tc, indent, jmp_if_false,
                                             jmp_if_done),
                };
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
        else_tk_ = else_tk;
        else_code_ = {tc, tz};

        return false;
    }

    //
    // statics
    //

    [[nodiscard]] static auto create_label_else_branch(
        const stmt_block& else_code, const std::string_view if_label,
        const std::string_view label_after_if) -> std::string {

        if (else_code.is_empty()) {
            return std::string{label_after_if};
        }

        return std::format("{}.else", if_label);
    }

    static auto trace_path(const statement& path, const field_coverage& entry,
                           assignment_flow& flow, field_coverage& merged,
                           bool& is_reachable) -> void {

        flow.assigned = entry;
        flow.is_reachable = true;
        path.trace_assignment(flow);
        merged.intersect(flow.assigned);
        is_reachable = is_reachable or flow.is_reachable;
    }
};
