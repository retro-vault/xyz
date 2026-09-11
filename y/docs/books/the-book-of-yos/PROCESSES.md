# Processes

A **process** is the OS-level container for a running program. You can think of it as a named group that owns resources and has at least one thread of execution. When you launch a program, *yos* creates a `process_t` record, allocates a main thread for it, and starts the thread running.

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

Process names hold at most 7 characters plus the terminator. The name is for identification and debugging; it has no effect on scheduling. `pflags` is always written as `0` by `process_start`; no flag is currently defined.

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
yos_process_t *process = yos->load_process("editor.sys");
if (!process) {
    uint8_t reason = *yos->process_load_error;   /* YOS_PROCESS_LOAD_* */
    /* report or handle reason */
}
```

The file must be an XPRG version 1 *process* image containing a relocatable XL payload (see [Program and Service Images](PROGRAM-IMAGES.md)). `process_load` (`kernel/process_load.s`) reads the 64-byte descriptor onto its own stack, verifies the magic, version, kind, required YOS ABI, payload CRC-32, XL header bounds and entry point, allocates the image from `__heap`, applies every XL relocation in place, and calls `process_start` with the descriptor's name, entry point and `stack size + CONTEXT_SIZE`. Finally it transfers ownership of the image block to the new process so it is freed when the process is reaped. Service-kind XPRG images are rejected with `YOS_PROCESS_LOAD_NOT_PROCESS`.

`process_load_error` points at the kernel's `_process_last_error` byte; it is cleared at the start of every `load_process` call.

The ROM invokes the same loader for `shell.sys` on the current esxDOS drive (`kernel/boot_shell.s`) before enabling scheduler interrupts.

## Exiting a Process

```c
void process_exit(void);
```

`process_exit` (`kernel/process_exit.s`) is small: it reads `thread_current` and, if there is one, jumps to `thread_exit` for it. That moves the calling thread to the terminated queue and halts; everything else happens on the next scheduler tick:

1. `__thread_cleanup_terminated` frees the thread's stack and object.
2. It then calls `process_reap` for the thread's process.
3. `process_reap` checks `_process_has_threads`, which scans the suspended, running, waiting and terminated lists. If any other thread still references the process, the process survives this tick. Otherwise `process_reap` clears `main_thread`, destroys every event, timer and service owned by the process, frees every `__heap` block owned by it — including a loaded XPRG image — and removes the `process_t` from `process_first`.

So `process_exit` terminates the *calling* thread only. A process with several threads is reaped when its last thread terminates, whichever one that is. If a process's main thread function simply `return`s, the startup stub reaches `thread_exit` and the same path runs — calling `exit_process` explicitly is equivalent.

## Relationship Between Processes and Threads

```
process_t "shell"
    ├── main_thread: thread_t (entry function)          process = "shell"
    └── [child threads created by the process]
              ├── thread_t (worker A)                   process = "shell"
              └── thread_t (worker B)                   process = "shell"
```

Threads live in the global queues (`thread_first_running`, ...) and are associated with a process through their `process` field. `_process_has_threads` scans all four lists for that field to decide whether a process can be reaped.

## The Kernel Is Not a Process

*Yos* itself is not represented as a process. `main` runs on the dedicated `__sys_stack` with interrupts disabled, loads the initial process, installs the IM2 scheduler and then idles in a `HALT` loop. The first interrupt finds `thread_current == NULL`, saves nothing, and dispatches the first runnable thread; from then on the kernel stack is never used again. Kernel-owned objects (the clock and keyboard timers, the `yos` and `gpx` services) have owner `NONE` and are never reaped.

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

- **Process names are at most 7 characters.** `process_start` copies the name until the terminator without bounds checking; `process_load` truncates XPRG names (up to 15 characters) to 7 before calling it.
- **There is no inter-process isolation.** All processes share the same flat 64 KB address space. A buggy process can overwrite the memory of any other process or the OS itself. This is inherent in the ZX Spectrum's architecture.
- **Stack size must be sufficient for all nested calls.** Include headroom for the 22-byte context the scheduler saves on the thread's stack at every 50 Hz tick, plus all the function frames the thread will call.
- **Owner matters for reclamation.** Events, timers and services created with the process as owner are released when the process is reaped; memory from `allocate_memory` has no owner and is not.
