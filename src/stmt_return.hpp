#pragma once

#include <cstddef>
#include <string_view>

#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

// 'return' leaves the function where its frame says: 'main' exits with 0, an
// inlined body jumps to the end of its call, a noinline body returns
class stmt_return final : public statement {
  public:
    stmt_return(const toc& tc, const token src_loc_tk) : statement{src_loc_tk} {
        set_type(tc.builtins().void_type());
    }

    stmt_return() = default;

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const frame& func_frm{tc.scopes().current_func_frame()};

        tc.set_path_open(false);

        if (func_frm.is_main()) {
            x.exit(tok(), indent,
                   operand::imm(0, tc.builtins().default_type()));

            return;
        }

        if (not func_frm.is_inlined_func()) {
            x.return_function(indent);
            return;
        }

        // an inlined body ends where the call continues
        x.branch(indent, func_frm.func_ret_label());
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.assert_set_at_return();
        flow.end_path();
    }
};
