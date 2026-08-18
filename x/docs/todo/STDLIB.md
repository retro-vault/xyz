# Standard Library Status

Implementation status of the xcc Z80 C library (`lib/libc`) against the C23
standard library, plus the common POSIX/BSD/GNU extensions this target ships.

This is a status matrix. For the narrative discussion of partial headers and
recommended next steps see [LIBC-GAPS.md](LIBC-GAPS.md). For the planned
`long long` / `double` runtime helpers (which back the wide-integer and
floating-point library entry points) see [BIG-NUMBERS.md](BIG-NUMBERS.md).

The current asm libc no longer uses hidden writable module scratchpads for
ordinary per-call helper state. The writable objects that remain are deliberate
library state or API-defined static-return storage, for example `errno`,
`rand`, `atexit` tables, heap metadata, signal tables, `FILE` pools, `strtok`,
`tmpnam`, and the standard time-string buffers.

Legend:

- **asm** — hand-written Z80 assembly in `lib/libc/src/<area>/`
- **C** — C implementation compiled by `xcc`
- **header** — provided as a macro / `static inline` / `_Generic` in the header
- **—** — not implemented
- **gcc-tested** — covered by `tests/libc/` (executed in the xz80 emulator and
  compared against the host result)

---

## Header inventory

All headers live in `lib/libc/include/` and are staged to
`bin/x/z80/include/`.

| Header | State |
|--------|-------|
| `assert.h` | minimal (failure sink only) |
| `complex.h` | implemented (float-first complex surface) |
| `ctype.h` | complete |
| `errno.h` | present (process-global) |
| `fenv.h` | partial (one soft-float env) |
| `float.h` | complete (limits macros) |
| `inttypes.h` | complete |
| `iso646.h` | complete |
| `limits.h` | complete |
| `locale.h` | partial (`"C"` locale only) |
| `math.h` | **sparse** (small subset) |
| `setjmp.h` | present (SP/PC/IX) |
| `signal.h` | partial (synchronous `raise`) |
| `stdalign.h` `stdbool.h` `stdnoreturn.h` | complete (compatibility) |
| `stdarg.h` `stddef.h` `stdint.h` | complete |
| `stdatomic.h` | header + runtime (`__atomic_*` live in the runtime, not libc) |
| `stdbit.h` | complete (C23 bit ops, header-inline) |
| `stdckdint.h` | placeholder (no real overflow report) |
| `stdlib.h` | mostly complete (no C23 alloc/formatted-float extras) |
| `string.h` | complete + extensions |
| `strings.h` | complete (BSD) |
| `tgmath.h` | partial (tracks the implemented `complex.h`/`math.h` surface) |
| `uchar.h` | partial (single-byte encoding, restartable API present) |
| `wchar.h` `wctype.h` | wide strings + classification |
| `stdio.h` | partial (fd-backed streams + formatter family) |
| `threads.h` | partial (single-thread fallback) |
| `time.h` | implemented (all in assembly; sys clock hooks) |

---

## `<stdlib.h>`

| Function | Status | Notes |
|----------|--------|-------|
| `abs` `labs` `llabs` | asm, gcc-tested | |
| `div` | asm, gcc-tested | returns `{quot,rem}` in DE:HL |
| `ldiv` `lldiv` | asm, gcc-tested | 8-/16-byte struct return |
| `atoi` `atol` `atoll` | asm, gcc-tested | |
| `strtol` `strtoul` `strtoll` `strtoull` | asm, gcc-tested | |
| `rand` `srand` | asm, gcc-tested | |
| `malloc` `calloc` `realloc` `free` | asm, gcc-tested | fixed in-library heap |
| `bsearch` `qsort` | asm, gcc-tested | insertion-sort `qsort` |
| `abort` `exit` `_Exit` `atexit` | asm, gcc-tested | |
| `quick_exit` `at_quick_exit` | asm, gcc-tested | |
| `atof` `strtod` `strtof` `strtold` | asm | decimal parser, exponent support, no hex-float locale forms |
| `getenv` `system` | asm, gcc-tested | unhosted stubs (`getenv` always null, `system` unsupported) |
| `mblen` `mbtowc` `wctomb` `mbstowcs` `wcstombs` | asm, gcc-tested | single-byte execution charset only |
| `aligned_alloc` | asm | over-allocates, stores base-pointer metadata, tested through exec runtime |
| `strfromd` `strfromf` `strfroml` | **—** | C23 float formatting |

