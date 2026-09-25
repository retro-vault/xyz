# Memory, Time, and Concurrency

## Use normal C allocation

On `--platform=yos`, standard allocation crosses the YOS service table:

```c
#include <stdlib.h>

unsigned char *bytes = malloc(128);
if (!bytes)
    return 1;
bytes = realloc(bytes, 256);
free(bytes);
```

`calloc` and `aligned_alloc` build on the same backend. Each ordinary
allocation carries a private size word, so `realloc` can preserve data.
The kernel records the current process as owner and can reclaim leaked
blocks once the process is reaped. `malloc(0)` returns `NULL`; a failed
allocation sets `errno` to `ENOMEM`. Because a standard C pointer is 16
bits, these calls allocate only in the bank currently executing the
process or library, and never return a near pointer into a different
bank.

The raw `allocate_memory`, `shrink_memory`, and `free_memory` table
entries are useful for building a custom allocator, but do not mix raw
and libc pointers:

```c
yos_user_ptr_t raw = yos->allocate_memory(40);
if (raw) {
    yos->shrink_memory(raw, 24); /* keep 24 bytes, release the rest */
    yos->free_memory(raw);       /* not free(raw) */
}
```

Raw allocation searches every configured user-heap bank and returns a
three-byte far pointer. XCC dereferences typed `[[xcc::far]]` pointers
through the YOS RST 30 data gate. Keep the far type — casting it to
`void *` discards the bank, and is only valid once the caller has
separately proved it names the current bank.

## Time is a 50 Hz counter

```c
uint16_t then = yos->clock_ticks();
while ((uint16_t)(yos->clock_ticks() - then) < 50) {
    /* approximately one second */
}
```

Unsigned subtraction handles a single 16-bit wrap correctly. The public
value is the low half of the kernel's monotonic tick counter.
`gettimeofday` and `settimeofday` return `-1`/`ENOSYS` — YOS has no Unix
epoch clock at all.

A timer invokes a no-argument hook from interrupt/scheduler context. The
kernel's current implementation fires after `ticks + 1` frame ticks and
then reloads periodically:

```c
static volatile unsigned flashes;
static void tick(void) { ++flashes; }

yos_timer_t *timer = yos->create_timer(tick, 49); /* about once a second */
/* ... */
yos->destroy_timer(timer);
```

Timer hooks must be quick, nonblocking, and careful with shared state.
The public timer adapter remains kernel-owned in ABI 1, so destroy every
timer you create explicitly.

## Critical sections

Use the nestable pair only around the smallest possible shared update:

```c
yos->enter_critical_section();
shared_head = new_head;
yos->leave_critical_section();
```

The first call disables interrupts; the final matching leave restores
the previous interrupt state, so a callback entered with interrupts
already disabled stays disabled throughout. All registers and flags are
preserved. Balance every path, with at most 127 nested sections; an
unmatched leave is simply a no-op. This synchronizes against maskable
interrupts and IM2 threads — it is not an NMI-safe API, since NMI belongs
to esxDOS.

Do not wait, exit or suspend the current thread, run large loops, or wrap
disk loading in an outer critical section. The scheduler, input scans,
and timers cannot run at all while interrupts are masked.

## What the kernel makes thread-safe

Heap and list mutations, event creation/set/destruction, timer
publication/removal, service registration/lookup, vector updates, the
keyboard queue, and mouse timer-state snapshots are all protected.
Filesystem descriptor reservation, validation, native I/O, and commit
form one single critical section — an append seek plus a write is one
transaction too. Native esxDOS gates remain protected because firmware
maps the YOS ROM out while it runs; long disk calls can therefore delay
scheduling and lose clock ticks. File descriptors and the current
directory are system-wide, not private per process, so a sequence such
as `chdir` followed by `open` is not atomic as a pair.

