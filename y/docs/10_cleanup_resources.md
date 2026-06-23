# Cleaning Up Resources

This chapter explains what happens when processes and threads terminate, and how `yos` is designed to reclaim owned resources.

## Why Cleanup Matters

On a 48K machine, memory leaks and orphaned objects quickly break the system. `yos` therefore treats cleanup as a core kernel responsibility, not as an optional app concern.

The cleanup model is based on **ownership**:

- resources are system objects (`sysobj_t`)
- each object has an `owner`
- objects are tracked in linked lists
- when an owner dies, its objects should be released

## Current Lifecycle Hooks

Relevant functions:

- `thread_exit(thread_t *t)` moves a running thread to the terminated list.
- `process_exit(void)` gets the current process, calls `_process_cleanup(proc)`, and removes the process object.
- `so_destroy(...)` removes an object from its list and frees it from system heap.

## Important Current State

`_process_cleanup(process_t *p)` currently exists but is an empty stub in `kernel/process.c`.

That means the cleanup architecture is present, but full resource sweep logic is still to be implemented.

In practice today:

- process records are removed by `process_exit`
- thread state transitions happen correctly
- full owner-based cascade cleanup is not yet complete in one central pass

## Intended Cleanup Flow

The intended flow is:

1. identify all objects owned by a process
2. terminate owned threads safely
3. release owned timers, events, services, and memory blocks
4. remove the process object itself

Because all resources share `sysobj_t` + list conventions, this can be implemented as list scans with owner checks.

## What To Keep In Mind While Developing

- Always set correct owner when creating a resource.
- Unregister services explicitly before process exit.
- Keep thread stacks and dynamic allocations tied to clear owners.
- Treat `_process_cleanup` as a planned extension point for robust process teardown.

## Related Chapters

- [Resource Accounting](04_resource_accounting.md)
- [Memory Management](05_memory_management.md)
- [Threads](06_threads.md)
- [Processes](07_processes.md)
