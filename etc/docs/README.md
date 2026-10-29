# baz: how the compiler works

The design documents of the compiler, in reading order: each chapter
builds on the ones before it.

For a reviewer:

1. the language: 'README.md', then 'etc/tutorial.baz'
2. the code: the reading order in 1.1, the choices in chapter 7
3. build: './make.sh build' (clang++ with c++26), one translation unit
4. test: 'qa/coverage/test-all.sh', see its header and AGENTS.md "Tests"
   (needs nasm, ld, llvm-mc, llvm-objcopy, ld.lld, llvm-profdata, llvm-cov,
   qemu-riscv32 and qemu-system-riscv32)
5. lint and format: 'qa/lint/clang-tidy.sh', 'qa/lint/check-all.sh' (the
   design rules), 'qa/lint/format-source.sh'
6. fuzzing: 'qa/fuzz/fuzz-all.sh', see 'qa/fuzz/README.md'
7. known limitations and planned work: 'etc/todo.txt', the reasons of past
   decisions: 'etc/todo-resolved.txt'

## 1. Architecture: from source to assembly and binary image

### 1.1 Pipeline

```text
main.cpp         options, the backend for '--target', error printing
  |
program.hpp      parse: every file, an included one before its includer,
  |              and the definite assignment of the results
  |              compile: globals, 'main', noinline bodies, handlers,
  |              used routines, read-only data, data and variables
  |              report after the code
  v
nodes            statement.hpp, stmt_*.hpp, expr_*.hpp: a node parses
  |              itself, then compiles through 'toc' (toc.hpp, the names
  |              and the checks) into 'machine'
  v
machine.hpp      one virtual method per operation the front end needs,
  |              e.g. 'add_subtract', 'copy_block', 'shift'; allocates
  |              the registers ('register_pool')
  |              machine_x86_64.hpp, machine_rv32i.hpp with its targets
  |              machine_rv32i_linux.hpp and the bare-metal
  |              machine_rv32i_qemu.hpp and machine_rv32i_fpga.hpp
  v
assembler.hpp    buffers the lines, optimizes the jumps
                 assembler_x86_64.hpp: nasm text
                 assembler_rv32i.hpp: llvm assembler text and the flat
                 binary image of the rv32i targets
```

The build is one translation unit: 'main.cpp' includes the headers.

The headers in layers, a header includes only headers of its own layer and of
the layers with a lower number ('qa/lint/check-layers.py' checks it):

```text
1. vocabulary        token, lut, compile_error, operand, type,
                     constant_parser, constant_arith, source_files, access_path
2. assembler         assembler, assembler_x86_64, assembler_rv32i
3. backend           register_pool, machine, machine_x86_64, machine_rv32i,
                     machine_rv32i_linux, machine_rv32i_bare_metal,
                     machine_rv32i_qemu, machine_rv32i_fpga, register_report
4. names and checks  tokenizer, generics, ident_info, ub_reads, ub_assigned,
                     toc
5. nodes             statement, unary_ops, parse_lookahead,
                     decouple, parse_statement, stmt_identifier, ub_order,
                     bulk_operands, expr_*, stmt_*
6. driver            program, main.cpp
```

The backend, layer 3, reaches the layers with a higher number only through
callbacks: the versions of 'machine::emit_most_efficient' and the code of
'machine::emit_discarded' compile statements, as do the count and address
emitters of the bulk operations; 'store_bytes' asks 'toc' for a read-only
label.

Reading order, each step builds on the ones before it. The code takes more
than a day: 'read' marks what to read through, 'skim' what to read by its
header comments and class maps first.

```text
1. main.cpp, program.hpp       skim: the driver and the passes
2. compile_error.hpp           read: the error every step throws (1.9)
   type.hpp, operand.hpp, ident_info.hpp, access_path.hpp, ub_reads.hpp,
   ub_assigned.hpp
                               skim: the vocabulary of the node signatures,
                               'type', 'operand', 'destination',
                               'ident_info', 'folded_value', 'value_use', and
                               the walks every node takes part in (3.1)
   toc.hpp, its file map       read the map at the top, not the file: what
                               every node asks 'toc' for (the file is step 11)
3. statement.hpp               read: the node interface every statement has
4. tokenizer.hpp, parse_lookahead.hpp
                               skim: the tokens and 'classify_term', which
                               every parse step asks first
5. stmt_block.hpp, parse_statement.hpp
                               read: a block, the statement it parses next
                               and the definitions of the top level
6. stmt_if.hpp, stmt_loop.hpp, stmt_builtin_foo.hpp
                               read: control flow, the simplest nodes; 'foo'
                               loops over an array
7. stmt_def_var.hpp, stmt_assign_var.hpp
                               read: a variable defined and assigned
8. expr_any.hpp, expr_arith.hpp, expr_bool.hpp, expr_type.hpp
                               skim: values, arithmetic, conditions,
                               instances
9. stmt_identifier.hpp, stmt_identifier_impl.hpp
                               skim: a path of names, fields and indexes, or
                               a literal; the index steps of the walk are in
                               the _impl file
   stmt_builtin_*.hpp          skim: the built-ins, conversion, io, array
                               copy, compare and length, exit, which
                               'parse_statement' dispatches
   ub_order.hpp                read: the order checks, why code may compute
                               an operand before one written earlier
10. stmt_def_func.hpp, stmt_call.hpp, generics.hpp, stmt_def_type.hpp
                               skim: functions, inlined and noinline calls,
                               the generic instances (2.11)
11. toc.hpp                    skim: what the names are while compiling
12. machine.hpp, register_pool.hpp, machine_x86_64.hpp, assembler.hpp
                               skim: the operations, the registers (1.8) and
                               one backend; the class map of 'machine' is
                               read
13. machine_rv32i.hpp, assembler_rv32i.hpp
                               skim: the load/store backend and its image
```

A first day: chapters 1, 7, 3 and 4 of this document, then the header
comments and class maps of steps 1 to 5.

The formatter ('qa/lint/format-source.sh') sorts the members of a class by
name within their section banners ('// statics', '// class methods', ...), so
a class does not read top to bottom in the order its steps run; the class and
file maps at the top give that order, e.g. those of 'toc', 'machine' and
'stmt_call'.

Invariants the code relies on:

1. a statement parses in its constructor and emits nothing until it is compiled;
   a node has no default constructor, its members are made in the member list
   (the one exception is the body of a 'stmt_def_func', see 'stmt_block'); a
   compile may run more than once (1.4)
2. the front end never writes assembly, it calls 'machine' operations
   with operands; the backend lowers them to what the target encodes
3. registers are freed in the reverse order of their allocation, an
   operation frees the registers it allocated before it returns
4. a name is resolved again at every compile through the scopes, an
   inlined parameter is bound to what its argument resolved to, never a
   copy
5. a 'destination' is not changed after it is made, a field or element
   of it is a destination of its own
6. labels are named by source location and call path, so a body inlined
   twice or compiled twice gets the same or distinct labels as needed

### 1.2 Passes

1. parse ('program' constructor): every statement parses itself in its
   constructor and emits nothing, 'toc::begin_compile' ends the parse. The
   grammar needs to know the names (7.2), so the parse declares types,
   functions, constants and variables in 'toc' as it goes. Where a type or a
   size is learned from the initializer, the initializer is parsed, the
   tokenizer rewound and parsed again
   ('initializer_declaration::trial_parse'). A generic definition is kept as
   text and parsed again for each instance ('generics.hpp'). A function checks
   when parsed that every path through it sets its result
   ('stmt_def_func::assert_sets_result', 'ub_assigned.hpp').

2. compile ('program::build'): the statements are walked again with a fresh
   scope stack in 'toc'. Global variables and data are declared again, then
   'main' is compiled where the program starts. An inlined call compiles the
   body of the callee in place (1.3). A noinline call adds a body to compile for
   the array lengths of its arguments; the bodies are compiled after 'main', and
   compiling one can add more. Then each function but 'main' is checked once
   without its calls and emits nothing (1.5). Then the failure handlers that the
   code branches to ('machine::emit_used_failure_handlers'), the routines and
   helpers the code and the handlers marked used
   ('machine::emit_used_routines'), the read-only strings, the data and the
   variables.

3. output ('machine::finish', 'write_assembly'): the buffered lines get their
   jumps optimized, the assembly is written, the rv32i targets also write the
   binary image.

### 1.3 Names during compile: scopes and bindings

'toc' keeps a stack of scopes: blocks, loops, 'foo' loops and functions. What
a name means is decided by one rule, 'scope_stack::lookup': the scopes of the
innermost function from the innermost outwards, then the root scope that
holds the globals; the first scope that declares the name decides. A scope
declares a variable or a constant of a name, not both, a second declaration is
rejected. The body of a function is the block above its scope, a name declared
there that repeats a parameter or the result is rejected, a deeper block or a
'foo' may hide it ('definition_checks::assert_not_hiding_signature'). In a
function scope a parameter hides a constant of the generic instance, which is
the context of the definition. Every question about a name asks this lookup:
the resolver ('ident_resolver'), the aliasing check, and the lookaheads of the
parser, where a variable, a parameter or a constant hides a type or a generic
function of its name, e.g. after 'let point = 3' the 'point' of 'point + 1' is
the constant; a built-in type, e.g. 'i8', names no value. A name followed by
'(' is a call, no value is called. The same
statement can resolve to another variable, register or constant each time it
is compiled.

