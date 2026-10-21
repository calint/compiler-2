# baz: how the compiler works

The design documents of the compiler, in reading order: each chapter
assumes the ones before it.

For a reviewer:

1. the language: 'README.md', then 'etc/tutorial.baz'
2. the code: the reading order in 1.1, the choices in chapter 7
3. build: './make.sh build' (clang++ with c++26), one translation unit
4. test: 'qa/coverage/test-coverage.sh --target=x86_64 run' and
   '--target=rv32i-fpga run' run the about 1100 cases of 'cases.sh' on the
   two main targets; 'qa/coverage/test-all.sh' runs them on all four
   targets with the other suites and a coverage report in
   'qa/coverage/report/' (needs nasm, ld, llvm-mc, ld.lld, qemu-riscv32 and
   qemu-system-riscv32)
5. lint and format: 'qa/lint/clang-tidy.sh', 'qa/lint/format-source.sh'
6. fuzzing: 'qa/fuzz/fuzz-all.sh', see 'qa/fuzz/README.md'
7. known limitations and planned work: 'etc/todo.txt', the reasons of past
   decisions: 'etc/todo-resolved.txt'

## Contents

- [1. Architecture: from source to assembly and binary image](#1-architecture-from-source-to-assembly-and-binary-image)
    - [1.1 Pipeline](#11-pipeline)
    - [1.2 Passes](#12-passes)
    - [1.3 Names during compile: frames and aliases](#13-names-during-compile-frames-and-aliases)
    - [1.4 Dry runs](#14-dry-runs)
    - [1.5 Checks](#15-checks)
    - [1.6 Reports](#16-reports)
- [2. Generic types: how 'type' with parameters becomes real types](#2-generic-types-how-type-with-parameters-becomes-real-types)
    - [2.1 The three kinds of 'type'](#21-the-three-kinds-of-type)
    - [2.2 Plain type](#22-plain-type)
    - [2.3 Generic type](#23-generic-type)
    - [2.4 Generic type based on another generic type](#24-generic-type-based-on-another-generic-type)
    - [2.5 Alias](#25-alias)
    - [2.6 Building the instance, level by level](#26-building-the-instance-level-by-level)
    - [2.7 Methods](#27-methods)
    - [2.8 Errors](#28-errors)
    - [2.9 Order rules](#29-order-rules)
    - [2.10 Generic functions](#210-generic-functions)
- [3. Access paths: aliasing check of what is reached through run-time indexes](#3-access-paths-aliasing-check-of-what-is-reached-through-run-time-indexes)
    - [3.1 Problem](#31-problem)
    - [3.2 The path](#32-the-path)
    - [3.3 Why paths and not bytes](#33-why-paths-and-not-bytes)
    - [3.4 Comparing two paths](#34-comparing-two-paths)
    - [3.5 Building paths](#35-building-paths)
    - [3.6 The other checks on a read](#36-the-other-checks-on-a-read)
    - [3.7 Fuzzing](#37-fuzzing)
- [4. Noinline calls: aliasing validated by a dry run of the body](#4-noinline-calls-aliasing-validated-by-a-dry-run-of-the-body)
    - [4.1 Problem](#41-problem)
    - [4.2 Dry run](#42-dry-run)
    - [4.3 Signature: checked once per kind of call](#43-signature-checked-once-per-kind-of-call)
    - [4.4 Temporaries](#44-temporaries)
- [5. Versions of the same code: compiled, measured, the shorter kept](#5-versions-of-the-same-code-compiled-measured-the-shorter-kept)
    - [5.1 Why](#51-why)
    - [5.2 Why so many](#52-why-so-many)
    - [5.3 State: each version starts from the same machine](#53-state-each-version-starts-from-the-same-machine)
    - [5.4 Testing the version that is dropped](#54-testing-the-version-that-is-dropped)
- [6. Addresses: how a path such as 'w.rooms[i].items[j].x' becomes an operand](#6-addresses-how-a-path-such-as-wroomsiitemsjx-becomes-an-operand)
    - [6.1 The operand](#61-the-operand)
    - [6.2 Walking the path](#62-walking-the-path)
    - [6.3 The scale](#63-the-scale)
    - [6.4 A constant added to an index](#64-a-constant-added-to-an-index)
    - [6.5 Addresses an inlined call computed](#65-addresses-an-inlined-call-computed)
    - [6.6 The bases](#66-the-bases)
    - [6.7 Registers](#67-registers)
    - [6.8 Where in the code](#68-where-in-the-code)
- [7. Design decisions](#7-design-decisions)
    - [7.1 No intermediate representation](#71-no-intermediate-representation)
    - [7.2 A node parses itself and keeps every token](#72-a-node-parses-itself-and-keeps-every-token)
    - [7.3 One base class for every node](#73-one-base-class-for-every-node)
    - [7.4 Generic definitions are text parsed again](#74-generic-definitions-are-text-parsed-again)
    - [7.5 Calls are inlined, parameters are aliases](#75-calls-are-inlined-parameters-are-aliases)
    - [7.6 'toc' is one facade over the tables](#76-toc-is-one-facade-over-the-tables)
    - [7.7 One translation unit](#77-one-translation-unit)
    - [7.8 The first error ends the compile](#78-the-first-error-ends-the-compile)

## 1. Architecture: from source to assembly and binary image

### 1.1 Pipeline

```text
main.cpp         options, the backend for '--target', error printing
  |
program.hpp      parse: every file, 'include' in place
  |              definite assignment of the results of the functions
  |              compile: globals, 'main', noinline bodies, handlers,
  |              read-only data, data and variables
  |              report after the code
  v
machine.hpp      one virtual method per operation the front end needs,
  |              e.g. 'add_subtract', 'copy', 'check_bounds'; allocates
  |              the registers ('register_pool')
  |              machine_x86_64.hpp, machine_rv32i.hpp and the bare-metal
  |              targets machine_rv32i_qemu.hpp and machine_rv32i_fpga.hpp
  v
assembler.hpp    buffers the lines, optimizes the jumps
                 assembler_x86_64.hpp: nasm text
                 assembler_rv32i.hpp: llvm assembler text and the flat
                 binary image of the rv32i targets
```

The build is one translation unit: 'main.cpp' includes the headers.

Reading order, each file assumes the ones before it:

```text
1. main.cpp, program.hpp       the driver and the passes
2. statement.hpp               the node interface every statement has
3. stmt_block.hpp, stmt_if.hpp, stmt_loop.hpp
                               control flow, the simplest nodes
4. expr_any.hpp, expr_arith.hpp, expr_bool.hpp, expr_type.hpp
                               values: arithmetic, conditions, instances
5. stmt_identifier.hpp         a place: a name with fields and indexes
6. stmt_def_func.hpp, stmt_call.hpp
                               functions, inlined and noinline calls
7. toc.hpp                     what the names are while compiling
8. machine.hpp, machine_x86_64.hpp, assembler.hpp
                               the operations and one backend
9. machine_rv32i.hpp, assembler_rv32i.hpp
                               the load/store backend and its image
```

Invariants the code relies on:

1. a statement parses in its constructor and emits nothing until it is
   compiled; a compile may run more than once ([1.4](#14-dry-runs)) and adds
   the same to 'toc' each time
2. the front end never writes assembly, it calls 'machine' operations
   with operands; the backend lowers them to what the target encodes
3. registers are freed in the reverse order of their allocation, an
   operation frees the registers it allocated before it returns
4. a name is resolved again at every compile through the frames, an
   inlined parameter is an alias of its argument, never a copy
5. a 'destination' is not changed after it is made, a field or element
   of it is a destination of its own
6. labels are named by source location and call path, so a body inlined
   twice or compiled twice gets the same or distinct labels as needed

### 1.2 Passes

1. parse ('program' constructor): every statement parses itself in its
   constructor and emits nothing, 'toc::begin_compile' ends the parse. The
   grammar needs to know the names, e.g. 'point{' is a record literal only
   when 'point' is a type, 'f<' is a generic call only when 'f' is a generic
   function, so the parse declares types, functions, constants and
   variables in 'toc' as it goes. Where a type or a size is learned from the
   initializer, the initializer is parsed, the tokenizer rewound and parsed
   again ('stmt_def_var::trial_parse'). A generic definition is kept as text
   and parsed again for each instance ('generics.hpp').

2. definite assignment ('assignment_analysis' in 'program.hpp'): every path
   through a function with a result sets it, see 'ub_assigned.hpp'.

3. compile ('program::build'): the statements are walked again with a fresh
   scope stack in 'toc'. Global variables and data are declared again, then
   'main' is compiled where the program starts. An inlined call compiles the
   body of the callee in place ([1.3](#13-names-during-compile-frames-and-aliases)). A noinline call adds a body to
   compile for the array lengths of its arguments; the bodies are compiled
   after 'main', and compiling one can add more. Then each function that no
   compile reached is checked and emits nothing ([1.5](#15-checks)). Then the
   failure handlers of the checks that are on, the read-only strings, the data
   and the variables.

4. output ('machine::finish', 'write_assembly'): the buffered lines get their
   jumps optimized, the assembly is written, the rv32i targets also write the
   binary image.

### 1.3 Names during compile: frames and aliases

'toc' keeps a stack of frames: blocks, loops, 'foo' loops and functions. What
a name means is decided by one rule, 'scope_stack::lookup': the frames of the
innermost function from the innermost outwards, then the root frame that
holds the globals; the first frame that declares the name decides. A frame
declares a variable or a constant of a name, not both, a second declaration is
rejected. The body of a function is the block above its frame, a name declared
there that repeats a parameter or the result is rejected, a deeper block or a
'foo' may hide it ('definition_checks::assert_not_hiding_signature'). In a
function frame a parameter hides a constant of the generic instance, which is
the context of the definition. Every question about a name asks this lookup:
the resolver ('ident_resolver'), the aliasing check, and the lookaheads of the
parser, where a variable, a parameter or a constant hides a type or a generic
function of its name, e.g. after 'let point = 3' the 'point' of 'point + 1' is
the constant. A name followed by '(' is a call, no value is called. The same
statement can resolve to another variable, register or constant each time it
is compiled.

An inlined call does not copy its arguments. It enters a function frame that
maps each parameter name to the argument ('alias_info'), e.g. 'p' to
'world.rooms[i]' with the address of that element computed once, '3' to the
constant 3, or 'x' to a scratch register holding the value of an expression.
The result name maps to the destination of the call. Resolving 'p.x' in the
body follows the alias and continues the walk in the frames of the caller.
The result has the type of the destination, except that a narrower integer
widens into an operand of an operator, e.g. the 'g()' of 'w + g()'
('destination::as_operand'): an inlined result is made in a register of its
type and copied. 'stmt_call::compile' checks the result before the inlined and
the noinline paths split, so both accept the same.

A noinline function has a body of its own and a frame in memory that holds the
addresses of its result and arguments ('stmt_call.hpp', 'noinline_call').

What resolution gives is an 'ident_info': a constant, a register or a
variable with its memory operand and the deepest address known from the calls
('known_address'). 'statement::compile' writes its value to a 'destination':
nowhere for a statement, a register, or the storage a resolved name is at.

### 1.4 Dry runs

Some code is compiled and then dropped: a dry run only to be checked, and
the longer of two versions after both are measured:

1. 'toc::check_only' and 'machine::emit_discarded': the body of a noinline
   function compiled as if inlined with the arguments of a call, for the
   aliasing errors of that call, see
   [chapter 4](#4-noinline-calls-aliasing-validated-by-a-dry-run-of-the-body),
   and the body of a function that no compile reached ([1.5](#15-checks));
   'toc::dry_run_scope' and 'machine::version_scope' put back what it
   changed: the registers, the frame depth, the string constants and the use
   of storage, and the routines, helpers and failure stubs it marked used
   ('machine::mark_used') are dropped
2. 'toc::emit_most_efficient' and 'machine::emit_most_efficient': two versions
   are compiled, each from the state before it, and the one with fewer
   instructions is kept with the state it ended in, e.g. with and without a
   scratch register, see [5.3](#53-state-each-version-starts-from-the-same-machine)

So a statement can be compiled more than once. What its compile adds to 'toc'
must be the same the second time: labels are named by source location and
call path, string constants with the same text share a label, a noinline body
and a checked call are added once.

### 1.5 Checks

'--checks' ('check_options' in 'toc.hpp') turns on run-time checks, which the
machine emits as a compare and a branch to a failure handler, and compile-time
rejections (aliasing, narrowing, uninitialized reads). 'toc::checks' gives
the options, they build the option records of the machine methods.

The rules checked while compiling, e.g. a constant index in bounds, a
constant that fits its variable, 'break' in a loop, hold in every function,
also in one that nothing calls, so whether a program is accepted does not
depend on its calls. 'function_table' records the functions whose body a
compile reached, in place, as a noinline body or in a dry run; after the
noinline bodies, 'program::check_unreached_functions' compiles each other
function like a noinline body under 'toc::check_only'
('stmt_def_func::check_body'). Its errors have no "called from" frame. An
unsized array parameter has no length there ('ident_info::is_length_unknown'),
so the rules that need the length are left to the calls: 's[7]' and
'array_copy(s, d, 2)' are accepted, 's[-1]' is not. The 'n' of a 'foo' over
it is a read-only variable there and in the parse, not a constant, so
'let m = n' is not folded before a call gives the length. An error that
depends on the arguments of a call cannot be told without one
('compile_error::depends_on_call'): running out of scratch registers, e.g.
twelve parameters that one call gives the same argument, and the limits of
inlined calls, e.g. 'r(x - 1)' in 'r(x)' that a constant argument ends; such
an error ends the check and the function is accepted. An instance of a generic
definition is a function like the others: a use of a generic function makes
it, an alias of a generic type makes its methods, and an instance that nothing
calls is checked too; a generic function that nothing instantiates is listed
in the report ([1.6](#16-reports)).

### 1.6 Reports

After the code, as comments: noinline bodies, uninstantiated generics, jump
optimizations, usage ('report_renderer' in 'program.hpp'), and with
'--report=registers' the use of registers at the busiest point
('register_report.hpp').

## 2. Generic types: how 'type' with parameters becomes real types

### 2.1 The three kinds of 'type'

```text
type point { x, y }                       plain
type text<capacity> { ... }               generic
type text<capacity> = list<i8, capacity>  generic, based on another
type str = text<127>                      alias, a real type
```

A plain type is a type right away.

A generic type is not a type. It is a recipe. Its fields depend on the
parameters, so the size of an array or the type of a field is not known until
the parameters have values. Nothing is checked inside it yet; the compiler
only remembers where its text starts.

An alias names a generic type with arguments. Only an alias makes a real
type, and it does so by reading the recipe again with the arguments filled
in. Reading the text again, instead of substituting into a stored form, means
an instance is checked and laid out exactly like a type written by hand: there
is one way to read fields, not two that must agree.

An instance needs a name, because a type is printed in errors and used in
labels. The alias gives it that name, so 'text<127>' is not written in place
of a type; it is named once and the name is used.

### 2.2 Plain type

The fields are read between '{' and '}', separated by ',' (a trailing ',' is
allowed). A duplicate name is rejected, a field without a type name has the
default type, and the type is registered under its name.

### 2.3 Generic type

```text
type list<T type, capacity> {
    array T[capacity],
    len,
}
```

'T type' is a type parameter, 'capacity' is a constant parameter. A parameter
may not reuse the name of something that already exists, so that inside the
recipe a name has one meaning.

The braced block is skipped without reading the fields. An error in the fields
is therefore found only when an alias reads them: until then there are no
values to check them with. The text is kept so the source can be printed back.

### 2.4 Generic type based on another generic type

```text
type text<capacity> = list<i8, capacity>
```

This specializes a generic type without repeating its fields. The base must
already be a generic type. Its arguments are kept as written, because they may
name parameters ('capacity') that have no value yet; they are resolved when an
alias gives the values. The type has no fields of its own: its fields are
those of the base.

### 2.5 Alias

```text
type str = text<8>
```

The generic type must already be defined, and the count of arguments must
match. Each argument is checked against its parameter: a type parameter gets a
type, a constant parameter gets a constant. The result is a list of bindings
('capacity' = 8), from which the instance is built ([2.6](#26-building-the-instance-level-by-level)),
'str' is registered as a real type, and the methods are made
([2.7](#27-methods)).

### 2.6 Building the instance, level by level

Starting at the generic type the alias names, the bases are followed down
until one has fields of its own.

```text
str = text<8>
  level 1: text, bindings capacity = 8
           base is list<i8, capacity>
  level 2: list, bindings T = i8, capacity = 8
           no base: read the fields here
```

At a level with a base, the parameters of that level are made visible in a
temporary block ('capacity' becomes the constant 8, a type parameter becomes a
name for its type) and the base arguments are resolved in it, giving the
bindings of the next level. The block is then closed. Each level has its own
parameters, and closing the block lets a base reuse the same parameter names
without a clash.

At the last level the parameters are made visible the same way, the fields are
read from the saved start of the generic definition as for a plain type, and
they are added to the type being built. All levels build the same type, so
'str' gets the fields 'array i8[8]' and 'len'.

Every level is recorded as an instance of its generic type with its bindings.
This is what lets the methods of every level be made for 'str', and what lets
an error name each step that led to it.

### 2.7 Methods

```text
func list.is_full() res bool { ... }
```

A method of a generic type is a recipe too, for the same reason as the fields:
its body depends on the parameters. Every instance of the type gets its own
copy, read again with the bindings of that instance visible.

1. When an alias is made, the methods are made for every level, starting at
   the deepest base. So 'str' gets the methods of 'list' first, then those of
   'text', and a method of 'text' can call one of 'list'.
2. When a method of a generic type is defined after some aliases already
   exist, it is made at once for each of them. Otherwise the order of methods
   and aliases in the source would decide which instances have the method.

### 2.8 Errors

An error in an instance is caused by the arguments but is in the generic
text. Reporting only one of the two places is not enough to fix it, so the
error is reported where the bad code is, and then each step that led there is
listed with "instantiated by".

```text
type list<T type, capacity> { ... }
type text<capacity> = list<capacity, capacity>
type str = text<8>

example.baz:9:28: generic parameter 'T' of 'list' needs a type, got the
                  constant 'capacity'
type text<capacity> = list<capacity, capacity>
                           ^~~~~~~~
example.baz:11:6: instantiated by
type str = text<8>
     ^~~
```

For an error inside a base, there is one extra frame per base, showing the
base with the arguments it got, e.g. 'list<i8, 8>', before the alias frame.

### 2.9 Order rules

1. A generic type is defined before an alias or a based generic type names
   it, since the alias reads it at once.
2. A method of a generic type may come before or after the aliases
   ([2.7](#27-methods), item 2).

### 2.10 Generic functions

```text
func make<T type>(n) res T { ... }
func same<T type>(a T, b T) res bool { ... }
func tokenizer.to<T type>() res T { ... }
```

A generic function has type parameters only; a constant parameter is
rejected. Like a generic type it is a recipe: the text after 'func' is kept
and the body is not checked until a call needs an instance.

A constructor cannot be generic: it is called as 'point.at(...)', with no
place for type arguments, so 'func point.at<T type>(v T) self' is rejected at
its 'self'.

#### 2.10.1 Definition

Each type parameter has the kind 'type' and may not reuse the name of
something that already exists.

So that a call can omit the type arguments, the definition records for each
type parameter where a call can find it:

1. the first parameter declared with exactly that type, e.g. 'a T'; an array
   parameter 'a T[]' does not count, since the argument's type is an array of
   'T', not 'T'
2. whether the result is declared with exactly that type, 'res T'

A method of a generic type with type parameters of its own, e.g.
'func text.append<T type>(s T)', is one generic function per instance of the
type ('str.append', 'name.append'). Errors name it after the generic type,
'text.append', which is the name written in the source.

#### 2.10.2 Call: the type arguments

A call gives the type arguments in one of two ways:

```text
make<str>(3)       written: T = str
same(s, t)         deduced from the argument 's': T = the type of 's'
x = tz.to()        deduced from the destination: T = the type of 'x'
```

Written, after '<', the count must match the type parameters and each
argument names a type; a constant is rejected.

Deduced, without '<', each type parameter is looked for in two places, in
this order. The examples use 'make' and 'same' from the start of
[2.10](#210-generic-functions) and a type 'name'.

1. The argument for the parameter declared with it (2.10.1, item 1). For
   'same(a T, b T)' that is the first argument; the second is not looked at.

   The argument is not parsed yet: it is parsed with the type its parameter
   has in the instance, so the instance must be known first (2.10.4,
   item 2).
   Instead the argument is read ahead as text, and only an argument whose
   type shows in that text tells the type:

   1. a variable, a field or an element that is the whole argument

      ```text
      same(s, t)              T = the type of 's'
      same(p.x, q.x)          T = the type of the field 'x'
      same(a[i], t)           T = the element type of 'a'
      ```

   2. a value that names its type: an instance literal, a constructor call,
      or a call of a function that is not generic and returns a value

      ```text
      same(name{}, t)         T = name
      same(point.at(1, 2), p) T = point
      same(mk(), t)           T = the return type of 'mk'
      ```

      A generic function does not count: its return type is not known
      until it is instantiated.

      Only such a deducing argument names the type of its literal: where
      the parameter type is known, e.g. 'same<name>({}, t)', the literal is
      written without it, like a field, an element or an assigned value
      ('literal_typing' in 'expr_type.hpp').

   3. anything else tells nothing, and step 2 is tried

      ```text
      same(1, t)              a literal: its type depends on where it goes
      same({}, t)             '{}' does not name a type
      same(s + 1, t)          an expression, not the whole argument
      same(s.get(), t)        a method call
      same()                  the argument is missing
      ```

2. The destination, when the result is declared with it (2.10.1, item 2):
   the type the call is assigned to.

   ```text
   var m = name{}
   m = make(2)                 T = name, the type of 'm'
   ```

   'var m = make(2)' has no destination type: 'm' takes its type from the
   call, so there is nothing to take it from.

3. If neither tells the type, the call is rejected and asks for the type
   arguments to be written:

   ```text
   example.baz:15:13: cannot deduce 'T' of generic function 'make', write
                      'make<...>'
   var m = make(2)
           ^~~~
   ```

   Written, the same call is 'var m = make<name>(2)'.

A generic method called without '(' and without '<' is rejected with "needs
type arguments", since there is nothing to deduce from.

#### 2.10.3 Call: the instance

The instance is named by the generic name and the type names, e.g.
'make<str>' or 'tokenizer.to<name>'. A label has no '<', '>' or ',', so there
it is 'tokenizer.to..name'. The '..' keeps two instances apart: a name has no
'..', so 'f<a,b>' is 'f..a.b' and the method instance 'f.a<b>' is 'f.a..b'.

An existing instance is used as it is, so each combination of types is
compiled once. Otherwise the definition is read again from its start with
each type parameter naming its type argument (and the generic type of a method
naming its instance), and the result is added as an ordinary function.

From then on the call is a call of an ordinary function: its return type is
the type of the call, its arguments are checked like those of any call, and it
is compiled inline or noinline like any function. Nothing after the instance
is made needs to know it came from a generic.

An error in the body of the instance is reported where the bad code is, then
the call that needed the instance, for the same reason as in
[2.8](#28-errors):

```text
example.baz:9:9: field 'size' not found in type 'name'
    res.size = n
        ^~~~
example.baz:13:13: called from 'make<name>'
    var m = make<name>(2)
            ^~~~
```

#### 2.10.4 Order rules

1. A generic function is defined before a call names it.
2. The type arguments are fixed before the arguments are parsed, so an
   argument is parsed with the type its parameter has in the instance. This
   is why deduction looks ahead instead of parsing the argument first.

## 3. Access paths: aliasing check of what is reached through run-time indexes

### 3.1 Problem

Two accesses to the same root variable alias when they can reach a common
byte. With a run-time index the byte is not known, so the check must answer
"can these ever be the same byte for some value of the indexes".

The check answers false only when the accesses are provably disjoint.
Anything unknown is "may alias": a missed alias is undefined behavior in the
compiled program, a false alarm only asks for the code to be written
differently.

### 3.2 The path

An access is described by the path from its root variable: one step per
field, constant index or run-time index. The root variable itself is not a
step.

```text
step         holds
field        offset of the field in its parent
fixed index  the index
run-time     nothing

rooms[i].items[1].name

steps:  index (run-time)  field 'items'  index (constant 1)  field 'name'
```

An index always follows a field or the root variable. Both paths of a
comparison start at the same root variable and follow the same types, so at
one position they are both fields or both indexes.

A field is identified by its offset in the parent. Fields never overlap (every
field has a size, a type has a field and an array has at least one element),
so different offsets are different fields.

### 3.3 Why paths and not bytes

A run-time index turns a single byte range into a set of ranges, one per
element, spaced by the element size. Comparing two such sets as bytes means
lining up periods, slots and alignment, and a constant index after a run-time
index makes the set harder still to describe. A path keeps the structure that
the bytes lose: two accesses that take different fields at the same position
can never meet, whatever the indexes are. The check never compares bytes, so
it is exact for every path the language can write.

### 3.4 Comparing two paths

The steps line up, so they are walked together:

```text
run-time index on either side             => go on, may be the same
field, different offset in the parent     => disjoint
constant indexes, different               => disjoint
same field (same offset) or same index    => go on
paths ran out                             => may alias (one is inside the
                                             other, or they are the same)
```

Examples:

```text
'items[i].id' and 'items[j].name'
    different fields                                  => disjoint
'items[i].name' and 'items[j].name'
    same field, run-time index                        => may alias
'es[1].items[i].f' and 'es[k].other'
    [1] and [k] go on, 'items' and 'other' differ     => disjoint
'arr[1]' and 'arr[2]'
    different constant indexes                        => disjoint
'arr[1]' and 'arr[i]'
    run-time index                                    => may alias
'es' and 'es[i].other'
    the first path ran out                            => may alias
```

### 3.5 Building paths

An identifier adds a step per element of what it names: '.name' adds a field,
'[3]' a constant index, '[i]' a run-time index.

An alias (a parameter, a result, the element 'e' of a 'foo') is bound to the
path of what it names, and an access through it is the path of the target
followed by the path of the access inside it. This way an access through any
number of aliases is compared as if it were written out on the root variable.
The element 'e' of a 'foo' adds a run-time index, since it is any element of
the array.

A target whose path is not known has no path. It stands for the whole
variable and always may alias.

### 3.6 The other checks on a read

Other checks look at the reads of a variable and do need bytes, because they
ask whether a read is inside what has been assigned:

```text
type point {x, y}

var x = x + 1                  the variable is not set while its value is
                               computed: bytes 0..4 are read, none are
                               assigned => "use of uninitialized variable"

func point.at(x) self {        a constructor must assign every field: it
    self.x = x                 assigns bytes 0..4, bytes 4..8 are not
}                              assigned => "function may return without
                               setting its return value"

func make() res point {        the same for the result of a function
    res.x = 1
}

p = {p.y, p.x}                 the field 'x' is written first, bytes 0..4,
                               then the item 'p.x' reads them => "'p.x' is
                               read but has been overwritten in the same
                               statement"
```

These checks share one walk over the reads, which hands out both the bytes and
the path of each access, so each check takes what it needs.

The root variable is the whole variable, and each field and each constant
index narrows it. After a run-time index the element is unknown, so the bytes
stay the whole array: a write through 'arr[i]' assigns nothing, and a read
through it needs the whole array to be assigned. Both are the safe answer when
the element is not known.

### 3.7 Fuzzing

The aliasing check is tested against a brute force of the bytes, in which
every value of every run-time index is tried. Two fuzzers do this:

1. one compares the path comparison with the brute force on random types and
   pairs of paths; the answer must be exact, neither unsound (a missed alias)
   nor imprecise (a false alarm)
2. one sends random programs through the compiler: the element 'e' of a 'foo'
   and 'mut' parameters, inline and noinline, against a read of the variable,
   with mixed scalar sizes; "may share storage" must be reported exactly when
   the bytes can be the same

A long run, one seed per core with large counts, takes about six minutes on 16
cores and is meant for any change to the check.

## 4. Noinline calls: aliasing validated by a dry run of the body

### 4.1 Problem

An inline call is compiled at the call site, so its parameters are aliases of
the arguments and the aliasing check ([chapter 3](#3-access-paths-aliasing-check-of-what-is-reached-through-run-time-indexes)) sees
which arguments name the same variable. A noinline body is compiled once and
receives addresses, so it cannot know which arguments are the same variable.
Whether a body is unsafe depends on the call, so the error ("may share
storage") has to be reported at the call, not once for the body.

### 4.2 Dry run

At each noinline call, before the call is compiled for real, the body is
compiled as if it were inlined with these arguments, and the result is
dropped. The aliasing errors that this inline compilation raises are the
errors of the call. An error carries the call as a frame, so it points both at
the call and at the line in the body.

This reuses the inline check as it is. A separate analysis of which parameters
a body writes and reads would duplicate the check and could disagree with it.

A dry run must leave no trace, or the program would change depending on which
calls were checked:

1. the code is discarded
2. string constants added meanwhile are removed
3. the frame depth and the peak storage size are restored; the locals of the
   body live in the callee's own frame, so the caller's frame capacity is not
   checked against them
4. a noinline call reached inside a dry run does not emit an instance of its
   function

### 4.3 Signature: checked once per kind of call

A dry run costs a compilation of the body, so it is not repeated for calls
that cannot differ. What the body can reject depends only on which bytes each
argument passes: which arguments are in the same variable, which bytes of it
they are, and which are globals (the body may name the global too). The call
is reduced to a signature:

```text
name + for the result and each argument:
    ' array N'          when an array, the body can reject an index by
                        the length
    ' global g' path    a global, by name
    ' local K' path     a local, K is the order of first appearance
    path                the bytes passed, the access path of chapter 3:
                        '.8' the field at offset 8, '[2]' element 2, '[?]'
                        an element at a run-time index, '*' when not
                        known, nothing for the whole variable
+ ' temporary' or ' variable' per argument

f(v, v, n)       "f local 0 local 0 local 1 variable variable variable"
f(v, w, n)       "f local 0 local 1 local 2 ..."
f(x, y, z)       same as the line above, checked once
f(p.x, p.y, n)   "f local 0.0 local 0.8 local 1 ...", 'p' has two 'i64'
f(g[i], w, n)    "f global g[?] local 0 local 1 ..."
```

Locals are numbered by first appearance, not named, because which local is
passed does not matter, only which arguments are in the same one and which
bytes of it they are. A global is named, because the body may refer to it
directly. Two calls that pass different bytes of the same variable may get
different verdicts, so the path is part of the signature.

A signature already seen is not dry run again. The signature is recorded
before the dry run starts, so a call of the same kind reached inside the body
(recursion) stops at once. The kinds are finitely many, so nested dry runs
end.

A pointer argument cannot tell which variable it names, so it gives no
signature: the dry run is skipped and no aliasing is reported for it.

### 4.4 Temporaries

A value without storage (a constant, 'point{1, 2}') is passed in a temporary
of the call. The dry run names it as a temporary argument, not as a register
of the caller that the body does not see. The body is checked with the value
of the first call of its kind, so whether an argument is a temporary or a
variable is part of the signature.

## 5. Versions of the same code: compiled, measured, the shorter kept

### 5.1 Why

Some statements can be compiled in more than one way, and which way is
shorter depends on code that has not been compiled yet:

1. a memory destination: computing in place costs a load and a store per
   operation on rv32i, a scratch register costs one extra store
2. a narrow scratch register is normalized after each operation on rv32i, a
   wide one needs none because the store truncates
3. an indexed address on rv32i (no base + index addressing): used as it is,
   every access adds base and index again; folded into a register, it costs
   one 'add' but holds a register
4. a displacement kept in each access ('28(t1)') saves an 'addi' unless the
   field offsets then exceed the 12-bit immediate and each access needs a
   'lui'/'add'

The answer depends on how many times an inlined body accesses an argument and
on the offsets of the fields it reaches. Predicting this means a cost model
that duplicates the code generator and must stay in step with every backend
change. Compiling each version and measuring is exact and needs no model.

The versions are measured in machine instructions, a pseudo instruction
counts as the ones it expands to. The shorter is kept; on ties the plain
version, since it holds no extra register.

### 5.2 Why so many

A version compiles everything under it, including the choices inside it, so a
choice in an inlined body is compiled once per version of every choice above
it. The work is the product of the version counts along a path, not the sum.

It stays polynomial in the depth: a version that folds an address into a
register gives the body an argument that is no longer indexed, so only the
version that keeps the index passes the choice down.

Choices made for a chain of inlined calls, each passing an indexed argument
on:

```text
depth of the chain    0    1    2    3    4    5    6    7    8
x86_64 choices        1    2    3    4    5    6    7    8    9
rv32i choices         5   12   21   32   45   60   77   96  117
```

x86_64 has base + index addressing, so only the assignment chooses, once per
body. rv32i grows as (n + 1) * (n + 5). The trade is compile time for code
size: the depth 8 chain compiles in about 75 ms on rv32i against 12 ms on
x86_64.

### 5.3 State: each version starts from the same machine

A version changes more than the lines it emits. 'machine::version_scope' saves
what it changes in the machine and puts it back when the version ends, also
when an error unwinds it:

1. the registers held, with what a passed lower bounds check proved
2. the trace of the calls, which the register report and errors print
3. the most scratch registers in use, the footer of the output
4. the routines, helpers and failure stubs marked used
   ('machine::mark_used'), a set of values like the registers: those of a
   dropped version or of discarded code are dropped

Each version starts from the saved state; after the choice the state the kept
version ended in remains. So the footer and '--report=registers' describe the
code that is in the output, and a routine only a dropped version calls is not
emitted.

A version that runs out of scratch registers is not available and the other
one is kept, e.g. an inlined call with fifteen indexed arguments on rv32i
where folding each address into a register would need more registers than
there are. The error carries the kind 'compile_error::error_kind::
out_of_registers', which is set where the error is made; when both versions
run out, the error of the first is reported. Any other error is an error of
the program and ends the compile. An error of discarded code is never one of
a version: discarded code starts from the registers of a noinline body, which
do not depend on the code around it.

What a failed version entered must be left when its error unwinds:

1. 'assembler::capture' drops the lines it captured
2. 'toc::emit_most_efficient' pops the frames and their storage that an
   inlined body entered back to the frames of the caller
3. the rv32i bulk registers of an array copy or comparison are left
   ('machine_rv32i::bulk_depth_scope')

The registers that 'machine::address_scope' and the call frames that
'machine::call_frame_scope' hold are left by their own scopes.

### 5.4 Testing the version that is dropped

Only the kept version runs, so a miscompile in the other one would stay
hidden. '--versions=first' and '--versions=last', for testing, keep that
version instead of the smaller, a version that runs out of registers is still
not kept. 'qa/coverage/test-versions.sh' runs the cases of 'cases.sh' that
run the program with each of them on x86_64 and rv32i-fpga
('VERSIONS=first test-coverage.sh run') and compares the exit codes and
outputs with the expected ones.

## 6. Addresses: how a path such as 'w.rooms[i].items[j].x' becomes an operand

### 6.1 The operand

A variable, a field or an element is reached through one memory operand:

```text
[base + index * scale + displacement]
```

The front end builds it ('compile_lea' in 'stmt_identifier.hpp', named after
the x86 instruction because the result is what 'lea' would compute) and the
backend lowers it to what the target encodes:

```text
x86_64   base and index registers, scale 1, 2, 4 or 8, a 32-bit displacement
rv32i    a base register and a 12-bit offset; an index, a scale and a larger
         displacement are added into a register right before the access
```

So the front end never asks how an access is encoded, it only asks the
backend which scales it can lower ('can_lower_index_scale'). The operand holds
one index register; the second run-time index of a path needs the address so
far in a register first.

### 6.2 Walking the path

'compile_lea' walks the elements of the path from the root variable and keeps
the operand built so far:

```text
start      the storage of the root variable, '[rbp + 0]'; or the address of
           a parameter of a noinline function, loaded from its frame slot;
           or an address an inlined call already computed (6.5)
field      its offset is added to the displacement
constant   index * element size is added to the displacement, the index is
index      checked against the array size at compile time
run-time   the index is computed into a register and becomes the index of
index      the operand, with the element size as the scale; an operand that
           already has an index is folded into a register first
           ('fold_indexed_address')
```

The example below has

```text
type item { tag i8, x }               x86_64: 16 B, 'x' at 8
type room { id, items item[4] }       x86_64: 72 B, 'items' at 8
type world { rooms room[8] }
var w = world                         at '[rbp]'
```

A path of constants only is one displacement, no instruction computes it:

```text
w.rooms[1].items[2].x = 6             1 * 72 + 8 + 2 * 16 + 8 = 120

    mov qword [rbp + 120], 6
```

Two run-time indexes:

```text
w.rooms[i].items[j].x = 5

    mov r15, qword [rbp + 576]        i
    imul r15, 72                      72 is not a scale x86 encodes (6.3)
    lea r15, [rbp + r15 + 8]          'rooms[i].items' folded, the operand
                                      had an index already
    mov r14, qword [rbp + 584]        j
    shl r14, 4                        16 is not encodable either
    mov qword [r15 + r14 + 8], 5      '+ 8' is the field 'x'
```

On rv32i ('item' is 8 B with 'x' at 4, 'room' is 36 B with 'items' at 4, 's0'
points 2032 B past 'vars', 6.6) the scale 8 of 'j' stays in the operand and
is lowered at the store:

```text
    lw t0, -1744(s0)                  i
    slli t1, t0, 3                    i * 36 as (i * 8 + i) * 4
    add t0, t1, t0
    slli t0, t0, 2
    add t0, s0, t0                    'rooms[i].items' folded
    addi t0, t0, -2028                'w' at -2032, 'items' at +4
    lw t1, -1740(s0)                  j
    slli t2, t1, 3                    the operand '[t0 + t1 * 8 + 4]' lowered
    add t2, t2, t0
    sw t3, 4(t2)
```

### 6.3 The scale

An element size the backend can lower stays the scale of the operand: 1, 2, 4
and 8 on x86_64, any power of two on rv32i (a shift at the access). Any other
size multiplies the index register first ('scale_index') and the operand gets
scale 1: 'imul' on x86_64, shifts and adds on rv32i (the non-adjacent form of
the factor, 'multiply_by_constant').

### 6.4 A constant added to an index

'w.rooms[i].items[j + 1]' needs no 'add': the '+ 1' becomes 16 more of
displacement ('compile_without_trailing_addend'):

```text
    mov qword [r15 + r14 + 24], 7     8 for 'x' and 16 for the '+ 1'
```

Only without bounds checks: a check compares the whole index with the array
size, so with '--checks=upper' the index is 'j + 1' in the register:

```text
    mov r14, qword [rbp + 584]
    add r14, 1
    cmp r14, 4
    jge baz_bounds_panic
    shl r14, 4
    mov qword [r15 + r14 + 8], 7
```

A run-time index is checked in its register before it is scaled. With the
count of a bulk operation, e.g. 'array_copy(a[i], b, n)', the check covers
'i + n' ('lea_request::reg_count').

### 6.5 Addresses an inlined call computed

An argument with a run-time index is computed once at the call, the body of
the inlined function reaches it through its parameter. The alias of the
parameter keeps the operand ('bound_storage::lea'):

```text
w.rooms[i].items[j].set(8)            func mut item.set(v) { self.x = v }

    ...                               r15 and r14 as in 6.2
    alias self -> w.rooms.items (lea: r15 + r14)
    mov qword [r15 + r14 + 8], 8      'self.x'
```

A path in the body starts at that address instead of at the root variable
('known_address'): its 'trailing_count' is the number of elements of the path
after the parameter, 1 for the 'x' of 'self.x', so 'compile_lea' starts at the
element the address belongs to and adds only the rest. The registers stay
allocated until the body is compiled. On rv32i the body may instead use the
address folded into one register, whichever is shorter (5.1).

### 6.6 The bases

x86_64 addresses variables from 'rbp', which points at the data. rv32i
addresses them from 's0', which points 2032 B past the start of 'vars', so
the 12-bit offsets, -2048 to 2047, reach the first 4080 B of variables
directly ('variables_base_past_vars_bytes'), further ones need a 'lui'. A noinline body addresses its locals from
its frame base, 'rbx' or 's1'.

### 6.7 Registers

The scratch registers of a path ('path_registers') are the caller's until the
path allocates its own. A fold may overwrite only registers that are the
path's own ('fold_register'):

```text
1. the register the caller loads the final address into anyway
2. a base the path loaded itself, a pointer or an earlier fold
3. the index register being folded, the next index then gets a new one
4. otherwise a new register
```

The caller frees all of them after its access, in reverse order of
allocation.

### 6.8 Where in the code

```text
stmt_identifier::compile_lea          the walk of 6.2
stmt_identifier::compile_indexed_element
                                      a constant or a run-time index
stmt_identifier::add_index            the index register and the scale
stmt_identifier::fold_indexed_address the second index
stmt_identifier::compile_checked_index
                                      the bounds check and the '+ c' of 6.4
toc::get_lea_operand                  an operand without a path to walk is
                                      the storage of the name
ident_builder::place_operand_from_lea a built-in reached below a known
                                      address, for the resolver
machine_x86_64::with_lowered_addresses, machine_rv32i::lower_address
                                      what the target encodes
```

## 7. Design decisions

The choices a reader may question first, why they were made and what was
rejected. The open costs of each are items of 'etc/todo.txt'.

### 7.1 No intermediate representation

A statement compiles itself by calling the operations of 'machine' with
operands, the backend writes the instructions. There is one level to read,
the output reads like hand-written code and every statement is quoted above
its instructions.

Rejected: a lower-level representation and passes over it, another layer to
learn for optimizations that are local here; where the shorter of two forms
depends on code not compiled yet, both are compiled and measured (chapter 5).

Cost: no optimization across statements, e.g. a value stored and loaded again
by the next statement ('etc/todo.txt' section 2).

### 7.2 A node parses itself and keeps every token

A call is checked against its callee while it is parsed: the type arguments
of a generic call are deduced from its arguments, and an argument of a 'mut'
parameter must be writable. The grammar uses the names too: 'point{' is a
record literal only when 'point' is a type, 'f<' a generic call only when 'f'
is a generic function, 'T.at' a constructor only when 'T' names a type (1.2).
So a statement parses in its constructor and declares names in 'toc' as it
goes. It keeps every token with its whitespace and comments: the source is
reproduced exactly ('--reproduce-source' checks it in every test) and quoted
in the comments of the output.

Rejected: a tree built before the names are known, the checks of a call would
move to a second walk over the tree; generic bodies are already kept as text
(7.4), so the grammar alone would not need the names.

Cost: a name is defined before it is used, so two functions cannot call each
other, also not when they are 'noinline' ('etc/todo.txt' section 4).

### 7.3 One base class for every node

'statement' has 20 virtual methods, each node overrides the ones it needs
(the groups are listed above the class). A default that no other node reaches
is 'std::unreachable()', the others answer for a node that lacks the
capability, e.g. 'folded_constant' is empty. Every node parses, writes its
source back, compiles and takes part in the definite assignment walk. Code
asks a node a few capability queries ('is_expression', 'is_name_or_literal',
...), no code switches on the kind of a node.

Rejected: the nodes as a 'std::variant', a closed set gains nothing when
nothing switches on the kind, and every child would be boxed and every node
kind would change the visitors; one tree per concern, each node would become
four classes kept in step.

### 7.4 Generic definitions are text parsed again

An instance of a generic type or function is its definition parsed again with
each parameter naming its argument (chapter 2). An instance is then exactly
an ordinary definition: one code path, the same checks and the same errors,
with the instances that led to an error listed as frames.

Rejected: substituting arguments in a parsed tree, every node class would
need a copy, and the parser could not tell 'T{' or 'T.at' apart without
knowing what 'T' names.

Cost: an error in a generic definition shows at its first instance; the
generic functions nothing instantiates are listed in the report after the
code.

### 7.5 Calls are inlined, parameters are aliases

The programs are super loops, so a call is compiled in place by default and a
parameter names its argument instead of copying it: no call overhead, and a
constant argument folds through the body. 'noinline' gives a function a body
of its own for recursion and size.

Cost: a parameter and another name may reach the same bytes, so aliasing is
checked at compile time (chapters 3 and 4).

### 7.6 'toc' is one facade over the tables

Every statement receives 'toc', which owns the scopes, the types, the
functions, the generics, the data and the storage layout. A method of 'toc'
changes state or combines tables, a question to one table goes through its
const accessor, e.g. 'tc.scopes().is_alias(name)'.

'toc' is also the compile context: it carries the machine and whether the
program is being parsed or compiled, and whether a dry run is active. A
statement compiles with the names and the machine together, e.g. a variable
resolves to an operand that the machine then uses, and the mode decides what
a declaration does, e.g. no storage check while parsing; one parameter keeps
them consistent.

Rejected: passing the tables one by one, every signature would carry the
ones its callees need.

### 7.7 One translation unit

'main.cpp' includes the headers, there is no build system and the compiler
sees the whole program. The include cycles between the statement classes are
broken in 'decouple.hpp', 'decouple_impl.hpp' and 'expr_type_impl.hpp' only.
A forward declaration of a class used only by reference or pointer is the
opaque-pointer pattern and is also allowed: 'toc' in 'statement.hpp',
'stmt_def_dat' and 'stmt_def_func' in 'toc.hpp'.

Cost: every change rebuilds everything.

### 7.8 The first error ends the compile

An error is a 'compile_error' at the token that caused it, with the
inlined calls and generic instances that led there. No code recovers from an
error, so no error leads to a second, wrong one. The one exception is a
version of code that runs out of scratch registers: the other version is kept
and the error is dropped with the state of the failed version, see
[5.3](#53-state-each-version-starts-from-the-same-machine).

Cost: one error per run.
