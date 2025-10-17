;; --- highlights.scm ---
;; Synchronized with the latest grammar.js (Types, Arrays, Return Annotations)

;; === General ===
(comment) @comment

;; === Keywords and Control Flow ===
(field_keyword) @keyword.declaration
(func_keyword) @keyword.function
(type_keyword) @keyword.declaration
(var_keyword) @keyword.declaration
(return_keyword) @keyword.control

;; Control Flow Keywords
(if_keyword) @keyword.control
(loop_keyword) @keyword.control

;; === Types and Built-ins ===
(bool_type) @type.builtin
(i8_type) @type.builtin
(i16_type) @type.builtin
(i32_type) @type.builtin
(i64_type) @type.builtin

;; Custom Type Definitions (The name of the type itself)
(type_definition
  name: (identifier) @type.definition
)

;; Type Name Usage inside member fields/function types (custom types)
(sized_array_type
  (identifier) @type.name
)
(unsized_array_type
  (identifier) @type.name
)

;; === Literals ===
(string_literal) @string
(number_literal) @number

;; === Punctuation ===
[ "{" "}" "(" ")" "=" ":" "," ] @punctuation.bracket
[ "[" "]" ] @punctuation.delimiter

;; === Definitions and Declarations ===

;; Top-level field names
(field_definition
  name: (identifier) @variable.declaration
)

;; Function names
(function_definition
  name: (identifier) @function.declaration
)

;; Member field names inside a type definition
(member_field
  name: (identifier) @field
)

;; Variables declared with 'var'
(variable_declaration
  destination: (identifier) @variable.declaration
)

;; === Function Signature and Return ===

;; Highlight the name of function arguments (parameters)
(parameter
  (identifier) @parameter.function
)

;; Highlight the quasi-variable name in the return annotation
(return_annotation
  (identifier) @variable.return
)

;; === Statements and Expressions (Variable Usage) ===

;; Assignment destination (variable being modified)
(assignment_statement
  destination: (identifier) @variable
)

;; Identifiers used as arguments in function calls
(function_call
  (identifier) @variable.call
)

;; Identifiers used as bare expressions (e.g., condition in if_statement or lone statement)
(if_statement
  (identifier) @variable.call
)

;; Identifiers used in parenthesized expressions (covers nested calls/expressions)
(parenthesized_expression
  (identifier) @variable.call
)
