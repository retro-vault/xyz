# How to Test After Refactoring

Run the end-to-end suite from the repository root:

```
./tests/e2e_test.sh
```

This builds all tools and then runs every test phase. Exit code is 0 on full pass.

If the binaries are already current, skip the build:

```
./tests/e2e_test.sh --no-build
```

To run a single phase while hunting a specific regression:

```
./tests/e2e_test.sh --no-build --phase xcc
./tests/e2e_test.sh --no-build --phase xlink
./tests/e2e_test.sh --no-build --phase xas
./tests/e2e_test.sh --no-build --phase xar
./tests/e2e_test.sh --no-build --phase chain
```

## What each phase covers

| Phase | What it tests | Pass bar |
|-------|--------------|----------|
| **xcc** | C compiler unit tests — 60 cases across core, opt, and sema suites | 0 failures |
| **xlink** | Linker unit tests — 42 cases covering parsing, placement, relocation, emission | 0 failures |
| **xas** | Assembler parity — compiles all 50 xcc core test inputs with both xas and sdasz80, links both, compares code bytes | 0 failures (1 skip is a known sdasz80 limitation) |
| **xar** | Archive tool smoke test — create, list, link against archive | 0 failures |
| **chain** | Full pipeline — 8 C programs through xcc → xas → xlink, verifies valid XL binary output | 0 failures |

## Refactoring checklist

1. Make your change.
2. Run `./tests/e2e_test.sh`. All 5 phases must pass.
3. If the **xas** phase regresses, the failing test name maps directly to `src/xc/xcc/tests/data/core/<name>.c` — read the generated assembly to find what changed.
4. If the **chain** phase regresses on a specific program, the phase prints which tool failed (xcc / xas / xlink), which narrows the search immediately.
5. Commit only when the suite is green.

## Prerequisites

- `sdasz80` must be on `PATH` for the xas parity phase. If absent, that phase is skipped automatically.
- Python 3 must be on `PATH` (used to parse XL binary headers).
- All other tools (xcc, xas, xar, xlink) are built from source by the suite itself unless `--no-build` is passed.