An inlined call does not copy its arguments. It enters a function scope that
maps each parameter name to the argument ('param_binding'), e.g. 'p' to
'world.rooms[i]' with the address of that element computed once, '3' to the
constant 3, or 'x' to a scratch register holding the value of an expression. The
result name maps to the destination of the call. A binding holds what the
argument resolved to in the scopes of the caller: resolving 'p' in the body
gives it, and 'p.x' steps to the field from there. The scopes
below the binding do not change while the body compiles, so this is what a
lookup in the caller would find. The result has the type of the destination,
except that a narrower integer widens into it like a name, e.g. 'exit(f8())' or
the 'g()' of 'w + g()': an inlined result is made in a register of its type and
copied ('inline_call::compile_widened'); a call is an argument by its value, so
it widens into a parameter where a name, bound to the parameter, must have its
type. 'stmt_call::compile' checks the result before the inlined and the noinline
paths split, so both accept the same. A result that the body passes to a 'mut'
parameter is made in a temporary of the call when the destination is a register,
so a 'mut' argument always has storage ('body_uses::is_result_passed_as_mut').

A noinline function has a body of its own and a frame in memory that holds the
addresses of its result and arguments (4.1).

What resolution gives is an 'ident_info': a constant, a register or a variable
with its memory operand, which may be an address an inlined call computed. The
resolver looks up one name; a path resolves its root and steps each field
('ident_info::field', 'stmt_identifier::for_each_prefix'). 'statement::compile'
writes its value to a 'destination': nowhere for a statement, a register, or the
storage a resolved name is at.

### 1.4 Dry runs

Some code is compiled and then dropped: a dry run only to be checked, and
the longer of two versions after both are measured:

1. 'toc::dry_run' and 'machine::emit_discarded': the body of each function
   without its calls (1.5), and code that a compile skips ('toc::check_skipped',
   1.5.1): the code after a statement that leaves on every path, a branch whose
   condition folds to false, the branches and the 'else' after one that always
   runs, the operands of 'and' and 'or' after the one that decides and the body
   of a 'foo' whose count folds to 0; and the noinline bodies that only such
   code asks for (1.5 item 4)
2. 'toc::emit_most_efficient' over 'machine::emit_most_efficient': two versions
   are compiled, each from the state before it, and the one with fewer
   instructions is kept with the state it ended in, e.g. with and without a
   scratch register, see 5.3

What dropped code changed is put back by its owner, also when an error
unwinds it:

1. 'toc::dry_run_scope', for a dry run: the 'toc::compile_state' (the
   deepest scope count, the storage peak and the frame peak of the scope
   that holds the storage, and whether the code after is reached,
   'scope_stack::reach_state') and the string constants added; a noinline call
   reached inside a dry run adds a body to check only, not to emit
   ('toc::add_noinline_body')
2. 'toc::emit_most_efficient', for a version: the same peaks and reach state,
   each version starts from those before it and the kept one's remain, e.g.
   the frame size of a noinline body that '--checks=frame' checks; a string
   constant that the dropped version added stays, unused, so the labels stay
   unique
3. 'machine::version_scope', for a version and for the code of
   'machine::emit_discarded': the members of 'machine::version_state', e.g.
   the registers held and the routines marked used ('machine::mark_used')

So a statement can be compiled more than once, and what its compile adds to
'toc' is the same each time or put back by its owner; the class comment of
'toc' lists each write of a compile and how it keeps the rule, and
'statement::compile' points there.

### 1.5 Checks

'--checks' ('check_options' in 'toc.hpp') turns on run-time checks, which the
machine emits as a compare and a branch to a failure handler; 'line' adds the
source line to the message of a failed bounds, division, shift, overlap or
overflow check. The compile-time rules (narrowing, uninitialized reads,
constants out of range, shared storage) always hold, see 8.1. 'toc::checks'
gives the options, they build the option records of the machine methods.

The rule: whether a program is accepted does not depend on its calls, except
for what needs the arguments of a call. So it can depend on inlining: in
'res = 1 << n' an inlined 'sh(65)' folds the count and is rejected, a noinline
'sh' checks it at run time ('shift'). The run-time result of an accepted
program does not depend on inlining.

1. Every function is checked once without its calls, also one that nothing
   calls: after the noinline bodies, 'program::check_function_bodies' compiles
   each function but 'main' like a noinline body under 'toc::dry_run'
   ('stmt_def_func::check_body'). So a rule that needs no argument holds also in
   code that the constant arguments of a call fold away (1.5.1), and adding a
   call cannot make a rejected function accepted. Its errors have no "called
   from" frame. An instance of a generic definition is a function like the
   others: a use of a generic function makes it, an alias of a generic type
   makes its methods, and an instance that nothing calls is checked too; a
   generic function that nothing instantiates is listed in the report (1.6).
2. What a body lacks without a call is the length of an unsized array
   parameter ('ident_info::is_length_unknown') and the arguments:
   - the rules that need the length are left to the calls: 's[7]' and
     'array_copy(s, d, 2)' are accepted, 's[-1]' is not; the 'n' of a 'foo' over
     such a parameter is a read-only variable there and in the parse, not a
     constant (8.3 for a 'let' of it)
   - an error that depends on the arguments of a call
     ('compile_error::depends_on_call'): running out of scratch registers,
     e.g. twelve parameters that one call gives the same argument, or of the
     variable storage ('out_of_storage'), and the limits of inlined calls,
     e.g. 'r(x - 1)' in 'r(x)' that a constant argument ends; such an error
     ends the check and the function is accepted, in skipped code it is
     dropped and the check goes on ('toc::check_skipped'); a recursive
     inlined call is not expanded in the check
     ('toc::is_checking_without_call'), its value arguments are compiled at
     the types of their parameters
     ('stmt_call::check_arguments_alone'), so a recursion that only a
     constant argument ends does not end the check
3. A function that takes nothing from a call, a noinline one without unsized
   array parameters or an inlined one without parameters and result, is
   rejected by such an error. The check has the registers of a noinline body
   (1.8), as many as any call of a noinline function and one fewer than
   'main', which does not hold the frame base: an inlined function that
   needs every register of 'main' is rejected also where its only call, from
   'main', compiles (e.g. 14 nested 'a + (a * ...' levels on x86_64). An
   inlined result counts as taken from the call: it is the destination of
   the call, which the check reaches through an address, so it can need more
   registers.
4. A noinline call in skipped code or in the check of a function without
   its calls asks for a body that is checked with the lengths of its array
   arguments and not emitted, as an inlined call there compiles its body
   ('program::check_noinline_bodies'). So 'f(a)' in 'if 1 == 2 { ... }' is
   rejected for 's[5]' in 'f(s i8[])' on an 'i8[4]' inlined or noinline. Its
   errors have the "called from" frame of the first such call. A call with
   an array argument of unknown length asks for none, also for its other
   array arguments: 'f(ps, b)' in a function 'g(ps i8[])' that nothing
   calls, with 'b' an 'i8[4]', does not check 't[5]' of 'f(s i8[], t i8[])'
   with length 4, which an inlined 'f' rejects; the verdict of such code
   depends on inline versus noinline, a call of 'g' with a known length
   checks it.

### 1.5.1 Code that is not reached

A statement after a path that cannot continue is rejected at the parse: after
'return', 'break', 'continue', 'exit', an 'if' that leaves on every path or a
'loop' without 'break' ('stmt_block::trace_assignment', on the conditions known
where the code is written). The compile knows more, the conditions that constant
arguments of an inlined call decide: a branch whose condition folds to false is
not compiled, nor is the code after a statement that leaves on every path, e.g.
after the 'return' of 'if n > 5 return' in an inlined 'count(6)', which ends the
expansion of a recursion with constant arguments, or after a 'loop' whose
'break' folds away. The compile says it in the words of the parse walk
('assignment_flow'): the function scope keeps whether the current statement is
reached ('toc::is_reachable'), 'toc::end_path' and 'toc::record_break' end a
path, 'toc::path_merge' joins the branches of an 'if' with the same chain rule
as the parse, 'toc::join_loop' ends a loop by its 'loop_end'
('scope::is_left_by_break'; a 'foo' whose count folds above 0 runs out only
where its body or a 'continue' reaches the counter step,
'loop_end::when_stepped'), an inlined call whose body reaches neither its end
nor a 'return' leaves the code after the call unreached ('toc::pop_scope'), and
the end of 'main' is emitted when the compile reaches it.

