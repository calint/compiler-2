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
SRC=001 && EXP=58 && RUN

# scalar variable initialization and exit
SRC=002 && EXP=1 && RUN

# multiplication binds tighter than addition
SRC=003 && EXP=7 && RUN

# grouped boolean conjunction and disjunction
SRC=004 && EXP=0 && RUN

# boolean grouping with conjunction on both sides
SRC=005 && EXP=0 && RUN

# jump optimization patterns, the optimized output is compared with --nopt in
# 006.x86_64.diff and 006.rv32i.diff
SRC=006 && EXP=0 && DIFFNOPT

# negated comparison
SRC=007 && EXP=0 && RUN

# conjunction of independent comparisons
SRC=008 && EXP=0 && RUN

# chained boolean conjunction
SRC=009 && EXP=0 && RUN

# chained boolean disjunction
SRC=010 && EXP=0 && RUN

# addition of independently multiplied terms
SRC=011 && EXP=14 && RUN

# chained multiplication
SRC=012 && EXP=24 && RUN

# mutable data declaration
SRC=013 && EXP=2 && RUN

# assignments across data and variable declarations
SRC=014 && EXP=0 && RUN

SRC=015 && DIFF

SRC=016 && DIFF

SRC=017 && DIFF

SRC=018 && DIFF

# nested inlined function calls
SRC=019 && EXP=7 && RUN

# nested calls with repeated subexpressions
SRC=020 && EXP=12 && RUN

# nested calls combine variables and function results
SRC=021 && EXP=16 && RUN

# nested calls with local function temporaries
SRC=022 && EXP=17 && RUN

# nested call result propagates through multiple functions
SRC=023 && EXP=7 && RUN

# chained unary negation
SRC=024 && EXP=0 && RUN

# unary negation across equivalent expressions
SRC=025 && EXP=0 && RUN

# nested unary subtraction
SRC=026 && EXP=0 && RUN

SRC=027 && EXP=0 && RUN

SRC=028 && EXP=0 && RUN

# unary argument passed to a function result
SRC=029 && EXP=0 && RUN

# unary argument with a register-bound parameter
SRC=030 && EXP=0 && RUN

# grouped unary negation
SRC=031 && EXP=0 && RUN

# unary negation with multiplication precedence
SRC=032 && EXP=0 && RUN

# optimized nested unary multiplication
SRC=033 && EXP=0 && RUN

# unary negation of data and variables
SRC=034 && EXP=0 && RUN

SRC=035 && DIFFINP2

# negated function result
SRC=036 && EXP=0 && RUN

SRC=037 && EXP=0 && RUN

# nested negated function calls
SRC=038 && EXP=0 && RUN

SRC=039 && EXP=0 && RUN

# complement and negation composition
SRC=040 && EXP=0 && RUN

# complement with arithmetic precedence
SRC=041 && EXP=0 && RUN

# complement with register-bound function arguments
SRC=042 && EXP=0 && RUN

# bitwise operators and their precedence
SRC=043 && EXP=0 && RUN

# left and right shift operations
SRC=044 && EXP=0 && RUN

# division, remainder, and arithmetic precedence
SRC=045 && EXP=0 && RUN

# signed division with nested unary operands
SRC=046 && EXP=0 && RUN

SRC=047 && EXP=0 && RUN

# grouped negated boolean expressions
SRC=048 && EXP=0 && RUN

# short-circuit boolean disjunction
SRC=049 && EXP=0 && RUN

# short-circuit conjunction and disjunction
SRC=050 && EXP=0 && RUN

# conjunction with explicit negation
SRC=051 && EXP=0 && RUN

# scalar truthiness and conjunction
SRC=052 && EXP=0 && RUN

# nested struct fields
SRC=053 && EXP=0 && RUN

# assignments to narrow struct fields
SRC=054 && EXP=0 && RUN

# mixed-width function parameters
SRC=055 && EXP=0 && RUN

# signed unary results across integer widths
SRC=056 && EXP=0 && RUN

# boolean comparisons and coercion
SRC=057 && EXP=0 && RUN

# boolean function results
SRC=058 && EXP=0 && RUN

# conditional result assignment
SRC=059 && EXP=0 && RUN

# boolean inequality
SRC=060 && EXP=0 && RUN

# function parameters mutate caller variables
SRC=061 && EXP=0 && RUN

SRC=062 && DIFFPY

# relational operators over booleans and integers
SRC=063 && EXP=0 && RUN

# mixed arithmetic and bitwise precedence
SRC=064 && EXP=0 && RUN

# nested struct initialization and copying
SRC=065 && EXP=0 && RUN

# nested struct assignment and copying
SRC=066 && EXP=0 && RUN

# arguments with unary values
SRC=067 && EXP=0 && RUN

SRC=068 && EXP=0 && RUN

# struct literal fields from function results
SRC=069 && EXP=0 && RUN

# all functions are inlined
SRC=070 && EXP=0 && RUN

# function mutation of caller data
SRC=071 && EXP=0 && RUN

# constant boolean condition is folded
SRC=072 && EXP=5 && RUN

SRC=073 && EXP=0 && RUN

SRC=074 && EXP=0 && RUN

# note: A.I. generated by Claude 4.5 with minor changes
SRC=075 && EXP=0 && RUN

# note: A.I. generated by Claude 4.5 with minor change
SRC=076 && EXP=0 && RUN

# complements applied to array elements
SRC=077 && EXP=0 && RUN

# computed array indexes and element functions
SRC=078 && EXP=0 && RUN

SRC=079 && EXP=0 && RUN

# nested struct fields inside arrays
SRC=080 && EXP=0 && RUN

# assignment and copying of nested structs
SRC=081 && EXP=0 && RUN

# generated by Claude Sonnet 4.5
SRC=082 && EXP=0 && RUN

# all functions are inlined
SRC=083 && EXP=0 && RUN

# assignment between user-defined values
SRC=084 && EXP=0 && RUN

# assignment between user-defined values
SRC=085 && EXP=0 && RUN

SRC=086 && DIFFINP2

SRC=087 && DIFFINP2

SRC=088 && DIFFINP2

SRC=089 && EXP=0 && RUN

SRC=090 && DIFF

SRC=091 && DIFF

SRC=092 && DIFFINP2

# block-local variable shadowing
SRC=093 && EXP=0 && RUN

# block-local shadowing preserves the outer variable
SRC=094 && EXP=0 && RUN

# escaped characters in string data
SRC=095 && DIFF

# remove an input prefix with array_copy
SRC=096 && DIFFINP2

# array_copy from an indexed struct element
SRC=097 && EXP=0 && RUN

SRC=098 && DIFFINP2

# function argument cannot read its own uninitialized caller variable
SRC=099 && COMPERR

# conditional return assignment is not guaranteed
SRC=100 && COMPERR

# variable cannot initialize itself
SRC=101 && COMPERR

# array parameter mutation is visible to the caller
SRC=102 && EXP=0 && RUN

# nested link mutation through a struct parameter
SRC=103 && EXP=0 && RUN

# nested array mutation through chained parameters
SRC=104 && EXP=0 && RUN

# array_copy of an indexed struct element
SRC=105 && EXP=0 && RUN

# arrays_equal on nested link arrays
SRC=106 && EXP=0 && RUN

# array and struct parameters share nested storage
SRC=107 && EXP=0 && RUN

# array_length follows nested array fields
SRC=108 && EXP=0 && RUN

SRC=109 && EXP=0 && RUN

SRC=110 && EXP=0 && RUN

SRC=111 && EXP=0 && RUN

SRC=112 && EXP=0 && RUN

# mixed-size struct layout and array stride
SRC=113 && EXP=0 && RUN

# mutation through function references and return values
SRC=114 && EXP=0 && RUN

# nested function mutation propagation
SRC=115 && EXP=0 && RUN

# break and continue in nested loops
SRC=116 && EXP=0 && RUN

SRC=117 && EXP=0 && RUN

# state mutation across nested function calls
SRC=118 && EXP=0 && RUN

# constant index past the end is rejected at compile
SRC=119 && COMPERR

# negative constant index is rejected at compile
SRC=120 && COMPERR

# recursive initialization is rejected, also inside a conversion
SRC=121 && COMPERR

# return skips the required result assignment
SRC=122 && COMPERR

# break skips the required result assignment
SRC=123 && COMPERR

# continue does not leave the loop, the later break still skips the result
SRC=124 && COMPERR

# variable cannot initialize itself
SRC=125 && COMPERR

# all functions are inlined
SRC=126 && EXP=0 && RUN

# indexed struct argument is passed by reference
SRC=127 && EXP=0 && RUN

# indexed values participate in a shift expression
SRC=128 && EXP=0 && RUN

# indexed values participate in multiplication
SRC=129 && EXP=0 && RUN

# indexed values participate in division
SRC=130 && EXP=0 && RUN

# indexed array values passed to a function
SRC=131 && EXP=0 && RUN

# i32 array values passed to a typed function
SRC=132 && EXP=0 && RUN

# typed array values passed to arithmetic functions
SRC=133 && EXP=0 && RUN

# indexed value inside a function argument expression
SRC=134 && EXP=0 && RUN

SRC=135 && EXP=0 && RUN

SRC=136 && EXP=0 && RUN

SRC=137 && EXP=0 && RUN

SRC=138 && EXP=0 && RUN

SRC=139 && EXP=0 && RUN

SRC=140 && EXP=0 && RUN

SRC=141 && EXP=0 && RUN

# constant boolean expression folding
SRC=142 && EXP=0 && RUN

SRC=143 && EXP=0 && RUN

# == compares user-defined values
SRC=144 && EXP=0 && RUN

# == compares arrays
SRC=145 && EXP=0 && RUN

# boolean assignment from comparisons and integers
SRC=146 && EXP=0 && RUN

