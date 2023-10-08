#pragma once
#include "unary_ops.hpp"
#include "statement.hpp"

inline unique_ptr<statement> create_statement_from_tokenizer(toc &tc,
                                                             tokenizer &t);
inline unique_ptr<statement> create_statement_from_tokenizer(toc &tc, token tk,
                                                             unary_ops uops,
                                                             tokenizer &t);
