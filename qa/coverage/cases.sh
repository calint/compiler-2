# The list of compiler test cases, sourced by 'test-coverage.sh' (and
# 'test-cases.sh') after it defines the helpers below. One case is a source
# 'tests/NNN.baz' and a helper that says how it is judged. The comment above a
# case is the first comment of its source.
#
#   EXP=N        the expected exit code of the program or run
#   RUN          compile, run, and compare the exit code with 'EXP'
#   RUN_ERR      like 'RUN', and compare the error output with 'NNN.out'
#   RUN_ERR_OPTS like 'RUN_ERR' with the compiler options of its argument
#   RUN_NO_CHECKS like 'RUN' compiled without the runtime checks
#   DIFF         run, and compare the output with 'NNN.out'
#   DIFFINP      like 'DIFF' with 'NNN.in' as input
#   DIFFINP2     like 'DIFFINP' with the input given one line per read
#   DIFFPY       compile, run the script 'NNN.py' and compare its output with
#                'NNN.out'
#   DIFFNOPT     run, and compare the difference of the output of '--nopt' and
#                of the optimized compile with 'NNN.TARGET.diff'
#   COMPERR      the compile fails with exit code 1 and its message is
#                'NNN.TARGET.out' or else 'NNN.out'
#
# A case inside 'if [[ $MACHINE ... ]]' runs only for those targets (x86_64,
# rv32i, rv32i-qemu, rv32i-fpga). A commented case is disabled on purpose.

# arithmetic precedence and grouping
SRC=0001 && EXP=58 && RUN

# scalar variable initialization and exit
SRC=0002 && EXP=1 && RUN

# multiplication binds tighter than addition
SRC=0003 && EXP=7 && RUN

# grouped boolean conjunction and disjunction
SRC=0004 && EXP=0 && RUN

# boolean grouping with conjunction on both sides
SRC=0005 && EXP=0 && RUN

# jump optimization patterns, the optimized output is compared with --nopt in
# 0006.x86_64.diff and 0006.rv32i.diff
SRC=0006 && EXP=0 && DIFFNOPT

# negated comparison
SRC=0007 && EXP=0 && RUN

# conjunction of independent comparisons
SRC=0008 && EXP=0 && RUN

# chained boolean conjunction
SRC=0009 && EXP=0 && RUN

# chained boolean disjunction
SRC=0010 && EXP=0 && RUN

# addition of independently multiplied terms
SRC=0011 && EXP=14 && RUN

# chained multiplication
SRC=0012 && EXP=24 && RUN

# mutable data declaration
SRC=0013 && EXP=2 && RUN

# assignments across data and variable declarations
SRC=0014 && EXP=0 && RUN

SRC=0015 && DIFF

SRC=0016 && DIFF

SRC=0017 && DIFF

SRC=0018 && DIFF

# nested inlined function calls
SRC=0019 && EXP=7 && RUN

# nested calls with repeated subexpressions
SRC=0020 && EXP=12 && RUN

# nested calls combine variables and function results
SRC=0021 && EXP=16 && RUN

# nested calls with local function temporaries
SRC=0022 && EXP=17 && RUN

# nested call result propagates through multiple functions
SRC=0023 && EXP=7 && RUN

# chained unary negation
SRC=0024 && EXP=0 && RUN

# unary negation across equivalent expressions
SRC=0025 && EXP=0 && RUN

# nested unary subtraction
SRC=0026 && EXP=0 && RUN

SRC=0027 && EXP=0 && RUN

SRC=0028 && EXP=0 && RUN

# unary argument passed to a function result
SRC=0029 && EXP=0 && RUN

# unary argument with a register-bound parameter
SRC=0030 && EXP=0 && RUN

# grouped unary negation
SRC=0031 && EXP=0 && RUN

# unary negation with multiplication precedence
SRC=0032 && EXP=0 && RUN

# optimized nested unary multiplication
SRC=0033 && EXP=0 && RUN

# unary negation of data and variables
SRC=0034 && EXP=0 && RUN

SRC=0035 && DIFFINP2

# negated function result
SRC=0036 && EXP=0 && RUN

SRC=0037 && EXP=0 && RUN

# nested negated function calls
SRC=0038 && EXP=0 && RUN

SRC=0039 && EXP=0 && RUN

# complement and negation composition
SRC=0040 && EXP=0 && RUN

# complement with arithmetic precedence
SRC=0041 && EXP=0 && RUN

# complement with register-bound function arguments
SRC=0042 && EXP=0 && RUN

# bitwise operators and their precedence
SRC=0043 && EXP=0 && RUN

# left and right shift operations
SRC=0044 && EXP=0 && RUN

# division, remainder, and arithmetic precedence
SRC=0045 && EXP=0 && RUN

# signed division with nested unary operands
SRC=0046 && EXP=0 && RUN

SRC=0047 && EXP=0 && RUN

# grouped negated boolean expressions
SRC=0048 && EXP=0 && RUN

# short-circuit boolean disjunction
SRC=0049 && EXP=0 && RUN

# short-circuit conjunction and disjunction
SRC=0050 && EXP=0 && RUN

# conjunction with explicit negation
SRC=0051 && EXP=0 && RUN

# scalar truthiness and conjunction
SRC=0052 && EXP=0 && RUN

# nested struct fields
SRC=0053 && EXP=0 && RUN

# assignments to narrow struct fields
SRC=0054 && EXP=0 && RUN

# mixed-width function parameters
SRC=0055 && EXP=0 && RUN

# signed unary results across integer widths
SRC=0056 && EXP=0 && RUN

# boolean comparisons and coercion
SRC=0057 && EXP=0 && RUN

# boolean function results
SRC=0058 && EXP=0 && RUN

# conditional result assignment
SRC=0059 && EXP=0 && RUN

# boolean inequality
SRC=0060 && EXP=0 && RUN

# function parameters mutate caller variables
SRC=0061 && EXP=0 && RUN

SRC=0062 && DIFFPY

# relational operators over booleans and integers
SRC=0063 && EXP=0 && RUN

# mixed arithmetic and bitwise precedence
SRC=0064 && EXP=0 && RUN

# nested struct initialization and copying
SRC=0065 && EXP=0 && RUN

# nested struct assignment and copying
SRC=0066 && EXP=0 && RUN

# arguments with unary values
SRC=0067 && EXP=0 && RUN

SRC=0068 && EXP=0 && RUN

# struct literal fields from function results
SRC=0069 && EXP=0 && RUN

# all functions are inlined
SRC=0070 && EXP=0 && RUN

# function mutation of caller data
SRC=0071 && EXP=0 && RUN

# constant boolean condition is folded
SRC=0072 && EXP=5 && RUN

SRC=0073 && EXP=0 && RUN

SRC=0074 && EXP=0 && RUN

# note: A.I. generated by Claude 4.5 with minor changes
SRC=0075 && EXP=0 && RUN

# note: A.I. generated by Claude 4.5 with minor change
SRC=0076 && EXP=0 && RUN

# complements applied to array elements
SRC=0077 && EXP=0 && RUN

# computed array indexes and element functions
SRC=0078 && EXP=0 && RUN

SRC=0079 && EXP=0 && RUN

# nested struct fields inside arrays
SRC=0080 && EXP=0 && RUN

# assignment and copying of nested structs
SRC=0081 && EXP=0 && RUN

# generated by Claude Sonnet 4.5
SRC=0082 && EXP=0 && RUN

# all functions are inlined
SRC=0083 && EXP=0 && RUN

# assignment between user-defined values
SRC=0084 && EXP=0 && RUN

# assignment between user-defined values
SRC=0085 && EXP=0 && RUN

SRC=0086 && DIFFINP2

SRC=0087 && DIFFINP2

SRC=0088 && DIFFINP2

SRC=0089 && EXP=0 && RUN

SRC=0090 && DIFF

SRC=0091 && DIFF

SRC=0092 && DIFFINP2

# block-local variable shadowing
SRC=0093 && EXP=0 && RUN

# block-local shadowing preserves the outer variable
SRC=0094 && EXP=0 && RUN

# escaped characters in string data
SRC=0095 && DIFF

# remove an input prefix with array_copy
SRC=0096 && DIFFINP2

# array_copy from an indexed struct element
SRC=0097 && EXP=0 && RUN

SRC=0098 && DIFFINP2

# function argument cannot read its own uninitialized caller variable
SRC=0099 && COMPERR

# conditional return assignment is not guaranteed
SRC=0100 && COMPERR

# variable cannot initialize itself
SRC=0101 && COMPERR

# array parameter mutation is visible to the caller
SRC=0102 && EXP=0 && RUN

# nested link mutation through a struct parameter
SRC=0103 && EXP=0 && RUN

# nested array mutation through chained parameters
SRC=0104 && EXP=0 && RUN

# array_copy of an indexed struct element
SRC=0105 && EXP=0 && RUN

# arrays_equal on nested link arrays
SRC=0106 && EXP=0 && RUN

# array and struct parameters share nested storage
SRC=0107 && EXP=0 && RUN

# array_length follows nested array fields
SRC=0108 && EXP=0 && RUN

SRC=0109 && EXP=0 && RUN

SRC=0110 && EXP=0 && RUN

SRC=0111 && EXP=0 && RUN

SRC=0112 && EXP=0 && RUN

# mixed-size struct layout and array stride
SRC=0113 && EXP=0 && RUN

# mutation through function references and return values
SRC=0114 && EXP=0 && RUN

# nested function mutation propagation
SRC=0115 && EXP=0 && RUN

# break and continue in nested loops
SRC=0116 && EXP=0 && RUN

SRC=0117 && EXP=0 && RUN

# state mutation across nested function calls
SRC=0118 && EXP=0 && RUN

# constant index past the end is rejected at compile
SRC=0119 && COMPERR

# negative constant index is rejected at compile
SRC=0120 && COMPERR

# recursive initialization is rejected, also inside a conversion
SRC=0121 && COMPERR

# return skips the required result assignment
SRC=0122 && COMPERR

# break skips the required result assignment
SRC=0123 && COMPERR

# continue does not leave the loop, the later break still skips the result
SRC=0124 && COMPERR

# variable cannot initialize itself
SRC=0125 && COMPERR

# all functions are inlined
SRC=0126 && EXP=0 && RUN

# indexed struct argument is passed by reference
SRC=0127 && EXP=0 && RUN

# indexed values participate in a shift expression
SRC=0128 && EXP=0 && RUN

# indexed values participate in multiplication
SRC=0129 && EXP=0 && RUN

# indexed values participate in division
SRC=0130 && EXP=0 && RUN

# indexed array values passed to a function
SRC=0131 && EXP=0 && RUN

# i32 array values passed to a typed function
SRC=0132 && EXP=0 && RUN

# typed array values passed to arithmetic functions
SRC=0133 && EXP=0 && RUN

# indexed value inside a function argument expression
SRC=0134 && EXP=0 && RUN

SRC=0135 && EXP=0 && RUN

SRC=0136 && EXP=0 && RUN

SRC=0137 && EXP=0 && RUN

SRC=0138 && EXP=0 && RUN

SRC=0139 && EXP=0 && RUN

SRC=0140 && EXP=0 && RUN

SRC=0141 && EXP=0 && RUN

# constant boolean expression folding
SRC=0142 && EXP=0 && RUN

SRC=0143 && EXP=0 && RUN

# == compares user-defined values
SRC=0144 && EXP=0 && RUN

# == compares arrays
SRC=0145 && EXP=0 && RUN

# boolean assignment from comparisons and integers
SRC=0146 && EXP=0 && RUN

# arithmetic and bitwise assignment operators
SRC=0147 && EXP=0 && RUN

SRC=0148 && EXP=0 && RUN

# constant identifiers in data initializers
SRC=0149 && EXP=0 && RUN

SRC=0150 && EXP=0 && RUN

# nested string-field mutation through functions
SRC=0151 && EXP=0 && RUN

