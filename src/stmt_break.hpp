#pragma once
// reviewed: 2025-09-28

#include <format>

#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "x86.hpp"

class stmt_break final : public statement {
  public:
    stmt_break(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    stmt_break() = default;

    auto compile(toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x.comment_source(tc, *this, indent);

        // get current loop exit label
        const std::string_view loop_label{tc.get_looping_label_or_throw(tok())};

        // jump out of the loop or foo
        x.jmp( indent, std::format("{}_end", loop_label));
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
