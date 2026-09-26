"(" @comment
")" @comment
"[" @comment
"]" @comment
"{" @comment
"}" @comment

(func_keyword) @keyword.function
(noinline_keyword) @keyword.modifier
(type_keyword) @keyword.type
(dat_keyword) @keyword.storage
(const_keyword) @keyword.storage
(var_keyword) @keyword
(if_keyword) @keyword.conditional
(loop_keyword) @keyword.repeat
(foo_keyword) @keyword.repeat
(else_keyword) @keyword.conditional
(else_if_keyword) @keyword.conditional
(break_keyword) @keyword.control
(continue_keyword) @keyword.control
(return_keyword) @keyword.return
(not_keyword) @keyword.operator
(and_keyword) @keyword.operator
(or_keyword) @keyword.operator

(const_definition
  destination: (identifier) @constant
  initializer: (_)? @constant)

(bool_type) @type
(i8_type) @type
(i16_type) @type
(i32_type) @type
(i64_type) @type
(sized_array_type) @type
(unsized_array_type) @type

(identifier) @variable

; 'self' is the implicit receiver of a method
((identifier) @type
  (#eq? @type "self")
  (#set! priority 110))

(function_definition name: (identifier) @function)
(function_definition receiver_type: (identifier) @type)
(type_definition name: (identifier) @type.definition)
(member_field name: (identifier) @variable.member)
(member_field type: (_) @type)
(parameter name: (identifier) @variable.parameter)
(return_annotation
  name: (identifier) @variable.parameter
  type: (identifier)? @type)

(parameter
  name: (identifier) @variable.parameter
  type: (identifier)? @type)

(variable_declaration
  destination: (identifier) @variable
  initializer: (_)? @variable)

((variable_declaration
  type: (identifier) @type)
  (#set! priority 110))

(data_declaration
  destination: (identifier) @variable
  type: (identifier)? @type)

(sized_array_type type: (identifier) @type)

(unsized_array_type type: (identifier) @type)

(function_call function: (identifier) @function.call)
(foo_statement array: (identifier) @variable)

(foo_statement
  array: (identifier) @variable)

(function_definition name: (identifier) @function)
(type_definition name: (identifier) @type.definition)

(string_literal) @string
(escape_sequence) @string.escape
(character_literal) @string
(number_literal) @number
(boolean_literal) @boolean

(comment) @comment

(comparison_operator) @operator
(unary_expression operator: ["-" "~"] @operator)
(multiplicative_expression operator: ["*" "/" "%"] @operator)
(additive_expression operator: ["+" "-"] @operator)
(shift_expression operator: ["<<" ">>"] @operator)
(bitwise_and_expression operator: ["&"] @operator)
(bitwise_or_expression operator: ["|"] @operator)
