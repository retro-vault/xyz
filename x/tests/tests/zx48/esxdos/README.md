# ZX Spectrum esxDOS disk tests

These tests exercise the self-contained `zx-esxdos` RAM and
`zx-esxdos-rom` replacement-ROM targets. They require a staged toolchain
containing those platforms:

```sh
make -C x stage-includes stage-xcc-support
python3 x/tests/tests/zx48/esxdos/run_abi.py \
  --work build/zx-esxdos/abi-validation
```

`run_abi.py` builds an independent Z80 firmware-call model with `libxz80`,
then compiles the public C probe at `-O0`, `-Os`, and `-Of`, with both default
caller ABIs. Public filesystem functions retain their explicit `sdcccall(1)`
ABI. Every native call clobbers all undocumented outputs, IX, IY and the
alternate register bank. Every direct call checks stack and IX/IY preservation.
The model uses native esxDOS handle numbers that differ from public descriptors.

For the ROM target, select its platform explicitly. This runs `-Os` and
`-Of` with both caller ABIs, checks hostile register clobbers and verifies
that application ROM remains unchanged:

```sh
python3 x/tests/tests/zx48/esxdos/run_abi.py \
  --platform=zx-esxdos-rom --work build/zx-esxdos/rom-abi-validation
```

The shared C probe covers firmware version, create/open/exclusive/truncate,
read/write access, EOF and short reads, append after seeking, flush/close,
32-bit positions beyond 64 KiB, negative relative seeks, standard C
streams, rename/remove, file/directory metadata, directories and bounded
`getcwd`. Focused checks add signed offset boundaries, all 256 firmware error
codes, invalid descriptors and RAM spans, a full descriptor table, metadata
overflow, malformed firmware strings and exit-time closing, including one
failed close that must not prevent the remaining closes.

The model represents the real firmware's directory-size sentinel
`0xffffffff`; public directory `st_size` is zero. It poisons F_SEEK's BCDE
output so a wrapper cannot accidentally rely on undocumented return values.

For real firmware validation, use ZEsarUX with a matching esxDOS 0.8.9 ROM
and an IDE image containing that release's `SYS` directory:

```sh
python3 x/tests/tests/zx48/esxdos/run_firmware.py \
  --zesarux /path/to/zesarux \
  --rom /path/to/esxdos089/ESXIDE.BIN \
  --ide /path/to/divide-esxdos089.ide \
  --profile=-Os --work build/zx-esxdos/firmware-size
```

Repeat with `--profile=-Of` for the speed profile. ZEsarUX runs its actual
Z80 esxDOS ROM and emulated divIDE/IDE hardware; its host filesystem handler
is disabled by default and is never enabled by this harness. The supplied
IDE image is copied into the new work directory before starting, so the
source image is preserved. The test creates/removes only `XDISK.TMP`,
`XSTDIO.TMP`, `XMOVED.TMP`, and `XTESTDIR` inside that private copy.

Each work directory must be new. The firmware runner records tool/image
hashes, exact commands, the debugger transcript and a failure memory dump
when needed. The ABI runner records all six commands and results. The
firmware test complements the deterministic model; passing the model alone
does not demonstrate real firmware or physical hardware compatibility.

