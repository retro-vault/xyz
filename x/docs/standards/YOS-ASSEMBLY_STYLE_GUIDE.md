# Assembly Style Guide for YOS

This guide is the YOS-specific overlay on top of the general Z80 assembly
style. Use it for `src/yos/` and closely related YOS-side assembly files.

## 0. Whitespace Rules

**CRITICAL**: All indentation must use spaces, never tabs. Assembly files
must not contain any tab characters.

## 1. File Header Format

All header comments and directives **must be indented** (8 spaces) to align with code:

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

        .globl  function_one
        .globl  function_two

        .equ    CONSTANT1, 0x42
        .equ    CONSTANT2, 0xff

        .area   _CODE
```

**All directives must be indented**: `.module`, `.globl`, `.equ`, `.area`, `.ds`, `.byte`, `.dw`, etc.

## 2. Global Routine Comments

**Must be indented** (8 spaces) with two semicolons `;;`:

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

## 6. Utility Functions

All utility/helper functions: `__function_name` (double underscore)
Examples: `__kbd_scan`, `__clock_tick`, `__mouse_calibrate`

Utility/helper functions should be placed in a separate file with the same
name as the original, but prefixed with an underscore (`_`) when that split
improves readability.
Example: `windows.s` -> helper functions go in `_windows.s`

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
7. Hand-written assembly only: All code must be hand-written Z80 assembly. Do not use C compilers or generated assembly.
