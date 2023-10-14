#pragma once

class stmt_def_type_field final : public statement {
  token type_tk_{};

public:
  inline stmt_def_type_field(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)} {

    // the type of 'stmt_def_type_field' is void
    set_type(tc.get_type_void());

    // if type is not specified it is assumed the default type
    if (not tz.is_next_char(':')) {
      return;
    }
    // get type name
    type_tk_ = tz.next_token();
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
    if (type_tk_.is_empty()) {
      return;
    }
    os << ':';
    type_tk_.source_to(os);
  }

  [[nodiscard]] inline auto name() const -> const string & {
    return tok().name();
  }

  [[nodiscard]] inline auto type_str() const -> const string & {
    return type_tk_.name();
  }
};
