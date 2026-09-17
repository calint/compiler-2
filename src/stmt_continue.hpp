#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"
#include "x86.hpp"

class stmt_continue final : public statement {
  public:
    stmt_continue(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    stmt_continue() = default;

    auto compile(toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x.comment_source(tok(), indent, statement::trimmed_source(*this));

        // get current loop start labels
        const std::string_view loop_label{tc.get_looping_label_or_throw(tok())};

        if (tc.is_in_loop_block()) {
            x.jmp(indent, loop_label);
            return;
        }

        // is in foo block
        x.jmp(indent, std::string{loop_label} + "_continue");
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
