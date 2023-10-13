#pragma once

class stmt_comment final : public statement {
  string line_{};

public:
  inline stmt_comment(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, line_{tz.read_rest_of_line()} {

    set_type(tc.get_type_void());
  }

  inline stmt_comment() = default;
  inline stmt_comment(const stmt_comment &) = default;
  inline stmt_comment(stmt_comment &&) = default;
  inline auto operator=(const stmt_comment &) -> stmt_comment & = default;
  inline auto operator=(stmt_comment &&) -> stmt_comment & = default;

  inline ~stmt_comment() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    os << line_ << endl;
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.comment_source(*this, os, indent);
  }
};
