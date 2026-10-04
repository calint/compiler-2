"(" @comment
")" @comment
"[" @comment
"]" @comment
"{" @comment
"}" @comment

(func_keyword) @keyword.function
(noinline_keyword) @keyword.function
(type_keyword) @keyword.function
(dat_keyword) @keyword.function
(let_keyword) @keyword.function
(mut_keyword) @keyword.function
(var_keyword) @keyword.function
(if_keyword) @keyword.function
(loop_keyword) @keyword.function
(foo_keyword) @keyword.function
(else_keyword) @keyword.function
(else_if_keyword) @keyword.function
(break_keyword) @keyword.function
(continue_keyword) @keyword.function
(return_keyword) @keyword.function
(not_keyword) @keyword.function
(and_keyword) @keyword.function
(or_keyword) @keyword.function

(let_definition
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
(typed_initializer type: (identifier) @type)
(typed_array_initializer type: (identifier) @type)
(type_definition name: (identifier) @type.definition)
(member_field name: (identifier) @variable.member)
(member_access (identifier) @variable.member)
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

(data_declaration
  destination: (identifier) @variable
  initializer: (_)? @variable)

; file level 'dat', 'var' and 'let' use the module color to stand out from
; locals
(program
  (let_definition
    destination: (identifier) @module))

(program
  (data_declaration
    destination: (identifier) @module))

(program
  (variable_declaration
    destination: (identifier) @module))

(sized_array_type type: (identifier) @type)

(unsized_array_type type: (identifier) @type)

(function_call function: (identifier) @function.call)
(foo_statement array: (identifier) @variable)

(foo_statement
  array: (identifier) @variable)

; references to file level 'dat', 'var' and 'let', predicate in
; plugin/baz-globals.lua
; placed after the '@variable' patterns so that it wins
((identifier) @module
  (#baz-global? @module))

; references to parameters and the named return value, predicate in
; plugin/baz-globals.lua, placed after the '@variable' patterns so that it wins
((identifier) @variable.parameter
  (#baz-parameter? @variable.parameter))

; a type name used as a value, e.g. 'var tz = tokenizer', predicate in
; plugin/baz-globals.lua
((identifier) @type
  (#baz-type? @type))

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
