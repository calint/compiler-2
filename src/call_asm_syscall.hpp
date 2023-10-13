#pragma once

class call_asm_syscall final : public call_asm {
public:
  inline call_asm_syscall(toc &tc, token tk, tokenizer &tz)
      : call_asm{tc, move(tk), tz} {

    set_type(tc.get_type_void());
  }

  inline call_asm_syscall() = default;
  inline call_asm_syscall(const call_asm_syscall &) = default;
  inline call_asm_syscall(call_asm_syscall &&) = default;
  inline auto operator=(const call_asm_syscall &)
      -> call_asm_syscall & = default;
  inline auto operator=(call_asm_syscall &&) -> call_asm_syscall & = default;

  inline ~call_asm_syscall() override = default;

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.comment_source(*this, os, indent);
    toc::indent(os, indent);
    os << "syscall" << endl;
  }
};
