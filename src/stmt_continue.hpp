#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_continue final : public statement {
  public:
    stmt_continue(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    ~stmt_continue() override = default;

    stmt_continue() = default;
    stmt_continue(const stmt_continue&) = default;
    stmt_continue(stmt_continue&&) = default;
    auto operator=(const stmt_continue&) -> stmt_continue& = default;
    auto operator=(stmt_continue&&) -> stmt_continue& = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get current loop start labels
        const std::string_view loop_label{tc.get_loop_label_or_throw(tok())};
        // jump to it
        toc::asm_jmp(os, indent, loop_label);
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