SRC=0152 && EXP=0 && RUN

SRC=0153 && EXP=0 && RUN

# nested struct literal initialization
SRC=0154 && EXP=0 && RUN

# struct assignment from a function result
SRC=0155 && EXP=0 && RUN

# nested user-type array fields passed through functions
SRC=0156 && EXP=0 && RUN

# unterminated string is rejected
SRC=0157 && COMPERR

# too many initializers for a user type
SRC=0158 && COMPERR

# unterminated escaped string is rejected
SRC=0159 && COMPERR

# malformed decimal literal is rejected
SRC=0160 && COMPERR

# malformed hexadecimal literal is rejected
SRC=0161 && COMPERR

# malformed binary literal is rejected
SRC=0162 && COMPERR

# source and destination array sizes must match
SRC=0163 && COMPERR

# array initializer cannot fill a scalar field
SRC=0164 && COMPERR

# array initializers require commas
SRC=0165 && COMPERR

# assignment is not a boolean condition
SRC=0166 && COMPERR

# invalid comparison operator is rejected
SRC=0167 && COMPERR

# boolean operators require valid operands
SRC=0168 && COMPERR

# array_length requires an array argument
SRC=0169 && COMPERR

# array_copy requires matching array types
SRC=0170 && COMPERR

# arrays_equal requires matching array types
SRC=0171 && COMPERR

# function argument count must match
SRC=0172 && COMPERR

# duplicate constant declarations are rejected
SRC=0173 && COMPERR

# duplicate type declarations are rejected
SRC=0174 && COMPERR

# array type size requires a closing bracket
SRC=0175 && COMPERR

# break is only valid inside a loop
SRC=0176 && COMPERR

# continue is only valid inside a loop
SRC=0177 && COMPERR

# function parameters require a type delimiter
SRC=0178 && COMPERR

# type fields require a name
SRC=0179 && COMPERR

# array_length requires an opening parenthesis
SRC=0180 && COMPERR

# array_length requires a closing parenthesis
SRC=0181 && COMPERR

# arrays_equal arguments require commas
SRC=0182 && COMPERR

SRC=0183 && COMPERR

SRC=0184 && COMPERR

SRC=0185 && COMPERR

SRC=0186 && COMPERR

SRC=0187 && COMPERR

SRC=0188 && COMPERR

# unknown function calls are rejected
SRC=0189 && COMPERR

# function return value is ignored
SRC=0190 && COMPERR

# void function cannot produce a value
SRC=0191 && COMPERR

# scalar value cannot satisfy an array parameter
SRC=0192 && COMPERR

# dat requires a declared name
SRC=0193 && COMPERR

# strings require i8 arrays
SRC=0194 && COMPERR

# data array type requires a closing bracket
SRC=0195 && COMPERR

# data array initializer requires commas
SRC=0196 && COMPERR

# data array cannot exceed its declared size
SRC=0197 && COMPERR

# boolean data requires a boolean initializer
SRC=0198 && COMPERR

# type declarations require fields
SRC=0199 && COMPERR

# type fields require delimiters
SRC=0200 && COMPERR

# type fields require known types
SRC=0201 && COMPERR

# type field array requires a closing bracket
SRC=0202 && COMPERR

# function parameters require known types
SRC=0203 && COMPERR

# function results require known types
SRC=0204 && COMPERR

# unknown field access is rejected
SRC=0205 && COMPERR

# scalar indexing is rejected
SRC=0206 && COMPERR

# array index expression requires a closing bracket
SRC=0207 && COMPERR

# conditional expression requires a closing parenthesis
SRC=0208 && COMPERR

# == requires matching array sizes
SRC=0209 && COMPERR

# array_length result requires a wide destination
SRC=0210 && COMPERR

# array_copy requires parentheses
SRC=0211 && COMPERR

# array_copy arguments require commas
SRC=0212 && COMPERR

# array_copy requires a count argument
SRC=0213 && COMPERR

# arrays_equal arguments require commas
SRC=0214 && COMPERR

# array literal size requires a closing bracket
SRC=0215 && COMPERR

# constant declarations require a name
SRC=0216 && COMPERR

# constant declarations require an equals sign
SRC=0217 && COMPERR

# constant values require known identifiers
SRC=0218 && COMPERR

# variable array initializers require commas
SRC=0219 && COMPERR

# decimal constants must fit in i64
SRC=0220 && COMPERR

# hexadecimal constants must fit in i64
SRC=0221 && COMPERR

SRC=0222 && COMPERR

SRC=0223 && COMPERR

# function results must be assigned
SRC=0224 && COMPERR

# distinct user types cannot be assigned
SRC=0225 && COMPERR

# scalar values cannot be assigned to user types
SRC=0226 && COMPERR

# user type initializers must match their fields
SRC=0227 && COMPERR

SRC=0228 && COMPERR

# nested user-type array fields must match
SRC=0229 && COMPERR

# == compares indexed byte values
SRC=0230 && EXP=0 && RUN

# arrays_equal compares indexed byte ranges
SRC=0231 && EXP=0 && RUN

# unary negation of an array element is rejected here
SRC=0232 && COMPERR

# == compares indexed i32 values
SRC=0233 && EXP=0 && RUN

# incomplete addition is rejected
SRC=0234 && COMPERR

# a call rejects a trailing comma
SRC=0235 && COMPERR

# function calls reject a trailing comma
SRC=0236 && COMPERR

# function arguments require commas
SRC=0237 && COMPERR

# array index expressions require a closing bracket
SRC=0238 && COMPERR

# a string cannot exceed the size of its data field
SRC=0239 && COMPERR

# a data array initializer is complete without '{}', a value after it is
# unexpected
SRC=0240 && COMPERR

# user-type array initializers cannot exceed their size
SRC=0241 && COMPERR

# user-type fields require structured initializers
SRC=0242 && COMPERR

# == compares indexed i16 values
SRC=0243 && EXP=0 && RUN

# arrays_equal compares i32 array values
SRC=0244 && EXP=0 && RUN

# distinct user types cannot be passed to a function
SRC=0245 && COMPERR

# unary operations are forbidden on arrays_equal
SRC=0246 && COMPERR

# a whole array is not compared with a number
SRC=0247 && COMPERR

# arrays_equal requires parentheses
SRC=0248 && COMPERR

# arrays_equal requires a closing parenthesis
SRC=0249 && COMPERR

# fixed-size arrays accept empty initializers
SRC=0250 && EXP=0 && RUN

# inferred empty array is rejected
SRC=0251 && COMPERR

# fixed-size array may use an empty initializer
SRC=0252 && EXP=0 && RUN

# fixed-size array assignment may use an empty initializer
SRC=0253 && EXP=0 && RUN

# array literals cannot be passed as function arguments
SRC=0254 && COMPERR

# function shift by a parameter expression
SRC=0255 && EXP=0 && RUN

# missing initializer delimiter is rejected
SRC=0256 && COMPERR

# signed sixteen-bit division
SRC=0257 && EXP=0 && RUN

# signed eight-bit division
SRC=0258 && EXP=0 && RUN

# string escape decoding
SRC=0259 && DIFF

# hexadecimal string escape decoding
SRC=0260 && DIFF

# unary operation on array_length is rejected
SRC=0261 && EXP=0 && RUN

# array_length requires an array
SRC=0262 && COMPERR

# a data array initializer is complete without '{}', a scalar after it is
# unexpected
SRC=0263 && COMPERR

# scalar source cannot be assigned to an array
SRC=0264 && COMPERR

# data array initializer closes each element
SRC=0265 && EXP=0 && RUN

# malformed single-statement if block inside a braced function
SRC=0266 && COMPERR

# statement requires an assignment operator
SRC=0267 && COMPERR

# data array initializer must close
SRC=0268 && COMPERR

# array_copy requires a closing parenthesis
SRC=0269 && COMPERR

# assignment applies unary negation before storing
SRC=0270 && EXP=0 && RUN

# constant-sized array declaration
SRC=0271 && EXP=0 && RUN

# negative constant array size is rejected
SRC=0272 && COMPERR

# negating a negative size produces a valid array
SRC=0273 && EXP=0 && RUN

SRC=0274 && COMPERR

# spaced unary negative constant
SRC=0275 && COMPERR

SRC=0276 && EXP=0 && RUN

# arbitrary whitespace between tokens
SRC=0277 && EXP=0 && RUN

# unknown function call is rejected
SRC=0278 && COMPERR

# duplicate function definitions are rejected
SRC=0279 && COMPERR

# unknown field access is rejected
SRC=0280 && COMPERR

# nested expression deeper than the eight x86 scratch registers r8-r15
SRC=0281 && EXP=0 && RUN

# constant declaration requires a value
SRC=0282 && COMPERR

# negative type-level array size is rejected
SRC=0283 && COMPERR

# indexing an instance element of an array parameter
SRC=0284 && EXP=0 && RUN

# direct default-type array indexing
SRC=0285 && EXP=0 && RUN_NO_CHECKS

# nested array and field indexing
SRC=0286 && EXP=0 && RUN

# incompatible user types cannot be passed
SRC=0287 && COMPERR

# function returns a constructed value
SRC=0288 && EXP=0 && RUN

# function transforms a returned value
SRC=0289 && EXP=0 && RUN

# nested returned values build a structure
SRC=0290 && EXP=0 && RUN

# function result initializes a struct field
SRC=0291 && EXP=0 && RUN

# struct assignment from a function result
SRC=0292 && EXP=0 && RUN

# indexed struct field receives a function result
SRC=0293 && EXP=0 && RUN

# void function results cannot be negated
SRC=0294 && COMPERR

# narrowing conversion preserves the low byte
SRC=0295 && EXP=0 && RUN

# partial array and instance initializers zero-fill the rest
SRC=0296 && EXP=0 && RUN

# boolean values can be copied
SRC=0297 && EXP=0 && RUN

# dotted field used as an array index
SRC=0298 && EXP=0 && RUN

# indexed field passed by reference
SRC=0299 && EXP=0 && RUN

# indexing a non-array field must fail
SRC=0300 && COMPERR

# valid lower array boundary
SRC=0301 && EXP=0 && RUN

# valid upper array boundary
SRC=0302 && EXP=0 && RUN

# dynamic upper-bound failure
SRC=0303 && EXP=255 && RUN_ERR

# dynamic lower-bound read failure
SRC=0304 && EXP=255 && RUN_ERR

# constant outer nested index out of bounds is rejected at compile
SRC=0305 && COMPERR

# constant inner nested index out of bounds is rejected at compile
SRC=0306 && COMPERR

# arithmetic upper-bound failure
SRC=0307 && EXP=255 && RUN_ERR

# lower-bound check without upper check
SRC=0308 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower,line"

# upper-bound check without lower check
SRC=0309 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=upper,line"

# panic line from a multiline index
SRC=0310 && EXP=255 && RUN_ERR

# parenthesized in-range index
SRC=0311 && EXP=0 && RUN

# arithmetic underflow
SRC=0312 && EXP=255 && RUN_ERR

# arithmetic overflow
SRC=0313 && EXP=255 && RUN_ERR

# extreme index exposes NASM overflow
if [[ $MACHINE == x86_64 ]]; then SRC=0314 && EXP=255 && RUN_ERR; fi

# out-of-range index returned by a function
SRC=0315 && EXP=255 && RUN_ERR

# valid boundary of a one-element array
SRC=0316 && EXP=0 && RUN

# one-element lower failure without line output
SRC=0317 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower"

# one-element upper failure without line output
SRC=0318 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=upper"

# signed eight-bit index remains negative
SRC=0319 && EXP=255 && RUN_ERR

# signed sixteen-bit index remains negative
SRC=0320 && EXP=255 && RUN_ERR

# signed thirty-two-bit index remains negative
SRC=0321 && EXP=255 && RUN_ERR

