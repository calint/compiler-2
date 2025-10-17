const BAZ_KEYWORDS = {
  FIELD: 'field',
  FUNC: 'func',
  RETURN: 'return',
};

module.exports = grammar({
  name: 'baz',

  // Treat comments and standard whitespace as extra content that can appear anywhere
  extras: $ => [
    /\s/,
    $.comment,
  ],

  // Define tokens that don't need a rule (like keywords)
  word: $ => $.identifier,

  // Rule that the parser attempts to match first
  rules: {
    // The top-level container: a sequence of zero or more definitions
    program: $ => repeat(
      choice(
        $.field_definition,
        $.function_definition,
      ),
    ),

    // --- Keywords ---
    field_keyword: $ => BAZ_KEYWORDS.FIELD,
    func_keyword: $ => BAZ_KEYWORDS.FUNC,
    return_keyword: $ => BAZ_KEYWORDS.RETURN,

    // --- Definitions ---

    // field identifier = (string | number)
    field_definition: $ => seq(
      $.field_keyword,
      $.identifier,
      '=',
      $._literal,
    ),

    // func identifier ( parameters ) body
    function_definition: $ => seq(
      $.func_keyword,
      $.identifier,
      '(',
      optional($.parameter_list),
      ')',
      $._function_body,
    ),

    // --- Function Components ---

    parameter_list: $ => sep1($.parameter, ','),

    // parameter: identifier optional(: type)
    parameter: $ => seq(
      $.identifier,
      optional(seq(
        ':',
        $.type_name
      )),
    ),

    // function body can be a single statement or a block
    _function_body: $ => choice(
      $.block,
      $._statement,
    ),

    // { statement... }
    block: $ => seq(
      '{',
      repeat($._statement),
      '}',
    ),

    // --- Statements ---

    _statement: $ => choice(
      $.assignment_statement,
      $.return_statement,
    ),

    // var = (string | number)
    assignment_statement: $ => seq(
      field('destination', $.identifier),
      '=',
      $._literal,
    ),

    // return (with no arguments, as per new rule)
    return_statement: $ => $.return_keyword,

    // --- Basic Primitives ---

    _literal: $ => choice(
      $.string_literal,
      $.number_literal,
    ),

    // A type name is just an identifier (e.g., Int, Float)
    type_name: $ => $.identifier,

    // An identifier (variable or function name)
    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    string_literal: $ => /"[^"\n]*"/,
    number_literal: $ => /-?[0-9]+(\.[0-9]+)?/,

    // Comments start with # and consume the rest of the line
    comment: $ => /#.*/,
  },
});

function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
