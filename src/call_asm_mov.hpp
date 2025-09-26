#pragma once

#include "stmt_call.hpp"

class call_asm_mov final : public stmt_call {
public:
  inline call_asm_mov(toc &tc, token tk, tokenizer &tz)
      : stmt_call{tc, {}, std::move(tk), tz} {

    set_type(tc.get_type_void());
  }

  inline call_asm_mov() = default;
  inline call_asm_mov(const call_asm_mov &) = default;
  inline call_asm_mov(call_asm_mov &&) = default;
  inline auto operator=(const call_asm_mov &) -> call_asm_mov & = default;
  inline auto operator=(call_asm_mov &&) -> call_asm_mov & = default;

  inline ~call_asm_mov() override = default;

  inline void
  compile(toc &tc, std::ostream &os, size_t indent,
          [[maybe_unused]] const std::string &dst = "") const override {

    tc.comment_source(*this, os, indent);

    if (arg_count() != 2) {
      throw compiler_exception(tok(), "expected 2 arguments");
    }

    //? the assembler command might not need to resolve expressions
    const ident_resolved &dst_resolved{tc.resolve_identifier(arg(0), false)};

    // mark the dst as initiated
    tc.set_var_is_initiated(dst_resolved.id);

    const statement &src_arg{arg(1)};
    if (src_arg.is_expression()) {
      src_arg.compile(tc, os, indent + 1, dst_resolved.id);
      return;
    }

    // src is not an expression
    const ident_resolved &src_resolved{tc.resolve_identifier(src_arg, true)};
    if (src_resolved.is_const()) {
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
