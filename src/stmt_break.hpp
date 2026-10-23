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

class stmt_break final : public statement {
  public:
    stmt_break(const toc& tc, const token src_loc_tk) : statement{src_loc_tk} {
        tc.scopes().assert_in_loop(src_loc_tk);
        set_type(tc.builtins().void_type());
    }

    stmt_break() = default;

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const std::optional<loop_target> loop{tc.scopes().innermost_loop()};

        // note: the parse rejected a 'break' outside a loop
        assert(loop);

        x.branch(indent, toc::end_label(loop->label));
        tc.set_path_open(false);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.record_break();
    }
};
