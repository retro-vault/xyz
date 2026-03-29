# Threads

A **thread** is an independent flow of execution. Multiple threads share the same address space but each has its own stack and its own set of CPU registers. *Yos* implements preemptive, round-robin multithreading: the 50 Hz hardware interrupt switches between runnable threads automatically, without any thread needing to cooperate.

## The Thread Structure

Every thread is represented by a `thread_t` object allocated on the OS heap:

```c
typedef struct thread_s {
    sysobj_t hdr;           /* list link + owner (must be first) */
    uint16_t sp;            /* saved stack pointer */
    uint8_t  startup[10];   /* small code stub (CALL + JP opcodes) */
    event_t  **wait;        /* array of events this thread waits on */
    uint8_t  num_events;    /* number of events in the wait array */
    uint8_t  state;         /* current thread state */
    thread_t **joined;      /* other threads waiting for this one */
    void     *process;      /* parent process */
} thread_t;
```

Key points for a junior developer:

- **`sp`** — when a thread is not running, the CPU's stack pointer is saved here. When the scheduler switches back to this thread, it restores `SP` from this field.
- **`startup[10]`** — a 10-byte machine-code stub that is written into the struct at creation time. It calls the thread's entry function and then calls `thread_exit()` when the function returns.
- **`state`** — one of five values (see below).

## Thread States

A thread is always in exactly one of the following states:

| State constant | Value | Meaning |
|---|---|---|
| `THREAD_STATE_SUSPENDED` | 0 | Created but not yet started, or explicitly paused |
| `THREAD_STATE_RUNNING` | 1 | Eligible to run; the scheduler may pick it at any 50 Hz tick |
| `THREAD_STATE_WAITING` | 2 | Blocked waiting for one or more events to be signalled |
| `THREAD_STATE_JOINED` | 3 | Waiting for another thread to finish |
| `THREAD_STATE_TERMINATED` | 4 | Finished; resources not yet freed |

There are four corresponding linked lists:

```c
thread_t *thread_first_suspended;   /* SUSPENDED threads */
thread_t *thread_first_running;     /* RUNNING threads   */
thread_t *thread_first_waiting;     /* WAITING threads   */
thread_t *thread_first_terminated;  /* TERMINATED threads */
```

`thread_current` points to whichever thread is currently executing.

## Creating and Starting a Thread

```c
thread_t *thread_create(
    void (*entry_point)(),  /* function the thread will run */
    uint16_t stack_size,    /* stack size in bytes          */
    void *process           /* owning process (or NULL)     */
);
```

`thread_create` allocates a `thread_t` and a stack of `stack_size` bytes from the user heap. It writes a 10-byte startup stub into the struct and places the stub's address as the initial return address on the stack. The thread is created in the `SUSPENDED` state — it does not run until you call `thread_resume`.

```c
/* Create a thread with a 512-byte stack */
thread_t *t = thread_create(my_function, 512, thread_current->process);
if (!t) { /* handle allocation failure */ }

/* Move it to the RUNNING queue so the scheduler picks it up */
thread_resume(t);
```

> **Stack size guidance:** 512 bytes is a reasonable minimum. Bear in mind that each 50 Hz interrupt saves 22 bytes of register context on the thread's own stack, and any C functions the thread calls push their own frames on top. If the thread calls deeply nested functions, increase the stack accordingly.

## The Startup Stub

When `thread_create` sets up a new thread, it writes these 10 bytes into `thread_t.startup[]`:

```
Offset  Bytes  Instruction
0       CD lo hi  CALL entry_point     ; call the user's function
3       21 lo hi  LD HL, <thread_t*>   ; load thread pointer into HL
6       E5        PUSH HL              ; push it as argument
7       CD lo hi  CALL thread_exit     ; exit the thread (never returns)
```

The first time the scheduler dispatches the thread, `reti` inside the context-restore path jumps to this stub. The stub calls `entry_point`. When `entry_point` returns, the stub calls `thread_exit` to cleanly remove the thread from the running queue. **A thread function should simply return when it is done** — there is no need to call `thread_exit` manually.

## Context Switching: How It Works

The 50 Hz hardware interrupt fires at `RST 0x38`. *Yos* installs `__thread_robin` as the handler for this vector (in `main()`, via `sys_vec_set`). Every 20 ms the following sequence runs:

### Step 1 — Save the current thread's context

The Z80's interrupt mechanism automatically pushes the interrupted PC onto the stack. `__thread_robin` then pushes all remaining registers in this order:

```
Pushed last → lower address (SP points here after save)
    HL'  DE'  BC'       ← alternate register set
    AF'                  ← alternate accumulator + flags
    IY   IX              ← index registers
    DE   BC   HL   AF    ← main register set
    [interrupted PC]     ← pushed automatically by the CPU
```

This block is exactly `CONTEXT_SIZE = 22` bytes. The current value of `SP` (pointing at the bottom of this block) is stored in `thread_current->sp`.

