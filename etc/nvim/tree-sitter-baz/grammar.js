module.exports = grammar({
  name: 'baz',

  precedences: $ => [
    [$.logical_or],
    [$.logical_and],
    [$.equality],
    [$.comparison],
    [$.bitwise_or],
    [$.bitwise_xor],
    [$.bitwise_and],
    [$.shift],
    [$.additive],
    [$.multiplicative],
  ],

  rules: {
    source_file: $ => repeat($._definition),

    _definition: $ => choice(
      $.field_declaration,
      $.type_definition,
      $.function_definition
    ),

    field_declaration: $ => seq(
      'field',
      $.identifier,
      '=',
      $._expression
    ),

    type_definition: $ => seq(
      'type',
      $.identifier,
      '{',
      optional($.field_list),
      '}'
    ),

    field_list: $ => seq(
      $.type_field,
      repeat(seq(',', optional(/\s/), $.type_field))
    ),

    type_field: $ => seq(
      $.identifier,
      optional(seq(':', $._type))
    ),

    function_definition: $ => seq(
      'func',
      $.identifier,
      $.parameter_list,
      optional($.return_type),
      $.block
    ),

    parameter_list: $ => seq(
      '(',
      optional($.parameters),
      ')'
    ),

    parameters: $ => seq(
      $.parameter,
      repeat(seq(',', optional(/\s/), $.parameter))
    ),

    parameter: $ => seq(
      $.identifier,
      optional(seq(':', $.param_type))
    ),

    param_type: $ => choice(
      /reg_[a-z0-9]+/,
      $._type
    ),

    return_type: $ => seq(
      ':',
      $._type,
      $.identifier
    ),

    _type: $ => choice(
      'bool',
      'i8',
      'i16',
      'i32',
      'i64',
      $.identifier,
      $.array_type
    ),

    array_type: $ => seq(
      $._type,
      '[',
      optional($._expression),
      ']'
    ),

    block: $ => seq(
      '{',
      repeat($._statement),
      '}'
    ),

    _statement: $ => choice(
      $.variable_declaration,
      $.assignment,
      $.if_statement,
      $.loop_statement,
      $.break_statement,
      $.continue_statement,
      $.return_statement,
      $.expression_statement,
      $.block
    ),

    variable_declaration: $ => seq(
      'var',
      $.identifier,
      optional(seq(':', $._type)),
      optional(seq('=', choice($.struct_literal, $._expression)))
    ),

    assignment: $ => seq(
      $._lvalue,
      '=',
      choice($.struct_literal, $._expression)
    ),

    _lvalue: $ => $.postfix,

    if_statement: $ => prec.right(seq(
      'if',
      $._expression,
      $._simple_statement,
      optional(seq(
        'else',
        $._simple_statement
      ))
    )),

    _simple_statement: $ => choice(
      $.block,
      $.variable_declaration,
      $.assignment,
      $.break_statement,
      $.continue_statement,
      $.return_statement,
      $.loop_statement,
      $.if_statement
    ),

    loop_statement: $ => seq(
      'loop',
      $.block
    ),

    break_statement: $ => 'break',

    continue_statement: $ => 'continue',

    return_statement: $ => 'return',

    expression_statement: $ => $._expression,

    _expression: $ => choice(
      $.logical_or
    ),

    logical_or: $ => seq(
      $.logical_and,
      repeat(seq('or', $.logical_and))
    ),

    logical_and: $ => seq(
      $.equality,
      repeat(seq('and', $.equality))
    ),

    equality: $ => seq(
      $.comparison,
      repeat(seq(choice('==', '!='), $.comparison))
    ),

    comparison: $ => seq(
      $.bitwise_or,
      repeat(seq(choice('<', '<=', '>', '>='), $.bitwise_or))
    ),

    bitwise_or: $ => seq(
      $.bitwise_xor,
      repeat(seq('|', $.bitwise_xor))
    ),

    bitwise_xor: $ => seq(
      $.bitwise_and,
      repeat(seq('^', $.bitwise_and))
    ),

    bitwise_and: $ => seq(
      $.shift,
      repeat(seq('&', $.shift))
    ),

    shift: $ => seq(
      $.additive,
      repeat(seq(choice('<<', '>>'), $.additive))
    ),

    additive: $ => prec.left(seq(
      $.multiplicative,
      repeat(seq(choice('+', '-'), $.multiplicative))
    )),

    multiplicative: $ => prec.left(seq(
      $.unary,
      repeat(seq(choice('*', '/', '%'), $.unary))
    )),

    unary: $ => seq(
      repeat(choice('not', '-', '~')),
      $.postfix
    ),

    postfix: $ => prec.left(seq(
      $.primary,
      repeat(choice(
        seq('.', $.identifier),
        seq('[', $._expression, ']'),
        seq('(', optional($.arguments), ')')
      ))
    )),

    primary: $ => choice(
      $.identifier,
      $.number,
      $.string,
      $.boolean,
      seq('(', $._expression, ')')
    ),

    arguments: $ => seq(
      $._expression,
      repeat(seq(',', optional(/\s/), $._expression))
    ),

    struct_literal: $ => seq(
      '{',
      $._expression,
      repeat(seq(',', optional(/\s/), $._expression)),
      '}'
    ),

    number: $ => choice(
      /0x[0-9a-fA-F]+/,
      /0b[01]+/,
      /-?\d+/
    ),

    string: $ => seq(
      '"',
      repeat(choice(
        /[^"\\]/,
        seq('\\', /./)
      )),
      '"'
    ),

    boolean: $ => choice('true', 'false'),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/
  },

  extras: $ => [
    /\s/,
    /#.*/
  ]
});
