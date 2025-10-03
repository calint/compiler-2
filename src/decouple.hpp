#pragma once
// solves circular references
// implemented in 'main.cpp'

// reviewed: 2025-09-28

#include <memory>

class toc;
class tokenizer;
class statement;
class unary_ops;
class token;
class type;
class expr_any;
class stmt_identifier;

inline auto create_statement(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

inline auto create_statement_from_tokenizer(toc& tc, tokenizer& tz, token tk)
    -> std::unique_ptr<statement>;

inline auto create_expr_any(toc& tc, tokenizer& tz, const type& tp,
                            bool in_args) -> std::unique_ptr<expr_any>;

inline auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si)
    -> std::unique_ptr<statement>;