The compile decides, the parse is only the first validation, so every rule of
the compile holds in all code. Code that a compile skips is compiled for its
checks in a dry run ('toc::check_skipped') where what it folds is no constant of
a call: in 'main', in the initializer of a global and in the check of a body
without its calls. That is a branch whose condition folds to false, the branches
and the 'else' after one that always runs, the operands of 'and' and 'or' after
the one that decides the list, the body of a 'foo' whose count folds to 0, and
the code after a statement that leaves on every path, e.g. 'if 1 == 2 { n = a }'
with 'a' an array is rejected. Such code is judged as if it ran, with the
registers held where it stands; a 'break' or 'return' in it does not end the
paths of the code that runs ('scope_stack::reach_state'). The parse walk does
the same for the branches of an 'if' that it knows never run
('assignment_flow::path_merge::trace_skipped'): they are walked as if they ran
from the 'if', so in them too a read of an unset result, a 'return' before it is
set and a statement after a path that cannot continue are rejected. The
variables of skipped code take no storage, so '--vars' does not hold there
(8.3). In a compile with the constants of a call skipped code is not checked, a
constant argument decides it: 'if b != 0 { res = a / b }' in 'ratio(10, 0)'
guards the division, and the check of the body without its calls, where the
guard does not fold, holds the rules in it.

### 1.6 Reports

After the code, as comments: noinline bodies, uninstantiated generics, jump
optimizations, usage ('report_renderer' in 'program.hpp'), and with
'--report=registers' the use of registers at the busiest point
('register_report.hpp').

The usage entries:

```text
max scratch registers in use   the most scratch registers held at once
max scopes in use              the deepest nesting of scopes: the globals,
                               functions, blocks, loops and inlined calls
dat size                       the bytes of the 'dat' definitions
dat var padding                the padding between the dats and the variables
max body vars size             the most bytes of variables in use while one
                               body is compiled: the globals with 'main' or
                               with one noinline body, inlined calls included;
                               the frames of noinline calls stack at run time,
                               '--checks=frame' checks that they fit
instructions                   the machine instructions of the code
```

### 1.6.1 Reading a listing

Each construct emits a comment with its source and location, then its code
under labels named by where it is written and by the inlined calls around
it ('toc::call_path_at'), so a label is unique per call path:

```text
if.L.C         an 'if' at line L, column C; '.N' its branch N, '.code',
               '.else', '.end'
cmp.L.C        a condition
bool.L.C.end   the end of a condition stored as a value
loop.L.C       a 'loop', '.end'
foo.L.C        a 'foo', '.continue', '.end'
init.L.C       the read-only bytes of a constant '{...}', 'str.L.C' of a string
func.NAME..L.C a body inlined by the call at L:C, '.end'
func.NAME      a noinline body, 'func.NAME..len.4' for array lengths,
               'size.func.NAME' its frame size; 'f<i8>' is 'f..i8'
baz_NAME       a failure handler, and a routine of the x86_64 runtime,
               e.g. 'baz_bounds_panic'
.Lbaz_NAME     a routine of the rv32i runtime; numbered, a jump target
               inside one operation, e.g. '.Lbaz_equal.0' on x86_64 and
               '.Lbaz_jump.0' on rv32i
```

E.g. 'if.7.5.36.13' is the 'if' at 7:5 in a body inlined by the call at
36:13. Inside a noinline body for array lengths every label ends in the
lengths, e.g. 'loop.14.5.len.3'. A location in an included file is 'fN.L.C',
N the index of the file. A path follows a name after '..', as type arguments
do; a plain and a generic function or method cannot share a name, so the
function 'h' for array lengths ('func.h..len.4') and the method 'h.len'
inlined at 4:1 ('func.h.len..4.1') have labels of their own.

The comment lines say what was compiled and how:

```text
bind x -> 3            a parameter of an inlined body names its argument
src: operand           an operand read as it is stored; 'src: constant', 'src:
                       folded constant' a value known at compile time,
                       'src: expression' one computed first
copy field 'x'         a part of an instance literal
zero padding: 4 B      the bytes no field writes
allocate scratch register -> r15, free scratch register r15
                       the registers a step holds
kept the second of two versions of 6 and 4 instructions, the smaller
                       the version kept of code compiled two ways (5)
```

The variables the compiler adds are named by the byte offset of the call in
its file, not by its line: 'call-arg-OFFSET-I' holds argument I,
'call-result-OFFSET-0' the result. The '-' keeps them apart from the names of
the source.

### 1.7 Values known at compile time

A constant follows the rules of a variable; only the code differs, an
immediate instead of a load. One virtual, 'statement::fold', evaluates a value
known at compile time, also an array size ('stmt_array_size' folds at 'i64',
e.g. 'i8[n * 2]'). The one exception is read at the parse from a literal or a
constant name: the constant arguments of a generic ('toc::constant_value_of').
The result of 'fold', 'folded_value', is stored as it is wherever a constant is
kept ('ident_info', the alias of an inlined argument, the constant of a 'let'):

- typed: it has the rules of a variable of its 'own_type': a conversion
  ('i8(300)', 'i16(k)'), a constant argument of an inlined call,
  'array_length', a list led by one, a 'let' of a type other than the
  default type, e.g. 'let k = i8(3)' or 'let b = true'
- untyped: a literal or a 'let' of untyped values, checked by its value where
  it is used, e.g. 'let k = 5' fits an 'i8', 'let k = 300' does not

The default type ('int') is the width of a register, 'i64' on x86_64 and 'i32'
on rv32i (7.12). A variable or a parameter without a type has it, and so has
a list led by a literal, since the first element decides the type of a list:
'1 << k' with 'k' an 'i32' is computed at the default type. Their arithmetic
and how they mix with a fixed width depend on the target (chapter 8); a
portable program writes the type, e.g. 'lim i32' or 'i32(1) << k'.

Where it is used is a 'value_use': the width the value is computed at and
whether its arithmetic is checked for overflow, made by 'toc::use_of' from a
destination or by 'toc::use_at' for a value without one (a comparison side,
an initializer, an argument). A destination carries the overflow policy
('destination::as_wrapping' for the argument of a conversion). The value is
wrapped to the width of its use; the unary ops apply by
'unary_ops::evaluate_at', which rejects a negation of the minimum under the
overflow check.

A conversion 'T(arg)' ('stmt_builtin_convert') behaves like a temporary of
type T: its argument is a list computed at W, the wider of T and the type of
the argument's first element (on a tie the argument's type, so the 'b' of
'i8(b)' stays 'bool'), where a wider element is rejected as narrowed and the
arithmetic wraps. A destination carries whether the arithmetic of its value is
checked for overflow ('destination::as_wrapping', 'toc::overflow_options'):
the argument is compiled into a wrapping one, which its scratch registers
keep; a call in it builds destinations of its own, so its arguments and body
are checked as anywhere else, while the call's own unary ops follow the
argument. The conversion's own '-' and '~' apply where
the value is used, at the width of the destination or of the list it is an
operand of, as for '-v' of a variable of type T. The argument is compiled as
an operand, so a narrower call result widens into it.

'fold' does not check that the value fits; a conversion folds its argument
like a list at its width W, where it wraps, checks it there unless it is one
untyped constant, which converts whole (e.g. 'i32(0xfedc1234)' on rv32i), and
narrows the result to its type; 'statement::fold_checked' adds the check of
'assert_not_narrowed' at the same width, so a folded comparison side or
initializer is rejected where a computed one is. A constant reaches a
destination only through the list that holds it ('expr_arith', the
immediate by 'folded_value::operand_at'); the nodes in a list never emit one.
A comparison side has its own rule ('expr_bool_op::right_operand',
'immediate_of'): the backend compares at the width of the left side, so an
untyped constant on the right is checked to fit it ('assert_sides_fit')
and a typed one keeps its type.

The result: a typed constant and a variable of the same type and value get the
same verdict and the same result; an overflow or zero divisor certain at
compile time is an error where the variable gives the panic. The spelling mode
of 'qa/fuzz/fuzz-exprs.py' checks it.

### 1.8 Registers

The front end holds registers, the backend names and picks them
('register_pool.hpp', 'machine'):

1. A register has one of three holders: a scratch register, picked by
   'machine::alloc_scratch_register' in the order of the target
   ('target_description::scratch_registers'); a named register, fixed by an
   instruction or the calling convention ('machine::alloc_named_register'),
   e.g. 'rcx' of a shift count on x86_64 or the argument registers of a
   system call; and the backend's own, the variables base and the frame base
   of a noinline body, which no source location asked for.
2. Registers are freed in the reverse order of their allocation. A
   front-end step holds the registers of its operands in
   'machine::held_registers', which frees them when the step ends; a
   backend operation holds its own in 'machine::operation_scope', which also
   keeps the registers of its operands from being picked while it is
   lowered.
3. No free scratch register is the error kind 'out_of_registers'
   ('machine::out_of_registers_error'), and so is a named register that a
   scratch value holds, since which register a scratch value takes depends on
   how many are live. A named register that another operation fixed is an error
   of the program that advises to compute an operand with 'let'. Who reads the
   kind: 1.9.
4. An inlined body compiles with the registers of its caller held, so a deep
   chain of inlined calls can run out where the same body as noinline does
   not. The check of a body without its calls (1.5) starts from the registers
   a noinline body starts with ('machine::noinline_body_pool': the backend's
   own and the frame base), so its verdict does not depend on the code around
   it; skipped code keeps the registers held where it stands (1.5.1). A
   noinline body has one register fewer than 'main', which holds no frame base
   (1.5 item 3).