# arithmetic and bitwise assignment operators
SRC=147 && EXP=0 && RUN

SRC=148 && EXP=0 && RUN

# constant identifiers in data initializers
SRC=149 && EXP=0 && RUN

SRC=150 && EXP=0 && RUN

# nested string-field mutation through functions
SRC=151 && EXP=0 && RUN

SRC=152 && EXP=0 && RUN

SRC=153 && EXP=0 && RUN

# nested struct literal initialization
SRC=154 && EXP=0 && RUN

# struct assignment from a function result
SRC=155 && EXP=0 && RUN

# nested user-type array fields passed through functions
SRC=156 && EXP=0 && RUN

# unterminated string is rejected
SRC=157 && COMPERR

# too many initializers for a user type
SRC=158 && COMPERR

# unterminated escaped string is rejected
SRC=159 && COMPERR

# malformed decimal literal is rejected
SRC=160 && COMPERR

# malformed hexadecimal literal is rejected
SRC=161 && COMPERR

# malformed binary literal is rejected
SRC=162 && COMPERR

# source and destination array sizes must match
SRC=163 && COMPERR

# array initializer cannot fill a scalar field
SRC=164 && COMPERR

# array initializers require commas
SRC=165 && COMPERR

# assignment is not a boolean condition
SRC=166 && COMPERR

# invalid comparison operator is rejected
SRC=167 && COMPERR

# boolean operators require valid operands
SRC=168 && COMPERR

# array_length requires an array argument
SRC=169 && COMPERR

# array_copy requires matching array types
SRC=170 && COMPERR

# arrays_equal requires matching array types
SRC=171 && COMPERR

# function argument count must match
SRC=172 && COMPERR

# duplicate constant declarations are rejected
SRC=173 && COMPERR

# duplicate type declarations are rejected
SRC=174 && COMPERR

# array type size requires a closing bracket
SRC=175 && COMPERR

# break is only valid inside a loop
SRC=176 && COMPERR

# continue is only valid inside a loop
SRC=177 && COMPERR

# function parameters require a type delimiter
SRC=178 && COMPERR

# type fields require a name
SRC=179 && COMPERR

# array_length requires an opening parenthesis
SRC=180 && COMPERR

# array_length requires a closing parenthesis
SRC=181 && COMPERR

# arrays_equal arguments require commas
SRC=182 && COMPERR

SRC=184 && COMPERR

SRC=185 && COMPERR

SRC=186 && COMPERR

SRC=187 && COMPERR

SRC=188 && COMPERR

SRC=189 && COMPERR

# unknown function calls are rejected
SRC=190 && COMPERR

# function return value is ignored
SRC=191 && COMPERR

# void function cannot produce a value
SRC=192 && COMPERR

# scalar value cannot satisfy an array parameter
SRC=193 && COMPERR

# dat requires a declared name
SRC=194 && COMPERR

# strings require i8 arrays
SRC=195 && COMPERR

# data array type requires a closing bracket
SRC=196 && COMPERR

# data array initializer requires commas
SRC=197 && COMPERR

# data array cannot exceed its declared size
SRC=198 && COMPERR

# boolean data requires a boolean initializer
SRC=199 && COMPERR

# type declarations require fields
SRC=200 && COMPERR

# type fields require delimiters
SRC=201 && COMPERR

# type fields require known types
SRC=202 && COMPERR

# type field array requires a closing bracket
SRC=203 && COMPERR

# function parameters require known types
SRC=204 && COMPERR

# function results require known types
SRC=205 && COMPERR

# unknown field access is rejected
SRC=206 && COMPERR

# scalar indexing is rejected
SRC=207 && COMPERR

# array index expression requires a closing bracket
SRC=208 && COMPERR

# conditional expression requires a closing parenthesis
SRC=209 && COMPERR

# == requires matching array sizes
SRC=210 && COMPERR

# array_length result requires a wide destination
SRC=211 && COMPERR

# array_copy requires parentheses
SRC=212 && COMPERR

# array_copy arguments require commas
SRC=213 && COMPERR

# array_copy requires a count argument
SRC=214 && COMPERR

# arrays_equal arguments require commas
SRC=215 && COMPERR

# array literal size requires a closing bracket
SRC=216 && COMPERR

# constant declarations require a name
SRC=217 && COMPERR

# constant declarations require an equals sign
SRC=218 && COMPERR

# constant values require known identifiers
SRC=219 && COMPERR

# variable array initializers require commas
SRC=220 && COMPERR

# decimal constants must fit in i64
SRC=221 && COMPERR

# hexadecimal constants must fit in i64
SRC=222 && COMPERR

SRC=223 && COMPERR

SRC=224 && COMPERR

# function results must be assigned
SRC=225 && COMPERR

# distinct user types cannot be assigned
SRC=226 && COMPERR

# scalar values cannot be assigned to user types
SRC=227 && COMPERR

# user type initializers must match their fields
SRC=228 && COMPERR

SRC=229 && COMPERR

# nested user-type array fields must match
SRC=230 && COMPERR

# == compares indexed byte values
SRC=231 && EXP=0 && RUN

# arrays_equal compares indexed byte ranges
SRC=232 && EXP=0 && RUN

# unary negation of an array element is rejected here
SRC=233 && COMPERR

# == compares indexed i32 values
SRC=234 && EXP=0 && RUN

# incomplete addition is rejected
SRC=235 && COMPERR

# a call rejects a trailing comma
SRC=236 && COMPERR

# function calls reject a trailing comma
SRC=237 && COMPERR

# function arguments require commas
SRC=238 && COMPERR

# array index expressions require a closing bracket
SRC=239 && COMPERR

# a string cannot exceed the size of its data field
SRC=240 && COMPERR

# a data array initializer is complete without '{}', a value after it is
# unexpected
SRC=241 && COMPERR

# user-type array initializers cannot exceed their size
SRC=242 && COMPERR

# user-type fields require structured initializers
SRC=243 && COMPERR

# == compares indexed i16 values
SRC=244 && EXP=0 && RUN

# arrays_equal compares i32 array values
SRC=245 && EXP=0 && RUN

# distinct user types cannot be passed to a function
SRC=246 && COMPERR

# unary operations are forbidden on arrays_equal
SRC=248 && COMPERR

# a whole array is not compared with a number
SRC=251 && COMPERR

# arrays_equal requires parentheses
SRC=252 && COMPERR

# arrays_equal requires a closing parenthesis
SRC=253 && COMPERR

# fixed-size arrays accept empty initializers
SRC=254 && EXP=0 && RUN

# inferred empty array is rejected
SRC=255 && COMPERR

# fixed-size array may use an empty initializer
SRC=256 && EXP=0 && RUN

# fixed-size array assignment may use an empty initializer
SRC=257 && EXP=0 && RUN

# array literals cannot be passed as function arguments
SRC=258 && COMPERR

# function shift by a parameter expression
SRC=259 && EXP=0 && RUN

# missing initializer delimiter is rejected
SRC=260 && COMPERR

# signed sixteen-bit division
SRC=261 && EXP=0 && RUN

# signed eight-bit division
SRC=262 && EXP=0 && RUN

# string escape decoding
SRC=263 && DIFF

# hexadecimal string escape decoding
SRC=264 && DIFF

# unary operation on array_length is rejected
SRC=265 && EXP=0 && RUN

# array_length requires an array
SRC=266 && COMPERR

# a data array initializer is complete without '{}', a scalar after it is
# unexpected
SRC=267 && COMPERR

# scalar source cannot be assigned to an array
SRC=268 && COMPERR

# data array initializer closes each element
SRC=269 && EXP=0 && RUN

# malformed single-statement if block inside a braced function
SRC=270 && COMPERR

# statement requires an assignment operator
SRC=271 && COMPERR

# data array initializer must close
SRC=272 && COMPERR

# array_copy requires a closing parenthesis
SRC=273 && COMPERR

# assignment applies unary negation before storing
SRC=274 && EXP=0 && RUN

# constant-sized array declaration
SRC=275 && EXP=0 && RUN

# negative constant array size is rejected
SRC=276 && COMPERR

# negating a negative size produces a valid array
SRC=277 && EXP=0 && RUN

SRC=278 && COMPERR

# spaced unary negative constant
SRC=279 && COMPERR

SRC=280 && EXP=0 && RUN

# arbitrary whitespace between tokens
SRC=281 && EXP=0 && RUN

# unknown function call is rejected
SRC=282 && COMPERR

# duplicate function definitions are rejected
SRC=283 && COMPERR

# unknown field access is rejected
SRC=284 && COMPERR

# nested expression deeper than the eight x86 scratch registers r8-r15
SRC=285 && EXP=0 && RUN

# constant declaration requires a value
SRC=286 && COMPERR

# negative type-level array size is rejected
SRC=287 && COMPERR

# indexing an instance element of an array parameter
SRC=288 && EXP=0 && RUN

# direct default-type array indexing
SRC=289 && EXP=0 && RUN_NO_CHECKS

# nested array and field indexing
SRC=290 && EXP=0 && RUN

# incompatible user types cannot be passed
SRC=291 && COMPERR

# function returns a constructed value
SRC=292 && EXP=0 && RUN

# function transforms a returned value
SRC=293 && EXP=0 && RUN

# nested returned values build a structure
SRC=294 && EXP=0 && RUN

# function result initializes a struct field
SRC=295 && EXP=0 && RUN

# struct assignment from a function result
SRC=296 && EXP=0 && RUN

# indexed struct field receives a function result
SRC=297 && EXP=0 && RUN

# void function results cannot be negated
SRC=298 && COMPERR

# narrowing conversion preserves the low byte
SRC=299 && EXP=0 && RUN

# partial array and instance initializers zero-fill the rest
SRC=300 && EXP=0 && RUN

# boolean values can be copied
SRC=301 && EXP=0 && RUN

