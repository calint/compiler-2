#pragma once
#include "stmt_if_branch.hpp"

class stmt_if final : public statement {
  vector<stmt_if_branch> branches_{};
  vector<token> else_if_tokens_{};
  stmt_block else_code_{};

public:
  inline stmt_if(toc &tc, const token &tk, tokenizer &tz) : statement{{}, tk} {
    set_type(tc.get_type_void());
    // if a=b {x} else if c=d {y} else {z}
    // 'a=b {x}', 'c=d {y}' are 'branches'
    // 'if' token has been read
    while (true) {
      // read branch i.e. a=b {x}
      branches_.emplace_back(tc, tz);

      // check if it is a 'else if' or 'else' or a new statement
      token tkn{tz.next_token()};
      if (not tkn.is_name("else")) {
        // not 'else', push token back instream and exit
        tz.put_back_token(tkn);
        return;
      }
      // is 'else'
      // check if 'else if'
      token tkn2{tz.next_token()};
      if (not tkn2.is_name("if")) {
        // not 'else if', push token back instream and exit
        tz.put_back_token(tkn2);
        // 'else' branch
        // save tokens to be able to reproduce the source
        else_if_tokens_.push_back(move(tkn));
        // read the 'else' code
        else_code_ = {tc, tz};
        return;
      }
      // 'else if': continue reading if branches
      // save tokens to be able to reproduce the source
      else_if_tokens_.push_back(move(tkn));
      else_if_tokens_.push_back(move(tkn2));
    }
  }

  inline stmt_if() = default;
  inline stmt_if(const stmt_if &) = default;
  inline stmt_if(stmt_if &&) = default;
  inline auto operator=(const stmt_if &) -> stmt_if & = default;
  inline auto operator=(stmt_if &&) -> stmt_if & = default;

  inline ~stmt_if() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    // output first branch
    const stmt_if_branch &branch{branches_[0]};
    branch.source_to(os);
    // output the remaining 'else if' branches
    const size_t n{branches_.size()};
    for (size_t i{1}; i < n; i++) {
      const stmt_if_branch &br{branches_[i]};
      // 'else if' tokens as read from source
      else_if_tokens_[((i - 1)) << 1U].source_to(os);
      else_if_tokens_[((i - 1) << 1U) + 1].source_to(os);
      br.source_to(os);
    }
    // the 'else' code
    if (not else_code_.is_empty()) {
      else_if_tokens_.back().source_to(os);
      else_code_.source_to(os);
    }
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    // make unique labels considering in-lined functions
    const string &call_path{tc.get_inline_call_path(tok())};
    const string &src_loc{tc.source_location_for_use_in_label(tok())};
    const string &cp{call_path.empty() ? "" : "_" + call_path};

    const string &label_after_if{"if_" + src_loc + cp + "_end"};
    const string &label_else_branch{
        not else_code_.is_empty() ? "if_else_" + src_loc + cp : label_after_if};

    const size_t n{branches_.size()};

    bool branch_evaluated_to_true{false};
    for (size_t i{0}; i < n; i++) {
      const stmt_if_branch &e{branches_[i]};
      string jmp_if_false{label_else_branch};
      string jmp_after_if{label_after_if};
      if (i < n - 1) {
        // if branch is false jump to next if
        jmp_if_false = branches_[i + 1].if_bgn_label(tc);
      } else {
        // if last branch and no 'else' then
        //   no need to jump to 'after_if' after the code for the branch
        //   has been executed. just continue
        if (else_code_.is_empty()) {
          jmp_after_if = "";
        }
      }
      optional<bool> const_eval{
          e.compile(tc, os, indent, jmp_if_false, jmp_after_if)};
      if (const_eval and *const_eval) {
        branch_evaluated_to_true = true;
        break;
      }
    }
    if (not branch_evaluated_to_true) {
      if (not else_code_.is_empty()) {
        toc::asm_label(*this, os, indent, label_else_branch);
        else_code_.compile(tc, os, indent + 1);
      }
    }
    toc::asm_label(*this, os, indent, label_after_if);
  }
};
