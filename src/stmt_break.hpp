#pragma once

#include "statement.hpp"
#include "toc.hpp"

class stmt_break final : public statement {
  public:
    stmt_break(toc& tc, token tk) : statement{std::move(tk)} {
        set_type(tc.get_type_void());
    }

    stmt_break() = default;
    stmt_break(const stmt_break&) = default;
    stmt_break(stmt_break&&) = default;
    auto operator=(const stmt_break&) -> stmt_break& = default;
    auto operator=(stmt_break&&) -> stmt_break& = default;

    ~stmt_break() override = default;

    void compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const override {

        tc.comment_source(*this, os, indent);

        // get current loop exit label
        const std::string& loop_label{tc.get_loop_label_or_throw(tok())};
        // jump out of the loop
        toc::asm_jmp(tok(), os, indent, loop_label + "_end");
    }
};
