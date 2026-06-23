# xcc Debug Information Reference

Passing `-g` to xcc activates debug info output.  The format depends on the
assembler dialect selected with `-masm=`:

| Dialect | Format | Files produced |
|---------|--------|----------------|
| `-masm=sdasz80` (default) | SDCC `;!` inline directives + `.adb` | `output.s`, `output.adb` |
| `-masm=gnuas` | DWARF 2 sections + inline `.file`/`.loc` | `output.s` |

The implementation lives in:

| File | Class | Purpose |
|------|-------|---------|
| `include/backend/z80/debug_info.h` | `debug_info_emitter` (abstract) | common interface |
| `include/backend/z80/dwarf.h` / `src/backend/z80/dwarf.cpp` | `dwarf_emitter` | DWARF 2 |
| `include/backend/z80/sdcc_debug.h` / `src/backend/z80/sdcc_debug.cpp` | `sdcc_debug_emitter` | SDCC `;!` |

The driver (`src/driver/main.cpp`) creates the right subclass based on the
`-g` and `-masm=` combination and injects it into `z80_gen` via `set_debug()`.

---

## SDCC `;!` directives (`-masm=sdasz80 -g`)

SDCC-style directives are embedded as comments directly in the `.s` assembly
file.  They start with `; !` (semicolon-space-exclamation).  SDCDB (the SDCC
debugger) reads them to reconstruct source-level symbol information.

### Directive summary

| Directive | When emitted | Purpose |
|-----------|-------------|---------|
| `; !FILE path` | Once, at module start | Opens a compilation unit |
| `; !DEFT name %type` | Once per type alias, at module start | Declares a named type |
| `; !FUNC name %rettype` | Once per function, before prologue | Opens a function scope |
| `; !DEFS var %type loc` | Once per parameter, after `; !FUNC` | Declares a symbol and its location |
| `; !LINE n` | Before each instruction that starts a new source line | Maps assembly to source |
| `; !ENDF` | Once per function, after `ret` | Closes a function scope |
| `; !ENDFILE` | Once, at module end | Closes the compilation unit |

### `; !FILE`

```
; !FILE path/to/source.c
```

Written once at the start of the module.  `path` is the path passed to xcc on
the command line.

### `; !DEFT`

```
; !DEFT name %typecode
```

Declares a named type alias.  xcc emits one `DEFT` line for each of the
twelve fundamental C types:

```
; !DEFT char %S8
; !DEFT unsigned char %U8
; !DEFT short %S16
; !DEFT unsigned short %U16
; !DEFT int %S16
; !DEFT unsigned int %U16
; !DEFT long %S32
; !DEFT unsigned long %U32
; !DEFT long long %S64
; !DEFT unsigned long long %U64
; !DEFT float %SF
; !DEFT double %SF
```

### `; !FUNC`

```
; !FUNC funcname %rettype
```

