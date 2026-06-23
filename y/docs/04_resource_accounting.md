# Resource Accounting

Every object managed by *yos* — threads, memory blocks, timers, events, services — is a **system resource**. The operating system tracks all allocated resources so it can, among other things, automatically release everything a process owns when that process exits. This chapter explains the data structures and conventions behind this tracking.

## Linked Lists

Resources of the same type are grouped into singly-linked lists. The generic list infrastructure lives in `list.c` and works with *any* structure whose first field is a `list_item_t`:

```c
typedef struct list_item_s
{
    void *next;         /* pointer to next item, or NULL */
    uint8_t data[0];    /* payload begins here */
} list_item_t;
```

Because `next` is always the first field, the list functions can traverse any linked structure without knowing anything about its payload. This is the C equivalent of a base class. The available operations are:

| Function | Description |
|---|---|
| `list_insert(first, el)` | Insert `el` at the **head** of the list |
| `list_append(first, el)` | Append `el` at the **tail** of the list |
| `list_remove(first, el)` | Unlink and return `el` from the list |
| `list_remove_first(first)` | Unlink and return the head element |
| `list_find(first, prev, match, arg)` | Find first element matching a predicate |
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

A pre-built predicate `list_match_eq` is provided for the common case of matching by pointer equality.

## System Objects (`sysobj_t`)

Every tracked resource needs two things: a list link (so it can live inside a linked list) and an *owner* (so the OS knows which process to blame when it needs to clean up). The `sysobj_t` header combines both:

```c
typedef struct sysobj_s {
    union {
        list_item_t hdr;    /* binary-compatible list link */
        void* next;         /* shortcut to the next pointer */
    };
    void* owner;            /* which process/thread owns this */
} sysobj_t;
```

The `union` ensures that `sysobj_t` is binary-compatible with `list_item_t`. This means the list functions work unchanged on system objects — they see a valid `next` pointer at offset zero.

The `owner` field holds the address of the owning `process_t` (or `NONE` / `NULL` if no specific owner). When a process is destroyed, the OS can scan every resource list and free anything whose `owner` matches the dying process.

## Deriving Your Own Resource Type

Every concrete resource type — `thread_t`, `block_t`, `timer_t`, `event_t`, `service_t` — begins with a `sysobj_t hdr` as its first member. This is the *yos* convention for resource derivation:

```c
/* Example: a timer is a system object */
typedef struct timer_s {
    sysobj_t hdr;           /* MUST be first */
    void (*hook)();         /* timer callback */
    uint16_t ticks;         /* fire interval in 50 Hz ticks */
    uint16_t _tick_count;   /* countdown (internal use) */
} timer_t;
```

Because `hdr` occupies offset 0, a `timer_t *` can be safely cast to a `sysobj_t *` or a `list_item_t *` and passed directly to any list or sysobj function. In object-oriented terms, `timer_t` *inherits* from `sysobj_t` which in turn *inherits* from `list_item_t`.

The memory layout looks like this:

```
timer_t in memory:
┌────────────────────┬──────────┬────────────────────────────────────┐
│ next (2 bytes)     │ owner    │ hook | ticks | _tick_count | ...   │
│ (list_item_t.next) │ (2 bytes)│ (resource-specific fields)         │
└────────────────────┴──────────┴────────────────────────────────────┘
 ↑
 list functions see this as list_item_t
```

## Allocating and Freeing System Objects

Two helper functions in `sysobj.c` wrap the common pattern of allocating a resource and inserting it into a list:

```c
/* Allocate a new system object of 'size' bytes, assign 'owner',
   and insert it at the head of '*first'. Returns the new object,
   or NULL on allocation failure. */
void *so_create(void **first, uint16_t size, void *owner);

/* Remove 'so' from list '*first' and free its memory.
   Returns the freed object, or NULL if not found. */
void *so_destroy(void **first, void *so);
```

A typical resource constructor looks like this:

```c
timer_t *tmr_install(void (*hook)(), uint16_t ticks, void *owner) {
    timer_t *t;
    if (t = (timer_t *)so_create(
            (void **)&_tmr_first, sizeof(timer_t), owner)) {
        t->hook = hook;
        t->ticks = ticks;
        t->_tick_count = ticks;
    }
    return t;    /* NULL if allocation failed */
}
```

And the corresponding destructor is simply:

```c
timer_t *tmr_uninstall(timer_t *t) {
    return (timer_t *)so_destroy((void **)&_tmr_first, (void *)t);
}
```

## Ownership and Process Cleanup

The `owner` field is set by `so_create` to whatever you pass as the third argument. For resources owned by a specific process, pass a pointer to its `process_t`. For OS-level resources with no specific owner, pass `NONE` (which is `0`/`NULL`).

When a process exits, `_process_cleanup()` is called. Its job is to walk every resource list — timers, events, memory blocks, threads, services — and destroy any entry whose `owner` matches the dying process. This prevents resource leaks even if user code forgets to free everything.

> **For junior developers:** Think of ownership like borrowing a library book. Each book (resource) has a borrower's card (owner field). When someone leaves (process exits), the library automatically collects all books they borrowed, regardless of where the books currently are on the shelves.

## Ownership Chain Example

Here is the chain of ownership when a process creates a thread:

```
process_t  (owner = NONE)
    └── thread_t  (owner = process_t *)
            └── stack block_t  (owner = thread_t *)
```

When the process exits, the cleanup routine:
1. Finds the `thread_t` whose `owner == process_t *`, terminates it, and frees it.
2. Finds the `block_t` (stack memory) whose `owner == thread_t *`, and frees it.
3. Removes the `process_t` itself from the process list and frees it.

This cascading cleanup is why correctly setting the `owner` field when calling `so_create` is essential.
