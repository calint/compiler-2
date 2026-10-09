#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <string_view>

#include "decouple.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_check.hpp"

class stmt_return final : public statement {
  public:
    stmt_return(const toc& tc, const token src_loc_tk) : statement{src_loc_tk} {
        set_type(tc.get_type_void());
    }

    stmt_return() = default;

    //
    // overridden methods
    //

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        if (not tc.is_inlined_func()) {
            x.return_function(indent);
            return;
        }

        const std::string_view return_label{tc.get_func_return_label()};

        if (return_label.empty()) {
            // note: return from 'main' is exiting
            x.exit(tok(), indent, operand::imm("0", tc.get_type_default()));
            return;
        }

        // jump to return labels
        x.branch(indent, return_label);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.assert_set_at_return();
        flow.end_path();
    }
};
