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
allocation has a private size word so `realloc` can preserve data. The kernel
records the current process as owner and can reclaim leaked blocks when the
process is reaped. `malloc(0)` returns `NULL`; failed allocation sets `errno`
to `ENOMEM`.

The raw `allocate_memory`, `shrink_memory` and `free_memory` table entries
are useful for a custom allocator, but do not mix raw and libc pointers:

```c
void *raw = yos->allocate_memory(40);
if (raw) {
    yos->shrink_memory(raw, 24); /* keep 24 bytes, release the rest */
    yos->free_memory(raw);       /* not free(raw) */
}
```

## Time is a 50 Hz counter

```c
uint16_t then = yos->clock_ticks();
while ((uint16_t)(yos->clock_ticks() - then) < 50) {
    /* approximately one second */
}
```

Unsigned subtraction handles a single 16-bit wrap correctly. The public value
is the low half of the kernel's monotonic tick counter. `gettimeofday` and
`settimeofday` return `-1`/`ENOSYS`; YOS has no Unix epoch clock.

A timer invokes a no-argument hook from interrupt/scheduler context. The
kernel's current implementation fires after `ticks + 1` frame ticks and then
reloads periodically:

```c
static volatile unsigned flashes;
static void tick(void) { ++flashes; }

yos_timer_t *timer = yos->create_timer(tick, 49); /* about once a second */
/* ... */
yos->destroy_timer(timer);
```

Timer hooks must be quick, nonblocking, and careful with shared state. The
public timer adapter remains kernel-owned in ABI 1, so explicitly destroy
every timer you create.

## Critical sections

Use the nestable pair only around the smallest shared update:

```c
yos->enter_critical_section();
shared_head = new_head;
yos->leave_critical_section();
```

The first call disables interrupts; the final matching leave restores the
previous interrupt state. Thus a callback entered with interrupts disabled
stays disabled. All registers and flags are preserved. Balance every path,
with at most 127 nested sections. An unmatched leave is a no-op.
This is synchronization against maskable interrupts and IM2 threads, not an
NMI-safe API; NMI belongs to esxDOS.

Do not wait, exit/suspend the current thread, run large loops, or add an outer
critical section around disk loading. The scheduler, input scans and timers
cannot run while interrupts are masked.

## What the kernel makes thread-safe

Heap/list mutations, event creation/set/destruction, timer publication/removal,
service registration/lookup, vector updates, the keyboard queue and mouse
timer-state snapshots are protected. Filesystem descriptor reservation,
validation, native I/O and commit form one critical section; append seek plus
write is also one transaction. Native esxDOS gates remain protected because
firmware maps out the YOS ROM. Long disk calls can therefore delay scheduling
and lose clock ticks. File descriptors and the current directory are
system-wide, not private
per process. A sequence such as `chdir` then `open` is not atomic as a pair.

`load_process` and `load_library` complete synchronously for their caller.
A global try-lock protects the whole load and initializer; a competing or
recursive load returns `NULL`/`YOS_PROCESS_LOAD_BUSY`, without waiting. Retry
later with interrupts enabled. CRC/relocation and library initialization do
not mask interrupts for their entire duration. A shared interface becomes
visible only after successful initialization.
Loading is not cancellation-safe: do not asynchronously terminate a thread
inside a load/initializer. There is no recovery protocol for abandoning the
active loader's lock, open descriptor and stack frame.

The fixed cells behind `yos->error_number` and `yos->process_load_error` are
saved/restored with the running thread. Another thread cannot overwrite your
raw syscall error before you read it. This does **not** convert the separately
linked C library's process-local `errno` object into thread-local storage:
multithreaded callers should use raw YOS filesystem entries and
`*yos->error_number`, or protect a libc call together with its `errno` read.

GPX contexts are separately allocated and process-owned. The screen is still
shared: raster spans/bitmap rows and individual pixel updates are protected;
sprite save/draw and restore calls are protected as complete small operations.
Text and compound shapes can interleave between primitives. Use separate
screen regions or application synchronization for overlapping artwork.

Caller-owned buffers, mutable service state, sprite lifetimes and multi-call
operations remain the caller's responsibility. Never free a buffer/context,
close a handle, or unregister code while another thread still intends to use
it. `readdir` returns storage within its `DIR`; copy it before another thread
reads the same directory. Thread-safe kernel calls are not universally safe
from interrupt callbacks: callbacks may set events or resume a suspended
thread, but must not block, do disk I/O, remove timers from the active chain,
or explicitly enable interrupts.

## Events

```c
yos_event_t *ready = yos->create_event(NULL);
if (ready) {
    yos->set_event(ready, YOS_EVENT_SET);
    yos->set_event(ready, YOS_EVENT_RESET);
    yos->destroy_event(ready);
}
```

The owner argument is an opaque owner pointer; pass `NULL` unless kernel-level
code has a valid process owner. `set_event` validates that the object is still
registered and returns it, or `NULL` for an invalid object.

The scheduler understands waiting-thread event arrays internally, but ABI 1
does not publish a function that places a thread into that waiting state.
Events are therefore signalling/state objects for now, not a complete public
blocking primitive.

## Threads

New threads begin suspended and need a process handle:

```c
static void worker(void) { for (;;) { /* work */ } }

yos_thread_t *thread = yos->create_thread(worker, 256, process);
if (thread)
    yos->resume_thread(thread);
```

`suspend_thread(thread)` yields when suspending the current thread;
`resume_thread(thread)` moves a suspended thread to the runnable queue;
`exit_thread(thread)` marks a runnable thread for scheduler cleanup. There is
no public current-process getter or thread join in ABI 1. A process returned by
`create_process` or `load_process` supplies the required process handle; code
inside an ordinarily loaded process cannot yet create an additional owned
thread without receiving that handle from its launcher.

## Processes

Create a process around a resident entry function:

```c
static void child(void) { /* ... */ }

yos_process_t *process = yos->create_process("child", child, 256);
```

This allocates the process and its initial runnable thread. It does not copy
or relocate code; the entry function must remain valid. For a disk image use:

```c
yos_process_t *process = yos->load_process("APP.SYS");
if (!process)
    failure = *yos->process_load_error;
```

`exit_process()` terminates the calling thread. Its process is reclaimed only
after its last thread exits.
There is no wait call, parent relationship, or status return. Calling
`create_process` or `load_process` from a process does not make it the parent;
the new process object has a null owner and no creator field.

Next: [Files, Input, and Graphics](FILES-INPUT-AND-GRAPHICS.md).
