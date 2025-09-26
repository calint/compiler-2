#pragma once

#include "statement.hpp"
#include "toc.hpp"

class stmt_continue final : public statement {
  public:
    stmt_continue(toc& tc, token tk) : statement{std::move(tk)} {
        set_type(tc.get_type_void());
    }

    stmt_continue() = default;
    stmt_continue(const stmt_continue&) = default;
    stmt_continue(stmt_continue&&) = default;
    auto operator=(const stmt_continue&) -> stmt_continue& = default;
    auto operator=(stmt_continue&&) -> stmt_continue& = default;

    ~stmt_continue() override = default;

    void compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const override {

        tc.comment_source(*this, os, indent);

        // get current loop start labels
        const std::string& loop_label = tc.get_loop_label_or_throw(tok());
        // jump to it
        toc::asm_jmp(tok(), os, indent, loop_label);
    }
};
