#pragma once
// reviewed: 2025-09-28

#include <string>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "null_stream.hpp"
#include "statement.hpp"
#include "stmt_const.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class stmt_def_const final : public statement {
    token name_tk_;
    token equals_tk_;
    stmt_const const_;

  public:
    stmt_def_const(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (name_tk_.is_empty()) {
            throw compiler_exception(name_tk_, "expected name of constant");
        }

        equals_tk_ = tz.is_next_char_token('=');
        if (equals_tk_.is_empty()) {
            throw compiler_exception(name_tk_,
                                     "expected '=' followed by a constant "
                                     "value");
        }

        const_ = {tc, tz, 0};

        if (not const_.has_value()) {
            throw compiler_exception(const_.tok(), "expected constant value");
        }

        set_type(tc.get_type_void());

        // add var to toc without causing output by passing a null x86
        null_stream null_strm;
        x86 x{null_strm, tc.source()};

        tc.add_const(x, name_tk_, 0, name_tk_.text(), const_.value());
    }

    stmt_def_const() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);
        const_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] x86& x,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        tc.add_const(x, name_tk_, indent, name_tk_.text(), const_.value());
    }
};
