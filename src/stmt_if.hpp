#pragma once
// reviewed: 2025-09-28

#include "stmt_block.hpp"
#include "stmt_if_branch.hpp"
#include <algorithm>

class stmt_if final : public statement {
    std::vector<stmt_if_branch> branches_;
    std::vector<token> else_if_tokens_;
    stmt_block else_code_;

  public:
    stmt_if(toc& tc, token tk, tokenizer& tz) : statement{tk} {
        set_type(tc.get_type_void());
        // e.g. if a == b {x = 1} else if c == d {y = 2} else {z = 3} broken
        // down in branches 'a == b {x = 1}', 'c == d {y = 2}' ending with an
        // optional 'else' block

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
            // check if 'else if'
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

    ~stmt_if() override = default;

    stmt_if() = default;
    stmt_if(const stmt_if&) = default;
    stmt_if(stmt_if&&) = default;
    auto operator=(const stmt_if&) -> stmt_if& = default;
    auto operator=(stmt_if&&) -> stmt_if& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        // output first branch
        const stmt_if_branch& branch{branches_.at(0)};
        branch.source_to(os);
        // output the remaining 'else if' branches
        const size_t n{branches_.size()};
        for (size_t i{1}; i < n; i++) {
            const stmt_if_branch& else_if_branch{branches_.at(i)};
            // 'else if' tokens as read from source
            else_if_tokens_.at((i - 1) << 1U).source_to(os);
            else_if_tokens_.at(((i - 1) << 1U) + 1).source_to(os);
            else_if_branch.source_to(os);
        }
        // the 'else' code
        if (not else_code_.is_empty()) {
            else_if_tokens_.back().source_to(os);
            else_code_.source_to(os);
        }
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        // make unique labels considering in-lined functions
        const std::string_view call_path{tc.get_call_path(tok())};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string label_after_if =
            call_path.empty() ? std::format("if_{}_end", src_loc)
                              : std::format("if_{}_{}_end", src_loc, call_path);
        const std::string label_else_branch = stmt_if::create_label_else_branch(
            else_code_, call_path, src_loc, label_after_if);

        const size_t n{branches_.size()};

        bool branch_evaluated_to_true{};
        for (size_t i{}; i < n; i++) {
            const stmt_if_branch& if_branch{branches_.at(i)};
            std::string jmp_if_false{label_else_branch};
            std::string jmp_if_done{label_after_if};
            if (i < n - 1) {
                // if branch is false jump to next if
                jmp_if_false = branches_.at(i + 1).if_bgn_label(tc);
            } else {
                // if the last branch and no 'else', then no need to jump to
                // 'after_if' after the code of the branch has been executed.
                // just continue
                if (else_code_.is_empty()) {
                    jmp_if_done = "";
                }
            }
            // compile the condition which might return that the condition was a
            // constant evaluation
            std::optional<bool> const_eval{
                if_branch.compile(tc, os, indent, jmp_if_false, jmp_if_done)};
            // if the condition was a constant evaluation and the result was
            // true
            if (const_eval and *const_eval) {
                branch_evaluated_to_true = true;
                break;
            }
        }
        // if it wasn't a constant evaluation that was true, generate the else
        // code
        if (not branch_evaluated_to_true) {
            if (not else_code_.is_empty()) {
                toc::asm_label(tok(), os, indent, label_else_branch);
                else_code_.compile(tc, os, indent, dst);
            }
        }

        toc::asm_label(tok(), os, indent, label_after_if);
    }

    [[nodiscard]] auto else_block() const -> const stmt_block& {
        return else_code_;
    }

    [[nodiscard]] auto is_var_used(const std::string_view var) const
        -> bool override {

        return std::ranges::any_of(branches_, [&var](const stmt_if_branch& e) {
            return e.is_var_used(var);
        });
    }

  private:
    static auto create_label_else_branch(const stmt_block& else_code,
                                         const std::string_view call_path,
                                         const std::string_view src_loc,
                                         const std::string_view label_after_if)
        -> std::string {

        if (else_code.is_empty()) {
            return std::string{label_after_if};
        }

        return (call_path.empty()
                    ? std::format("if_else_{}", src_loc)
                    : std::format("if_else_{}_{}", src_loc, call_path));
    }
};