### 1.9 Errors

An error is a 'compile_error' ('compile_error.hpp') at the token that caused it,
or where the next token starts when a token is missing. The first error ends the
compile (7.8), 'main.cpp' prints it with its frames.

1. Frames: the code that led to an error adds itself while the error unwinds
   through it: an inlined call adds "called from" with its source, the
   instance of a generic function or type adds the instances that led to it
   ('generic_trace'), and the error of a noinline body gets the call that
   needed the body (4.1).
2. Kinds ('compile_error::error_kind'): 'source', an error of the program, and
   the kinds that depend on the code compiled or on the arguments of a call:
   'out_of_registers', 'out_of_storage' and 'inlining_limit'. Four places read
   the kind and change the error instead of ending with it: a version that runs
   out is dropped when the other one is available (5.3), an inlined recursion
   that runs out of registers is reported as the recursion, the check of a
   function without its calls ends at such an error (1.5), and skipped code in
   a body that takes from a call drops it ('toc::check_skipped'); the comment of
   'error_kind' names the functions. An error that ends a dry run is a 'source'
   one ('toc::throw_dry_run_error'), so none of them drops it.
3. A file that cannot be read is an 'unlocated_error', which
   'program::load_file' locates at its 'include'.

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

The generic type must already be defined, and the count of arguments must match.
Each argument is checked against its parameter: a type parameter gets a type, a
constant parameter gets a constant. The result is a list of bindings ('capacity'
= 8), from which the instance is built (2.6), 'str' is registered as a real
type, and the methods are made (2.7).

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

1. When an alias is made, the methods are made for every level in the order
   they are defined, as for a type written by hand: a method of 'text' can
   call one of 'list' defined before it, and one of 'list' can call one of
   'text' defined before it.
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
2. A method of a generic type may come before or after the aliases (2.7, item
   2).
3. The fields and the methods of a generic type see the names defined
   before them, like a plain definition, also when an alias after them
   instantiates them.

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
type ('str.append', 'name.append'). An error in a call or in the body names
the instance, e.g. "cannot deduce 'T' of generic function 'str.append'"
(0780); a clash of two definitions and the report of a generic never
instantiated name it after the generic type, 'text.append', the name written
in the source.

#### 2.10.2 Call: the type arguments

A call gives the type arguments in one of two ways:

```text
make<str>(3)       written: T = str
same(s, t)         deduced from the argument 's': T = the type of 's'
x = tz.to()        deduced from the destination: T = the type of 'x'
```

Written, after '<', the count must match the type parameters and each
argument names a type; a constant is rejected. A type argument is a type
name, not an array: a parameter 'a T[]' takes an array of 'T', so
'f<i32>(buf)' passes the 'i32[]' 'buf'.

Deduced, without '<', each type parameter is looked for in two places, in this
order. The examples use 'make' and 'same' from the start of 2.10 and a type
'name'.

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
      a call of a function that is not generic and returns a value, or a
      built-in with a value

      ```text
      same(name{}, t)         T = name
      same(point.at(1, 2), p) T = point
      same(mk(), t)           T = the return type of 'mk'
      same(i8(n), t)          T = i8
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
      ```

   A missing argument is rejected: 'same()' gives "expected argument 1
   ('a')".

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

A generic function or method called without '(' and without '<' is rejected
with "needs type arguments", since there is nothing to deduce from.

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

An error in the body of the instance is reported where the bad code is, then the
call that needed the instance, for the same reason as in 2.8:

```text
example.baz:9:9: field 'size' not found in type 'name'
    res.size = n
        ^~~~
example.baz:13:13: instantiated by 'make<name>'
    var m = make<name>(2)
            ^~~~
```

#### 2.10.4 Order rules

1. A generic function is defined before a call names it.
2. The type arguments are fixed before the arguments are parsed, so an
   argument is parsed with the type its parameter has in the instance. This
   is why deduction looks ahead instead of parsing the argument first.
3. The body sees the names defined before the generic function, like a
   plain body, also when a call after them instantiates it. Methods,
   constructors and instances of generic functions are reached through
   their type and their generic, so the body sees those defined before the
   call that instantiates it, e.g. the methods of a type argument.

### 2.11 Where in the code

```text
generic_registry (generics.hpp)       the generic definitions and their
                                      instances, kept as text
type_instantiator (stmt_def_type.hpp) a generic type instance from an alias
func_instantiator::define_func        a generic function or method kept as
  (stmt_def_func.hpp)                 text
generic_call::parse (stmt_call.hpp)   the type arguments of a call, written
                                      or deduced from the arguments
func_instantiator::instantiate_func,  the instance of a generic function, the
  instantiate_methods                 methods of a type instance
  (stmt_def_func.hpp)
generic_reparse, generic_scope        the text parsed again with each
  (toc.hpp)                           parameter naming its argument
generic_trace (generics.hpp)          the instances that led to an error
```

## 3. Reads and writes: the no-UB checks

### 3.1 Reads, writes and their order

Evaluation is left to right: the operands of a list, the arguments of a
call, the parts of a statement and the parts a literal writes. Code
generation follows the same order, so the checks walk a statement in that
order and check each read only against what was written before it; the code
that is checked is the code that is emitted. A name in an argument, a side of
a comparison or a bulk or io argument is read where it is used, after the
later operands, so a call in a later operand that may write it is rejected
('assert_not_written_later'); an arithmetic list reads each operand at its
place.

Each node states its operands in evaluation order in its 'visit_operands', and
the walks of the checks follow from it; its 'compile' emits them in that order,
held by review and the tests, e.g. 1678, also for the built-ins that check a
range after its count (7.14). The
default of 'visit_operands' is 'std::unreachable()': a node that is asked and
does not say fails in the tests instead of skipping the checks. An operand is a
'list_operand' ('ub_reads.hpp'): a node it reads, or a place of which only the
indexes are read, e.g. the buffer that 'read' fills. The walks and their
defaults are listed in the class comment of 'statement', the order checks with
the order each relies on in the header of 'ub_order.hpp'. The aliasing of the
arguments of a call is not an order, it is 3.2 to 3.7 and the call rule of 4.2
('stmt_call::assert_no_shared_storage').

### 3.2 Aliasing: the problem

Two accesses to the same root variable alias when they can reach a common
byte. With a run-time index the byte is not known, so the check must answer
"can these ever be the same byte for some value of the indexes".

The check answers false only when the accesses are provably disjoint.
Anything unknown is "may alias": a missed alias is undefined behavior in the
compiled program, a false alarm only asks for the code to be written
differently.

### 3.3 The path

An access is described by the path from its root variable: one step per
field, constant index or run-time index. The root variable itself is not a
step. A constant index is a range of indexes, one long for 'a[1]', longer
for the elements a literal writes at once (3.7) and from its start for the
start of a bulk operand (3.6).

```text
step         holds
field        offset of the field in its parent
index range  the first index and the end
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

### 3.4 Why paths and not bytes

A run-time index turns a single byte range into a set of ranges, one per
element, spaced by the element size. Comparing two such sets as bytes means
lining up periods, slots and alignment, and a constant index after a run-time
index makes the set harder still to describe. A path keeps the structure that
the bytes lose: two accesses that take different fields at the same position
can never meet, whatever the indexes are. The check never compares bytes, so
it is exact for every path the language can write.

### 3.5 Comparing two paths

The steps line up, so they are walked together:

```text
run-time index on either side             => go on, may be the same
field, different offset in the parent     => disjoint
constant index ranges, no index in both   => disjoint
same field (same offset) or ranges meet   => go on
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

### 3.6 Building paths

An identifier adds a step per element of what it names: '.name' adds a field,
'[3]' a constant index, '[i]' a run-time index. The start element of a bulk
operand, e.g. 'g[1]' of 'array_copy(s, g[1], 3)', adds the range from it to the
end of the array.

A parameter or the result of an inlined call is a root variable of its own:
storage it shares with a name of the caller is rejected at the call (4.2), so
in an accepted call only a body's own names overlap. The element 'e' of a
'foo' stands for the array it iterates with a run-time index, since it is any
element of the array.

The whole variable has the empty path, e.g. a temporary of a call, which is a
root variable of its own.

### 3.7 A value written in parts

A value is written part by part, in the order 'compile' writes it ('plan_writes'
makes a 'write_plan'): a field list field by field, a braced array element by
element (constant elements, a string and the zeroed rest at once), a call or a
copy at once. Each part has its path from the destination.
Each read of a part is resolved to its root variable ('toc::storage_of'), so
the element 'e' of a 'foo' and the array it iterates reach the same variable:

```text
p = {p.y, p.x}                 'x' is written first, then 'p.x' reads it =>
                               "'p.x' is read but has been overwritten in the
                               same statement"; with a run-time index on either
                               side "may have been overwritten"

e = {1, ps[0].x}               the same in a 'foo' over 'ps', 'e' may be
                               'ps[0]': "may have been overwritten"

e = ps[1] * 2 + ps[1]          'e' may be 'ps[1]': the compile would write
                               'e' before it reads 'ps[1]' again, so the value
                               goes through a scratch register
```

