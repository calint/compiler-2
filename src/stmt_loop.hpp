#pragma once
// reviewed: 2025-09-28

#include "statement.hpp"
#include "stmt_block.hpp"

class stmt_loop final : public statement {
    stmt_block code_;

    [[nodiscard]] auto create_unique_label(const toc& tc) const -> std::string {
        const std::string_view call_path{tc.get_call_path(tok())};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string lbl{
            std::format("loop_{}{}", src_loc,
                        (call_path.empty() ? std::string{}
                                           : std::format("_{}", call_path)))};
        return lbl;
    }

  public:
    stmt_loop(toc& tc, token tk, tokenizer& tz) : statement{tk} {
        set_type(tc.get_type_void());
        const std::string lbl{create_unique_label(tc)};
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

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] std::string_view dst) const -> void override {

        toc::indent(os, indent, true);
        tc.comment_token(os, tok());

        const std::string lbl{create_unique_label(tc)};
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
};