`load_process` and `load_library` complete synchronously for their
caller. A global try-lock protects the whole load and its initializer; a
competing or recursive load returns `NULL`/`YOS_PROCESS_LOAD_BUSY`
immediately, without waiting — retry later with interrupts enabled. CRC
checking, relocation, and library initialization do not mask interrupts
for their entire duration. A shared interface only becomes visible after
initialization succeeds. Loading is not cancellation-safe: never
asynchronously terminate a thread inside a load or initializer — there is
no recovery protocol for abandoning the active loader's lock, its open
descriptor, or its stack frame.

The fixed cells behind `yos->error_number` and
`yos->process_load_error` are saved and restored with the running
thread, so another thread can never overwrite your raw syscall error
before you get to read it. This does **not** turn the separately linked C
library's process-local `errno` object into thread-local storage —
multithreaded callers should use the raw YOS filesystem entries and
`*yos->error_number` instead, or protect a libc call together with its
`errno` read as one unit.

GPX contexts are separately allocated and process-owned. The screen
itself is still shared: raster spans, bitmap rows, and individual pixel
updates are protected, and sprite save/draw/restore calls are protected
as complete small operations. Text and compound shapes, however, can
still interleave between primitives — use separate screen regions or
your own application synchronization for overlapping artwork.

Caller-owned buffers, mutable service state, sprite lifetimes, and
multi-call operations remain the caller's own responsibility. Never free
a buffer or context, close a handle, or unregister code while another
thread still intends to use it. `readdir` returns storage that belongs to
its `yos_directory_t`; copy it before another thread reads the same
directory. Thread-safe kernel calls are not universally safe from
interrupt callbacks either: a callback may set an event or resume a
suspended thread, but it must never block, perform disk I/O, remove a
timer from the active chain, or explicitly enable interrupts.

## Events and blocking

```c
yos_event_t *ready = yos->create_event(NULL);
if (ready) {
    yos->set_event(ready, YOS_EVENT_SET);
    yos->wait_event(ready);           /* ABI 2: block until consumed */
    yos->set_event(ready, YOS_EVENT_RESET);
    yos->destroy_event(ready);
}
```

The owner argument is an opaque owner pointer; pass `NULL` unless
kernel-level code has a valid process owner to give it. `set_event`
checks that the object is still registered and returns it, or `NULL` for
an invalid one.

ABI 2 adds `wait_event`, the public blocking primitive built on this
signalling object: it suspends the calling thread with no scheduled CPU
time until the scheduler consumes exactly one signal on the event. See
[`wait_event`](../the-book-of-yos/THREADS.md#wait_eventevent--abi-2) for
the full contract, including why it must be called with interrupts
enabled, outside any critical section, and never from a timer hook.

## Threads

New threads begin suspended and need a process handle:

```c
static void worker(void) { for (;;) { /* work */ } }

yos_thread_t *thread = yos->create_thread(worker, 256, process);
if (thread)
    yos->resume_thread(thread);
```

`suspend_thread(thread)` yields when it suspends the current thread;
`resume_thread(thread)` moves a suspended thread onto the runnable
queue; `exit_thread(thread)` marks a runnable thread for scheduler
cleanup. There is no public current-process getter and no thread join in
ABI 1. A process returned by `create_process` or `load_process` supplies
the process handle these calls need — code inside an ordinarily loaded
process cannot yet create an additional owned thread without first
receiving that handle from its launcher.

## Processes

Create a process around a resident entry function:

```c
static void child(void) { /* ... */ }

yos_process_t *process = yos->create_process("child", child, 256);
```

This allocates the process and its initial runnable thread. It neither
copies nor relocates code, so the entry function must remain valid for
as long as the process runs. For a disk image, use:

```c
yos_process_t *process = yos->load_process("APP.PRC");
if (!process)
    failure = *yos->process_load_error;
```

`exit_process()` terminates only the calling thread. Its process is
reclaimed only once its last thread exits. There is no wait call, no
parent relationship, and no status return: calling `create_process` or
`load_process` from a process does not make it the parent, and the new
process object always has a null owner and no creator field.

Next: [Files, Input, and Graphics](FILES-INPUT-AND-GRAPHICS.md).
