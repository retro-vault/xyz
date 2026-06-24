# Changelog

Release status:
- `v1.0.0` through `v1.7.0` are Alpha releases.
- `v1.7.1` is the first Beta release.

## v1.7.1 - Beta - 2026-06-20

- Completed the `x/tests/c23` `xcc-z80` setup by adding real helper scripts:
  `x/tests/c23/setups/xcc-z80/bin/xcc_z80_driver.sh` and
  `x/tests/c23/setups/xcc-z80/bin/run_xcc_z80.sh`.
- Kept the dedicated `x` release pipeline that builds and publishes `x.tar.gz`.
- Preserved the `xcc`, `xas`, `xar`, `xld`, `xobjcopy`, `xopt`, `xgdb`, and
  `xemu` tool set introduced in earlier alpha tags.

## v1.7.0 - Alpha - 2026-06-20

- Updated the `x/tests/c23` `xcc-z80` profile to pass an explicit compiler path
  into its driver template.
- Kept the dedicated `x` release workflow that archives only `bin/x` as
  `x.tar.gz`.
- The helper scripts referenced by the `xcc-z80` profile were still not present
  in this tag.

## v1.6.0 - Alpha - 2026-06-20

- Added an automated MI smoke test for `xgdb`:
  `x/src/xgdb/tests/mi_smoke_test.sh`.
- Switched release automation from a monolithic `xyz-release.tar.gz` payload to
  a dedicated `x.tar.gz` build that stages only the `x` distribution.
- Updated the release workflow to the `xgdb` VS Code extension path and the
  dedicated `bin/x` packaging layout.

## v1.5.0 - Alpha - 2026-06-20

- Split staged output into distinct `bin/x`, `bin/y`, and `bin/z` prefixes,
  making the `x` toolchain a relocatable install tree with its own manuals and
  packages.
- Added `xobjcopy` as an object/archive conversion tool and `xopt` as a
  standalone Z80 assembly optimizer.
- Expanded `xld` beyond the earlier SDCC-only flow: GNU ELF object input,
  GNU/SDCC archive reading, linker scripts, map output, derived ELF plus
  DWARF sidecars in GNU mode, and documented Intel HEX output all appear in
  the staged manuals and sources.
- Expanded `xas` with macro processing and source-to-source dialect conversion
  between SDCC-style and GNU-style assembly.
- Grew `xcc` substantially with fixed-point float runtime families
  (`fixed8_8`, `fixed16_16`, `fixed24_8`), many new optimizer regressions, and
  broader ABI/runtime tests.
- Expanded the target libc surface with `stdio.h`, `threads.h`, `fcntl.h`,
  `unistd.h`, `sys/stat.h`, `sys/types.h`, plus large additions in complex
  math, transcendentals, wide-character support, and thread runtime code.
- Added the repository-wide `x/tests/c23` compatibility suite, including an SDCC
  setup and an `xcc-z80` profile skeleton. In this tag the `xcc-z80` profile
  already exists, but its helper scripts are not yet present.
- Added staged tool manuals under `x/docs/dist/man/` for `xar`, `xas`, `xcc`,
  `xgdb`, `xld`, `xobjcopy`, and `xopt`.

## v1.4.0 - Alpha - 2026-06-07

- Marked the transition from the earlier `xlink`/`xdbg`-centric stack to a
  staged `xc` toolchain built around `xcc`, `xas`, `xar`, `xld`, `xgdb`, and
  `xemu`.
- Added a large assembler-based libc surface under `x/libc`, including new
  headers and implementations for `assert`, `complex`, `ctype`, `errno`,
  `fenv`, `inttypes`, `locale`, `math`, `setjmp`, `signal`, `stdlib`,
  `string`, `time`, `uchar`, `wchar`, and `wctype`.
- Restructured project documentation around `docs/README.md`, standards,
  how-to guides, and staged distribution docs.
- Started staging the toolchain as an explicit host-plus-target slice with
  host executables and target headers/libraries.

## v1.3.0 - Alpha - 2026-05-24

- Improved `xdbg` source-less debugging: functions without resolvable source
  files now fall back to symbol/disassembly views instead of being attached to
  the wrong source file.
- Added DAP disassembly handling in `xdbg`, including advertised
  `supportsDisassembleRequest` capability and byte-accurate instruction output.
- Added `stepOut` support in the DAP path by continuing to a temporary
  breakpoint at the stack return address.
- Improved debugger integration guidance for emulator authors, including the
  `server.close()` shutdown path and the separation between target state and
  frontend source lookup.
- Refined `xlink`-generated `.xdbg` output for library functions that have
  symbol ranges but no resolved source file on disk.

## v1.2.0 - Alpha - 2026-05-23

- Expanded `xlink` substantially with new parsers and emitters for `.adb`,
  `.cdb`, `.lst`, NoICE output, linked `.xdbg` output, runtime injection, and
  library archive handling.
- Added tests for the new `xlink` capabilities, including runtime, CDB, NoICE,
  library parsing, and `.xdbg` emission paths.
- Documented `xlink` as a linker that can selectively pull from libraries,
  emit `XL` and `BIN`, generate NoICE files, produce linked SDCC debug data,
  and consume optional sidecars from compiler and assembler builds.

## v1.1.0 - Alpha - 2026-05-17

- Kept the same executable surface as `v1.0.0`, centered on `xlink`,
  `xdbg`, and `xdbg-z80`, plus the supporting host utilities.
- Added debugger integration documentation to the packaged docs set.
- Updated staged distribution documentation and Debian packaging metadata.

## v1.0.0 - Alpha - 2026-05-17

- Initial tagged `x` baseline with a staged distribution centered on `xlink`,
  `xdbg`, and `xdbg-z80`.
- Shipped host-side support libraries including `libxdbg.a`, `libxdbg_cli.a`,
  `libxdbg_dap.a`, `libxdbg_mi.a`, and `libxdbgstub.a`, alongside the
  repository's other host tools.
- Staged public debugger headers and the host-side debugger support libraries
  in the `xgdb` include tree.
- `xlink` already handled SDCC-style `.rel` and `.lib` inputs, demand-driven
  library inclusion, `XL` and flat `BIN` output, and SDCC v1 plus `XL4`
  record parsing.
- `xdbg` already exposed CLI, MI, and DAP frontend code, plus a reference Z80
  remote target.
- Early `xas` and `xcc` source trees were already present in the repository,
  but the staged distribution was still focused on the linker/debugger stack
  rather than the later full compiler-suite packaging.
