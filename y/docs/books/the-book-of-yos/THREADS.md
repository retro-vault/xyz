# Threads

A **thread** is an independent flow of execution. Multiple threads share the same address space but each has its own stack and its own set of CPU registers. *Yos* implements preemptive, round-robin multithreading: the 50 Hz hardware interrupt switches between runnable threads automatically, without any thread needing to cooperate.

## The Thread Structure

Every thread is a 24-byte `thread_t` object allocated on the kernel heap (`__sys_heap`). The assembly modules address its fields through the `.equ` offsets shown here:

```c
typedef struct thread_s {
    sysobj_t hdr;           /*  0: list link + owner (must be first) */
    uint16_t sp;            /*  4: saved stack pointer          THREAD_SP */
    uint8_t  startup[9];    /*  6: startup stub (see below) */
    uint8_t  load_error;    /* 15: saved loader status      THREAD_LOAD_ERROR */
    event_t  **wait;        /* 16: array of events to wait on   THREAD_WAIT */
    uint8_t  num_events;    /* 18: entries in wait[]            THREAD_NUM_EVENTS */
    uint8_t  state;         /* 19: current thread state         THREAD_STATE */
    int16_t  error_number;  /* 20: saved kernel errno       THREAD_ERRNO */
    void     *process;      /* 22: owning process               THREAD_PROCESS */
} thread_t;
```

Key points:

- **`sp`** — when a thread is not running, the CPU's stack pointer is saved here. When the scheduler switches back to this thread, it restores `SP` from this field.
- **`startup[9]`** — calls the entry function and jumps to `thread_exit` on
  return. The formerly unused tenth byte holds loader status; the formerly
  reserved join word holds kernel errno. Both error fields start at zero.
- **Error fields** — the scheduler saves the live public cells before timer
  callbacks, and restores the next thread's values before returning. The
  object stays 24 bytes and the register context stays 22 bytes.
- **`state`** — one of the values below.
- **`process`** — the real process membership used by cleanup.
  `hdr.owner` is normally zero. Library initialization temporarily places
  its library object there as an allocation/registration owner override,
  without changing real process membership or growing the thread object.

## Thread States

A thread is always in exactly one of the following states:

| State constant | Value | Meaning |
|---|---|---|
| `STATE_SUSPENDED` | 0 | Created but not yet started, or explicitly paused |
| `STATE_RUNNING` | 1 | Eligible to run; the scheduler may pick it at any 50 Hz tick |
| `STATE_WAITING` | 2 | Blocked until one of its events is signalled |
| `STATE_JOINED` | 3 | Reserved; no kernel routine sets it |
| `STATE_TERMINATED` | 4 | Finished; resources not yet freed |

There are four corresponding linked lists plus the current-thread pointer, all in `kernel/_thread_state.s`:

```c
thread_t *thread_current;
thread_t *thread_first_suspended;   /* SUSPENDED threads */
thread_t *thread_first_running;     /* RUNNING threads   */
thread_t *thread_first_waiting;     /* WAITING threads   */
thread_t *thread_first_terminated;  /* TERMINATED threads */
```

Moving a thread between two lists is always done by `__thread_lswitch`, which takes the source and destination heads, the thread, the new state byte and an "immediate" flag. Inside a critical section it unlinks the thread, stores the state, re-links it, and — if the flag is set — executes `HALT` so the next interrupt reschedules at once.

## Creating and Starting a Thread

```c
thread_t *thread_create(
    void (*entry_point)(),  /* function the thread will run */
    uint16_t stack_size,    /* stack size in bytes          */
    void *process           /* owning process (or NULL)     */
);
```

`thread_create` (`kernel/thread_create.s`) allocates a `thread_t` from `__sys_heap` and a stack of `stack_size` bytes from `__heap`, with the *thread* as the stack block's owner. It sets `sp = stack + stack_size - CONTEXT_SIZE`, writes the startup stub, and places the stub's address in the return-address slot of that initial context. The thread is created in the `SUSPENDED` state — it does not run until you call `thread_resume`. If either allocation fails, everything is rolled back and `NULL` is returned.

