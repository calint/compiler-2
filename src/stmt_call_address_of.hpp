#pragma once

#include "compiler_exception.hpp"
#include "stmt_call.hpp"
#include "unary_ops.hpp"

class stmt_call_address_of : public stmt_call {
  public:
    stmt_call_address_of(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : stmt_call{tc, std::move(uops), tk, tz} {

        set_type(tc.get_type_default());

        if (not get_unary_ops().is_empty()) {
            throw compiler_exception{
                tok(),
                "no unary operations allowed on this built-in function call"};
        }
    }

    ~stmt_call_address_of() override = default;

    stmt_call_address_of() = default;
    stmt_call_address_of(const stmt_call_address_of&) = default;
    stmt_call_address_of(stmt_call_address_of&&) = default;
    auto operator=(const stmt_call_address_of&)
        -> stmt_call_address_of& = default;
    auto operator=(stmt_call_address_of&&) -> stmt_call_address_of& = default;

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        if (arg_count() != 1) {
            throw compiler_exception{tok(), "expected 1 argument"};
        }

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        if (not dst_info.is_register()) {
            throw compiler_exception{arg(0).tok(),
                                     "destination must be a register"};
        }

        const ident_info src_info{tc.make_ident_info(arg(0), false)};

        if (not src_info.is_var()) {
            throw compiler_exception{arg(0).tok(),
                                     "argument must be a variable"};
        }

        // variable, register or field
        tc.asm_cmd(tok(), os, indent, "lea", dst_info.id_nasm,
                   std::format("[rsp - {}]", -src_info.stack_ix));
    }
};
