#pragma once

#include "stmt_call.hpp"

class call_asm : public stmt_call {
public:
  inline call_asm(toc &tc, token tk, tokenizer &t)
      : stmt_call{tc, move(tk), {}, t} {
    set_type(tc.get_type_void());
  }

  inline call_asm() = default;
  inline call_asm(const call_asm &) = default;
  inline call_asm(call_asm &&) = default;
  inline call_asm &operator=(const call_asm &) = default;
  inline call_asm &operator=(call_asm &&) = default;

  inline ~call_asm() override = default;
};
