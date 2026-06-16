# `none` — the libc retargeting template

This backend is **empty shells**. It defines every symbol the target-independent
C library needs from a platform, but each one does the minimum: console output
is discarded, there is no filesystem, the clock reads as the Unix epoch. A
program links and runs against it, but does nothing observable until you fill in
the hooks.

Use it as a starting point: **copy `lib/sys/none/` to `lib/sys/<your-target>/`,
implement the functions, and build with `PLATFORM=<your-target>`.**

The contract is declared in [`lib/sys/include/sys.h`](../include/sys.h) and the
clock half in `<time.h>`. All functions use SDCC's `sdcccall(1)` convention: the
first word-sized arguments arrive in **HL, DE, BC**, and results come back in
**DE** (or **DEHL** for a `long`). Backends may be written in C or assembly — the
symbols are identical. (The one exception is `heap_region`, which returns
two values in registers and must be assembly; see below.)

## The contract

| Symbol | C prototype | Must do | Returns |
|---|---|---|---|
| `_entry` | *(crt0)* | set the stack, zero `.bss`, copy the init image, call `main`, then `exit` | — |
| `_exit` | `void _exit(int status)` | end the program | does not return |
| `heap_region` | *(asm)* | report the heap's memory region | `HL`=base, `DE`=limit |
| `gettimeofday` | `int gettimeofday(struct timespec *tv)` | read the wall clock | `0`, or `-1` |
| `settimeofday` | `int settimeofday(const struct timespec *tv)` | set the wall clock *(optional)* | `0`, or `-1` |
| `open` | `int open(const char *p, int fl, int mode)` | open a file | fd ≥ 3, or `-1` |
| `close` | `int close(int fd)` | close a file | `0`, or `-1` |
| `read` | `int read(int fd, void *buf, unsigned n)` | read bytes (fd 0 = stdin) | count, `0`=EOF, `-1` |
| `write` | `int write(int fd, const void *buf, unsigned n)` | write bytes (fd 1/2 = console) | count, or `-1` |
| `lseek` | `long lseek(int fd, long off, int whence)` | reposition a file | new offset, or `-1` |
| `unlink` | `int unlink(const char *p)` | remove a file | `0`, or `-1` |
| `rename` | `int rename(const char *o, const char *n)` | rename a file | `0`, or `-1` |

### What ties to what

- **Console.** `puts`, `printf`, `putchar` all reduce to `write(1, …)`; `getchar`,
  `scanf` reduce to `read(0, …)`. Implementing just `write` makes output visible —
  that is normally the first hook you write. Some devices need newline → CR/LF
  translation; do it in `write` if yours does.
- **Files.** `fopen`/`fread`/`fwrite`/`remove`/`rename` sit on `open`/`read`/
  `write`/`lseek`/`close`/`unlink`/`rename`. A target with no
  filesystem leaves these as the failing shells and still has full console I/O.
- **Heap.** `malloc`/`free`/`calloc`/`realloc` need only `heap_region`,
  which hands the allocator one memory region to manage (there is no `sbrk`). See
  [RETARGET-LIBC.md](../../../docs/howtos/RETARGET-LIBC.md) §2.
- **Clock.** `time`, `clock`, `timespec_get` call `gettimeofday`.
- **Startup/exit.** `crt0.s` is the entry point; set `STACK_TOP` for your RAM.
  `_exit` ends the program (halt, or return to a monitor).

## Reference: empty C implementations

Equivalent C for every hook (what the assembly shells in this directory do).
Drop this into one `.c` file in your target directory, delete the shells you
replace with assembly, and fill in the `TODO`s.

```c
#include <sys.h>
#include <time.h>
#include <string.h>

/* ---- process ---------------------------------------------------------- */
void _exit(int status) {
    (void)status;
    for (;;) { /* TODO: return `status` to your supervisor, or halt */ }
}

/* ---- wall clock ------------------------------------------------------- */
int gettimeofday(struct timespec *tv) {
    tv->tv_sec  = 0;           /* TODO: seconds since 1970 from your RTC */
    tv->tv_nsec = 0;
    return 0;
}
int settimeofday(const struct timespec *tv) {
    (void)tv;
    return -1;                 /* TODO: program your RTC, return 0 on success */
}

/* ---- byte I/O --------------------------------------------------------- */
int write(int fd, const void *buf, unsigned len) {
    (void)fd;
    const unsigned char *p = buf;
    for (unsigned i = 0; i < len; i++) {
        /* TODO: send p[i] to your console device */
    }
    return (int)len;           /* claim everything was written */
}
int read(int fd, void *buf, unsigned len) {
    (void)fd; (void)buf; (void)len;
    return 0;                  /* TODO: read from your console; 0 = EOF */
}
int  open(const char *path, int flags, int mode) {
    (void)path; (void)flags; (void)mode;
    return -1;                 /* TODO: no filesystem */
}
int  close(int fd)                          { (void)fd; return 0; }
long lseek(int fd, long off, int whence)    { (void)fd; (void)off; (void)whence; return -1; }
int  unlink(const char *path)         { (void)path; return -1; }
int  rename(const char *o, const char *n) { (void)o; (void)n; return -1; }
```

`heap_region` returns *two* values in registers, so it stays in assembly
(`heap_region.s`); the template reserves a fixed static arena, which you size
with `SYS_HEAP_SIZE` or replace with the gap between the program end and the
stack.

## Building against your target

The toolchain selects the backend with `PLATFORM`. Once your directory exists:

```sh
make PLATFORM=<your-target> ...
```

`crt0.s` is assembled as the entry object and every other `*.s`/`*.c` in the
directory is archived into `lib<your-target>.a`, alongside the `linker.lk` /
`linker.ld` memory map you provide here.
