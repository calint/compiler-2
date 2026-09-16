#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"
#include "x86.hpp"

class stmt_return final : public statement {
  public:
    stmt_return(const toc& tc, token tk) : statement{tk} {
        set_type(tc.get_type_void());
    }

    stmt_return() = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x86::comment_source(tc, *this, os, indent);

        // get the jump target to exit function
        const std::string_view ret_lbl{tc.get_func_return_label()};

        if (ret_lbl.empty()) {
            // note: return from 'main' is exiting
            x86::mov(tc, tok(), os, indent, "rdi", "0");
            x86::mov(tc, tok(), os, indent, "rax", "60");
            x86::syscall(tc, os, indent);
            return;
        }

        // jump to return labels
        x86::jmp(tc, os, indent, ret_lbl);
    }

    [[nodiscard]] auto is_code_after_this_unreachable() const -> bool override {
        return true;
    }
};
