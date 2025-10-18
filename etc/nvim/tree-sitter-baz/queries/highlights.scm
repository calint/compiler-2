; highlights.scm

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

; Identifiers
(identifier) @variable
(function_definition name: (identifier) @function)
(type_definition name: (identifier) @type.definition)
(member_field name: (identifier) @variable.member)
(parameter name: (identifier) @variable.parameter)

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
