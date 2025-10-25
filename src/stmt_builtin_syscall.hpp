#pragma once
// reviewed: 2025-09-28

#include "stmt_call.hpp"

class stmt_builtin_syscall final : public stmt_call {
  public:
    stmt_builtin_syscall(toc& tc, token tk, tokenizer& tz)
        : stmt_call{tc, {}, tk, tz} {

        set_type(tc.get_type_void());
    }

    ~stmt_builtin_syscall() override = default;

    stmt_builtin_syscall() = default;
    stmt_builtin_syscall(const stmt_builtin_syscall&) = default;
    stmt_builtin_syscall(stmt_builtin_syscall&&) = default;
    auto operator=(const stmt_builtin_syscall&)
        -> stmt_builtin_syscall& = default;
    auto operator=(stmt_builtin_syscall&&) -> stmt_builtin_syscall& = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        if (arguments_size() != 0) {
            throw compiler_exception{tok(), "didn't expect arguments"};
        }

        toc::indent(os, indent);
        std::println(os, "syscall");
    }
};
