# Architecture & Structure

This document describes the current state of the repository and the target structure that enables independent distribution of the "X Tools" while keeping the OS and other components maintainable.

## Current State (as of the restructuring discussion)

The project is a single monorepo containing several related but conceptually distinct products:

- **Toolchain ("X Tools")**: xcc (C compiler), xas (assembler), xld (linker), xopt (standalone Z80 assembly optimizer), xar, xobjcopy, xgdb, xemu, plus supporting bits (some runtime code, bfd, etc.). Located primarily under `src/xc/`.
- **C Library**: Hand-written Z80 assembler implementation of (mostly) C23 libc. Located in `lib/libc/`. Must be usable both by the toolchain (for testing) and by the OS.
- **Low-level Runtime**: SDCC-compatible helpers for 64-bit integers, double, float, etc. (`src/xc/xcc/lib/runtime/` + `tests/tests/runtime/`).
- **Operating System**: YOS kernel, drivers, basic applications (`src/yos/` and related directories).
- **Supporting libraries & tools**: xz80 emulator, xbfd, debug infrastructure, host tools (appmake, microdrive tools, etc.).
- **Tests**: Canonical non-benchmark suites now live under `tests/tests/`, while benchmarks remain in the top-level `tests/` benchmark directories.
- **Packaging & Distribution**: `pkg/`, staging into `bin/`, some VSIX packaging for the debugger.

### Problems with the Current Layout
- Everything builds together via a single root `Makefile` `SUBDIRS` list (`lib tools src/xc lib/libc src/yos`).
- It is difficult to build and package just the compiler/linker suite ("x tools") independently for distribution.
- Tests are not clearly owned by the component that produces the code being tested.
- Adding a window/GUI system (or other future components) will further increase coupling and complexity.
- End-to-end tests that genuinely need multiple components are hard to distinguish from local component tests.

## Target Structure

We are moving toward a monorepo with **clear product boundaries** and **component-owned tests**.

Recommended top-level layout (evolve toward this incrementally):

```
xyz/
├── Makefile                  # thin orchestrator (current delegation style)
├── AGENTS.md                 # instructions for AI agents (and humans)
├── docs/
│   ├── ARCHITECTURE.md       # this file
│   ├── CURRENT-STATUS.md     # living handoff / recent work summary
│   └── standards/            # Z80-CODING-STYLE.md, etc.
├── pkg/                      # packaging (split xtools vs yos later)
│
├── toolchain/                # THE INDEPENDENT "X TOOLS" PRODUCT
│   ├── xcc/
│   ├── xas/
│   ├── xld/
│   ├── xopt/
│   ├── xar/
│   ├── xobjcopy/
│   ├── xgdb/
│   ├── xemu/
│   ├── common/               # shared code needed by the tools
│   ├── tests/                # local toolchain tests (compiler features, C23 language, etc.)
│   └── Makefile
│
├── libc/                     # C library (usable by tools and by OS)
│   ├── include/
│   ├── src/
│   ├── sys/                  # none/, cpm3/, spectrum backends, etc.
│   ├── tests/                # local libc tests (moved from tests/libc/)
│   └── Makefile
│
├── runtime/                  # low-level SDCC runtime helpers
│   ├── src/
│   ├── tests/                # local runtime tests (moved from tests/runtime/)
│   └── Makefile
│
├── os/                       # YOS operating system
│   ├── kernel/
│   ├── drivers/
│   ├── apps/
│   ├── include/
│   ├── tests/                # local OS tests
│   └── Makefile
│
├── gui/                      # Future window / GUI system (zwin, etc.)
│   ├── ...
│   └── tests/
│
├── tests/                    # CROSS-COMPONENT / E2E ONLY
│   ├── e2e/
│   ├── integration/
│   ├── c23/                  # the full C23 compatibility suite (compiler + libc surface)
│   ├── benchmarks/
│   └── tools/                # shared test infrastructure (runtime_machine, ihx2bin, xz80 wrappers, etc.)
│
├── tools/                    # Host utilities (appmake, microdrive, serial, ...)
├── archive/                  # historical / vendored material (keep as-is)
└── (build/, bin/, dist/ are build outputs)
```

### Product View

- **X Tools** (distributable independently):
  - `toolchain/` + `libc/include/` + `runtime/` (the parts needed for cross-compilation and basic hosted C).
  - Should be buildable with `make xtools` (or `make -C toolchain`).
  - Packaging target should produce a standalone "xtools" artifact (tar, deb, etc.) that does not pull in the full OS.

