# Resource Accounting

Every object *yos* manages — threads, memory blocks, timers, events,
services — is a **system resource**. The kernel tracks every allocated
resource so that, among other things, it can automatically release
everything a process owns the moment that process exits. This chapter
covers the data structures and conventions behind that tracking.

## Linked lists

Resources of the same type are grouped into singly-linked lists. The
generic list infrastructure lives in `kernel/list_*.s` (one routine per
module) and works with *any* structure whose first field is a
`list_item_t`:

```c
typedef struct list_item_s
{
    void *next;         /* pointer to next item, or NULL */
    uint8_t data[0];    /* payload begins here */
} list_item_t;
```

Because `next` is always the first field, the list functions can traverse
any linked structure without knowing anything about its payload — the
assembly equivalent of a base class. The available operations, shown here
with the C prototypes they implement, are:

| Function | Description |
|---|---|
| `list_insert(first, el)` | Insert `el` at the **head** of the list |
| `list_append(first, el)` | Append `el` at the **tail** of the list |
| `list_remove(first, el)` | Unlink and return `el` from the list |
| `list_remove_first(first)` | Unlink and return the head element |
| `list_find(first, prev, match, arg)` | Find the first element matching a predicate |
| `list_iterate(first, fn, arg)` | Call `fn` for every element |

The `match` parameter of `list_find` is a user-supplied predicate:

```c
uint8_t my_match(list_item_t *p, uint16_t arg) {
    my_struct_t *s = (my_struct_t *)p;
    return s->id == arg;          /* return 1 if this is the element */
}

my_struct_t *found = (my_struct_t *)list_find(
    (list_item_t *)first,
    (list_item_t **)&prev,
    my_match,
    (uint16_t)target_id
);
```

A pre-built predicate, `list_match_eq`, covers the common case of matching
by pointer equality. Every list routine takes the list head in `HL` and the
element in `DE` (`sdcccall(1)`), and the kernel only ever calls them from
inside a critical section.

## System objects (`sysobj_t`)

Every tracked resource needs two things: a list link, so it can live inside
a linked list, and an *owner*, so the kernel knows which process to blame
when cleanup time comes. The `sysobj_t` header combines both:

```c
typedef struct sysobj_s {
    union {
        list_item_t hdr;    /* binary-compatible list link */
        void* next;         /* shortcut to the next pointer */
    };
    uint8_t owner_bank;     /* FF for a fixed/common owner */
    void* owner;            /* owner address */
} sysobj_t;
```

The `union` guarantees `sysobj_t` is binary-compatible with `list_item_t`,
which is what lets the list functions work unchanged on system objects —
they simply see a valid `next` pointer at offset zero.

The three-byte owner is a packed far pointer in
`bank,address-low,address-high` order. It identifies the owning `process_t`,
`thread_t`, or threadless library object; kernel-owned objects use bank `FFh`
and address zero. When a process or library is destroyed, `process_reap`
matches both bank and address while scanning resource lists.

This field is not process ancestry. Process objects themselves have owner
`NONE`; YOS keeps no parent pointer, child list, wait relation, or exit
status anywhere.

## Deriving your own resource type

Every concrete resource type — `thread_t`, `block_t`, `timer_t`, `event_t`,
`service_t`, `process_t` — begins with a `sysobj_t hdr` as its first five
bytes. This is the *yos* convention for resource derivation:

```c
/* Example: a timer is a system object (11 bytes) */
typedef struct timer_s {
    sysobj_t hdr;           /* offset 0: MUST be first */
    void (*hook)();         /* offset 5: timer callback */
    uint16_t ticks;         /* offset 7: fire interval in 50 Hz ticks */
    uint16_t _tick_count;   /* offset 9: countdown (internal use) */
} timer_t;
```

The kernel has no C headers of its own; each assembly module spells these
offsets out as `.equ` constants (`THREAD_SP`, `PROCESS_MAIN_THREAD`, and so
on). The C forms shown throughout this book exist purely for readability.

Because `hdr` occupies offset 0, a `timer_t *` can be cast safely to a
`sysobj_t *` or a `list_item_t *` and passed directly to any list or
`sysobj` function. In object-oriented terms, `timer_t` *inherits* from
`sysobj_t`, which in turn *inherits* from `list_item_t`.