### Step 2 — Run pending timers

`_tmr_chain()` is called to fire any timer hooks that have counted down to zero (see Chapter 9 for details).

### Step 3 — Select the next thread

`_thread_select_next()` implements simple round-robin scheduling:

- If the current thread is still `RUNNING` and is not the last in the list, return the *next* thread in the list.
- Otherwise return the *first* thread in the running list (wraps around).
- Any threads in the `WAITING` state that have had their events signalled are moved back to `RUNNING` before the selection.

### Step 4 — Restore the next thread's context

The new thread's `SP` is loaded from its `thread_t.sp` field, then all registers are popped in reverse order. `reti` pops the PC and re-enables interrupts. The new thread resumes exactly where it was interrupted.

### Stack layout visualised

```
High address (stack bottom for this thread)
┌──────────────────────┐  ← thread's stack_base + stack_size
│                      │
│ thread's local data  │  (function call frames, local variables)
│                      │
├──────────────────────┤  ← SP at the moment of interrupt
│ [interrupted PC]  2B │  pushed automatically by Z80
│ AF               2B  │
│ HL               2B  │
│ BC               2B  │
│ DE               2B  │
│ IX               2B  │
│ IY               2B  │
│ AF'              2B  │
│ BC'              2B  │
│ DE'              2B  │
│ HL'              2B  │  ← thread_t.sp saved here (22 bytes above pre-interrupt SP)
└──────────────────────┘
Low address (stack top)
```

## Thread API

### `thread_resume(t)`

Moves thread `t` from the `SUSPENDED` queue to the `RUNNING` queue. Use this to start a newly created thread or to unpause one that was suspended.

```c
thread_t *t = thread_create(worker, 512, proc);
thread_resume(t);   /* t will now be scheduled */
```

### `thread_suspend(t)`

Moves thread `t` from `RUNNING` to `SUSPENDED`. The thread stops being scheduled until someone calls `thread_resume` on it again. If you call `thread_suspend` on the *currently running* thread, the function also executes a `halt` instruction to yield immediately to the next interrupt.

```c
/* Pause our own thread */
thread_suspend(thread_current);
/* When resumed, execution continues here */
```

### `thread_exit(t)`

Moves thread `t` to the `TERMINATED` queue and yields. Normally called automatically by the startup stub when a thread's entry function returns. You should not need to call this directly.

### `thread_wait4events(events, num_events)`

Blocks the current thread until at least one of the specified events is signalled. The thread moves to the `WAITING` queue and will not consume any CPU time until the scheduler detects a signalled event.

```c
event_t *evts[2] = { event_a, event_b };
thread_wait4events(evts, 2);   /* sleep until event_a or event_b fires */
```

### `thread_join(t)`

Blocks the calling thread until thread `t` terminates. Useful for waiting on a worker thread to complete its task before proceeding.

## Events

An event is a lightweight signalling object with two states: `nonsignaled` and `signaled`.

```c
/* Create an event (must be destroyed when no longer needed) */
event_t *e = evt_create(owner);

/* Signal it (e.g. from a timer callback or interrupt handler) */
evt_set(e, signaled);

/* Reset it */
evt_set(e, nonsignaled);

/* Block a thread until e is signalled */
event_t *evts[] = { e };
thread_wait4events(evts, 1);

/* Destroy when done */
evt_destroy(e);
```

The scheduler checks all threads in the `WAITING` state on every 50 Hz tick and moves any thread whose event has been signalled back to the `RUNNING` queue.

## A Complete Example

```c
#include <kernel/thread.h>
#include <kernel/process.h>

/* Two independent threads draw to the screen */

void draw_left() {
    while (1) {
        /* draw something on the left half */
    }
}

void draw_right() {
    while (1) {
        /* draw something on the right half */
    }
}

void my_app() {
    thread_t *left  = thread_create(draw_left,  512, thread_current->process);
    thread_t *right = thread_create(draw_right, 512, thread_current->process);

    thread_resume(left);
    thread_resume(right);

    /* Both threads now run concurrently. my_app continues here.
       All three threads share the CPU via the 50 Hz scheduler. */
}
```

## Common Pitfalls

**Stack overflow.** If a thread uses more stack than was allocated, it silently overwrites adjacent heap blocks. There are no guard pages. Symptoms: corrupted data in seemingly unrelated variables, random crashes. Increase `stack_size` if in doubt.

**Shared data.** Multiple threads accessing the same global variable without protection will race. Use `ir_disable()` / `ir_enable()` to create a critical section:

```c
ir_disable();
shared_counter++;   /* protected from interrupt-driven context switch */
ir_enable();
```

**Long interrupt latency.** Holding interrupts disabled for more than a few microseconds will starve the scheduler and all timer callbacks. Keep critical sections as short as possible.

**Do not free a thread's stack manually.** The OS frees it automatically when the thread is cleaned up via resource accounting.
