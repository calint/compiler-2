#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_comment final : public statement {
    unary_ops uops_;
    std::string_view line_;
    token ws1_;

  public:
    stmt_comment(const toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : statement{tk}, uops_{std::move(uops)}, line_{tz.read_rest_of_line()},
          ws1_{tz.next_whitespace_token()} {
        set_type(tc.get_type_void());
    }

    ~stmt_comment() override = default;

    stmt_comment() = default;
    stmt_comment(const stmt_comment&) = default;
    stmt_comment(stmt_comment&&) = default;
    auto operator=(const stmt_comment&) -> stmt_comment& = default;
    auto operator=(stmt_comment&&) -> stmt_comment& = default;

    auto source_to(std::ostream& os) const -> void override {
        uops_.source_to(os);
        statement::source_to(os);
        std::println(os, "{}", line_);
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);
    }
};