```c
yos_t *yos = (yos_t *)query_service("yos");

/* Create a thread with a 512-byte stack, owned by this process */
yos_thread_t *t = yos->create_thread(my_function, 512, my_process);
if (!t) { /* handle allocation failure */ }

/* Move it to the RUNNING queue so the scheduler picks it up */
yos->resume_thread(t);
```

> **Stack size guidance:** 512 bytes is a reasonable minimum. Bear in mind that each 50 Hz interrupt saves 22 bytes of register context on the thread's own stack, and any functions the thread calls push their own frames on top. `process_load` adds the 22-byte context to the stack size declared in an XPRG descriptor; `create_thread` does not, so include it yourself.

## The Startup Stub

`_thread_prepare_startup` writes the nine-byte stub and clears loader status:

```
Offset  Bytes     Instruction
0       CD lo hi  CALL entry_point     ; call the user's function
3       21 lo hi  LD HL, <thread_t*>   ; thread pointer is the first argument
6       C3 lo hi  JP thread_exit       ; exit the thread (never returns)
9       00        initial loader status (not executed)
```

The first time the scheduler dispatches the thread, `RETI` at the end of the context-restore path pops the stub's address as the "interrupted PC" and lands in the stub. The stub calls `entry_point`. When `entry_point` returns, the stub loads the thread pointer into `HL` (the `sdcccall(1)` first argument) and jumps to `thread_exit`. **A thread function should simply return when it is done** — there is no need to call `thread_exit` manually.

## Context Switching: How It Works

The scheduler `__thread_robin` (`kernel/_thread_robin.s`) is reached through the IM2 vector at `0x5EFF` on every 50 Hz frame interrupt (see [The Boot Process](BOOT.md)). Every 20 ms the following sequence runs:

### Step 1 — Save the current thread's context

The Z80's interrupt mechanism automatically pushes the interrupted PC onto the stack. `__thread_robin` then pushes all remaining registers in this order:

```
Pushed last → lower address (SP points here after save)
    HL'  DE'  BC'        ← alternate register set
    AF'                  ← alternate accumulator + flags
    IY   IX              ← index registers
    DE   BC   HL   AF    ← main register set
    [interrupted PC]     ← pushed automatically by the CPU
```

This block is exactly `CONTEXT_SIZE = 22` bytes. The current value of `SP` (pointing at the bottom of this block) is stored in `thread_current->sp`. If `thread_current` is `NULL` (the very first interrupt, while the kernel is still in its idle loop) nothing is saved.

### Step 2 — Run pending timers

`__tmr_chain` fires any timer hooks whose countdown reached zero (see [Clock and Timers](CLOCK.md)).

### Step 3 — Select the next thread

`__thread_select_next` (`kernel/_thread_select_next.s`) does three things:

1. Calls `__thread_cleanup_terminated`, which frees the stack and object of every thread on the terminated list (except the one whose stack the interrupt is currently using) and reaps their processes — see [Cleaning Up Resources](CLEANUP-RESOURCES.md).
2. Walks the waiting list. Any thread that has at least one event in its `wait[]` array in the `YOS_EVENT_SET` state is moved back to the running list.
3. Picks the next runnable thread: if `thread_current` is still `RUNNING` and has a successor in the running list, that successor; otherwise the head of the running list (wrap-around).

If no thread is runnable, `__thread_robin` keeps the current one.

### Step 4 — Restore the next thread's context

The new thread's `SP` is loaded from its `thread_t.sp` field, then all registers are popped in reverse order. `EI; RETI` pops the PC and re-enables interrupts. The new thread resumes exactly where it was interrupted (or at its startup stub, the first time).

### Stack layout visualised

```
High address (stack bottom for this thread)
┌──────────────────────┐  ← stack + stack_size
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
│ HL'              2B  │  ← thread_t.sp saved here (22 bytes below pre-interrupt SP)
└──────────────────────┘
Low address (stack top)
```

