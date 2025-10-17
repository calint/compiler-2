;; --- highlights.scm ---
;; Defines highlighting queries for the Baz language based on the grammar.js
;; This version uses specific captures to allow for fine-grained coloring
;; of keywords, function arguments, and types.

;; === Keywords ===
(field_keyword) @keyword.declaration  ;; For 'field'
(func_keyword) @keyword.function     ;; For 'func'
(return_keyword) @keyword.control    ;; For 'return'

;; === Literals ===
(string_literal) @string
(number_literal) @number

;; === Definitions and Declarations ===

;; Top-level field names (variables)
(field_definition
  name: (identifier) @variable.declaration
)

;; Function names
(function_definition
  name: (identifier) @function.declaration
)

;; === Parameters and Types ===

;; Highlight the name of function arguments (parameters).
;; Using @parameter.function for a distinct color for arguments.
(parameter
  name: (identifier) @parameter.function
)

;; Highlight type annotations.
;; Using @type.builtin for a distinct color for types like Int, Float, etc.
(parameter
  type: (type_name) @type.builtin
)

;; === Statements (Inside function bodies) ===

;; Highlight the destination of an assignment as a regular variable (e.g., in `x = 10`)
(assignment_statement
  destination: (identifier) @variable
)

;; Highlight the returned literal value (the value itself is caught by @string/@number)
(return_statement
  value: (_) @variable.builtin
)
