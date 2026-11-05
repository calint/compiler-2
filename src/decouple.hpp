#pragma once

// the declarations that break include cycles:
//   - the parse functions that 'stmt_block' and 'expr_arith' call, which
//     make the statements of a block and the elements of an expression,
//     defined in 'parse_statement.hpp', which includes every statement
//   - the classes 'expr_type' holds, 'stmt_call' and 'expr_any', whose
//     headers include 'expr_type.hpp'; its members that use them are in
//     'expr_type_impl.hpp'

#include <memory>

#include "token.hpp"

class toc;
class tokenizer;
class statement;
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
