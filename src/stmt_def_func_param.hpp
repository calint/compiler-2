#pragma once

class stmt_def_func_param final : public statement {
public:
  inline stmt_def_func_param(toc &tc, tokenizer &t)
      : statement{t.next_token()} {
    assert(not tok().is_name(""));

    if (not t.is_next_char(':')) {
      set_type(tc.get_type_default());
      return;
    }

    while (true) {
      keywords_.emplace_back(t.next_token());
      if (t.is_next_char(':'))
        continue;
      break;
    }

    for (const token &kw : keywords()) {
      if (kw.name().starts_with("reg_"))
        continue;
      set_type(tc.get_type_or_throw(*this, kw.name()));
      return;
    }

    set_type(tc.get_type_default());
  }

  inline stmt_def_func_param() = default;
  inline stmt_def_func_param(const stmt_def_func_param &) = default;
  inline stmt_def_func_param(stmt_def_func_param &&) = default;
  inline stmt_def_func_param &operator=(const stmt_def_func_param &) = default;
  inline stmt_def_func_param &operator=(stmt_def_func_param &&) = default;

  inline ~stmt_def_func_param() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    if (not keywords_.empty()) {
      os << ":";
      const size_t n = keywords_.size() - 1;
      size_t i{0};
      for (const token &t : keywords_) {
        t.source_to(os);
        if (i++ != n)
          os << ":";
      }
    }
  }

  inline const vector<token> &keywords() const { return keywords_; }

  inline const string &name() const { return tok().name(); }

  inline string get_register_or_empty() const {
    for (const token &kw : keywords()) {
      if (!kw.name().starts_with("reg_"))
        continue;
      // requested register for this argument
      return kw.name().substr(4, kw.name().size());
    }
    return "";
  }

private:
  vector<token> keywords_{};
};
