#pragma once
// reviewed: 2025-09-28

#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

class stmt_builtin_foo final : public statement {
    stmt_identifier ident_;
    stmt_block code_;

  public:
    stmt_builtin_foo(toc& tc, token tk, tokenizer& tz)
        : statement{tk, unary_ops{}} {

        set_type(tc.get_type_void());

        ident_ = {tc, unary_ops{}, tz.next_token(), tz};
        if (not ident_.is_array()) {
            throw compiler_exception(ident_.tok(), "expected an array");
        }

        // add vars to toc without emitting output so that the code block can be
        // parsed

        const ident_info ii{tc.make_ident_info(ident_)};
        tc.enter_foo("");
        const var_info var_e{
            .name{"e"},
            .type_ptr{&ii.type_ref()},
            .src_loc_tk{},
            .reg{},
        };

        tc.add_var(token{}, 0, var_e, false);

        const var_info var_i{
            .name{"i"},
            .type_ptr{&tc.get_type_default()},
            .src_loc_tk{},
            .reg{},
        };

        tc.add_var(token{}, 0, var_i, false);

        tc.add_const(token{}, 0, "n", static_cast<int64_t>(ii.array_size));

        code_ = {tc, tz};

        tc.exit_foo("");
    }

    stmt_builtin_foo() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        ident_.source_to(os);
        code_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        // emit a one-line trimmed comment for the definition

        machine& x{tc.machine()};

        x.comment(tok(), indent, "foo {}", statement::trimmed_source(ident_));

        const std::string loop_label{tc.get_call_path_extend(tok(), "foo")};
        tc.enter_foo(loop_label);

        const operand reg_iter{x.alloc_scratch_register(ident_.tok(), indent,
                                                        tc.get_type_default())};

        const ident_info ii{tc.make_ident_info(ident_)};

        const var_info var_e{
            .name{"e"},
            .type_ptr{&ii.type_ref()},
            .src_loc_tk{tok()},
            .reg{reg_iter},
        };

        tc.add_var(ident_.tok(), indent, var_e, false);

        const var_info var_i{
            .name{"i"},
            .type_ptr{&tc.get_type_default()},
            .src_loc_tk{tok()},
            .reg{},
        };

        tc.add_var(ident_.tok(), indent, var_i, false);

        const ident_info var_i_info{tc.make_ident_info(tok(), "i")};

        // add a constant for array size
        tc.add_const(ident_.tok(), indent, "n",
                     static_cast<int64_t>(ii.array_size));

        x.comment(ident_.tok(), indent, "initiate iterator {}", var_e.name);

        // load address of referenced array into 'reg_iter'
        if (ii.has_lea() or ident_.is_indexed()) {
            std::vector<operand> allocated_registers;

            const operand op{ident_.compile_lea(
                tc, indent, tok(), allocated_registers, {}, ii.lea_path)};

            x.address_of(tok(), indent, reg_iter, op);

            x.free_scratch_registers(tok(), indent, allocated_registers);
        } else {
            x.address_of(tok(), indent, reg_iter, ii.operand);
        }

        x.comment(ident_.tok(), indent, "initiate counter {}", var_i.name);
        x.copy_value(tok(), indent, var_i_info.operand,
                     operand::imm("0", tc.get_type_default()));
        x.label(indent, loop_label);
        code_.compile(tc, indent, ident_info::make_empty());
        x.label(indent + 1, loop_label + "_continue");
        x.advance_array_iteration(indent + 2, reg_iter, var_i_info.operand,
                                  ii.type_ref().size(), ii.array_size,
                                  loop_label);

        x.label(indent, loop_label + "_end");

        x.free_scratch_register(tok(), indent, reg_iter);

        tc.exit_foo(loop_label);
    }
};