# dotted field used as an array index
SRC=302 && EXP=0 && RUN

# indexed field passed by reference
SRC=303 && EXP=0 && RUN

# indexing a non-array field must fail
SRC=304 && COMPERR

# valid lower array boundary
SRC=305 && EXP=0 && RUN

# valid upper array boundary
SRC=306 && EXP=0 && RUN

# dynamic upper-bound failure
SRC=307 && EXP=255 && RUN_ERR

# dynamic lower-bound read failure
SRC=308 && EXP=255 && RUN_ERR

# constant outer nested index out of bounds is rejected at compile
SRC=309 && COMPERR

# constant inner nested index out of bounds is rejected at compile
SRC=310 && COMPERR

# arithmetic upper-bound failure
SRC=311 && EXP=255 && RUN_ERR

# lower-bound check without upper check
SRC=312 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower,line"

# upper-bound check without lower check
SRC=313 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=upper,line"

# panic line from a multiline index
SRC=314 && EXP=255 && RUN_ERR

# parenthesized in-range index
SRC=315 && EXP=0 && RUN

# arithmetic underflow
SRC=316 && EXP=255 && RUN_ERR

# arithmetic overflow
SRC=317 && EXP=255 && RUN_ERR

# extreme index exposes NASM overflow
if [[ $MACHINE == x86_64 ]]; then SRC=318 && EXP=255 && RUN_ERR; fi

# out-of-range index returned by a function
SRC=319 && EXP=255 && RUN_ERR

# valid boundary of a one-element array
SRC=320 && EXP=0 && RUN

# one-element lower failure without line output
SRC=321 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower"

# one-element upper failure without line output
SRC=322 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=upper"

# signed eight-bit index remains negative
SRC=323 && EXP=255 && RUN_ERR

# signed sixteen-bit index remains negative
SRC=324 && EXP=255 && RUN_ERR

# signed thirty-two-bit index remains negative
SRC=325 && EXP=255 && RUN_ERR

# signed default-type index remains negative
SRC=326 && EXP=255 && RUN_ERR

# constant index past the end of an array argument is rejected at compile
SRC=327 && COMPERR

# element access checks a computed index
SRC=328 && EXP=255 && RUN_ERR

# negative constant index into an array argument is rejected at compile
SRC=329 && COMPERR

# valid upper boundary through an array parameter
SRC=330 && EXP=0 && RUN

# combined bounds checks without line output
SRC=331 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=upper,lower"

# unary negative constant index is rejected at compile
SRC=332 && COMPERR

# computed inner index in a nested array
SRC=333 && EXP=255 && RUN_ERR

# valid upper boundary through a computed index
SRC=334 && EXP=0 && RUN

# valid default-type element scaling
SRC=335 && EXP=0 && RUN

# valid index returned by a function
SRC=336 && EXP=0 && RUN

# zero-length array must reject index zero
SRC=337 && COMPERR

# valid two-byte element scaling
SRC=338 && EXP=0 && RUN

# valid four-byte element scaling
SRC=339 && EXP=0 && RUN

# minimum signed narrow index must remain negative
SRC=340 && EXP=255 && RUN_ERR

# bounds checking must resolve an index from a field
SRC=341 && EXP=255 && RUN_ERR

# computed index must respect array-parameter bounds
SRC=342 && EXP=255 && RUN_ERR

# an empty array cannot be declared, so its element cannot be accessed
SRC=343 && COMPERR

# explicit zero-sized variable arrays are invalid
SRC=344 && COMPERR

# explicit zero-sized data arrays are invalid
SRC=345 && COMPERR

# empty string fills a sized dat array with zeros
SRC=346 && EXP=0 && RUN

# assign an empty string to a sized array variable
SRC=347 && EXP=0 && RUN

# empty string is invalid without an explicit array size
SRC=348 && COMPERR

# boolean result cannot be assigned to an integer variable
SRC=349 && COMPERR

# boolean not applied to arrays_equal
SRC=350 && EXP=0 && RUN

# boolean values cannot be arithmetic operands
SRC=351 && COMPERR

# boolean arrays_equal result used in non-boolean arithmetic argument
SRC=352 && COMPERR

# missing field initializer delimiter is rejected
SRC=353 && COMPERR

# user types with identical fields are still incompatible
SRC=354 && COMPERR

# array initializer element types must match
SRC=355 && COMPERR

# scalar values cannot initialize array fields
SRC=356 && COMPERR

# dotted function names are rejected
SRC=357 && COMPERR

# array_copy handles a byte remainder
SRC=358 && EXP=0 && RUN

# unknown field after array indexing is rejected
SRC=359 && COMPERR

# empty brace data arrays require a size
SRC=360 && COMPERR

# too many data array elements are rejected before parsing the next element
SRC=361 && COMPERR

# arrays_equal writes its result to a memory field
SRC=362 && EXP=0 && RUN

# mixed width values and indexed addressing
SRC=363 && EXP=0 && RUN

# too many elements in a user type array
SRC=364 && COMPERR

# truncate a qword to byte word and dword
if [[ $MACHINE == x86_64 ]]; then SRC=365 && EXP=0 && RUN; fi

# empty arithmetic parentheses
SRC=366 && COMPERR

# nested empty arithmetic parentheses
SRC=367 && COMPERR

# constants shadow across nested blocks
SRC=368 && EXP=0 && RUN

# duplicate constants in one block are rejected
SRC=369 && COMPERR

# global variable shadowing in a nested block
SRC=370 && EXP=0 && RUN

# global dat must precede global vars
SRC=371 && COMPERR

SRC=372 && EXP=0 && RUN

# foo over an array of user types
SRC=373 && EXP=0 && RUN

# foo over a user type with an array field
SRC=374 && EXP=0 && RUN

# foo over a holder with an array of user types
SRC=375 && EXP=0 && RUN

# foo over an indexed array of user types
SRC=376 && EXP=0 && RUN

# foo over an array field of an indexed holder argument
SRC=377 && EXP=0 && RUN

# foo over an array field of an indexed holder array argument
SRC=378 && EXP=0 && RUN

# foo over a points array in an indexed holder
SRC=379 && EXP=0 && RUN

# foo requires an array identifier
SRC=380 && COMPERR

# foo requires an array identifier
SRC=381 && COMPERR

# foo loop variable passed to a mutating function
SRC=382 && EXP=0 && RUN

# foo loop variable indexes its data field
SRC=383 && EXP=0 && RUN

# foo passes a user type field to a mutating function
SRC=384 && EXP=0 && RUN

# foo indexes an array of user types in a user-type field
SRC=385 && EXP=0 && RUN

# increment indexes an array member of its user-type argument
SRC=386 && EXP=0 && RUN

# foo and increment both index through nested user types
SRC=387 && EXP=0 && RUN

# variable array initialization for built-in and user types
SRC=388 && EXP=0 && RUN

# aggregate assignment with an array field
SRC=389 && EXP=0 && RUN

# array variable assignment
SRC=390 && EXP=0 && RUN

# local array initializes an array of types with array fields
SRC=391 && EXP=0 && RUN

SRC=392 && EXP=0 && RUN

# regression: ident_info field destination and alias lea propagation
SRC=393 && EXP=0 && RUN

# regression: nested destinations plus by-reference call semantics
SRC=394 && EXP=0 && RUN

# regression: shorthand bool checks must compare using scalar-width regs
SRC=395 && EXP=0 && RUN

SRC=396 && EXP=0 && RUN

SRC=397 && EXP=0 && RUN

# == compares indexed default-type values (true and false paths)
SRC=398 && EXP=0 && RUN

# shorthand bool truthiness with unary '~' across scalar widths
SRC=399 && EXP=0 && RUN

# regression: stmt_call alias must pin indexed operand (is_indexed on)
SRC=400 && EXP=0 && RUN

# regression: stmt_call alias plain variables (is_indexed off)
SRC=401 && EXP=0 && RUN

# regression: user-type return and user-type indexed argument aliasing
SRC=402 && EXP=0 && RUN

# lea regression: built-in indexed src/dst aliases and index mutation
SRC=403 && EXP=0 && RUN

# lea regression: user-type indexed args (non-encodable element size path)
SRC=404 && EXP=0 && RUN

# lea regression: nested field writes through user-type alias with lea path
SRC=405 && EXP=0 && RUN

# lea regression: user-type return assigned to indexed dst from indexed src
SRC=406 && EXP=0 && RUN

# branch coverage: stmt_identifier bounds check on array without indexing
# targets compile_effective_address branch: if (is_last and not
# reg_size.empty() and curr_info.is_array)
SRC=407 && EXP=0 && RUN

# branch coverage: decouple_impl recursive assign for non-builtin fields
# targets compile_assign branch: if (not tf.type().is_built_in())
# e.compile_assign(...)
SRC=408 && EXP=0 && RUN

# branch coverage: built-in array field assigned from array identifier targets
# compile_assign branches: if (tf.is_array and src.is_array_identifier())
# validate_array_assignment if (tf.is_array) { validate_array_assignment;
# x.copy(...); }
SRC=409 && EXP=0 && RUN

# resolver path: alias chain with depth growth and no lea
SRC=410 && EXP=0 && RUN

# resolver path: alias chain with lea + indexed user-type argument
SRC=411 && EXP=0 && RUN

# register names are ordinary identifiers on every target
SRC=412 && EXP=0 && RUN

# signed byte multiplication: wide results expose missing sign extension
SRC=413 && EXP=0 && RUN

# non-inline reference arguments: narrow writes, globals and inline forwarding
SRC=414 && EXP=0 && RUN

# non-inline return destinations: nested calls, pointer forwarding and early
# returns
SRC=415 && EXP=0 && RUN

# non-inline print_num: repeated calls, local arrays and inline syscall
# helpers
SRC=416 && DIFF

