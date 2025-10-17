// --- grammar.js ---

/**
 * Tree-sitter grammar definition for the Baz language.
 * This defines the formal syntax rules for parsing Baz code.
 */
module.exports = grammar({
  // The name must match the directory name (e.g., 'tree-sitter-baz')
  name: 'baz',

  rules: {
    // The top-level rule: a program is a sequence of definitions.
    program: $ => repeat($._definition),

    // An internal rule to group fields and functions as definitions.
    _definition: $ => choice(
      $.field_definition,
      $.function_definition
    ),

    // Rule 1: A field definition
    // Example: field my_var = "hello"
    field_definition: $ => seq(
      $.field_keyword,
      // 'name' field used for easy highlighting/querying
      field('name', $.identifier),
      '=',
      // 'value' field used for easy highlighting/querying
      field('value', $._literal)
    ),

    // Rule 2: A function definition
    // Example: func calculate(a: Int, b: Float) { return 0 } or func get_name() "Baz"
    function_definition: $ => seq(
      $.func_keyword,
      // 'name' field used for easy highlighting/querying
      field('name', $.identifier),
      '(',
      optional($.parameter_list),
      ')',
      // Function body, which can be a single statement or a block
      field('body', choice(
        $._statement,
        $.block
      ))
    ),

    // A list of parameters separated by commas (used inside func definition)
    parameter_list: $ => sep1($.parameter, ','),

    // Defines a single parameter with optional type annotation
    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(
        ':',
        field('type', $.type_name)
      ))
    ),

    // Type names are currently just identifiers (e.g., Int, String, etc.)
    type_name: $ => $.identifier,

    // A block is a sequence of statements enclosed in braces
    block: $ => seq(
      '{',
      repeat($._statement),
      '}'
    ),

    // General statement (assignment and return)
    _statement: $ => choice(
      $.assignment_statement,
      $.return_statement
    ),

    // Simple assignment statement (for use inside functions)
    // Example: x = 10
    assignment_statement: $ => seq(
      field('destination', $.identifier),
      '=',
      field('source', $._literal)
    ),

    // Simple return statement
    // Example: return 0
    return_statement: $ => seq(
      $.return_keyword,
      optional(field('value', $._literal))
    ),

    // Internal rule for constants that can be assigned to a field or returned
    _literal: $ => choice(
      $.string_literal,
      $.number_literal
    ),

    // --- LEXICAL TOKENS (Keywords and Primitives) ---

    // Keywords defined by the user request
    field_keyword: $ => 'field',
    func_keyword: $ => 'func',
    return_keyword: $ => 'return',

    // Standard identifier format
    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    // String literal (simple double-quoted strings)
    string_literal: $ => /"([^"\n]|\\(.|\n))*"/,

    // Numeric constant (integers or decimals)
    number_literal: $ => /[0-9]+(\.[0-9]+)?/,
  }
});

/**
 * Helper function to define a separated list (e.g., a, b, c)
 * @param {Rule} rule The rule for the items (e.g., $.identifier)
 * @param {string} separator The string separating items (e.g., ',')
 */
function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
