# Assembly Style Guide for YOS

This guide is the YOS-specific overlay on top of the general Z80 assembly
style in [`x/docs/standards/Z80-CODING-STYLE.md`](../../../x/docs/standards/Z80-CODING-STYLE.md).
Use it for the assembly kernel under `y/src/z80/` and for YOS-side assembly
in `y/tests/`. The vendored libgpx modules under `y/src/z80/gpx/` follow the
same rules but are upstream code; fix bugs there, do not restyle them.

The kernel is one callable routine per module (see the
[Book of YOS](../books/THE-BOOK-OF-YOS.md) source map): public entry points
live in `name.s`, internal helpers and shared state in `_name.s`.

## 0. Whitespace Rules

**CRITICAL**: All indentation must use spaces, never tabs. Assembly files
must not contain any tab characters.

## 1. File Header Format

All header comments and directives **must be indented** (8 spaces) to align with code. Two header forms are in use; both are acceptable, pick one per file and do not mix them.

The compact form used by most kernel modules — a one-line purpose, the licence, and the copyright:

```asm
        ; Install a YOS timer.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module tmr_install
        .optsdcc -mz80 sdcccall(1)

        .globl  _tmr_install
        .globl  __tmr_first
        .globl  _so_create

        .area   _CODE
```

The long form used by the scheduler, the thread startup stub and the libgpx modules — file name, description, licence, and a dated change line:

```asm
        ;; filename.s
        ;;
        ;; Description of what this file does
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) [YEAR] [AUTHOR]
        ;;
        ;; YYYY-MM-DD   [INITIALS]

        .module filename
        .optsdcc -mz80 sdcccall(1)

        .globl  function_one
        .globl  function_two

        .equ    CONSTANT1, 0x42
        .equ    CONSTANT2, 0xff

        .area   _CODE
```

**All directives must be indented**: `.module`, `.globl`, `.equ`, `.area`, `.ds`, `.byte`, `.dw`, etc.

Every module that contains code declares `.optsdcc -mz80 sdcccall(1)`; the whole kernel uses that calling convention (first argument `HL`, second `DE`, rest on the stack, 16-bit result in `DE`). Modules that only hold data may omit it.

Structure offsets are not shared through include files. A module that touches a kernel object repeats the `.equ` block it needs (`THREAD_SP`, `THREAD_STATE`, `PROCESS_MAIN_THREAD`, ...) with the values documented in the Book of YOS; keep those blocks identical across modules.

## 2. Global Routine Comments

**Must be indented** (8 spaces) and must state, at minimum, the inputs, the outputs, and the clobbered registers. The compact form that most kernel modules use:

```asm
        ; _thread_create, sdcccall(1)
        ; inputs: hl = entry, de = stack size, process at sp+2
        ; outputs: de = thread or zero; removes process argument
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: entry -2, stack size -4, thread -6, process +4
_thread_create::
        code here
```

Say explicitly when a routine removes its own stack arguments (`removes process argument`) and when it preserves `IX`/`IY`, because callers rely on both. A `frame:` line listing the IX-relative offsets is expected in every routine that sets up an `IX` frame.

The long form with `;;`, used where a routine deserves a fuller description:

```asm
        ;; ------------------------------------------------------------
        ;; _routine_name
        ;; routine description, can be multiline and include hyphens
        ;;
        ;; Signature: (only if exposed to C)
        ;;   uint8_t mdr_detect_drives(void)
        ;;
        ;; Arguments: (only if it has them)
        ;;
        ;; Return: what it returns and where
        ;;
        ;; Clobbers: which registers it clobbers
        ;;
        ;; References:
        ;;   which other routine or global symbols it references
function_name::
        code here
        ;; continuation of notes if multi-line
```

