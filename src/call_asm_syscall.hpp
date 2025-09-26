#pragma once

#include "stmt_call.hpp"

class call_asm_syscall final : public stmt_call {
  public:
    inline call_asm_syscall(toc& tc, token tk, tokenizer& tz)
        : stmt_call{tc, {}, std::move(tk), tz} {

        set_type(tc.get_type_void());
    }

    inline call_asm_syscall() = default;
    inline call_asm_syscall(const call_asm_syscall&) = default;
    inline call_asm_syscall(call_asm_syscall&&) = default;
    inline auto operator=(const call_asm_syscall&)
        -> call_asm_syscall& = default;
    inline auto operator=(call_asm_syscall&&) -> call_asm_syscall& = default;

    inline ~call_asm_syscall() override = default;

    inline void
    compile(toc& tc, std::ostream& os, size_t indent,
            [[maybe_unused]] const std::string& dst = "") const override {

        tc.comment_source(*this, os, indent);

        if (arg_count() != 0) {
            throw compiler_exception(tok(), "didn't expect arguments");
        }

        toc::indent(os, indent);
        os << "syscall" << std::endl;
    }
};