## Thread API

The kernel routines are `thread_create`, `thread_resume`, `thread_suspend` and `thread_exit`; the `yos_t` table exposes them as `create_thread`, `resume_thread`, `suspend_thread` and `exit_thread`.

### `thread_resume(t)`

Moves thread `t` from the `SUSPENDED` queue to the `RUNNING` queue without yielding. Use this to start a newly created thread or to unpause one that was suspended.

### `thread_suspend(t)`

Moves thread `t` from `RUNNING` to `SUSPENDED` and executes `HALT`, so the next interrupt reschedules immediately. If `t` is the calling thread, execution continues after `thread_suspend` only once somebody calls `thread_resume` on it.

```c
/* Pause our own thread */
yos->suspend_thread(me);
/* When resumed, execution continues here */
```

### `thread_exit(t)`

Moves thread `t` to the `TERMINATED` queue and halts forever; the thread's stack and object are reclaimed by the scheduler on the next tick. Normally reached through the startup stub when a thread's entry function returns. You should not need to call this directly.

### Not (yet) available

There is no `thread_wait4events` or `thread_join`. The `WAITING` state and the
event-wakeup scan exist, but no public routine moves a thread onto that list.
The former reserved join word now stores errno. A thread that needs to block
can suspend itself and have another thread or a short timer hook resume it;
arrange the signal/suspend handshake to avoid a missed wakeup.

## Events

An event is a lightweight signalling object: a 5-byte system object whose single payload byte holds `YOS_EVENT_RESET` (0) or `YOS_EVENT_SET` (1).

```c
/* Create an event (must be destroyed when no longer needed) */
yos_event_t *e = yos->create_event(owner);

/* Signal it (e.g. from a timer callback or interrupt handler) */
yos->set_event(e, YOS_EVENT_SET);

/* Reset it */
yos->set_event(e, YOS_EVENT_RESET);

/* Destroy when done */
yos->destroy_event(e);
```

`set_event` verifies that `e` is on the event list before writing, and returns `NULL` if it is not. Events owned by a process are destroyed automatically when the process is reaped.

## A Complete Example

Thread creation needs the process handle that will own the new threads. ABI 1
does not expose a current-process getter, so this helper is for a launcher that
already has that handle (for example, the code that called `create_process`):

```c
#include <yos.h>

static yos_t *yos;

/* Two independent threads draw to the screen */

void draw_left(void) {
    for (;;) { /* draw something on the left half */ }
}

void draw_right(void) {
    for (;;) { /* draw something on the right half */ }
}

void start_workers(yos_process_t *process) {
    yos = (yos_t *)query_service("yos");

    yos_thread_t *left  = yos->create_thread(draw_left,  512, process);
    yos_thread_t *right = yos->create_thread(draw_right, 512, process);

    if (left)  yos->resume_thread(left);
    if (right) yos->resume_thread(right);

    /* The workers now share this process and the 50 Hz scheduler. */
}
```

## Common Pitfalls

**Stack overflow.** If a thread uses more stack than was allocated, it silently overwrites adjacent heap blocks. There are no guard pages. Symptoms: corrupted data in seemingly unrelated variables, random crashes. Increase `stack_size` if in doubt.

**Shared data.** Multiple threads accessing the same global variable without protection will race. Use `enter_critical_section()` / `leave_critical_section()` to create a critical section:

```c
yos->enter_critical_section();
shared_counter++;   /* protected from interrupt-driven context switch */
yos->leave_critical_section();
```

**Long interrupt latency.** Holding interrupts disabled for more than a few microseconds will starve the scheduler and all timer callbacks. Keep critical sections as short as possible.

**Returning from the process's last thread.** The process itself is reaped once
its last thread terminates. `allocate_memory` records the current process and
is reclaimed then; successful public timers remain kernel-owned and must be
destroyed explicitly.

**Do not free a thread's stack manually.** The OS frees it automatically when the thread is cleaned up via resource accounting.
