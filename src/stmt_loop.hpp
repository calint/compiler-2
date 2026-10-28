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
        : statement{src_loc_tk}, code_{parse_code(tc, tz, src_loc_tk)} {}

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

        {
            const toc::scope_entry loop{
                tc,
                scope::loop(label, loop_end::at_break, label),
            };

            code_.compile(tc, indent, destination{});

            // the end is reached only by a 'break' that is compiled, the parse
            // rejects code after a loop without one
            tc.join_loop();
        }

        x.branch(indent, label);
        x.label(indent, toc::end_label(label));
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.trace_loop(code_, loop_end::at_break);
    }

  private:
    //
    // statics
    //

    // the block parses in the scope of the loop, where 'break' and
    // 'continue' find it
    [[nodiscard]] static auto parse_code(toc& tc, tokenizer& tz,
                                         const token& loop_tk) -> stmt_block {

        const std::string label{tc.create_unique_label(loop_tk, "loop")};

        const toc::scope_entry loop{
            tc,
            scope::loop(label, loop_end::at_break, label),
        };

        return {tc, tz};
    }
};
