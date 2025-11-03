#pragma once
// reviewed: 2025-09-28

#include <string_view>

#include "stmt_call.hpp"

class stmt_builtin_mov final : public stmt_call {
  public:
    stmt_builtin_mov(toc& tc, token tk, tokenizer& tz)
        : stmt_call{tc, {}, tk, tz} {

        set_type(tc.get_type_void());
    }

    ~stmt_builtin_mov() override = default;

    stmt_builtin_mov() = default;
    stmt_builtin_mov(const stmt_builtin_mov&) = default;
    stmt_builtin_mov(stmt_builtin_mov&&) = default;
    auto operator=(const stmt_builtin_mov&) -> stmt_builtin_mov& = default;
    auto operator=(stmt_builtin_mov&&) -> stmt_builtin_mov& = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        if (arguments_size() != 2) {
            throw compiler_exception{tok(), "expected 2 arguments"};
        }

        // the assembler command might not need to resolve expressions
        const ident_info dst_info{tc.make_ident_info(argument(0))};

        const statement& src_arg{argument(1)};
        if (src_arg.is_expression() or src_arg.is_identifier()) {
            src_arg.compile(tc, os, indent + 1, dst_info);
            return;
        }

        // 'src_arg' is not an expression
        const ident_info src_info{tc.make_ident_info(src_arg)};
        if (src_info.is_const()) {
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.operand,
                       std::format("{}{}", src_arg.get_unary_ops().to_string(),
                                   src_info.operand));
            return;
        }
        // variable, register or field
        tc.asm_cmd(tok(), os, indent, "mov", dst_info.operand,
                   src_info.operand);
        src_arg.get_unary_ops().compile(tc, os, indent, dst_info.operand);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        argument(1).assert_var_not_used(var);
    }

    [[nodiscard]] auto is_var_set(const std::string_view var) const
        -> bool override {
        return argument(0).identifier() == var;
    }
};
