#pragma once
#include "call_asm.hpp"

class call_asm_mov final : public call_asm {
public:
  inline call_asm_mov(toc &tc, token tk, tokenizer &tz)
      : call_asm{tc, move(tk), tz} {

    set_type(tc.get_type_void());
  }

  inline call_asm_mov() = default;
  inline call_asm_mov(const call_asm_mov &) = default;
  inline call_asm_mov(call_asm_mov &&) = default;
  inline auto operator=(const call_asm_mov &) -> call_asm_mov & = default;
  inline auto operator=(call_asm_mov &&) -> call_asm_mov & = default;

  inline ~call_asm_mov() override = default;

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {

    tc.comment_source(*this, os, indent);

    const ident_resolved &dst_resolved{tc.resolve_identifier(arg(0), false)};
    if (arg_count() != 2) {
      throw compiler_exception(tok(), "expected 2 arguments");
    }
    const statement &src_arg{arg(1)};
    //? the assembler commands might not need this
    if (src_arg.is_expression()) {
      src_arg.compile(tc, os, indent + 1, dst_resolved.id_nasm);
      return;
    }
    // src is not an expression
    const ident_resolved &src_resolved{tc.resolve_identifier(src_arg, true)};
    if (src_resolved.is_const()) {
      // a constant
      tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm,
                 src_arg.get_unary_ops().to_string() + src_resolved.id_nasm);
      return;
    }
    // variable, register or field
    tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm,
               src_resolved.id_nasm);
    src_arg.get_unary_ops().compile(tc, os, indent, dst_resolved.id_nasm);
  }
};
