#pragma once

#include "stmt_if_branch.hpp"

class stmt_if final : public statement {
    std::vector<stmt_if_branch> branches_{};
    std::vector<token> else_if_tokens_{};
    stmt_block else_code_{};

  public:
    inline stmt_if(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)} {
        set_type(tc.get_type_void());
        // e.g. if a == b {x = 1} else if c == d {y = 2} else {z = 3}
        // broken down in branches 'a == b {x = 1}', 'c == d {y = 2}' ending
        // with an optional 'else' block

        // 'if' token has been read
        while (true) {
            // read branch e.g. a == b {x = 1}
            branches_.emplace_back(tc, tz);

            // check if it is a 'else if' or 'else' or a new statement
            token tkn{tz.next_token()};
            if (not tkn.is_name("else")) {
                // not 'else', push token back in stream and exit
                tz.put_back_token(tkn);
                return;
            }
            // is 'else'
            // check if 'else if'
            token tkn2{tz.next_token()};
            if (not tkn2.is_name("if")) {
                // not 'else if', push token back in stream and exit
                tz.put_back_token(tkn2);
                // 'else' branch
                // save tokens to be able to reproduce the source
                else_if_tokens_.emplace_back(std::move(tkn));
                // read the 'else' code
                else_code_ = {tc, tz};
                return;
            }
            // 'else if': continue reading if branches
            // save tokens to be able to reproduce the source
            else_if_tokens_.emplace_back(std::move(tkn));
            else_if_tokens_.emplace_back(std::move(tkn2));
        }
    }

    inline stmt_if() = default;
    inline stmt_if(const stmt_if&) = default;
    inline stmt_if(stmt_if&&) = default;
    inline auto operator=(const stmt_if&) -> stmt_if& = default;
    inline auto operator=(stmt_if&&) -> stmt_if& = default;

    inline ~stmt_if() override = default;

    inline void source_to(std::ostream& os) const override {
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

    inline void
    compile(toc& tc, std::ostream& os, size_t indent,
            [[maybe_unused]] const std::string& dst = "") const override {

        // make unique labels considering in-lined functions
        const std::string& call_path{tc.get_inline_call_path(tok())};
        const std::string& src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string& cp{call_path.empty() ? "" : "_" + call_path};

        const std::string& label_after_if{"if_" + src_loc + cp + "_end"};
        const std::string& label_else_branch{not else_code_.is_empty()
                                                 ? "if_else_" + src_loc + cp
                                                 : label_after_if};

        const size_t n{branches_.size()};

        bool branch_evaluated_to_true{false};
        for (size_t i{0}; i < n; i++) {
            const stmt_if_branch& e{branches_.at(i)};
            std::string jmp_if_false{label_else_branch};
            std::string jmp_after_if{label_after_if};
            if (i < n - 1) {
                // if branch is false jump to next if
                jmp_if_false = branches_.at(i + 1).if_bgn_label(tc);
            } else {
                // if last branch and no 'else' then
                //   no need to jump to 'after_if' after the code of the branch
                //     has been executed. just continue
                if (else_code_.is_empty()) {
                    jmp_after_if = "";
                }
            }
            // compile the condition which might return that the condition was a
            // constant evaluation
            std::optional<bool> const_eval{
                e.compile(tc, os, indent, jmp_if_false, jmp_after_if)};
            // if condition was a constant evaluation and result was true
            if (const_eval and *const_eval) {
                branch_evaluated_to_true = true;
                break;
            }
        }
        // if it wasn't a constant evaluation that was true generate the else
        // code
        if (not branch_evaluated_to_true) {
            if (not else_code_.is_empty()) {
                toc::asm_label(tok(), os, indent, label_else_branch);
                else_code_.compile(tc, os, indent + 1);
            }
        }

        toc::asm_label(tok(), os, indent, label_after_if);
    }
};
