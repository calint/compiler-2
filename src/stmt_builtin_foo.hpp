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
#include "utils.hpp"

class stmt_builtin_foo final : public statement {
    stmt_identifier ident_;
    stmt_block code_;

  public:
    stmt_builtin_foo(toc& tc, token tk, tokenizer& tz)
        : statement{tk, unary_ops{}} {

        set_type(tc.get_type_void());

        ident_ = stmt_identifier{tc, unary_ops{}, tz.next_token(), tz};
        if (not ident_.is_array()) {
            throw compiler_exception(ident_.tok(), "expected an array");
        }

        // add var to toc without emitting output by using a null stream
        null_stream null_strm;
        x86 x{null_strm, tc.source()};

        const ident_info ii{tc.make_ident_info_parsing(ident_)};
        tc.enter_foo("");
        const var_info var_e{
            .name{"e"},
            .type_ptr{&ii.type()},
            .declared_at_tk{},
            .reg{},
        };
        tc.add_var(x, token{}, 0, var_e, false);

        const var_info var_i{
            .name{"i"},
            .type_ptr{&tc.get_type_default()},
            .declared_at_tk{},
            .reg{},
        };
        tc.add_var(x, token{}, 0, var_i, false);

        tc.add_const(x, token{}, 0, "n", static_cast<int64_t>(ii.array_size));
        code_ = {tc, tz};
        tc.exit_foo("");
    }

    stmt_builtin_foo() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        ident_.source_to(os);
        code_.source_to(os);
    }

    auto compile(toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        // one line trimmed comment for the definition
        std::stringstream ss;
        ident_.source_to(ss);
        // make a comment-friendly string by collapsing whitespace
        x.comment_line(tok(), indent, "foo {}",
                       std::regex_replace(ss.str(), utils::regex_ws(), " "));

        const std::string loop_label{tc.get_call_path_extend(tok(), "foo")};
        tc.enter_foo(loop_label);

        const std::string reg_iter{
            x.alloc_scratch_register(tok(), indent, tc.get_type_default())};

        const ident_info ii{tc.make_ident_info(x, ident_)};

        const var_info var_e{
            .name{"e"},
            .type_ptr{&ii.type()},
            .declared_at_tk{tok()},
            .reg{reg_iter},
        };

        tc.add_var(x, ident_.tok(), indent, var_e, false);

        const var_info var_i{
            .name{"i"},
            .type_ptr{&tc.get_type_default()},
            .declared_at_tk{tok()},
            .reg{},
        };

        tc.add_var(x, ident_.tok(), indent, var_i, false);

        const ident_info var_i_info{tc.make_ident_info(x, tok(), "i")};

        const std::string& var_i_addr_op{
            var_i_info.operand.str(toc::size_qword)};

        // load address of referenced array into 'reg_iter'
        if (ii.has_lea() or ident_.is_indexed()) {
            std::vector<std::string> allocated_registers;

            const operand op{stmt_identifier::compile_effective_address(
                tok(), tc, x, indent, ident_.elems(), allocated_registers, "",
                ii.lea_path)};

            x.lea(indent, reg_iter, op.address_str());

            for (const std::string& reg :
                 allocated_registers | std::views::reverse) {
                x.free_scratch_register(tok(), indent, reg);
            }
        } else {
            x.lea(indent, reg_iter, ii.operand.address_str());
        }

        // add a constant for array size
        tc.add_const(x, ident_.tok(), indent, "n",
                     static_cast<int64_t>(ii.array_size));

        x.mov(tok(), indent, var_i_addr_op, "0");
        x.label(indent, loop_label);
        code_.compile(tc, x, indent, toc::make_ident_info_empty());
        x.label(indent + 1, loop_label + "_continue");
        x.add(indent + 2, reg_iter, std::format("{}", ii.type().size()));
        x.inc(indent + 2, var_i_addr_op);
        x.cmp(indent + 2, var_i_addr_op, std::format("{}", ii.array_size));
        x.jne(indent + 2, loop_label);
        x.label(indent, loop_label + "_end");

        x.free_scratch_register(tok(), indent, reg_iter);

        tc.exit_foo(loop_label);
    }
};
