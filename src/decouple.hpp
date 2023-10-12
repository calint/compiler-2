#pragma once

// solves circular references
// implemented in main.cpp

class toc;
class expr_any;
class statement;
class tokenizer;
class unary_ops;
class type;
class unary_ops;
class token;

inline auto create_statement_from_tokenizer(toc &tc, tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_statement_from_tokenizer(toc &tc, const unary_ops &uops,
                                            const token &tk, tokenizer &tz)
    -> unique_ptr<statement>;

inline auto create_expr_any_from_tokenizer(toc &tc, tokenizer &tz,
                                           const type &tp, bool in_args)
    -> unique_ptr<expr_any>;