# signed default-type index remains negative
SRC=0322 && EXP=255 && RUN_ERR

# constant index past the end of an array argument is rejected at compile
SRC=0323 && COMPERR

# element access checks a computed index
SRC=0324 && EXP=255 && RUN_ERR

# negative constant index into an array argument is rejected at compile
SRC=0325 && COMPERR

# valid upper boundary through an array parameter
SRC=0326 && EXP=0 && RUN

# combined bounds checks without line output
SRC=0327 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=upper,lower"

# unary negative constant index is rejected at compile
SRC=0328 && COMPERR

# computed inner index in a nested array
SRC=0329 && EXP=255 && RUN_ERR

# valid upper boundary through a computed index
SRC=0330 && EXP=0 && RUN

# valid default-type element scaling
SRC=0331 && EXP=0 && RUN

# valid index returned by a function
SRC=0332 && EXP=0 && RUN

# zero-length array must reject index zero
SRC=0333 && COMPERR

# valid two-byte element scaling
SRC=0334 && EXP=0 && RUN

# valid four-byte element scaling
SRC=0335 && EXP=0 && RUN

# minimum signed narrow index must remain negative
SRC=0336 && EXP=255 && RUN_ERR

# bounds checking must resolve an index from a field
SRC=0337 && EXP=255 && RUN_ERR

# computed index must respect array-parameter bounds
SRC=0338 && EXP=255 && RUN_ERR

# an empty array cannot be declared, so its element cannot be accessed
SRC=0339 && COMPERR

# explicit zero-sized variable arrays are invalid
SRC=0340 && COMPERR

# explicit zero-sized data arrays are invalid
SRC=0341 && COMPERR

# empty string fills a sized dat array with zeros
SRC=0342 && EXP=0 && RUN

# assign an empty string to a sized array variable
SRC=0343 && EXP=0 && RUN

# empty string is invalid without an explicit array size
SRC=0344 && COMPERR

# boolean result cannot be assigned to an integer variable
SRC=0345 && COMPERR

# boolean not applied to arrays_equal
SRC=0346 && EXP=0 && RUN

# boolean values cannot be arithmetic operands
SRC=0347 && COMPERR

# boolean arrays_equal result used in non-boolean arithmetic argument
SRC=0348 && COMPERR

# missing field initializer delimiter is rejected
SRC=0349 && COMPERR

# user types with identical fields are still incompatible
SRC=0350 && COMPERR

# array initializer element types must match
SRC=0351 && COMPERR

# scalar values cannot initialize array fields
SRC=0352 && COMPERR

# dotted function names are rejected
SRC=0353 && COMPERR

# array_copy handles a byte remainder
SRC=0354 && EXP=0 && RUN

# unknown field after array indexing is rejected
SRC=0355 && COMPERR

# empty brace data arrays require a size
SRC=0356 && COMPERR

# too many data array elements are rejected before parsing the next element
SRC=0357 && COMPERR

# arrays_equal writes its result to a memory field
SRC=0358 && EXP=0 && RUN

# mixed width values and indexed addressing
SRC=0359 && EXP=0 && RUN

# too many elements in a user type array
SRC=0360 && COMPERR

# truncate a qword to byte word and dword
if [[ $MACHINE == x86_64 ]]; then SRC=0361 && EXP=0 && RUN; fi

# empty arithmetic parentheses
SRC=0362 && COMPERR

# nested empty arithmetic parentheses
SRC=0363 && COMPERR

# constants shadow across nested blocks
SRC=0364 && EXP=0 && RUN

# duplicate constants in one block are rejected
SRC=0365 && COMPERR

# global variable shadowing in a nested block
SRC=0366 && EXP=0 && RUN

# global dat must precede global vars
SRC=0367 && COMPERR

SRC=0368 && EXP=0 && RUN

# foo over an array of user types
SRC=0369 && EXP=0 && RUN

# foo over a user type with an array field
SRC=0370 && EXP=0 && RUN

# foo over a holder with an array of user types
SRC=0371 && EXP=0 && RUN

# foo over an indexed array of user types
SRC=0372 && EXP=0 && RUN

# foo over an array field of an indexed holder argument
SRC=0373 && EXP=0 && RUN

# foo over an array field of an indexed holder array argument
SRC=0374 && EXP=0 && RUN

# foo over a points array in an indexed holder
SRC=0375 && EXP=0 && RUN

# foo requires an array identifier
SRC=0376 && COMPERR

# foo requires an array identifier
SRC=0377 && COMPERR

# foo loop variable passed to a mutating function
SRC=0378 && EXP=0 && RUN

# foo loop variable indexes its data field
SRC=0379 && EXP=0 && RUN

# foo passes a user type field to a mutating function
SRC=0380 && EXP=0 && RUN

# foo indexes an array of user types in a user-type field
SRC=0381 && EXP=0 && RUN

# increment indexes an array member of its user-type argument
SRC=0382 && EXP=0 && RUN

# foo and increment both index through nested user types
SRC=0383 && EXP=0 && RUN

# variable array initialization for built-in and user types
SRC=0384 && EXP=0 && RUN

# aggregate assignment with an array field
SRC=0385 && EXP=0 && RUN

# array variable assignment
SRC=0386 && EXP=0 && RUN

# local array initializes an array of types with array fields
SRC=0387 && EXP=0 && RUN

SRC=0388 && EXP=0 && RUN

# regression: ident_info field destination and alias lea propagation
SRC=0389 && EXP=0 && RUN

# regression: nested destinations plus by-reference call semantics
SRC=0390 && EXP=0 && RUN

# regression: shorthand bool checks must compare using scalar-width regs
SRC=0391 && EXP=0 && RUN

SRC=0392 && EXP=0 && RUN

SRC=0393 && EXP=0 && RUN

# == compares indexed default-type values (true and false paths)
SRC=0394 && EXP=0 && RUN

# shorthand bool truthiness with unary '~' across scalar widths
SRC=0395 && EXP=0 && RUN

# regression: stmt_call alias must pin indexed operand (is_indexed on)
SRC=0396 && EXP=0 && RUN

# regression: stmt_call alias plain variables (is_indexed off)
SRC=0397 && EXP=0 && RUN

# regression: user-type return and user-type indexed argument aliasing
SRC=0398 && EXP=0 && RUN

# lea regression: built-in indexed src/dst aliases and index mutation
SRC=0399 && EXP=0 && RUN

# lea regression: user-type indexed args (non-encodable element size path)
SRC=0400 && EXP=0 && RUN

# lea regression: nested field writes through user-type alias with lea path
SRC=0401 && EXP=0 && RUN

# lea regression: user-type return assigned to indexed dst from indexed src
SRC=0402 && EXP=0 && RUN

# branch coverage: stmt_identifier bounds check on array without indexing
# targets compile_effective_address branch: if (is_last and not
# reg_size.empty() and curr_info.is_array)
SRC=0403 && EXP=0 && RUN

# branch coverage: decouple_impl recursive assign for non-builtin fields
# targets compile_assign branch: if (not tf.type().is_built_in())
# e.compile_assign(...)
SRC=0404 && EXP=0 && RUN

# branch coverage: built-in array field assigned from array identifier targets
# compile_assign branches: if (tf.is_array and src.is_array_identifier())
# validate_array_assignment if (tf.is_array) { validate_array_assignment;
# x.copy(...); }
SRC=0405 && EXP=0 && RUN

# resolver path: alias chain with depth growth and no lea
SRC=0406 && EXP=0 && RUN

# resolver path: alias chain with lea + indexed user-type argument
SRC=0407 && EXP=0 && RUN

# register names are ordinary identifiers on every target
SRC=0408 && EXP=0 && RUN

# signed byte multiplication: wide results expose missing sign extension
SRC=0409 && EXP=0 && RUN

# non-inline reference arguments: narrow writes, globals and inline forwarding
SRC=0410 && EXP=0 && RUN

# non-inline return destinations: nested calls, pointer forwarding and early
# returns
SRC=0411 && EXP=0 && RUN

# non-inline print_num: repeated calls, local arrays and inline syscall
# helpers
SRC=0412 && DIFF

# non-inline recursion: factorial results and caller-local preservation
SRC=0413 && EXP=0 && RUN

# non-inline recursion: factorial results and caller-local preservation
SRC=0413 && EXP=0 && OPTS="--vars=0x40000 --checks=frame --reproduce-source" RUN

# non-inline recursion: factorial results and caller-local preservation
SRC=0413 && EXP=255 && RUN_ERR_OPTS "--vars=0x40 --checks=frame"

# non-inline user types: nested fields, reference mutation, copies and returns
SRC=0414 && EXP=0 && RUN

# non-inline user types: nested fields, reference mutation, copies and returns
SRC=0414 && EXP=0 && OPTS="--vars=0x40000 --checks=frame --reproduce-source" RUN

# non-inline return value cannot be discarded
SRC=0415 && COMPERR

# non-inline void call cannot supply a value
SRC=0416 && COMPERR

# unary operator on a non-inline call is applied to its result
SRC=0417 && EXP=2 && RUN

# a non-inline result is written to an element of an array literal
SRC=0418 && EXP=0 && RUN

# non-inline result destination must have the declared type
SRC=0419 && COMPERR

# non-inline argument can be a computed expression
SRC=0420 && EXP=2 && RUN

# non-inline argument can have unary operators
SRC=0421 && EXP=4 && RUN

# non-inline argument can be a constant
SRC=0422 && EXP=7 && RUN

# a whole array is passed to a non-inline array parameter
SRC=0423 && EXP=0 && RUN

# non-inline argument must have the declared parameter type
SRC=0424 && COMPERR

# an element cannot be passed to a non-inline array parameter
SRC=0425 && COMPERR

# builtin exit with a constant status
SRC=0426 && EXP=42 && RUN

# copy an i32 scalar and exit with the copied value
SRC=0427 && EXP=42 && RUN

# write and read an indexed field in a 12-byte instance
SRC=0428 && EXP=42 && RUN

# reject redefinition of the reserved exit builtin
SRC=0429 && COMPERR

# portable read/write with byte counts, errors, and discarded results
# the uart ignores descriptors, so there are no descriptor errors
if [[ $MACHINE != rv32i-qemu && $MACHINE != rv32i-fpga ]]; then SRC=0430 && DIFFINP; fi

# read requires a descriptor and an array
SRC=0431 && COMPERR

# write requires an array, not an address
SRC=0432 && COMPERR

# foo over 8196-byte instances, beyond the RV32I addi immediate range
SRC=0433 && EXP=0 && RUN

# non-inline recursion, references, aggregate returns and large local frames
SRC=0434 && EXP=0 && RUN

# non-inline recursion, references, aggregate returns and large local frames
SRC=0434 && EXP=0 && OPTS="--vars=0x40000 --checks=upper,lower,line,frame --reproduce-source" RUN

# non-inline recursion, references, aggregate returns and large local frames
SRC=0434 && EXP=255 && RUN_ERR_OPTS "--vars=0x1020 --checks=frame"

# arrays with omitted element type use the target's default integer type
SRC=0435 && EXP=0 && RUN

# name-first return declarations with default, explicit and aggregate types
SRC=0436 && EXP=0 && RUN

# canonical boolean results passed directly to a register argument inspect
# from the project root: ./baz qa/coverage/tests/0438.baz --target=x86_64 >
# gen.s use --target=rv32i for the other backend; omit checks to isolate
# result emission x86: first arrays_equal ends with sete, second with setne in
# the argument register no extra result register, normalization, or xor should
# appear between it and assert rv32i: equality writes 1/0 directly; negation
# swaps these values without xori assert's own truth test is separate and
# still needed
SRC=0438 && EXP=0 && RUN