Example of global routine comment:
```asm
        ;; ------------------------------------------------------------
        ;; _mdr_format
        ;; Dispatch strategy:
        ;;   stream-write 254 free sectors with regenerated headers.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_format(uint8_t drive, char *cart_name)
        ;;
        ;; Arguments:
        ;;   A  = drive number (1-8)
        ;;   DE = cartridge name (C string, padded to 10 chars)
        ;;   stack: dest
        ;;
        ;; Returns:
        ;;   A  = 0 for success, 1 for failure
        ;; 
        ;; Clobbers:
        ;;   A, HL, DE
        ;;
        ;; References:
        ;;   __mdr_motor_on
        ;;   __mdr_detect_gap
```

## 3. Local Label Naming
- All local labels must be prefixed with dot: `.label_name`
- Local labels are NOT indented (start at column 1)
- Exception: labels that are part of public named areas (e.g., `key_map`)

## 4. Local Subroutine Documentation

For local subroutines (prefixed with dot), if complex, use **indented** (8 spaces) two semicolons `;;`:

```asm
        ;; .subroutine_name
        ;; param:  [description]
        ;; return: [where result goes]
        ;; clobbers: [registers used]
.subroutine_name:
        code here
```

## 5. End-of-Line Comments

- Use single `;` for inline comments (at end of instruction line)
- Align to column 41 if the instruction fits before it
- Use two `;` only for stand-alone comment lines at the start of a line (indented)

```asm
        ld      a,#0x42                 ; load 'B' into A
        ;; this is a standalone comment block
        ;; explaining what comes next
        or      b
```

## 6. Utility Functions and Module Naming

Public, C-callable routines carry the SDCC single underscore: `_thread_create`,
`_svc_register`, `_open`. Internal helpers and shared state use a double
underscore: `__thread_robin`, `__kbd_scan`, `__clock_tick`, `__svc_first`.

One callable routine per module. The module is named after the routine
without the SDCC underscore: `thread_create.s` defines `_thread_create`,
`_thread_robin.s` defines `__thread_robin`. Shared state for a subsystem goes
in `_<subsystem>_state.s` (`_thread_state.s`, `_svc_state.s`,
`_clock_state.s`). This lets `xld` drop every routine the ROM does not
reference; keep it that way and do not merge modules for convenience.

Where the kernel needs something libc would normally provide (string copy,
compare, CRC-32) it has its own minimal helper (`__string_copy`,
`__string_compare`, `__crc32`). The ROM links neither libc nor the X runtime;
do not add a dependency on either.

## 7. Register Aliases

Use consistent naming:
- Pairs: `bc`, `de`, `hl`
- Individual bytes: `a`, `b`, `c`, `d`, `e`, `h`, `l`
- Alternate: `af`, `af'`, `bc'`, `de'`, `hl'`, `ix`, `iy`

## 8. Coding Rules for Limited Environment (Z80)

1. Severe memory constraint: We are running on a machine with only a few kilobytes of RAM. Code size must be kept as small as possible.
2. Reusability: Reuse existing routines whenever possible.
3. Optimization priority: Size optimization comes first. Speed optimization is secondary, but no major speed penalties are allowed (especially when drawing to the screen).
4. Register usage: When possible, use the Z80 alternate register set (EXX, EX AF,AF') to reduce memory usage.
5. Index registers: If IX or IY are used, they must be saved and restored.
6. No global context: Routines must be stateless and independent (except for configurable global settings). They must work correctly in a context-switching environment.
7. Hand-written assembly only: All kernel code must be hand-written Z80 assembly. Do not use C compilers or generated assembly. (The only compiled code in the build is the placeholder `shell.sys` process, which is an application, not part of the ROM.)
8. Fixed addresses are sacred: the first 256 bytes of `crt0rom.s`, the reserved ranges in `linker.lk` (`0x04C6`, `0x0562`, `0x3D00-0x3DFF`), the IM2 word at `0x5EFF` and the ROM size limit of 16 KiB are all checked by the build or by `tests/kernel-z80`. Never move them to make room.