The memory layout looks like this. List functions see the object as a
`list_item_t` starting at offset 0:

| Offset | Field | Size | Notes |
|---|---|---|---|
| 0 | `next` | 2 bytes | `list_item_t.next` |
| 2 | `owner` | 3 bytes | Packed far owner: bank, low, high |
| 5 | resource fields | rest | `hook`, `ticks`, `_tick_count`, … |

## Allocating and freeing system objects

Two helpers, `kernel/so_create.s` and `kernel/so_destroy.s`, wrap the
common pattern of allocating a resource from `__sys_heap` and inserting it
into a list:

```c
/* Allocate a new system object of 'size' bytes, assign 'owner',
   and insert it at the head of '*first'. Returns the new object,
   or NULL on allocation failure. */
void *so_create(void **first, uint16_t size, void *owner);

/* Remove 'so' from list '*first' and free its memory.
   Returns the freed object, or NULL if not found. */
void *so_destroy(void **first, void *so);
```

A typical resource constructor (`kernel/tmr_install.s`) does the equivalent
of:

```c
timer_t *tmr_install(void (*hook)(), uint16_t ticks, void *owner) {
    timer_t *t;
    enter_critical_section();
    if (t = (timer_t *)so_create(
            (void **)&_tmr_first, sizeof(timer_t), owner)) {
        t->hook = hook;
        t->ticks = ticks;
        t->_tick_count = ticks;
    }
    leave_critical_section();
    return t;    /* NULL if allocation failed */
}
```

`so_create` is itself an internal, unprotected helper — the caller must
hold the critical section through full initialization, not merely through
allocation and list insertion. `so_destroy` protects its complete
unlink/free transaction with `__critical_call`, so nesting it inside an
already-protected caller is safe.

The corresponding destructor (`kernel/tmr_uninstall.s`) is simply:

```c
timer_t *tmr_uninstall(timer_t *t) {
    return (timer_t *)so_destroy((void **)&_tmr_first, (void *)t);
}
```

The same pattern builds events (`evt_create`/`evt_destroy`, 6 bytes),
services (`svc_register`/`svc_unregister`, 23 bytes), library references
(7 bytes), threads (`thread_create`, 39 bytes), and processes or
threadless library owners (`process_start`, or the library loader, 17
bytes).

## Ownership and process cleanup

The `owner` field is set by `so_create` to whatever you pass as the third
argument. For resources owned by a specific process, pass a pointer to its
`process_t`. For OS-level resources with no specific owner, pass `NONE`
(which is `0`/`NULL`).

When a process has no threads left, the scheduler calls `process_reap`. It
destroys the process's owned events, timers, services, and fixed OS-heap
blocks; releases library references and allocations from every executable
bank; and only then destroys the process itself. A library is reaped once
its reference count reaches zero. This scheme prevents leaks even when
application code forgets to release process-owned allocations explicitly.
The full details are in [Cleaning Up Resources](CLEANUP-RESOURCES.md).

> Think of ownership as a library card system: each book (resource) carries
> a borrower's card (the `owner` field). When a borrower leaves — a process
> exits — the library automatically reclaims every book checked out under
> their card, wherever those books currently sit on the shelves.

## Ownership chain example

Here is the chain of ownership when a process creates a thread:

| Object | Owner / link | Notes |
|---|---|---|
| `process_t` | `NONE` | No parent field |
| `thread_t` | `process = process_t *` | Created by the process |
| stack `block_t` | `owner = thread_t *` | Freed when the thread exits |
| application block / service / event | `owner = process_t *` | Freed when the process is reaped |
| `library_reference` | `owner = process_t *` | Points at the library object |

When the thread terminates and the next tick arrives, the cleanup pass:

1. Frees every fixed OS-heap block whose `owner == thread_t *` (the stack)
   and destroys the `thread_t`.
2. Calls `process_reap` for the process. If no other thread of that
   process is still alive, it destroys the owned events, timers, and
   services, frees every fixed OS-heap block and every banked user-heap
   block whose `owner == process_t *` (including the loaded XPRG image),
   and finally removes the `process_t` itself from the process list.

This cascading cleanup is exactly why setting the `owner` field correctly
on every call to `so_create` or `mem_allocate` matters.
