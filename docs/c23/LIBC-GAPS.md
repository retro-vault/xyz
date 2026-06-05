# libc Gaps

This document tracks the remaining work needed before `lib/libc` can be
described as a broadly usable C23 library for `xcc`.

## Current Shape

- The library currently focuses on foundational headers plus hand-written Z80
  assembly for `ctype.h`, `string.h`, `setjmp.h`, `errno.h`, and assertion
  support.
- The staged archive is `bin/lib/z80/libc.a`.
- The staged public headers live in `bin/include/z80/`.

## Implemented But Partial

- `assert.h` is only a minimal failure sink today.
  It records the failure context in globals and halts forever.
  It does not print to `stderr`, call `abort()`, or integrate with hosted I/O.

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

- `string.h` is partly locale-neutral and partly blocked on allocation work.
  `strcoll()` is equivalent to `strcmp()`.
  `strxfrm()` is the identity transform.
  `strerror()` only distinguishes success from a generic error.
  `strdup()` and `strndup()` depend on `malloc()`, which is not implemented
  yet in this libc.

- `stdalign.h`, `stdbool.h`, and `stdnoreturn.h` are compatibility headers.
  They provide the standard spellings and C23 transition behavior, but no
  extra runtime functionality.

- `stdckdint.h` is only a surface-level placeholder today.
  The macros compute the requested result but do not report real overflow yet.

## Missing Headers Entirely

- `complex.h`
- `fenv.h`
- `inttypes.h`
- `locale.h`
- `math.h`
- `signal.h`
- `stdatomic.h`
- `stdbit.h`
- `stdio.h`
- `stdlib.h`
- `tgmath.h`
- `threads.h`
- `time.h`
- `uchar.h`
- `wchar.h`
- `wctype.h`

## Missing Runtime Facilities

- Dynamic allocation: `malloc`, `calloc`, `realloc`, `free`
- Process termination: `abort`, `exit`, `quick_exit`, `atexit`
- Formatted and stream I/O
- Locale model and locale tables
- Math runtime and `<math.h>` entry points
- Signal handling
- Time/date support
- Wide-character and Unicode library support
- Threading and atomics library support
- Environment access and process startup/shutdown helpers

## Toolchain And Distribution Gaps

- `xas` accepts `.optsdcc ...` for source compatibility, but the current REL
  and ELF pipelines do not encode per-function calling-convention metadata in a
  standard object-file field.

## Recommended Next Steps

- Implement `stdlib.h` with allocation first, so `strdup()` and `strndup()`
  become linkable.
- Add `abort()` and a tiny diagnostics path so `assert.h` can do something
  closer to the standard contract.
- Build out `stdio.h` and `stdlib.h` before attempting larger hosted surfaces
  like `locale.h`, `time.h`, or wide-character support.
- Keep extending the target-side `bin/include/z80/` and `bin/lib/z80/`
  surfaces as more C23 headers and runtime entry points land.
