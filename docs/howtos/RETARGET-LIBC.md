# Retargeting the xcc Z80 libc

The libc in `lib/libc/` is **target-independent**. Every routine — the calendar
math, the string and wide-string families, the soft-float math, the formatted
I/O that *will* sit in `<stdio.h>` — is written so that the only target-specific
code is a small **platform layer**. Porting the library to a new board or
operating system means implementing that layer; nothing in `lib/libc/src/`
changes.

This document lists the hooks the platform layer must provide, grouped by the
standard-library feature they unlock:

1. **Clock** — two hooks, and the whole of `<time.h>` works. *(Implemented.)*
2. **Heap** — one hook (or two symbols) gives the program its dynamic memory,
   and `malloc`/`calloc`/`realloc`/`free` work.
3. **Console character I/O** — output and input of one byte, and
   `putchar`/`getchar`/`puts`/`printf`/`scanf` work.
4. **File / disk I/O** — a handful of stream calls, and the rest of `<stdio.h>`
   (`fopen`/`fread`/`fwrite`/`fseek`/…) works.
5. **Startup** — `crt0` and process exit.

---

## The platform-backend mechanism

Platform hooks live in **`lib/sys/<backend>/`**, one assembly file per hook
(same one-function-per-file rule as the rest of the library). The same backend
directory is also the right home for platform startup and memory-map files such
as `crt0.s`, `linker.ld`, and `linker.lk`. The backend is selected at build
time:

```sh
make -C lib/libc/src SYS=none     # bare-metal default (stubs)
make -C lib/libc/src SYS=myos     # pull in lib/sys/myos/ instead
```

The libc `Makefile` globs `lib/sys/$(SYS)/*.s` (and `*.c`) and archives those
objects into `libc.a` alongside the target-independent code. It deliberately
excludes `crt0.s`, which remains a standalone startup object. The shipped
**`none`** backend provides empty/identity stubs so the library links and runs
(clock reads as the epoch, etc.) on bare metal with no OS.

To create a backend, copy `lib/sys/none/` to `lib/sys/<youros>/` and fill in the
bodies. Add `crt0.s` and optional linker scripts there too if the target needs
its own startup or memory map. You only need the hooks for the features you
want; an unused hook can be left as the `none` stub.

### Calling convention

All hooks use the compiler's `sdcccall(1)` convention, the same as the rest of
libc:

| Argument / result | Register(s) |
|-------------------|-------------|
| 1st arg, 16-bit (e.g. a pointer) | `HL` |
| 1st arg, 32-bit (`long`) | `DE:HL` (`DE`=low, `HL`=high) |
| 2nd arg, 16-bit | `DE` |
| further args | on the stack, at `4(ix)`, `6(ix)`, … after `push ix` |
| `int` / pointer return | `DE` |
| `long` return | `DE:HL` (`DE`=low, `HL`=high) |

Public C symbol `foo` is the assembly label `_foo::`.

---

## 1. Clock — `<time.h>` *(implemented)*

The entire `<time.h>` — `time`, `clock`, `difftime`, `mktime`,
`gmtime[_r]`, `localtime[_r]`, `asctime[_r]`, `ctime[_r]`, `strftime`,
`timespec_get` — is built **in assembly** on top of exactly two hooks:

```c
int __sys_gettimeofday(struct timespec *tv);        /* read the wall clock */
int __sys_settimeofday(const struct timespec *tv);  /* set  the wall clock */
```

(All sys-layer hooks carry the `__sys_` prefix; the assembly labels are
`___sys_gettimeofday::` / `___sys_settimeofday::`.)

`struct timespec { time_t tv_sec; long tv_nsec; }` (8 bytes: seconds at offset
0, nanoseconds at offset 4; both 32-bit). The hook writes/reads the whole
struct; on a seconds-only clock just set `tv_nsec = 0`.

See `lib/sys/none/sys_gettimeofday.s` / `sys_settimeofday.s` for the stub shape. An OS
replaces them to read its RTC, and every calendar/formatting function comes to
life with no other change. This two-hook design is the template the rest of
this document follows.

> Note: there is no timezone/DST model — local time equals UTC. `time_t` is a
> signed 32-bit count of seconds (valid roughly 1902–2038).

---

## 2. Heap — `malloc`, `calloc`, `realloc`, `free`

Dynamic allocation needs one target-specific decision: **where the heap lives
and how big it is**. The allocator itself — the free-list bookkeeping, block
splitting and coalescing behind `malloc`/`calloc`/`realloc`/`free` — is
target-independent and stays in libc; the platform only provides the raw
memory.

The recommended hook is the classic break-pointer call:

