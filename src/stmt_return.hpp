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
    const string &ret{tc.get_func_return_label_or_throw(tok())};
    if (ret.empty()) {
      // not in-lined
      const vector<func_return_info> &returns{tc.get_func_returns(tok())};
      if (not returns.empty()) {
        const func_return_info &ret_info{returns.at(0)};
        const ident_resolved &ret_resolved{
            tc.resolve_identifier(tok(), ret_info.ident_tk.name(), false)};
        const string &src_resolved{ret_resolved.id_nasm};
        tc.asm_cmd(tok(), os, indent, "mov", "rax", src_resolved);
      }
      toc::asm_pop(tok(), os, indent, "rbp");
      toc::asm_ret(tok(), os, indent);
      return;
    }
    // in-lined
    toc::asm_jmp(tok(), os, indent, ret);
  }
};