# canonical boolean results assigned to a memory variable inspect from the
# project root: ./baz qa/coverage/tests/0439.baz --target=x86_64 > gen.s use
# --target=rv32i for the other backend; omit checks to isolate result emission
# inspect the initialization and reassignment of b, separately from assert(b)
# x86: sete byte [b] for equality, setne byte [b] for negated equality rv32i:
# produce the final 0/1 value and sb it directly neither assignment needs a
# second normalization or an xor for negation
SRC=0439 && EXP=0 && RUN

# inline function bodies require braces even for one statement
SRC=0440 && COMPERR

# non-inline function bodies require braces across newlines too
SRC=0441 && COMPERR

# a function cannot omit its body at end of input
SRC=0442 && COMPERR

# braced functions retain single-statement if and loop bodies
SRC=0443 && EXP=0 && RUN

# colonless declarations retain default types, arrays and adjacent statements
SRC=0444 && EXP=0 && RUN

# colon syntax is no longer accepted for parameters
SRC=0445 && COMPERR

# colon syntax is no longer accepted for variables
SRC=0446 && COMPERR

# colon syntax is no longer accepted for fields
SRC=0447 && COMPERR

# colon syntax is no longer accepted for function results
SRC=0448 && COMPERR

# colon syntax is no longer accepted for data
SRC=0449 && COMPERR

# arrays follow names, with optional element types
SRC=0450 && EXP=0 && RUN

# the old type-after-brackets order is rejected, the initializer gives the
# type
SRC=0451 && COMPERR

# a type after the variable name is rejected, the initializer gives the type
SRC=0452 && COMPERR

# array brackets follow the field element type
SRC=0453 && COMPERR

# array brackets follow the parameter element type
SRC=0454 && COMPERR

# omitting identical memory copies preserves subsequent operations
SRC=0455 && EXP=0 && RUN

# structured address scales include non-powers of two and values above 255
SRC=0456 && EXP=0 && RUN

# constant last elements after non-constant elements decide the list only when
# they short-circuit it
SRC=0457 && EXP=0 && RUN

# constant nested lists as last elements branch to the enclosing true target
SRC=0458 && EXP=0 && RUN

# negated parenthesized constants are expressions
SRC=0459 && EXP=0 && RUN

# an 'else' assignment does not cover an 'if' branch that skips the result
SRC=0460 && COMPERR

# a conditional return skips the result assignment
SRC=0461 && COMPERR

# a conditional break skips the result assignment
SRC=0462 && COMPERR

# the result is read inside an 'if' branch before it is set
SRC=0463 && COMPERR

# the result is read inside a loop before it is set
SRC=0464 && COMPERR

# results set on every path stay accepted, including early returns and loops
SRC=0465 && EXP=0 && RUN

# 64-bit constants outside the sign-extended 32-bit immediate range
if [[ $MACHINE == x86_64 ]]; then SRC=0466 && EXP=0 && RUN; fi

# a field of the variable is read in its own initializer
SRC=0467 && COMPERR

# the variable is copied from itself in its own initializer
SRC=0468 && COMPERR

# the variable is passed to the function that initializes it
SRC=0469 && COMPERR

# the variable is read in an index of its own initializer
SRC=0470 && COMPERR

# a field of the result is read before the result is set
SRC=0471 && COMPERR

# assigning one field does not set the whole result
SRC=0472 && COMPERR

# nested result fields set one at a time and read after they are set
SRC=0473 && EXP=0 && RUN

# a field set in only one branch is not set after the 'if'
SRC=0474 && COMPERR

# fields set on every path through 'if', 'else' and early returns
SRC=0475 && EXP=0 && RUN

# constant indexes set every element of an array field
SRC=0476 && EXP=0 && RUN

# a constant index sets only its element
SRC=0477 && COMPERR

# a runtime index does not prove which element is set
SRC=0478 && COMPERR

# a field of the result is read before it is set
SRC=0479 && COMPERR

# a return inside 'foo' skips the result assignment
SRC=0480 && COMPERR

# a result that aliases an argument is rejected when it is checked for
SRC=0481 && OPTS="--checks=noub" COMPERR

# the destination is read by a later element of its own expression
SRC=0482 && EXP=0 && RUN

# an instance value reads the destination it assigns
SRC=0483 && COMPERR

# a boolean list reads the destination it assigns
SRC=0484 && EXP=0 && RUN

# a runtime index may already have written the element an instance value reads
SRC=0485 && COMPERR

# a narrower operand widens to the destination size in arithmetic
SRC=0486 && EXP=0 && RUN

# a comparison may not truncate a wider right-hand side
SRC=0487 && COMPERR

# a comparison may not truncate a constant
SRC=0488 && COMPERR

# an inline argument must have the declared parameter type
SRC=0489 && COMPERR

# 'i8(...)' 'i16(...)' 'i32(...)' narrow on purpose and the store truncates
SRC=0490 && EXP=0 && RUN

# narrowing a variable must be stated with the builtin
SRC=0491 && COMPERR

# a constant outside the signed range must be stated with the builtin
SRC=0492 && COMPERR

# an inline result names the destination so the types must match
SRC=0493 && COMPERR

# an instance literal assigns every element of an array-of-instances field
SRC=0494 && EXP=0 && RUN

# an array literal may not have more elements than the array
SRC=0495 && COMPERR

# an array field literal may not have more elements than the field
SRC=0496 && COMPERR

# an instance argument can be a temporary such as a literal
SRC=0497 && EXP=3 && RUN

# an instance argument can be a temporary such as a call result
SRC=0498 && EXP=3 && RUN

# a named constant argument is the caller's constant even when the callee
# declares a constant with the same name
SRC=0499 && EXP=0 && RUN

# constant arguments with unary operators are folded before aliasing
SRC=0500 && EXP=0 && RUN

# function names do not clash with registers, assembler keywords or internal
# labels
SRC=0501 && EXP=0 && RUN

# inline call labels are unique for any function name
SRC=0502 && EXP=0 && RUN

# constant argument must fit the parameter type
SRC=0503 && COMPERR

# constant argument with unary operators must fit the parameter type
SRC=0504 && COMPERR

# folding negation of the minimum constant wraps like the run-time negation
if [[ $MACHINE == x86_64 ]]; then SRC=0505 && EXP=0 && RUN; fi

# a negated variable argument must fit the parameter type
SRC=0506 && COMPERR

# a loop without 'break' is left only by 'return', so the result is set
SRC=0507 && EXP=0 && RUN

# a non-inline result can be an operand in an expression
SRC=0508 && EXP=2 && RUN

# 'read' and 'write' take an array, an optional element count and an optional
# start element, like 'pread'/'pwrite'
SRC=0509 && DIFFINP

# a run-time element count beyond the array panics
SRC=0510 && EXP=255 && RUN_ERR

# the start element plus the count must stay within the array
SRC=0511 && EXP=255 && RUN_ERR

# an element is not an array, the parameter would see the whole array's length
SRC=0512 && COMPERR

# the start is the 4th argument, not an element
SRC=0513 && COMPERR

# without a count the whole array is transferred, an instance array field too
SRC=0514 && DIFFINP

# a range may end exactly at the array end, an empty range may start there
SRC=0515 && DIFFINP

# a start past the array end panics
SRC=0516 && EXP=255 && RUN_ERR

# a negative start panics
SRC=0517 && EXP=255 && RUN_ERR

# a negative count with a start panics
SRC=0518 && EXP=255 && RUN_ERR

# a negative 'array_copy' count with start elements panics
SRC=0519 && EXP=255 && RUN_ERR

# a negative 'arrays_equal' count with start elements panics
SRC=0520 && EXP=255 && RUN_ERR

# constant multiplication with shifts and adds or subtracts matches the
# runtime multiply for many constants, operands and widths
SRC=0521 && EXP=0 && RUN
UB_ALIAS="--vars=0x40000 --checks=upper,lower,line,alias --reproduce-source"

# --checks=alias: an inline result may share storage with an argument
SRC=0522 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: an instance result may share storage with another element
SRC=0523 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a non-inline result may share storage with an argument
SRC=0524 && OPTS="$UB_ALIAS" COMPERR

# two by-reference arguments may share storage when one is 'mut'
SRC=0525 && COMPERR

# --checks=alias: an inline parameter resolves to the global it aliases
SRC=0526 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a global passed to a non-inline function that also names it
SRC=0527 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: calls whose references cannot share storage are accepted
SRC=0528 && EXP=0 && OPTS="$UB_ALIAS" RUN

# comments inside arithmetic expressions and call arguments
SRC=0529 && EXP=0 && RUN

# comments inside initializers, type definitions and parameter lists
SRC=0530 && EXP=0 && RUN

# comments in single statement blocks and boolean expressions
SRC=0531 && EXP=0 && RUN

# a '#' inside a string is not a comment and a comment needs no space
SRC=0532 && DIFF

# a comment at the end of the file without a final newline
SRC=0533 && EXP=0 && RUN

# nested expressions deeper than the nine x86 scratch registers that no instruction needs
SRC=0534 && EXP=0 && RUN

# x86: division needs 'rdx' while it holds a last resort scratch value
if [[ $MACHINE == x86_64 ]]; then SRC=0535 && COMPERR; fi

# x86: a variable shift needs 'rcx' while it holds a last resort scratch value
if [[ $MACHINE == x86_64 ]]; then SRC=0536 && COMPERR; fi

# x86: a nested expression deeper than all fourteen scratch registers
if [[ $MACHINE == x86_64 ]]; then SRC=0537 && COMPERR; fi
# SRC=0538 && EXP=0 && RUN # note: generates a huge file and is very slow

# natural alignment: words at multiples of 4 and half words at even offsets,
# padding between and after fields is zero so instances compare byte by byte
SRC=0539 && EXP=0 && RUN

# character literals are constants with the value of their byte
SRC=0540 && DIFF

# character literal without a character
SRC=0541 && COMPERR

# character literal with more than one character
SRC=0542 && COMPERR

# character literal with an unsupported escape
SRC=0543 && COMPERR

# character literal without the closing quote
SRC=0544 && COMPERR

# strings initialize and assign 'i8' arrays: the bytes are copied from
# read-only data and the rest of the array is zeroed
SRC=0545 && DIFF

# a string longer than the destination array
SRC=0546 && COMPERR

# only 'i8' arrays take strings
SRC=0547 && COMPERR

# a string is not an argument, it would refer to read-only data
SRC=0548 && COMPERR

# an unsized parameter has the size of the argument, which is too small
SRC=0549 && COMPERR

# an empty string cannot give a 'var' array its size
SRC=0550 && COMPERR

# a '{...}' initializer of an unsized parameter uses the argument size: the
# unlisted elements of the argument are zeroed
SRC=0551 && EXP=0 && RUN

# a '{...}' initializer of an unsized parameter cannot have more elements than
# the argument
SRC=0552 && COMPERR

# short strings are stored with immediates when that takes no more code than
# copying from read-only data, the constants are word aligned
SRC=0553 && DIFF

# string with an unsupported escape
SRC=0554 && COMPERR

# constant '{...}' elements are stored as packed bytes and 'array_copy' with a
# constant count copies like an assignment
SRC=0555 && EXP=0 && RUN

# a constant 'array_copy' count past the end of the array panics
SRC=0556 && EXP=255 && RUN_ERR

# methods: 'func list.add(x)' is called as 'lst.add(x)' with 'lst' as the
# implicit first parameter 'self'
SRC=0557 && EXP=0 && RUN

# method on a type that does not exist
SRC=0558 && COMPERR

# methods on built-in types are not supported
SRC=0559 && COMPERR

# a method may share its name with a field: 'p.x(3)' calls the method and
# 'p.x' is the field, also when a variable is named like its type
SRC=0560 && EXP=0 && RUN

# the receiver of a method cannot be a whole array
SRC=0561 && COMPERR

# a method name must be followed by the arguments
SRC=0562 && COMPERR

# arguments of a method are numbered without the receiver
SRC=0563 && COMPERR

# the receiver is a reference like the other arguments
SRC=0564 && COMPERR

