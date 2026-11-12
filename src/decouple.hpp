#pragma once

// the declarations that break include cycles:
//   - the parse functions that 'stmt_block' and 'expr_arith' call, which
//     make the statements of a block and the elements of an expression,
//     defined in 'parse_statement.hpp', which includes every statement
//   - the class 'expr_type' holds, 'expr_any', whose header includes
//     'expr_type.hpp'; the members of 'expr_type' that use it and the parse
//     of an instance value ('expr_any::parse_instance'), which makes a
//     'stmt_call', are in 'expr_type_impl.hpp'

#include <memory>

#include "token.hpp"

class parse_toc;
class tokenizer;
class statement;
class expr_any;
class type;
class unary_ops;

// an element of an arithmetic expression after its unary ops 'uops', which
// the caller read; 'destination_type' is the type a generic call takes for its
// result, null when not known
[[nodiscard]] auto parse_operand(parse_toc& tc, tokenizer& tz, unary_ops uops,
                                 const type* destination_type)
    -> std::unique_ptr<statement>;

// a statement of a block, 'tk' its first token
[[nodiscard]] auto parse_statement(parse_toc& tc, tokenizer& tz,
                                   const token& tk)
    -> std::unique_ptr<statement>;
