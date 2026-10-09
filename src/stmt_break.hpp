#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <string_view>

#include "decouple.hpp"
#include "machine.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_check.hpp"

class stmt_break final : public statement {
  public:
    stmt_break(const toc& tc, const token src_loc_tk) : statement{src_loc_tk} {
        set_type(tc.get_type_void());
    }

    stmt_break() = default;

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // get current loop exit label
        const std::string_view loop_label{tc.get_looping_label_or_throw(tok())};

        // jump out of the loop or foo
        x.branch(indent, toc::end_label(loop_label));
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.record_break();
    }
};
