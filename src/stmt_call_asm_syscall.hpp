#pragma once
// reviewed: 2025-09-28

#include "stmt_call.hpp"

class stmt_call_asm_syscall final : public stmt_call {
  public:
    stmt_call_asm_syscall(toc& tc, token tk, tokenizer& tz)
        : stmt_call{tc, {}, tk, tz} {

        set_type(tc.get_type_void());
    }

    ~stmt_call_asm_syscall() override = default;

    stmt_call_asm_syscall() = default;
    stmt_call_asm_syscall(const stmt_call_asm_syscall&) = default;
    stmt_call_asm_syscall(stmt_call_asm_syscall&&) = default;
    auto operator=(const stmt_call_asm_syscall&)
        -> stmt_call_asm_syscall& = default;
    auto operator=(stmt_call_asm_syscall&&) -> stmt_call_asm_syscall& = default;

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        if (arguments_size() != 0) {
            throw compiler_exception{tok(), "didn't expect arguments"};
        }

        toc::indent(os, indent);
        std::println(os, "syscall");
    }
};
