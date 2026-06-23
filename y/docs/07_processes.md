# Processes

A **process** is the OS-level container for a running program. You can think of it as a named group that owns resources and has at least one thread of execution. When you launch a program, *yos* creates a `process_t` record, allocates a main thread for it, and starts the thread running.

## The Process Structure

```c
typedef struct process_s {
    sysobj_t hdr;               /* list link + owner (must be first) */
    uint8_t  pflags;            /* process flags (see below) */
    char     pname[MAX_PNAME_LEN]; /* name, max 7 chars + NUL */
    thread_t *main_thread;      /* the thread created at launch */
} process_t;
```

`MAX_PNAME_LEN` is 8, so process names can be at most 7 characters long (leaving room for the null terminator). The name is for identification and debugging; it has no effect on scheduling.

### Process Flags

| Constant | Value | Meaning |
|---|---|---|
| `PROCESS_INTERNAL` | `0x01` | OS-internal process; not a user program |

Currently only one flag is defined. The flag field is reserved for future expansion (e.g. a process priority level or security domain).

### The Main Thread

Every process has exactly one **main thread** (`main_thread`). It is created automatically by `process_start` and is the entry point for the process. A process may create additional threads during its lifetime using `thread_create`, but those threads are not stored in `process_t` directly — they are tracked through the global thread queues and by their `owner` field, which points back to the `process_t`.

## Starting a Process

```c
process_t *process_start(
    char *pname,            /* process name (max 7 chars) */
    void (*entry_point)(),  /* function the main thread will run */
    size_t stack_size       /* stack size in bytes for the main thread */
);
```

`process_start` performs the following steps:

1. Allocates a `process_t` on the OS heap via `so_create`, inserting it into the global `process_first` list.
2. Copies `pname` into `p->pname`.
3. Calls `thread_create(entry_point, stack_size, p)` to allocate the main thread and its stack. The `process_t *` is passed as the thread's `owner`.
4. Calls `thread_resume(p->main_thread)` to move the thread to the running queue. The thread will be scheduled at the next 50 Hz interrupt.

```c
/* Launch the shell with a 1 KB stack */
process_t *p = process_start("ysh", ysh, 1024);
if (!p) {
    /* handle failure: not enough memory */
}
```

After `process_start` returns, the calling code and the new process's main thread both run concurrently (once the scheduler is active). The caller is not blocked.

## Exiting a Process

```c
void process_exit();
```

`process_exit` is the clean way to terminate the current process. It:

1. Retrieves the current process via `thread_current->process`.
2. Calls `_process_cleanup(proc)` to release process-owned resources.
3. Calls `so_destroy` to remove the `process_t` from the process list and free it.

> **Note:** `process_exit` does *not* yet terminate child threads explicitly — that is handled by the resource accounting cleanup pass, which walks every resource list and frees anything whose `owner` matches the dying process. See Chapter 4 for details.

If a process's main thread function simply `return`s, the startup stub (written by `thread_create`) calls `thread_exit` automatically. This cleans up the main thread, but you should call `process_exit` explicitly if the process owns other resources that need release.

## Relationship Between Processes and Threads

```
process_t "ysh"
    ├── main_thread: thread_t (ysh entry function)
    └── [child threads created by ysh, tracked via owner field]
              ├── thread_t (worker A, owner = process_t "ysh")
              └── thread_t (worker B, owner = process_t "ysh")
```

All threads with `owner == process_t *` belong to that process and will be cleaned up when the process exits. Child threads are not stored inside `process_t` — they live in the global thread queues (`thread_first_running`, etc.) and are found via the `owner` field during cleanup.

## OS System Process

*Yos* itself is not represented as a process. The kernel runs before the scheduler is active, using the dedicated `__sys_stack`. Once `main()` installs the thread scheduler and returns, the kernel stack is effectively abandoned and the system lives entirely in thread stacks from that point on.

## A Complete Example

```c
#include <kernel/process.h>
#include <kernel/thread.h>

/* A small worker that counts to 100 then exits */
void counter_main() {
    for (int i = 0; i < 100; i++) {
        /* do work */
    }
    /* returning from main thread function calls thread_exit automatically */
}

/* Launched from the shell or another process */
void launch_counter() {
    process_t *p = process_start("count", counter_main, 256);
    if (!p) {
        /* handle allocation failure */
        return;
    }
    /* counter_main now runs concurrently */
}
```

## Tips and Limitations

- **Process names are at most 7 characters.** Longer strings will be silently truncated by `strcpy` with no bounds check — keep names short.
- **There is currently no inter-process isolation.** All processes share the same flat 64 KB address space. A buggy process can overwrite the memory of any other process or the OS itself. This is inherent in the ZX Spectrum's architecture.
- **Stack size must be sufficient for all nested calls.** Include headroom for the 22-byte context the scheduler saves on the thread's stack at every 50 Hz tick, plus all the C function frames the thread will call.
- **`process_exit` should be called before the process's last thread returns** if the process owns non-thread resources (e.g., timers or events it created). Otherwise those resources will be cleaned up lazily the next time the OS walks the resource lists.
