# Assembly Style Guide for yos

## 0. Whitespace Rules

**CRITICAL**: All indentation must use SPACES, never TABS. Assembly files must not contain any tab characters.

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
        ;; extern [return_type] routine_name([params]);
        ;; param:  [param location/description]
        ;; return: [where return value goes]
        ;; affects: [list of registers modified]
        ;; notes:   [optional: important implementation notes]
function_name::
        ; code here
        ;; continuation of notes if multi-line
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
        ;; affects: [registers used]
.subroutine_name:
        ; code here
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

## 7. Register Aliases

Use consistent naming:
- Pairs: `bc`, `de`, `hl`
- Individual bytes: `a`, `b`, `c`, `d`, `e`, `h`, `l`
- Alternate: `af`, `af'`, `bc'`, `de'`, `hl'`, `ix`, `iy`
