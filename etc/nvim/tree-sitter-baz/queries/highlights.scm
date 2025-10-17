; Keywords in context
(field_declaration "field" @keyword)
(type_definition "type" @keyword)
(function_definition "func" @keyword)
(variable_declaration "var" @keyword)
(if_statement "if" @keyword)
(if_statement "else" @keyword)
(loop_statement "loop" @keyword)
(break_statement) @keyword
(continue_statement) @keyword
(return_statement) @keyword

(logical_and "and" @keyword)
(logical_or "or" @keyword)
(unary "not" @keyword)

; Built-in types
"i8" @type
"i16" @type
"i32" @type
"i64" @type
"bool" @type

(boolean) @constant.builtin

; Numbers and strings
(number) @number
(string) @string

; Operators
"+" @operator
"-" @operator
"*" @operator
"/" @operator
"%" @operator
"<<" @operator
">>" @operator
"&" @operator
"|" @operator
"^" @operator
"~" @operator
"=" @operator
"==" @operator
"!=" @operator
"<" @operator
"<=" @operator
">" @operator
">=" @operator

; Delimiters
"(" @punctuation.bracket
")" @punctuation.bracket
"{" @punctuation.bracket
"}" @punctuation.bracket
"[" @punctuation.bracket
"]" @punctuation.bracket

"." @punctuation.delimiter
"," @punctuation.delimiter
":" @punctuation.delimiter

; Type and function definitions - highlight the first identifier specially
(type_definition (identifier) @type.definition)
(function_definition (identifier) @function)

; Variable declarations - highlight the first identifier specially
(variable_declaration (identifier) @variable.definition)

; Parameters
(parameter (identifier) @parameter)

; General identifiers - lowest priority
(identifier) @variable
