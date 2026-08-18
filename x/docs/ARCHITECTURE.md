# Architecture & Structure

This document describes the current product boundaries that let X ship as an
independent toolchain while YOS and future components consume its staged
interfaces.

## Current product layout

The project is a single monorepo containing several related but conceptually distinct products:

- **Toolchain ("X Tools")**: xcc (C compiler), xas (assembler), xld (linker), xopt (standalone Z80 assembly optimizer), xar, xobjcopy, xprog, xgdb, xemu, plus supporting bits (some runtime code, bfd, etc.). Located primarily under `x/src/`.
- **C Library**: Hand-written Z80 assembler implementation of the C23-oriented
  libc surface. Located in `x/libc/` and shared by standalone X targets and YOS.
- **Low-level Runtime**: SDCC-compatible helpers for 64-bit integers, double,
  float, etc. (`x/runtime/` + `x/tests/tests/runtime/`).
- **Operating System**: YOS kernel, drivers, and applications under `y/`.
- **Supporting libraries & tools**: xz80 emulator, xbfd, debug infrastructure, host tools (appmake, microdrive tools, etc.).
- **Tests**: Canonical X non-benchmark suites live under `x/tests/tests/`, while
  X benchmarks remain under `x/tests/benchmarks/` and Y tests under `y/tests/`.
- **Packaging & Distribution**: X and Y own `x/pkg/` and `y/pkg/`; staged
  products land under `bin/x/`, `bin/y/`, and `bin/z/`.

The root `Makefile` is now a thin orchestrator. `make -C x` builds and stages a
self-contained toolchain prefix without requiring YOS; `make -C y` builds the
OS product; the default root build composes them in dependency order.

Remaining structural pressure is narrower: recursive Make still performs much
of the staging, several broad test harnesses remain centralized under
`x/tests/tests/`, and true hardware/emulator integration such as the ZX MCP
suite is optional rather than part of every host-only run.

## Repository structure

The current product boundaries are:

```
xyz/
├── Makefile                  # thin X-then-Y orchestrator
├── AGENTS.md                 # repository conventions and commands
├── x/                        # independently buildable X toolchain product
│   ├── src/                  # xcc, xas, xld, xprog, xemu, ...
│   ├── lib/                  # host SDK libraries
│   ├── libc/                 # target assembly libc and headers
│   ├── runtime/              # target compiler/runtime helpers
│   ├── platforms/            # none, emu, cpm3, zx-ram, zx-rom
│   ├── examples/             # target examples, including ZX/Fuse
│   ├── tests/                # canonical tests and benchmarks
│   ├── docs/                 # architecture, manuals, how-tos, standards
│   └── pkg/                  # X packages
├── y/                        # YOS product, tests, docs, and packages
├── z/                        # future windowing/product placeholder
├── archive/                  # historical and inactive material
├── build/                    # intermediate output
└── bin/                      # staged x/, y/, and z/ products
```

### Product View

- **X Tools** (distributable independently):
  - `x/` contains the host tools, target headers, runtime, libc, and platforms.
  - Build it with `make -C x`; the relocatable prefix lands in `bin/x/`.
  - `make packages` at the root runs the optional product packaging pass.

- **YOS / Full System**:
  - `y/` consumes the staged X tools, libc, runtime, and platform scripts.
  - `z/` remains the future GUI/windowing product boundary.

- **Libraries** (libc + runtime) are intentionally shared but have clear owners and local tests.

## Test Strategy

**Rule of thumb**:
- If a test only needs one component → put it in that component's `tests/` directory.
- If a test genuinely requires the compiler + libc + OS + emulator together → put it under root `tests/` (e2e or integration).

Current examples and current locations:
- `x/tests/tests/libc/` currently holds the big direct assembly test base, including the in-tree C23 dispatch.
- `x/tests/tests/runtime/` holds the runtime helper matrices for ll/double and related coverage.
- `x/tests/tests/c23/` holds the manifest-driven C23 compatibility suite — used for both compiler acceptance and libc surface verification.
- `x/tests/tests/zx48/` owns the optional real-ROM/MCP cross-stack regression.
- Benchmarks remain outside that canonical tree for now.

When adding tests for new C23 features (or anything else):
- Add semantic/functional verification in `x/tests/tests/libc/c23_cases.c` or
  an owning manifest under `x/tests/tests/c23/cases/`.
- Use `x/tests/tests/c23/` as the source of truth for the full matrix of
  language and library behavior.

## Build & Distribution

