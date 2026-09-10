# ZX Spectrum esxDOS disk platform

`--platform=zx-esxdos` builds a 48K Spectrum RAM program with basic
POSIX-style disk functions backed by esxDOS 0.8.9 on divIDE or compatible
hardware. Boot the machine normally with its Sinclair ROM and esxDOS before
starting the program. The target calls the resident firmware through its
`RST 0x08` API and uses the current esxDOS drive and working directory.

## Build and load

```sh
xcc -Os --platform=zx-esxdos --oformat=binary main.c -o APP.BIN
```

Copy `APP.BIN` to the esxDOS disk. At the BASIC prompt:

```text
CLEAR 32767
LOAD *"APP.BIN" CODE 32768
RANDOMIZE USR 32768
```

`CLEAR` keeps BASIC's stack below the application. The `*` requests the disk
explicitly, including when a TAP image is attached. `LOAD ... CODE` accepts
the raw, headerless binary at the address shown. The entry address is fixed
at `0x8000` (32768).

The repository's `x/examples/zx-esxdos/diskio.c` creates `XCCDISK.TXT`, writes
and flushes a message, seeks back, reads and verifies it, and closes the
file. It leaves that file on disk for inspection.

## File API

Include `<fcntl.h>` for open flags, `<unistd.h>` for descriptors, and
`<sys/stat.h>` for metadata. The basic interface is:

| Function | Operation |
|---|---|
| `open(path, flags)` | Open a file; return a descriptor at least 3 |
| `close(fd)` | Close a file |
| `read(fd, buffer, count)` | Read bytes; return the actual count, or zero at EOF |
| `write(fd, buffer, count)` | Write bytes; return the actual count |
| `lseek(fd, offset, whence)` | Seek using `SEEK_SET`, `SEEK_CUR` or `SEEK_END` |
| `fsync(fd)` | Ask esxDOS to flush the file |
| `fstat(fd, info)`, `stat(path, info)` | Query file metadata |
| `unlink(path)` | Remove a file |
| `rename(oldpath, newpath)` | Rename a file; declared by `<stdio.h>` |
| `chdir(path)`, `getcwd(buffer, size)` | Select or query the current directory |
| `mkdir(path, mode)`, `rmdir(path)` | Create or remove a directory |

XCC's `open` takes exactly two arguments. Supported access flags include
`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`, `O_APPEND`, and
`O_EXCL` together with `O_CREAT`. `O_EXCL` without `O_CREAT` fails with
`EINVAL`.
Permission arguments are accepted where declared for source compatibility;
esxDOS FAT files do not provide Unix ownership or permissions.

Descriptors 0, 1 and 2 remain the keyboard, screen output and screen error
output. Disk descriptors are mapped separately from esxDOS's raw handles.
The backend reserves 16 disk slots; firmware resources may impose a lower
limit. Failed operations return `-1` and set `<errno.h>`'s `errno`; `getcwd`
returns a null pointer on failure.
The ordinary libc `fopen`, `fread`, `fwrite`, `fseek`, `fflush`, `fclose`,
`remove` and file-formatted I/O use these platform hooks automatically.
The current libc writes through to the backend; `fflush` does not replace
`fsync`. Use `fsync(fd)` or `close`/`fclose` to flush firmware state.

This is a small filesystem interface, with firmware behavior where it
differs from POSIX. In particular, `rename` fails when the destination
already exists; it does not atomically replace an existing file. Paths and
metadata follow esxDOS/FAT semantics. `lseek` reports the actual resulting
position, including firmware clamping at EOF; sparse files are not promised.
File positions and sizes must fit signed 32-bit `off_t` (at most
2,147,483,647 bytes). Each read/write transfers at most 32,767 bytes, even
when a larger count is requested; callers should handle short transfers.
A native firmware error returns `-1`: its register contents do not provide
a reliable partial byte count, even if the device performed some I/O.

