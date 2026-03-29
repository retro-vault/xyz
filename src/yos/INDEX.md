# YOS Documentation Index

This is the main index for `yos` documentation.

The docs are split by subsystem so each chapter can evolve with the code.

## Start Here

- [README.md](README.md) - architecture overview and runtime flow.

## Legacy Snapshot (Preserved)

- [docs/01_legacy_readme_snapshot.md](docs/01_legacy_readme_snapshot.md)
  - full original monolithic README content (kept intact)

## Chapters

1. [The Boot Process](docs/03_boot.md)
   - Z80 power-up path at `0x0000`
   - ROM vectors jumping through RAM for hookability
   - GSINIT initialization and interrupt model (IM1)
2. [Resource Accounting](docs/04_resource_accounting.md)
   - list-based resource tracking
   - `sysobj_t` ownership model
   - create/destroy patterns used by kernel objects
3. [Cleaning Up Resources](docs/10_cleanup_resources.md)
   - current teardown path and lifecycle hooks
   - what is implemented now vs what is planned
4. [Memory Management](docs/05_memory_management.md)
   - dual heap design (`__sys_heap` and `__heap`)
   - first-fit allocation, splitting, and coalescing
5. [Threads](docs/06_threads.md)
   - thread structure and state machine
   - scheduler context save/restore (`__thread_robin`)
   - waits, joins, and event-based wakeup
6. [Processes](docs/07_processes.md)
   - `process_start` / `process_exit`
   - main-thread model and ownership relations
7. [System Calls and Services](docs/08_syscalls.md)
   - service table mechanism (`yos` service)
   - `query_service` and `RST 0x10` dispatch path
   - custom service registration patterns
8. [Clock and Timers](docs/09_clock.md)
   - 50 Hz tick-driven clock
   - timer chain behavior and callback constraints

## Source Map

Main implementation points in the codebase:

- startup: `startup/crt0rom.s`
- kernel init: `main.c`
- scheduler: `kernel/throbin.s`, `kernel/thread.c`
- memory allocator: `kernel/mem.c`
- services/syscalls: `kernel/service.c`, `kernel/syscall.c`
- processes: `kernel/process.c`
- timers/events: `kernel/timer.c`, `kernel/evt.c`
