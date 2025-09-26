#pragma once

#include "statement.hpp"
#include "toc.hpp"

class stmt_comment final : public statement {
    std::string line_;

  public:
    stmt_comment(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)}, line_{tz.read_rest_of_line()} {

        set_type(tc.get_type_void());
    }

    stmt_comment() = default;
    stmt_comment(const stmt_comment&) = default;
    stmt_comment(stmt_comment&&) = default;
    auto operator=(const stmt_comment&) -> stmt_comment& = default;
    auto operator=(stmt_comment&&) -> stmt_comment& = default;

    ~stmt_comment() override = default;

    void source_to(std::ostream& os) const override {
        statement::source_to(os);
        os << line_ << '\n';
    }

    void compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const override {
        tc.comment_source(*this, os, indent);
    }
};
