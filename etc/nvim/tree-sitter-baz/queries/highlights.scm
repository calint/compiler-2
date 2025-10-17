;; --- highlights.scm ---
;; NOTE: This file uses anonymous node matching (no 'name:' or 'destination:') 
;; and has been fixed to avoid querying the problematic '_expression' node.

;; === General ===
(comment) @comment

;; === Keywords and Control Flow ===
(field_keyword) @keyword.declaration
(func_keyword) @keyword.function
(return_keyword) @keyword.control
(if_keyword) @keyword.control
(loop_keyword) @keyword.control
(var_keyword) @keyword.declaration
(type_keyword) @keyword.declaration

;; === Literals ===
(string_literal) @string
(number_literal) @number

;; === Definitions and Declarations (Anonymous Matching) ===

;; Field names: Matches the second child (the identifier) of the field_definition node.
(field_definition
  (_)
  (identifier) @variable.declaration
)

;; Function names: Matches the second child (the identifier) of the function_definition node.
(function_definition
  (_)
  (identifier) @function.declaration
)

;; Type definition name
(type_definition
  (identifier) @type.definition
)

;; Member field names (first identifier in the rule)
(member_field
  (identifier) @field
)

;; === Parameters and Types ===

;; Highlight the name of function arguments (parameters).
(parameter
  (identifier) @parameter.function
)

;; Highlight Custom Type Identifiers when used as array base types
(sized_array_type
  (identifier) @type.builtin
)
(unsized_array_type
  (identifier) @type.builtin
)

;; Built-in Type Tokens
(type_i64) @type.builtin
(type_bool) @type.builtin
(type_i8) @type.builtin
(type_i16) @type.builtin
(type_i32) @type.builtin

;; Highlight Array Brackets as Punctuation
(sized_array_type "[" @punctuation.bracket)
(sized_array_type "]" @punctuation.bracket)
(unsized_array_type "[]" @punctuation.bracket) @punctuation.bracket

;; === Statements and Expressions ===

;; Highlight the assignment destination: Matches the identifier being assigned to.
(assignment_statement
  (identifier) @variable
)

;; NEW: Variable declaration name
(variable_declaration
  (identifier) @variable.declaration
)

;; Function Call name
(function_call
  (identifier) @function.call
)

;; Fix: Highlight identifiers used as function arguments (expressions)
;; Query the identifier directly, assuming it's a child of argument_list or an intermediary parent
(argument_list
  (identifier) @variable.call
)

;; Fix: Highlight condition identifiers in if/loop statements
;; Query the identifier directly, assuming it's a child of if_statement/loop_statement
(if_statement
  (identifier) @variable.call
)
(loop_statement
  (identifier) @variable.call
)
