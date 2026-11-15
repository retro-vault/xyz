# Platform source ownership

`src/z80/kernel`, `drivers`, `fs` and `bank` provide the Spectrum ROM runtime.
`src/z80/cooperative` owns the Partner/CPC loader, process and service lifetime,
allocation, lists, CP/M filesystem and ABI failure helpers. Partner hardware
lives under `platforms/partner`; CPC hardware under `platforms/cpc`; Next's
remaining kernel and filesystem variants under `platforms/next`.

Each platform's `sources.json` explicitly selects maintained sources. Next
reuses common modules directly and declares section placement where necessary.
`python3 y/scripts/materialize_platform.py next --output build/next-source`
materializes a downstream source tree. The tool validates paths and section
counts before writing, preserves unchanged timestamps, and removes only stale
files it previously owned. It never patches instruction bodies.

Partner/CPC share CP/M operations and the six-byte heap block format.
`YOS_CPC_AMSDOS=1` selects CPC disk geometry, directory attributes, direct common
heap access and fast visible-byte operations. It defaults to zero. Hardware
boot, banking, controllers and interrupts remain platform-specific. ALTO owns
GUI adapters and its final bank/section placement and links these sources.

The 82-slot public table is common; scheduling capabilities remain distinct.
Cooperative optional-operation adapters return failure with the same register
and packed-stack convention. They do not pretend to implement threads or raw
GPX services. ALTO drawing is provided through its own facade.

The Partner substrate derives from MIT-licensed iskra-delta/baremetal commit
`5fb38c9e135820a4feae7bc2e99b907333716d8f`; its maintained fixes are now ordinary
sources, with the original notice in `src/z80/cooperative/LICENSE`.

Tests: `make -C y/src/z80 test-platform-sources`, the production ROM test target,
and ALTO's Partner/CPC emulator suites. Graphics primitives belong to libgpx.

The shared far heap uses `YOS_CPC_AMSDOS` for CPC common-memory fast paths.
`YOS_EXTERNAL_FAR_GATE=1` lets a resident desktop supply its bank-safe access
gates; standalone Partner builds retain the loader forwarding entries. No
downstream source truncation is required. `test_cooperative_memory.py` assembles
all combinations and verifies that exactly the selected owner defines the gates.

`YOS_RESIDENT_DESKTOP=1` selects the single-copy Partner bank entry and the
common-memory floppy interrupt wrapper. The default keeps the standalone boot
fixture. Both configurations are assembled by the cooperative source tests;
downstream builders select the option instead of replacing function bodies.

Cooperative `load_library(path, 0)` now creates a private instance instead of
reusing a globally named service. Its library record retains the acquiring
process at bytes 4–5 (zero for shared libraries); private initializer
registrations have visibility 2, while public registrations use 1. Queries
skip private registrations. Process reaping frees each of its private library
images, export tables, registrations and owned memory before reclaiming the
process itself. Shared idle collection and the retained configuration cache
keep their existing policy. ALTO's `check-terminal-instances.py` exercises two
independent command services, closing during a directory scan, reopening and
complete memory reclamation on real native emulators.

### CP/M user-area folders

The shared CPC/Partner adapter exposes a folders-only root. Areas 0–4 default
to `bin`, `lib`, `etc`, `var`, `sys`; unmapped areas are hidden by default.
Numeric components remain synonyms after renaming. `A:/` enumerates mapped
folders (optionally all 16 areas), whereas `A:/0/` enumerates only area-zero files. Bare file paths retain
area-zero compatibility. `rewinddir` resets the root cursor in DIR byte 23;
`stat` reports both roots and area paths as directories. The raw libcpmfs DIR
ABI and physical formats are unchanged.

The host publishes a `cpm.dirs` named-service table containing one near entry:
`HL=name, E=area`, returning A=0 on success or FF on invalid input. The adapter
copies at most 12 characters plus NUL to common stack storage before entering
the resident resolver. Names are folded to lowercase, must be unique, and
cannot begin with a dot/digit, contain separators/whitespace, or equal `alto`.
An empty name removes the mapping. E=255 selects the visibility setting: HL
points to exactly `0` (hide unmapped) or `1` (show numeric names). Invalid
values are rejected. Startup defaults to hidden; hiding never disables numeric
paths. The host config key is `[directories] show_unmapped=0`. The 16 × 13-byte
cache and one visibility byte live in the writable resident image, not common RAM. No new `yos_t` slot is needed.

The application configuration worker feeds `[directories]` entries at startup;
all filesystem calls then resolve aliases centrally. Stable installation paths
`A:/ALTO/bin`, `lib`, `etc`, `var`, `sys`, and `usr/share/...` select their
physical roles independently of display aliases. Common adapters (`path.s`)
and private parsing (`path-resolve.s`) are separate objects: their external
references receive kernel veneers before any access to private state. Input
strings are copied before switching banks. Native `fd0:`/`hd0:` prefixes never
read beyond the input terminator.

The cooperative image loader retains a separate 32-byte normalized physical
pathname. Deriving a short process/service name never strips the user area
from the name passed to the disk layer. CPC's AMSDOS entry remains in area 0
and opens `4:COMMON.BIN` / `4:KERNEL.BIN`. Partner's disk builder accepts
`--user 4` to put `SYSTEM.BIN` in the system area and records that area in its
ROM boot descriptor; the default remains area 0 for existing fixtures.

Cooperative process placement first probes for image bytes plus 4352 bytes of local
startup space, releases the probe, and allocates just the image in that same
bank. The loader does not yield between these operations. It falls back to
ordinary packed placement if neither bank has that margin. Libraries retain
the existing policy. This keeps dynamic fonts/data from exhausting a small
resident-bank tail while the application bank is still mostly empty.

The common `__partner_popup_mouse` adapter returns window-relative pointer
coordinates into the caller's bank. The downstream public popup API must select
this adapter; the internal coordinate calculator returns values in registers
and is reached through a kernel veneer.