# non-inline recursion: factorial results and caller-local preservation
SRC=417 && EXP=0 && RUN

# non-inline recursion: factorial results and caller-local preservation
SRC=417 && EXP=0 && OPTS="--vars=0x40000 --checks=frame --reproduce-source" RUN

# non-inline recursion: factorial results and caller-local preservation
SRC=417 && EXP=255 && RUN_ERR_OPTS "--vars=0x40 --checks=frame"

# non-inline user types: nested fields, reference mutation, copies and returns
SRC=418 && EXP=0 && RUN

# non-inline user types: nested fields, reference mutation, copies and returns
SRC=418 && EXP=0 && OPTS="--vars=0x40000 --checks=frame --reproduce-source" RUN

# non-inline return value cannot be discarded
SRC=419 && COMPERR

# non-inline void call cannot supply a value
SRC=420 && COMPERR

# unary operator on a non-inline call is applied to its result
SRC=421 && EXP=2 && RUN

# a non-inline result is written to an element of an array literal
SRC=422 && EXP=0 && RUN

# non-inline result destination must have the declared type
SRC=423 && COMPERR

# non-inline argument can be a computed expression
SRC=424 && EXP=2 && RUN

# non-inline argument can have unary operators
SRC=425 && EXP=4 && RUN

# non-inline argument can be a constant
SRC=426 && EXP=7 && RUN

# a whole array is passed to a non-inline array parameter
SRC=427 && EXP=0 && RUN

# non-inline argument must have the declared parameter type
SRC=428 && COMPERR

# an element cannot be passed to a non-inline array parameter
SRC=429 && COMPERR

# builtin exit with a constant status
SRC=430 && EXP=42 && RUN

# copy an i32 scalar and exit with the copied value
SRC=431 && EXP=42 && RUN

# write and read an indexed field in a 12-byte instance
SRC=432 && EXP=42 && RUN

# reject redefinition of the reserved exit builtin
SRC=433 && COMPERR

# portable read/write with byte counts, errors, and discarded results
# the uart ignores descriptors, so there are no descriptor errors
if [[ $MACHINE != rv32i-qemu && $MACHINE != rv32i-fpga ]]; then SRC=434 && DIFFINP; fi

# read requires a descriptor and an array
SRC=435 && COMPERR

# write requires an array, not an address
SRC=436 && COMPERR

# foo over 8196-byte instances, beyond the RV32I addi immediate range
SRC=437 && EXP=0 && RUN

# non-inline recursion, references, aggregate returns and large local frames
SRC=438 && EXP=0 && RUN

# non-inline recursion, references, aggregate returns and large local frames
SRC=438 && EXP=0 && OPTS="--vars=0x40000 --checks=upper,lower,line,frame --reproduce-source" RUN

# non-inline recursion, references, aggregate returns and large local frames
SRC=438 && EXP=255 && RUN_ERR_OPTS "--vars=0x1020 --checks=frame"

# arrays with omitted element type use the target's default integer type
SRC=439 && EXP=0 && RUN

# name-first return declarations with default, explicit and aggregate types
SRC=440 && EXP=0 && RUN

# canonical boolean results passed directly to a register argument inspect
# from the project root: ./baz qa/coverage/tests/442.baz --target=x86_64 >
# gen.s use --target=rv32i for the other backend; omit checks to isolate
# result emission x86: first arrays_equal ends with sete, second with setne in
# the argument register no extra result register, normalization, or xor should
# appear between it and assert rv32i: equality writes 1/0 directly; negation
# swaps these values without xori assert's own truth test is separate and
# still needed
SRC=442 && EXP=0 && RUN

# canonical boolean results assigned to a memory variable inspect from the
# project root: ./baz qa/coverage/tests/443.baz --target=x86_64 > gen.s use
# --target=rv32i for the other backend; omit checks to isolate result emission
# inspect the initialization and reassignment of b, separately from assert(b)
# x86: sete byte [b] for equality, setne byte [b] for negated equality rv32i:
# produce the final 0/1 value and sb it directly neither assignment needs a
# second normalization or an xor for negation
SRC=443 && EXP=0 && RUN

# inline function bodies require braces even for one statement
SRC=444 && COMPERR

# non-inline function bodies require braces across newlines too
SRC=445 && COMPERR

# a function cannot omit its body at end of input
SRC=446 && COMPERR

# braced functions retain single-statement if and loop bodies
SRC=447 && EXP=0 && RUN

# colonless declarations retain default types, arrays and adjacent statements
SRC=448 && EXP=0 && RUN

# colon syntax is no longer accepted for parameters
SRC=449 && COMPERR

# colon syntax is no longer accepted for variables
SRC=450 && COMPERR

# colon syntax is no longer accepted for fields
SRC=451 && COMPERR

# colon syntax is no longer accepted for function results
SRC=452 && COMPERR

# colon syntax is no longer accepted for data
SRC=453 && COMPERR

# arrays follow names, with optional element types
SRC=454 && EXP=0 && RUN

# the old type-after-brackets order is rejected, the initializer gives the
# type
SRC=455 && COMPERR

# a type after the variable name is rejected, the initializer gives the type
SRC=456 && COMPERR

# array brackets follow the field element type
SRC=457 && COMPERR

# array brackets follow the parameter element type
SRC=458 && COMPERR

# omitting identical memory copies preserves subsequent operations
SRC=459 && EXP=0 && RUN

# structured address scales include non-powers of two and values above 255
SRC=460 && EXP=0 && RUN

# constant last elements after non-constant elements decide the list only when
# they short-circuit it
SRC=461 && EXP=0 && RUN

# constant nested lists as last elements branch to the enclosing true target
SRC=462 && EXP=0 && RUN

# negated parenthesized constants are expressions
SRC=463 && EXP=0 && RUN

# an 'else' assignment does not cover an 'if' branch that skips the result
SRC=464 && COMPERR

# a conditional return skips the result assignment
SRC=465 && COMPERR

# a conditional break skips the result assignment
SRC=466 && COMPERR

# the result is read inside an 'if' branch before it is set
SRC=467 && COMPERR

# the result is read inside a loop before it is set
SRC=468 && COMPERR

# results set on every path stay accepted, including early returns and loops
SRC=469 && EXP=0 && RUN

# 64-bit constants outside the sign-extended 32-bit immediate range
if [[ $MACHINE == x86_64 ]]; then SRC=470 && EXP=0 && RUN; fi

# a field of the variable is read in its own initializer
SRC=471 && COMPERR

# the variable is copied from itself in its own initializer
SRC=472 && COMPERR

# the variable is passed to the function that initializes it
SRC=473 && COMPERR

# the variable is read in an index of its own initializer
SRC=474 && COMPERR

# a field of the result is read before the result is set
SRC=475 && COMPERR

# assigning one field does not set the whole result
SRC=476 && COMPERR

# nested result fields set one at a time and read after they are set
SRC=477 && EXP=0 && RUN

# a field set in only one branch is not set after the 'if'
SRC=478 && COMPERR

# fields set on every path through 'if', 'else' and early returns
SRC=479 && EXP=0 && RUN

# constant indexes set every element of an array field
SRC=480 && EXP=0 && RUN

# a constant index sets only its element
SRC=481 && COMPERR

# a runtime index does not prove which element is set
SRC=482 && COMPERR

# a field of the result is read before it is set
SRC=483 && COMPERR

# a return inside 'foo' skips the result assignment
SRC=484 && COMPERR

# a result that aliases an argument is rejected when it is checked for
SRC=485 && OPTS="--checks=noub" COMPERR

# the destination is read by a later element of its own expression
SRC=486 && EXP=0 && RUN

# an instance value reads the destination it assigns
SRC=487 && COMPERR

# a boolean list reads the destination it assigns
SRC=488 && EXP=0 && RUN

# a runtime index may already have written the element an instance value reads
SRC=489 && COMPERR

# a narrower operand widens to the destination size in arithmetic
SRC=490 && EXP=0 && RUN

# a comparison may not truncate a wider right-hand side
SRC=491 && COMPERR

# a comparison may not truncate a constant
SRC=492 && COMPERR

# an inline argument must have the declared parameter type
SRC=493 && COMPERR

# 'i8(...)' 'i16(...)' 'i32(...)' narrow on purpose and the store truncates
SRC=494 && EXP=0 && RUN

# narrowing a variable must be stated with the builtin
SRC=495 && COMPERR

# a constant outside the signed range must be stated with the builtin
SRC=496 && COMPERR

# an inline result names the destination so the types must match
SRC=497 && COMPERR

# an instance literal assigns every element of an array-of-instances field
SRC=498 && EXP=0 && RUN

# an array literal may not have more elements than the array
SRC=499 && COMPERR

# an array field literal may not have more elements than the field
SRC=500 && COMPERR

# an instance argument can be a temporary such as a literal
SRC=501 && EXP=3 && RUN

# an instance argument can be a temporary such as a call result
SRC=502 && EXP=3 && RUN

# a named constant argument is the caller's constant even when the callee
# declares a constant with the same name
SRC=503 && EXP=0 && RUN

# constant arguments with unary operators are folded before aliasing
SRC=504 && EXP=0 && RUN

# function names do not clash with registers, assembler keywords or internal
# labels
SRC=505 && EXP=0 && RUN

# inline call labels are unique for any function name
SRC=506 && EXP=0 && RUN

# constant argument must fit the parameter type
SRC=507 && COMPERR

# constant argument with unary operators must fit the parameter type
SRC=508 && COMPERR

# folding negation of the minimum constant wraps like the run-time negation
if [[ $MACHINE == x86_64 ]]; then SRC=509 && EXP=0 && RUN; fi

# a negated variable argument must fit the parameter type
SRC=510 && COMPERR

# a loop without 'break' is left only by 'return', so the result is set
SRC=511 && EXP=0 && RUN

