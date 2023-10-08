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
    tc.source_comment(*this, os, indent);
    const string &ret{tc.get_func_return_label_or_throw(*this)};
    if (ret.empty()) {
      // not in-lined
      const string &ret_var{tc.get_func_return_var_name_or_throw(*this)};
      if (not ret_var.empty()) {
        const ident_resolved &ir{tc.resolve_identifier(*this, ret_var, false)};
        const string &src_resolved{ir.id_nasm};
        tc.asm_cmd(*this, os, indent, "mov", "rax", src_resolved);
      }
      tc.asm_pop(*this, os, indent, "rbp");
      tc.asm_ret(*this, os, indent);
      return;
    }
    // in-lined
    tc.asm_jmp(*this, os, indent, ret);
  }
};