---

## `<string.h>` / `<strings.h>`

Standard `<string.h>` — all implemented in **asm**:

`memchr` `memcmp` `memcpy` `memmove` `memset` `memset_explicit` `memccpy`
`strcat` `strncat` `strcpy` `strncpy` `strcmp` `strncmp` `strcoll` `strxfrm`
`strlen` `strnlen` `strcspn` `strspn` `strchr` `strrchr` `strpbrk` `strstr`
`strtok` `strdup` `strndup` `strerror`

> `strcoll` == `strcmp` and `strxfrm` is the identity transform (locale-neutral);
> `strerror` only distinguishes success from a generic error.

Extensions — all **asm, gcc-tested**:

| Function | Origin |
|----------|--------|
| `stpcpy` `stpncpy` | POSIX |
| `mempcpy` `memrchr` `rawmemchr` `strchrnul` | GNU |
| `memmem` | GNU |
| `strcasecmp` `strncasecmp` | POSIX |
| `strlcpy` `strlcat` | BSD |
| `strsep` | BSD |
| `strcasestr` | GNU |
| `strsignal` | POSIX |
| `swab` | POSIX |
| `bcopy` `bzero` `bcmp` `index` `rindex` | BSD (`strings.h`) |
| `ffs` `ffsl` `ffsll` | POSIX/GNU (`strings.h`) |

Not yet implemented: `strverscmp`, `basename`, `dirname`.

---

## `<ctype.h>`

Complete (ASCII), all **asm**:

`isalnum` `isalpha` `isblank` `iscntrl` `isdigit` `isgraph` `islower`
`isprint` `ispunct` `isspace` `isupper` `isxdigit` `tolower` `toupper`
`isascii` `toascii` *(`isascii`/`toascii` are asm, gcc-tested)*

---

## `<complex.h>` / `<tgmath.h>`

Implemented in **asm**:

`creal[f/l]` `cimag[f/l]` `conj[f/l]` `cabs[f/l]` `carg[f/l]` `cproj[f/l]`
`cexp[f/l]` `clog[f/l]` `cpow[f/l]` `csqrt[f/l]`
`csin[f/l]` `ccos[f/l]` `ctan[f/l]`
`casin[f/l]` `cacos[f/l]` `catan[f/l]`
`csinh[f/l]` `ccosh[f/l]` `ctanh[f/l]`
`casinh[f/l]` `cacosh[f/l]` `catanh[f/l]`

Notes:

- The current public surface is still float-first: `double` and `long double`
  spellings presently alias the same single-precision complex entry points.
- `CMPLXF` / `CMPLX` / `CMPLXL` now go through a tiny internal constructor
  helper instead of relying on direct complex arithmetic lowering for constant
  construction.
- `tgmath.h` tracks the currently implemented complex surface with `_Generic`
  mappings for `creal`, `cimag`, `conj`, `cabs`, `carg`, `cproj`, `cexp`,
  `clog`, `cpow`, `csqrt`, `csin`, `ccos`, `ctan`, `casin`, `cacos`,
  `catan`, `csinh`, `ccosh`, `ctanh`, `casinh`, `cacosh`, and `catanh`.

## `<math.h>`

The largest gap in the library. Implemented:

`float` is the existing 32-bit soft-float format. `double` is now the 64-bit
runtime format, and `long double` currently aliases `double`. The
non-transcendental family below now has real stack-based `double` /
`long double` wrappers on top of the single-precision kernels.

