# C23 Suite Integration for xcc / retro-vault xyz

The full C23 compatibility suite (originally from /home/tstih/data/tstih/c23)
has been copied into this directory (`x/tests/c23/`).

## Contents (copied "all")
- `tests/cases/` — 63 feature tests (core-language, library, time, iec-60559,
  unicode/char8_t, initialization/structs, stdckdint, stdbit, free_sized,
  fromfp/minmax, timespec_getres, attributes, preprocessor, etc.).
- `setups/sdcc-z80/` — reference Z80+ucsim setup (profile + drivers).
- `setups/xcc-z80/` — **new** skeleton for this project's xcc + xas + xld + xz80
  (profile + driver + run script).
- `docs/`, `tests/scripts/`, `tests/spec/`, top-level Makefile, etc.

## Using with xcc (this project)
1. Compile-only survey (no execution needed yet):
   ```sh
   cd x/tests/c23
   make matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json RUN_MODE=never
   ```

2. Full run (requires a working runner that produces the expected "OK <id>\n"
   on stdout and exits 0):
   - The driver (`setups/xcc-z80/bin/xcc_z80_driver.sh`) does:
     - xcc -S (with -I for x/libc/include)
     - xas --mode=sdcc
     - link attempt (best-effort using prebuilt objs from build/tests/libc if present)
   - The run script (`run_xcc_z80.sh`) converts .ihx -> .bin and is a placeholder.
     Extend it to drive the xz80 emulator (or reuse the project's
     `x/tests/libc/` harness + "none" sys hooks for putchar capture).

3. Recommended practical path for this project (already partially wired):
   - The in-tree `x/tests/libc/c23_cases.c` was enriched directly from this suite's
     cases (all categories + all major structs: div_t family, tm, timespec,
     lconv, fenv_t, mbstate_t, etc. + every C23 libc addition: strfrom*,
     fromfp* family, fmaximum*/fminimum* variants, roundeven, payloads,
     totalorder, free_sized/aligned, ckd_*, stdbit, char8_t + mbrtoc8, etc.).
   - Build & run:
     ```sh
     make -C x/tests/libc core-test
     ```
   - This exercises the compiler (xcc) + your asm libc + runtime in the
     emulator via the existing C-driven mechanism (return code in DE + hooked
     stdio buffers).

## Adapting the full matrix
- The profile and drivers under `setups/xcc-z80/` are starting points.
- For per-case .ihx execution you will likely want a crt0 + selective libc
  rels or to feed cases into a big dispatch image (the pattern used by
  `x/tests/libc/stdio_cases*.c` and `c23_cases.c`).
- See the original suite's `docs/howtos/TEST.md` (copied here) for the profile
  schema and how the Python matrix runner works.
- Your existing `x/tests/runtime/tools/ihx2bin.py` and the xz80 C++ bits are
  the natural execution engine.

## Structures / completeness
All categories and the key C structures from the input suite are represented
in the copied cases/ and were used to enrich `x/tests/libc/c23_cases.c`.

The external suite remains useful for golden results (gcc, sdcc, etc.) under
`build/results/`.

Happy C23 compiler + libc testing!
