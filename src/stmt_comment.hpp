#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_comment final : public statement {
    std::string line_;
    token ws1_;

  public:
    stmt_comment(const toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)}, line_{tz.read_rest_of_line()},
          ws1_{tz.next_whitespace_token()} {
        set_type(tc.get_type_void());
    }

    stmt_comment() = default;
    stmt_comment(const stmt_comment&) = default;
    stmt_comment(stmt_comment&&) = default;
    auto operator=(const stmt_comment&) -> stmt_comment& = default;
    auto operator=(stmt_comment&&) -> stmt_comment& = default;

    ~stmt_comment() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        os << line_ << '\n';
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);
    }
};
