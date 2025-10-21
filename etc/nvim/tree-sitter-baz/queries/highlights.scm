; highlights.scm

"(" @comment
")" @comment
"[" @comment
"]" @comment
"{" @comment
"}" @comment

; Keywords
(func_keyword) @keyword.function
(type_keyword) @keyword.type
(field_keyword) @keyword
(var_keyword) @keyword
(if_keyword) @keyword.conditional
(loop_keyword) @keyword.repeat
(else_keyword) @keyword.conditional
(else_if_keyword) @keyword.conditional
(break_keyword) @keyword.control
(continue_keyword) @keyword.control
(return_keyword) @keyword.return
(not_keyword) @keyword.operator
(and_keyword) @keyword.operator
(or_keyword) @keyword.operator

; Types
(bool_type) @type
(i8_type) @type
(i16_type) @type
(i32_type) @type
(i64_type) @type
(sized_array_type) @type
(unsized_array_type) @type

; Identifiers
(identifier) @variable
(function_definition name: (identifier) @function)
(type_definition name: (identifier) @type.definition)
(member_field name: (identifier) @variable.member)
(member_field ":" (_)? @type)
(parameter name: (identifier) @variable.parameter)
(return_annotation return_name: (identifier) @variable.parameter)

; Function parameters type
(parameter
  name: (identifier) @variable.parameter
  type: (identifier)? @type)

; Variable declaration
(variable_declaration
  destination: (identifier) @variable
  type: (identifier)? @type
  initializer: (_)? @variable)

; Sized array types
(sized_array_type (identifier) @type)

; Unsized array types
(unsized_array_type
  (identifier) @type)

; Function calls
(function_call function: (identifier) @function.call)

; Literals
(string_literal) @string
(number_literal) @number
(boolean_literal) @boolean

; Comments
(comment) @comment

; Operators
(comparison_operator) @operator
(unary_expression operator: ["-" "~"] @operator)
(multiplicative_expression operator: ["*" "/" "%"] @operator)
(additive_expression operator: ["+" "-"] @operator)
(shift_expression operator: ["<<" ">>"] @operator)
(bitwise_and_expression operator: ["&"] @operator)
(bitwise_or_expression operator: ["|"] @operator)
