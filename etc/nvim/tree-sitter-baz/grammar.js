module.exports = grammar({
  name: 'baz',

  // Treat whitespace and comments as ignorable extras that can appear anywhere
  extras: $ => [
    /\s/,
    $.comment,
  ],

  // Helper function for separated lists that must have at least one element
  // Placed outside rules for clean top-level declaration
  // ---

  rules: {
    // -------------------------------------------------------------------------
    // 1. PROGRAM ROOT & CORE STRUCTURES
    // -------------------------------------------------------------------------

    program: $ => repeat($._definition),

    // The primary entry point for control flow logic (used in if/loop bodies)
    _body: $ => choice(
      prec.right(1, $.block), // Prioritize block over single statement
      $._statement,
    ),

    // Body for a function, which can be a block or a single statement
    _function_body: $ => choice(
      prec.right(1, $.block), // Prefer parsing a block as the explicit function body
      $._statement,
    ),

    // A grouping of statements { ... }
    block: $ => seq(
      '{',
      repeat($._statement),
      '}',
    ),

    // A generalized rule for any valid statement
    _statement: $ => choice(
      // Structural Block
      $.block,
      // State Management
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

    // -------------------------------------------------------------------------
    // 2. TOP-LEVEL DEFINITIONS & DECLARATIONS
    // -------------------------------------------------------------------------

    _definition: $ => choice(
      $.field_definition,
      $.function_definition,
      $.type_definition,
    ),

    // field identifier = expression
    field_definition: $ => seq(
      $.field_keyword,
      field('name', $.identifier),
      '=',
      $._expression,
    ),

    // func identifier ( parameters ) return_annotation body
    function_definition: $ => seq(
      $.func_keyword,
      field('name', $.identifier),
      '(',
      optional($.parameter_list),
      ')',
      optional($.return_annotation),
      $._function_body,
    ),

    // type Identifier { member_field_list }
    type_definition: $ => seq(
      $.type_keyword,
      field('name', $.identifier),
      '{',
      optional($.member_field_list),
      '}',
    ),

    // Function/Type Metadata
    return_annotation: $ => seq(
      ':',
      $._definition_type,
      field('return_name', $.identifier)
    ),

    parameter_list: $ => sep1($.parameter, ','),

    // identifier : type_name (type is optional)
    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(':', $._parameter_type)),
    ),

    member_field_list: $ => sep1($.member_field, ','),

    // member_field name: type
    member_field: $ => seq(
      field('name', $.identifier),
      optional(seq(':', $._definition_type)),
    ),

    // -------------------------------------------------------------------------
    // 3. TYPE RULES
    // -------------------------------------------------------------------------

    // Types that can be defined in a field (allows sized arrays)
    _definition_type: $ => choice(
      $._base_type,
      $.sized_array_type,
    ),

    // Types that can be passed as a parameter (allows unsized arrays)
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

    // -------------------------------------------------------------------------
    // 4. STATEMENT IMPLEMENTATIONS
    // -------------------------------------------------------------------------

    // var identifier : type = expression
    variable_declaration: $ => seq(
      $.var_keyword,
      field('destination', $.identifier),
      optional(seq(':', $._definition_type)),
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

    break_statement: $ => $.break_keyword,
    continue_statement: $ => $.continue_keyword,

    // function_call identifier ( arguments )
    function_call: $ => seq(
      field('function', $.identifier),
      '(',
      optional($.argument_list),
      ')',
    ),

    argument_list: $ => sep1($._expression, ','),

    // Full if/else if/else structure (dangling else resolved with prec.right(1))
    if_statement: $ => prec.right(1, seq(
      $.if_keyword,
      $._expression,
      field('consequence', $._body),
      repeat($.else_if_clause),
      optional($.else_clause),
    )),

    // else if clause
    else_if_clause: $ => seq(
        $.else_if_keyword,
        $._expression,
        field('consequence', $._body),
    ),

    // else clause (inlining _body to resolve the token boundary error)
    else_clause: $ => seq(
        $.else_keyword,
        field('alternative', choice(
          prec.right(1, $.block),
          $._statement,
        )),
    ),

    loop_statement: $ => seq(
      $.loop_keyword,
      field('body', $._body),
    ),

    // -------------------------------------------------------------------------
    // 5. EXPRESSIONS & OPERATOR PRECEDENCE (Highest to Lowest)
    // -------------------------------------------------------------------------

    _expression: $ => choice(
      $._literal,
      $._access_chain,
      $.function_call,
      $.initializer_block,
      $.parenthesized_expression,
      $.unary_expression,          // Precedence 16: Unary Arithmetic/Bitwise (~, -)
      $.not_expression,            // Precedence 15: Unary Logic (not)
      $.multiplicative_expression, // Precedence 12: Multiplicative/Modulo (*, /, %)
      $.additive_expression,       // Precedence 11: Additive (+, -)
      $.shift_expression,          // Precedence 10: Shift (<<, >>)
      $.bitwise_and_expression,    // Precedence 9: Bitwise AND (&)
      $.bitwise_or_expression,     // Precedence 8: Bitwise OR (|)
      $.and_expression,            // Precedence 7: Logical AND (and)
      $.or_expression,             // Precedence 6: Logical OR (or)
      $.comparison_expression,     // Precedence 5: Comparison (==, !=, <=, <, >, >=)
    ),

    parenthesized_expression: $ => seq(
      '(',
      $._expression,
      ')',
    ),

    // Precedence 16
    unary_expression: $ => prec(16, seq(
        field('operator', choice('-', '~')),
        field('operand', $._expression),
    )),

    // Precedence 15
    not_expression: $ => prec(15, seq(
        $.not_keyword,
        field('operand', $._expression),
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

    comparison_operator: $ => choice(
        '==',
        '!=',
        '<=',
        '<',
        '>',
        '>=',
    ),

    // -------------------------------------------------------------------------
    // 6. ACCESSORS & INITIALIZERS
    // -------------------------------------------------------------------------

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

    // Structure or Array positional initialization block
    initializer_block: $ => seq(
      '{',
      optional($.initializer_list),
      '}',
    ),

    // List of expressions used for positional initialization
    initializer_list: $ => sep1($._expression, ','),


    // -------------------------------------------------------------------------
    // 7. TOKENS & KEYWORDS
    // -------------------------------------------------------------------------

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
    // Use prec(1) on this rule to define the 'else if' token
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
  }
});

// Helper function for separated lists that must have at least one element
function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
