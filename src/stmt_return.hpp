#pragma once
// reviewed: 2025-09-28

#include <print>

#include "statement.hpp"
#include "toc.hpp"

class stmt_return final : public statement {
  public:
    stmt_return(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    ~stmt_return() override = default;

    stmt_return() = default;
    stmt_return(const stmt_return&) = default;
    stmt_return(stmt_return&&) = default;
    auto operator=(const stmt_return&) -> stmt_return& = default;
    auto operator=(stmt_return&&) -> stmt_return& = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get the jump target to exit function
        const std::string_view ret_lbl{
            tc.get_func_return_label_or_throw(tok())};

        if (ret_lbl.empty()) {
            // note: return from 'main' is exiting
            std::println(os, "mov rdi, 0\nmov rax, 60\nsyscall");
            return;
        }

        // jump to return labels
        toc::asm_jmp(os, indent, ret_lbl);
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
