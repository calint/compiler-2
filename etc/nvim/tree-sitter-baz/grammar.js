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

    // --- Definitions ---

    // field identifier = expression
    field_definition: $ => seq(
      $.field_keyword,
      field('name', $.identifier), // Named field for highlighting the variable name
      '=',
      $._expression,
    ),

    // type Identifier { member_field_list }
    type_definition: $ => seq(
      $.type_keyword,
      field('name', $.identifier), // Named field for highlighting the type name
      '{',
      optional($.member_field_list),
      '}',
    ),

    member_field_list: $ => sep1($.member_field, ','),

    // member_field name: type
    member_field: $ => seq(
      field('name', $.identifier), // Named field for highlighting the member name
      optional(seq(':', $._definition_type)),
    ),

    // func identifier ( parameters ) return_annotation body
    function_definition: $ => seq(
      $.func_keyword,
      field('name', $.identifier), // Named field for highlighting the function name
      '(',
      optional($.parameter_list),
      ')',
      optional($.return_annotation), // Optional return type
      $._function_body,
    ),

    return_annotation: $ => seq(
      ':',
      $._definition_type,
      field('return_name', $.identifier) // Named field for quasi-variable
    ),

    parameter_list: $ => sep1($.parameter, ','),

    // identifier : type_name (type is optional)
    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(':', $._parameter_type)),
    ),

    // --- Types ---

    // Type used in definitions (member fields, function returns) - requires size for arrays
    _definition_type: $ => choice(
      $._base_type,
      $.sized_array_type,
    ),

    // Type used in parameters - arrays must be unsized
    _parameter_type: $ => choice(
      $._base_type,
      $.unsized_array_type,
    ),

    // Base type is either built-in or a custom identifier
    _base_type: $ => choice(
      $.bool_type, $.i8_type, $.i16_type, $.i32_type, $.i64_type,
      $.identifier // Custom type identifier
    ),

    // Array type for definitions (must have size)
    sized_array_type: $ => seq(
      $._base_type,
      '[',
      $.number_literal, // Size is a constant
      ']',
    ),

    // Array type for parameters (no size allowed)
    unsized_array_type: $ => seq(
      $._base_type,
      '[',
      ']',
    ),

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

    // A statement can be an assignment, declaration, return, function call, if, or loop
    _statement: $ => choice(
      $.assignment_statement,
      $.variable_declaration, // var declaration
      $.return_statement,
      $.function_call,
      $.if_statement, // NEW
      $.loop_statement, // NEW
    ),

    // var identifier = expression
    variable_declaration: $ => seq(
      $.var_keyword,
      field('destination', $.identifier),
      '=',
      $._expression,
    ),

    // identifier = expression
    assignment_statement: $ => seq(
      field('destination', $.identifier), // Named field for highlighting the variable being assigned to
      '=',
      $._expression,
    ),

    // return (no arguments)
    return_statement: $ => $.return_keyword,

    // function_call identifier ( arguments )
    function_call: $ => seq(
      field('function', $.identifier), // Named field for highlighting the function name
      '(',
      optional($.argument_list),
      ')',
    ),

    argument_list: $ => sep1($._expression, ','),

    // NEW: if statement: if expression block
    if_statement: $ => seq(
      $.if_keyword,
      $._expression,
      $.block,
    ),

    // NEW: loop statement: loop block
    loop_statement: $ => seq(
      $.loop_keyword,
      $.block,
    ),

    // Expression can be a literal, identifier (variable), function call, or sub-expression
    _expression: $ => choice(
      $._literal,
      $.identifier,
      $.function_call,
      $.parenthesized_expression,
    ),

    parenthesized_expression: $ => seq(
      '(',
      $._expression,
      ')',
    ),

    // --- Tokens (Keywords, Identifiers, Literals) ---

    // Keywords
    field_keyword: $ => 'field',
    func_keyword: $ => 'func',
    type_keyword: $ => 'type', // NEW
    var_keyword: $ => 'var',   // NEW
    return_keyword: $ => 'return',
    if_keyword: $ => 'if',
    loop_keyword: $ => 'loop',

    // Built-in Types (Keywords)
    bool_type: $ => 'bool',
    i8_type: $ => 'i8',
    i16_type: $ => 'i16',
    i32_type: $ => 'i32',
    i64_type: $ => 'i64',

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

    number_literal: $ => choice(
      // Hex: 0x followed by one or more hex digits
      /0x[0-9a-fA-F]+/,
      // Binary: 0b followed by one or more binary digits
      /0b[01]+/,
      // Decimal (Integer or Float)
      /\d+(\.\d+)?/,
    ),

    // Comments
    comment: $ => /#.*/,
  }
});

// Helper function for separated lists that must have at least one element
function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
