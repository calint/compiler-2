#pragma once
#include "stmt_def_type_field.hpp"

class stmt_def_type final : public statement {
  token name_{};
  token ws_{};
  vector<stmt_def_type_field> fields_{};
  type type_{};

public:
  inline stmt_def_type(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, name_{tz.next_token()} {

    if (not tz.is_next_char('{')) {
      throw compiler_exception(name_, "expected '{'");
    }

    while (true) {
      const token tknm{tz.next_token()};
      if (tknm.is_name("")) {
        ws_ = tknm;
        if (not tz.is_next_char('}')) {
          throw compiler_exception(tz, "expected '}'");
        }
        break;
      }
      fields_.emplace_back(tc, tknm, tz);
      if (tz.is_next_char('}')) {
        break;
      }
      if (not tz.is_next_char(',')) {
        throw compiler_exception(tz, "expected ',' and more fields");
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
    tc.add_type(name_, type_);
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
};
