#pragma once
#include "statement.hpp"
#include "unary_ops.hpp"

inline auto create_statement_from_tokenizer(toc &tc, tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_statement_from_tokenizer(toc &tc, token tk, unary_ops uops,
                                            tokenizer &t)
    -> unique_ptr<statement>;