# 'self' is the implicit first parameter of a method
SRC=0565 && COMPERR

# trailing whitespace ends at a newline: operators, brackets and parentheses
# may still start the next line
SRC=0566 && EXP=0 && RUN

# an instance literal field narrows only with 'i8(...)' etc, also for a plain
# identifier that is copied directly
SRC=0567 && COMPERR

# data initializers at the limits of their signed types are stored as written
SRC=0568 && EXP=0 && RUN

# an array data element outside the signed range of its type is rejected
SRC=0569 && COMPERR

# an instance data field is checked after its unary operations
SRC=0570 && COMPERR

# a scalar data value outside the signed range of its type is rejected
SRC=0571 && COMPERR

# constant multiplication by shifts and adds, e.g. the element size of an
# array index, keeps the original value until the last add or sub
SRC=0572 && EXP=0 && RUN

# an instance copied from an indexed array element copies one element and leaves
# the bytes after the destination untouched
SRC=0573 && EXP=0 && RUN

# an instance array field copied from an array of another size
SRC=0574 && COMPERR

# an instance copied from a whole array
SRC=0575 && COMPERR

# an instance array field copied from a single instance
SRC=0576 && COMPERR

# an array copied from an indexed array element
SRC=0577 && COMPERR

# 'array_length' of an indexed element
SRC=0578 && COMPERR

# a whole array read as a scalar is not its first element
SRC=0579 && COMPERR

# a field of an array is reached through an indexed element
SRC=0580 && COMPERR

# a whole array passed to a scalar parameter
SRC=0581 && COMPERR

# '==' of a whole array and an element
SRC=0582 && COMPERR

# aliases of array elements are read as the element
SRC=0583 && EXP=0 && RUN

# a trailing comma after the last field of a type
SRC=0584 && EXP=0 && RUN

# a trailing comma is followed by '}', not another comma
SRC=0585 && COMPERR

# rv32i bulk loops use words when the addresses prove more alignment than the
# type; tails and unaligned addresses keep smaller accesses
SRC=0586 && EXP=0 && RUN

# rv32i unrolled zero, copy and constant stores at an address inside a word
# take a byte and a halfword up to the word boundary, then words and a tail; a
# copy between different positions within a word keeps smaller accesses
SRC=0587 && EXP=0 && RUN

# rv32i zero, copy and compare loops at an address inside a word take a byte
# and a halfword head up to the loop width when every address is the same
# distance from it; a run-time count may end within the head
SRC=0588 && EXP=0 && RUN

# constant elements of an arithmetic list fold at compile at the width of the
# destination and give the same values as the run-time computation
# test-constant-folding.sh checks the instructions of lines marked '# folds'
SRC=0589 && EXP=0 && RUN

# a comparison may not truncate a folded constant
SRC=0590 && COMPERR

# a constant index in range is an offset known at compile, no index register
# is computed at run time test-constant-folding.sh checks the instructions of
# lines marked '# folds'
SRC=0591 && EXP=0 && RUN

# a constant range start past the end is rejected at compile
SRC=0592 && COMPERR

# a constant left side of a comparison may not truncate either
SRC=0593 && COMPERR

# '==' of a known size compares the qwords, then the remaining dword, word
# and byte; a difference in any byte of any part makes it false
SRC=0594 && EXP=0 && RUN

# constructors: 'func point.at(x, y) self' builds a 'point' in 'self' and is
# called as 'point.at(1, 2)'
SRC=0595 && EXP=0 && RUN

SRC=0596 && COMPERR

SRC=0597 && COMPERR

SRC=0598 && COMPERR

SRC=0599 && COMPERR

SRC=0600 && COMPERR

SRC=0601 && COMPERR

SRC=0602 && COMPERR

SRC=0603 && COMPERR

SRC=0604 && COMPERR

# an untyped variable has the type of its initializer: a comparison or a
# 'bool' operand is a 'bool', a single operand has its own type, several
# operands have the default type like constants test-deduced-types.sh checks
# the type on lines marked '# is TYPE', 'default' is the default type of the
# target
SRC=0605 && EXP=0 && RUN

# a typed instance literal must name the type of the destination
SRC=0606 && COMPERR

# an untyped variable cannot take its type from '{...}', e.g. 'point{1, 2}'
# names it
SRC=0607 && COMPERR

# an array literal must name the element type of the destination
SRC=0608 && COMPERR

# an array size in both the destination and the literal must agree
SRC=0609 && COMPERR

# an array literal without elements and without a size has no size
SRC=0610 && COMPERR

# an array literal with a size holds at most that many elements
SRC=0611 && COMPERR

# a type after the variable name is rejected, the initializer gives the type,
# e.g. 'var x = i32(0)'
SRC=0612 && COMPERR

# an array size after the variable name is rejected, an array literal gives
# the element type and size, e.g. 'var a = i8[4]{}'
SRC=0613 && COMPERR

# an unsized array parameter has no size for a copy in a new variable
SRC=0614 && COMPERR

# calling a method that does not exist reports the method, not a missing field
SRC=0615 && COMPERR

# data declarations take the type and size from the initializer like 'var'
SRC=0616 && EXP=0 && RUN

# a type after the data name is rejected, the initializer gives the type, e.g.
# 'dat x = i32(0)'
SRC=0617 && COMPERR

# an array size after the data name is rejected, an array literal gives the
# element type and size, e.g. 'dat a = i8[4]{}'
SRC=0618 && COMPERR

# a data conversion needs its closing parenthesis
SRC=0619 && COMPERR

# 'true' and 'false' are of type 'bool' and cannot be arithmetic operands
SRC=0620 && COMPERR

# an array literal without a type name has the default type which must match
# the destination
SRC=0621 && COMPERR

# a missing first argument of a user type is reported as missing rather than
# as a malformed instance value
SRC=0622 && COMPERR

# a call without a result inside a function with a result is a statement, not
# an assignment to the caller's destination
SRC=0623 && EXP=13 && RUN

# a result discarded inside a function with a result is not written to the
# caller's destination
SRC=0624 && COMPERR

# foo with a count visits the first 'count' elements
SRC=0625 && EXP=0 && RUN

# foo count larger than the array fails the upper bound check
SRC=0626 && EXP=255 && RUN_ERR

# negative foo count fails the lower bound check
SRC=0627 && EXP=255 && RUN_ERR

# the foo counter 'i' is read-only
SRC=0628 && COMPERR

# the foo counter 'i' is read-only also through a parameter
SRC=0629 && COMPERR

# data instance initializer requires the type name
SRC=0630 && COMPERR

# data built-in field initialized with braces
SRC=0631 && COMPERR

# data array initializer with trailing comma
SRC=0632 && COMPERR

# a backslash at the end of a line continues the string on the next line,
# leading whitespace of the next line is part of the string
SRC=0633 && EXP=0 && RUN

# --checks=alias: arguments sharing storage are allowed when both parameters
# are not 'mut', for inline and non-inline functions
SRC=0634 && EXP=0 && OPTS="$UB_ALIAS" RUN

# a parameter without 'mut' cannot be assigned
SRC=0635 && COMPERR

# a field of a parameter without 'mut' cannot be assigned
SRC=0636 && COMPERR

# 'array_copy' cannot copy into an array parameter without 'mut'
SRC=0637 && COMPERR

# 'read' cannot fill an array parameter without 'mut'
SRC=0638 && COMPERR

# the element of 'foo' over an array parameter without 'mut' cannot be
# assigned
SRC=0639 && COMPERR

# a parameter without 'mut' cannot be written through an inline callee
SRC=0640 && COMPERR

# a parameter without 'mut' cannot be passed to a 'mut' non-inline parameter
SRC=0641 && COMPERR

# a 'mut' parameter still conflicts with a read-only one
SRC=0642 && COMPERR

# --checks=alias: a parameter without 'mut' may not share storage with the
# result
SRC=0643 && OPTS="$UB_ALIAS" COMPERR

# 'mut' goes after the parameter name
SRC=0644 && COMPERR

# 'let' with a non-constant initializer is a 'var' that cannot be assigned
# after its initializer
SRC=0645 && EXP=0 && RUN

# a 'let' variable cannot be assigned
SRC=0646 && COMPERR

# a field of a 'let' variable cannot be assigned
SRC=0647 && COMPERR

# an element of a 'let' array cannot be assigned
SRC=0648 && COMPERR

# 'array_copy' cannot copy into a 'let' array
SRC=0649 && COMPERR

# 'read' cannot fill a 'let' array
SRC=0650 && COMPERR

# a 'let' variable cannot be passed to a 'mut' non-inline parameter
SRC=0651 && COMPERR

# a 'let' variable cannot be written through an inline callee
SRC=0652 && COMPERR

# a 'let' needs an initializer
SRC=0653 && COMPERR

# a file level 'let' variable cannot be assigned in a function
SRC=0654 && COMPERR

# a 'let' variable cannot be assigned in a function that is never called
SRC=0655 && COMPERR

# a parameter without 'mut' cannot be assigned in a function that is never
# called
SRC=0656 && COMPERR

# a method that writes its receiver cannot be called on a read-only argument,
# also in a function inlined into another
SRC=0657 && COMPERR

# a read-only argument passed to a function that writes it is rejected at the
# call, also inside a function that is never called
SRC=0658 && COMPERR

# the element 'e' of 'foo' writes the array, so passing a read-only array on
# to the function that walks it is rejected at the call
SRC=0659 && COMPERR

# 'read' writes its buffer, so passing a read-only array on to the function
# that reads into it is rejected at the call
SRC=0660 && COMPERR

# 'array_copy' writes its destination, so passing a read-only array on to the
# function that copies into it is rejected at the call
SRC=0661 && COMPERR

# 'array_copy' cannot copy into a read-only array parameter, also in a
# function that is never called
SRC=0662 && COMPERR

# 'read' cannot fill a read-only array parameter, also in a function that is
# never called
SRC=0663 && COMPERR

# a method without 'mut' cannot write 'self'
SRC=0664 && COMPERR

# a method without 'mut' cannot call a 'mut' method on 'self'
SRC=0665 && COMPERR

# a constructor builds 'self' so it needs no 'mut'
SRC=0666 && COMPERR

# 'mut' marks the receiver of a method, a function has none
SRC=0667 && COMPERR

# 'mut' before the receiver type lets a method write 'self', other methods
# work on read-only receivers, a function can still be named 'mut'
SRC=0668 && EXP=0 && RUN

# '==' compares the bytes of the fields, the padding of an instance is not
# compared: a constructor leaves the padding as the storage held it
SRC=0669 && EXP=0 && RUN

# an array literal with call elements writes each result at its element
SRC=0670 && EXP=0 && RUN

# 'array_length' is a constant at compile: comparisons, counts, indexes and
# arguments use its value test-constant-folding.sh checks the instructions of
# lines marked '# folds'
SRC=0671 && EXP=0 && RUN

# comparing two variables far from the start of the frame: on rv32i both
# addresses share one 'lui' and 'add', for 8, 16 and 32-bit variables
SRC=0672 && EXP=0 && RUN

# a constant added to or subtracted from a run-time index goes into the
# displacement when bounds checks are off, with checks on the sum is computed
# in the index register
SRC=0673 && EXP=0 && RUN

# a constant added to or subtracted from a run-time index goes into the
# displacement when bounds checks are off, with checks on the sum is computed
# in the index register
SRC=0673 && EXP=0 && RUN_NO_CHECKS

# dat items and variables are addressed from one base: on rv32i the base is
# the start of the variables so dat items have negative offsets
SRC=0674 && EXP=0 && RUN

# arguments that reach different bytes of one variable can be passed together
SRC=0675 && EXP=0 && RUN

# arguments with run-time indexes into one array may share storage
SRC=0676 && COMPERR

# a read-only variable can be initialized from an array of the default type
SRC=0677 && EXP=0 && RUN

