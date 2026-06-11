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

## Build Philosophy

- The root `Makefile` is a thin orchestrator. It delegates to components via `SUBDIRS`.
- Prefer building **subsets** when possible (see targets below).
- Output goes to `bin/` (dist) and `build/`.
- Most components use recursive Make. Some sub-areas may use CMake (check `archive/` and vendored dirs).
- Cross-compilation targets live under `bin/z80/` style layouts (sysroot concept).

### Useful Top-Level Targets

```bash
make                    # full build (all SUBDIRS + staging)
make xtools             # build only the toolchain (recommended for distribution)
make libc               # build only the C library
make runtime            # build only the low-level runtime
make yos                # build only the OS
make test-xtools        # run toolchain-local tests
make test-libc          # run libc-local tests
make test-e2e           # run cross-component end-to-end tests
make clean
```

See `docs/ARCHITECTURE.md` for the target structure that enables clean `make xtools` etc.

After building the toolchain you will typically want:
```bash
make stage-includes stage-xcc-support
```

## Testing Strategy (Local vs E2E)

- **Local/component tests** live with the component:
  - `toolchain/tests/` (or `src/xc/tests/`) — compiler, assembler, linker unit + feature tests.
  - `libc/tests/` — current `tests/libc/` content (test_main.cpp, cases, C23 dispatch, etc.).
  - `runtime/tests/`
  - `os/tests/`
- These should be runnable with `make -C <component> test` (or the root `make test-<component>` wrappers).

- **End-to-end / integration tests** live under `tests/`:
  - `tests/e2e/`
  - `tests/integration/`
  - `tests/c23/` (the full C23 compatibility suite — used for both compiler and libc surface testing)
  - `tests/benchmarks/`, `tests/debug/`, etc.
- E2E tests may require multiple components to be built first (the root `make test-e2e` target handles this).

When adding a new test:
- Put it in the component's own `tests/` directory if it only exercises that component.
- Put it under root `tests/` only if it genuinely needs the full stack (compiler + libc + OS + emulator).

## Working on the X Tools Independently

The "x tools" (xcc, xas, xld, ...) should be buildable and distributable without the full OS.

1. `make xtools` (or `make -C toolchain`)
2. The resulting artifacts in `bin/` (plus staged includes and runtime libs) form the distributable "xtools" package.
3. Packaging lives in `pkg/`. Look for (or create) `xtools`-specific packaging targets.

When publishing the toolchain:
- It should include the compiler suite + libc headers + minimal runtime + documentation.
- It should be usable against bare metal, YOS, or future systems via `--sysroot` / include paths.

## Coding & Style Conventions

- Z80 assembly style: see `docs/standards/Z80-CODING-STYLE.md`
- C coding style (where C is used, e.g. host tools and tests): see `docs/standards/CPP-CODING-STYLE.md` (mostly applicable to C too)
- YOS (OS) specific guidance: `src/yos/README.md` and `src/yos/INDEX.md`
- Keep the libc **thread-safe** — no new writable statics / `_DATA` section variables for new code. Use stack, registers, or explicit library state only.
- New C23 functionality is implemented in assembler (in existing `.s` files) unless writing headers.
- Tests: both "direct" (emulator symbol calls via `runtime_machine`) and "C-driven" (compile `.c` with `xcc`, run in emulator, compare behavior or output to host gcc).

## Common Tasks

**Build just the compiler and run its tests**
```bash
make xtools
make test-xtools
```

**Work on libc + run its tests**
```bash
make libc
make test-libc
```

**Run the full C23 compiler + libc surface test**
```bash
make -C tests/libc core-test
# or via the copied suite
cd tests/c23 && make matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json
```

**Add a new C23 feature test**
- Prefer adding to the in-tree `libc/tests/` dispatch (see `c23_cases.c`) for semantic verification.
- The external-style suite under `tests/c23/tests/cases/` is the source of truth for the full matrix.

**Distribute the x tools**
- Build with `make xtools`
- Use / extend packaging under `pkg/`
- The result should be installable independently (binaries + headers + runtime libs).

## Session / AI Context

- When returning to this directory, the session system will provide a compacted history.
- For long-term memory, rely primarily on this `AGENTS.md`, `docs/ARCHITECTURE.md`, and `docs/CURRENT-STATUS.md`.
- Feel free to read previous session compaction files under `~/.grok/sessions/...` if you need very detailed prior transcripts.

## Open / Recent Work (as of last major session)

- Heavy focus on completing a full C23 libc surface in pure assembler (strfrom*, fromfp* family, fmaximum*/fminimum* variants, roundeven*, totalorder*, get/setpayload, free_sized/aligned, stdckdint, char8_t support, %b in printf, stdio float, etc.).
- Large dual-style test base (direct emulator calls + xcc-compiled C cases) for both libc and runtime.
- Integration of an external C23 compatibility suite (`tests/c23/`) for compiler testing.
- Discussion and planning of repo restructuring for independent toolchain distribution (see `docs/ARCHITECTURE.md`).

Update this file and the architecture documents when major structural or philosophical changes are made.

## Contact / Next Steps

If you're an AI agent starting fresh here, read:
1. This `AGENTS.md`
2. `docs/ARCHITECTURE.md`
3. `docs/CURRENT-STATUS.md` (or `docs/HANDOFF.md`)
4. The component READMEs (`toolchain/README.md`, `libc/README.md`, `src/yos/README.md`, etc.)

Then ask the user what they want to work on.