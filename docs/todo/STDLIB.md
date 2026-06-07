# Standard Library Status

Implementation status of the xcc Z80 C library (`lib/libc`) against the C23
standard library, plus the common POSIX/BSD/GNU extensions this target ships.

This is a status matrix. For the narrative discussion of partial headers and
recommended next steps see [LIBC-GAPS.md](LIBC-GAPS.md). For the planned
`long long` / `double` runtime helpers (which back the wide-integer and
floating-point library entry points) see [BIG-NUMBERS.md](BIG-NUMBERS.md).

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
`bin/include/z80/`.

| Header | State |
|--------|-------|
| `assert.h` | minimal (failure sink only) |
| `complex.h` | partial (core helpers) |
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
| `stdlib.h` | mostly complete (no float parsing / multibyte / env) |
| `string.h` | complete + extensions |
| `strings.h` | complete (BSD) |
| `tgmath.h` | partial (tracks `complex.h`/`math.h`) |
| `uchar.h` | partial (single-byte encoding) |
| `wchar.h` `wctype.h` | wide strings + classification |
| **`stdio.h`** | **missing entirely** |
| **`threads.h`** | **missing entirely** |
| `time.h` | implemented (all in assembly; sys clock hooks) |

---

## `<stdlib.h>`

| Function | Status | Notes |
|----------|--------|-------|
| `abs` `labs` `llabs` | asm, gcc-tested | |
| `div` | asm, gcc-tested | returns `{quot,rem}` in DE:HL |
| `ldiv` `lldiv` | C | 8-/16-byte struct return |
| `atoi` `atol` `atoll` | C | |
| `strtol` `strtoul` `strtoll` `strtoull` | C | |
| `rand` `srand` | C | |
| `malloc` `calloc` `realloc` `free` | C | fixed in-library heap |
| `bsearch` `qsort` | C | insertion-sort `qsort` |
| `abort` `exit` `_Exit` `atexit` | C | |
| `quick_exit` `at_quick_exit` | C | |
| `atof` `strtod` `strtof` `strtold` | **—** | no float string parsing |
| `getenv` `system` | **—** | no hosted environment |
| `mblen` `mbtowc` `wctomb` `mbstowcs` `wcstombs` | **—** | multibyte conversion |
| `aligned_alloc` | **—** | C11 aligned allocation |
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
| `strcasecmp` `strncasecmp` | POSIX |
| `strlcpy` `strlcat` | BSD |
| `strsep` | BSD |
| `strcasestr` | GNU |
| `swab` | POSIX |
| `bcopy` `bzero` `bcmp` `index` `rindex` | BSD (`strings.h`) |
| `ffs` `ffsl` `ffsll` | POSIX/GNU (`strings.h`) |

Not yet implemented: `memmem`, `strverscmp`, `basename`, `dirname`,
`strsignal`.

---

## `<ctype.h>`

Complete (ASCII), all **asm**:

`isalnum` `isalpha` `isblank` `iscntrl` `isdigit` `isgraph` `islower`
`isprint` `ispunct` `isspace` `isupper` `isxdigit` `tolower` `toupper`
`isascii` `toascii` *(`isascii`/`toascii` are asm, gcc-tested)*

---

## `<math.h>`

The largest gap in the library. Implemented:

Because `float`, `double`, and `long double` are all 32-bit on this target,
each assembly routine below exposes the `f`-suffixed, unsuffixed, and
`l`-suffixed names from one body (e.g. `truncf` / `trunc` / `truncl`).

| Function | Status |
|----------|--------|
| `fabs[f/l]` | C |
| `copysign[f/l]` | C |
| `sqrt[f/l]` | C |
| `atan2[f/l]` | C |
| `trunc[f/l]` | asm, gcc-tested (bit-exact) |
| `floor[f/l]` `ceil[f/l]` `round[f/l]` | asm, gcc-tested (bit-exact) |
| `ldexp[f/l]` `scalbn[f/l]` | asm, gcc-tested |
| `ilogb[f/l]` `logb[f/l]` | asm, gcc-tested |
| `frexp[f/l]` | asm, gcc-tested |
| `fmax[f/l]` `fmin[f/l]` | asm, gcc-tested |
| `fdim[f/l]` | asm, gcc-tested |
| `modf[f/l]` | asm, gcc-tested (bit-exact, signed zero) |
| `nan[f/l]` | asm, gcc-tested |
| `significand[f]` | asm, gcc-tested |
| classification helpers `__libc_fpclassifyf` / `signbit` / `isnan` / `isinf` / `isfinite` | C / header |

The header (`math.h`) is **finalized**: it declares the complete C23 interface
(trig, exp/log, power, error/gamma, nearest-integer, decomposition, remainder,
FMA, min/max/diff), defines all standard macros (`HUGE_VAL[F/L]`, `INFINITY`,
`NAN`, `FP_*`, `FP_ILOGB0/NAN`, `math_errhandling`, the `M_*` constants), and
provides the classification (`fpclassify`/`isfinite`/`isinf`/`isnan`/
`isnormal`/`signbit`) and comparison (`isgreater`/`isless`/`isunordered`/…)
macros.  It parses cleanly through `xcc`.

Missing implementations (declared in the header, not yet linkable):

- **Rounding (remaining):** `rint*`, `nearbyint*`, `lround*`, `llround*`,
  `lrint*`, `llrint*`.
