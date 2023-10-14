#pragma once

class stmt_return final : public statement {
public:
  inline stmt_return(toc &tc, token tk) : statement{move(tk)} {
    set_type(tc.get_type_void());
  }

  inline stmt_return() = default;
  inline stmt_return(const stmt_return &) = default;
  inline stmt_return(stmt_return &&) = default;
  inline auto operator=(const stmt_return &) -> stmt_return & = default;
  inline auto operator=(stmt_return &&) -> stmt_return & = default;

  inline ~stmt_return() override = default;

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {

    tc.comment_source(*this, os, indent);

    // get the jump target to exit inlined functions
    const string &ret_lbl{tc.get_func_return_label_or_throw(tok())};
    if (ret_lbl.empty()) {
      // not in-lined
      const vector<func_return_info> &returns{tc.get_func_returns(tok())};
      if (not returns.empty()) {
        // get nasm identifier for the variable containing the return value
        const string &ret_resolved{
            tc.resolve_identifier(tok(), returns.at(0).ident_tk.name(), false)
                .id_nasm};
        // assign return value to 'rax's
        tc.asm_cmd(tok(), os, indent, "mov", "rax", ret_resolved);
      }
      // return
      toc::asm_pop(tok(), os, indent, "rbp");
      toc::asm_ret(tok(), os, indent);
      return;
    }

    // in-lined function, jump to return labels
    toc::asm_jmp(tok(), os, indent, ret_lbl);
  }
};
