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
        const toc::scope_entry loop{tc, scope::loop(label)};
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

        // the end is reached only by a 'break' that is compiled, the parse
        // rejects code after a loop without one
        bool is_left{};

        {
            const toc::scope_entry loop{tc, scope::loop(label)};
            code_.compile(tc, indent, destination{});
            is_left = tc.scopes().back().is_left_by_break();
        }

        x.branch(indent, label);
        x.label(indent, toc::end_label(label));

        tc.set_path_open(is_left);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.trace_loop(code_);
    }
};
