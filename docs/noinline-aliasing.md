# Aliasing and `noinline` calls

## The rule

A call to a `noinline` function is accepted when the same call would compile
as an inline call. The compiler checks this at each call site by compiling the
body with the real arguments and dropping the code, so the aliasing checks see
the variables the arguments name:

```
var g = 2

func swap_values(x mut, y mut) { ... }

func noinline outer(a mut) {
    swap_values(a, g)
}

func main() {
    var v = 1
    outer(v)        # ok: 'a' is 'v', not 'g'
    outer(g)        # error: 'a' and 'g' both name 'g'
}
```

```
x.baz:12:20: argument 2 'g' may share storage with argument 1 'a' (both name
    'g'), use a separate variable
x.baz:17:5: called from 'outer(g)'
```

## Why the check is per call

The body of a `noinline` function is compiled once and shared by all callers,
so a parameter is only a pointer in the generated code. Which variable it
points to is known only at a call site. Checking there gives the exact answer
for each caller.

## Details

- A call is checked once for each pattern of its arguments: which are globals
  and which are the same local. Calls with the same pattern share the check.
- A result that is not in memory, e.g. a call inside an expression, and an
  argument without storage, e.g. a constant or an expression, go through a
  temporary in the frame of the caller. A temporary is a distinct local, it
  never shares storage with another argument. The pattern of such a call
  marks those arguments, so the check is done once for them too. An instance
  argument that is not a variable, e.g. `point{1, 2}` or `mk(1, 2)`, is made
  in such a temporary for inlined and `noinline` functions alike.
- A call inside a `noinline` body that passes a parameter of that body is
  checked when the body's own callers are, with their arguments.
- A recursive call has the same pattern as the call that started it, so it is
  not checked again.
- A `noinline` function that is never called is compiled for syntax and types
  only, its aliasing is not checked.
- The check compiles the body once more per pattern, the code is dropped.

## Related

- Source: `check_noninline_aliasing()` in `src/stmt_call.hpp` and
  `toc::check_only()` in `src/toc.hpp`.
