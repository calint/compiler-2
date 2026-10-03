# Aliasing and `noinline` parameters

## The error

```
roome.baz:529:58: argument 1 'eid' may share storage with receiver
    'rooms.array.entities' ('eid' is a parameter of a 'noinline' function with
    unknown callers and may point into global 'rooms', see
    docs/noinline-aliasing.md), copy 'eid' to a local variable
roome.baz:970:9: called from 'action_go(eid, tz)'
```

The call at line 529, `rooms.array[to_room_id].entities.add(eid)`, hands `eid`
to a function that mutates `rooms`. If `eid` lives inside `rooms`, the mutation
can change `eid` while `add` still reads it.

## Which names are involved

The conflict is between the parameter and the global the callee writes to, not
between two parameters:

| pair                     | checked at | result                         |
|--------------------------|------------|--------------------------------|
| `eid`  vs. `tz`          | call site  | already proven distinct there  |
| `eid`  vs. global `rooms`| callee     | unknown, rejected              |
| `tz`   vs. global `rooms`| callee     | unknown, rejected if both used |

`tz` is `mut`, but that is not what triggers this error: the receiver
`rooms...entities` is the mutated side and `eid` is the argument.

## Why `noinline` cannot know

An inlined function is compiled at its call site, so the compiler sees the
real variable behind every argument. A `noinline` function is compiled once and
shared by all callers, so a parameter is only a pointer.

```
inlined: each call site is known

  caller A:  f(local_a)         f's 'eid' is 'local_a'   -> not in 'rooms'
  caller B:  f(rooms...x)       f's 'eid' is 'rooms...x' -> in 'rooms'
             (each copy is checked on its own)


noinline: one body, many callers

  caller A:  f(local_a) ----+
                            +--> f(eid)   'eid' is a pointer, to what?
  caller B:  f(rooms...x) --+

  memory
  +---------------------------+   +-----------+
  | global 'rooms'            |   | local_a   |
  |  +------+------+------+   |   +-----------+
  |  | ...  | x    | ...  |   |        ^
  |  +------+------+------+   |        |
  +--------------^------------+        |
                 |                     |
                 +---- 'eid' ----------+   one of these, unknown to f
```

Inside the body the compiler has to assume the worst case: `eid` points into
`rooms`. Then:

```
rooms.array[to_room_id].entities.add(eid)
        \_____________  _________/     \_/
                      \/                |
          writes into 'rooms'      reads 'eid'

   rooms: [ ... | entities[ a b c ] | ... ]
                          ^
                          'eid' may be one of a, b, c
                          'add' shifts or overwrites them -> 'eid' changes
                          while 'add' is still using it
```

Whether the caller really passes such an `eid` does not matter. The body is
shared, so one possible caller is enough to reject it. A pointer to a global
cannot be excluded without seeing every caller.

## Fix

Copy the value into a local variable. A local is its own storage, so no
callee write can reach it:

```
let id = eid                           # 'id' cannot point into 'rooms'
rooms.array[to_room_id].entities.add(id)
```

Copy whichever side is cheaper; for a scalar like `eid` this is a single move.
A copy of the mutated global or of a large struct is rarely the better choice.

## Related

- Message source: `shared_storage_reason()` in `src/stmt_call.hpp`.
- Example of the pattern in use: the note above `parse_input` in
  `etc/roome/roome.baz`.
