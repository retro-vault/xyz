# Cleaning Up Resources

This chapter explains what happens when threads and processes terminate,
and how `yos` reclaims what they owned.

## Why cleanup matters

On a 48K machine, memory leaks and orphaned objects break the system
quickly. `yos` therefore treats cleanup as a core kernel responsibility,
not as an optional concern left to applications.

The cleanup model rests on **ownership** (see
[Resource Accounting](RESOURCE-ACCOUNTING.md)):

- resources are system objects (`sysobj_t`) or heap blocks (`block_t`)
- each one carries an `owner`
- objects are tracked in per-type linked lists, blocks in their heap
- when an owner dies, everything it owns is released

## Who does the work

Nothing is freed at the moment a thread calls `thread_exit`. That routine
only moves the thread to the terminated list and halts, because the
thread is still running on the very stack that eventually has to be
freed. The actual reclamation happens inside the scheduler on the next
50 Hz tick, in `__thread_cleanup_terminated`
(`kernel/_thread_cleanup_terminated.s`), called from
`__thread_select_next` before any thread is chosen.

| Step | Routine | What it does |
|---|---|---|
| 1 | interrupt → `__thread_robin` | Scheduler tick |
| 2 | save current context | Store the interrupted thread |
| 3 | `__tmr_chain` | Fire due timers |
| 4 | `__thread_select_next` | Choose who runs next |
| 4a | `__thread_cleanup_terminated` | This chapter: for each terminated thread `t` except `thread_current`, `mem_free_owner(__sys_heap, t)` (stack), `so_destroy(terminated, t)` (object), `process_reap(t->process)` |
| 4b | wake waiting threads | Event/timer wakeups |
| 4c | pick next runnable | Next `RUNNING` thread |
| 5 | restore next context | Resume that thread |

`thread_current` is skipped because the interrupt running the cleanup
pass may itself be executing on that thread's own stack — it called
`thread_exit` and halted. It gets collected one tick later, once another
thread has become current.

## Thread cleanup

For each terminated thread:

1. `mem_free_owner(__sys_heap, thread)` frees every fixed OS-heap block
   whose owner is the thread. Since `thread_create` allocates the stack
   with the thread as owner, this step releases the stack — and any other
   OS block the thread happened to own.
2. `so_destroy(&thread_first_terminated, thread)` unlinks the 38-byte
   object and returns it to `__sys_heap`.
3. `process_reap(thread->process)` is called, which may or may not do
   anything further (see the next section).

## Process reaping

`process_reap` (`kernel/process_reap.s`) is the owner-based sweep. It
runs inside a critical section and begins with `_process_has_threads`,
which scans the suspended, running, waiting, and terminated lists for any
thread whose `process` field points at this process. If one exists, the
process is still alive and `process_reap` returns immediately. Otherwise,
in order:

| Step | Routine | What is released |
|---|---|---|
| 1 | `__so_reap` | owned events |
| 2 | `__so_reap` | owned timers |
| 3 | `__so_reap` | owned public and private/staged services |
| 4 | reference-release loop | each acquisition; libraries reaching zero are reaped |
| 5 | `mem_free_owner(__sys_heap, p)` | every fixed OS-heap block owned by the process |
| 6 | `__bank_free_owner(p)` | every bank-arena block owned by the process, including its XPRG image |
| 7 | `so_destroy(&process_first, p)` | the 16-byte process object itself |

Each "find and destroy" loop restarts from the head of its list after
every hit, so it stays safe even while the list is being relinked
underneath it.

## What is *not* reclaimed

Ownership is only as good as the owner it was assigned:

- `allocate_memory` and `register_service` use the current process, or
  the library-owner override during initialization. Their resources are
  reaped along with it.
- `create_timer(hook, ticks)` still creates kernel-owned timers. Destroy
  them before exit — background library callbacks are not supported.
- `create_event(owner)` does take an owner; pass your `yos_process_t *`
  and the event is reaped along with the process.
- Threads created with `create_thread(entry, stack, process)` get the
  process as their `process` field, so they keep the process alive and
  are reclaimed through the thread path described above; their stacks are
  owned by the thread itself, not the process.

Kernel-owned objects — the clock, keyboard, and mouse timers, and the
single `yos` service, which now also exposes the graphics API — carry
owner `NONE` by design and are never reaped.

The creator of a process plays no part in cleanup. A process object has
`owner = NONE` and no parent field; only its thread membership and its
own owned resources determine when it can be reaped.

## Example timeline

A process `count` with one thread returns from its entry function:

```
tick N     startup stub → thread_exit(t): t moved to terminated list, HALT
tick N+1   scheduler: t == thread_current, skipped; another thread runs
tick N+2   scheduler: t freed (stack + object); process_reap(count):
           no threads left → events, timers, services, image, process freed
```

If `count` had spawned a second thread that is still running,
`process_reap` returns at tick N+2 without touching anything, and the
whole sweep waits until that second thread terminates in turn.

## What to keep in mind while developing

- Always set the correct owner when creating a resource from kernel code.
- Destroy kernel-owned timers explicitly. Ordinary registered services
  are process-owned; never unregister a loader-managed library service
  yourself.
- Never free a thread's stack or a loaded image manually — the kernel
  owns both.
- A thread that must block waiting for another should `suspend_thread`
  itself. There is no join, so the exiting thread has to `resume_thread`
  its waiter before it returns.

## Related chapters

- [Resource Accounting](RESOURCE-ACCOUNTING.md)
- [Memory Management](MEMORY-MANAGEMENT.md)
- [Threads](THREADS.md)
- [Processes](PROCESSES.md)