# exit takes one argument
SRC=0678 && COMPERR

# a constant must be a parsable number
SRC=0679 && COMPERR

# convert requires a closing parenthesis
SRC=0680 && COMPERR

# a narrow destination computes a converted operand at the wide width
SRC=0681 && EXP=45 && RUN

# == requires arrays of the same element type
SRC=0682 && COMPERR

# a non-inline function reads an array parameter
SRC=0683 && EXP=0 && RUN

# a method name follows the dot
SRC=0684 && COMPERR

# a block requires its closing brace
SRC=0685 && COMPERR

# a built-in element of a data definition must be a constant
SRC=0686 && COMPERR

# an array field gets its initializer after a delimiter
SRC=0687 && COMPERR

# a call requires parentheses
SRC=0688 && COMPERR

# a constructor name follows the dot
SRC=0689 && COMPERR

# a constructor call requires parentheses
SRC=0690 && COMPERR

# constant operands that trap or wrap at run time are not folded
SRC=0691 && EXP=7 && RUN

# a function name has no fields
SRC=0692 && COMPERR

# read and write take 2 to 4 arguments
SRC=0693 && COMPERR

# read requires an array to fill
SRC=0694 && COMPERR

# a negative constant count is rejected at run time by the lower check
SRC=0695 && EXP=255 && RUN_ERR

# 'noinline' is a function name when parentheses follow
SRC=0696 && EXP=3 && RUN

# a field array size is required
SRC=0697 && COMPERR

# a string is not a constant operand of let
SRC=0698 && EXP=3 && RUN

# array_copy of an indexed struct element
SRC=0105 && EXP=0 && RUN_NO_CHECKS

# a let initializer starting with a symbol is not a constant
SRC=0699 && COMPERR

# read cannot fill a constant
SRC=0700 && COMPERR

# a negative count is rejected by the lower check alone
SRC=0701 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower"

# a constant count is range checked by the lower check alone
SRC=0702 && EXP=0 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower"

# a global passed to a non-inline function that also names it shares storage
SRC=0703 && COMPERR

# a constant passed to a mut parameter cannot be written
SRC=0704 && COMPERR

# an error in a method body names the method call
SRC=0705 && COMPERR

# a constant element decides a boolean list passed to an inline function
SRC=0706 && EXP=2 && RUN

# a type name followed by an operator is not a data value
SRC=0707 && COMPERR

# a read-only argument cannot be passed to a mut parameter of a method
SRC=0708 && COMPERR

# nested indexed addresses are folded into the array copy address registers
SRC=0709 && EXP=0 && RUN

# an index ending in a multiplied or divided constant gets no displacement
SRC=0710 && EXP=5 && RUN_NO_CHECKS

# adding the most negative value of the width stays an addition
SRC=0711 && EXP=0 && RUN

# an instance value reads the whole destination after writing a field
SRC=0712 && COMPERR

# a constant shift by a negative count is not folded, rv32i rejects it
if [[ $MACHINE == x86_64 ]]; then SRC=0713 && EXP=0 && RUN; else SRC=0713 && COMPERR; fi

# dat is only allowed in the global scope
SRC=0714 && COMPERR

# foo is a builtin iterator and cannot name a function
SRC=0715 && COMPERR

# a variable must fit in the vars section
SRC=0716 && OPTS="--vars=0x40" COMPERR

# a statement is not allowed at the top level
SRC=0717 && COMPERR

# main is compiled where the program starts
SRC=0718 && COMPERR

# a backslash before a crlf line end continues the string
SRC=0719 && EXP=0 && RUN

# bell and backspace escapes in strings and character literals
SRC=0720 && EXP=0 && RUN

# a character literal escape has one character
SRC=0721 && COMPERR

# a hex escape in a character literal has two digits
SRC=0722 && COMPERR

# a hex escape in a string has two hex digits
SRC=0723 && COMPERR

# an instance value starts with '{'
SRC=0724 && COMPERR

# a call result wider than the destination is narrowed explicitly
SRC=0725 && COMPERR

# x86: a division needs 'rdx' while the iterator of a loop holds it
if [[ $MACHINE == x86_64 ]]; then SRC=0726 && COMPERR; fi

# x86: the count of an 'array_copy' is computed while 'rcx' is reserved
if [[ $MACHINE == x86_64 ]]; then SRC=0727 && COMPERR; fi

# a system call inside nested loops keeps the loop register 'r11' of x86
SRC=0728 && DIFF

# rv32i: 'write' needs 'a1', which is held by a loop register
if [[ $MACHINE != x86_64 ]]; then SRC=0729 && COMPERR; fi

# rv32i: nested loops hold every scratch register
if [[ $MACHINE != x86_64 ]]; then SRC=0730 && COMPERR; fi

# rv32i: a constant copy size beyond the 32-bit address range
if [[ $MACHINE != x86_64 ]]; then SRC=0731 && OPTS="--vars=0x10000" COMPERR; fi

# rv32i: data elements must be 1, 2 or 4 bytes
if [[ $MACHINE != x86_64 ]]; then SRC=0732 && COMPERR; fi

# an unknown escape in a string constant
SRC=0733 && COMPERR

# rv32i: data beyond the 32-bit address range
if [[ $MACHINE == rv32i ]]; then SRC=0734 && COMPERR; fi

# copy between variables far from the variables base
SRC=0735 && EXP=7 && RUN

# and with zero and or with all bits yield constants
SRC=0736 && EXP=0 && RUN

# an array whose size exceeds the signed 64-bit range
SRC=0737 && COMPERR

# an instance field whose size exceeds the signed 64-bit range
SRC=0738 && COMPERR

# instance fields whose sizes together exceed the signed 64-bit range
SRC=0739 && COMPERR

# data arrays whose sizes together exceed the signed 64-bit range, rv32i
# rejects the first one as beyond its address range
if [[ $MACHINE == x86_64 ]]; then SRC=0740 && COMPERR; fi

# x86: a copy whose byte count exceeds the signed 64-bit range
if [[ $MACHINE == x86_64 ]]; then SRC=0741 && COMPERR; fi

# an unsupported escape after a line continuation is located on its own line
SRC=0742 && COMPERR

# rv32i: a 64-bit variable is wider than a register
if [[ $MACHINE != x86_64 ]]; then SRC=0743 && COMPERR; fi

# the largest constant shift counts of narrow values
SRC=0744 && EXP=1 && RUN

# a sum with a constant that does not fit one immediate
SRC=0745 && EXP=3 && RUN

# rv32i: loops nested so deeply that no scratch register is left for a far jump
if [[ $MACHINE != x86_64 ]]; then SRC=0746 && EXP=101 && RUN; fi

# rv32i: a remainder keeps its result while loop registers are saved
if [[ $MACHINE != x86_64 ]]; then SRC=0747 && EXP=2 && RUN; fi

# a range with a runtime start and count when only the upper limit is checked
SRC=0748 && OPTS="--vars=0x40000 --checks=upper --reproduce-source" DIFF

# rv32i: a constant shift count of a 32-bit value must be below 32
if [[ $MACHINE != x86_64 ]]; then SRC=0749 && COMPERR; fi

# x86: a constant shift count beyond the width is not folded
if [[ $MACHINE == x86_64 ]]; then SRC=0750 && EXP=7 && RUN; fi

# rv32i: a constant shift count of an 8-bit value must be below 8
if [[ $MACHINE != x86_64 ]]; then SRC=0751 && COMPERR; fi

# rv32i: a constant right shift count of a 16-bit value must be below 16
if [[ $MACHINE != x86_64 ]]; then SRC=0752 && COMPERR; fi

# a constant shift by zero changes nothing
SRC=0753 && EXP=5 && RUN

# a non-inline parameter shares storage with a global only if a caller passes it
SRC=0754 && EXP=21 && RUN

# a global reaches a global named by a callee through two non-inline calls
SRC=0755 && COMPERR

# a recursive non-inline call is checked once
SRC=0756 && EXP=0 && RUN

# an indexed argument may be the array element a non-inline body names
SRC=0757 && COMPERR

# a type or array initializer without '{}' is the same as with an empty '{}'
SRC=0758 && EXP=0 && RUN

# a built-in type initializer without '(0)' is the same as with '(0)'
SRC=0759 && EXP=0 && RUN

# a type name followed by an operator is not an initializer
SRC=0760 && COMPERR

# a non-inline function takes arrays of different lengths, one body each
SRC=0761 && EXP=0 && RUN

# a non-inline body uses its array parameter like an inline one
SRC=0762 && EXP=0 && RUN

# two array parameters, recursion, methods, a dat array and an uncalled
# function
SRC=0763 && EXP=0 && RUN
SRC=0763 && EXP=0 && OPTS="--vars=0x40000 --checks=frame --reproduce-source" RUN

# a constant index out of bounds is found for the lengths of one call
SRC=0764 && COMPERR

# a run-time index is checked against the length of each body
SRC=0765 && EXP=255 && RUN_ERR

# an array argument must have the element type of the parameter
SRC=0766 && COMPERR

# generic functions: type parameters replaced per list of type arguments
SRC=0767 && EXP=0 && RUN

# a generic function is called with type arguments when its arguments do not
# tell them
SRC=0768 && COMPERR

# a generic function takes as many type arguments as type parameters
SRC=0769 && COMPERR

# a type argument names a type
SRC=0770 && COMPERR

# the body of a generic function is checked per instance
SRC=0771 && COMPERR

# a type parameter has the kind 'type'
SRC=0772 && COMPERR

# generic types: constants and types as parameters, one method for all the
# instances of a type
SRC=0773 && EXP=0 && RUN

# an alias names a generic type
SRC=0774 && COMPERR

# a generic type takes as many arguments as parameters
SRC=0775 && COMPERR

# the argument of a constant parameter is a number or a constant
SRC=0776 && COMPERR

# a generic type is used with arguments
SRC=0777 && COMPERR

# the argument of a type parameter names a type
SRC=0778 && COMPERR

# a method of a generic type with a type parameter of its own
SRC=0779 && EXP=0 && RUN

# the type argument of a method is deduced from a variable, not from an
# expression
SRC=0780 && COMPERR

# the type arguments of a call end with '>'
SRC=0781 && COMPERR

# the type arguments of a call are not empty
SRC=0782 && COMPERR

# the arguments of an alias end with '>'
SRC=0783 && COMPERR

# the arguments of an alias are not empty
SRC=0784 && COMPERR

# an error in a method of a generic type names the alias that needs it
SRC=0785 && COMPERR

# also for a method defined after the alias
SRC=0786 && COMPERR

# an error in the fields of a generic type names the alias that needs it
SRC=0787 && COMPERR

# a generic parameter is declared once
SRC=0788 && COMPERR

# a generic parameter does not hide a type
SRC=0789 && COMPERR

# a generic type is not a type until an alias names an instance
SRC=0790 && COMPERR

# a generic type is not a value
SRC=0791 && COMPERR

# a generic function is not a value
SRC=0792 && COMPERR

# an alias does not take the name of a generic type
SRC=0793 && COMPERR

# the argument of a type parameter is not a constant
SRC=0794 && COMPERR

# the argument of a constant parameter is not a type
SRC=0795 && COMPERR

# the type argument of a call is not a constant
SRC=0796 && COMPERR

# a generic definition with a type literal, nested generics and recursion
SRC=0797 && EXP=0 && RUN

# a generic definition without its closing brace
SRC=0798 && COMPERR

# the label of a noinline generic instance does not clash with a noinline method
SRC=0799 && EXP=0 && RUN

# a generic parameter list has at least one parameter
SRC=0800 && COMPERR

# a generic parameter is a constant or has the kind 'type'
SRC=0801 && COMPERR

# generic parameters are separated by a comma
SRC=0802 && COMPERR

# a comma in a generic parameter list is followed by a parameter
SRC=0803 && COMPERR

