module.exports = grammar({
  name: 'baz',

  // Treat whitespace and comments as ignorable extras that can appear anywhere
  extras: $ => [
    /\s/, // standard whitespace
    $.comment,
  ],

  // Root of the program structure
  rules: {
    program: $ => repeat($._definition),

    _definition: $ => choice(
      $.field_definition,
      $.function_definition,
    ),

    // --- Definitions ---

    // field identifier = expression
    field_definition: $ => seq(
      $.field_keyword,
      field('name', $.identifier), // Named field for highlighting the variable name
      '=',
      $._expression, // Now uses _expression
    ),

    // func identifier ( parameters ) body
    function_definition: $ => seq(
      $.func_keyword,
      field('name', $.identifier), // Named field for highlighting the function name
      '(',
      field('parameters', optional($.parameter_list)), // Fixed: parameter_list is now non-empty
      ')',
      $._function_body,
    ),

    // Fixed: Must contain at least one parameter, handles comma separation internally
    parameter_list: $ => seq(
      $.parameter,
      repeat(seq(',', $.parameter))
    ),

    // identifier : type_name (type is optional)
    parameter: $ => seq(
      $.identifier,
      optional(seq(':', $.type_name)),
    ),

    type_name: $ => $.identifier,

    // --- Function Body and Statements ---

    _function_body: $ => choice(
      $._statement, // Single statement body
      $.block,      // Block statement body
    ),

    // Block: { statements | calls | control flow }
    block: $ => seq(
      '{',
      repeat($._statement),
      '}',
    ),

    // A statement can be an assignment, return, function call, if, or loop
    _statement: $ => choice(
      $.assignment_statement,
      $.return_statement,
      $.function_call,
      $.if_statement,
      $.loop_statement,
    ),

    // identifier = expression
    assignment_statement: $ => seq(
      field('destination', $.identifier), // Named field for highlighting the variable being assigned to
      '=',
      $._expression, // Now uses _expression
    ),

    // return (no arguments)
    return_statement: $ => $.return_keyword,

    // function_call identifier ( arguments )
    function_call: $ => seq(
      field('function', $.identifier), // Named field for highlighting the function name
      '(',
      field('arguments', optional($.argument_list)), // Fixed: argument_list is now non-empty
      ')',
    ),

    // Fixed: Must contain at least one expression, handles comma separation internally
    argument_list: $ => seq(
      $._expression,
      repeat(seq(',', $._expression))
    ),

    // if statement: if expression block
    if_statement: $ => seq(
      $.if_keyword,
      $._expression, // Now uses _expression for the condition
      $.block,
    ),

    // loop statement: loop block
    loop_statement: $ => seq(
      $.loop_keyword,
      $.block,
    ),

    // --- Expressions (The core unit of value) ---
    // Expression is now literal, identifier, function call, or parenthesized sub-expression
    _expression: $ => choice(
      $._literal,
      $.identifier,
      $.function_call,                 // Function call as an expression (e.g., as an argument)
      $.parenthesized_expression,      // NEW: Sub-expressions
    ),

    // NEW: ( expression )
    parenthesized_expression: $ => seq(
      '(',
      $._expression,
      ')'
    ),

    // --- Tokens (Keywords, Identifiers, Literals) ---

    // Keywords
    field_keyword: $ => 'field',
    func_keyword: $ => 'func',
    return_keyword: $ => 'return',
    if_keyword: $ => 'if',
    loop_keyword: $ => 'loop',

    // Identifiers (variable, function, type names)
    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    // Literals
    _literal: $ => choice(
      $.string_literal,
      $.number_literal,
    ),

    string_literal: $ => seq(
      '"',
      repeat(/[^"\n]/), // Any character except quote or newline
      '"',
    ),

    number_literal: $ => /\d+(\.\d+)?/, // Simple integer or float

    // Comments
    comment: $ => /#.*/,
  }
});
