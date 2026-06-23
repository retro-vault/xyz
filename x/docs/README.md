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
When the distribution tree is staged, only the compiler tool manuals are
collected, under `bin/x/share/doc/`.

## Current Key Documents

- [dist/README.md](dist/README.md)
  User-facing xtools README staged as `bin/x/README.md` (install and usage).
- [dist/man/](dist/man/)
  User-facing tool manuals staged into `bin/x/share/doc/`.

- [howtos/HOW-TO-TEST.md](howtos/HOW-TO-TEST.md)
  End-to-end and focused regression test workflows.

- [howtos/BENCHMARKS.md](howtos/BENCHMARKS.md)
  Repeatable codegen and bare-metal benchmark workflows against SDCC.

- [howtos/IR.md](howtos/IR.md)
  A practical guide to the three-address intermediate language that sits
  between the frontend and the Z80 backend.

- [howtos/DEBUGGER_INTEGRATION.md](howtos/DEBUGGER_INTEGRATION.md)
  How to connect an emulator or target to the current debugger stack.

- [howtos/OPTIMIZATIONS.md](howtos/OPTIMIZATIONS.md)
  What `xcc` optimization levels already do today.

- [todo/LIBC-GAPS.md](todo/LIBC-GAPS.md)
  Honest status of the current Z80 libc surface and missing pieces.

- [standards/CPP-CODING-STYLE.md](standards/CPP-CODING-STYLE.md)
  C++ coding rules and repository layout expectations.

- [standards/Z80-CODING-STYLE.md](standards/Z80-CODING-STYLE.md)
  General Z80 assembly style for runtime and support code.

- [../../y/docs/YOS-ASSEMBLY_STYLE_GUIDE.md](../../y/docs/YOS-ASSEMBLY_STYLE_GUIDE.md)
  More specific assembly conventions for the YOS codebase.
