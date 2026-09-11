# Cleaning Up Resources

This chapter explains what happens when threads and processes terminate, and how `yos` reclaims what they owned.

## Why Cleanup Matters

On a 48K machine, memory leaks and orphaned objects quickly break the system. `yos` therefore treats cleanup as a core kernel responsibility, not as an optional application concern.

The cleanup model is based on **ownership** (see [Resource Accounting](RESOURCE-ACCOUNTING.md)):

- resources are system objects (`sysobj_t`) or heap blocks (`block_t`)
- each carries an `owner`
- objects are tracked in per-type linked lists, blocks in their heap
- when an owner dies, everything it owns is released

## Who Does the Work

Nothing is freed at the moment a thread calls `thread_exit`. That routine only moves the thread to the terminated list and halts, because the thread is still running on the very stack that has to be freed. The actual reclamation runs inside the scheduler on the next 50 Hz tick, in `__thread_cleanup_terminated` (`kernel/_thread_cleanup_terminated.s`), called from `__thread_select_next` before any thread is chosen.

```
interrupt
  └─ __thread_robin
       ├─ save current context
       ├─ __tmr_chain
       ├─ __thread_select_next
       │    ├─ __thread_cleanup_terminated      ← this chapter
       │    │     for each terminated thread t (except thread_current):
       │    │        mem_free_owner(__heap, t)   free the stack
       │    │        so_destroy(terminated, t)   free the thread object
       │    │        process_reap(t->process)
       │    ├─ wake waiting threads
       │    └─ pick next runnable
       └─ restore next context
```

`thread_current` is skipped because the interrupt that runs the cleanup may itself be executing on that thread's stack (the thread called `thread_exit` and halted). It is collected one tick later, once another thread is current.

## Thread Cleanup

For each terminated thread:

1. `mem_free_owner(__heap, thread)` frees every user-heap block whose owner is the thread. `thread_create` allocates the stack with the thread as owner, so this releases the stack (and any other block the thread was made owner of).
2. `so_destroy(&thread_first_terminated, thread)` unlinks the 24-byte object and returns it to `__sys_heap`.
3. `process_reap(thread->process)` is called, which may or may not do anything (next section).

## Process Reaping

`process_reap` (`kernel/process_reap.s`) is the owner-based sweep. It runs inside a critical section and begins with `_process_has_threads`, which scans the suspended, running, waiting and terminated lists for any thread whose `process` field points at this process. If one exists the process is still alive and `process_reap` returns immediately. Otherwise, in order:

| Step | Routine | What is released |
|---|---|---|
| 1 | — | `main_thread` is cleared |
| 2 | `__process_find_owned(__evt_first, p)` + `evt_destroy` | every event owned by the process |
| 3 | `__process_find_owned(__tmr_first, p)` + `tmr_uninstall` | every timer owned by the process |
| 4 | `__process_find_owned(__svc_first, p)` + `svc_unregister` | every service owned by the process |
| 5 | `mem_free_owner(__heap, p)` | every user-heap block owned by the process — the loaded XPRG image first of all |
| 6 | `so_destroy(&process_first, p)` | the 15-byte process object itself |

Each "find and destroy" loop restarts from the head of its list after every hit, so it is safe against the list being relinked underneath it.

## What Is *Not* Reclaimed

Ownership is only as good as the owner you pass, and the public `yos_t` adapters pass `NONE`:

- `allocate_memory(size)` allocates with owner `NONE`. Free it yourself.
- `create_timer(hook, ticks)` and `register_service(name, table)` create kernel-owned objects. Destroy or unregister them before the process exits, or they will outlive it and point at freed memory.
- `create_event(owner)` does take an owner; pass your `yos_process_t *` and the event is reaped with the process.
- Threads created with `create_thread(entry, stack, process)` get the process as their `process` field, so they keep the process alive and are reclaimed through the thread path above; their stacks are owned by the thread, not the process.

Kernel-owned objects — the clock and keyboard timers, the `yos` and `gpx` services — are `NONE`-owned by design and are never reaped.

## Example Timeline

A process `count` with one thread returns from its entry function:

```
tick N     startup stub → thread_exit(t): t moved to terminated list, HALT
tick N+1   scheduler: t == thread_current, skipped; another thread runs
tick N+2   scheduler: t freed (stack + object); process_reap(count):
           no threads left → events, timers, services, image, process freed
```

If `count` had spawned a second thread that is still running, `process_reap` returns at tick N+2 without touching anything, and the whole sweep happens when that second thread terminates.

## What To Keep In Mind While Developing

- Always set the correct owner when creating a resource from kernel code.
- From application code, unregister services and destroy timers explicitly before the last thread returns.
- Never free a thread's stack or a loaded image manually; the kernel owns both.
- A thread that must block for another should `suspend_thread` itself; there is no join, so the exiting thread has to `resume_thread` its waiter before returning.

## Related Chapters

- [Resource Accounting](RESOURCE-ACCOUNTING.md)
- [Memory Management](MEMORY-MANAGEMENT.md)
- [Threads](THREADS.md)
- [Processes](PROCESSES.md)
