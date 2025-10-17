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
      $._function_body,
    ),

    return_annotation: $ => seq(
      ':',
      $._definition_type,
      field('return_name', $.identifier)
    ),

    // parameter_list is explicitly defined to require 1 or more elements (No empty match allowed!)
    parameter_list: $ => seq(
      $.parameter,
      repeat(seq(',', $.parameter)),
    ),

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

    _function_body: $ => choice(
      $._statement,
      $.block,
    ),

    block: $ => seq(
      '{',
      repeat($._statement),
      '}',
    ),

    _statement: $ => choice(
      $.assignment_statement,
      $.variable_declaration,
      $.return_statement,
      $.function_call,
      $.if_statement,
      $.loop_statement,
    ),

    // var identifier : type = expression
    variable_declaration: $ => seq(
      $.var_keyword,
      field('destination', $.identifier),
      optional(seq(':', $._definition_type)),
      // Initializer is now optional
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

    // function_call identifier ( arguments )
    function_call: $ => seq(
      field('function', $.identifier),
      '(',
      optional($.argument_list), // List is optional (avoids runtime error for ())
      ')',
    ),

    // argument_list is explicitly defined to require 1 or more elements (No empty match allowed!)
    argument_list: $ => seq(
      $._expression,
      repeat(seq(',', $._expression)),
    ),

    // --- Expressions and Access ---

    // Expression can be a literal, access_chain, function call, comparison, struct literal, or sub-expression
    _expression: $ => choice(
      $._literal,
      $._access_chain,
      $.function_call,
      $.comparison_expression,
      $.struct_literal, // Allows { name: value, ... } syntax for initialization
      $.parenthesized_expression,
    ),
    
    // Structure initialization literal (e.g., var p: Point = { x: 10, y: 20 })
    struct_literal: $ => seq(
      '{',
      optional($.field_initializer_list),
      '}',
    ),

    field_initializer_list: $ => sep1($.field_initializer, ','),

    field_initializer: $ => seq(
      field('name', $.identifier),
      ':',
      field('value', $._expression),
    ),


    // Boolean comparison expression
    // prec.left(10) gives this a medium precedence for operator chaining
    comparison_expression: $ => prec.left(10, seq(
        field('left', $._expression),
        field('operator', $.comparison_operator),
        field('right', $._expression),
    )),

    // Comparison operators (already defined as distinct tokens)
    comparison_operator: $ => choice(
        '==',
        '!=',
        '<=',
        '<',
        '>',
        '>=',
    ),

    // --- Existing Access and Control Flow Rules ---

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

    // Full if/else if/else structure
    if_statement: $ => seq(
      $.if_keyword,
      $._expression, // Handles both full comparison and shorthand check (e.g., `if count`)
      $.block,
      repeat($.else_if_clause), // Zero or more else if blocks
      optional($.else_clause),  // Optional else block at the end
    ),
    
    // else if clause
    else_if_clause: $ => seq(
        $.else_if_keyword,
        $._expression,
        $.block,
    ),

    // else clause
    else_clause: $ => seq(
        $.else_keyword,
        $.block,
    ),

    loop_statement: $ => seq(
      $.loop_keyword,
      $.block,
    ),

    // --- Tokens (Keywords, Identifiers, Literals) ---

    field_keyword: $ => 'field',
    func_keyword: $ => 'func',
    type_keyword: $ => 'type',
    var_keyword: $ => 'var',
    return_keyword: $ => 'return',
    if_keyword: $ => 'if',
    loop_keyword: $ => 'loop',
    
    // Keywords for control flow
    else_if_keyword: $ => prec(1, seq('else', /\s+/, 'if')),
    else_keyword: $ => 'else',

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
    
    // Boolean literals
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
      /0x[0-9a-fA-F]+/,
      /0b[01]+/,
      /\d+(\.\d+)?/,
    ),

    comment: $ => /#.*/,
  }
});

// Helper function for separated lists that must have at least one element
function sep1(rule, separator) {
  return seq(rule, repeat(seq(separator, rule)));
}