| Function | Status |
|----------|--------|
| `fabs[f/l]` | asm, gcc-tested |
| `copysign[f/l]` | asm, gcc-tested |
| `sqrt[f/l]` | asm, gcc-tested |
| `atan[f/l]` | asm, gcc-tested |
| `asin[f/l]` | asm, gcc-tested |
| `acos[f/l]` | asm, gcc-tested |
| `sin[f/l]` | asm, gcc-tested |
| `cos[f/l]` | asm, gcc-tested |
| `tan[f/l]` | asm, gcc-tested |
| `sinh[f/l]` | asm |
| `cosh[f/l]` | asm |
| `tanh[f/l]` | asm |
| `asinh[f/l]` | asm |
| `acosh[f/l]` | asm |
| `atanh[f/l]` | asm |
| `atan2[f/l]` | asm, gcc-tested |
| `trunc[f/l]` | asm, gcc-tested (bit-exact) |
| `floor[f/l]` `ceil[f/l]` `round[f/l]` | asm, gcc-tested (bit-exact) |
| `ldexp[f/l]` `scalbn[f/l]` | asm, gcc-tested |
| `ilogb[f/l]` `logb[f/l]` | asm, gcc-tested |
| `frexp[f/l]` | asm, gcc-tested |
| `fmax[f/l]` `fmin[f/l]` | asm, gcc-tested |
| `fdim[f/l]` | asm, gcc-tested |
| `modf[f/l]` | asm, gcc-tested (bit-exact, signed zero) |
| `exp[f/l]` `exp2[f/l]` `expm1[f/l]` | asm, gcc-tested |
| `log[f/l]` `log2[f/l]` `log10[f/l]` `log1p[f/l]` | asm, gcc-tested |
| `pow[f/l]` `cbrt[f/l]` | asm, gcc-tested |
| `erf[f/l]` `erfc[f/l]` | asm |
| `tgamma[f/l]` `lgamma[f/l]` | asm |
| `rint*` `nearbyint*` `lround*` `llround*` `lrint*` `llrint*` | asm, gcc-tested |
| `scalbln*` | asm, gcc-tested |
| `fma*` | asm, gcc-tested |
| `hypot*` | asm, gcc-tested |
| `fmod*` `remainder*` `remquo*` | asm, gcc-tested |
| `nextafter*` | asm, gcc-tested |
| `nan[f/l]` | asm, gcc-tested |
| `significand[f/l]` | asm, gcc-tested |
| classification helpers `__libc_fpclassifyf` / `signbit` / `isnan` / `isinf` / `isfinite` | C / header |

The header (`math.h`) is **finalized**: it declares the complete C23 interface
(trig, exp/log, power, error/gamma, nearest-integer, decomposition, remainder,
FMA, min/max/diff), defines all standard macros (`HUGE_VAL[F/L]`, `INFINITY`,
`NAN`, `FP_*`, `FP_ILOGB0/NAN`, `math_errhandling`, the `M_*` constants), and
provides the classification (`fpclassify`/`isfinite`/`isinf`/`isnan`/
`isnormal`/`signbit`) and comparison (`isgreater`/`isless`/`isunordered`/…)
macros.  It parses cleanly through `xcc`.

Remaining implementation gap in `<math.h>`:

- The classic transcendental families are now linkable. What remains is
  broader accuracy hardening rather than missing entry points.

The single-precision soft-float runtime (`__fsadd`, `__fsmul`, `__fsdiv`,
`__fscmp`, …) and the new 64-bit `double` runtime (`__dbadd`, `__dbmul`,
`__dbdiv`, `__dbcmp`, conversions) already exist in `x/runtime/`,
so the rounding / decompose / min-max families are now achievable. `truncf`
is the proven template. The transcendental family needs polynomial kernels
and is a larger effort.

---

## `<time.h>`

Implemented **entirely in assembly** (`x/libc/src/time/`), built on two
platform clock hooks supplied by the selected backend (`x/platforms/<name>/`):

```
int gettimeofday(struct timespec *tv);        // read wall clock
int settimeofday(const struct timespec *tv);  // set  wall clock
```

The `none` backend ships empty shells (epoch 0); an OS replaces them and the
whole header comes to life. `time_t`/`clock_t` are 32-bit; local time == UTC
(no timezone/DST); calendar arithmetic uses the runtime `long` helpers.

One function per file (`.rel` granularity), so a program links only the
calendar/formatting code it actually calls:

