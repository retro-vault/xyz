# Quick Handoff for Returning Sessions

This is a short, high-signal summary intended to be the first thing an AI (or human) reads when coming back to this project after working elsewhere.

**Project**: Z80 retro stack (toolchain + libc + OS + supporting pieces) in `/home/tstih/data/retro-vault/xyz`.

**Most recent major threads**:
- Completed a large amount of the C23 libc surface in pure assembler (only editing existing `.s` files, stack/register-only for new state, full dual direct + C-driven test coverage).
- Integrated an external C23 compatibility test suite (copied into `tests/c23/`) and enriched the in-tree dispatch.
- Discussed and planned a repo restructuring so the "x tools" (xcc/xas/xld/...) can be built and published independently while still supporting the OS and good local + E2E testing.

**Key durable documents** (read these first on return):
1. `AGENTS.md` (root) — working conventions, build targets, test philosophy, common tasks.
2. `docs/ARCHITECTURE.md` — current problems + target modular structure (toolchain/ as the independent product, component-owned tests, etc.).
3. `docs/CURRENT-STATUS.md` — detailed summary of the C23 work, the test base, and open items.
4. This `docs/HANDOFF.md` — the ultra-short version.

**How to get back into the code quickly**:
- Build the current toolchain/lib: normal `make` or the relevant sub-makes.
- Run the main libc + C23 tests: `make -C tests/libc core-test`
- Explore the external-style C23 matrix: `cd tests/c23 && make matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json` (or with `RUN_MODE=never` for compile-only).
- The libc C23 implementation lives in existing assembler files under `lib/libc/src/` (look for recent additions around strfrom, fromfp, fmaximum, stdckdint, free_*, timespec_getres, char8_t, etc.).
- Test harnesses: `tests/libc/test_main.cpp` (direct + C-driven) and `tests/runtime/`.

**If you need deeper history**:
- The environment will supply a compacted session summary when you start in this directory.
- Previous session artifacts live under `~/.grok/sessions/<path-to-this-dir>/`.

**Current priority / open work** (from the last session):
- The restructuring proposal in `docs/ARCHITECTURE.md` (make `xtools` a first-class independent build + distribution target, move tests to be component-local, keep a small E2E bucket).
- Keep the dual "both" testing approach as new features are added.
- The copied `tests/c23/` suite + the enriched in-tree `c23_cases.c` should be maintained together.

When starting a new session here, the AI should read the three docs listed above and then ask the user what they want to tackle next (restructuring steps, more C23 work, OS features, distribution/packaging, etc.).

Update the status and handoff documents when major milestones are reached.