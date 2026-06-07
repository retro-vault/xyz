# Z80-CODING-STYLE.md - Z80 Assembly Coding Standards

This document defines the Z80 assembly style for code in
`src/xc/xcc/lib/runtime/`, `lib/libc/src/`, `tests/`, and other
hand-written Z80 modules.

The baseline style comes from the imported and adapted runtime modules now
living under `src/xc/xcc/lib/runtime/`, grouped into folders such as
`int8/`, `int16/`, `int32/`, `int64/`, `float/`, `double/`, `common/`,
`atomic/`, `jumps/`, and `sys/`.

Those imported files already show several good patterns:

- One helper per file.
- ABI notes near the top.
- Locals, inputs, outputs, and clobbers documented together.
- Global exports listed before code.
- Lowercase mnemonics, directives, and local labels.
- Core helpers kept separate from ABI bridge wrappers.

This guide keeps those structural ideas, but applies stricter
formatting rules.

## 1. Scope

Use this style for:

- Imported SDCC runtime code after cleanup.
- xcc ABI bridge modules.
- libc assembly under `lib/libc/src/`.
- Startup code, trampolines, and test support assembly.
- Any new hand-written Z80 source file.

## 2. File Layout

Each file should follow this order:

1. Indented file header comment.
2. Indented assembler directives such as `.module` and `.area`.
3. Indented `.globl` declarations.
4. A per-function comment block with helper name, inputs, outputs,
   and clobbers.
5. Unindented labels.
6. Indented instructions and comments.
7. Local helper labels, also unindented.

Prefer one externally visible helper per file.

## 3. Mandatory Formatting Rules

- Use spaces only. Never use tabs.
- No line may be longer than 72 columns.
- Indent everything except labels.
- That rule includes directives, instructions, blank-line comments,
  and block comments.
- Labels start in column 1.
- Inline comments must start in column 41.
- Keep one blank line between major sections.
- Keep register, symbol, and helper names lowercase when the ABI does
  not force another spelling.

## 4. File Header

Every assembly file must begin with an indented header comment.

Use this shape:

```asm
        ; 16-bit multiply wrapper for the xcc ABI.
        ; Converts stack-passed operands to the SDCC helper ABI.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
```

Rules:

- Say exactly what the file does.
- Mention ABI translation when relevant.
- Mention whether the file is imported, adapted, or native.
- Keep the header concise and specific.
- Do not add decorative banner rows.

## 5. Function Comment Block

Every exported helper should have a plain comment block immediately
before its entry label.

Use this shape:

```asm
        ; __divulong
        ; inputs:  x in DE:HL (DE=low16, HL=high16), y at 4(ix)..7(ix)
        ;          (lsb..msb)
        ; outputs: DE:HL = unsigned quotient x / y
        ; clobbers: af, bc, de, hl, ix
```

Rules:

- calling convention
- argument locations
- return value location
- clobbered registers
- local stack layout when non-trivial, documented nearby when helpful

Keep this block directly above the first label for that helper.

## 6. Indentation and Alignment

Use eight spaces for the normal indent level.

Apply that indent to:

- comment-only lines
- `.module`, `.area`, `.globl`, and similar directives
- instructions

Do not indent labels.

Good shape:

```asm
        .module xcc_mul16
        .area   _CODE
        .globl  __mul16

__mul16:
        push    ix
        ld      ix, #0
        add     ix, sp
```

## 7. Inline Comments

Inline comments are optional, but when used they must line up.

The `;` for an inline comment must begin in column 41.

Example:

```asm
        ld      l, 4(ix)                ; lhs low byte
        ld      h, 5(ix)                ; lhs high byte
        call    __mulint                ; imported core
```

Rules:

- Use inline comments for non-obvious register meaning.
- Do not comment every instruction.
- Prefer block comments above a group of instructions.

## 8. Code Comments

Prefer short block comments above a related instruction group.

Good:

```asm
        ; reserve 12 bytes of local stack space
        ld      hl, #-12
        add     hl, sp
        ld      sp, hl
```

Rules:

- Explain the intent of a block, not the spelling of each mnemonic.
- Use inline comments sparingly.
- Remove duplicated or decorative comments that add no meaning.

## 9. Labels

Labels are the only text that may start in column 1.

Rules:

- Public entry labels use the ABI spelling exactly.
- Internal labels should be short and descriptive.
- Local labels may use a leading dot, for example `.mul_loop`.
- Keep local label naming consistent within one file.

## 10. Directives

Imported files use a stable directive pattern that we should keep:

- `.module`
- `.optsdcc` when needed
- `.area`
- `.globl`

Indent directives and align operands where practical.

Good:

```asm
        .module mullong
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __mullong
```

## 11. Naming

Follow these naming rules unless the external ABI forces otherwise:

- file names: lowercase snake_case
- helper names: lowercase snake_case
- local labels: lowercase snake_case or short dotted labels
- exported symbols: preserve ABI spelling exactly

Do not encode implementation origin in file names. In particular, do
not use prefixes such as `xcc_` or `sdcc_`.

Use one descriptive naming scheme across the whole runtime:

- plain helper names when there is no conflict:
  `divsigned.s`, `divschar.s`, `mullong.s`, `fp_zero32.s`
- prefer folding public aliases into the real implementation file instead
  of creating a trampoline bridge
- `_core` only when a file is truly private shared machinery; do not use
  it for the public helper itself

## 12. Imported Library Policy

When importing or adapting code from SDCC or similar external runtimes:

- Split helpers into one function per file where practical.
- Add a real file header.
- Reflow comments and code to the 72-column limit.
- Replace tabs with spaces.
- Indent all comments and directives.
- Add per-function `inputs` / `outputs` / `clobbers` comments if they
  are missing.
- Keep copyright and license notes intact.

## 13. Short Template

Use this as the starting point for new files:

```asm
        ; helper purpose line one
        ; helper purpose line two
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module helper_name
        .area   _CODE
        .globl  __helper_name

        ; __helper_name
        ; inputs: describe arguments
        ; outputs: describe return values
        ; clobbers: list registers

__helper_name:
        push    ix
        ld      ix, #0
        add     ix, sp
        ret
```

## 15. Summary

Keep the good structural habits from the imported SDCC runtime code,
but normalize all new and touched assembly to this stricter house
style:

- spaces only
- 72 columns max
- labels at column 1
- everything else indented
- inline comments at column 41
- banner comments with five leading minus signs
