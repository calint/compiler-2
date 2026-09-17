#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "stmt_call.hpp"

class stmt_builtin_syscall final : public stmt_call {
  public:
    stmt_builtin_syscall(toc& tc, token tk, tokenizer& tz)
        : stmt_call{tc, {}, tk, tz.is_next_char_token('('), tz} {

        if (arguments_size() != 0) {
            throw compiler_exception{tok(), "unexpected arguments"};
        }

        set_type(tc.get_type_void());
    }

    stmt_builtin_syscall() = default;

    auto compile([[maybe_unused]] toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x.comment_source(*this, indent);
        x.syscall( indent);
    }
};
