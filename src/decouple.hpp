#pragma once
#include "unary_ops.hpp"
#include "statement.hpp"

inline auto create_statement_from_tokenizer(toc &tc,
                                                             tokenizer &t) -> unique_ptr<statement>;
inline auto create_statement_from_tokenizer(toc &tc, token tk,
                                                             unary_ops uops,
                                                             tokenizer &t) -> unique_ptr<statement>;
