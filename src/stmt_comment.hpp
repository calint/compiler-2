#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_comment final : public statement {
    unary_ops uops_;
    std::string_view line_;

  public:
    stmt_comment(const toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : statement{tk}, uops_{std::move(uops)}, line_{tz.read_rest_of_line()} {

        set_type(tc.get_type_void());
    }

    stmt_comment() = default;

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        statement::source_to(os);
        std::println(os, "{}", line_);
    }

    auto compile([[maybe_unused]] toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x.comment_source(*this, indent);
    }
};
