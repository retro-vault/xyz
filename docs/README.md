# Documentation Map

This directory contains the project’s maintained documentation.

The subfolders are organized by purpose:

- `dist/`
  Distribution and staging layout documentation.
  Start here when you need to understand what ends up in `bin/`.

- `howtos/`
  Practical guides for common workflows such as testing and debugger
  integration.

- `research/`
  Reference material and imported background documents.
  These files are informative, not normative.

- `standards/`
  Project coding and formatting standards.
  These are the rules to follow when editing C++, C, and assembly.

- `todo/`
  Focused gap analyses and forward-looking work lists.
  These documents should describe the current state honestly, not an
  aspirational future state.

Component-specific README files still live with their owning tool or library.
When the distribution tree is staged, those documents are collected under
`bin/share/xtools/docs/components/`.

## Current Key Documents

- [dist/README.md](/home/tstih/data/retro-vault/xyz/docs/dist/README.md)
  Staged distribution layout under `bin/`.

- [howtos/HOW-TO-TEST.md](/home/tstih/data/retro-vault/xyz/docs/howtos/HOW-TO-TEST.md)
  End-to-end and focused regression test workflows.

- [howtos/DEBUGGER_INTEGRATION.md](/home/tstih/data/retro-vault/xyz/docs/howtos/DEBUGGER_INTEGRATION.md)
  How to connect an emulator or target to the current debugger stack.

- [todo/LIBC-GAPS.md](/home/tstih/data/retro-vault/xyz/docs/todo/LIBC-GAPS.md)
  Honest status of the current Z80 libc surface and missing pieces.

- [standards/CPP-CODING-STYLE.md](/home/tstih/data/retro-vault/xyz/docs/standards/CPP-CODING-STYLE.md)
  C++ coding rules and repository layout expectations.

- [standards/Z80-CODING-STYLE.md](/home/tstih/data/retro-vault/xyz/docs/standards/Z80-CODING-STYLE.md)
  General Z80 assembly style for runtime and support code.

- [standards/YOS-ASSEMBLY_STYLE_GUIDE.md](/home/tstih/data/retro-vault/xyz/docs/standards/YOS-ASSEMBLY_STYLE_GUIDE.md)
  More specific assembly conventions for the YOS codebase.