| Function | File | Status |
|----------|------|--------|
| `time` | time.s | asm, gcc-tested |
| `clock` | clock.s | asm |
| `difftime` | difftime.s | asm, gcc-tested |
| `timespec_get` | timespec_get.s | asm |
| `gmtime_r` `localtime_r` | gmtime_r.s | asm, gcc-tested vs host |
| `gmtime` `localtime` | gmtime.s | asm |
| `mktime` | mktime.s | asm, gcc-tested vs host |
| `asctime_r` | asctime_r.s | asm, gcc-tested vs host |
| `asctime` | asctime.s | asm |
| `ctime_r` | ctime_r.s | asm |
| `ctime` | ctime.s | asm |
| `strftime` | strftime.s | asm, gcc-tested vs host (~25 specifiers) |

`gmtime_r` exports the shared leap test / month table that `mktime` reuses.

Selectable backend: `--platform=<name>` selects the staged CRT, linker script,
and `lib<name>.a` hook archive.

---

## `<stdio.h>`

Implemented **in asm**:

- character / line output: `putchar` `fputc` `putc` `puts` `fputs`
- character / block input: `getchar` `fgetc` `getc` `ungetc` `fgets`
  `fread`
- block / fd-backed output: `fwrite`
- file handles: `fopen` `freopen` `tmpfile` `fclose` `fseek` `ftell`
  `fgetpos` `fsetpos` `rewind`
  `fflush` `feof` `ferror` `clearerr`
- convenience / filesystem helpers: `remove` `rename` `tmpnam`
  `perror` `setbuf` `setvbuf`
- formatter family: `printf` `fprintf` `sprintf` `snprintf`
  `vprintf` `vfprintf` `vsprintf` `vsnprintf`
- scanning family: `scanf` `fscanf` `sscanf`
  `vscanf` `vfscanf` `vsscanf`

Current model:

- all public stdio entry points use `sdcccall(0)` so stack layout is uniform
  across the formatter family and the fd-backed stream wrappers
- `FILE` is a tiny unbuffered fd-backed descriptor layered over platform
  `open/read/write/lseek/close`
- the `none` backend ships an in-memory fake filesystem used by the libc tests
- the remaining writable stdio globals are intentional library state: the
  standard streams, the fixed `FILE` pool, tmpfile bookkeeping, and `tmpnam`'s
  API-defined static buffer/counter

Still missing:

- formatted floating-point output (`%f`/`%e`/`%g`/`%a`)
- real buffering behind `setbuf`/`setvbuf` (the current implementations are
  compatibility no-ops)
- formatted wide stdio (`fwprintf`, `fwscanf`, ...)

---

## `<inttypes.h>`

| Function | Status |
|----------|--------|
| `imaxabs` | asm, gcc-tested |
| `imaxdiv` | asm |
| `strtoimax` `strtoumax` | asm |
| `wcstoimax` `wcstoumax` | asm |
| `PRI*` / `SCN*` format macros | header, complete |

---

## `<wchar.h>` / `<wctype.h>`

Wide string/array family (16-bit `wchar_t`) — **asm**:

`wcslen` `wcsnlen` `wcscpy` `wcsncpy` `wcscat` `wcsncat` `wcscmp` `wcsncmp`
`wcschr` `wcsrchr` `wcsspn` `wcscspn` `wcspbrk` `wcsstr` `wcstok`
`wcscoll` `wcsxfrm`
`wmemchr` `wmemcmp` `wmemcpy` `wmemmove` `wmemset`

Single-byte restartable conversion layer — **asm**:

`btowc` `wctob` `mbrlen` `mbrtowc` `wcrtomb` `mbsinit`
`mbsrtowcs` `wcsrtombs`

Wide numeric conversion layer — **asm**:

`wcstof` `wcstod` `wcstold`
`wcstol` `wcstoul` `wcstoll` `wcstoull`

Basic wide stdio is now present in asm too:

`fgetwc` `fgetws` `fputwc` `fputws` `getwc` `getwchar` `putwc` `putwchar`
`ungetwc`

Missing: formatted wide I/O (`fwprintf` etc.), `wcscoll`/`wcsxfrm` beyond
identity, stateful multibyte conversion beyond the target's single-byte
execution charset, and the wide counterparts of the new BSD/GNU string
extensions (`wcpcpy`, `wcscasecmp`, …).

`<wctype.h>` provides the classification family against the 16-bit model.

---

## `<complex.h>`