# a parameter of a generic function does not hide a type
SRC=0804 && COMPERR

# a generic function cannot have the name of a function
SRC=0805 && COMPERR

# a function cannot have the name of a generic function
SRC=0806 && COMPERR

# a generic type cannot have the name of another generic type
SRC=0807 && COMPERR

# the constant argument of an alias must make a valid type
SRC=0808 && COMPERR

# a generic type is known where the alias is, not before
SRC=0809 && COMPERR

# a type parameter is not known outside its generic
SRC=0810 && COMPERR

# the arguments of an instance are checked like those of any function
SRC=0811 && COMPERR

# an alias is defined once
SRC=0812 && COMPERR

# a method of a generic type is defined once, the instance reports it
SRC=0813 && COMPERR

# a function that is not generic takes no type arguments
SRC=0814 && COMPERR

# also in an expression
SRC=0815 && COMPERR

# an alias names a generic type, not a type
SRC=0816 && COMPERR

# the type arguments of a call are deduced from variables, fields and elements
SRC=0817 && EXP=0 && RUN

# a literal does not tell the type
SRC=0818 && COMPERR

# the arguments of a deduced instance are checked like any other
SRC=0819 && COMPERR

# an array parameter does not tell the type
SRC=0820 && COMPERR

# a type name assigned is the zero value of that type
SRC=0821 && EXP=0 && RUN

# a type name alone is its zero value in an instance literal and in an index
SRC=0822 && EXP=0 && RUN

# a type name alone as an argument is a temporary, a zeroed instance
SRC=0823 && EXP=1 && RUN

# an array assigned without braces needs a size
SRC=0824 && COMPERR

# an array declared without braces needs a size
SRC=0825 && COMPERR

# a parameter has a name
SRC=0826 && COMPERR

# the parameters of a generic function are read where it is defined
SRC=0827 && COMPERR

# the type arguments of a call are deduced from the type it is assigned to
SRC=0828 && EXP=0 && RUN

# data larger than the address range of rv32i is rejected at its declaration
if [[ $MACHINE != x86_64 ]]; then SRC=0829 && COMPERR; fi

# user type instances and arrays are compared with == and !=
SRC=0830 && EXP=0 && RUN

# an instance of a user type is not a condition
SRC=0831 && COMPERR

# a user type instance is compared only with an instance of the same type
SRC=0832 && COMPERR

# a user type instance is not compared with a number by an operator
SRC=0833 && COMPERR

# 'int' names the default type, also as a type argument and a field type
SRC=0834 && EXP=0 && RUN

# 'int' is a type name, a type cannot be defined with it
SRC=0835 && COMPERR

# nested foo loops deep enough for x86_64 to count in memory
SRC=0836 && EXP=0 && RUN

# a copy of memory needs rsi, rdi and rcx, the last resort registers on x86_64,
# below nine live scratch values and the bounds checks
SRC=0837 && EXP=0 && RUN

# a constructor is called on a type parameter, T.at(...) builds a point in place
SRC=0838 && EXP=0 && RUN

# a constructor called on a type parameter must exist for its argument
SRC=0839 && COMPERR

# x86: running out of registers in nested inline calls reports who holds them and
# what a noinline frame would save
if [[ $MACHINE == x86_64 ]]; then SRC=0840 && COMPERR; fi

# an operator other than equality does not compare user type instances
SRC=0841 && COMPERR

# a whole array is not compared with an instance of its element type
SRC=0842 && COMPERR

# an instance is not compared with a whole array of its type
SRC=0843 && COMPERR

# a generic method needs type arguments when they cannot be deduced
SRC=0844 && COMPERR

# an array field of a data instance is initialized with braces
SRC=0845 && COMPERR

# a generic type without a body is rejected where the body should start
SRC=0846 && COMPERR

# a generic function without a body reports the '}' that closes nothing
SRC=0847 && COMPERR

# a type parameter is deduced from a later argument, nested calls are skipped
SRC=0848 && EXP=0 && RUN

# a type parameter is not deduced from an expression argument
SRC=0849 && COMPERR

# a type parameter is not deduced from an argument that is missing
SRC=0850 && COMPERR

# generics that are never instantiated are listed in the footer, 'test-cli.sh'
# checks it
SRC=0851 && EXP=0 && RUN

# x86: a function name longer than the register report is not wrapped
if [[ $MACHINE == x86_64 ]]; then SRC=0852 && COMPERR; fi

# the code ends with the jump of a loop, no function body follows it
SRC=0853 && EXP=3 && RUN

# two instances of a generic 'noinline' function have unique labels
SRC=0854 && EXP=6 && RUN

# a non-inline 'bool' result in conditions, expression and nested arguments
SRC=0855 && EXP=17 && RUN

# a non-inline argument that is narrowed to its parameter is rejected
SRC=0856 && COMPERR

# the unary operators of a non-inline call apply to its result
SRC=0857 && EXP=100 && RUN

# the receiver or first slot of a non-inline call is passed in a register
SRC=0858 && EXP=40 && RUN

# instance arguments that are not variables, to an inlined and a non-inline
# function
SRC=0859 && EXP=72 && RUN

# a type parameter is deduced from an instance literal, a constructor call and
# a call result
SRC=0860 && EXP=81 && RUN

# a bare instance literal does not tell the type parameter
SRC=0861 && COMPERR

# a noinline function that nothing calls leaves no code, a method that passes
# its own receiver on does not load it into the receiver register
SRC=0862 && DIFFPY

# a constant argument has the type of its parameter in an inlined function
SRC=0863 && COMPERR

# the same in a noinline function, the message is the same
SRC=0864 && COMPERR

# a constant argument is narrowed like a variable of its parameter's type
SRC=0865 && COMPERR

# a constant argument of a wider parameter, inlined and noinline
SRC=0866 && EXP=22 && RUN

# two constant arguments are compared like variables of their parameters' types
SRC=0867 && COMPERR

# a method of an alias clashes with the generic method defined before it
SRC=0868 && COMPERR

# a generic method clashes with the method of an alias defined before it
SRC=0869 && COMPERR

# an inlined recursion that does not end at compile time is rejected
SRC=0870 && COMPERR

# a result that aliases an argument compiles on every target
SRC=0871 && EXP=0 && OPTS="--vars=0x40000 --checks=upper,lower,line,-alias" RUN

# a delimiter where a definition should start is rejected
SRC=0872 && COMPERR

# a delimiter where a statement should start is rejected
SRC=0873 && COMPERR

# a delimiter where the single statement of a block should start is rejected
SRC=0874 && COMPERR

# a function name is an identifier
SRC=0875 && COMPERR

# a type without a name is rejected
SRC=0876 && COMPERR

# a global variable is not initialized with a call
SRC=0877 && COMPERR

# exit has no value to assign
SRC=0878 && COMPERR

# a generic type has its body right after its parameters
SRC=0879 && COMPERR

# a string cannot start a statement
SRC=0880 && COMPERR

# a string after the branch of an if cannot start a statement
SRC=0881 && COMPERR

# the destination of array_copy is an array
SRC=0882 && COMPERR

# the compared of arrays_equal is an array
SRC=0883 && COMPERR

# an array is not assigned the result of a call
SRC=0884 && COMPERR

# an expression is not an array argument
SRC=0885 && COMPERR

# a record literal initializes a global variable
SRC=0886 && EXP=3 && RUN

# a variable shadows the function of the same name
SRC=0887 && EXP=5 && RUN

# a data constant is not missing after a unary operator
SRC=0888 && COMPERR

# a data constant is not a string
SRC=0889 && COMPERR

# a boolean value is not a name
SRC=0890 && COMPERR

# a global variable is initialized with a boolean expression
SRC=0891 && EXP=3 && RUN

# a condition is not the call of a function without a result
SRC=0892 && COMPERR

# a comparison is not made with the call of a function without a result
SRC=0893 && COMPERR

# blocks nested more than 128 levels are rejected
SRC=0894 && COMPERR

# expressions nested more than 128 levels are rejected
SRC=0895 && COMPERR

# inlined calls nested more than 256 levels are rejected
SRC=0896 && COMPERR

# an array field is not initialized with the name of its element type
SRC=0897 && COMPERR

# a constant shadows the function of the same name
SRC=0898 && EXP=8 && RUN

# a call checked for aliasing inside the body of a function that is not inlined
SRC=0899 && EXP=0 && OPTS="--checks=noub" RUN

# an index constant that no element is that far from is rejected
if [[ $MACHINE == x86_64 ]]; then SRC=0900 && OPTS="--vars=0x40000" COMPERR; fi

# a comment on the last line without a line end
SRC=0901 && EXP=3 && RUN

# a conversion of a struct value is rejected
SRC=0902 && COMPERR

# an included file with an include of its own and the same file included twice
SRC=0903 && EXP=42 && RUN

# an error in an included file is reported with the name of the file
SRC=0904 && COMPERR

# an include after a definition is rejected
SRC=0905 && COMPERR

# a file that includes the file that includes it is parsed once
SRC=0906 && EXP=3 && RUN

# an included file that does not exist is rejected
SRC=0907 && COMPERR

# a definition in an included file is reported with its file at the clash
SRC=0908 && COMPERR

# an include without a file name in quotes is rejected
SRC=0909 && COMPERR

# an include of a directory is rejected
SRC=0910 && COMPERR

# an included file without content adds nothing
SRC=0911 && EXP=7 && RUN

# a unary operator before a hex or binary constant
SRC=0912 && EXP=0 && RUN

# the mark under a token counts characters, not bytes
SRC=0913 && COMPERR

# a constant shift count that is not a byte is written as one, rv32i rejects
# a count outside the width
if [[ $MACHINE == x86_64 ]]; then SRC=0914 && EXP=0 && RUN; fi

# a conversion truncates to its type also when the value is stored in a wider
# variable, and is computed at its own width in a narrow expression
SRC=0915 && EXP=0 && RUN

# a folded conversion keeps its type as the first element of an expression
SRC=0916 && EXP=0 && RUN

# a constant expression argument is folded at the width of its parameter
SRC=0917 && EXP=0 && RUN

# a string cannot span lines, the source is echoed in assembly comments
SRC=0918 && COMPERR

# --checks=alias: a 'foo' element is not assigned a value that reads the array
SRC=0919 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a 'foo' element assigned from itself, another array and a
# separate variable is accepted
SRC=0920 && EXP=0 && OPTS="$UB_ALIAS" RUN

# --checks=alias: a 'foo' element reading other fields of its variable is
# accepted, reading its own field is not
SRC=0921 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a result and an argument that are different fields are accepted
SRC=0922 && EXP=0 && OPTS="$UB_ALIAS" RUN

# --checks=alias: a result and an argument that are the same field conflict
SRC=0923 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a function that writes a 'mut' parameter must not read the
# data that the argument names
SRC=0924 && OPTS="$UB_ALIAS" COMPERR
SRC=0925 && OPTS="$UB_ALIAS" COMPERR
SRC=0926 && OPTS="$UB_ALIAS" COMPERR
SRC=0927 && OPTS="$UB_ALIAS" COMPERR
SRC=0928 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: arguments that are other bytes than the data the function
# reads are accepted
SRC=0929 && EXP=0 && OPTS="$UB_ALIAS" RUN
SRC=0930 && EXP=0 && OPTS="$UB_ALIAS" RUN
SRC=0931 && EXP=0 && OPTS="$UB_ALIAS" RUN

# --checks=alias: a call result written into data that the function reads
SRC=0932 && OPTS="$UB_ALIAS" COMPERR
SRC=0933 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a run-time element written and read at the same field
SRC=0934 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a run-time element written and read at different fields
SRC=0935 && EXP=0 && OPTS="$UB_ALIAS" RUN

# --checks=alias: a destination set from its own whole value
SRC=0936 && EXP=0 && OPTS="$UB_ALIAS" RUN

