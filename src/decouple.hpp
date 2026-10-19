#pragma once

// the declarations that break include cycles, defined in 'decouple_impl.hpp':
//   - the statement factories create every statement class, and the
//     statement headers that need them ('stmt_block', 'expr_arith') are
//     included by those classes
//   - 'expr_type' holds the classes below, whose headers include
//     'expr_type.hpp' through 'expr_any.hpp'

#include <memory>

#include "token.hpp"

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class stmt_call;
class expr_any;

[[nodiscard]] auto create_statement_in_expr_arith(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                  const token tk)
    -> std::unique_ptr<statement>;

// a statement that starts with a name: a call or an assignment
[[nodiscard]] auto create_statement_of_identifier(toc& tc, tokenizer& tz,
                                                  const token tk)
    -> std::unique_ptr<statement>;