Path arguments must be nonempty NUL-terminated RAM strings of at most 255
bytes. Data buffers must also lie in RAM, outside the firmware ROM window,
without wrapping past `0xFFFF`. `getcwd` requires caller-provided storage and
returns `ERANGE` if it is too small. `stat`/`fstat` provide type, size and FAT
read-only status through the small existing `struct stat`; they do not add
Unix owners, timestamps or persistent inode identities. Directory sizes are
reported as zero, translating esxDOS's `0xFFFFFFFF` directory sentinel.

File access requires resident esxDOS;
`zx-ram` and `zx-rom` retain their existing console-only behavior.

`<sys/esxdos.h>` declares `zx_esxdos_version()` and the descriptor/path limits.
The version query reports the initialized firmware's BCD version; it is not
a probe for absent hardware or uninitialized firmware.

## Runtime and memory

| Address range | Use |
|---|---|
| `0x0000`–`0x3FFF` | Sinclair ROM and esxDOS firmware mapping |
| `0x4000`–`0x5AFF` | Bitmap display and attributes |
| `0x5B00`–`0x7FFF` | Active loader/BASIC storage before entry; available for explicit application use afterward |
| `0x8000`–linked end | Program, constants and writable static storage |
| linked end–`0xEFFF` | libc heap |
| `0xF000`–`0xFFFF` | Default descending stack allowance |

Startup disables interrupts, sets SP to `0xFFFF` and IY to the Sinclair
system-variable base `0x5C3A`, initializes C storage and the bitmap console,
and calls `main`. The console and keyboard scanner are the same assembly
implementations used by the other X Spectrum targets, including non-blocking
`trygetchar()` and the proportional Tamsyn font.

The supported external file calls in stock esxDOS 0.8.9 do not require a
permanent Spectrum workspace reservation. Internal filesystem buffers and
state occupy divIDE RAM; ordinary calls use the application's stack and
provided buffers. The default load address remains `0x8000` to avoid the
active loader. Once entry has disabled interrupts and installed its own
stack, the application can reuse `0x5B00`–`0x7FFF`, including the old system
variables. The supplied `x/examples/zx-esxdos/lowram.c` demonstrates a
private heap arena there and a 9,216-byte disk round trip. The default libc
heap still begins after the linked image; low RAM is available explicitly.
After reclaiming it, do not return to BASIC, invoke its ROM services or
enable its stock interrupt handler.

Both esxDOS targets share a 4 KiB default stack allowance. It is not an
esxDOS minimum. The heap limit is the linker `_STACK` boundary. To change
it, copy the installed script and use `-T`: update `AREA _STACK = F000`
and `RESERVE F000-FFFF` in compact syntax, or `_STACK 0xF000`, `RESERVE`
and the RAM region length in GNU syntax. For a 2 KiB allowance use boundary
`F800`/`0xF800` and GNU RAM length `0x7800` with this target's `0x8000`
origin. Initial SP remains `0xFFFF`. Budget for the actual application,
libc and wrappers sharing that stack.

Returning from `main` calls the ordinary freestanding exit path, which
records the status, attempts to close every open disk descriptor (3–18),
and halts. A failed close does not prevent attempts to close the remaining
descriptors. The target does not return to BASIC or build
esxDOS dot commands. A wall-clock interface is not supplied.

The four final low-arena example images, covering `-Os`/`-Of` and both
caller ABIs, match successful actual BASIC-loader boots. The companion ROM
target passes 67 stock-firmware cold boots with FAT16/FAT32; all 18
deterministic disk ABI lanes pass. All four original Spectrum MCP modes
also pass with the final staged tools. The repository's
`compact-ram-validation-2026-09.json` records these results, the configurable
stack checks and final tool identities. Earlier disk/ROM validation records
retain their original layouts and tool identities.

## References

The firmware interface follows the
[z88dk esxDOS implementation](https://github.com/z88dk/z88dk/tree/master/libsrc/target/zx/esxdos)
and the esxDOS 0.8.9 API. The disk-explicit `LOAD *"..." CODE` syntax is
[documented by the firmware author](https://board.esxdos.org/viewtopic.php?id=106).
