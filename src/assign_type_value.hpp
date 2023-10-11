#pragma once
#include "statement.hpp"
#include "type.hpp"
#include "decouple.hpp"

class expr_any;

class assign_type_value final : public statement {
  vector<unique_ptr<expr_any>> exprs_{};

public:
  inline assign_type_value(toc & /*tc*/, tokenizer &tz, const type &tp)
      : statement{tz.next_whitespace_token()} {

    for (const type_field &fld : tp.fields()) {
      (void)fld;
      //   exprs_.emplace_back(tc, fld.tp, tz, true);
    }
  }
};