The compile decides that in
'expr_arith::destination_is_clobbered_before_read', with the reads resolved
like above.

Two names that the body makes itself, e.g. the element 'e' of a 'foo' and the
array it iterates, are protected by the compile as above; storage that only a
call shares between two names is rejected at the call (4.2).

### 3.8 Fuzzing the aliasing check

The aliasing check is tested against a brute force of the bytes, in which
every value of every run-time index is tried, on random paths and on random
programs: 'qa/fuzz/fuzz-alias.sh', see 'qa/fuzz/README.md'.

### 3.9 What a call does

The effect of a call is a summary that the parse makes of its body ('body_uses',
4.2): the globals the body reads and writes, also through its calls, and what
it reads and writes of each argument through its parameter, e.g. only 'q.y'
of 'sety(q)' ('stmt_call::visit_callee_reach'). The calls in the arguments
come first. The parse records the same summary for the caller. A function
that calls itself has no complete summary yet: there the argument of a 'mut'
parameter is written whole and of another parameter read whole, so no fixed
point over the bodies is needed (8.3).

### 3.10 Definite assignment

'ub_assigned.hpp' tells which bytes of a variable are set before they are read.
'assignment_flow' walks the result of a function through its body, which every
path must set before it returns ('stmt_def_func::assert_sets_result'). The
parse tells which reads and writes name the result, by the one lookup of 1.3
('stmt_identifier::names_result'), so a name that hides it is not the result.
Each statement reports what it does ('assign', 'end_path', 'record_break'), the
class joins the paths ('trace_loop' with its 'loop_end', 'path_merge' for an
'if'), and 'assigned_bytes' keeps the byte ranges set on every path. A read
outside them is "use of uninitialized variable". The same walk finds the
statements that are not reached (1.5.1); a function without a result has no
bytes to assign. A variable is set
where it is declared, so the one local that can be read unset is the variable
its own initializer names, e.g. 'var x = x + 1' ('stmt_def_var'). The compile
tells which code is reached with the same join rules ('toc::path_merge').

Unlike the aliasing check, these checks need bytes, because they ask whether
a read is inside what has been assigned:

```text
type point {x, y}

var x = x + 1                  the variable is not set while its value is
                               computed: its bytes are read, none are
                               assigned => "use of uninitialized variable"

func point.at(x) self {        a constructor must assign every field: it
    self.x = x                 assigns the bytes of 'x', not those of 'y'
}                              => "function may return without setting
                               its result 'self'"

func make() res point {        the same for the result of a function
    res.x = 1
}
```

They share one walk over the reads with the aliasing check, which hands out
both the bytes and the path of each access, so each check takes what it
needs.

The root variable is the whole variable, and each field and each constant
index narrows it. After a run-time index the element is unknown, so the bytes
stay the whole array: a write through 'arr[i]' assigns nothing, and a read
through it needs the whole array to be assigned. Both are the safe answer when
the element is not known.

## 4. Noinline calls: the frame and the shared storage

### 4.1 The call and its frame

A noinline function has one body per set of array lengths of its array
arguments, emitted after 'main' ('toc::add_noinline_body'), and its callers
pass addresses, not values ('noinline_call' in 'stmt_call.hpp'):

```text
frame base +------------------------+
           | caller's storage       |
frame      +------------------------+ <- 'toc::next_frame_address',
address    | result address         |    the callee's frame base
           | argument address       |    one slot per parameter
           +------------------------+
           | callee's locals        |
           +------------------------+
```

1. The slots: the address of the result when there is one, then one per
   parameter ('stmt_def_func::frame_slots'); the receiver of a method is
   passed in the slot register instead, x86_64 'r11' and rv32i 's11'. A
   result or an argument that is not a variable in memory goes through a
   temporary of the call, the result is copied out after the call. The
   arguments are compiled in their order: a temporary is computed at its
   argument, so is the address of a variable, held in a register until the
   slots are written when it has a run-time index, e.g. the 'w()' of
   'f(v[w()], g + 1)' runs before 'g + 1' (1729).
2. The frame starts after the storage of the caller, aligned to an address,
   not at the stack pointer: in a noinline body after its frame, which its
   frame base holds (x86_64 'rbx', rv32i 's1'), in 'main' at the variables
   base (x86_64 'rbp', rv32i 's0'). The frame size of a body is the peak of
   its storage, a constant named after the body ('frame_size_label').
3. 'machine::call_function' saves the registers allocated at the call on the
   stack, all but the variables base, since the callee may use every other
   register, sets the frame base to the frame address, calls and restores
   them.
4. With '--checks=frame' the call checks that the callee frame fits in the
   variables ('machine::check_frame_capacity', '--vars'), with
   '--checks=stack' on the bare-metal targets that the call fits in the
   stack ('machine::check_stack_capacity'), both in
   'noinline_call::populate_frame' before the slots are written.

The body is compiled for the first call that needs it. An error in the body
carries that call as a frame ('noinline_body::call_tk'), so it points both at
the call and at the line in the body. Every body is also checked without its
calls (1.5).

### 4.2 Shared storage

A noinline body is compiled once and receives addresses, so it cannot know
which arguments are the same variable; an inlined body treats a parameter as a
variable of its own too (3.6). Whether a body is unsafe depends on
the call, so the error ("may share storage") is reported at the call, not once
for the body.

The call is judged by what it passes and by what the parse recorded of the
body, without compiling the body for it ('stmt_call::assert_no_shared_storage'),
inlined and noinline alike, so the verdict does not depend on inlining:

The parse records what each body reaches that its callers name too
('body_uses'): the globals, its parameters and its result, each with its path
and whether it is written, also through its calls. Of the result destination,
the arguments and the globals the callee uses, two may not reach the same
bytes where one of them writes them, e.g. 'f(a, a)' where 'f' writes 'p.x' and
reads 'q.x', 'f(g)' where 'f' writes its parameter and reads 'g', or 'g = f()'
where 'f' reads 'g'; 'f(a, a)' where 'f' writes 'p.x' and reads 'q.y' is
accepted. A call of the function it is in reaches its arguments and its result
whole, its record is not complete yet (3.9).

A value without storage (a constant, '{1, 2}') shares storage with nothing: a
noinline call passes it in a temporary of the call, an inlined call folds a
constant.

## 5. Versions of the same code: compiled, measured, the shorter kept

### 5.1 Why

Some statements can be compiled in more than one way, and which way is
shorter depends on code that has not been compiled yet:

1. a memory destination: computing in place costs a load and a store per
   operation on rv32i, a scratch register costs one extra store
2. an indexed argument on rv32i (no base + index addressing): used as it is,
   every access adds base and index again; folded into a register, it costs
   one 'add' but holds a register

The answer depends on the operations of an expression and on how many times an
inlined body accesses an argument. Predicting this means a cost model
that duplicates the code generator and must stay in step with every backend
change. Compiling each version and measuring is exact and needs no model.

Other choices were measured and gave up at most 0.1% of the rv32i code, so
they are fixed (decided 2026-10-26): a narrow scratch register, a conversion
computed in its destination, the result of an inlined call and its unary
operators in the destination, an address register that holds the
displacement.

The versions are measured in machine instructions, a pseudo instruction
counts as the ones it expands to. The shorter is kept, on ties the first; a
caller passes first the version without an extra register where one of them
has it ('toc::emit_most_efficient').

### 5.2 Why so many

A version compiles everything under it, including the choices inside it, so a
choice in an inlined body is compiled once per version of every choice above
it. The work is the product of the version counts along a path, not the sum.

It stays polynomial in the depth when the chain passes the same indexed
argument on: a version that folds an address into a register gives the body
an argument that is no longer indexed, so only the version that keeps the
index passes the choice down.

Choices made for a chain of inlined calls, each passing an indexed argument
on:

```text
depth of the chain    0    1    2    3    4    5    6    7    8
x86_64 choices        1    2    3    4    5    6    7    8    9
rv32i choices         3    8   15   24   35   48   63   80   99
```

x86_64 has base + index addressing, so only the assignment chooses, once per
body. rv32i grows as (n + 1) * (n + 3). The trade is compile time for code
size: the depth 8 chain compiles in about 13 ms on rv32i against 8 ms on
x86_64.

It grows exponentially when each level passes a new indexed argument, e.g.
'r(ps, ps[i + n], i, n - 1)' in the body of 'r': every version of a level has
an indexed argument to pass down, so on rv32i each level doubles the work. Such
a recursion 11 levels deep compiles in about 0.4 s on rv32i against 5 ms on
x86_64; a 'noinline' function compiles its body once.

Not every choice is measured. An expression stored to memory that calls an
inlined function is compiled in place ('statement::has_inlined_call'):
measuring it would compile the body of the call once per version, so a chain
of such calls, e.g. 'var t = f(x) * 2 + x' in each body, would double the work
per level. The padding of an instance is zeroed gap by gap
or as the whole instance by a gap count that each backend measured once
('machine::zero_padding'): compiling both would multiply the versions above
it, and an instance with thousands of gaps would cost compile time.

### 5.3 State: each version starts from the same machine

