#pragma once

// the declarations that break include cycles, defined in 'parse_statement.hpp'
// and 'expr_type_impl.hpp':
//   - the statement factories create the statements of a block and the
//     elements of an expression, and the statement headers that need them
//     ('stmt_block', 'expr_arith') are included by those classes
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
class type;
class unary_ops;

// an element of an arithmetic expression after its unary ops 'uops', which
// the caller read; 'destination_type' is the type a generic call takes for its
// result, null when not known
[[nodiscard]] auto parse_operand(toc& tc, tokenizer& tz, unary_ops uops,
                                 const type* destination_type)
    -> std::unique_ptr<statement>;

// a statement of a block, 'tk' its first token
[[nodiscard]] auto parse_statement(toc& tc, tokenizer& tz, const token tk)
    -> std::unique_ptr<statement>;
