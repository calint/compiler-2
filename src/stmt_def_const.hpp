#pragma once
// reviewed: 2025-09-28

#include <optional>
#include <string>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class stmt_def_const final : public statement {
    token name_tk_;
    unary_ops uops_;
    token value_tk_;

  public:
    stmt_def_const(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (name_tk_.is_empty()) {
            throw compiler_exception(name_tk_, "expected name of constant");
        }

        if (not tz.is_next_char('=')) {
            throw compiler_exception(name_tk_,
                                     "expected '=' and constant value");
        }

        uops_ = unary_ops{tz};
        value_tk_ = tz.next_token();
        std::string num_str{uops_.to_string()};
        num_str += value_tk_.text();

        int64_t value{};
        if (std::optional<int64_t> num{
                toc::parse_to_constant(value_tk_, num_str)}) {
            value = *num;
        } else {
            throw compiler_exception(
                value_tk_, std::format("cannot parse constant '{}'", num_str));
        }

        set_type(tc.get_type_default());

        tc.add_const(name_tk_, name_tk_.text(), value);
    }

    stmt_def_const() = default;
    stmt_def_const(const stmt_def_const&) = default;
    stmt_def_const(stmt_def_const&&) = default;
    auto operator=(const stmt_def_const&) -> stmt_def_const& = default;
    auto operator=(stmt_def_const&&) -> stmt_def_const& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        std::print(os, "=");
        uops_.source_to(os);
        value_tk_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {
        // note: constant is added to toc at constructor
        //       no further actions necessary
    }
};
