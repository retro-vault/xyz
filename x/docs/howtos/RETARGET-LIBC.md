# Retargeting the X Z80 libc

The target-independent library lives under `x/libc/src/`. A machine or
operating-system port supplies a small platform directory under
`x/platforms/<name>/`; libc itself does not change.

The platform contract is declared in
[`x/libc/include/sys.h`](../../libc/include/sys.h). The
[`none`](../../platforms/none/) backend is the starting template. CP/M 3 is a
hosted example, while [`zx-ram` and `zx-rom`](ZX-SPECTRUM-48K.md) demonstrate
bare hardware, a fixed RAM image, and a replacement ROM.

## Platform directory

A staged platform contains:

| File | Purpose |
|---|---|
| `crt0.s` | Entry point, stack setup, static initialization, `main`, and exit |
| `linker.lk` | SDCC-style memory map for xld |
| `linker.ld` | GNU-style memory map for xld |
| `_exit.s` | Non-returning process/firmware termination |
| `heap_region.s` | Default allocator arena, returned in HL/DE |
| `gettimeofday.s`, `settimeofday.s` | Optional wall-clock hooks |
| `open.s`, `close.s`, `read.s`, `write.s`, `lseek.s` | Descriptor I/O |
| `unlink.s`, `rename.s` | File removal and rename hooks |
| Other `*.s`/`*.c` | Target-private helpers pulled into the platform archive |

`crt0.s` remains a standalone startup object. Every other source in the
directory is archived as `lib<name>.a`. The two linker scripts are staged next
to it, allowing `--platform=<name>` to select the complete target contract.

This project conventionally implements shipped target support in Z80 assembly.
C prototypes below explain the ABI; they are not a requirement to implement
the backend in C.

## Create and stage a backend

```sh
cp -R x/platforms/none x/platforms/myboard
```

Edit the startup, memory map, and hooks, then stage it:

```sh
make -C x PLATFORM=myboard stage-xcc-support
```

Or rebuild the complete X prefix:

```sh
make -C x PLATFORM=myboard
```

The resulting files are:

```text
bin/x/z80/lib/crt0-myboard.rel
bin/x/z80/lib/crt0-myboard.s
bin/x/z80/lib/libmyboard.a
bin/x/z80/lib/linker-myboard.lk
bin/x/z80/lib/linker-myboard.ld
```

Use the backend through the driver:

```sh
bin/x/bin/xcc -Os --platform=myboard main.c -o app.xl
```

For fixed memory, select `--oformat=binary` and encode the address/range in the
platform linker scripts.

## Calling convention

Public XCC declarations use the native `[[sdcc::sdccall(1)]]` attribute
directly. Platform hooks use that calling convention:

XCC also accepts the historical extra-`c` attribute spelling for source
compatibility, but new headers and examples use the canonical spelling above.

| Value | Location |
|---|---|
| First 16-bit argument | HL |
| Second 16-bit argument | DE |
| Third 16-bit argument | BC |
| Further arguments | Stack (`4(ix)`, `6(ix)`, … after `push ix`) |
| `int` or pointer return | DE |
| `long` return | DE:HL, in the compiler's established word order |

A public C symbol `foo` is normally exported as `_foo::` in SDCC assembly.
`heap_region` is a deliberate special convention: return base in HL and the
one-past-end limit in DE.

## Startup contract

`crt0.s` must provide `_entry` and normally performs this sequence:

1. Establish a valid stack pointer.
2. Copy initialized storage to its run address.
3. Clear BSS.
4. Initialize target services needed before C runs.
5. Call `_main` using the selected ABI's argument contract.
6. Transfer `main`'s result to `_exit`/`exit` correctly.

