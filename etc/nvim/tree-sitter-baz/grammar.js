module.exports = grammar({
  name: "baz",

  extras: $ => [
    /\s/,
    $.comment,
  ],

  rules: {
    source_file: $ => repeat($._definition),

    _definition: $ => choice(
      $.function_definition,
      $.type_definition,
      $.variable_declaration,
      $.expression_statement,
    ),

    function_definition: $ => seq(
      "func",
      $.identifier,
      $.parameter_list,
      optional(seq(":", $.type, $.identifier)),
      $.block
    ),

    parameter_list: $ => seq(
      "(",
      optional(commaSep($.parameter)),
      ")"
    ),

    parameter: $ => seq(
      $.identifier,
      optional(seq(":", $.type))
    ),

    type_definition: $ => seq(
      "type",
      $.identifier,
      $.block
    ),

    block: $ => seq(
      "{",
      repeat($._statement),
      "}"
    ),

    _statement: $ => choice(
      $.variable_declaration,
      $.assignment,
      $.expression_statement,
      $.if_statement,
    ),

    variable_declaration: $ => seq(
      "var",
      $.identifier,
      optional(seq(":", $.type)),
      optional(seq("=", $._expression))
    ),

    assignment: $ => seq(
      $._lvalue,
      "=",
      $._expression
    ),

    if_statement: $ => seq(
      "if",
      $._expression,
      $._statement
    ),

    expression_statement: $ => $._expression,


    _expression: $ => choice(
      $.binary_expression,
      $.unary_expression,

      $.function_call,
      prec.left(4, $.member_access),
      prec.left(4, $.array_access),

      $.identifier,
      $.number,
      $.boolean,
      $.parenthesized_expression,
    ),

    binary_expression: $ => choice(
      prec.left(1, seq($._expression, "==", $._expression)),
      prec.left(2, seq($._expression, "+", $._expression)),
    ),

    unary_expression: $ => prec(3, seq( // Use prec(3) to make it lower than prec.left(4)
      "not",
      $._expression
    )),

    function_call: $ => prec(6, seq( // Prec is on the entire sequence
      $.identifier,
      "(",
      optional(commaSep($._expression)),
      ")"
    )),

    member_access: $ => seq(
      $._expression,
      ".",
      $.identifier
    ),

    array_access: $ => seq(
      $._expression,
      "[",
      $._expression,
      "]"
    ),

    parenthesized_expression: $ => seq(
      "(",
      $._expression,
      ")"
    ),

    _lvalue: $ => choice(
      $.identifier,
      $.member_access,
      $.array_access
    ),

    type: $ => seq(
      $.identifier,
      optional(seq("[", $.number, "]"))
    ),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    number: $ => /0x[0-9a-fA-F]+|[0-9]+/,

    boolean: $ => choice("true", "false"),

    comment: $ => token(seq("#", /.*/)),
  }
});

// Helper for comma-separated lists
function commaSep(rule) {
  return seq(rule, repeat(seq(",", rule)));
}