Implemented (**asm**): `creal*` `cimag*` `conj*` `cabs*` `carg*` `cproj*`
`cexp*` `clog*` `cpow*` `csqrt*` `csin*` `ccos*` `ctan*` `csinh*`
`ccosh*` `ctanh*`.

Missing: the inverse circular/hyperbolic complex families.

---

## `<stdatomic.h>`

The C11 generic atomics dispatch (via the header) to the compiler-emitted
`__atomic_*` intrinsics (`__atomic_load_1/2`, `__atomic_store_1/2`,
`__atomic_exchange_1/2`, `__atomic_compare_exchange_1/2`,
`__atomic_fetch_{add,sub,and,or,xor}_1/2`, `__atomic_flag_*`).

These are **language-support code and live in the runtime**
(`x/runtime/atomic/`), not in `libc.a`. Only 1- and 2-byte
widths are provided; 4- and 8-byte atomic object operations fail at link
time rather than miscompiling.

---

## Other headers

| Header | Implemented | Missing |
|--------|-------------|---------|
| `assert.h` | `assert` failure sink (records context, halts) | `stderr` reporting |
| `errno.h` | `errno`, `EDOM`/`ERANGE`/`EILSEQ` | thread-local `errno` |
| `setjmp.h` | `setjmp` / `longjmp` (SP, PC, IX) | signal-mask / wider context |
| `signal.h` | `signal` / `raise` (synchronous) | OS signal integration |
| `locale.h` | `setlocale`/`localeconv` (`"C"` only) | real locale catalogue |
| `fenv.h` | one soft-float environment; sticky flags | auto-raised exceptions |
| `uchar.h` | `mbrtoc16` `c16rtomb` `mbrtoc32` `c32rtomb` (single-byte) | shift-state / normalization |
| `stdbit.h` | full C23 bit utilities (header-inline) | — |
| `stdckdint.h` | `ckd_add/sub/mul` (compute only) | real overflow flag |

---

## `<threads.h>`

Implemented as a **single-threaded asm fallback**:

- `call_once`
- mutex family: `mtx_init` `mtx_destroy` `mtx_lock` `mtx_trylock`
  `mtx_timedlock` `mtx_unlock`
- condition family: `cnd_init` `cnd_destroy` `cnd_signal`
  `cnd_broadcast` `cnd_wait` `cnd_timedwait`
- thread identity / lifecycle shims: `thrd_current` `thrd_equal`
  `thrd_sleep` `thrd_yield` `thrd_exit`
- thread-specific storage: `tss_create` `tss_delete` `tss_get` `tss_set`

Current model:

- there is no scheduler or true concurrency
- `thrd_create`, `thrd_join`, and `thrd_detach` are linkable assembly stubs
  that return `thrd_error`
- mutexes, once-flags, and TLS are usable by single-threaded portable code
- condition variables are local signal/timed-out shims, not blocking waits
- the writable thread objects that remain are the intended single-thread
  fallback state, not per-call scratch storage

## Missing headers (entire)

---

## Test coverage

`tests/libc/` assembles the hand-written routines, links them at `0x0000`,
runs each in the xz80 emulator, and compares results to the host (gcc)
computation. Coverage now includes the stdlib integer/parsing family, the
single-byte multibyte conversion entry points, restartable wide conversions,
string and ctype extensions, wide-char helpers, time/calendar code, the
non-transcendental and trig math slices, locale/fenv/signal/uchar helpers,
and the fd-backed stdio formatter/input/output layer.

The growing test surface now exceeds what one flat 64K Z80 image can carry, so
the wide stdio slice is verified in a focused companion image from the same
directory while the broad core harness continues to exercise the rest of libc.

---

## Suggested priority order

1. **Math accuracy hardening** (`exp/log/pow/hyperbolic/error-gamma`) — the
   entry points are present, but this is still the biggest pure-library area
   for tighter approximation and edge-case work.
2. **Float string conversion** (`strtod`/`strtof`/`atof`) — pairs naturally with
   `stdio.h` float formatting.
3. **`stdio.h` scanning + buffering** — `scanf` family and real buffered FILEs.
4. **`threads.h`** — decide whether the target exposes threads at all.
5. Remaining string/wide extensions (`memmem`, wide BSD/GNU helpers, locale-aware collation, …).
6. Transcendental `complex.h` kernels.
