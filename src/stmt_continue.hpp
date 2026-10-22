#pragma once

#include <cassert>
#include <cstddef>
#include <optional>

#include "ident_info.hpp"
#include "machine.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

class stmt_continue final : public statement {
  public:
    stmt_continue(const toc& tc, const token src_loc_tk)
        : statement{src_loc_tk} {

        tc.scopes().assert_in_loop(src_loc_tk);
        set_type(tc.builtins().void_type());
    }

    stmt_continue() = default;

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const std::optional<loop_target> loop{tc.scopes().innermost_loop()};

        // note: the parse rejected a 'continue' outside a loop
        assert(loop);

        if (not loop->is_foo) {
            x.branch(indent, loop->label);
            return;
        }

        // a 'foo' advances its counter before the next iteration
        x.branch(indent, toc::continue_label(loop->label));
    }

    // the next iteration starts with at least the loop entry coverage
    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.end_path();
    }
};
