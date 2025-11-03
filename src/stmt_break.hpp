#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_break final : public statement {
  public:
    stmt_break(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    ~stmt_break() override = default;

    stmt_break() = default;
    stmt_break(const stmt_break&) = default;
    stmt_break(stmt_break&&) = default;
    auto operator=(const stmt_break&) -> stmt_break& = default;
    auto operator=(stmt_break&&) -> stmt_break& = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get current loop exit label
        const std::string_view loop_label{tc.get_loop_label_or_throw(tok())};
        // jump out of the loop
        toc::asm_jmp(os, indent, std::format("{}_end", loop_label));
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
