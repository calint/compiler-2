#pragma once

#include "statement.hpp"
#include "stmt_block.hpp"

class stmt_loop final : public statement {
    stmt_block code_{};

  public:
    inline stmt_loop(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)} {
        set_type(tc.get_type_void());
        const std::string& lbl{"loop_" +
                               tc.source_location_for_use_in_label(tok())};
        tc.enter_loop(lbl);
        code_ = {tc, tz};
        tc.exit_loop(lbl);
    }

    inline stmt_loop() = default;
    inline stmt_loop(const stmt_loop&) = default;
    inline stmt_loop(stmt_loop&&) = default;
    inline auto operator=(const stmt_loop&) -> stmt_loop& = default;
    inline auto operator=(stmt_loop&&) -> stmt_loop& = default;

    inline ~stmt_loop() override = default;

    inline void
    compile(toc& tc, std::ostream& os, size_t indent,
            [[maybe_unused]] const std::string& dst = "") const override {

        toc::indent(os, indent, true);
        tc.comment_token(os, tok());

        // make unique label for this loop considering in-lined functions
        //   current path of source locations where in-lined functions have been
        //     called
        const std::string& call_path{tc.get_inline_call_path(tok())};
        // current source location
        const std::string& src_loc{tc.source_location_for_use_in_label(tok())};
        // the loop label
        const std::string& lbl{"loop_" + (call_path.empty()
                                              ? src_loc
                                              : (src_loc + "_" + call_path))};
        toc::asm_label(tok(), os, indent, lbl);
        // enter loop scope
        tc.enter_loop(lbl);
        // compile loop body
        code_.compile(tc, os, indent);
        // jump to loop start after body
        toc::asm_jmp(tok(), os, indent, lbl);
        // exit loop label
        toc::asm_label(tok(), os, indent, lbl + "_end");
        // exit loop scope
        tc.exit_loop(lbl);
    }

    inline void source_to(std::ostream& os) const override {
        statement::source_to(os);
        code_.source_to(os);
    }
};