The September 2026 validation used ZEsarUX source commit
`f187c7ebeff3e56a23fa3636271f54734f5fa16e` and official esxDOS 0.8.9
([firmware archive](https://www.esxdos.org/files/esxdos089.zip), SHA-256
`d455888361cd13d455e362a4c841452b6f4568d30f9069443044affdd40de9a4`).
The matching `ESXIDE.BIN` SHA-256 is
`771d5f21de48a3e9969adef7708ddb9a2834ba8729c5f702e5c0bb6067ebaa57`.
ZEsarUX's official 13.0 extras FAT16 image was copied and its `SYS` and `BIN`
files replaced with the official 0.8.9 files, using `mcopy` at the image's
1 MiB partition offset. The test tools, firmware and disk images remain
local build inputs; they are not redistributed with X Tools.

The [September validation record](validation-2026-09.json) contains the exact
source and firmware identities and retained evidence. All 18 S/M/L model,
optimization-profile and caller-ABI lanes pass. The real-firmware C probe
passes in Os and Of, as do all four example/profile/ABI combinations. The
four existing Spectrum MCP modes and the staged platform/package inventory
also pass; this is not a claim that a Debian archive was rebuilt.

## Cold boot from an application ROM

`run_rom_firmware.py` compiles a `zx-esxdos-rom` application and starts a
fresh 48K machine with that 16 KiB image as its only base ROM, actual esxDOS
0.8.9 firmware, and emulated divIDE/IDE. It supplies no Sinclair ROM, snapshot,
CPU/register rewrite, RAM injection or host filesystem handler. The disk is
copied privately, and its matching esxDOS configuration must set `AutoBoot=0`.
The runner requires application code and constants in ROM, exactly 48 bytes
of RAM syscall gates, and ROM-to-RAM copying restricted to writable data.
It rejects the earlier whole-application RAM copy design.

```sh
python3 x/tests/tests/zx48/esxdos/run_rom_firmware.py \
  --zesarux /path/to/zesarux \
  --rom /path/to/esxdos089/ESXIDE.BIN \
  --ide /path/to/divide-esxdos089.ide \
  --profile=-Os --abi=1 \
  --work build/zx-esxdos/rom-cold-size
```

The default source is the 65-phase filesystem probe. Select `--profile=-Of`
for speed, `--abi=0` for the alternate caller convention, or `--mode=gnu` for
actual GNU/ELF object generation. Use `--xcc bin/x-s/bin/xcc` or
`--xcc bin/x-l/bin/xcc` for the other model prefixes. Every run needs a new
work directory.

To exercise initialization and a 10 KiB ROM constant payload, add the current
C and assembly fixtures to the same command:

```sh
python3 x/tests/tests/zx48/esxdos/run_rom_firmware.py \
  --zesarux /path/to/zesarux \
  --rom /path/to/esxdos089/ESXIDE.BIN \
  --ide /path/to/divide-esxdos089.ide \
  --profile=-Os --abi=1 \
  --source x/tests/tests/zx48/esxdos/rom_xip_startup.c \
  --asm x/tests/tests/zx48/esxdos/rom_xip_startup.s \
  --work build/zx-esxdos/rom-xip-startup
```

This checks writable initial values, a pointer into ROM, zero-initialized
storage, an additional GSINIT contribution, all 10 KiB of constant data,
and a firmware call after initialization. The original 12 KiB
`rom_startup.c`/`.s` pair remains historical: its contiguous constant section
does not fit the current ROM layout's reserved paging holes.

To exercise ROM pathnames, ROM writes across 128-byte buffer boundaries,
mixed ROM/RAM rename arguments, append, metadata and writable-buffer checks:

```sh
python3 x/tests/tests/zx48/esxdos/run_rom_firmware.py \
  --zesarux /path/to/zesarux \
  --rom /path/to/esxdos089/ESXIDE.BIN \
  --ide /path/to/divide-esxdos089.ide \
  --profile=-Os --abi=1 \
  --source x/tests/tests/zx48/esxdos/rom_io.c \
  --work build/zx-esxdos/rom-xip-inputs
```

For the public disk example, use
`--source x/examples/zx-esxdos-rom/diskio.c`. The runner checks the actual
base-ROM contents, the normal exit marker and expected file contents on the
private disk. It also checks that the final mapped base ROM and RAM gates
match their compiled images and that termination executes in ROM. Each
report retains compiler/emulator commands, input hashes, ROM/map hashes and
the debugger transcript.

The preceding direct-ROM layout passed 43 distinct cold-boot configurations: 15 native
M, 20 native S/L and eight GNU M. Coverage includes the 65-phase filesystem
probe, 49-phase ROM-input probe, both caller ABIs, all three example profiles
and the 10 KiB startup fixture. The deterministic firmware model passed
12 ROM lanes (S/M/L × Os/Of × ABI0/1), each with 553 checks and 891 hostile
calls, plus six unchanged M RAM lanes with 523 checks and 855 calls each.
All four original Spectrum MCP modes also passed with the then-recorded
compiler and linker.

All 43 images rebuilt with the then-recorded tools matched their successful
firmware executions byte for byte. The oversized-code and historical
12 KiB contiguous-constant fixtures were correctly rejected by that linker.

All 18,047 canonical compiler variants passed with unchanged frozen inputs
and their recorded linker identity. The later linker guard-placement
correction separately passed all 103 normal and ASan/UBSan component tests
and a four-mode Spectrum MCP run with that recorded build.
Its pinned z88dk24 `-Os` and `-Of` reruns passed 24/24 each, with all 48
size/cycle measurements unchanged. The
[direct ROM execution record](xip-validation-2026-09.json) records the
consolidated results and separate tool identities for that earlier layout.

The [earlier September ROM validation record](rom-validation-2026-09.json)
covers 27 configurations of the superseded whole-application RAM-copy design.
Its results, including the larger relocation fixture and host-tool
regressions, remain historical evidence.

## Reclaimed low RAM and configurable stack

The ROM target now places writable storage at `0x5B00`, recovering all
9,472 bytes formerly left below `0x8000`. Supported external file calls in
stock esxDOS 0.8.9 use the divIDE's internal RAM for filesystem buffers and
state; they need no permanent Spectrum workspace after boot. Application
buffers, call frames and the bounded ROM-input copies remain application
RAM costs. The RAM target still loads at `0x8000`, but may explicitly use
`0x5B00`–`0x7FFF` after entry has finished the BASIC loader transition.

The new validation distinguishes these claims by tracing actual firmware
reads/writes and stack pointers, overwriting the entire reclaimed range,
and checking the supported file operations with data held there. The
RAM example `x/examples/zx-esxdos/lowram.c` uses a 9,216-byte private-arena
allocation and verifies its disk round trip. Its proof includes the actual
BASIC `CLEAR`/disk `LOAD`/`USR` sequence before low RAM is touched.

The default high-stack allowance remains 4 KiB for both disk targets; the
heap limit now follows the linker `_STACK` marker. Custom-script validation
also moves `_STACK`, its reservation and the GNU RAM region end together.
This does not claim that any fixed stack size is sufficient for every C
application. See the installed disk guides for the script settings.

The [compact-RAM validation record](compact-ram-validation-2026-09.json)
records 67 successful stock-firmware ROM cold boots with FAT16/FAT32 and
18 deterministic ABI lanes using the final tools. All four final RAM
low-arena example images match successful actual BASIC-loader boots.
The final linker passes 156 tests in normal and sanitized builds, plus
executable reserved-hole and REL/ELF addend checks. The generic local-heap
ABI repair passes 36 C cases and 5,688,465 direct checks. All 48 latest XCC
benchmark cells pass with unchanged size and cycle measurements.
All four original Spectrum MCP modes also pass with the final staged tools.

Existing validation JSON files retain their original layouts and input/tool
identities, including the historical 18,047-variant compiler run. The new
proof also uses a separately formatted FAT32 image with matching official
SYS/BIN files and `AutoBoot=0`; all filesystem writes use private image
copies.