- **YOS / Full System**:
  - `os/` + `gui/` + the full `libc/` + `runtime/`.
  - Depends on the toolchain for building user applications.

- **Libraries** (libc + runtime) are intentionally shared but have clear owners and local tests.

## Test Strategy

**Rule of thumb**:
- If a test only needs one component → put it in that component's `tests/` directory.
- If a test genuinely requires the compiler + libc + OS + emulator together → put it under root `tests/` (e2e or integration).

Current examples and current locations:
- `tests/tests/libc/` currently holds the big dual direct + C-driven test base, including the in-tree C23 dispatch.
- `tests/tests/runtime/` currently holds the runtime helper matrices for ll/double and related coverage.
- `tests/tests/c23/` holds the external C23 compatibility suite — used for both compiler acceptance and libc surface verification.
- Benchmarks remain outside that canonical tree for now.

When adding tests for new C23 features (or anything else):
- Add the semantic/functional verification inside the component (for now, extend `tests/tests/libc/c23_cases.c` or the dispatch).
- Use the copied suite under `tests/tests/c23/tests/cases/` as the source of truth for the full matrix of language + library features.

## Build & Distribution

- Root `Makefile` remains a thin delegator + staging layer.
- Add clear phony targets at the root:
  - `make xtools`
  - `make libc`
  - `make runtime`
  - `make yos`
  - `make test-xtools`
  - `make test-e2e`
- `stage-*` targets can be made more product-specific over time (`stage-xtools-includes`, etc.).
- For distribution of the x tools:
  - Build the toolchain product.
  - Stage the necessary headers and runtime libraries.
  - Package via `pkg/` (separate xtools packaging recipe is desirable).
- The current staged xtools layout is now prefix-rooted and relocatable:
  - `bin/x/` is the standalone xtools install prefix.
  - `bin/x/bin/` contains the installed executables.
  - `bin/x/include/` and `bin/x/lib/` hold host SDK headers and libraries
    (`xbfd`, `rsp`, `xgdb`, `xemu`, `xz80`).
  - `bin/x/z80/include/` stages the target libc headers and `yos.h`.
  - `bin/x/z80/lib/` stages `crt0`, linker scripts, `libruntime.a`, `libc.a`,
    the default `libnone.a`, and named platform payloads such as `libcpm3.a`.
  - `bin/y/` holds YOS build outputs plus YOS-adjacent host tools and support
    libraries.
  - `bin/z/` holds staged target assets, apps, and media.
- Default probing is GCC-style and driven by the executable install location:
  - `xcc` probes relative to its prefix for headers.
  - `xld` probes relative to its prefix for runtime/startup libraries.
  - The common standard library and runtime stay shared under
    `bin/x/z80/lib/`.
  - The current default staged platform payload is bare-metal `none`; CP/M 3
    is selected explicitly with `--platform=cpm3`.
- The resulting xtools package should be usable with different sysroots (bare metal, YOS, future GUI system, third-party OS, etc.).

## Migration Path (Incremental)

1. Introduce root phony targets (`xtools`, `test-xtools`, etc.) and wire them to the existing subdirectories.
2. Move/rename `src/xc/` → `toolchain/` (or keep the internal name but document the product boundary).
3. Move `tests/libc/` content into `libc/tests/`.
4. Move `tests/runtime/` content into `runtime/tests/`.
5. Move other per-component tests out of the root `tests/` directory.
6. Keep only true E2E/integration material (and shared test tools) under root `tests/`.
7. Update packaging and documentation to treat "xtools" as a first-class distributable product.
8. Add `docs/ARCHITECTURE.md`, `AGENTS.md`, and `docs/CURRENT-STATUS.md` (this work).

## Open Questions / Future Work

- Exact layout inside `toolchain/` (flat vs. one-dir-per-tool).
- How much of the current `lib/` should move into `toolchain/common/` vs. stay as shared libraries.
- Sysroot layout for the distributed xtools (`bin/z80/`, proper `--sysroot` support, etc.).
- Whether to keep the current heavy recursive-Make approach or gradually introduce a lighter meta-build for the products.
- Packaging split (separate xtools vs. yos packages).

See `docs/CURRENT-STATUS.md` for the latest state of the restructuring effort and any open tasks.

This document should be updated whenever major structural decisions are made.
