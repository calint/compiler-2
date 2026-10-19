#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>

#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

class stmt_loop final : public statement {
    stmt_block code_;

  public:
    stmt_loop(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk} {

        set_type(tc.builtins().void_type());

        // e.g. 'loop { x = x + 1 if x == 9 break }'
        const std::string label{tc.create_unique_label(tok(), "loop")};
        tc.enter_loop(label);
        code_ = {tc, tz};
        tc.exit_loop(label);
    }

    stmt_loop() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        code_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, "loop");

        const std::string label{tc.create_unique_label(tok(), "loop")};
        x.label(indent, label);
        tc.enter_loop(label);
        code_.compile(tc, indent, dst);
        x.branch(indent, label);
        x.label(indent, toc::end_label(label));
        tc.exit_loop(label);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        const std::optional<assigned_bytes> breaks{code_.trace_loop_body(flow)};

        // without a 'break' only 'return' or 'exit' leave the loop
        if (not breaks) {
            flow.end_path();
            return;
        }

        flow.assigned = *breaks;
    }
};
