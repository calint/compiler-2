#pragma once
// reviewed: 2025-09-28

#include "stmt_call.hpp"

class call_asm_syscall final : public stmt_call {
  public:
    call_asm_syscall(toc& tc, token tk, tokenizer& tz)
        : stmt_call{tc, {}, std::move(tk), tz} {

        set_type(tc.get_type_void());
    }

    call_asm_syscall() = default;
    call_asm_syscall(const call_asm_syscall&) = default;
    call_asm_syscall(call_asm_syscall&&) = default;
    auto operator=(const call_asm_syscall&) -> call_asm_syscall& = default;
    auto operator=(call_asm_syscall&&) -> call_asm_syscall& = default;

    ~call_asm_syscall() override = default;

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        if (arg_count() != 0) {
            throw compiler_exception(tok(), "didn't expect arguments");
        }

        toc::indent(os, indent);
        os << "syscall\n";
    }
};