- Root `Makefile` remains a thin delegator + staging layer.
- The default root `make` path now builds the staged X toolchain first and
  then builds YOS against that staged toolchain; packaging is split into a
  separate `make packages` step instead of being part of the default build.
- For distribution of the x tools:
  - Build the toolchain product with `make -C x`.
  - Stage the necessary headers and runtime libraries.
  - Package via `x/pkg/` or the root `make packages` pass.
- YOS consumes the staged X toolchain directly (`xcc`, `xas`, `xld`, staged
  libc/runtime, and platform scripts).
- The current staged xtools layout is now prefix-rooted and relocatable:
  - `bin/x/` is the standalone xtools install prefix.
  - `bin/x/bin/` contains the installed executables.
  - `bin/x/include/` and `bin/x/lib/` hold host SDK headers and libraries
    (`xbfd`, `rsp`, `xgdb`, `xemu`, `xz80`).
  - `bin/x/z80/include/` stages the target libc headers and `yos.h`; its
    `<platform>/` subdirectories contain target-private headers selected by
    `xcc --platform`.
  - `bin/x/z80/lib/` stages `crt0`, linker scripts, `libruntime.a`, `libc.a`,
    the default `libnone.a`, and named platform payloads such as `libcpm3.a`,
    `libzx-ram.a`, and `libzx-rom.a`.
  - `bin/y/` holds YOS build outputs plus YOS-adjacent host tools and support
    libraries.
  - `bin/z/` holds staged target assets, apps, and media.
- Default probing is GCC-style and driven by the executable install location:
  - `xcc` probes relative to its prefix for headers.
  - `xld` probes relative to its prefix for runtime/startup libraries.
  - The common standard library and runtime stay shared under
    `bin/x/z80/lib/`.
  - The current default staged platform payload is bare-metal `none`; CP/M 3
    and the two ZX Spectrum 48K forms are selected explicitly with
    `--platform=cpm3`, `--platform=zx-ram`, or `--platform=zx-rom`.
  - Flat ROM links support distinct virtual and load addresses. GNU scripts
    use `AT>region`; SDCC-style scripts use `COPY area`. The generated
    `s__AREA_LOAD`/`l__AREA_LOAD` symbols form the CRT copy contract.
  - `zx-ram` owns `0x5CCB` upward beneath a heap ceiling of `0xF000`;
    `zx-rom` owns the fixed `0x0000`–`0x3FFF` image and places writable state
    at `0x5B00`. Each platform directory is self-contained and carries its own
    assembly console, non-blocking `<conio.h>` `kbhit()` scanner, blocking libc input
    derived from that scanner, and snatch-exported Tamsyn font; there is no
    non-target pseudo-platform directory.
  - Every immediate directory below `x/platforms/` names one selectable
    target. The shared hook declaration lives in `x/libc/include/sys.h`, while
    target-only headers live below `x/platforms/<target>/include/` and are
    exposed only when that target is selected. Cross-target source includes
    are forbidden; `make -C x check-platform-layout` enforces the directory
    and source-include invariants before staging.
  - Platform examples mirror target names as immediate directories below
    `x/examples/`. A platform-specific example belongs to one target directory
    and does not share source through a cross-target common directory.
- The resulting xtools package should be usable with different sysroots (bare metal, YOS, future GUI system, third-party OS, etc.).

## Remaining evolution

1. Continue moving broad tests toward their owning components while keeping
   shared emulator infrastructure centralized.
2. Keep genuinely cross-stack tests, such as the ZX ROM/tape MCP flow,
   explicit and optional when they require external assets.
3. Extract more CPU/model/ABI definitions into `x/targets/` where that removes
   duplication without hiding platform memory contracts.
4. Reduce recursive staging work where dependency-aware rules can safely
   replace full archive rebuilds.
5. Keep the X package independently installable and ensure every staged
   platform ships its CRT, scripts, archive, and user documentation together.

## Open Questions / Future Work

- How much of `x/lib/` should remain shared host SDK surface versus private tool code.
- Whether a future explicit `--sysroot` should supplement the current
  relocatable `<prefix>/z80/` probing contract.
- Whether to keep the current heavy recursive-Make approach or gradually introduce a lighter meta-build for the products.
- How future 128K/banked Spectrum targets should expose paging without
  weakening the simple 48K platform contract.

See `x/docs/CURRENT-STATUS.md` for the latest state of the restructuring effort and any open tasks.

This document should be updated whenever major structural decisions are made.