A version changes more than the lines it emits. 'machine::version_scope' and
'toc::emit_most_efficient' save what it changes and put it back when the version
ends, also when an error unwinds it; 1.4 lists it by owner. The registers held
carry what a passed lower bounds check proved, and the routines marked used
('machine::mark_used') are a set of values like the registers, so those of a
dropped version or of discarded code are dropped.

Each version starts from the saved state; after the choice the state the kept
version ended in remains. So the footer and '--report=registers' describe the
code that is in the output, and a routine only a dropped version calls is not
emitted.

A version that runs out of scratch registers or of variable storage, or of
inlined call levels, is not available and the other one is kept, e.g. an
inlined call with fifteen indexed arguments on rv32i where folding each address
into a register would need more registers than there are, the temporary of a
scratch version past '--vars', or an inlined recursion that runs out of
registers in one version only. The error carries a kind other than 'source'
('compile_error::error_kind'), which is set where the error is made; when both
versions run out, the error of the first is reported. Any other error is an
error of the program and ends the compile. An error of discarded code is a
'source' error ('toc::throw_dry_run_error'), so a version never drops it; the
check of a body starts from the registers of a noinline body, which do not
depend on the code around it, and skipped code from the registers held where
it stands, as if it ran ('machine::discarded_code').

What a failed version entered must be left when its error unwinds:

1. 'assembler::capture' drops the lines it captured
2. the 'toc::scope_entry' of each scope an inlined body entered pops it and
   its storage, back to the scopes of the caller
3. the rv32i state of an array copy or comparison is a local of the call
   ('machine_rv32i::bulk_operation'), which the unwinding drops

The registers that 'machine::operation_scope' holds and the frames of the
register trace that 'machine::register_trace_scope' holds are left by their
own scopes.

### 5.4 Testing the version that is dropped

Only the kept version runs, so a miscompile in the other one would stay hidden:
'--versions=first' and '--versions=last' keep that version instead of the
smaller, and 'qa/coverage/test-versions.sh' runs the cases with each.

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
           ('path_address::fold_indexed_address')
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
    sal r14, 4                        16 is not encodable either
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
    sal r14, 4
    mov qword [r15 + r14 + 8], 7
```

A run-time index is checked in its register before it is scaled. With the
count of a bulk operation, e.g. 'array_copy(a[i], b, n)', the check covers
'i + n' after the count ('lea_request::start_register', 7.14).

### 6.5 Addresses an inlined call computed

An argument with a run-time index is computed once at the call, the body of the
inlined function reaches it through its parameter. The binding of the parameter
keeps the operand ('ident_info::at_address') and, for the comment, the argument
as written ('param_binding::text'):

```text
w.rooms[i].items[j].set(8)            func mut item.set(v) { self.x = v }

    ...                               r15 and r14 as in 6.2
    bind self -> w.rooms[i].items[j] (address: r15 + r14)
    mov qword [r15 + r14 + 8], 8      'self.x'
```

A path in the body starts at that address, the operand its root resolves to, and
'compile_lea' adds only the rest, the offset of 'x' for 'self.x'. The registers
stay allocated until the body is compiled. On rv32i the body may instead use the
address folded into one register, whichever is shorter (5.1).

### 6.6 The bases

A 'dat' is placed by the parse: 'toc::add_dat' gives it the next offset in
'data_table' and declares its name, so the dats are all placed when the
parse ends and the variables start at a fixed point after them
('storage_layout::vars_start_bytes'); this is why a 'dat' comes before any
'var'. The compile declares it again like a global ('toc::add_var'), and
'program::emit_data_section' writes its bytes ('stmt_def_dat::compile_data').

x86_64 addresses variables from 'rbp', which points at the data: the variables
follow the dats after the padding that aligns them ('dat var padding' in the
report), so 'rbp' reaches both, and 'baz.ld' asserts the order. rv32i
addresses them from 's0', which points 2032 B past the start of 'vars', so the
12-bit offsets, -2048 to 2047, reach the first 4080 B of variables directly
('variables_base_past_vars_bytes'), further ones need a 'lui'. A noinline body
addresses its locals from its frame base, 'rbx' or 's1'.

### 6.7 Registers

The scratch registers of a path ('stmt_identifier::path_address') are the
caller's until the path allocates its own. A fold may overwrite only
registers that are the path's own ('path_address::fold_register'):

```text
1. the register the caller loads the final address into anyway
2. a base the path loaded itself, an address held in a slot or an earlier
   fold
3. the index register being folded, the next index then gets a new one
4. otherwise a new register
```

The caller frees all of them after its access, in reverse order of
allocation.

### 6.8 Where in the code

```text
stmt_identifier::compile_lea          the walk of 6.2
path_address::indexed_element         a constant or a run-time index (_impl)
path_address::add_index               the index register and the scale
                                      (_impl)
path_address::fold_indexed_address    the second index
stmt_identifier::compile_checked_index
                                      the bounds check and the '+ c' of 6.4
                                      (_impl)
stmt_identifier::get_lea_operand      an operand without a path to walk is
                                      the storage of the name
machine_x86_64::with_lowered_addresses, machine_rv32i::lower_address
                                      what the target encodes
