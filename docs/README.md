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

- [dist/README.md](/home/tstih/data/retro-vault/xyz/docs/dist/README.md)
  User-facing xtools README staged as `bin/x/README.md` (install and usage).
- [dist/man/](/home/tstih/data/retro-vault/xyz/docs/dist/man)
  User-facing tool manuals staged into `bin/x/share/doc/`.

- [howtos/HOW-TO-TEST.md](/home/tstih/data/retro-vault/xyz/docs/howtos/HOW-TO-TEST.md)
  End-to-end and focused regression test workflows.

- [howtos/BENCHMARKS.md](/home/tstih/data/retro-vault/xyz/docs/howtos/BENCHMARKS.md)
  Repeatable codegen and bare-metal benchmark workflows against SDCC.

- [howtos/IR.md](/home/tstih/data/retro-vault/xyz/docs/howtos/IR.md)
  A practical guide to the three-address intermediate language that sits
  between the frontend and the Z80 backend.

- [GAPS.md](/home/tstih/data/retro-vault/xyz/docs/GAPS.md)
  Current benchmark/code-quality gap analysis against SDCC.

- [SDCC-CODEGEN.md](/home/tstih/data/retro-vault/xyz/docs/SDCC-CODEGEN.md)
  Notes from studying the original SDCC Z80 backend and adapting its
  winning code-generation ideas into `xcc`.

- [howtos/DEBUGGER_INTEGRATION.md](/home/tstih/data/retro-vault/xyz/docs/howtos/DEBUGGER_INTEGRATION.md)
  How to connect an emulator or target to the current debugger stack.

- [howtos/OPTIMIZATIONS.md](/home/tstih/data/retro-vault/xyz/docs/howtos/OPTIMIZATIONS.md)
  What `xcc` optimization levels already do today.

- [todo/LIBC-GAPS.md](/home/tstih/data/retro-vault/xyz/docs/todo/LIBC-GAPS.md)
  Honest status of the current Z80 libc surface and missing pieces.

- [standards/CPP-CODING-STYLE.md](/home/tstih/data/retro-vault/xyz/docs/standards/CPP-CODING-STYLE.md)
  C++ coding rules and repository layout expectations.

- [standards/Z80-CODING-STYLE.md](/home/tstih/data/retro-vault/xyz/docs/standards/Z80-CODING-STYLE.md)
  General Z80 assembly style for runtime and support code.

- [standards/YOS-ASSEMBLY_STYLE_GUIDE.md](/home/tstih/data/retro-vault/xyz/docs/standards/YOS-ASSEMBLY_STYLE_GUIDE.md)
  More specific assembly conventions for the YOS codebase.
