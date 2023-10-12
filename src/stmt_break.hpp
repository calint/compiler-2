#pragma once

class stmt_break final : public statement {
public:
  inline stmt_break(toc &tc, const token &tk) : statement{{}, tk} {
    set_type(tc.get_type_void());
  }

  inline stmt_break() = default;
  inline stmt_break(const stmt_break &) = default;
  inline stmt_break(stmt_break &&) = default;
  inline auto operator=(const stmt_break &) -> stmt_break & = default;
  inline auto operator=(stmt_break &&) -> stmt_break & = default;

  inline ~stmt_break() override = default;

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.source_comment(*this, os, indent);
    // get loop label
    const string &loop_label{tc.get_loop_label_or_throw(*this)};
    // jump out of the loop
    toc::asm_jmp(*this, os, indent, loop_label + "_end");
  }
};
