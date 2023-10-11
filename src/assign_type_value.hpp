#pragma once
#include "decouple.hpp"
#include "statement.hpp"
#include "type.hpp"

class expr_any;

class assign_type_value final : public statement {
  vector<unique_ptr<expr_any>> exprs_{};

public:
  inline assign_type_value(toc &tc, tokenizer &tz, const type &tp)
      : statement{tz.next_token()} {

    set_type(tp);

    if (not tok().is_name("")) {
      // e.g. obj.pos = p
      return;
    }

    // e.g. obj.pos = {x, y}
    if (not tz.is_next_char('{')) {
      throw compiler_exception(tz, "expected '{' to open assign type '" +
                                       tp.name() + "'");
    }

    const vector<type_field> &flds{tp.fields()};
    const size_t nflds{flds.size()};
    for (size_t i{0}; i < nflds; i++) {
      const type_field &fld{flds[i]};
      exprs_.emplace_back(create_expr_any_from_tokenizer(tc, tz, fld.tp, true));
      if (i < nflds - 1) {
        if (not tz.is_next_char(',')) {
          throw compiler_exception(tz, "expected ',' and value of field '" +
                                           flds[i + 1].name + "' in type '" +
                                           tp.name() + "'");
        }
      }
    }

    if (not tz.is_next_char('}')) {
      throw compiler_exception(tz, "expected '}' to close assign type '" +
                                       tp.name() + "'");
    }
  }

  // implemented in main.cpp
  inline void source_to(ostream &os) const override;
  inline void compile(toc &tc, ostream &os, size_t indent,
                      const string &dst = "") const override;

private:
  inline static void compile_recursive(const assign_type_value &atv, toc &tc,
                                       ostream &os, size_t indent,
                                       const string &src, const string &dst,
                                       const type &dst_type);
};
