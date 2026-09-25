# Processes

A **process** is the OS-level container for a running program: a named
group that owns resources and normally has one or more threads. When you
launch a program, *yos* creates a `process_t`, allocates a main thread, and
starts it. The same compact record is also reused for a library ownership
object, which carries a reference count but never a thread of its own.

## The process structure

The process object is 17 bytes on the kernel heap:

```c
typedef struct process_s {
    sysobj_t hdr;               /*  0: list link + owner (must be first) */
    uint8_t  pflags;            /*  5: process flags        PROCESS_FLAGS */
    char     pname[8];          /*  6: name, 7 chars + NUL  PROCESS_NAME */
    thread_t *main_thread;      /* 14: thread created at launch  PROCESS_MAIN_THREAD */
    uint8_t bank;               // 16: image bank, FF for common entry
} process_t;
```

Process names are bounded to 7 characters plus the terminator. The name
exists for debugging, not for scheduling or library identity. Normal
processes carry `pflags = 0`. [Libraries](LIBRARIES.md) reuse this same
record without a thread: flags 1/3 mark private/shared libraries, bytes
6–8 hold the service pointer and image ABI, the word at offset 14 becomes
their reference count, and byte 16 is the resident image bank.

There is deliberately no parent field. `hdr.owner` is `NONE` for ordinary
process and library objects — that field is generic resource ownership,
not a parent process. Calling `create_process` or `load_process` from
another process establishes no ancestry, no wait relationship, and no
exit-status channel.

### The main thread

Every process starts with exactly one **main thread** (`main_thread`),
created by `process_start`, which serves as the process's entry point. A
process may create additional threads during its lifetime with
`create_thread`, but those threads are not stored in `process_t` at all —
they live in the global thread queues and point back to the process
through their own `process` field.

## Starting a process

```c
process_t *process_start(
    char *pname,            /* process name (max 7 chars) */
    void (*entry_point)(),  /* function the main thread will run */
    uint16_t stack_size     /* stack size in bytes for the main thread */
);
```

`process_start` (`kernel/process_start.s`) performs the following steps:

1. Allocates a `process_t` from `__sys_heap` through `so_create`,
   inserting it at the head of the global `process_first` list with owner
   `NONE`.
2. Copies `pname` into `pname` and clears `pflags`.
3. Calls `thread_create(entry_point, stack_size, p)` to allocate the main
   thread and its stack, then stores the thread in `main_thread` and the
   process in the thread's own `process` field.
4. Calls `thread_resume` on the main thread, moving it to the running
   queue. It will be scheduled at the next 50 Hz interrupt.

If the thread cannot be created, the process object is destroyed again
and `NULL` is returned.

```c
yos_t *yos = (yos_t *)query_service("yos");

/* Launch a worker with a 1 KB stack */
yos_process_t *p = yos->create_process("worker", worker_main, 1024);
if (!p) {
    /* handle failure: not enough memory */
}
```

Once `create_process` returns, the calling code and the new process's
main thread run concurrently. The caller is never blocked waiting for it.

> **Stack size.** `create_process` passes `stack_size` straight through to
> `thread_create`, so the 22-byte scheduler context comes out of the
> number you give it. Only `load_process` adds that context on top of the
> descriptor's declared stack size automatically.

## Loading a process from disk

Applications normally start programs from disk rather than from a
function pointer:

```c
yos_process_t *process = yos->load_process("editor.prc");
if (!process) {
    uint8_t reason = *yos->process_load_error;   /* YOS_PROCESS_LOAD_* */
    /* report or handle reason */
}
```

The file must be an XPRG version 1 *process* image containing a
relocatable XL payload (see
[Program and Service Images](PROGRAM-IMAGES.md)). `process_load`
(`kernel/process_load.s`) reads the 64-byte descriptor onto its own stack
and the XL payload into temporary loader memory, verifies the magic,
version, kind, required YOS ABI, CRC-32, XL bounds, and entry point, then
relocates the code in place inside the same buffer. It splits that owned
allocation twice — first freeing the trailing relocation records, then the
leading XL header — before creating the main thread, so no second code
allocation or copy is ever needed. `process_start` receives the
descriptor's name, the relocated entry point, and
`stack size + CONTEXT_SIZE`; ownership of only the compact resident code
block transfers to the process. Service-kind XPRG images are rejected with
`YOS_PROCESS_LOAD_NOT_PROCESS`.

