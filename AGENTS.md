# AGENTS.md — Working with AI Agents on this Project

This file contains conventions, build instructions, and context for AI coding agents (Grok, Claude, etc.) working in this repository.

## Project Overview

This is a Z80 retro-computing monorepo containing:
- **X Tools** — the cross-development toolchain (xcc C compiler, xas assembler, xld linker, xar archiver, xobjcopy, xgdb, etc.).
- **libc** — hand-written Z80 assembler C library (C23 surface, thread-safe, no static data where possible).
- **Runtime** — low-level SDCC-compatible helpers (int64, double, float, etc.).
- **YOS** — the operating system (kernel, drivers, apps).
- Supporting libraries (xz80 emulator, xbfd, etc.), tests, benchmarks, packaging, and debug tools.
- Future components (window/GUI system).

The goal is a clean, distributable toolchain that can be used independently, plus an OS that builds on it.

Canonical release notes now live with each product root:
- `x/CHANGELOG.md`
- `y/CHANGELOG.md`
- `z/CHANGELOG.md`

## Build Philosophy

- The root `Makefile` is a thin orchestrator. It now delegates primarily to
  the migrated product roots `x/` and `y/`.
- Prefer building **subsets** when possible (see targets below).
- Output goes to `bin/` (dist) and `build/`.
- Most components use recursive Make. Some sub-areas may use CMake (check `archive/` and vendored dirs).
- The current staged xtools sysroot lives under `bin/x/z80/`, with related YOS
  outputs under `bin/y/` and staged target assets under `bin/z/`.

### Useful Current Commands

```bash
make                    # full build (current root flow)
make xtools             # build the standalone xtools prefix
make clean

make -C x               # build the migrated X product tree
make -C y               # build the migrated Y product tree
make -C x/libc          # build only the C library
make -C y/src           # build only the OS
make -C x/tests/libc test
make -C x/tests/runtime test
```

See `x/docs/ARCHITECTURE.md` for the planned future layout and wrapper targets.

After building the toolchain you will typically want:
```bash
make stage-includes stage-xcc-support
```

## Testing Strategy (Local vs E2E)

- **Tool-local tests** already live with some components:
  - `x/src/*/tests` — compiler, assembler, linker, and tool-specific tests
  - `x/lib/xz80/tests`
  - `x/lib/xgdb/tests`

- **Current migrated test suites** now live under the product roots:
  - `x/tests/libc/` — the main libc direct + C-driven suite
  - `x/tests/runtime/` — runtime helper tests
  - `x/tests/c23/` — the external C23 compatibility suite
  - `x/tests/benchmarks/`, `x/tests/debug/`, and other toolchain-side harnesses
  - `y/tests/` — YOS-side apps, media, and emulator harnesses

- The long-term direction is still component-owned tests plus a smaller E2E
  bucket, but the repository has not fully moved there yet.

When adding a new test:
- Put it in the component's own `tests/` directory if it only exercises that component.
- Prefer placing it under `x/tests/` or `y/tests/` according to ownership; keep cross-stack cases small and explicit.

## Working on the X Tools Independently

The "x tools" (xcc, xas, xld, ...) should be buildable and distributable without the full OS.

1. `make xtools` (or `make -C x all` for the migrated X product build)
2. The resulting artifacts in `bin/x/` (plus staged includes and runtime libs) form the distributable "xtools" package.
3. Packaging lives in `x/pkg/` and `y/pkg/`. Look there for xtools- or YOS-specific packaging targets.

When publishing the toolchain:
- It should include the compiler suite + libc headers + minimal runtime + documentation.
- It should be usable against bare metal, YOS, or future systems via `--sysroot` / include paths.

## Coding & Style Conventions

- Z80 assembly style: see `x/docs/standards/Z80-CODING-STYLE.md`
- C coding style (where C is used, e.g. host tools and tests): see `x/docs/standards/CPP-CODING-STYLE.md` (mostly applicable to C too)
- YOS (OS) specific guidance: `y/README.md` and the copied docs under `y/docs/`
- Keep the libc **thread-safe** — no new writable statics / `_DATA` section variables for new code. Use stack, registers, or explicit library state only.
- New C23 functionality is implemented in assembler (in existing `.s` files) unless writing headers.
- Tests: both "direct" (emulator symbol calls via `runtime_machine`) and "C-driven" (compile `.c` with `xcc`, run in emulator, compare behavior or output to host gcc).

## Common Tasks

**Build just the compiler and run its tests**
```bash
make xtools
make -C x/src/xcc test
```

**Work on libc + run its tests**
```bash
make -C x/libc
make -C x/tests/libc test
```

**Run the runtime test suite**
```bash
make xtools
make -C x/tests/runtime test
```

**Run the full C23 compiler + libc surface test**
```bash
make -C x/tests/libc core-test
# or via the copied suite
cd x/tests/c23 && make matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json
```

**Add a new C23 feature test**
- Prefer adding to the current in-tree `x/tests/libc/` dispatch (see `c23_cases.c`) for semantic verification.
- The external-style suite under `x/tests/c23/tests/cases/` is the source of truth for the full matrix.

**Distribute the x tools**
- Build with `make xtools`
- Use / extend packaging under `x/pkg/`
- The result should be installable independently (binaries + headers + runtime libs).

## Session / AI Context

- When returning to this directory, the session system will provide a compacted history.
- For long-term memory, rely primarily on this `AGENTS.md`, `x/docs/ARCHITECTURE.md`, and `x/docs/CURRENT-STATUS.md`.
- Feel free to read previous session compaction files under `~/.grok/sessions/...` if you need very detailed prior transcripts.

## Open / Recent Work (as of last major session)

- Heavy focus on completing a full C23 libc surface in pure assembler (strfrom*, fromfp* family, fmaximum*/fminimum* variants, roundeven*, totalorder*, get/setpayload, free_sized/aligned, stdckdint, char8_t support, %b in printf, stdio float, etc.).
- Large dual-style test base (direct emulator calls + xcc-compiled C cases) for both libc and runtime.
- Integration of an external C23 compatibility suite (`x/tests/c23/`) for compiler testing.
- Discussion and planning of repo restructuring for independent toolchain distribution (see `x/docs/ARCHITECTURE.md`).

Update this file and the architecture documents when major structural or philosophical changes are made.

## Contact / Next Steps

If you're an AI agent starting fresh here, read:
1. This `AGENTS.md`
2. `x/docs/ARCHITECTURE.md`
3. `x/docs/CURRENT-STATUS.md` (or `x/docs/HANDOFF.md`)
4. The component changelogs (`x/CHANGELOG.md`, `y/CHANGELOG.md`, `z/CHANGELOG.md`)
5. The component READMEs (`x/README.md`, `y/README.md`, `x/lib/README.md`, etc.)

Then ask the user what they want to work on.
