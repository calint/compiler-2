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

// 'break' leaves the innermost 'loop' or 'foo', 'continue' goes on with its
// next round
class stmt_loop_jump final : public statement {
    bool is_break_{};

  public:
    stmt_loop_jump(const toc& tc, const token src_loc_tk)
        : statement{src_loc_tk}, is_break_{src_loc_tk.is_text("break")} {

        tc.scopes().assert_in_loop(src_loc_tk);
    }

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const std::optional<loop_target> loop{tc.scopes().innermost_loop()};

        // note: the parse rejected a 'break' or 'continue' outside a loop
        assert(loop);

        if (is_break_) {
            tc.record_break();
            x.branch(indent, toc::end_label(loop->label));
        } else {
            tc.record_continue();

            // e.g. the head of a 'loop', the counter step of a 'foo'
            x.branch(indent, loop->continue_label);
        }
    }

    // the next round of a 'continue' starts with at least the loop entry
    // coverage
    auto trace_assignment(assignment_flow& flow) const -> void override {
        if (is_break_) {
            flow.record_break();
        } else {
            flow.end_path();
        }
    }
};