The retained block comes from the configured `0xC000`–`0xFFFF` bank
arenas, and the process and its threads record which logical bank it
landed in.

`process_load_error` points at `_process_last_error`, a fixed cell whose
value is saved and restored per thread. Every completed process or
library load writes its result there: zero on success, one of the error
codes below otherwise. A concurrent or recursive load simply returns
BUSY; the initiating call still completes synchronously.

The ROM uses this same loader for `shell.sys` on the current esxDOS drive
(`kernel/boot_shell.s`), before it arms IM2 scheduling. Each firmware call
masks interrupts while divIDE has the YOS ROM paged out.

## Exiting a process

```c
void process_exit(void);
```

`process_exit` (`kernel/process_exit.s`) is small: it reads
`thread_current` and, if one exists, jumps to `thread_exit` for it. That
moves the calling thread to the terminated queue and halts — everything
else happens on the next scheduler tick:

1. `__thread_cleanup_terminated` frees the thread's stack and object.
2. It then calls `process_reap` for the thread's process.
3. `process_reap` checks all four thread queues. If any thread still
   references the process, it survives. Otherwise, cleanup destroys its
   owned events, timers, and services, releases all library references,
   frees its user-heap blocks (including the loaded image), and removes
   the process object entirely. Libraries that reach zero references are
   reaped in the same pass.

So `process_exit` only terminates the *calling* thread. A process with
several threads is reaped once its last thread terminates, whichever one
that happens to be. If a process's main thread function simply `return`s,
the startup stub reaches `thread_exit` and the same path runs — calling
`exit_process` explicitly amounts to the same thing.

## Relationship between processes and threads

| Object | Role | `process` field |
|---|---|---|
| `process_t` `"shell"` | The process | — |
| `thread_t` main thread | Entry function | `"shell"` |
| `thread_t` worker A | Child thread created by the process | `"shell"` |
| `thread_t` worker B | Child thread created by the process | `"shell"` |

Threads live in the global queues (`thread_first_running`, and so on) and
are associated with a process only through their `process` field.
`_process_has_threads` scans all four lists for that field to decide
whether a process can be reaped.

## The kernel is not a process

*Yos* itself is not represented as a process. `main` runs on the dedicated
`__sys_stack` with interrupts disabled, loads the initial process,
installs the IM2 scheduler, and then idles in a `HALT` loop. The first
interrupt finds `thread_current == NULL`, saves nothing, and dispatches
the first runnable thread; from that point on, the kernel stack is never
used again. Kernel-owned objects — the clock, keyboard, and mouse timers,
and the single `yos` service, which now also exposes the graphics API —
carry owner `NONE` and are never reaped.

## A complete example

```c
#include <yos.h>

static yos_t *yos;

/* A small worker that counts to 100 then exits */
void counter_main(void) {
    for (int i = 0; i < 100; i++) {
        /* do work */
    }
    /* returning from the main thread function terminates the thread;
       the process is reaped on the next tick */
}

/* Launched from the shell or another process */
void launch_counter(void) {
    yos_process_t *p = yos->create_process("count", counter_main, 256);
    if (!p) {
        /* handle allocation failure */
        return;
    }
    /* counter_main now runs concurrently */
}
```

## Tips and limitations

- **Process names are at most 7 characters.** `process_start` uses the
  bounded `__string_copy`; `process_load` passes an XPRG name of up to 15
  characters, and the process record safely retains just the first seven.
- **There is no protection boundary.** Banking isolates visibility, not
  privilege — a buggy process can still overwrite common memory or its own
  selected 16 KiB bank.
- **Stack size must cover every nested call.** Budget headroom for the
  22-byte context the scheduler saves on the thread's stack at every
  50 Hz tick, on top of every function frame the thread will actually
  call.
- **Owner matters for reclamation.** Process-owned events, services,
  library references, and `allocate_memory` blocks are all released once
  the process is reaped. Public timers, however, remain kernel-owned and
  must be destroyed explicitly.