Opens a function scope.  `funcname` is the unmangled C name (not the
underscore-prefixed assembly label).  `%rettype` is the return type code
(see [Type codes](#type-codes) below).

Example:

```
; !FUNC add %S16
```

### `; !DEFS`

```
; !DEFS varname %type location
```

Declares one named symbol and its location.  xcc emits `DEFS` lines for
every function parameter (identified from `RECEIVE` icodes).

Location formats for Z80:

| Format | Meaning |
|--------|---------|
| `IX+N` | IX-relative at offset +N (parameters: `IX+4`, `IX+6`, …) |
| `IX-N` | IX-relative at offset -N (locals: `IX-2`, `IX-4`, …) |
| `D` | Static storage (global or static local) |
| `SP+N` | SP-relative (used when IX is not available) |

Parameter offsets start at `IX+4` (first argument) and grow by the argument's
size.  The offsets are taken directly from the `RECEIVE` icode's `result`
operand `stack_offset` field, with 4 added for the saved IX and return address.

Example:

```
; !FUNC add %S16
; !DEFS a %S16 IX+4
; !DEFS b %S16 IX+6
```

### `; !LINE`

```
; !LINE n
```

Emitted before the first instruction of source line `n` whenever the source
line changes.  Duplicate consecutive lines are suppressed.

### `; !ENDF`

```
; !ENDF
```

Emitted immediately after the function's `ret` instruction (in the epilogue).
Closes the scope opened by `; !FUNC`.

### `; !ENDFILE`

```
; !ENDFILE
```

Emitted once at the very end of the module, after all functions.  Closes the
scope opened by `; !FILE`.

---

### Type codes

Type codes appear after `%` in `DEFT`, `FUNC`, and `DEFS` directives.

| Type code | C type | Size |
|-----------|--------|------|
| `%S8` | `char`, `signed char` | 1 byte |
| `%U8` | `unsigned char`, `_Bool` | 1 byte |
| `%S16` | `short`, `int`, `enum` | 2 bytes |
| `%U16` | `unsigned short`, `unsigned int` | 2 bytes |
| `%S32` | `long` | 4 bytes |
| `%U32` | `unsigned long` | 4 bytes |
| `%S64` | `long long` | 8 bytes |
| `%U64` | `unsigned long long` | 8 bytes |
| `%SF` | `float`, `double` (both 4-byte soft-float on Z80) | 4 bytes |
| `%V` | `void` or unknown | — |
| `%ASCII` | `char *` or `unsigned char *` (C string) | 2 bytes (pointer) |
| `%*T` | pointer to type `T` (e.g., `%*%S16` = `int *`) | 2 bytes |
| `%[T]` | array of type `T` | varies |
| `%struct:Tag` | struct with tag `Tag` | varies |
| `%union:Tag` | union with tag `Tag` | varies |

`%SF` is used for both `float` and `double` because xcc maps both to the same
4-byte IEEE 754 single representation on Z80.

---

### `.adb` file

xcc writes a companion `.adb` file alongside the `.s` file (same base name,
`.adb` extension).  It contains function entry points and their source line
numbers for SDCDB's source-level stepping.

Format:

```
[path/to/source.c]
F_funcname:startline:0:0:0
```

One `F` line is written per function.  `_funcname` is the mangled assembly
label (underscore-prefixed C name).  `startline` is the source line where the
function's first instruction was emitted.

Example for a two-function module:

```
[hello.c]
F_main:3:0:0:0
F_add:12:0:0:0
```

---

### Complete example (`-masm=sdasz80 -g`)

Source (`add.c`):

```c
int add(int a, int b) {
    return a + b;
}
```

Annotated assembly output:

```asm
; !FILE add.c
; !DEFT char %S8
; !DEFT unsigned char %U8
; !DEFT short %S16
; !DEFT unsigned short %U16
; !DEFT int %S16
; !DEFT unsigned int %U16
; !DEFT long %S32
; !DEFT unsigned long %U32
; !DEFT long long %S64
; !DEFT unsigned long long %U64
; !DEFT float %SF
; !DEFT double %SF
	.module xcc_output
	.area _CODE
; !FUNC add %S16
; !DEFS a %S16 IX+4
; !DEFS b %S16 IX+6
	.globl _add
_add:
	push	ix
	ld	ix, #0
	add	ix, sp
; !LINE 2
	ld	l, 4 (ix)
	ld	h, 5 (ix)
	ld	e, 6 (ix)
	ld	d, 7 (ix)
	add	hl, de
	jp	__add_end
__add_end:
	ld	sp, ix
	pop	ix
	ret
; !ENDF
; !ENDFILE
```

---

## DWARF 2 sections (`-masm=gnuas -g`)

When targeting GNU as (`-masm=gnuas`), xcc emits DWARF 2 debug information.
Two categories of output are produced: inline directives woven into the code
stream, and three binary debug sections appended at the end of the module.

### Inline directives

#### `.file`

```asm
	.file 1 "path/to/source.c"
```

Emitted once at module start (`begin_module()`).  Introduces the source file
to the assembler's internal line-number table.  The file number `1` is fixed;
xcc currently compiles one translation unit at a time.

#### `.loc`

```asm
	.loc 1 N 0
```

Emitted before the first instruction of source line `N` whenever the source
line changes.  `1` is the file number matching `.file 1`.  The column `0`
means "unknown column".  Duplicate consecutive lines are suppressed.

The assembler consumes these directives to build the `.debug_line` section
automatically.

### End-of-function label

```asm
.Ldbg_FUNCNAME_end:
```

Emitted by `end_function()` immediately after the function's `ret`
instruction.  `FUNCNAME` is the unmangled C name (e.g., `.Ldbg_main_end`).

This label is referenced in both `.debug_info` and `.debug_aranges` as the
high-PC bound of the function.  Using a label (rather than a hard-coded
address) lets the assembler and linker fill in the correct value.

### Debug sections

All three sections are emitted at end-of-module by `end_module()`, after all
functions have been written.

#### `.debug_abbrev`

Describes the shape of the DIEs in `.debug_info`.  xcc emits exactly two
abbreviation entries:

| Abbrev | DW_TAG | Children | Attributes |
|--------|--------|----------|-----------|
| 1 | `DW_TAG_compile_unit` (0x11) | yes | `DW_AT_producer` (string), `DW_AT_language` (data2), `DW_AT_name` (string), `DW_AT_low_pc` (addr), `DW_AT_high_pc` (addr), `DW_AT_stmt_list` (data4) |
| 2 | `DW_TAG_subprogram` (0x2e) | no | `DW_AT_name` (string), `DW_AT_low_pc` (addr), `DW_AT_high_pc` (addr), `DW_AT_external` (flag) |

The table is terminated with a `0x00` byte.

Encoded layout:

```asm
	.section .debug_abbrev
	.byte 1          ; abbrev code 1
	.byte 0x11       ; DW_TAG_compile_unit
	.byte 0x01       ; DW_CHILDREN_yes
	.byte 0x25, 0x08 ; DW_AT_producer, DW_FORM_string
	.byte 0x13, 0x05 ; DW_AT_language, DW_FORM_data2
	.byte 0x03, 0x08 ; DW_AT_name, DW_FORM_string
	.byte 0x11, 0x01 ; DW_AT_low_pc, DW_FORM_addr
	.byte 0x12, 0x01 ; DW_AT_high_pc, DW_FORM_addr
	.byte 0x10, 0x06 ; DW_AT_stmt_list, DW_FORM_data4
	.byte 0x00, 0x00 ; end of attributes
	.byte 2          ; abbrev code 2
	.byte 0x2e       ; DW_TAG_subprogram
	.byte 0x00       ; DW_CHILDREN_no
	.byte 0x03, 0x08 ; DW_AT_name, DW_FORM_string
	.byte 0x11, 0x01 ; DW_AT_low_pc, DW_FORM_addr
	.byte 0x12, 0x01 ; DW_AT_high_pc, DW_FORM_addr
	.byte 0x3f, 0x0c ; DW_AT_external, DW_FORM_flag
	.byte 0x00, 0x00 ; end of attributes
	.byte 0x00       ; end of table
```

#### `.debug_info`

Contains the compile-unit DIE (abbrev 1) followed by one subprogram DIE
(abbrev 2) per function.

Structure:

```asm
	.section .debug_info
.Ldebug_info_begin:
	; CU header
	.long .Ldebug_info_end - .Ldebug_info_begin - 4  ; unit_length
	.word 2              ; DWARF version 2
	.long 0              ; debug_abbrev_offset
	.byte 2              ; address_size = 2 (Z80 is 16-bit)

	; Compile unit DIE (abbrev 1)
	.byte 1
	.ascii "xcc 0.1.0"  ; DW_AT_producer (null-terminated)
	.byte 0
	.word 0x000c         ; DW_AT_language = DW_LANG_C99
	.ascii "source.c"   ; DW_AT_name (null-terminated)
	.byte 0
	.word _first_func    ; DW_AT_low_pc  (first function entry)
	.word .Ldbg_last_func_end  ; DW_AT_high_pc (past last function)
	.long 0              ; DW_AT_stmt_list (offset into .debug_line)

	; One subprogram DIE per function (abbrev 2)
	.byte 2
	.ascii "funcname"   ; DW_AT_name
	.byte 0
	.word _funcname      ; DW_AT_low_pc
	.word .Ldbg_funcname_end  ; DW_AT_high_pc
	.byte 1              ; DW_AT_external (1 = global, 0 = static)

	; ... one DIE per function ...

	.byte 0              ; null DIE (end of CU children)
.Ldebug_info_end:
```

String values (`DW_FORM_string`) are emitted as `.ascii "text"` followed by
`.byte 0` (null terminator).

`DW_AT_language` is always `0x000c` (DW_LANG_C99) regardless of the actual
C standard used.

`DW_AT_low_pc` of the compile unit is the entry label of the first function.
`DW_AT_high_pc` of the compile unit is `.Ldbg_LAST_end` — the end label of
the last function emitted.

#### `.debug_aranges`

Provides fast lookup of which compile unit owns a given address range.  One
tuple is emitted per function.

```asm
	.section .debug_aranges
.Ldebug_aranges_begin:
	; Header
	.long .Ldebug_aranges_end - .Ldebug_aranges_begin - 4
	.word 2    ; version
	.long 0    ; debug_info_offset
	.byte 2    ; address_size = 2
	.byte 0    ; segment_size = 0
	.word 0    ; padding to 4-byte alignment

	; One tuple per function: (start_addr, length)
	.word _funcname
	.word .Ldbg_funcname_end - _funcname

	; ... one tuple per function ...

	; Terminator
	.word 0
	.word 0
.Ldebug_aranges_end:
```

#### `.debug_line`

xcc does **not** emit `.debug_line` directly.  The assembler generates it
automatically from the `.file` and `.loc` directives emitted inline during
code generation.

---

### Complete example (`-masm=gnuas -g`)

Source (`add.c`):

```c
int add(int a, int b) {
    return a + b;
}
```

Annotated assembly output:

```asm
	.file 1 "add.c"
	.text
	.global _add
_add:
	push	ix
	ld	ix, 0
	add	ix, sp
	.loc 1 2 0
	ld	l, (ix+4)
	ld	h, (ix+5)
	ld	e, (ix+6)
	ld	d, (ix+7)
	add	hl, de
	jp	__add_end
__add_end:
	ld	sp, ix
	pop	ix
	ret
.Ldbg_add_end:

	.section .debug_abbrev
	.byte 1
	; ... (see above)

	.section .debug_info
.Ldebug_info_begin:
	; ... (see above)

	.section .debug_aranges
.Ldebug_aranges_begin:
	; ... (see above)
```

---

## Choosing a format

| Criterion | SDCC `;!` (sdasz80) | DWARF 2 (gnuas) |
|-----------|---------------------|-----------------|
| Debugger | SDCDB | GDB, LLDB |
| File overhead | Minimal (comments) | Three binary sections |
| Type information | Named type aliases, param locations | Function address ranges only |
| Line information | Inline `; !LINE` | `.file`/`.loc` → `.debug_line` |
| Companion file | `.adb` (function map) | None |
| Integration | Z88DK / SDCC toolchain | GNU binutils toolchain |
