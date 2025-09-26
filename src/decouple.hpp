#pragma once

// solves circular references
// implemented in main.cpp

class toc;
class tokenizer;
class statement;
class unary_ops;
class token;
class type;
class expr_any;

inline auto create_statement_from_tokenizer(toc &tc, tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_statement_from_tokenizer(toc &tc, unary_ops uops, token tk,
                                            tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_expr_any_from_tokenizer(toc &tc, tokenizer &tz,
                                           const type &tp, bool in_args)
    -> unique_ptr<expr_any>;
