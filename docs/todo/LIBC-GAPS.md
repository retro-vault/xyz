# libc Gaps

This document tracks the remaining work needed before `lib/libc` can be
described as a broadly usable C23 library for `xcc`.

## Current Shape

- The library now covers the foundational freestanding headers plus the first
  target-independent slice of the general library: `ctype.h`, `string.h`,
  `stdlib.h`, `locale.h`, `signal.h`, `wchar.h`, `wctype.h`, `uchar.h`,
  `fenv.h`, `inttypes.h`, `stdbit.h`, `stdatomic.h`, `math.h`, and
  `complex.h`.
- The staged archive is `bin/lib/z80/libc.a`.
- The staged public headers live in `bin/include/z80/`.
- The assembly libc no longer depends on ad-hoc writable module scratchpads
  for ordinary per-call workspace. Remaining writable objects are intentional
  shared state or API-defined static-return storage such as `errno`,
  `rand`, `atexit` tables, heap metadata, signal tables, `FILE` pools,
  `strtok` continuation state, `tmpnam` state, and the static buffers behind
  `asctime`, `ctime`, and `gmtime`.

## Implemented But Partial

- `assert.h` is only a minimal failure sink today.
  It records the failure context in globals and halts forever.
  It does not print to `stderr` or integrate with hosted I/O.

- `ctype.h` is ASCII-only.
  The classifiers and case converters assume a single-byte ASCII execution
  character set and do not provide locale-sensitive behavior.

- `errno.h` is process-global.
  `errno` is one global integer cell, not thread-local storage.
  Only the standard C macros `EDOM`, `ERANGE`, and `EILSEQ` are defined.

- `setjmp.h` saves only the execution state required by current `xcc`
  generated code.
  The implementation captures SP, PC, and IX.
  There is no signal-mask concept and no wider machine-context preservation.

- `string.h` is partly locale-neutral.
  `strcoll()` is equivalent to `strcmp()`.
  `strxfrm()` is the identity transform.
  `strerror()` only distinguishes success from a generic error.

- `complex.h` and `tgmath.h` now cover the standard complex surface through
  the circular, inverse circular, hyperbolic, and inverse hyperbolic helper
  families. The current limitation is ABI depth rather than missing entry
  points: `double` and `long double` spellings still alias the float-first
  implementation.

- `fenv.h` models one process-wide soft-float environment only.
  Exception flags are sticky and user-visible, but the arithmetic helpers do
  not yet raise them automatically as computations execute.

- `locale.h` exposes only the built-in `"C"` locale.
  `setlocale()` accepts `"C"`, `"POSIX"`, and the empty string as aliases
  for that one locale profile.

- `math.h` is much broader now, but its transcendental families still need
  accuracy hardening rather than just more entry points.

- `signal.h` provides one process-global disposition table and synchronous
  `raise()` delivery only.
  There is no operating-system signal integration.

- `stdatomic.h` currently ships helper code for 8-bit and 16-bit operations.
  Wider atomic object types are declared, but using the generic operation
  macros on unsupported widths will fail at link time rather than silently
  miscompile.

- `stdalign.h`, `stdbool.h`, and `stdnoreturn.h` are compatibility headers.
  They provide the standard spellings and C23 transition behavior, but no
  extra runtime functionality.

- `stdckdint.h` is only a surface-level placeholder today.
  The macros compute the requested result but do not report real overflow yet.

- `stdlib.h` is freestanding and intentionally small.
  It includes a fixed in-library heap, simple `rand()`, insertion-sort based
  `qsort()`, and no environment-variable or process-spawn support.

- `uchar.h` maps between multibyte text and UTF-16/UTF-32 using the current
  single-byte execution encoding only.
  There is no multibyte state machine, shift-state encoding, or Unicode
  normalization layer.

- `wchar.h` and `wctype.h` implement wide strings, classification, and the
  basic wide stream bridge (`fgetwc`, `fgetws`, `fputwc`, `fputws`, `getwc`,
  `getwchar`, `putwc`, `putwchar`, `ungetwc`) with the current 16-bit
  `wchar_t` model. Formatted wide I/O and locale-aware collation are still
  absent.

## Missing Headers Entirely

None. The remaining work is completeness and hosted-behavior depth inside
headers that now exist.

## Missing Runtime Facilities

- Formatted floating-point and formatted wide-character stream I/O
- Full locale catalogue and locale tables beyond the built-in `"C"` locale
- The still-hardening `<math.h>` approximation kernels
- Richer threading semantics beyond the current single-thread fallback
- Wide atomic helper coverage for 32-bit and 64-bit object operations
- Environment access and hosted process helpers

## Toolchain And Distribution Gaps

- `xas` accepts `.optsdcc ...` for source compatibility, but the current REL
  and ELF pipelines do not encode per-function calling-convention metadata in a
  standard object-file field.

## Recommended Next Steps

- Keep extending `stdio.h` toward hosted behavior: float formatting, real
  buffering, and formatted wide I/O.
- Harden the new transcendental math and complex kernels for edge cases and
  accuracy, then add the inverse complex families.
- Widen `stdatomic.h` beyond 8-bit and 16-bit helpers if larger atomic object
  types become a practical requirement.
