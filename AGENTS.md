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
- The ordinary root and `make -C x` builds default to the medium `X_MODEL=M`
  distribution. Use `X_MODEL=S|L` explicitly or the root `x-s`, `x-m`, and
  `x-l` targets when another model is required. In M, source-level `double`
  and `long double` alias the selected `float` ABI; L retains the genuine
  64-bit software-double ABI.
- The YOS ROM and the active `y/tests/*-yos` app builds now use the staged
  `bin/x/bin/{xcc,xas,xld}` toolchain directly with `-Os`; the old Docker /
  SDCC-only path is no longer the default build route.
- "Cross-platform" in this repo means GNU Make plus a POSIX-like shell/tool
  environment (for example Linux, macOS, or MSYS2 on Windows), because the
  Makefiles still use commands such as `rm`, `cp`, `mkdir`, and `sed`.

### Useful Current Commands

```bash
make                    # full build (current root flow)
make packages           # build and verify the X .deb and XGDB VSIX
make -C x               # build the standalone xtools prefix
make clean

make -C x               # build the migrated X product tree
make -C y               # build the migrated Y product tree
make -C x/libc          # build only the C library
make -C y/src           # build only the OS
bash x/tests/run_tests.sh --filter xcc
make test-x-models      # verify the S, M, and L feature-filtered matrices
bash x/tests/tests/e2e/run.sh --no-build # exhaustive suite; uses bin/x-l
python3 x/tests/tests/zx48/run_mcp.py --mcp /path/to/zx-spectrum-mcp --rom /path/to/48.rom
python3 x/tests/tests/cpc/run_mcp.py --mcp /path/to/amstrad-cpc-mcp --roms /path/to/roms
x/tests/benchmarks/z88dk24/prepare.sh
x/tests/benchmarks/z88dk24/run.sh
```

The complete target memory maps, Fuse commands, Tamsyn console contract, and
unsupported-service rules are in `x/docs/howtos/ZX-SPECTRUM-48K.md`.

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
  - `x/tests/tests/zx48/` — optional real-ROM/MCP coverage for the staged
    `zx-ram` and `zx-rom` platforms plus TAP/TZX playback
  - `x/tests/tests/cpc/` — optional real-ROM/MCP coverage for CPC 464 CDT and
    writable CPC 664/6128 AMSDOS DSK workflows
  - `x/tests/benchmarks/z88dk24/` — locked seven-lane full-program comparison;
    use its `prepare.sh` before `run.sh`, and preserve the independent corpus,
    target-sysroot, nightly-zsdcc, and 80cc commit pins

- The long-term direction is still component-owned tests plus a smaller E2E
  bucket, but the repository has not fully moved there yet.

When adding a new test:
- Prefer placing it under `x/tests/tests/` or `y/tests/` according to ownership.
- Give it a `test.cfg` with a stable `id`, `component`, and any needed `legacy_path` aliases.
- Keep cross-stack cases small and explicit.

## Working on the X Tools Independently

The "x tools" (xcc, xas, xld, ...) should be buildable and distributable without the full OS.

1. `make -C x all`
2. The resulting artifacts in `bin/x/` (plus staged includes and runtime libs) form the distributable "xtools" package.
3. `make packages` produces and verifies the X Debian package and XGDB VSIX;
   the package contract is documented in `x/pkg/README.md`.

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
- Every immediate child of `x/platforms/` is one selectable target. Do not add
  shared, `common`, or `include` pseudo-target directories there. Put the
  common hook contract in `x/libc/include/sys.h`; put target-only public
  headers below that target's own `include/` directory. A target must not
  include sources from a sibling target. `make -C x check-platform-layout`
  enforces these structural rules.
- ZX target code follows the same assembly-only tradition. Each target is
  self-contained under `x/platforms/zx-ram/` or `x/platforms/zx-rom/`; keep
  their mirrored console, keyboard, and font sources synchronized and rerun
  all four MCP modes.
- CPC target code is self-contained under `x/platforms/cpc-464/`,
  `cpc-664/`, or `cpc-6128/`. The 464 must not acquire AMSDOS buffers; keep
  the mirrored disk-target sources synchronized and rerun all three MCP
  models after firmware, file, linker, or media changes.
- Platform examples follow the same one-directory-per-target rule under
  `x/examples/` (`cpm3`, `cpc-464`, `cpc-664`, `cpc-6128`, `zx-ram`,
  `zx-rom`, and so on). Do not combine
  targets in one example directory or import example source from a sibling.

## Common Tasks

**Build just the compiler and run its tests**
```bash
make -C x
make -C x/src/xcc test
```

**Work on libc + run its tests**
```bash
make -C x/libc
bash x/tests/run_tests.sh --filter xcc
```

**Run the runtime test suite**
```bash
make -C x
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
- Build with `make -C x`
- Build installable artifacts with `make packages`.
- Repeat the finished Debian archive audit with `make -C x/pkg/debian check`.
- Keep every packaged platform's CRT object/source, both linker scripts,
  archive, and user guide represented in the package verification manifest.

## Session / AI Context

- When returning to this directory, the session system will provide a compacted history.
- For long-term memory, rely primarily on this `AGENTS.md`, `x/docs/ARCHITECTURE.md`, and `x/docs/CURRENT-STATUS.md`.
- Feel free to read previous session compaction files under `~/.grok/sessions/...` if you need very detailed prior transcripts.

## Open / Recent Work (as of last major session)

- Heavy focus on completing a full C23 libc surface in pure assembler (strfrom*, fromfp* family, fmaximum*/fminimum* variants, roundeven*, totalorder*, get/setpayload, free_sized/aligned, stdckdint, char8_t support, %b in printf, stdio float, etc.).
- Large dual-style test base (direct emulator calls + xcc-compiled C cases) for both libc and runtime.
- Integration of an external C23 compatibility suite (`x/tests/tests/c23/`) for compiler testing.
- Discussion and planning of repo restructuring for independent toolchain distribution (see `x/docs/ARCHITECTURE.md`).
- XCC optimization profiles use `-Of` as the validated speed baseline and
  keep `-O3` as an intentionally empty alias of it, ready for the next speed
  experiment. Pure size policy belongs only in `-Os`; validate new `-O3` work
  in both ABI modes before promoting speed wins to `-Of` and Pareto-safe size
  wins to `-Os`.
- XCC's `--runtime=z88dk-classic` profile reports inferred printf/scanf
  capabilities through zcc's per-link `-zcc-opt` file. Keep this analysis
  source-independent and conservative for dynamic or escaped formatters; do
  not replace it with benchmark-specific pragma masks. The locked 24-program
  matrix records `-Os` as strict smallest on 24/24 valid competitor envelopes
  and `-Of` on 22/24.
- The locked z88dk24 audit currently has both XCC M profiles correct on 24/24;
  `-Of` is strictly fastest on 13/24 rows against the best valid current
  zsdcc/80cc result. Preserve that result structurally: compiler rules must be
  selected from IR, CFG, liveness, aliases, and clobbers, never benchmark
  names, source fragments, magic workload constants, or program fingerprints.

Update this file and the architecture documents when major structural or philosophical changes are made.

## Contact / Next Steps

If you're an AI agent starting fresh here, read:
1. This `AGENTS.md`
2. `x/docs/ARCHITECTURE.md`
3. `x/docs/CURRENT-STATUS.md` (or `x/docs/HANDOFF.md`)
4. The component changelogs (`x/CHANGELOG.md`, `y/CHANGELOG.md`, `z/CHANGELOG.md`)
5. The component READMEs (`x/README.md`, `y/README.md`, `x/lib/README.md`, etc.)

Then ask the user what they want to work on.
