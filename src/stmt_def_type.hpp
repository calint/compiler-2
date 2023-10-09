#pragma once

class stmt_def_type_field final : public statement {
public:
  inline stmt_def_type_field(toc &tc, token tk, tokenizer &t)
      : statement{move(tk)} {
    set_type(tc.get_type_void());

    if (not t.is_next_char(':')) {
      return;
    }
    type_ = t.next_token();
  }

  inline stmt_def_type_field() = default;
  inline stmt_def_type_field(const stmt_def_type_field &) = default;
  inline stmt_def_type_field(stmt_def_type_field &&) = default;
  inline auto operator=(const stmt_def_type_field &)
      -> stmt_def_type_field & = default;
  inline auto operator=(stmt_def_type_field &&)
      -> stmt_def_type_field & = default;

  inline ~stmt_def_type_field() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    if (type_.is_empty()) {
      return;
    }
    os << ':';
    type_.source_to(os);
  }

  [[nodiscard]] inline auto name() const -> const string & {
    return tok().name();
  }

  [[nodiscard]] inline auto type_str() const -> const string & {
    return type_.name();
  }

private:
  token type_{};
};

class stmt_def_type final : public statement {
public:
  inline stmt_def_type(toc &tc, token tk, tokenizer &t)
      : statement{move(tk)}, name_{t.next_token()} {
    if (not t.is_next_char('{')) {
      throw compiler_exception(name_, "expected '{'");
    }

    while (true) {
      token tknm{t.next_token()};
      if (tknm.is_name("")) {
        ws_ = tknm;
        if (not t.is_next_char('}')) {
          throw compiler_exception(t, "expected '}'");
        }
        break;
      }
      fields_.emplace_back(tc, move(tknm), t);
      if (t.is_next_char('}')) {
        break;
      }
      if (not t.is_next_char(',')) {
        throw compiler_exception(t, "expected ',' and more fields");
      }
    }

    type_.set_name(name_.name());
    if (fields_.empty()) {
      type_.set_size(tc.get_type_default().size());
    } else {
      for (const stmt_def_type_field &fld : fields_) {
        const type &tp{fld.type_str().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(fld, fld.type_str())};
        type_.add_field(fld.tok(), fld.name(), tp);
      }
    }
    tc.add_type(*this, type_);
  }

  inline stmt_def_type() = default;
  inline stmt_def_type(const stmt_def_type &) = default;
  inline stmt_def_type(stmt_def_type &&) = default;
  inline auto operator=(const stmt_def_type &) -> stmt_def_type & = default;
  inline auto operator=(stmt_def_type &&) -> stmt_def_type & = default;

  inline ~stmt_def_type() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    name_.source_to(os);
    os << '{';
    ws_.source_to(os);
    size_t i{0};
    const size_t n{fields_.size()};
    for (const stmt_def_type_field &tf : fields_) {
      tf.source_to(os);
      if (i < n - 1) {
        os << ',';
      }
      i++;
    }
    os << '}';
  }

  inline void compile([[maybe_unused]] toc &tc, [[maybe_unused]] ostream &os,
                      [[maybe_unused]] size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {}

private:
  token name_{};
  token ws_{};
  vector<stmt_def_type_field> fields_{};
  type type_{};
};
