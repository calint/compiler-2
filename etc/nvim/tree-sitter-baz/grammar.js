module.exports = grammar({
  name: 'baz',

  // Treat whitespace and comments as ignorable extras that can appear anywhere
  extras: $ => [
    /\s/,
    $.comment,
  ],

  // Root of the program structure
  rules: {
    program: $ => repeat($._definition),

    _definition: $ => choice(
      $.field_definition,
      $.function_definition,
      $.type_definition,
    ),

    // --- Definitions (Top-Level Declarations) ---

    // field identifier = expression
    field_definition: $ => seq(
      $.field_keyword,
      field('name', $.identifier),
      '=',
      $._expression,
    ),

    // type Identifier { member_field_list }
    type_definition: $ => seq(
      $.type_keyword,
      field('name', $.identifier),
      '{',
      optional($.member_field_list),
      '}',
    ),

    member_field_list: $ => sep1($.member_field, ','),

    // member_field name: type
    member_field: $ => seq(
      field('name', $.identifier),
      optional(seq(':', $._definition_type)),
    ),

    // func identifier ( parameters ) return_annotation body
    function_definition: $ => seq(
      $.func_keyword,
      field('name', $.identifier),
      '(',
      optional($.parameter_list), // List is optional (avoids runtime error for ())
      ')',
      optional($.return_annotation),
      $._function_body, // Now uses the flexible body rule
    ),

    return_annotation: $ => seq(
      ':',
      $._definition_type,
      field('return_name', $.identifier)
    ),

    // parameter_list requires 1 or more elements
    parameter_list: $ => sep1($.parameter, ','),

    // identifier : type_name (type is optional)
    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(':', $._parameter_type)),
    ),

    // --- Types ---

    _definition_type: $ => choice(
      $._base_type,
      $.sized_array_type,
    ),

    _parameter_type: $ => choice(
      $._base_type,
      $.unsized_array_type,
    ),

    _base_type: $ => choice(
      $.bool_type, $.i8_type, $.i16_type, $.i32_type, $.i64_type,
      $.identifier
    ),

    sized_array_type: $ => seq(
      $._base_type,
      '[',
      $.number_literal,
      ']',
    ),

    unsized_array_type: $ => seq(
      $._base_type,
      '[',
      ']',
    ),

    // --- Function Body and Statements ---

    // Allows either a block or a single statement for function body.
    _function_body: $ => choice(
      prec.right(1, $.block), // Prefer parsing a block as the explicit function body (resolves conflict)
      $._statement,           // Allow a single statement (e.g., `if not b exit(1)`)
    ),
    
    // Allows either a block or a single statement for control flow bodies
    _body: $ => choice(
      prec.right(1, $.block), // RESOLUTION: Prefer parsing block directly (resolves conflict with _statement)
      $._statement,
    ),

    block: $ => seq(
      '{',
      repeat($._statement),
      '}',
    ),

    _statement: $ => choice(
      // Structural Block (Allows nesting for scope)
      $.block,
      // Data/State management
      $.variable_declaration,
      $.assignment_statement,
      // Actions
      $.function_call,
      $.return_statement,
      // Control Flow
      $.if_statement,
      $.loop_statement,
      // Loop Control
      $.break_statement,
      $.continue_statement,
    ),

    // var identifier : type = expression
    variable_declaration: $ => seq(
      $.var_keyword,
      field('destination', $.identifier),
      optional(seq(':', $._definition_type)),
      // Initializer is optional
      optional(seq('=', $._expression)),
    ),

    // access_chain = expression
    assignment_statement: $ => seq(
      field('destination', $._access_chain),
      '=',
      $._expression,
    ),

    // return (no arguments)
    return_statement: $ => $.return_keyword,

    // Statements for loop control
    break_statement: $ => $.break_keyword,
    continue_statement: $ => $.continue_keyword,

    // function_call identifier ( arguments )
    function_call: $ => seq(
      field('function', $.identifier),
      '(',
      optional($.argument_list),
      ')',
    ),

    // argument_list requires 1 or more elements
    argument_list: $ => sep1($._expression, ','),

    // Full if/else if/else structure
    if_statement: $ => prec.right(1, seq( // Precedence to resolve dangling else conflict
      $.if_keyword,
      $._expression,
      field('consequence', $._body), // Now uses flexible body
      repeat($.else_if_clause),
      optional($.else_clause),
    )),
    
    // else if clause
    else_if_clause: $ => seq(
        $.else_if_keyword, // Use the custom token
        $._expression,
        field('consequence', $._body), // Now uses flexible body
    ),

    // else clause (FIXED: Inlining _body logic to resolve token boundary error)
    else_clause: $ => seq(
        $.else_keyword,
        field('alternative', choice(
          prec.right(1, $.block), // Prioritize block if present
          $._statement,          // Otherwise, look for a single statement
        )),
    ),

    loop_statement: $ => seq(
      $.loop_keyword,
      field('body', $._body), // Now uses flexible body
    ),

    // --- Expressions and Access (Ordered by Precedence, Highest to Lowest) ---

    _expression: $ => choice(
      $._literal,
      $._access_chain,
      $.function_call,
      $.initializer_block,
      $.parenthesized_expression,
      
      // Precedence 16: Unary Arithmetic/Bitwise (~, -)
      $.unary_expression, 
      
      // Precedence 15: Unary Logic (not)
      $.not_expression, 
      
      // Precedence 12: Multiplicative/Modulo (*, /, %)
      $.multiplicative_expression, 
      
      // Precedence 11: Additive (+, -)
      $.additive_expression,       
      
      // Precedence 10: Shift (<<, >>)
      $.shift_expression,          
      
      // Precedence 9: Bitwise AND (&)
      $.bitwise_and_expression,    
      
      // Precedence 8: Bitwise OR (|)
      $.bitwise_or_expression,     
      
      // Precedence 7: Logical AND (and)
      $.and_expression,            
      
      // Precedence 6: Logical OR (or)
      $.or_expression,             
      
      // Precedence 5: Comparison (==, !=, <=, <, >, >=)
      $.comparison_expression,     
    ),

    // Precedence 16
    unary_expression: $ => prec(16, seq(
        field('operator', choice('-', '~')),
        field('operand', $._expression),
    )),
    
    // Precedence 15
    not_expression: $ => prec(15, seq(
        $.not_keyword,
        $._expression,
    )),

    // Precedence 12 (Left Associative)
    multiplicative_expression: $ => prec.left(12, seq(
        field('left', $._expression),
        field('operator', choice('*', '/', '%')),
        field('right', $._expression),
    )),
    
    // Precedence 11 (Left Associative)
    additive_expression: $ => prec.left(11, seq(
        field('left', $._expression),
        field('operator', choice('+', '-')),
        field('right', $._expression),
    )),
    
    // Precedence 10 (Left Associative)
    shift_expression: $ => prec.left(10, seq(
        field('left', $._expression),
        field('operator', choice('<<', '>>')),
        field('right', $._expression),
    )),
    
    // Precedence 9 (Left Associative)
    bitwise_and_expression: $ => prec.left(9, seq(
        field('left', $._expression),
        field('operator', '&'),
        field('right', $._expression),
    )),
    
    // Precedence 8 (Left Associative)
    bitwise_or_expression: $ => prec.left(8, seq(
        field('left', $._expression),
        field('operator', '|'),
        field('right', $._expression),
    )),

    // Precedence 7 (Left Associative)
    and_expression: $ => prec.left(7, seq(
        field('left', $._expression),
        field('operator', $.and_keyword),
        field('right', $._expression),
    )),

    // Precedence 6 (Left Associative)
    or_expression: $ => prec.left(6, seq(
        field('left', $._expression),
        field('operator', $.or_keyword),
        field('right', $._expression),
    )),

    // Precedence 5 (Left Associative)
    comparison_expression: $ => prec.left(5, seq(
        field('left', $._expression),
        field('operator', $.comparison_operator),
        field('right', $._expression),
    )),

    // Structure or Array positional initialization block
    initializer_block: $ => seq(
      '{',
      optional($.initializer_list),
      '}',
    ),

    // List of expressions used for positional initialization
    initializer_list: $ => sep1($._expression, ','),

    comparison_operator: $ => choice(
        '==',
        '!=',
        '<=',
        '<',
        '>',
        '>=',
    ),

    _access_chain: $ => seq(
      $.identifier,
      repeat(choice(
        $.member_access,
        $.array_indexing,
      )),
    ),

    member_access: $ => seq(
      '.',
      $.identifier,
    ),

    array_indexing: $ => seq(
      '[',
      $._expression,
      ']',
    ),

    parenthesized_expression: $ => seq(
      '(',
      $._expression,
      ')',
    ),

    // --- Tokens (Keywords, Identifiers, Literals) ---

    // Definition Keywords
    field_keyword: $ => 'field',
    func_keyword: $ => 'func',
    type_keyword: $ => 'type',

    // Statement Keywords
    var_keyword: $ => 'var',
    return_keyword: $ => 'return',
    if_keyword: $ => 'if',
    loop_keyword: $ => 'loop',
    
    // Flow Control Keywords
    else_if_keyword: $ => prec(1, seq('else', /\s+/, 'if')),
    else_keyword: $ => 'else',
    break_keyword: $ => 'break',
    continue_keyword: $ => 'continue',

    // Logical Keywords
    not_keyword: $ => 'not',
    and_keyword: $ => 'and',
    or_keyword: $ => 'or',

    // Type Keywords
    bool_type: $ => 'bool',
    i8_type: $ => 'i8',
    i16_type: $ => 'i16',
    i32_type: $ => 'i32',
    i64_type: $ => 'i64',

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    _literal: $ => choice(
      $.string_literal,
      $.number_literal,
      $.boolean_literal,
    ),
    
    // Literal definitions
    boolean_literal: $ => choice(
        'true',
        'false',
    ),

    string_literal: $ => seq(
      '"',
      repeat(/[^"\n]/),
      '"',
    ),

    number_literal: $ => choice(
      /0x[0-9a-fA-F]+/, // Hex
      /0b[01]+/,        // Binary
      /\d+(\.\d+)?/,    // Decimal (int or float)
    ),

    comment: $ => /#.*/,
  }
});

// Helper function for separated lists that must have at least one element
function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
