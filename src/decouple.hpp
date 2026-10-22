#pragma once

// the declarations that break include cycles, defined in 'decouple_impl.hpp'
// and 'expr_type_impl.hpp':
//   - the statement factories create the statements of a block and the
//     elements of an expression, and the statement headers that need them
//     ('stmt_block', 'expr_arith') are included by those classes
//   - 'expr_type' holds the classes below, whose headers include
//     'expr_type.hpp' through 'expr_any.hpp'
//   - 'stmt_call' deduces a type argument from the value of a built-in, whose
//     statement headers include 'stmt_call.hpp'

#include <memory>

#include "token.hpp"

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class stmt_call;
class expr_any;
class type;

[[nodiscard]] auto create_statement_in_expr_arith(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                  const token tk)
    -> std::unique_ptr<statement>;

// a statement that starts with a name: a call or an assignment
[[nodiscard]] auto create_statement_of_identifier(toc& tc, tokenizer& tz,
                                                  const token tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto builtin_value_type(const toc& tc, const token& tk,
                                      tokenizer& tz) -> const type*;