# a non-inline result can be an operand in an expression
SRC=512 && EXP=2 && RUN

# 'read' and 'write' take an array, an optional element count and an optional
# start element, like 'pread'/'pwrite'
SRC=513 && DIFFINP

# a run-time element count beyond the array panics
SRC=514 && EXP=255 && RUN_ERR

# the start element plus the count must stay within the array
SRC=515 && EXP=255 && RUN_ERR

# an element is not an array, the parameter would see the whole array's length
SRC=516 && COMPERR

# the start is the 4th argument, not an element
SRC=517 && COMPERR

# without a count the whole array is transferred, an instance array field too
SRC=518 && DIFFINP

# a range may end exactly at the array end, an empty range may start there
SRC=519 && DIFFINP

# a start past the array end panics
SRC=520 && EXP=255 && RUN_ERR

# a negative start panics
SRC=521 && EXP=255 && RUN_ERR

# a negative count with a start panics
SRC=522 && EXP=255 && RUN_ERR

# a negative 'array_copy' count with start elements panics
SRC=523 && EXP=255 && RUN_ERR

# a negative 'arrays_equal' count with start elements panics
SRC=524 && EXP=255 && RUN_ERR

# constant multiplication with shifts and adds or subtracts matches the
# runtime multiply for many constants, operands and widths
SRC=525 && EXP=0 && RUN
UB_ALIAS="--vars=0x40000 --checks=upper,lower,line,alias --reproduce-source"

# --checks=alias: an inline result may share storage with an argument
SRC=526 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: an instance result may share storage with another element
SRC=527 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a non-inline result may share storage with an argument
SRC=528 && OPTS="$UB_ALIAS" COMPERR

# two by-reference arguments may share storage when one is 'mut'
SRC=529 && COMPERR

# --checks=alias: an inline parameter resolves to the global it aliases
SRC=530 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a global passed to a non-inline function that also names it
SRC=531 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: calls whose references cannot share storage are accepted
SRC=532 && EXP=0 && OPTS="$UB_ALIAS" RUN

# comments inside arithmetic expressions and call arguments
SRC=533 && EXP=0 && RUN

# comments inside initializers, type definitions and parameter lists
SRC=534 && EXP=0 && RUN

# comments in single statement blocks and boolean expressions
SRC=535 && EXP=0 && RUN

# a '#' inside a string is not a comment and a comment needs no space
SRC=536 && DIFF

# a comment at the end of the file without a final newline
SRC=537 && EXP=0 && RUN

# nested expressions deeper than the nine x86 scratch registers that no instruction needs
SRC=538 && EXP=0 && RUN

# x86: division needs 'rdx' while it holds a last resort scratch value
if [[ $MACHINE == x86_64 ]]; then SRC=539 && COMPERR; fi

# x86: a variable shift needs 'rcx' while it holds a last resort scratch value
if [[ $MACHINE == x86_64 ]]; then SRC=540 && COMPERR; fi

# x86: a nested expression deeper than all fourteen scratch registers
if [[ $MACHINE == x86_64 ]]; then SRC=541 && COMPERR; fi
# SRC=542 && EXP=0 && RUN # note: generates a huge file and is very slow

# natural alignment: words at multiples of 4 and half words at even offsets,
# padding between and after fields is zero so instances compare byte by byte
SRC=543 && EXP=0 && RUN

# character literals are constants with the value of their byte
SRC=544 && DIFF

# character literal without a character
SRC=545 && COMPERR

# character literal with more than one character
SRC=546 && COMPERR

# character literal with an unsupported escape
SRC=547 && COMPERR

# character literal without the closing quote
SRC=548 && COMPERR

# strings initialize and assign 'i8' arrays: the bytes are copied from
# read-only data and the rest of the array is zeroed
SRC=549 && DIFF

# a string longer than the destination array
SRC=550 && COMPERR

# only 'i8' arrays take strings
SRC=551 && COMPERR

# a string is not an argument, it would refer to read-only data
SRC=552 && COMPERR

# an unsized parameter has the size of the argument, which is too small
SRC=553 && COMPERR

# an empty string cannot give a 'var' array its size
SRC=554 && COMPERR

# a '{...}' initializer of an unsized parameter uses the argument size: the
# unlisted elements of the argument are zeroed
SRC=555 && EXP=0 && RUN

# a '{...}' initializer of an unsized parameter cannot have more elements than
# the argument
SRC=556 && COMPERR

# short strings are stored with immediates when that takes no more code than
# copying from read-only data, the constants are word aligned
SRC=557 && DIFF

# string with an unsupported escape
SRC=558 && COMPERR

# constant '{...}' elements are stored as packed bytes and 'array_copy' with a
# constant count copies like an assignment
SRC=559 && EXP=0 && RUN

# a constant 'array_copy' count past the end of the array panics
SRC=560 && EXP=255 && RUN_ERR

# methods: 'func list.add(x)' is called as 'lst.add(x)' with 'lst' as the
# implicit first parameter 'self'
SRC=561 && EXP=0 && RUN

# method on a type that does not exist
SRC=562 && COMPERR

# methods on built-in types are not supported
SRC=563 && COMPERR

# a method may share its name with a field: 'p.x(3)' calls the method and
# 'p.x' is the field, also when a variable is named like its type
SRC=564 && EXP=0 && RUN

# the receiver of a method cannot be a whole array
SRC=565 && COMPERR

# a method name must be followed by the arguments
SRC=566 && COMPERR

# arguments of a method are numbered without the receiver
SRC=567 && COMPERR

# the receiver is a reference like the other arguments
SRC=568 && COMPERR

# 'self' is the implicit first parameter of a method
SRC=569 && COMPERR

# trailing whitespace ends at a newline: operators, brackets and parentheses
# may still start the next line
SRC=570 && EXP=0 && RUN

# an instance literal field narrows only with 'i8(...)' etc, also for a plain
# identifier that is copied directly
SRC=571 && COMPERR

# data initializers at the limits of their signed types are stored as written
SRC=572 && EXP=0 && RUN

# an array data element outside the signed range of its type is rejected
SRC=573 && COMPERR

# an instance data field is checked after its unary operations
SRC=574 && COMPERR

# a scalar data value outside the signed range of its type is rejected
SRC=575 && COMPERR

# constant multiplication by shifts and adds, e.g. the element size of an
# array index, keeps the original value until the last add or sub
SRC=576 && EXP=0 && RUN

# an instance copied from an indexed array element copies one element and leaves
# the bytes after the destination untouched
SRC=577 && EXP=0 && RUN

# an instance array field copied from an array of another size
SRC=578 && COMPERR

# an instance copied from a whole array
SRC=579 && COMPERR

# an instance array field copied from a single instance
SRC=580 && COMPERR

# an array copied from an indexed array element
SRC=581 && COMPERR

# 'array_length' of an indexed element
SRC=582 && COMPERR

# a whole array read as a scalar is not its first element
SRC=583 && COMPERR

# a field of an array is reached through an indexed element
SRC=584 && COMPERR

# a whole array passed to a scalar parameter
SRC=585 && COMPERR

# '==' of a whole array and an element
SRC=586 && COMPERR

# aliases of array elements are read as the element
SRC=587 && EXP=0 && RUN

# a trailing comma after the last field of a type
SRC=588 && EXP=0 && RUN

# a trailing comma is followed by '}', not another comma
SRC=589 && COMPERR

# rv32i bulk loops use words when the addresses prove more alignment than the
# type; tails and unaligned addresses keep smaller accesses
SRC=590 && EXP=0 && RUN

# rv32i unrolled zero, copy and constant stores at an address inside a word
# take a byte and a halfword up to the word boundary, then words and a tail; a
# copy between different positions within a word keeps smaller accesses
SRC=591 && EXP=0 && RUN

# rv32i zero, copy and compare loops at an address inside a word take a byte
# and a halfword head up to the loop width when every address is the same
# distance from it; a run-time count may end within the head
SRC=592 && EXP=0 && RUN

# constant elements of an arithmetic list fold at compile at the width of the
# destination and give the same values as the run-time computation
# test-constant-folding.sh checks the instructions of lines marked '# folds'
SRC=593 && EXP=0 && RUN

# a comparison may not truncate a folded constant
SRC=594 && COMPERR

# a constant index in range is an offset known at compile, no index register
# is computed at run time test-constant-folding.sh checks the instructions of
# lines marked '# folds'
SRC=595 && EXP=0 && RUN

# a constant range start past the end is rejected at compile
SRC=596 && COMPERR

# a constant left side of a comparison may not truncate either
SRC=597 && COMPERR

# '==' of a known size compares the qwords, then the remaining dword, word
# and byte; a difference in any byte of any part makes it false
SRC=598 && EXP=0 && RUN

# constructors: 'func point.at(x, y) self' builds a 'point' in 'self' and is
# called as 'point.at(1, 2)'
SRC=599 && EXP=0 && RUN

SRC=600 && COMPERR

SRC=601 && COMPERR

SRC=602 && COMPERR

SRC=603 && COMPERR

SRC=604 && COMPERR

SRC=605 && COMPERR

SRC=606 && COMPERR

SRC=607 && COMPERR

SRC=608 && COMPERR

# an untyped variable has the type of its initializer: a comparison or a
# 'bool' operand is a 'bool', a single operand has its own type, several
# operands have the default type like constants test-deduced-types.sh checks
# the type on lines marked '# is TYPE', 'default' is the default type of the
# target
SRC=609 && EXP=0 && RUN

# a typed instance literal must name the type of the destination
SRC=610 && COMPERR

# an untyped variable cannot take its type from '{...}', e.g. 'point{1, 2}'
# names it
SRC=611 && COMPERR

# an array literal must name the element type of the destination
SRC=612 && COMPERR

# an array size in both the destination and the literal must agree
SRC=613 && COMPERR

