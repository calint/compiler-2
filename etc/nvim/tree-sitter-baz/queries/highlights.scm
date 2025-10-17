;; --- highlights.scm ---
;; Defines highlighting queries for the Baz language.
;; NOTE: THIS VERSION USES ANONYMOUS NODE MATCHING (no 'name:' or 'destination:').
;; This resolves the "Invalid field name" error that occurs when the parser is
;; not synchronized with the grammar that uses named fields.

;; === General ===
(comment) @comment

;; === Keywords and Control Flow ===
(field_keyword) @keyword.declaration
(func_keyword) @keyword.function
(return_keyword) @keyword.control

;; === Literals ===
(string_literal) @string
(number_literal) @number

;; === Definitions and Declarations (Anonymous Matching) ===

;; Field names: Matches the second child (the identifier) of the field_definition node.
;; Color: @variable.declaration
(field_definition
  (_)
  (identifier) @variable.declaration  ; The field name is the second child of field_definition
)

;; Function names: Matches the second child (the identifier) of the function_definition node.
;; Color: @function.declaration
(function_definition
  (_)
  (identifier) @function.declaration ; The function name is the second child of function_definition
)

;; === Parameters and Types ===

;; Highlight the name of function arguments (parameters).
;; Color: @parameter.function
(parameter
  (identifier) @parameter.function
)

;; Highlight type annotations.
;; Color: @type.builtin
(parameter
  (type_name) @type.builtin
)

;; === Statements (Inside function bodies) ===

;; Highlight the assignment destination: Matches the first identifier in the assignment_statement.
;; Color: @variable
(assignment_statement
  (identifier) @variable ; The assignment destination is the first child (identifier)
  (_)
)
