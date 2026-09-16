#pragma once
// reviewed: 2025-09-28

#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "null_stream.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

class stmt_builtin_foo final : public statement {
    token open_paren_tk_;
    stmt_identifier ident_;
    token close_paren_tk_;
    stmt_block code_;

  public:
    stmt_builtin_foo(toc& tc, token tk, tokenizer& tz)
        : statement{tk, unary_ops{}} {

        set_type(tc.get_type_void());

        open_paren_tk_ = tz.is_next_char_token('(');
        if (open_paren_tk_.is_empty()) {
            throw compiler_exception(tz, "expected '('");
        }
        ident_ = {tc, unary_ops{}, tz.next_token(), tz};
        if (not ident_.is_array()) {
            throw compiler_exception(ident_.tok(), "expected an array");
        }
        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception(tz, "expected ')'");
        }

        // static dry compilation

        null_stream os;

        const ident_info ii{tc.make_ident_info(ident_)};
        tc.enter_foo("");
        const var_info e{
            .name{"e"},
            .type_ptr{&ii.type()},
            .declared_at_tk{},
            .reg{},
        };
        tc.add_var(tok(), os, 0, e, false);

        const var_info var_i{
            .name{"i"},
            .type_ptr{&tc.get_type_default()},
            .declared_at_tk{},
            .reg{},
        };
        tc.add_var(tok(), os, 0, var_i, false);

        tc.add_const(tok(), os, 0, "n", static_cast<int64_t>(ii.array_size));
        code_ = {tc, tz};
        tc.exit_foo("");
    }

    stmt_builtin_foo() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        ident_.source_to(os);
        close_paren_tk_.source_to(os);
        code_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        tc.comment_source(*this, os, indent);

        const std::string loop_label{tc.get_call_path_extend(tok(), "foo")};
        tc.enter_foo(loop_label);

        const std::string reg_iter{tc.alloc_scratch_register(
            tok(), os, indent, tc.get_type_default())};

        const ident_info ii{tc.make_ident_info(ident_)};

        const var_info var_e{
            .name{"e"},
            .type_ptr{&ii.type()},
            .declared_at_tk{tok()},
            .reg{reg_iter},
        };

        tc.add_var(tok(), os, indent, var_e, false);

        const var_info var_i{
            .name{"i"},
            .type_ptr{&tc.get_type_default()},
            .declared_at_tk{tok()},
            .reg{},
        };

        tc.add_var(tok(), os, indent, var_i, false);

        const ident_info var_i_info{tc.make_ident_info(tok(), "i")};

        const std::string& var_i_addr_op{
            var_i_info.operand.str(toc::size_qword)};

        // load address of referenced array into 'reg_iter'
        if (ii.has_lea() or ident_.is_indexed()) {
            std::vector<std::string> allocated_registers;

            const operand op{stmt_identifier::compile_effective_address(
                tok(), tc, os, indent, ident_.elems(), allocated_registers, "",
                ii.lea_path)};

            toc::asm_lea(os, indent, reg_iter, op.address_str());

            for (const std::string& reg :
                 allocated_registers | std::views::reverse) {
                tc.free_scratch_register(tok(), os, indent, reg);
            }
        } else {
            toc::asm_lea(os, indent, reg_iter, ii.operand.address_str());
        }

        // add a constant for array size
        tc.add_const(tok(), os, indent, "n",
                     static_cast<int64_t>(ii.array_size));

        tc.asm_cmd(tok(), os, indent, "mov", var_i_addr_op, "0");
        toc::asm_label(os, indent, loop_label);
        code_.compile(tc, os, indent, toc::make_ident_info_empty());
        toc::asm_label(os, indent + 1, loop_label + "_continue");
        toc::asm_add(os, indent + 2, reg_iter,
                     std::format("{}", ii.type().size()));
        toc::asm_inc(os, indent + 2, var_i_addr_op);
        toc::asm_cmp(os, indent + 2, var_i_addr_op,
                     std::format("{}", ii.array_size));
        toc::asm_jne(os, indent + 2, loop_label);
        toc::asm_label(os, indent, loop_label + "_end");

        tc.free_scratch_register(tok(), os, indent, reg_iter);

        tc.exit_foo(loop_label);
    }
};