# an array literal without elements and without a size has no size
SRC=614 && COMPERR

# an array literal with a size holds at most that many elements
SRC=615 && COMPERR

# a type after the variable name is rejected, the initializer gives the type,
# e.g. 'var x = i32(0)'
SRC=616 && COMPERR

# an array size after the variable name is rejected, an array literal gives
# the element type and size, e.g. 'var a = i8[4]{}'
SRC=617 && COMPERR

# an unsized array parameter has no size for a copy in a new variable
SRC=618 && COMPERR

# calling a method that does not exist reports the method, not a missing field
SRC=619 && COMPERR

# data declarations take the type and size from the initializer like 'var'
SRC=620 && EXP=0 && RUN

# a type after the data name is rejected, the initializer gives the type, e.g.
# 'dat x = i32(0)'
SRC=621 && COMPERR

# an array size after the data name is rejected, an array literal gives the
# element type and size, e.g. 'dat a = i8[4]{}'
SRC=622 && COMPERR

# a data conversion needs its closing parenthesis
SRC=623 && COMPERR

# 'true' and 'false' are of type 'bool' and cannot be arithmetic operands
SRC=624 && COMPERR

# an array literal without a type name has the default type which must match
# the destination
SRC=627 && COMPERR

# a missing first argument of a user type is reported as missing rather than
# as a malformed instance value
SRC=628 && COMPERR

# a call without a result inside a function with a result is a statement, not
# an assignment to the caller's destination
SRC=629 && EXP=13 && RUN

# a result discarded inside a function with a result is not written to the
# caller's destination
SRC=630 && COMPERR

# foo with a count visits the first 'count' elements
SRC=631 && EXP=0 && RUN

# foo count larger than the array fails the upper bound check
SRC=632 && EXP=255 && RUN_ERR

# negative foo count fails the lower bound check
SRC=633 && EXP=255 && RUN_ERR

# the foo counter 'i' is read-only
SRC=634 && COMPERR

# the foo counter 'i' is read-only also through a parameter
SRC=635 && COMPERR

# data instance initializer requires the type name
SRC=636 && COMPERR

# data built-in field initialized with braces
SRC=637 && COMPERR

# data array initializer with trailing comma
SRC=638 && COMPERR

# a backslash at the end of a line continues the string on the next line,
# leading whitespace of the next line is part of the string
SRC=639 && EXP=0 && RUN

# --checks=alias: arguments sharing storage are allowed when both parameters
# are not 'mut', for inline and non-inline functions
SRC=640 && EXP=0 && OPTS="$UB_ALIAS" RUN

# a parameter without 'mut' cannot be assigned
SRC=641 && COMPERR

# a field of a parameter without 'mut' cannot be assigned
SRC=642 && COMPERR

# 'array_copy' cannot copy into an array parameter without 'mut'
SRC=643 && COMPERR

# 'read' cannot fill an array parameter without 'mut'
SRC=644 && COMPERR

# the element of 'foo' over an array parameter without 'mut' cannot be
# assigned
SRC=645 && COMPERR

# a parameter without 'mut' cannot be written through an inline callee
SRC=646 && COMPERR

# a parameter without 'mut' cannot be passed to a 'mut' non-inline parameter
SRC=647 && COMPERR

# a 'mut' parameter still conflicts with a read-only one
SRC=648 && COMPERR

# --checks=alias: a parameter without 'mut' may not share storage with the
# result
SRC=649 && OPTS="$UB_ALIAS" COMPERR

# 'mut' goes after the parameter name
SRC=650 && COMPERR

# 'let' with a non-constant initializer is a 'var' that cannot be assigned
# after its initializer
SRC=651 && EXP=0 && RUN

# a 'let' variable cannot be assigned
SRC=652 && COMPERR

# a field of a 'let' variable cannot be assigned
SRC=653 && COMPERR

# an element of a 'let' array cannot be assigned
SRC=654 && COMPERR

# 'array_copy' cannot copy into a 'let' array
SRC=655 && COMPERR

# 'read' cannot fill a 'let' array
SRC=656 && COMPERR

# a 'let' variable cannot be passed to a 'mut' non-inline parameter
SRC=657 && COMPERR

# a 'let' variable cannot be written through an inline callee
SRC=658 && COMPERR

# a 'let' needs an initializer
SRC=659 && COMPERR

# a file level 'let' variable cannot be assigned in a function
SRC=660 && COMPERR

# a 'let' variable cannot be assigned in a function that is never called
SRC=661 && COMPERR

# a parameter without 'mut' cannot be assigned in a function that is never
# called
SRC=662 && COMPERR

# a method that writes its receiver cannot be called on a read-only argument,
# also in a function inlined into another
SRC=663 && COMPERR

# a read-only argument passed to a function that writes it is rejected at the
# call, also inside a function that is never called
SRC=664 && COMPERR

# the element 'e' of 'foo' writes the array, so passing a read-only array on
# to the function that walks it is rejected at the call
SRC=665 && COMPERR

# 'read' writes its buffer, so passing a read-only array on to the function
# that reads into it is rejected at the call
SRC=666 && COMPERR

# 'array_copy' writes its destination, so passing a read-only array on to the
# function that copies into it is rejected at the call
SRC=667 && COMPERR

# 'array_copy' cannot copy into a read-only array parameter, also in a
# function that is never called
SRC=668 && COMPERR

# 'read' cannot fill a read-only array parameter, also in a function that is
# never called
SRC=669 && COMPERR

# a method without 'mut' cannot write 'self'
SRC=670 && COMPERR

# a method without 'mut' cannot call a 'mut' method on 'self'
SRC=671 && COMPERR

# a constructor builds 'self' so it needs no 'mut'
SRC=672 && COMPERR

# 'mut' marks the receiver of a method, a function has none
SRC=673 && COMPERR

# 'mut' before the receiver type lets a method write 'self', other methods
# work on read-only receivers, a function can still be named 'mut'
SRC=674 && EXP=0 && RUN

# '==' compares the bytes of the fields, the padding of an instance is not
# compared: a constructor leaves the padding as the storage held it
SRC=675 && EXP=0 && RUN

# an array literal with call elements writes each result at its element
SRC=676 && EXP=0 && RUN

# 'array_length' is a constant at compile: comparisons, counts, indexes and
# arguments use its value test-constant-folding.sh checks the instructions of
# lines marked '# folds'
SRC=677 && EXP=0 && RUN

# comparing two variables far from the start of the frame: on rv32i both
# addresses share one 'lui' and 'add', for 8, 16 and 32-bit variables
SRC=678 && EXP=0 && RUN

# a constant added to or subtracted from a run-time index goes into the
# displacement when bounds checks are off, with checks on the sum is computed
# in the index register
SRC=679 && EXP=0 && RUN

# a constant added to or subtracted from a run-time index goes into the
# displacement when bounds checks are off, with checks on the sum is computed
# in the index register
SRC=679 && EXP=0 && RUN_NO_CHECKS

# dat items and variables are addressed from one base: on rv32i the base is
# the start of the variables so dat items have negative offsets
SRC=680 && EXP=0 && RUN

# arguments that reach different bytes of one variable can be passed together
SRC=681 && EXP=0 && RUN

# arguments with run-time indexes into one array may share storage
SRC=682 && COMPERR

# a read-only variable can be initialized from an array of the default type
SRC=683 && EXP=0 && RUN

# exit takes one argument
SRC=684 && COMPERR

# a constant must be a parsable number
SRC=685 && COMPERR

# convert requires a closing parenthesis
SRC=686 && COMPERR

# a narrow destination computes a converted operand at the wide width
SRC=687 && EXP=45 && RUN

# == requires arrays of the same element type
SRC=688 && COMPERR

# a non-inline function reads an array parameter
SRC=689 && EXP=0 && RUN

# a method name follows the dot
SRC=690 && COMPERR

# a block requires its closing brace
SRC=691 && COMPERR

# a built-in element of a data definition must be a constant
SRC=692 && COMPERR

# an array field gets its initializer after a delimiter
SRC=693 && COMPERR

# a call requires parentheses
SRC=694 && COMPERR

# a constructor name follows the dot
SRC=695 && COMPERR

# a constructor call requires parentheses
SRC=696 && COMPERR

# constant operands that trap or wrap at run time are not folded
SRC=697 && EXP=7 && RUN

# a function name has no fields
SRC=698 && COMPERR

# read and write take 2 to 4 arguments
SRC=699 && COMPERR

# read requires an array to fill
SRC=700 && COMPERR

# a negative constant count is rejected at run time by the lower check
SRC=701 && EXP=255 && RUN_ERR

# 'noinline' is a function name when parentheses follow
SRC=702 && EXP=3 && RUN

# a field array size is required
SRC=703 && COMPERR

# a string is not a constant operand of let
SRC=704 && EXP=3 && RUN

# array_copy of an indexed struct element
SRC=105 && EXP=0 && RUN_NO_CHECKS

# a let initializer starting with a symbol is not a constant
SRC=705 && COMPERR

# read cannot fill a constant
SRC=706 && COMPERR

# a negative count is rejected by the lower check alone
SRC=707 && EXP=255 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower"

# a constant count is range checked by the lower check alone
SRC=708 && EXP=0 && RUN_ERR_OPTS "--vars=0x10000 --checks=lower"

# a global passed to a non-inline function that also names it shares storage
SRC=709 && COMPERR

# a constant passed to a mut parameter cannot be written
SRC=710 && COMPERR

# an error in a method body names the method call
SRC=711 && COMPERR

# a constant element decides a boolean list passed to an inline function
SRC=712 && EXP=2 && RUN

# a type name followed by an operator is not a data value
SRC=713 && COMPERR

# a read-only argument cannot be passed to a mut parameter of a method
SRC=714 && COMPERR

# nested indexed addresses are folded into the array copy address registers
SRC=715 && EXP=0 && RUN

