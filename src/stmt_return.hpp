#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_return final : public statement {
  public:
    stmt_return(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    stmt_return() = default;
    stmt_return(const stmt_return&) = default;
    stmt_return(stmt_return&&) = default;
    auto operator=(const stmt_return&) -> stmt_return& = default;
    auto operator=(stmt_return&&) -> stmt_return& = default;

    ~stmt_return() override = default;

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get the jump target to exit function
        const std::string& ret_lbl{tc.get_func_return_label_or_throw(tok())};

        if (ret_lbl.empty()) {
            // note: return from 'main' is exit
            os << "mov rdi, 0\nmov rax, 60\nsyscall\n";
            return;
        }

        // jump to return labels
        toc::asm_jmp(tok(), os, indent, ret_lbl);
    }
};
