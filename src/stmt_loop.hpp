#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"

class stmt_loop final : public statement {
    stmt_block code_;

  public:
    stmt_loop(toc& tc, token tk, tokenizer& tz) : statement{tk} {
        set_type(tc.get_type_void());
        const std::string lbl{tc.create_unique_label(tok(), "loop")};
        tc.enter_loop(lbl);
        code_ = {tc, tz};
        tc.exit_loop(lbl);
    }

    stmt_loop() = default;

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x86& x{tc.machine()};

        x.comment(tok(), indent, "label");

        const std::string lbl{tc.create_unique_label(tok(), "loop")};
        x.label(indent, lbl);
        tc.enter_loop(lbl);
        code_.compile(tc, indent, dst_info);
        x.jmp(indent, lbl);
        x.label(indent, std::format("{}_end", lbl));
        tc.exit_loop(lbl);
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        code_.source_to(os);
    }

    [[nodiscard]] auto
    is_var_set([[maybe_unused]] const std::string_view var) const
        -> bool override {

        return code_.is_var_set(var);
    }
};
