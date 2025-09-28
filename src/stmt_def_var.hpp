#pragma once
// reviewed: 2025-09-28

#include "stmt_assign_var.hpp"

class null_stream : public std::ostream {
    class null_buffer : public std::streambuf {
      public:
        auto overflow(int c) -> int override { return c; }
    } nb_{};

  public:
    null_stream() : std::ostream(&nb_) {}
};

class stmt_def_var final : public statement {
    token name_tk_;
    token type_tk_;
    stmt_assign_var initial_value_;

  public:
    stmt_def_var(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)}, name_tk_{tz.next_token()} {

        // check if type declared
        if (tz.is_next_char(':')) {
            type_tk_ = tz.next_token();
        }
        // expect initialization
        if (not tz.is_next_char('=')) {
            throw compiler_exception(name_tk_, "expected '=' and initializer");
        }
        // get type reference from token
        const type& tp{type_tk_.name().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.name())};
        set_type(tp);

        // add var to toc without causing output by passing a null stream
        null_stream null_strm;
        tc.add_var(name_tk_, null_strm, 0, name_tk_.name(), tp, false);
        initial_value_ = {tc, name_tk_, type_tk_, tz};
    }

    stmt_def_var() = default;
    stmt_def_var(const stmt_def_var&) = default;
    stmt_def_var(stmt_def_var&&) = default;
    auto operator=(const stmt_def_var&) -> stmt_def_var& = default;
    auto operator=(stmt_def_var&&) -> stmt_def_var& = default;

    ~stmt_def_var() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        initial_value_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.add_var(name_tk_, os, indent, name_tk_.name(), get_type(), false);
        tc.comment_source(*this, os, indent);
        initial_value_.compile(tc, os, indent, name_tk_.name());
    }
};
