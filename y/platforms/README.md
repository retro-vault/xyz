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