```c
void *__sys_sbrk(int increment);   /* grow the heap by `increment` bytes;
                                      returns the PREVIOUS break, or
                                      (void *)-1 if the request cannot be met */
```

`malloc` calls `__sys_sbrk` to obtain memory in coarse chunks and then hands out
sub-blocks from them; `free` returns blocks to libc's internal free list (it
does not shrink the system break). `increment` is a signed 16-bit byte count, so
a backend may also support handing memory back with a negative argument, but it
is not required — returning `(void *)-1` for any shrink request is fine.

```
malloc / calloc / realloc ─►  (libc free-list allocator)  ──► __sys_sbrk(+n)
free                       ─►  (returns to the free list)
```

### `none` backend: a fixed static arena

With no OS there is no real memory manager, so the bare-metal `__sys_sbrk` bumps
a pointer through a statically reserved block and reports exhaustion once it is
used up:

```c
/* lib/sys/none/sys_sbrk.s — contract shown in C */
#define __SYS_HEAP_SIZE 8192U
static unsigned char __sys_heap[__SYS_HEAP_SIZE];
static unsigned char *__sys_brk = __sys_heap;

void *__sys_sbrk(int increment) {
    unsigned char *prev = __sys_brk;
    unsigned char *next = prev + increment;
    if (next < __sys_heap || next > __sys_heap + __SYS_HEAP_SIZE)
        return (void *)-1;            /* out of heap */
    __sys_brk = next;
    return prev;
}
```

Change `__SYS_HEAP_SIZE` to size the arena for the board.

### Alternative: linker-defined bounds

If you would rather have the **linker/`crt0`** place the heap (typically the gap
between the end of `.bss` and the stack), expose two symbols instead of a hook —
`__heap_start` and `__heap_top` — and let `__sys_sbrk` bump between them. This
keeps the heap out of the static image (it costs no ROM/file size, only RAM) and
lets each link decide the size. Either approach satisfies the allocator; pick
whichever matches how the target lays out memory.

> Current state: the in-tree `malloc` (still C, in `stdlib.c`) uses a baked-in
> 8 KB static arena rather than this hook. Routing it through `__sys_sbrk` is
> part of the pending `stdlib.c` → assembly conversion; the hook above is the
> intended retargeting surface.

---

## 3. Console character I/O — `putchar`, `getchar`, `printf`, `scanf`

`<stdio.h>` is **not implemented yet**; this section specifies the platform
layer it will be built on, so a backend can be written ahead of time. The
design mirrors the clock layer: the standard, target-independent code (FILE
buffering, the `printf`/`scanf` conversion engine) lives in libc; the platform
provides only the raw byte transfer.

The recommended layer is the classic POSIX trio, on pre-opened descriptors
`0`=stdin, `1`=stdout, `2`=stderr:

```c
int __sys_write(int fd, const void *buf, unsigned len); /* bytes written, or -1 */
int __sys_read (int fd, void *buf, unsigned len);       /* bytes read, 0 = EOF, -1 = err */
```

Everything character-oriented reduces to these two:

```
putchar / putc / puts / fputs ─┐
printf / fprintf / vprintf  ───┼─►  (FILE buffer)  ──► __sys_write(1, buf, n)
perror                       ──┘                        __sys_write(2, buf, n)

getchar / getc / fgets / gets ─┐
scanf / fscanf / vscanf     ───┴─►  (FILE buffer)  ◄── __sys_read(0, buf, n)
```

**Minimum to get program output and input working:** implement `__sys_write`
for `fd == 1`/`2` (route to the console/UART) and `__sys_read` for `fd == 0`
(route to the keyboard/UART). A board with only a single-byte console can
implement them as thin loops over a one-byte primitive:

```c
/* board UART/console primitives (whatever the hardware needs) */
extern void __board_putc(char c);
extern int  __board_getc(void);   /* blocking; -1 on EOF */

int __sys_write(int fd, const void *buf, unsigned len) {
    const unsigned char *p = buf;
    (void)fd;                       /* 1 and 2 both go to the console */
    for (unsigned i = 0; i < len; ++i) __board_putc((char)p[i]);
    return (int)len;
}

int __sys_read(int fd, void *buf, unsigned len) {
    unsigned char *p = buf;
    unsigned i;
    (void)fd;
    for (i = 0; i < len; ++i) { int c = __board_getc(); if (c < 0) break; p[i] = (unsigned char)c; }
    return (int)i;
}
```

(Write these in assembly under `lib/sys/<youros>/` for the shipped library;
the C above is just the contract.) With those two hooks, all of
`putchar`/`getchar`/`puts`/`printf`/`scanf` will work — exactly as the clock
hooks light up all of `<time.h>`.

---

## 4. File / disk I/O — the rest of `<stdio.h>`

