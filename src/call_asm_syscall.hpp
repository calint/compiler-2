#pragma once

class call_asm_syscall final : public call_asm {
public:
  inline call_asm_syscall(toc &tc, token tk, tokenizer &t)
      : call_asm{tc, move(tk), t} {
    set_type(tc.get_type_void());
  }

  inline call_asm_syscall() = default;
  inline call_asm_syscall(const call_asm_syscall &) = default;
  inline call_asm_syscall(call_asm_syscall &&) = default;
  inline call_asm_syscall &operator=(const call_asm_syscall &) = default;
  inline call_asm_syscall &operator=(call_asm_syscall &&) = default;

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.source_comment(*this, os, indent);
    toc::indent(os, indent);
    os << "syscall" << endl;
  }
};
