#pragma once

#include"unary_ops.hpp"

inline unique_ptr<statement>create_statement_from_tokenizer(tokenizer&t);
inline unique_ptr<statement>create_statement_from_tokenizer(token tk,unary_ops uops,tokenizer&t);
