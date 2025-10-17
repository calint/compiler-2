;; baz highlights.scm

[
  "func"
  "var"
  "type"
  "field"
  "if"
  "else"
  "loop"
  "not"
  "and"
  "or"
] @keyword

(return_statement) @keyword
(break_statement) @keyword
(continue_statement) @keyword

(identifier) @variable
(string) @string
(number) @number
(boolean) @boolean
(comment) @comment
