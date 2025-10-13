#pragma once
// reviewed: 2025-09-28

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

    ~stmt_loop() override = default;

    stmt_loop() = default;
    stmt_loop(const stmt_loop&) = default;
    stmt_loop(stmt_loop&&) = default;
    auto operator=(const stmt_loop&) -> stmt_loop& = default;
    auto operator=(stmt_loop&&) -> stmt_loop& = default;

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {

        tc.comment_token(tok(), os, indent);

        const std::string lbl{tc.create_unique_label(tok(), "loop")};
        toc::asm_label(tok(), os, indent, lbl);
        tc.enter_loop(lbl);
        code_.compile(tc, os, indent, dst);
        toc::asm_jmp(tok(), os, indent, lbl);
        toc::asm_label(tok(), os, indent, std::format("{}_end", lbl));
        tc.exit_loop(lbl);
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        code_.source_to(os);
    }

    [[nodiscard]] auto code() const -> const stmt_block& { return code_; }
};
