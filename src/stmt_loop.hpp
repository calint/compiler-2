#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"

class stmt_loop final : public statement {
    stmt_block code_;

  public:
    stmt_loop(toc& tc, const token tk, tokenizer& tz) : statement{tk} {
        set_type(tc.get_type_void());
        const std::string lbl{tc.create_unique_label(tok(), "loop")};
        tc.enter_loop(lbl);
        code_ = {tc, tz};
        tc.exit_loop(lbl);
    }

    stmt_loop() = default;

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, "label");

        const std::string lbl{tc.create_unique_label(tok(), "loop")};
        x.label(indent, lbl);
        tc.enter_loop(lbl);
        code_.compile(tc, indent, dst_info);
        x.branch(indent, lbl);
        x.label(indent, std::format("{}_end", lbl));
        tc.exit_loop(lbl);
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        code_.source_to(os);
    }

    [[nodiscard]] auto is_var_set(const std::string_view var) const
        -> bool override {

        // the loop completes only through its own 'break'
        if (code_.may_break_unset(var)) {
            return false;
        }

        return code_.is_var_set(var);
    }

    [[nodiscard]] auto may_return_unset(const std::string_view var) const
        -> bool override {

        return code_.may_return_unset(var);
    }

    // 'may_break_unset' stays false because breaks in 'code_' end this loop

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        code_.assert_var_not_used(var);
    }
};