# an index ending in a multiplied or divided constant gets no displacement
SRC=716 && EXP=5 && RUN_NO_CHECKS

# adding the most negative value of the width stays an addition
SRC=717 && EXP=0 && RUN

# an instance value reads the whole destination after writing a field
SRC=718 && COMPERR

# a constant shift by a negative count is not folded, rv32i rejects it
if [[ $MACHINE == x86_64 ]]; then SRC=719 && EXP=0 && RUN; else SRC=719 && COMPERR; fi

# dat is only allowed in the global scope
SRC=720 && COMPERR

# foo is a builtin iterator and cannot name a function
SRC=721 && COMPERR

# a variable must fit in the vars section
SRC=722 && OPTS="--vars=0x40" COMPERR

# a statement is not allowed at the top level
SRC=723 && COMPERR

# main is compiled where the program starts
SRC=724 && COMPERR

# a backslash before a crlf line end continues the string
SRC=725 && EXP=0 && RUN

# bell and backspace escapes in strings and character literals
SRC=726 && EXP=0 && RUN

# a character literal escape has one character
SRC=727 && COMPERR

# a hex escape in a character literal has two digits
SRC=728 && COMPERR

# a hex escape in a string has two hex digits
SRC=729 && COMPERR

# an instance value starts with '{'
SRC=730 && COMPERR

# a call result wider than the destination is narrowed explicitly
SRC=731 && COMPERR

# x86: a division needs 'rdx' while the iterator of a loop holds it
if [[ $MACHINE == x86_64 ]]; then SRC=733 && COMPERR; fi

# x86: the count of an 'array_copy' is computed while 'rcx' is reserved
if [[ $MACHINE == x86_64 ]]; then SRC=734 && COMPERR; fi

# a system call inside nested loops keeps the loop register 'r11' of x86
SRC=735 && DIFF

# rv32i: 'write' needs 'a1', which is held by a loop register
if [[ $MACHINE != x86_64 ]]; then SRC=736 && COMPERR; fi

# rv32i: nested loops hold every scratch register
if [[ $MACHINE != x86_64 ]]; then SRC=737 && COMPERR; fi

# rv32i: a constant copy size beyond the 32-bit address range
if [[ $MACHINE != x86_64 ]]; then SRC=738 && OPTS="--vars=0x10000" COMPERR; fi

# rv32i: data elements must be 1, 2 or 4 bytes
if [[ $MACHINE != x86_64 ]]; then SRC=739 && COMPERR; fi

# an unknown escape in a string constant
SRC=740 && COMPERR

# rv32i: data beyond the 32-bit address range
if [[ $MACHINE == rv32i ]]; then SRC=741 && COMPERR; fi

# copy between variables far from the variables base
SRC=742 && EXP=7 && RUN

# and with zero and or with all bits yield constants
SRC=743 && EXP=0 && RUN

# an array whose size exceeds the signed 64-bit range
SRC=744 && COMPERR

# an instance field whose size exceeds the signed 64-bit range
SRC=745 && COMPERR

# instance fields whose sizes together exceed the signed 64-bit range
SRC=746 && COMPERR

# data arrays whose sizes together exceed the signed 64-bit range, rv32i
# rejects the first one as beyond its address range
if [[ $MACHINE == x86_64 ]]; then SRC=747 && COMPERR; fi

# x86: a copy whose byte count exceeds the signed 64-bit range
if [[ $MACHINE == x86_64 ]]; then SRC=748 && COMPERR; fi

# an unsupported escape after a line continuation is located on its own line
SRC=749 && COMPERR

# rv32i: a 64-bit variable is wider than a register
if [[ $MACHINE != x86_64 ]]; then SRC=750 && COMPERR; fi

# the largest constant shift counts of narrow values
SRC=751 && EXP=1 && RUN

# a sum with a constant that does not fit one immediate
SRC=752 && EXP=3 && RUN

# rv32i: loops nested so deeply that no scratch register is left for a far jump
if [[ $MACHINE != x86_64 ]]; then SRC=753 && EXP=101 && RUN; fi

# rv32i: a remainder keeps its result while loop registers are saved
if [[ $MACHINE != x86_64 ]]; then SRC=754 && EXP=2 && RUN; fi

# a range with a runtime start and count when only the upper limit is checked
SRC=755 && OPTS="--vars=0x40000 --checks=upper --reproduce-source" DIFF

# rv32i: a constant shift count of a 32-bit value must be below 32
if [[ $MACHINE != x86_64 ]]; then SRC=756 && COMPERR; fi

# x86: a constant shift count beyond the width is not folded
if [[ $MACHINE == x86_64 ]]; then SRC=757 && EXP=7 && RUN; fi

# rv32i: a constant shift count of an 8-bit value must be below 8
if [[ $MACHINE != x86_64 ]]; then SRC=758 && COMPERR; fi

# rv32i: a constant right shift count of a 16-bit value must be below 16
if [[ $MACHINE != x86_64 ]]; then SRC=759 && COMPERR; fi

# a constant shift by zero changes nothing
SRC=760 && EXP=5 && RUN

# a non-inline parameter shares storage with a global only if a caller passes it
SRC=761 && EXP=21 && RUN

# a global reaches a global named by a callee through two non-inline calls
SRC=762 && COMPERR

# a recursive non-inline call is checked once
SRC=763 && EXP=0 && RUN

# an indexed argument may be the array element a non-inline body names
SRC=764 && COMPERR

# a type or array initializer without '{}' is the same as with an empty '{}'
SRC=765 && EXP=0 && RUN

# a built-in type initializer without '(0)' is the same as with '(0)'
SRC=766 && EXP=0 && RUN

# a type name followed by an operator is not an initializer
SRC=767 && COMPERR

# a non-inline function takes arrays of different lengths, one body each
SRC=768 && EXP=0 && RUN

# a non-inline body uses its array parameter like an inline one
SRC=769 && EXP=0 && RUN

# two array parameters, recursion, methods, a dat array and an uncalled
# function
SRC=770 && EXP=0 && RUN
SRC=770 && EXP=0 && OPTS="--vars=0x40000 --checks=frame --reproduce-source" RUN

# a constant index out of bounds is found for the lengths of one call
SRC=771 && COMPERR

# a run-time index is checked against the length of each body
SRC=772 && EXP=255 && RUN_ERR

# an array argument must have the element type of the parameter
SRC=773 && COMPERR

# generic functions: type parameters replaced per list of type arguments
SRC=774 && EXP=0 && RUN

# a generic function is called with type arguments when its arguments do not
# tell them
SRC=775 && COMPERR

# a generic function takes as many type arguments as type parameters
SRC=776 && COMPERR

# a type argument names a type
SRC=777 && COMPERR

# the body of a generic function is checked per instance
SRC=778 && COMPERR

# a type parameter has the kind 'type'
SRC=779 && COMPERR

# generic types: constants and types as parameters, one method for all the
# instances of a type
SRC=780 && EXP=0 && RUN

# an alias names a generic type
SRC=781 && COMPERR

# a generic type takes as many arguments as parameters
SRC=782 && COMPERR

# the argument of a constant parameter is a number or a constant
SRC=783 && COMPERR

# a generic type is used with arguments
SRC=784 && COMPERR

# the argument of a type parameter names a type
SRC=785 && COMPERR

# a method of a generic type with a type parameter of its own
SRC=786 && EXP=0 && RUN

# the type argument of a method is deduced from a variable, not from an
# expression
SRC=787 && COMPERR

# the type arguments of a call end with '>'
SRC=788 && COMPERR

# the type arguments of a call are not empty
SRC=789 && COMPERR

# the arguments of an alias end with '>'
SRC=790 && COMPERR

# the arguments of an alias are not empty
SRC=791 && COMPERR

# an error in a method of a generic type names the alias that needs it
SRC=792 && COMPERR

# also for a method defined after the alias
SRC=793 && COMPERR

# an error in the fields of a generic type names the alias that needs it
SRC=794 && COMPERR

# a generic parameter is declared once
SRC=795 && COMPERR

# a generic parameter does not hide a type
SRC=796 && COMPERR

# a generic type is not a type until an alias names an instance
SRC=797 && COMPERR

# a generic type is not a value
SRC=798 && COMPERR

# a generic function is not a value
SRC=799 && COMPERR

# an alias does not take the name of a generic type
SRC=800 && COMPERR

# the argument of a type parameter is not a constant
SRC=801 && COMPERR

# the argument of a constant parameter is not a type
SRC=802 && COMPERR

# the type argument of a call is not a constant
SRC=803 && COMPERR

# a generic definition with a type literal, nested generics and recursion
SRC=804 && EXP=0 && RUN

# a generic definition without its closing brace
SRC=805 && COMPERR

# the label of a noinline generic instance does not clash with a noinline method
SRC=806 && EXP=0 && RUN

# a generic parameter list has at least one parameter
SRC=807 && COMPERR

# a generic parameter is a constant or has the kind 'type'
SRC=808 && COMPERR

# generic parameters are separated by a comma
SRC=809 && COMPERR

# a comma in a generic parameter list is followed by a parameter
SRC=810 && COMPERR

# a parameter of a generic function does not hide a type
SRC=811 && COMPERR

# a generic function cannot have the name of a function
SRC=812 && COMPERR

# a function cannot have the name of a generic function
SRC=813 && COMPERR

# a generic type cannot have the name of another generic type
SRC=814 && COMPERR

# the constant argument of an alias must make a valid type
SRC=815 && COMPERR

# a generic type is known where the alias is, not before
SRC=816 && COMPERR

# a type parameter is not known outside its generic
SRC=817 && COMPERR

# the arguments of an instance are checked like those of any function
SRC=818 && COMPERR

# an alias is defined once
SRC=819 && COMPERR

# a method of a generic type is defined once, the instance reports it
SRC=820 && COMPERR

