# vendored z80ex

This directory contains the vendored `z80ex` source used by:

- `libxdbg`, for Z80 disassembly
- `xdbg-z80`, for the reference Z80 execution target

It is kept in-repo so the debugger stack does not depend on an external
checkout such as `mudap`.

## Files Used Here

The current build uses:

- `include/z80ex.h`
- `include/z80ex_common.h`
- `include/z80ex_dasm.h`
- `typedefs.h`
- `macros.h`
- `ptables.c`
- `z80ex.c`
- `z80ex_dasm.c`
- `opcodes/*.c`

## Provenance

The original upstream license text is preserved in
[COPYING](/home/tstih/data/retro-vault/xyz/third_party/z80ex/COPYING).

The upstream project README is preserved in
[README](/home/tstih/data/retro-vault/xyz/third_party/z80ex/README).

## Notes

- This is a vendored dependency, not a library with a repo-local public
  API contract of its own.
- Local code should treat `z80ex` as an implementation detail behind the
  `libxdbg` disassembler interface and the `xdbg-z80` reference target.
