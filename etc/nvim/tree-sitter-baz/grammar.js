// grammar.js
// Final conflict-free Tree-sitter grammar for your custom DSL

module.exports = grammar({
  name: 'baz',

  extras: $ => [/\s/, $.comment],

  conflicts: $ => [
    [$.statement, $.expression_statement],
    [$.statement, $.expression_statement, $._expression],
  ],

  rules: {
    source_file: $ => repeat($._definition),

    _definition: $ => choice(
      $.function_definition,
      $.type_definition,
      $.field_definition
    ),

    // -------------------------------
    // Comments
    // -------------------------------
    comment: _ => token(seq('#', /.*/)),

    // -------------------------------
    // Field definition: field x = 42
    // -------------------------------
    field_definition: $ => seq(
      'field',
      field('name', $.identifier),
      '=',
      field('value', $._expression)
    ),

    // -------------------------------
    // Type definitions
    // -------------------------------
    type_definition: $ => seq(
      'type',
      field('name', $.identifier),
      '{',
      commaSep($.field_declaration),
      '}'
    ),

    field_declaration: $ => seq(
      field('name', $.identifier),
      optional(seq(':', field('type', $.type)))
    ),

    // -------------------------------
    // Function definitions
    // -------------------------------
    function_definition: $ => seq(
      'func',
      field('name', $.identifier),
      field('params', $.parameter_list),
      optional(seq(':', field('return_type', $.type), field('return_var', $.identifier))),
      choice($.block, $.statement)
    ),

    parameter_list: $ => seq(
      '(',
      optional(commaSep($.parameter)),
      ')'
    ),

    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(':', field('type', $.type)))
    ),

    // -------------------------------
    // Types
    // -------------------------------
    type: $ => prec.right(seq(
      $.identifier,
      optional(seq('[', optional($._expression), ']'))
    )),

    // -------------------------------
    // Blocks and statements
    // -------------------------------
    block: $ => seq('{', repeat($.statement), '}'),

    statement: $ => prec.right(1, choice(
      $.variable_declaration,
      $.assignment,
      $.if_statement,
      $.loop_statement,
      $.return_statement,
      $.function_call,
      $.expression_statement,
      $.break_statement,
      $.continue_statement
    )),

    variable_declaration: $ => seq(
      'var',
      field('name', $.identifier),
      optional(seq(':', field('type', $.type))),
      '=',
      field('value', $._expression)
    ),

    assignment: $ => seq(
      field('target', $.lvalue),
      '=',
      field('value', $._expression)
    ),

    lvalue: $ => prec(3, seq(
      $.identifier,
      repeat(choice(
        seq('.', $.identifier),
        seq('[', $._expression, ']')
      ))
    )),
    
    if_statement: $ => prec.right(seq(
      'if',
      field('condition', $._expression),
      choice($.statement, $.block),
      optional(seq('else', choice($.statement, $.block)))
    )),

    loop_statement: $ => seq(
      'loop',
      choice($.block, $.statement)
    ),

    break_statement: _ => 'break',
    continue_statement: _ => 'continue',

    return_statement: _ => 'return',

    // Restrict expressions allowed as statements (no struct literals here)
    expression_statement: $ =>
      prec.left(1, choice(
        $.function_call,
        $.identifier,
        $.binary_expression,
        $.unary_expression,
        $.field_access,
        $.index_expression,
        $.literal,
        $.parenthesized_expression
      )),

    // -------------------------------
    // Expressions
    // -------------------------------
    _expression: $ => choice(
      $.binary_expression,
      $.unary_expression,
      $.function_call,
      $.field_access,
      $.index_expression,
      $.struct_literal,
      $.literal,
      $.identifier,
      $.parenthesized_expression
    ),

    parenthesized_expression: $ => seq('(', $._expression, ')'),

    // Operator precedence (lowest → highest)
    binary_expression: $ => choice(
      prec.left(1, seq($._expression, 'or', $._expression)),
      prec.left(2, seq($._expression, 'and', $._expression)),
      prec.left(3, seq($._expression, choice('==', '!=', '<', '<=', '>', '>='), $._expression)),
      prec.left(4, seq($._expression, choice('|', '^'), $._expression)),
      prec.left(5, seq($._expression, '&', $._expression)),
      prec.left(6, seq($._expression, choice('<<', '>>'), $._expression)),
      prec.left(7, seq($._expression, choice('+', '-'), $._expression)),
      prec.left(8, seq($._expression, choice('*', '/', '%'), $._expression))
    ),

    // Unary binds tighter than field/index
    unary_expression: $ => prec(10, seq(
      choice('-', 'not', '~'),
      $._expression
    )),

    // Field and index bind just below unary
    field_access: $ => prec(9, seq(
      $._expression,
      '.',
      $.identifier
    )),

    index_expression: $ => prec(9, seq(
      $._expression,
      '[',
      $._expression,
      ']'
    )),

    // Function call tighter than identifier
    function_call: $ => prec(11, seq(
      $.identifier,
      '(',
      optional(commaSep($._expression)),
      ')'
    )),

    // Struct literal (cannot appear directly as a statement)
    struct_literal: $ => prec(8, seq(
      '{',
      commaSep($._expression),
      '}'
    )),

    // -------------------------------
    // Literals
    // -------------------------------
    literal: $ => choice(
      $.number,
      $.string,
      $.boolean
    ),

    number: _ => token(choice(
      /0x[0-9A-Fa-f]+/,
      /0b[01]+/,
      /[0-9]+/
    )),

    string: _ => token(seq('"', /[^"]*/, '"')),

    boolean: _ => choice('true', 'false'),

    identifier: _ => /[A-Za-z_][A-Za-z0-9_]*/
  }
});

// -------------------------------
// Helper
// -------------------------------
function commaSep(rule) {
  return seq(rule, repeat(seq(',', rule)), optional(','));
}
