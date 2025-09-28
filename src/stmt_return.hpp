#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "toc.hpp"

class stmt_return final : public statement {
  public:
    stmt_return(toc& tc, token tk) : statement{std::move(tk)} {
        set_type(tc.get_type_void());
    }

    stmt_return() = default;
    stmt_return(const stmt_return&) = default;
    stmt_return(stmt_return&&) = default;
    auto operator=(const stmt_return&) -> stmt_return& = default;
    auto operator=(stmt_return&&) -> stmt_return& = default;

    ~stmt_return() override = default;

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.comment_source(*this, os, indent);

        // get the jump target to exit function
        const std::string& ret_lbl{tc.get_func_return_label_or_throw(tok())};

        if (ret_lbl.empty()) {
            // note: return from 'main' is exit
            os << "mov rdi, 0\nmov rax, 60\nsyscall\n";
            return;
        }

        // check that the return value has been assigned
        const std::vector<func_return_info>& returns{
            tc.get_func_returns(tok())};
        if (not returns.empty()) {
            const std::string& ret_var{returns.at(0).ident_tk.name()};
            if (not tc.is_var_initiated(ret_var)) {
                throw compiler_exception(tok(), "return variable '" + ret_var +
                                                    "' has not been assigned");
            }
        }
        // jump to return labels
        toc::asm_jmp(tok(), os, indent, ret_lbl);
    }
};
