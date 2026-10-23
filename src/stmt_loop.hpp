#pragma once

#include <cstddef>
#include <ostream>
#include <string>

#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

// e.g. 'loop { x = x + 1 if x == 9 break }'
class stmt_loop final : public statement {
    stmt_block code_;

  public:
    stmt_loop(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk} {

        set_type(tc.builtins().void_type());

        const std::string label{tc.create_unique_label(tok(), "loop")};
        const toc::frame_scope loop{tc, frame::loop(label)};
        code_ = {tc, tz};
    }

    stmt_loop() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        code_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, "loop");

        const std::string label{tc.create_unique_label(tok(), "loop")};
        x.label(indent, label);
        const toc::frame_scope loop{tc, frame::loop(label)};
        code_.compile(tc, indent, destination{});
        x.branch(indent, label);
        x.label(indent, toc::end_label(label));

        // note: the end is reached by a 'break', a loop without one is taken
        //       as one that ends too

        tc.set_path_open(true);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.trace_loop(code_);
    }
};