# a function that is not generic takes no type arguments
SRC=821 && COMPERR

# also in an expression
SRC=822 && COMPERR

# an alias names a generic type, not a type
SRC=823 && COMPERR

# the type arguments of a call are deduced from variables, fields and elements
SRC=824 && EXP=0 && RUN

# a literal does not tell the type
SRC=825 && COMPERR

# the arguments of a deduced instance are checked like any other
SRC=826 && COMPERR

# an array parameter does not tell the type
SRC=827 && COMPERR

# a type name assigned is the zero value of that type
SRC=828 && EXP=0 && RUN

# a type name alone is its zero value in an instance literal and in an index
SRC=829 && EXP=0 && RUN

# a type name alone as an argument is a temporary, a zeroed instance
SRC=830 && EXP=1 && RUN

# an array assigned without braces needs a size
SRC=831 && COMPERR

# an array declared without braces needs a size
SRC=832 && COMPERR

# a parameter has a name
SRC=833 && COMPERR

# the parameters of a generic function are read where it is defined
SRC=834 && COMPERR

# the type arguments of a call are deduced from the type it is assigned to
SRC=835 && EXP=0 && RUN

# data larger than the address range of rv32i is rejected at its declaration
if [[ $MACHINE != x86_64 ]]; then SRC=836 && COMPERR; fi

# user type instances and arrays are compared with == and !=
SRC=837 && EXP=0 && RUN

# an instance of a user type is not a condition
SRC=838 && COMPERR

# a user type instance is compared only with an instance of the same type
SRC=839 && COMPERR

# a user type instance is not compared with a number by an operator
SRC=840 && COMPERR

# 'int' names the default type, also as a type argument and a field type
SRC=841 && EXP=0 && RUN

# 'int' is a type name, a type cannot be defined with it
SRC=842 && COMPERR

# nested foo loops deep enough for x86_64 to count in memory
SRC=843 && EXP=0 && RUN

# a copy of memory needs rsi, rdi and rcx, the last resort registers on x86_64,
# below nine live scratch values and the bounds checks
SRC=844 && EXP=0 && RUN

# a constructor is called on a type parameter, T.at(...) builds a point in place
SRC=845 && EXP=0 && RUN

# a constructor called on a type parameter must exist for its argument
SRC=846 && COMPERR

# x86: running out of registers in nested inline calls reports who holds them and
# what a noinline frame would save
if [[ $MACHINE == x86_64 ]]; then SRC=847 && COMPERR; fi

# an operator other than equality does not compare user type instances
SRC=848 && COMPERR

# a whole array is not compared with an instance of its element type
SRC=849 && COMPERR

# an instance is not compared with a whole array of its type
SRC=850 && COMPERR

# a generic method needs type arguments when they cannot be deduced
SRC=851 && COMPERR

# an array field of a data instance is initialized with braces
SRC=852 && COMPERR

# a generic type without a body is rejected where the body should start
SRC=853 && COMPERR

# a generic function without a body reports the '}' that closes nothing
SRC=854 && COMPERR

# a type parameter is deduced from a later argument, nested calls are skipped
SRC=855 && EXP=0 && RUN

# a type parameter is not deduced from an expression argument
SRC=856 && COMPERR

# a type parameter is not deduced from an argument that is missing
SRC=857 && COMPERR

# generics that are never instantiated are listed in the footer, 'test-cli.sh'
# checks it
SRC=858 && EXP=0 && RUN

# x86: a function name longer than the register report is not wrapped
if [[ $MACHINE == x86_64 ]]; then SRC=859 && COMPERR; fi

# the code ends with the jump of a loop, no function body follows it
SRC=860 && EXP=3 && RUN

# two instances of a generic 'noinline' function have unique labels
SRC=861 && EXP=6 && RUN

# a non-inline 'bool' result in conditions, expression and nested arguments
SRC=862 && EXP=17 && RUN

# a non-inline argument that is narrowed to its parameter is rejected
SRC=863 && COMPERR

# the unary operators of a non-inline call apply to its result
SRC=864 && EXP=100 && RUN

# the receiver or first slot of a non-inline call is passed in a register
SRC=865 && EXP=40 && RUN

# instance arguments that are not variables, to an inlined and a non-inline
# function
SRC=866 && EXP=72 && RUN

# a type parameter is deduced from an instance literal, a constructor call and
# a call result
SRC=867 && EXP=81 && RUN

# a bare instance literal does not tell the type parameter
SRC=868 && COMPERR

# a noinline function that nothing calls leaves no code, a method that passes
# its own receiver on does not load it into the receiver register
SRC=869 && DIFFPY

# a constant argument has the type of its parameter in an inlined function
SRC=870 && COMPERR

# the same in a noinline function, the message is the same
SRC=871 && COMPERR

# a constant argument is narrowed like a variable of its parameter's type
SRC=872 && COMPERR

# a constant argument of a wider parameter, inlined and noinline
SRC=873 && EXP=22 && RUN

# two constant arguments are compared like variables of their parameters' types
SRC=874 && COMPERR

# a method of an alias clashes with the generic method defined before it
SRC=875 && COMPERR

# a generic method clashes with the method of an alias defined before it
SRC=876 && COMPERR

# an inlined recursion that does not end at compile time is rejected
SRC=877 && COMPERR

# a result that aliases an argument compiles on every target
SRC=878 && EXP=0 && RUN

# a delimiter where a definition should start is rejected
SRC=879 && COMPERR

# a delimiter where a statement should start is rejected
SRC=880 && COMPERR

# a delimiter where the single statement of a block should start is rejected
SRC=881 && COMPERR

# a function name is an identifier
SRC=882 && COMPERR

# a type without a name is rejected
SRC=883 && COMPERR

# a global variable is not initialized with a call
SRC=884 && COMPERR

# exit has no value to assign
SRC=885 && COMPERR

# a generic type has its body right after its parameters
SRC=886 && COMPERR

# a string cannot start a statement
SRC=887 && COMPERR

# a string after the branch of an if cannot start a statement
SRC=888 && COMPERR

# the destination of array_copy is an array
SRC=889 && COMPERR

# the compared of arrays_equal is an array
SRC=890 && COMPERR

# an array is not assigned the result of a call
SRC=891 && COMPERR

# an expression is not an array argument
SRC=892 && COMPERR

# a record literal initializes a global variable
SRC=893 && EXP=3 && RUN

# a variable shadows the function of the same name
SRC=894 && EXP=5 && RUN

# a data constant is not missing after a unary operator
SRC=895 && COMPERR

# a data constant is not a string
SRC=896 && COMPERR

# a boolean value is not a name
SRC=897 && COMPERR

# a global variable is initialized with a boolean expression
SRC=898 && EXP=3 && RUN

# a condition is not the call of a function without a result
SRC=899 && COMPERR

# a comparison is not made with the call of a function without a result
SRC=900 && COMPERR

# blocks nested more than 128 levels are rejected
SRC=901 && COMPERR

# expressions nested more than 128 levels are rejected
SRC=902 && COMPERR

# inlined calls nested more than 256 levels are rejected
SRC=903 && COMPERR

# an array field is not initialized with the name of its element type
SRC=904 && COMPERR

# a constant shadows the function of the same name
SRC=905 && EXP=8 && RUN

# a call checked for aliasing inside the body of a function that is not inlined
SRC=906 && EXP=0 && OPTS="--checks=noub" RUN

# an index constant that no element is that far from is rejected
if [[ $MACHINE == x86_64 ]]; then SRC=907 && OPTS="--vars=0x40000" COMPERR; fi

# a comment on the last line without a line end
SRC=908 && EXP=3 && RUN

# a conversion of a struct value is rejected
SRC=909 && COMPERR

# an included file with an include of its own and the same file included twice
SRC=910 && EXP=42 && RUN

# an error in an included file is reported with the name of the file
SRC=911 && COMPERR

# an include after a definition is rejected
SRC=912 && COMPERR

# a file that includes the file that includes it is parsed once
SRC=913 && EXP=3 && RUN

# an included file that does not exist is rejected
SRC=914 && COMPERR

# a definition in an included file is reported with its file at the clash
SRC=915 && COMPERR

# an include without a file name in quotes is rejected
SRC=916 && COMPERR

# an include of a directory is rejected
SRC=917 && COMPERR

# an included file without content adds nothing
SRC=918 && EXP=7 && RUN

# a unary operator before a hex or binary constant
SRC=919 && EXP=0 && RUN

# the mark under a token counts characters, not bytes
SRC=920 && COMPERR

# a constant shift count that is not a byte is written as one, rv32i rejects
# a count outside the width
if [[ $MACHINE == x86_64 ]]; then SRC=921 && EXP=0 && RUN; fi

# a conversion truncates to its type also when the value is stored in a wider
# variable, and is computed at its own width in a narrow expression
SRC=922 && EXP=0 && RUN

# a folded conversion keeps its type as the first element of an expression
SRC=923 && EXP=0 && RUN

# a constant expression argument is folded at the width of its parameter
SRC=924 && EXP=0 && RUN

# a string cannot span lines, the source is echoed in assembly comments
SRC=925 && COMPERR

# --checks=alias: a 'foo' element is not assigned a value that reads the array
SRC=926 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a 'foo' element assigned from itself, another array and a
# separate variable is accepted
SRC=927 && EXP=0 && OPTS="$UB_ALIAS" RUN

# --checks=alias: a 'foo' element reading other fields of its variable is
# accepted, reading its own field is not
SRC=928 && OPTS="$UB_ALIAS" COMPERR

# --checks=alias: a result and an argument that are different fields are accepted
SRC=929 && EXP=0 && OPTS="$UB_ALIAS" RUN

# --checks=alias: a result and an argument that are the same field conflict
SRC=930 && OPTS="$UB_ALIAS" COMPERR