# a noinline body that fits alone is called inside an expression that holds
# registers, the dry run of the body at the call site must not run out of them
if [[ $MACHINE == x86_64 ]]; then SRC=0937 && EXP=15 && OPTS="$UB_ALIAS" RUN; fi
if [[ $MACHINE != x86_64 ]]; then SRC=0938 && EXP=31 && OPTS="$UB_ALIAS" RUN; fi

# a noinline body calls another noinline function inside an expression that
# holds registers
if [[ $MACHINE == x86_64 ]]; then SRC=0939 && EXP=15 && OPTS="$UB_ALIAS" RUN; fi
if [[ $MACHINE != x86_64 ]]; then SRC=0940 && EXP=31 && OPTS="$UB_ALIAS" RUN; fi

# --checks=-alias: indexes that differ at run time are accepted, whatever the
# order in the list
SRC=0941 && EXP=0 && OPTS="--vars=0x40000 --checks=noub,-alias" RUN
SRC=0941 && EXP=0 && OPTS="--vars=0x40000 --checks=-alias,alias" RUN
SRC=0941 && COMPERR

# --checks=alias: arrays of different element sizes that overlap are assumed to
# reach the same bytes
SRC=0942 && COMPERR

# --checks=alias: the aliasing is made inside the body of the function that
# passes a parameter and a global that name the same data
SRC=0943 && COMPERR

# --checks=alias: the same for non-inlined functions, the aliasing is made
# inside the body of the function that passes a parameter and a global
SRC=0944 && COMPERR

# --checks=alias: the element 'e' of a 'foo' is passed to a function that reads
# the array, which can be the element itself
SRC=0945 && COMPERR

# --checks=alias: the same for a non-inlined function
SRC=0946 && COMPERR

# --checks=alias: a call result is written into the element 'e' of a 'foo' and
# the function reads the array, which can be the element itself
SRC=0947 && COMPERR

# --checks=alias: the element 'e' of a 'foo' is passed to a function that reads
# another array, which is accepted
SRC=0948 && EXP=0 && RUN

# --checks=alias: the element 'e' of a 'foo' inside the element of another 'foo'
# reads another field of the outer array, which is accepted
SRC=0949 && EXP=0 && RUN

# --checks=alias: the element 'e' of a 'foo' inside the element of another 'foo'
# reads the same field of the outer array, which can be the element itself
SRC=0950 && COMPERR

# --checks=alias: three nested 'foo' read a field outside the arrays they
# iterate, which is accepted
SRC=0951 && EXP=0 && RUN

# --checks=alias: three nested 'foo' read the array the second one iterates,
# which can contain the element itself
SRC=0952 && COMPERR

# --checks=alias: three nested 'foo' read another field of the element of the
# array the second one iterates, which is accepted
SRC=0953 && EXP=0 && RUN

# --checks=alias: the element 'e' of a 'foo' inside another 'foo' is assigned
# from another field of the same inner array element, which is accepted
SRC=0954 && EXP=0 && RUN

# --checks=alias: two run-time indexes in a path, the read is of the same field
# of an element that both indexes can select
SRC=0955 && COMPERR

# --checks=alias: two run-time indexes in a path, the read is of another field
# of the elements, which is accepted
SRC=0956 && EXP=0 && RUN

# --checks=alias: the destination has two run-time indexes and the read one, the
# fields in the elements are different, which is accepted
SRC=0957 && EXP=0 && RUN

# --checks=alias: the destination has two run-time indexes and the read one, the
# read is of the same field that the destination can select
SRC=0958 && COMPERR

# --checks=alias: the destination has one run-time index and the read two, the
# fields in the elements are different, which is accepted
SRC=0959 && EXP=0 && RUN

# --checks=alias: the read has two run-time indexes and the destination one, the
# fields in the elements are different, which is accepted
SRC=0960 && EXP=0 && RUN

# disjoint elements forwarded through parameters do not share storage, which is
# accepted
SRC=0961 && EXP=0 && RUN

# the same element forwarded through parameters shares storage
SRC=0962 && COMPERR

# --checks=alias: the element of a 'foo' over a constant element field and the
# read of another field of an element at a run-time index, which is accepted
SRC=0963 && EXP=0 && RUN

# --checks=alias: the element of a 'foo' over a constant element field and the
# read of the same field of an element at a run-time index
SRC=0964 && COMPERR

# --checks=division: defined divisions pass
SRC=0965 && EXP=0 && RUN_ERR_OPTS "--checks=division"

# --checks=division: zero divisor
SRC=0966 && EXP=255 && RUN_ERR_OPTS "--checks=division"

# --checks=division: zero divisor of a remainder
SRC=0967 && EXP=255 && RUN_ERR_OPTS "--checks=division"

# --checks=division: minimum divided by -1
SRC=0968 && EXP=255 && RUN_ERR_OPTS "--checks=division"

# --checks=division: minimum of 'i8' divided by -1
SRC=0969 && EXP=255 && RUN_ERR_OPTS "--checks=division"

if [[ $MACHINE == x86_64 ]]; then
    # --checks=division: minimum of 'i64' divided by -1, a trap of the hardware
    SRC=0970 && EXP=255 && RUN_ERR_OPTS "--checks=division"
fi

# --checks=division,line: zero divisor with the line, the fpga prints nothing
SRC=0971 && EXP=255 && RUN_ERR_OPTS "--checks=division,line"

# --checks=division,line: minimum divided by -1 with the line
SRC=0972 && EXP=255 && RUN_ERR_OPTS "--checks=division,line"

# --checks=upper,division,line: a bounds failure next to the division handler
SRC=0973 && EXP=255 && RUN_ERR_OPTS "--checks=upper,division,line"

# --checks=upper,division,line: the failing one of two divisions
SRC=0974 && EXP=255 && RUN_ERR_OPTS "--checks=upper,division,line"

# --checks=shift: counts within the width pass
SRC=0975 && EXP=0 && RUN_ERR_OPTS "--checks=shift"

# --checks=shift: count not below the width
SRC=0976 && EXP=255 && RUN_ERR_OPTS "--checks=shift"

# --checks=shift: negative count
SRC=0977 && EXP=255 && RUN_ERR_OPTS "--checks=shift"

# --checks=shift: count not below the width of 'i32'
SRC=0978 && EXP=255 && RUN_ERR_OPTS "--checks=shift"

# --checks=shift,line: count not below the width with the line
SRC=0979 && EXP=255 && RUN_ERR_OPTS "--checks=shift,line"

# --checks=shift: constant count outside the width
SRC=0980 && OPTS="--checks=shift" COMPERR

# --checks=overlap: disjoint, adjacent, identical and shifted down ranges pass
SRC=0981 && EXP=0 && RUN_ERR_OPTS "--checks=overlap"

# --checks=overlap: destination starts inside the source
SRC=0982 && EXP=255 && RUN_ERR_OPTS "--checks=overlap"

# --checks=overlap: destination starts inside the source, runtime count
SRC=0983 && EXP=255 && RUN_ERR_OPTS "--checks=overlap"

# --checks=overlap,line: overlapping copy with the line
SRC=0984 && EXP=255 && RUN_ERR_OPTS "--checks=overlap,line"

# --checks=stack: recursion that fits in the stack
SRC=0985 && EXP=0 && OPTS="--checks=stack" RUN

# --checks=stack: the operating system stops an overflow, no check is emitted
if [[ $MACHINE == x86_64 || $MACHINE == rv32i ]]; then SRC=0986 && EXP=0 && OPTS="--checks=stack" RUN; fi

# --checks=stack: recursion deeper than the stack panics
if [[ $MACHINE == rv32i-qemu || $MACHINE == rv32i-fpga ]]; then SRC=0986 && EXP=255 && RUN_ERR_OPTS "--stack=0x100 --checks=stack"; fi

# --memory: the stack starts at the end of a smaller memory and fits, the
# emulator has more memory
if [[ $MACHINE == rv32i-fpga ]]; then SRC=0985 && EXP=0 && OPTS="--memory=0x20000 --stack=0x1000 --checks=stack" RUN; fi

# --memory: recursion deeper than the stack of a smaller memory panics
if [[ $MACHINE == rv32i-fpga ]]; then SRC=0986 && EXP=255 && RUN_ERR_OPTS "--memory=0x20000 --stack=0x100 --checks=stack"; fi

# --checks=lower,stack: the check keeps the registers the call still uses
SRC=0987 && EXP=0 && OPTS="--checks=lower,stack" RUN

# --checks=overflow: results at the limits of the widths and conversions
SRC=0988 && EXP=0 && OPTS="--checks=overflow" RUN

# --checks=overflow: signed overflow of each operation
SRC=0989 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=0990 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=0991 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=0992 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=0993 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"

# --checks=overflow,line: overflow with the line
SRC=0994 && EXP=255 && RUN_ERR_OPTS "--checks=overflow,line"

# --checks=overflow: constant expression that overflows
SRC=0995 && OPTS="--checks=overflow" COMPERR

# --checks=overflow: product by a constant and a narrow array element
SRC=0996 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=0997 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"

# --checks=overflow: 64 bit only on x86_64
if [[ $MACHINE == x86_64 ]]; then SRC=0998 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"; fi

# --checks=overflow: a bitwise operation with a constant that changes nothing
SRC=0999 && EXP=0 && OPTS="--checks=overflow" RUN

# --checks=overflow: the negation of the minimum as an operand of a bitwise operation
SRC=1000 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=1001 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"

# --checks=overflow: the negation of the minimum of a typed constant is rejected
SRC=1002 && OPTS="--checks=overflow" COMPERR

# --checks=overflow: negation and complement of typed constants that fit run
SRC=1003 && EXP=3 && OPTS="--checks=overflow" RUN

# a local of a callee with the name of a variable the caller passes as 'mut' is another variable
SRC=1004 && EXP=10 && RUN

# an array of bool initialized with conditions gives each label of its own
SRC=1005 && EXP=5 && RUN

# 'void' is not the type of a value
SRC=1006 && COMPERR

# --checks=overflow: the negation of the minimum of a narrow type in a sum or difference
SRC=1007 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
SRC=1008 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"

# a built-in call needs a ',' between the arguments
SRC=1009 && COMPERR

# --checks=overflow: the negation of a folded constant that is the minimum of i64 is rejected
if [[ $MACHINE == x86_64 ]]; then SRC=1010 && OPTS="--checks=overflow" COMPERR; fi

# an empty 'else' block is written back by --reproduce-source
SRC=1011 && EXP=0 && RUN

# the end of a range of two 64 bit values that sum beyond the maximum is out of bounds
if [[ $MACHINE == x86_64 ]]; then SRC=1012 && EXP=255 && RUN_ERR; fi

# a write of a count that added to the start passes the maximum of i64 is out of bounds
if [[ $MACHINE == x86_64 ]]; then SRC=1013 && EXP=255 && RUN_ERR; fi

# a read of a count that added to the start passes the maximum of i64 is out of bounds
if [[ $MACHINE == x86_64 ]]; then SRC=1014 && EXP=255 && RUN_ERR; fi

# the destination of a copy at the maximum of i32 with a count of one is out of bounds
SRC=1015 && EXP=255 && RUN_ERR

# a shift count wider than the value is narrowed, as any operand
if [[ $MACHINE == x86_64 ]]; then SRC=1016 && COMPERR; fi

# a shift count wider than the value is narrowed, as any operand
SRC=1017 && COMPERR

# x86: a shift count computed in 'rcx' while the other scratch registers hold values is a register error
if [[ $MACHINE == x86_64 ]]; then SRC=1018 && COMPERR; fi

# a shift count wider than the value in a condition is checked as a whole
SRC=1019 && EXP=255 && RUN_ERR_OPTS "--checks=shift,line"

# a variable declared from an expression has the type of its first element, the sum overflows as an i8
SRC=1020 && EXP=255 && RUN_ERR_OPTS "--checks=overflow"
