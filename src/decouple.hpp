#pragma once
// solves circular references
// implemented in 'main.cpp'

// reviewed: 2025-09-28

#include <memory>

#include "token.hpp"

class toc;
class tokenizer;
class statement;
class unary_ops;
class token;
class type;
class expr_any;
class stmt_identifier;
class expr_any;

struct identifier_elem final {
    token name_tk;
    std::unique_ptr<expr_any> array_index_expr;
    token ws1;
    bool has_array_index_expr{};
};

class identifier_path final {
  public:
    std::vector<identifier_elem> elems;
};

inline auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz, token tk)
    -> std::unique_ptr<statement>;

inline auto create_expr_any(toc& tc, tokenizer& tz, const type& tp,
                            bool in_args) -> std::unique_ptr<expr_any>;

inline auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si)
    -> std::unique_ptr<statement>;
