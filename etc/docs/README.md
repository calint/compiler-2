# baz: how the compiler works

The design documents of the compiler, in reading order: each chapter
assumes the ones before it.

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
   after 'main', and compiling one can add more. Then the failure handlers of
   the checks that are on, the read-only strings, the data and the variables.

4. output ('machine::finish', 'write_assembly'): the buffered lines get their
   jumps optimized, the assembly is written, the rv32i targets also write the
   binary image.

### 1.3 Names during compile: frames and aliases

'toc' keeps a stack of frames: blocks, loops, 'foo' loops and functions. A
name is resolved by walking the frames from the innermost outwards
('ident_resolver'), so the same statement can resolve to another variable,
register or constant each time it is compiled.

An inlined call does not copy its arguments. It enters a function frame that
maps each parameter name to the argument ('alias_info'), e.g. 'p' to
'world.rooms[i]' with the address of that element computed once, '3' to the
constant 3, or 'x' to a scratch register holding the value of an expression.
The result name maps to the destination of the call. Resolving 'p.x' in the
body follows the alias and continues the walk in the frames of the caller.

A noinline function has a body of its own and a frame in memory that holds the
addresses of its result and arguments ('stmt_call.hpp', 'noinline_call').

What resolution gives is an 'ident_info': a constant, a register or a
variable with its memory operand and the deepest address known from the calls
('known_address'). 'statement::compile' writes its value to a 'destination':
nowhere for a statement, a register, or the storage a resolved name is at.

### 1.4 Dry runs

Some code is compiled only to be checked or measured, then dropped:

1. 'toc::check_only': the body of a noinline function compiled as if inlined
   with the arguments of a call, for the aliasing errors of that call, see
   [chapter 4](#4-noinline-calls-aliasing-validated-by-a-dry-run-of-the-body)
2. 'toc::measure_only' and 'machine::measure_code_size': the size of code,
   see [chapter 5](#5-versions-of-the-same-code-compiled-measured-the-shorter-kept);
   'toc::dry_run_scope' and 'machine::measure_scope' put back what it
   changed: the registers, the frame depth, the string constants and the use
   of storage
3. 'machine::emit_most_efficient': two versions are compiled and the one with
   fewer instructions is kept, e.g. with and without a scratch register

So a statement can be compiled more than once. What its compile adds to 'toc'
must be the same the second time: labels are named by source location and
call path, string constants with the same text share a label, a noinline body
and a checked call are added once.

### 1.5 Checks

'--checks' ('check_options' in 'toc.hpp') turns on run-time checks, which the
machine emits as a compare and a branch to a failure handler, and compile-time
rejections (aliasing, narrowing, uninitialized reads). 'toc::checks' gives
the options, they build the option records of the machine methods.

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

p = point{p.y, p.x}            the field 'x' is written first, bytes 0..4,
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

1. the code is measured and discarded
2. string constants added meanwhile are removed
3. the frame depth and the peak storage size are restored; the locals of the
   body live in the callee's own frame, so the caller's frame capacity is not
   checked against them
4. a noinline call reached inside a dry run does not emit an instance of its
   function

### 4.3 Signature: checked once per kind of call

A dry run costs a compilation of the body, so it is not repeated for calls
that cannot differ. What the body can reject depends only on which arguments
are the same variable and on which are globals (the body may name the global
too). The call is reduced to a signature:

```text
name + for the result and each argument:
    ' array N'          when an array, the body can reject an index by
                        the length
    ' global g'         a global, by name
    ' local K'          a local, K is the order of first appearance
+ ' temporary' or ' variable' per argument

f(v, v, n)   "f local 0 local 0 local 1 variable variable variable"
f(v, w, n)   "f local 0 local 1 local 2 ..."
f(x, y, z)   same as the line above, checked once
```

Locals are numbered by first appearance, not named, because which local is
passed does not matter, only which arguments are the same one. A global is
named, because the body may refer to it directly.

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

The versions are measured in instructions on x86_64 and in bytes on rv32i.
The shorter is kept; on ties the plain version, since it holds no extra
register.

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
