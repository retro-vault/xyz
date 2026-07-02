# AGENTS.md — Working with AI Agents on this Project

This file contains conventions, build instructions, and context for AI coding agents (Grok, Claude, etc.) working in this repository.

## Project Overview

This is a Z80 retro-computing monorepo containing:
- **X Tools** — the cross-development toolchain (xcc C compiler, xas assembler, xld linker, xar archiver, xobjcopy, xgdb, xemu, etc.).
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
- The default root `make` path builds the staged X toolchain first and then
  builds YOS natively with that staged toolchain. Packaging is no longer part
  of the default root build; use `make packages` when you explicitly want
  package artifacts.
- Prefer building **subsets** when possible (see targets below).
- Output goes to `bin/` (dist) and `build/`.
- Most components use recursive Make. Some sub-areas may use CMake (check `archive/` and vendored dirs).
- The current staged xtools sysroot lives under `bin/x/z80/`, with related YOS
  outputs under `bin/y/` and staged target assets under `bin/z/`.
- The YOS ROM and the active `y/tests/*-yos` app builds now use the staged
  `bin/x/bin/{xcc,xas,xld}` toolchain directly with `-Os`; the old Docker /
  SDCC-only path is no longer the default build route.
- "Cross-platform" in this repo means GNU Make plus a POSIX-like shell/tool
  environment (for example Linux, macOS, or MSYS2 on Windows), because the
  Makefiles still use commands such as `rm`, `cp`, `mkdir`, and `sed`.

### Useful Current Commands

```bash
make                    # full build (current root flow)
make packages           # optional packaging pass
make xtools             # build the standalone xtools prefix
make clean

make -C x               # build the migrated X product tree
make -C y               # build the migrated Y product tree
make -C x/libc          # build only the C library
make -C y/src           # build only the OS
bash x/tests/run_tests.sh --filter xcc
```

See `x/docs/ARCHITECTURE.md` for the planned future layout and wrapper targets.

After building the toolchain you will typically want:
```bash
make stage-includes stage-xcc-support
```

## Testing Strategy (Local vs E2E)

- **Canonical active non-benchmark X suite** now lives under `x/tests/tests/`:
  - `x/tests/tests/cases/` — the active manifest-driven run set
  - `x/tests/tests/c23_0001_*`, `x/tests/tests/c23_0002_*`, ... — hand-authored probe cases
  - `x/tests/tests/xcc/data/` — source payloads reused by generated compiler cases
  - `archive/x/tests/tests/` — legacy suite manifests and harness entrypoints kept out of the default unified run
- **Benchmarks remain separate for now**:
  - `x/tests/benchmarks/`, `x/tests/grouped_benchmarks/`, `x/tests/numeric_benchmarks/`
  - `y/tests/` — YOS-side apps, media, and emulator harnesses

- The long-term direction is still component-owned tests plus a smaller E2E
  bucket, but the repository has not fully moved there yet.

When adding a new test:
- Prefer placing it under `x/tests/tests/` or `y/tests/` according to ownership.
- Give it a `test.cfg` with a stable `id`, `component`, and any needed `legacy_path` aliases.
- Keep cross-stack cases small and explicit.

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
bash x/tests/run_tests.sh --filter xcc
```

**Run the runtime test suite**
```bash
make xtools
bash x/tests/run_tests.sh --filter xcc_exec_
```

**Run the full C23 compiler + libc surface test**
```bash
bash x/tests/run_tests.sh --filter xcc
# copied external matrix payload still lives under x/tests/tests/c23/
cd x/tests/tests/c23 && make matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json
```

**Add a new C23 feature test**
- Prefer adding to the current in-tree `x/tests/tests/libc/` dispatch (see `c23_cases.c`) for semantic verification.
- The external-style suite under `x/tests/tests/c23/tests/cases/` is the source of truth for the full matrix.

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
- Integration of an external C23 compatibility suite (`x/tests/tests/c23/`) for compiler testing.
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