```

## 7. Design decisions

The choices a reader may question first: for each, why it was made, what was
rejected and what it costs. A cost with planned work names its item in
'etc/todo.txt'; the other costs are accepted.

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
other, also not when they are 'noinline' ('etc/todo.txt' section 4). A
generic instance parsed inside a body could call that body's function back,
e.g. 'gen<W>' calling 'W.run' that instantiates it, before its summary (3.9)
is complete: such a call is rejected.

### 7.3 One base class for every node

'statement' has 12 virtual methods, each node overrides the ones it needs
(the groups are listed above the class). A default that no other node reaches
is 'std::unreachable()', the others answer for a node that lacks the
capability, e.g. 'fold' is empty. Every node parses, writes its source back,
compiles and takes part in the definite assignment walk. Code asks a node a few
capability queries ('operand_path', 'fold', ...); none names a kind.

Rejected: the nodes as a 'std::variant', a closed set gains nothing when
nothing switches on the kind, and every child would be boxed and every node
kind would change the visitors; one tree per concern, each node would become
four classes kept in step.

Cost: a virtual with a default lets a new node compile without the capability: a
default that no other node reaches is 'std::unreachable()', so a missing
override shows when the compiler runs, not when it is built; the operands of the
no-UB checks ('visit_operands', 3.1) have that default too, so no node skips the
checks without failing a test that reaches it.

### 7.4 Generic definitions are text parsed again

An instance of a generic type or function is its definition parsed again with
each parameter naming its argument (chapter 2). An instance is then exactly
an ordinary definition: one code path, the same checks and the same errors,
with the instances that led to an error listed as frames.

Rejected: substituting arguments in a parsed tree, every node class would
need a copy, and the parser could not tell 'T{' or 'T.at' apart without
knowing what 'T' names.

Cost: an error in a generic definition shows at its first instance, except
what the tokenizer reads at the definition: strings, character literals and
the closing '}'; the generic functions nothing instantiates are listed in the
report after the code.

### 7.5 Calls are inlined, parameters are bound to arguments

The programs are super loops, so a call is compiled in place by default and a
parameter names its argument instead of copying it: no call overhead, and a
constant argument folds through the body. 'noinline' gives a function a body
of its own for recursion and size.

Rejected: copying the arguments into the callee, a store per argument and no
constant folding through the body; a body per function by default, a call
and a return in the loops of a program that is one super loop.

Cost:

1. a parameter and another name may reach the same bytes, so aliasing is
   checked at compile time (chapter 3)
2. a noinline body cannot see its calls, so a call is rejected when what it
   passes may share storage with what the body writes (4.2)
3. every function is checked without its calls like a noinline body, and an
   error that needs the arguments of a call ends that check (1.5)
4. a statement compiles once per inlined call and per version, so its compile
   must add the same to 'toc' each time (1.4), and the compile time multiplies
   along nested choices (5.2)
5. an inlined body holds the registers of its caller (1.8), and the limits of
   inlined calls ('call_limits', e.g. 64 levels of an inlined recursion) depend
   on the caller

### 7.6 'toc' is one facade over the tables

Every statement receives 'toc', which owns the scopes, the types, the
functions, the generics, the data and the storage layout. A method of 'toc'
changes state or combines tables, a question to one table goes through its
const accessor, e.g. 'tc.scopes().lookup(name)'. The exception is a question
the statements ask at every step, which has a 'toc' method that forwards it:
'is_reachable', 'current_definition', 'source_of', and the two answers of
'ident_resolver', 'resolve_declared' and 'storage_of'.

'toc' is also the compile context: it carries the machine and whether the
program is being parsed or compiled, and whether a dry run is active. A
statement compiles with the names and the machine together, e.g. a variable
resolves to an operand that the machine then uses, and the mode decides what a
declaration does, e.g. the parse declares names without storage; one parameter
keeps them consistent.

Rejected: passing the tables one by one, every signature would carry the
ones its callees need.

Cost: 'toc.hpp' holds about 4k lines and 'toc' more than 70 public methods,
grouped by the map above the class; a node can reach every table through
it.

### 7.7 One translation unit

'main.cpp' includes the headers, there is no build system and the compiler
sees the whole program. The include cycles between the statement classes are
broken in 'decouple.hpp', 'parse_statement.hpp', 'expr_type_impl.hpp' and
'stmt_identifier_impl.hpp' (the members of 'stmt_identifier' that use its
index expressions, so that the expressions can include 'stmt_identifier.hpp');
for the same reason three headers define members of an earlier class:
'tokenizer.hpp' the 'compile_error' of a tokenizer, 'register_report.hpp'
'machine::register_error' and 'machine::register_peak_report', and
'stmt_identifier.hpp' the operand walks of 'statement' ('visit_reads',
'visit_call_effects', 'has_inlined_call', 'first_read_of',
'assert_result_not_read') and 'statement::bare_path'. A forward
declaration of a class used only by reference or pointer is the opaque-pointer
pattern and is also allowed, e.g. 'stmt_def_dat' and 'stmt_def_func' in
'toc.hpp'.

Rejected: a translation unit per header, each cycle between the node
classes would need an interface of its own.

Cost: every change rebuilds everything.

### 7.8 The first error ends the compile

An error is a 'compile_error' at the token that caused it, with the inlined
calls and generic instances that led there (1.9). No code recovers from an
error, so no error leads to a second, wrong one; the places that change an error
instead of ending with it are in 1.9.

Rejected: reporting several errors, each statement would need a point to
recover at and later errors could be caused by earlier ones.

Cost: one error per run.

### 7.9 A range of an array is spelled two ways

'array_copy' and 'arrays_equal' take two arrays and one count, so each array
says where it starts: 'array_copy(a[1], b, n)'. 'read', 'write' and the sector
functions take one array and follow the system calls: the count and the start
are trailing arguments that may be left out, 'read(fd, a, n, 1)', and without
them the whole array (or one sector) is transferred. An element there, e.g.
'read(fd, a[1])', would leave unclear whether it means one element or the rest
of the array, so the io functions reject it and name the start argument
('stmt_builtin_io::assert_buffer_path').

Rejected: one spelling for both. A trailing start for the bulk functions needs
one per array, and an element as the io buffer needs a count to say how far
it reaches, which the io functions make optional.

Cost: two range models in the code, 'bulk_operands' and 'stmt_builtin_io';
the check of a constant range ('assert_constant_range') is shared.

### 7.10 Two versions compiled and measured

Where the shorter of two forms depends on code that is not compiled yet, both
are compiled and the shorter is kept (chapter 5).

Rejected: a cost model, which would duplicate the code generator of each
backend and have to follow every change of it.

Cost: the compile time multiplies along nested choices (5.2); every compile must
be repeatable (1.4); the state a version changes is saved and put back by its
owner (5.3).

### 7.11 The front end holds registers in stack order

A node allocates the scratch registers its operands need from 'machine' and
frees them in the reverse order (1.8); there is no register allocator over a
function.

Rejected: virtual registers and an allocator, which need a representation below
the statements (7.1).

Cost: an expression can run out of scratch registers where an allocator would
spill to memory, the error advises to simplify the expression; and which
register a value takes depends on how many are live, so the error is one of the
kinds a version or the check of a function without its calls may drop (1.9);
the targets have different numbers of scratch registers, so whether such an
expression is accepted depends on the target (chapter 8).

### 7.12 The default type is the width of a register

'int', the type of a variable or a parameter without one, is 'i64' on x86_64 and
'i32' on rv32i (1.7), the width of a register, so a value of it needs no
extension or second register.

Rejected: one default type on every target; 'i64' doubles every value on
rv32i, 'i32' gives up the width of x86_64 (decided 2026-10-26).

Cost: the result of the default type's arithmetic, a panic included, depends on
the target, e.g. 'x + x' with 'x' 2000000000 is 4000000000 on x86_64 and an
overflow on rv32i (chapter 8); a program that writes its types does not.

### 7.13 The kind of a value is checked where it is parsed

The compile decides whether a program is accepted, the parse is its first
validation. One rule is checked at the parse: the kind of a value at its
destination, a 'bool' or an instance where a number is needed and a number or
an instance where a 'bool' is ('expr_any::assert_number_kind',
'expr_bool_op::assert_boolean_operands'); the compile checks the width of the
same value and asserts the kind ('stmt_identifier::assert_not_narrowed').

Why: every value passes one point at the parse, the constructor of 'expr_any'
with the type of its destination, while the compile reaches a value in about
seven ways, its compile, its write plan, a fold as a write ('dat') and as a
question ('let' learns whether it is a constant), the index of an element, its
constant value and the field of an instance literal; a check in each is a type
error accepted where one is missed. Since all code is compiled (1.5.1), the
phase changes no verdict.

Rejected: the compile as the one owner (tried 2026-10-26: each path that was
covered showed the next one).

Cost: one rule of the value at its destination is split by phase, the kind at
the parse and the width at the compile.

### 7.14 The range of a bulk operation is checked after its count

'array_copy', 'arrays_equal', 'read', 'write' and the sector functions emit
their arguments in the order of the source: the arrays or the buffer, then
the count. The range from a start needs the count, so the check is split. The
address walk checks a run-time start as '0 <= start <= length', or against
the length less a constant count, and copies it into a register the caller
allocated before the walk ('lea_request::start_register'); after the count,
'bulk_operands::run_time_emitters' checks 'start + count <= length' and frees
the register. No element is read or written before the check.

Rejected: the count first, with a check that the order makes no difference
(the check saw only writes to variables, so output, 'exit' and the run-time
checks ran in the reverse of the source order, bug hunt round 9 W1-1); the
count first in the language, e.g. 'array_copy(n, a, b)' (a language change for
a code generation detail).

Cost: a run-time start is held in a register while the count is computed.

## 8. Undefined behavior: what is rejected and what is checked

The goal is that every program the compiler accepts is free of undefined
behavior: each case below is either rejected at compile time or, with its
'--checks' option, checked at run time, so a result does not depend on the
target, on whether a call is inlined or on which version of code is kept.

The exceptions, all listed in this chapter:

1. the goal holds under 'noub'; by default the run-time checks are off (8.2)
2. the bytes that 'read' and 'sector_read' put into a buffer are not checked
   against their type (below)
3. the target: items 1 to 5 below
4. whether a program is accepted may depend on inlining (1.5, 8.3); its
   result does not

What depends on the target is listed here, nothing else does:

1. the default type, the width of a register (1.7, 7.12): its arithmetic, a
   panic included, its constants, e.g. 'var k = 3000000000' is rejected on
   rv32i, its shift counts, e.g. 'x << 40', whether it mixes with a fixed
   width, and an index, computed in it: 'a[i + i]' with 'i' an 'i32' of
   2000000000 panics 'bounds' on x86_64 and 'overflow' on rv32i; a list led
   by a literal has it (1.7): with 'k' an 'i32' of 31, '1 << k < 0' is false
   on x86_64 and true on rv32i, 'i32(1) << k < 0' is true on both
2. the types a target has: 'i64' is rejected on rv32i
3. the number of scratch registers: an expression that runs out of them on one
   target can be accepted on another (7.11), and the run-time checks take
   some, e.g. the 'frame' check of a noinline call with many indexed
   arguments; also the registers an instruction fixes:
   'exit(int(arrays_equal(a[s], b, c)))' is rejected on x86_64, where
   'arrays_equal' needs the 'rdi' that 'exit' holds
4. the limits of a target: '--vars' and the memory it addresses, which a
   type stays below, e.g. a field 'i8[0x100000000]' is rejected on rv32i,
   the depth of noinline calls that fits in '--vars' before 'frame' stops
   it, since a frame holds an address for each parameter and the result,
   8 B on x86_64 and 4 B on rv32i, and a deep noinline recursion, which
   'stack' stops on the bare-metal targets and the operating system on
   x86_64 and rv32i (8.1); on rv32i linux the linker places the program, so
   code, data and variables that fit in 4 GiB from address 0 can still fail
   the link, e.g. '--vars=0xffff0000' with 'ld.lld', which starts it at
   about 0x11000; on x86_64 nasm truncates a label to 4095 characters, so a
   name of about 4000 characters gives a listing that nasm rejects
5. the devices of the bare-metal targets: 'rv32i-qemu' has no sd card, so
   'sector_read' and 'sector_write' are rejected there; 'rv32i-fpga' does not
   detect a sector past the end of the card, which panics 'sd card' on x86_64
   and rv32i; the uart of 'rv32i-qemu' and 'rv32i-fpga' takes every file
   descriptor, a 'read' returns at most one line, a carriage return reads as a
   newline, and an end of transmission byte ends a read, also inside the
   input; code, data, variables and '--stack' fit in the memory of the
   device, 128 MiB on 'rv32i-qemu' and '--memory' on 'rv32i-fpga'

So whether a program is accepted can depend on the target through 1 to 5,
and its result through 1, 4 and 5; under 'noub' the result of an accepted
program that writes its types and stays within the limits does not.

The bytes that 'read' and 'sector_read' put into a buffer are assumed to be
written from the same type, e.g. by 'write' or 'sector_write' of such a
buffer: a 'bool' is 0 or 1 and the padding of a record is 0. Other bytes are
not checked; a 'bool' read as 120 is then neither true nor false, and two
records with equal fields compare unequal, since '==' compares their bytes.

The checks run in three phases: the parse rejects the reads of what is not
assigned and the kinds of value a destination does not take, the compile the
order, the aliasing, the storage a call shares, the narrowing and the
constants out of range, and the code the run-time checks.

### 8.1 The catalog

| class | example | how | option | tests |
|---|---|---|---|---|
| uninitialized read | 'var x = x', 'res' read before it is set | rejected ('stmt_def_var' for a declaration; definite assignment of the result, 'ub_assigned.hpp', 3.10) | always | 0099, 0100, 0463, 0464, 1301-1303 |
| an argument sharing bytes with another argument that the call writes | 'f(a, a)' with 'f' writing 'p.x' and reading 'q.x' | rejected ('stmt_call::assert_no_shared_storage', 3.7) | always | 0525, 0642, 0676 |
| a result or a global sharing storage with an argument | 'p = f(p)' with 'f' writing its result before it reads 'p', 'f(g)' with 'f' using the global 'g' | rejected ('stmt_call::assert_no_shared_storage', 3.7, 4.2) | always | 0522-0524, 0526, 0527, 0941 |
| a value written in parts reading a part written before it | 'p = {p.y, p.x}', 'q = {{1}, q.a[3]}', 'g = {7, gx()}' | rejected ('stmt_assign_var', the 'write_plan' of 'expr_type.hpp', 3.7) | always | 1608-1612, 1634, 1635, 1644 |
| evaluation order | 'f(arr[i], h(i))', 'i == h(i)' with 'h' writing 'i' | evaluation is left to right; an argument, a side of a comparison or a bulk or io argument that a call in a later one may write is rejected ('assert_not_written_later'), an arithmetic list reads each operand at its place; a destination that the value reads, also through a call, is written last ('reaches_destination'), 3.1 | always | 1636-1639, 1641, 1642, 1676; accepted: 1630-1633, 1640, 1643, 1678, 1702 |
| narrowing | 'b = w' with 'b' an 'i8' and 'w' an 'i16' | rejected; a conversion such as 'i8(w)' wraps on purpose (1.7) | always | 0725, 0856, 0865, 1016, 1017, 1019, 1021, 1022 |
| a constant out of range | 'a[9]' of 'a = i8[4]', 'x / 0', 'x << 70' | rejected ('assert_constant_index' for an index, 'fold' for the others, 1.7) | always | 0305, 0306, 0750-0752, 1118, 1119 |
| a constant expression that overflows | '0x7fffffffffffffff + 1' | rejected ('fold'), else it wraps | 'overflow' | 0995, 1002, 1010 |
| array index out of bounds | 'a[i]' | run time ('machine::check_bounds' from 'stmt_identifier::compile_checked_index', the range of a bulk operation or an io built-in ('bulk_operands', 'stmt_builtin_io') and the count of a 'foo') | 'upper', 'lower' | 0302, 0303, 0307-0309, 0626, 0627, 1355 |
| division by zero and 'MIN / -1' | 'a / b' | run time ('machine::divide' from 'expr_arith::emit_operation', not for a constant divisor other than -1); a constant 'MIN / -1' is rejected ('fold'), the 64-bit one also without the check, it has no value | 'division' | 0965-0968, 1775-1777 |
| shift count below zero or not below the width | 'a << n' | run time ('machine::shift' with 'check_options::shift_options', from 'expr_arith::emit_operation') | 'shift' | 0975-0977 |
| signed overflow | 'a + b' | run time ('machine::add_subtract', 'multiply' and 'unary' with 'toc::overflow_options') | 'overflow' | 0988, 0989, 0994 |
| 'array_copy' whose destination starts inside the source | 'array_copy(a, a[1], 3)' | run time ('machine::copy_elements' with the overlap check of 'stmt_builtin_array_copy') | 'overlap' | 0981-0983 |
| the variables of a noinline call past '--vars' | deep noinline recursion | run time ('machine::check_frame_capacity' from 'noinline_call::populate_frame', 4.1) | 'frame' | 0413, 0414, 0434 |
| the stack of a noinline call | deep noinline recursion | run time on rv32i-qemu and rv32i-fpga ('machine::check_stack_capacity' from 'noinline_call::populate_frame'); on x86_64 and rv32i the operating system stops the program | 'stack' | 0985-0987 |

'noub' turns on every check of the table: 'upper', 'lower', 'frame',
'division', 'shift', 'overlap', 'stack' and 'overflow'.

### 8.2 Without the run-time checks

The goal holds under 'noub'. By default no run-time check is on, so an accepted
program can reach the run-time classes of the table; the defaults stay so for
the size and the speed of the code (decided 2026-10-23). Measured on x86_64
without '--checks': a write past an array goes on silently, a division by zero
stops the program with a floating point exception, a shift by 65 shifts by 1
because the machine masks the count to the width of the register (6 bits on
x86_64, 5 on rv32i) and a signed overflow wraps, also a constant one. Such a
result depends on the target. With 'noub' each of them stops at a panic that
names the check, e.g. 'panic: bounds'.

A panic prints its message on a line of its own after the output of the
program, on standard error where there is one, on the uart of rv32i-qemu and
rv32i-fpga, and
exits with the code of its check, the same on every target
('machine::failure_handler'):

| panic | exit code |
| --- | --- |
| sd card | 248 |
| overflow | 249 |
| stack overflow | 250 |
| overlap | 251 |
| shift | 252 |
| division | 253 |
| frame overflow | 254 |
| bounds | 255 |

An unchecked division by zero on a bare-metal target exits 253 without the
message, x86_64 and rv32i linux stop with a signal.

### 8.3 Known limits

- A recursive call's effects treat every argument of a 'mut' parameter as
  written in whole, whatever the body writes, so it needs no fixed point of
  its summary; this rejects more than needed, it accepts no undefined
  behavior.
- Definite assignment does not count the writes of a call, e.g.
  'init(res)' is a read of 'res'; a conservative rejection.
- Whether a 'let' is a constant is decided at the parse
  ('make_stmt_def_var'), where the 'n' of a 'foo' over an unsized array
  parameter is a variable; so 'let m = n' stays a variable also at a call
  that gives the length, and 's[m]' past the end is left to the run-time
  bounds check where 's[n]' is rejected at compile time; an item of
  'etc/todo.txt'.
- The check of a body without its calls ends at the first error that depends on
  a call other than an inlined recursion, e.g. running out of scratch registers,
  so the code after it is not checked there (1.5): e.g. a later 'res = f(res)'
  is then judged only at each call, where an inlined result held in a register
  shares nothing, so it is accepted on x86_64 and rejected on rv32i.
- A noinline call there with an array argument of unknown length is not
  checked with the lengths of its other array arguments, which an inlined
  call is: in a function nothing calls the verdict depends on inline versus
  noinline (1.5 item 4).
- An inlined call that never returns ends the path where it is a whole
  statement, an 'if' condition, a 'foo' count or the body of a 'foo' that
  runs, but not inside a longer expression ('stop(k) + a[k]', an argument,
  an index then the value) or on one exit of an 'and' or 'or'
  ('if x == 1 and stop(k) == 1 { a[k] = 1 }', whose code runs only after
  'stop' returns): the code after it is compiled as reached, so a constant
  argument may make an inlined call reject what a noinline one accepts
  (1.5); no result changes. Ending the path there needs the reach of each
  exit of a condition and every operand list to stop at the call (bug hunt
  round 14 B1-1, B1-2).
- Code that a compile skips (1.5.1) is checked in a dry run, which emits
  nothing, so its variables take no storage and are not held to '--vars':
  in 'main', 'if 1 == 2 { var a = i8[300000] }' is accepted where the
  declaration alone is rejected with '--vars=0x40000'; none of it runs, it
  accepts no undefined behavior.
- The x86_64 listing leaves the size of each jump to nasm. On rare listings
  nasm's default search ('-Ox') does not settle ("unable to find valid values
  for all labels"), e.g. a '--nopt' listing of 'fuzz-flow' seed 71;
  'nasm -O1' assembles it, with longer jumps.

## 9. Changing a target or a check

The language is final; what a target or a check option touches, the copies
outside 'src' included:

- a target: a subclass of 'machine_rv32i_bare_metal' or 'machine_rv32i' with
  its 'target_description', and in 'main.cpp' its row in 'target_names'
  (name, backend, image, the options it reads) and the help
  text; outside: 'run.sh', 'qa/coverage/test-coverage.sh',
  'qa/coverage/test-all.sh' and 'qa/coverage/test-cli.sh' (their lists of
  targets) and 'qa/fuzz/fuzz-options.py'
- a check: a field of 'check_options' ('toc.hpp'), its name and help text in
  'main.cpp', a 'failure_handler' with its exit code in 'machine.hpp', the
  operation that takes the check in both backends, and the tables of chapter
  8; outside: 'qa/fuzz/fuzzlib.py' (the exit codes of the panics),
  'qa/fuzz/fuzz-options.py' and 'qa/coverage/test-cli.sh' (the check names)
