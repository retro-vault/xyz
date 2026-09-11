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

The raw `allocate_memory` and `free_memory` table entries are useful for a
custom allocator, but do not mix raw and libc pointers:

```c
void *raw = yos->allocate_memory(40);
if (raw)
    yos->free_memory(raw);       /* not free(raw) */
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
public timer adapter is kernel-owned in ABI 8, so explicitly destroy it.

## Critical sections

Use the nestable pair only around the smallest shared update:

```c
yos->enter_critical_section();
shared_head = new_head;
yos->leave_critical_section();
```

The first call disables interrupts; the final matching leave enables them.
Every path must balance the calls. Do not perform filesystem I/O, wait, or run
large loops while interrupts are disabled—the 50 Hz scheduler, input scans,
and timers all depend on them.

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

The scheduler understands waiting-thread event arrays internally, but ABI 8
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
no public current-process getter or thread join in ABI 8. A process returned by
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

`exit_process()` terminates the process represented by the current thread.
There is currently no wait call, parent relationship, or status return.

Next: [Files, Input, and Graphics](FILES-INPUT-AND-GRAPHICS.md).
