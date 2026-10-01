; symbols for aerial.nvim: functions (methods as 'type.name'), types and file level 'let', 'dat' and 'var'

(function_definition
  receiver_type: (identifier)? @receiver
  name: (identifier) @name
  (#baz-qualified-name! @receiver @name)
  (#set! "kind" "Function")) @symbol

(type_definition
  name: (identifier) @name
  (#set! "kind" "Struct")) @symbol

(program
  (let_definition
    destination: (identifier) @name
    (#set! "kind" "Constant")) @symbol)

(program
  (data_declaration
    destination: (identifier) @name
    (#set! "kind" "Variable")) @symbol)

(program
  (variable_declaration
    destination: (identifier) @name
    (#set! "kind" "Variable")) @symbol)
