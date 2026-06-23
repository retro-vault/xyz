# Z80 Target

This is the new home for extracted CPU- and ABI-level target definitions for the X tools.

At the moment, most Z80 target knowledge still lives inside the copied tool sources, especially under:

- `x/src/xcc/include/backend/z80/`
- `x/src/xcc/src/backend/z80/`
- `x/src/xas/`
- `x/src/xld/`

Platform-specific startup files, linker scripts, and sys hooks live under `x/platforms/`.
