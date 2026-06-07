# mdr-emu

Host-side microdrive driver harness for YOS.

Goal:
- run the built YOS ROM under an embedded Z80 core
- call `mdr_detect_drives`, `mdr_dir`, `mdr_load`, and `mdr_save` directly by symbol address
- emulate the microdrive ports locally so save/load regressions can be debugged without manual emulator runs

Current status:
- embeddable Z80 core vendored from `superzazu/z80` under MIT license
- ROM loader and symbol resolver implemented
- direct-call runner implemented
- host-side MDR image code now comes from `tools/microdrive/libmicrodrive`
- Fuse-inspired microdrive device model implemented for `MD_CTRL` / `MD_SEL` / `MD_DATA`
- non-trivial smoke coverage implemented for:
  - detect: no drive and one drive
  - dir/load: known-good `hello.mdr`
  - save round-trips: 123, 512, 777 bytes
  - duplicate filename rejection
  - full cartridge rejection
  - sequential mixed-size saves on one cartridge + persisted image reload check
  - missing-file load behavior
  - zero-length save rejection
  - fragmented cartridge save/load
  - dir capacity behavior (>32 files)
  - checksum-corrupted entry rejection (dir + load)

Commands:
- `make -C tests/mdr-emu test` runs one full smoke pass
- `make -C tests/mdr-emu stress` runs repeated smoke passes
- `make -C tests/mdr-emu size` prints microdrive object sizes and ROM size
- `make -C tests/mdr-emu repro-strict` (or `build/mdr-emu/mdr_emu repro-strict`) runs a Fuse-like strict
  IF1/Microdrive model against `mdrstep.mdr`:
  - control/data/net port decode by `port & 0x18`
  - motor rotation from control clock/data bits
  - preamble-gated writes (10x`00` + 2x`ff` before block bytes)
  - post-save detect/dir/load verification
  This command is intended to fail fast when save timing/alignment is wrong
  even if the looser smoke model still passes.

Why this shape:
- it avoids shell, keyboard, and process-exit issues
- it keeps the loop tight around the actual driver entry points
- it gives us deterministic host-side tests we can run repeatedly while optimizing behavior and size
