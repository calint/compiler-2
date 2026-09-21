#pragma once
// reviewed: 2025-09-28

#include "machine.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_return final : public statement {
  public:
    stmt_return(const toc& tc, const token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    stmt_return() = default;

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        if (not tc.is_inlined_func()) {
            x.return_function(indent);

            return;
        }

        // get the jump target to exit function
        const std::string_view ret_lbl{tc.get_func_return_label()};

        if (ret_lbl.empty()) {
            // note: return from 'main' is exiting
            x.exit_process(tok(), indent, 0);

            return;
        }

        // jump to return labels
        x.branch(indent, ret_lbl);
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
