#pragma once
#include "unary_ops.hpp"

// solves circular references
// implemented in main.cpp

class expr_any;
class statement;
class tokenizer;
class unary_ops;

inline auto create_statement_from_tokenizer(toc &tc, tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_statement_from_tokenizer(toc &tc, token tk, unary_ops &uops,
                                            tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_expr_any_from_tokenizer(toc &tc, tokenizer &tz,
                                           const type &tp)
    -> unique_ptr<expr_any>;
