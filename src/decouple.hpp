#pragma once
// solves circular references
// implemented in 'main.cpp'

// reviewed: 2025-09-28

#include <memory>

class toc;
class tokenizer;
class token;
class statement;
class stmt_identifier;
class type;
class expr_any;

//
// functions necessary to solve circular references implemented in 'main.cpp'
//

inline auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz, token tk)
    -> std::unique_ptr<statement>;

inline auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si)
    -> std::unique_ptr<statement>;

inline auto create_expr_any(toc& tc, tokenizer& tz, const type& tp,
                            bool in_args) -> std::unique_ptr<expr_any>;