- **Decompose / scale (remaining):** `scalbln*`, `nextafter*`.
- **Min/max/diff:** `fma*`.
- **Remainder:** `fmod*`, `remainder*`, `remquo*`.
- **Transcendental:** `sin* cos* tan* asin* acos* atan* sinh* cosh* tanh*`,
  `exp* exp2* expm1* log* log2* log10* log1p*`, `pow* hypot* cbrt*`,
  `erf*`, `tgamma*`, `lgamma*`.  These need polynomial kernels — the largest
  remaining math effort.

The single-precision soft-float runtime (`__fsadd`, `__fsmul`, `__fsdiv`,
`__fscmp`, …) and the new 64-bit `double` runtime (`__dbadd`, `__dbmul`,
`__dbdiv`, `__dbcmp`, conversions) already exist in `src/xc/xcc/lib/runtime/`,
so the rounding / decompose / min-max families are now achievable. `truncf`
is the proven template. The transcendental family needs polynomial kernels
and is a larger effort.

---

## `<time.h>`

Implemented **entirely in assembly** (`lib/libc/src/time/`), built on two
platform clock hooks supplied by the selected sys backend (`lib/sys/<sys>/`):

```
int __sys_gettimeofday(struct timespec *tv);        // read wall clock
int __sys_settimeofday(const struct timespec *tv);  // set  wall clock
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

Selectable backend: `make SYS=<os>` in `lib/libc/src` swaps `lib/sys/none` for
an OS backend.

---

## `<inttypes.h>`

| Function | Status |
|----------|--------|
| `imaxabs` | asm, gcc-tested |
| `imaxdiv` | C |
| `strtoimax` `strtoumax` | C |
| `PRI*` / `SCN*` format macros | header, complete |

`wcstoimax` / `wcstoumax` not implemented.

---

## `<wchar.h>` / `<wctype.h>`

Wide string/array family (16-bit `wchar_t`) — **C**:

`wcslen` `wcsnlen` `wcscpy` `wcsncpy` `wcscat` `wcsncat` `wcscmp` `wcsncmp`
`wcschr` `wcsrchr` `wcsspn` `wcscspn` `wcspbrk` `wcsstr` `wcstok`
`wmemchr` `wmemcmp` `wmemcpy` `wmemmove` `wmemset` `wctob`

Missing: wide I/O (`fwprintf` etc.), `wcscoll`/`wcsxfrm` beyond identity,
`wcsto*` numeric parsing, `mbrtowc`/`wcrtomb` state machines, the wide
counterparts of the new BSD/GNU string extensions (`wcpcpy`, `wcscasecmp`, …).

`<wctype.h>` provides the classification family against the 16-bit model.

---

## `<complex.h>`

Implemented (**asm/C**): `creal*` `cimag*` `conj*` `cabs*` `carg*`.

Missing: the transcendental complex family (`cexp` `clog` `cpow` `csqrt`
`csin` `ccos` `ctan` `csinh` …) and `cproj`.

---

## `<stdatomic.h>`

The C11 generic atomics dispatch (via the header) to the compiler-emitted
`__atomic_*` intrinsics (`__atomic_load_1/2`, `__atomic_store_1/2`,
`__atomic_exchange_1/2`, `__atomic_compare_exchange_1/2`,
`__atomic_fetch_{add,sub,and,or,xor}_1/2`, `__atomic_flag_*`).

These are **language-support code and live in the runtime**
(`src/xc/xcc/lib/runtime/atomic/`), not in `libc.a`. Only 1- and 2-byte
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
| `uchar.h` | `c16`/`c32` ↔ multibyte (single-byte) | shift-state / normalization |
| `stdbit.h` | full C23 bit utilities (header-inline) | — |
| `stdckdint.h` | `ckd_add/sub/mul` (compute only) | real overflow flag |

---

## Missing headers (entire)

- **`stdio.h`** — no formatted or stream I/O. Blocks `printf`, `assert`
  diagnostics to `stderr`, and most hosted code. Highest-impact gap.
- **`threads.h`** — no threading model.

---

## Test coverage

`tests/libc/` assembles the hand-written routines, links them at `0x0000`,
runs each in the xz80 emulator, and compares results to the host (gcc)
computation. Currently **36 test groups, all passing**, covering: `abs`
`labs` `llabs` `div` `imaxabs`; `stpcpy` `stpncpy` `mempcpy` `memrchr`
`strchrnul` `strcasecmp` `strncasecmp`; `bzero` `swab` `rawmemchr` `index`
`rindex` `bcopy` `bcmp` `strlcpy` `strlcat`; `ffs` `ffsl` `ffsll`;
`isascii` `toascii`; `truncf` `ldexpf`/`scalbnf` `ilogbf` `frexpf`
`fmaxf` `fminf` `floorf` `ceilf` `roundf` `nanf` `significandf` `logbf`
(and the `double`/`long double` aliases).

The C-implemented modules (`malloc`, `qsort`, `strtol`, wide-char, …) are not
yet covered by this harness.

---

## Suggested priority order

1. **`stdio.h`** (at least `snprintf`/`vsnprintf` + a `putchar` hook) — unblocks
   diagnostics, `assert`, and most hosted code.
2. **`math.h` rounding / decompose / min-max** — now unblocked by the float and
   double runtimes; `truncf` is the template. Single → double → long double.
3. **Float string conversion** (`strtod`/`strtof`/`atof`) — pairs naturally with
   `stdio.h` float formatting.
4. **`time.h`** once a clock source exists.
5. Remaining string/wide extensions (`memmem`, `strsep`, `wcpcpy`, …).
6. Transcendental `math.h` and `complex.h` kernels.