A RAM target usually links code and writable storage together. A ROM target
needs distinct load and run addresses for writable initialized data. Xld's GNU
scripts express that as `>ram AT>rom`; SDCC scripts use `COPY _DATA`. Startup
then copies `s__DATA_LOAD`/`l__DATA_LOAD` to `s__DATA`. See the
[xld manual](../dist/man/XLD.md#rom-load-addresses).

## Heap

The allocator needs only this target decision:

```c
[[sdcc::sdccall(1)]] void heap_region(void);
/* returns HL=base, DE=one-past-end limit */
```

`malloc` lazily initializes the default heap over that range. The allocator,
block metadata, splitting/coalescing, `calloc`, `realloc`, aligned allocation,
and the C23 sized-free forms remain target-independent assembly.

A fixed-RAM board can reserve an `_HEAP` arena. A hosted or linked-image target
can place a zero-sized `_HEAP` marker after BSS and return the gap from that
symbol to a stack reserve. The ZX backends use the latter pattern and stop at
`0xF000`.

## Clock

```c
[[sdcc::sdccall(1)]] int gettimeofday(struct timespec *tv);
[[sdcc::sdccall(1)]] int settimeofday(const struct timespec *tv);
```

Return `0` after reading/writing the clock or `-1` when the service is absent.
The target-independent `time`, `clock`, and `timespec_get` functions propagate
that result. A no-clock target should fail explicitly, as the ZX backends do;
it should not invent the Unix epoch.

X uses a 32-bit `time_t`. There is currently no timezone/DST platform hook;
local time follows the library's documented UTC behavior.

## Console and files

Descriptors follow the usual convention: 0 is stdin, 1 is stdout, 2 is
stderr, and opened files begin at 3.

```c
[[sdcc::sdccall(1)]] int open(const char *path, int flags, int mode);
[[sdcc::sdccall(1)]] int close(int fd);
[[sdcc::sdccall(1)]] int read(int fd, void *buffer, unsigned length);
[[sdcc::sdccall(1)]] int write(int fd, const void *buffer, unsigned length);
[[sdcc::sdccall(1)]] long lseek(int fd, long offset, int whence);
[[sdcc::sdccall(1)]] int unlink(const char *path);
[[sdcc::sdccall(1)]] int rename(const char *old_path, const char *new_path);
```

The target-independent stdio layer reduces console and file work to these
hooks:

- `puts`, `printf`, `putchar`, `fwrite`, and formatter output use `write`;
- `getchar`, `scanf`, `fgets`, and stream input use `read`;
- `fopen`/`freopen` use `open` and `close`;
- `fseek`, `ftell`, and `rewind` use `lseek`;
- `remove` uses `unlink` and `rename` uses the matching platform hook.

A console-only backend implements `read(0, ...)`, `write(1, ...)`, and
`write(2, ...)`, closes descriptors 0–2 successfully, and returns `-1` for file
descriptors and file operations. That still enables formatted stdin/stdout and
the rest of non-file libc.

## Exit

```c
void _exit(int status); /* never returns */
```

On an OS, return the status to the supervisor. On firmware, record it if useful
and stop or reset the machine. The ZX platforms expose `zx_exit_status`, then
disable interrupts and HALT permanently.

## Complete checklist

- [ ] Copy `x/platforms/none/` and rename the directory.
- [ ] Define the memory regions and entry point in both linker-script dialects.
- [ ] Set the stack and implement initialized-data/BSS startup.
- [ ] Return a valid non-overlapping heap range.
- [ ] Implement descriptor 0/1/2 behavior required by the target.
- [ ] Return explicit failures for unavailable file and clock services.
- [ ] Implement non-returning `_exit`.
- [ ] Stage with `make -C x PLATFORM=<name> stage-xcc-support`.
- [ ] Compile a small C program with `--platform=<name>`.
- [ ] Test initialized storage, BSS, heap, stdio, errors, and exit in a real
      emulator or on hardware.

## Shipped reference backends

| Backend | Character I/O | Files | Clock | Memory/startup |
|---|---|---|---|---|
| `none` | Discard/EOF shells | No | No useful clock | Template fixed arena |
| `emu` | Host-mapped emulator ports | Host test mapping | Platform test behavior | Emulator image |
| `cpm3` | BDOS console and `kbhit()` | CP/M descriptors | Platform behavior | Transient program area |
| `zx-ram` | Matrix `kbhit()`, blocking stdio/Tamsyn bitmap | No | No | `0x5CCB`, heap below `0xF000` |
| `zx-rom` | Matrix `kbhit()`, blocking stdio/Tamsyn bitmap | No | No | 16 KiB ROM, writable RAM at `0x5B00` |