To support real files (`fopen`, `fread`, `fwrite`, `fseek`, `fclose`, …) the
platform layer grows three more calls. The FILE layer, buffering, and the
`f*` wrappers stay target-independent; the backend just maps a path to a
descriptor and moves blocks:

```c
int  __sys_open (const char *path, int flags, int mode); /* fd >= 0, or -1 */
int  __sys_close(int fd);                                /* 0, or -1 */
long __sys_lseek(int fd, long offset, int whence);       /* new offset, or -1 */
```

with `__sys_read` / `__sys_write` (section 3) doing the transfers. `flags` and
`whence` follow the usual POSIX values (`O_RDONLY`/`O_WRONLY`/`O_RDWR`/`O_CREAT`
/`O_TRUNC`/`O_APPEND`; `SEEK_SET`/`SEEK_CUR`/`SEEK_END`).

```
fopen   ──► __sys_open            fread/fgetc ──► __sys_read
fclose  ──► __sys_close           fwrite/fputc ─► __sys_write
fseek/ftell/rewind ─► __sys_lseek
```

**"Basic disk reading"** is precisely `__sys_open` + `__sys_read` (+ `__sys_lseek`
for random access). A read-only data backend can implement just those three and
stub `__sys_write`/`__sys_close` (or have `__sys_open` reject write modes). How
those map onto hardware — a FAT driver, a raw sector device, a ROM filesystem —
is entirely the backend's business; libc never sees it.

### Suggested descriptor map

| fd | stream | backed by |
|----|--------|-----------|
| 0 | `stdin`  | console input  (`__sys_read`) |
| 1 | `stdout` | console output (`__sys_write`) |
| 2 | `stderr` | console output (`__sys_write`) |
| ≥3 | `fopen`ed files | `__sys_open` |

`stdin`/`stdout`/`stderr` are pre-opened by `crt0`/libc startup, so a program
gets working I/O before its first `fopen`.

---

## 5. Startup — `crt0` and exit

A complete target also needs startup/teardown code (conventionally `crt0.s`,
provided per-target — see `tests/hello/crt0.s` for a worked example). Its
responsibilities:

1. Set the stack pointer.
2. Zero `.bss` (the `_DATA`/`_INITIALIZED` area) and copy any initialized data.
3. (Optional) pre-open `stdin`/`stdout`/`stderr` and initialize the clock.
4. Call `main(argc, argv)`.
5. On return (or `exit`), run `atexit` handlers and halt / return to the OS.

The process-exit path also expects a way to stop. `exit`/`_Exit`/`abort` (in
`<stdlib.h>`) ultimately need a platform "halt or return to monitor" action; on
`none` this is a `HALT`. If you want a distinct hook, expose:

```c
void __sys_exit(int status);   /* never returns */
```

and have `crt0`/`_Exit` tail-call it.

---

## Checklist for a new backend

Create `lib/sys/<youros>/` and provide, as needed:

| Feature you want | Hooks to implement |
|------------------|--------------------|
| `<time.h>` | `__sys_gettimeofday`, `__sys_settimeofday` |
| `malloc`/`calloc`/`realloc`/`free` | `__sys_sbrk` (or `__heap_start`/`__heap_top`) |
| program output (`printf`, `putchar`, `puts`) | `__sys_write` (fd 1/2) |
| program input (`scanf`, `getchar`) | `__sys_read` (fd 0) |
| file reading | `__sys_open`, `__sys_read`, `__sys_lseek`, `__sys_close` |
| file writing | the above + `__sys_write` (fd ≥ 3) |
| clean exit | `__sys_exit` (or rely on the `crt0` `HALT`) |

Then build with `make -C lib/libc/src SYS=<youros>`. Anything you omit can stay
as the `none` stub, so a board can start with just `__sys_write` and grow from
there.

---

## Status summary

| Layer | Hooks | State |
|-------|-------|-------|
| Clock (`<time.h>`) | `__sys_gettimeofday`, `__sys_settimeofday` | **implemented** (`lib/sys/none/`) |
| Heap (`malloc`/…) | `__sys_sbrk` | **proposed** — `malloc` still uses a fixed static arena |
| Console (`printf`/`scanf`) | `__sys_write`, `__sys_read` | **proposed** — `<stdio.h>` not yet built |
| Files (`fopen`/`fread`/…) | `__sys_open`/`close`/`lseek` | **proposed** — `<stdio.h>` not yet built |
| Startup / exit | `crt0`, `__sys_exit` | per-target `crt0` exists; exit hook proposed |

The clock layer is the reference implementation: study `lib/sys/none/` and the
`lib/libc/src/time/` modules to see how a tiny platform surface supports an
entire standard header, then apply the same pattern to console and file I/O as
`<stdio.h>` lands.
