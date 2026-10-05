#pragma once
// reviewed: 2025-09-28

#include "machine.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_continue final : public statement {
  public:
    stmt_continue(const toc& tc, const token src_loc_tk)
        : statement{src_loc_tk} {

        set_type(tc.get_type_void());
    }

    stmt_continue() = default;

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // get current loop start labels
        const std::string_view loop_label{tc.get_looping_label_or_throw(tok())};

        if (tc.is_in_loop_block()) {
            x.branch(indent, loop_label);
            return;
        }

        // is in foo block
        x.branch(indent, toc::continue_label(loop_label));
    }

    // the next iteration starts with at least the loop entry coverage
    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.end_path();
    }
};
