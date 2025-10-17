;; --- highlights.scm ---
;; NOTE: This version uses ANONYMOUS NODE MATCHING (no named fields)
;; to ensure compatibility and avoid the "Invalid field name" error, 
;; while still providing good semantic coloring.

;; === General ===
(comment) @comment

;; === Keywords and Control Flow ===
(field_keyword) @keyword.declaration
(func_keyword) @keyword.function
(return_keyword) @keyword.control

;; Control Flow Keywords
(if_keyword) @keyword.control
(loop_keyword) @keyword.control
(var_keyword) @keyword.declaration  ;; NEW: Highlight 'var' keyword

;; === Literals ===
(string_literal) @string
(number_literal) @number

;; === Definitions and Declarations (Anonymous Matching) ===

;; Field names: Matches the identifier in the field_definition node.
(field_definition
  (_)
  (identifier) @variable.declaration
)

;; NEW: Variable Declaration names (e.g., 'my_local_var' in var my_local_var = 1)
;; Matches the second child (the identifier) in the variable_declaration node.
(variable_declaration
  (_)
  (identifier) @variable.declaration
)

;; Function names: Matches the identifier in the function_definition node.
(function_definition
  (_)
  (identifier) @function.declaration
)

;; === Function Calls and Arguments ===

;; 1. Function Call name (e.g., 'calculate' in calculate(x, y))
(function_call
  (identifier) @function.call
)

;; 2. Arguments (Identifiers only): Use a distinct color for identifiers being passed into the function.
;; This targets usage of variables as arguments.
(argument_list
  (identifier) @variable.call
)

;; === Parameters and Types ===

;; Highlight the name of function arguments (parameters) using the specific parameter capture group.
(parameter
  (identifier) @parameter.function
)

;; Highlight type annotations.
(parameter
  (type_name) @type.builtin
)

;; === Variables (Inside function bodies) ===

;; Highlight the assignment destination: Matches the first identifier in the assignment_statement.
;; This catches simple reassignment (my_var = 5)
(assignment_statement
  (identifier) @variable
  (_)
)

;; Highlight identifiers used in conditions or field values that are BEING USED.
;; We use @variable.call to clearly distinguish variable usage from declarations/assignments.

;; Identifiers within the parenthesized expression (e.g., a function argument like (x))
(parenthesized_expression
  (identifier) @variable.call
)

;; Identifiers used as conditions in an if statement
(if_statement
  (identifier) @variable.call
)

;; Identifiers used as field values (when the field value is an identifier)
(field_definition
  (identifier) @variable.call
)
