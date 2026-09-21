# Processes

A **process** is the OS-level container for a running program: a named group
that owns resources and normally has one or more threads. When you launch a
program, *yos* creates a `process_t`, allocates a main thread, and starts it.
The same compact record is also reused for a library ownership object, which
has a reference count but never a thread.

## The Process Structure

The process object is 15 bytes on the kernel heap:

```c
typedef struct process_s {
    sysobj_t hdr;               /*  0: list link + owner (must be first) */
    uint8_t  pflags;            /*  4: process flags        PROCESS_FLAGS */
    char     pname[8];          /*  5: name, 7 chars + NUL  PROCESS_NAME */
    thread_t *main_thread;      /* 13: thread created at launch  PROCESS_MAIN_THREAD */
} process_t;
```

Process names are bounded to 7 characters plus the terminator. The name is
for debugging, not scheduling or library identity. Normal processes have
`pflags = 0`. [Libraries](LIBRARIES.md) reuse this record without a thread:
flags 1/3 mark private/shared libraries, bytes 5–7 hold the service pointer
and image ABI, and the word at 13 is their reference count.

There is deliberately no parent field. `hdr.owner` is `NONE` for ordinary
process and library objects; it is generic resource ownership, not a parent
process. Calling `create_process` or `load_process` from another process does
not establish ancestry, a wait relationship, or an exit-status channel.

### The Main Thread

Every process starts with exactly one **main thread** (`main_thread`). It is created by `process_start` and is the entry point for the process. A process may create additional threads during its lifetime with `create_thread`, but those threads are not stored in `process_t` — they are tracked through the global thread queues and by their `process` field, which points back to the `process_t`.

## Starting a Process

```c
process_t *process_start(
    char *pname,            /* process name (max 7 chars) */
    void (*entry_point)(),  /* function the main thread will run */
    uint16_t stack_size     /* stack size in bytes for the main thread */
);
```

`process_start` (`kernel/process_start.s`) performs the following steps:

1. Allocates a `process_t` from `__sys_heap` via `so_create`, inserting it at the head of the global `process_first` list with owner `NONE`.
2. Copies `pname` into `pname` and clears `pflags`.
3. Calls `thread_create(entry_point, stack_size, p)` to allocate the main thread and its stack, then stores the thread in `main_thread` and the process in the thread's `process` field.
4. Calls `thread_resume` on the main thread, moving it to the running queue. The thread will be scheduled at the next 50 Hz interrupt.

If the thread cannot be created the process object is destroyed again and `NULL` is returned.

```c
yos_t *yos = (yos_t *)query_service("yos");

/* Launch a worker with a 1 KB stack */
yos_process_t *p = yos->create_process("worker", worker_main, 1024);
if (!p) {
    /* handle failure: not enough memory */
}
```

After `create_process` returns, the calling code and the new process's main thread both run concurrently. The caller is not blocked.

> **Stack size:** `create_process` passes `stack_size` straight to `thread_create`, so the 22-byte scheduler context comes out of the number you give. Only `load_process` adds the context on top of the descriptor's declared stack.

## Loading a Process from Disk

Applications normally start programs from disk rather than from a function pointer:

```c
yos_process_t *process = yos->load_process("editor.prc");
if (!process) {
    uint8_t reason = *yos->process_load_error;   /* YOS_PROCESS_LOAD_* */
    /* report or handle reason */
}
```

The file must be an XPRG version 1 *process* image containing a relocatable XL payload (see [Program and Service Images](PROGRAM-IMAGES.md)). `process_load` (`kernel/process_load.s`) reads the 64-byte descriptor onto its own stack and the XL payload into temporary loader memory, verifies the magic, version, kind, required YOS ABI, CRC-32, XL bounds and entry point, then relocates the code in the existing buffer. It splits that owned allocation twice, freeing the trailing relocation records and then the leading XL header, before creating the main thread. No second code allocation or copy is needed. `process_start` receives the descriptor's name, relocated entry and `stack size + CONTEXT_SIZE`; ownership of only the compact resident code block transfers to the process. Service-kind XPRG images are rejected with `YOS_PROCESS_LOAD_NOT_PROCESS`.

`process_load_error` points at `_process_last_error`, a fixed cell whose value
is saved/restored per thread. Every completed process/library load writes its
result there: zero on success, otherwise an error below. Concurrent/recursive
loads return BUSY; the initiating call completes synchronously.

The ROM invokes the same loader for `op.sys` on the current esxDOS drive
(`kernel/boot_shell.s`) before arming IM2 scheduling. Each firmware call masks
interrupts while divIDE has the YOS ROM paged out.

## Exiting a Process

```c
void process_exit(void);
```

`process_exit` (`kernel/process_exit.s`) is small: it reads `thread_current` and, if there is one, jumps to `thread_exit` for it. That moves the calling thread to the terminated queue and halts; everything else happens on the next scheduler tick:

1. `__thread_cleanup_terminated` frees the thread's stack and object.
2. It then calls `process_reap` for the thread's process.
3. `process_reap` checks all four thread queues. If any thread still
   references the process, it survives. Otherwise cleanup destroys its
   owned events, timers and services, releases all library references,
   frees its user-heap blocks (including the loaded image), and removes
   the process object. Libraries reaching zero references are reaped too.

So `process_exit` terminates the *calling* thread only. A process with several threads is reaped when its last thread terminates, whichever one that is. If a process's main thread function simply `return`s, the startup stub reaches `thread_exit` and the same path runs — calling `exit_process` explicitly is equivalent.

## Relationship Between Processes and Threads

| Object | Role | `process` field |
|---|---|---|
| `process_t` `"shell"` | The process | — |
| `thread_t` main thread | Entry function | `"shell"` |
| `thread_t` worker A | Child thread created by the process | `"shell"` |
| `thread_t` worker B | Child thread created by the process | `"shell"` |

Threads live in the global queues (`thread_first_running`, ...) and are associated with a process through their `process` field. `_process_has_threads` scans all four lists for that field to decide whether a process can be reaped.

## The Kernel Is Not a Process

*Yos* itself is not represented as a process. `main` runs on the dedicated
`__sys_stack` with interrupts disabled, loads the initial process, installs the
IM2 scheduler and then idles in a `HALT` loop. The first interrupt finds
`thread_current == NULL`, saves nothing, and dispatches the first runnable
thread; from then on the kernel stack is never used again. Kernel-owned objects
(the clock, keyboard and mouse timers and the `yos` and `gpx` services) have
owner `NONE` and are never reaped.

## A Complete Example

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

## Tips and Limitations

- **Process names are at most 7 characters.** `process_start` uses the bounded
  `__string_copy`; `process_load` passes an XPRG name of up to 15 characters,
  and the process record safely retains the first seven.
- **There is no inter-process isolation.** All processes share the same flat 64 KB address space. A buggy process can overwrite the memory of any other process or the OS itself. This is inherent in the ZX Spectrum's architecture.
- **Stack size must be sufficient for all nested calls.** Include headroom for the 22-byte context the scheduler saves on the thread's stack at every 50 Hz tick, plus all the function frames the thread will call.
- **Owner matters for reclamation.** Process-owned events, services, library
  references, and `allocate_memory` blocks are released when the process is
  reaped. Public timers remain kernel-owned and must be destroyed explicitly.
