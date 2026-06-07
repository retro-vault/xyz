# xcc — C11 Compliance TODO

Tracks missing C11 language features. Update this file when a feature is implemented.
Items are grouped by effort and impact. Features marked **Z80 N/A** are in the standard
but meaningless on a single-threaded, bare-metal Z80 target.

**Current status (2026-05-26):** 50/50 tests passing.

---

## Implemented (for reference)

| Feature | Notes |
|---------|-------|
| All integer types + `_Bool` | Z80 sizes: char=1, short/int=2, long=4 |
| `float` / `double` | Stored as 4-byte IEEE 754; arithmetic via soft-float stubs (`__fsadd/__fssub/__fsmul/__fsdiv/__fitosf/__fstoi`); binary operator type now inferred from operands when parser annotation absent |
| Pointers (any depth), arrays (fixed-size) | 16-bit flat address space |
| `struct`, `union` | Field read/write, compound-assign on fields, pointer-to-struct (`->`), nested structs, aggregate `{...}` init (locals); bit-fields; anonymous struct/union promotion |
| `enum` | Enumerators usable as integer constants |
| `typedef` | Scalars, structs, function pointer types; names fully resolved in type-start detection |
| `const`, `volatile`, `restrict` | Parsed; `const` enforced — assignments to const-qualified lvalues are compile errors |
| `if/else`, `while`, `do`, `for`, `break`, `continue`, `return` | |
| `switch`/`case`/`default` | Parsed and sema-complete; **dispatch broken** — condition discarded, all case bodies execute |
| `goto` and named labels | `goto label;` emits jump; **named label declarations do not parse** inside function bodies |
| Functions — declarations, definitions, recursion, variadic (parsed) | |
| All arithmetic, bitwise, logical, comparison operators | `*`/`/`/`%` via 16-bit runtime helpers |
| Compound assignment `+=` etc., pre/post `++`/`--` on scalars and struct fields | |
| Ternary `?:`, comma operator | |
| Explicit casts | |
| `sizeof`, `_Alignof(type)` | |
| `_Static_assert(expr, msg)` | Built-in constant expression evaluator |
| `__func__` predefined identifier | |
| String literals, adjacent concatenation | |
| Wide/Unicode string prefixes `L""` `u""` `U""` `u8""` | `u""` → char16_t (`.dw`, 2 bytes/elem); `U""` → char32_t (`.dl`, 4 bytes/elem); `L""` → wchar_t (`.dw`); `u8""` → plain char |
| Aggregate initializers `{...}` | Local arrays and structs; globals via `init_vals` → per-element `.db`/`.dw` in `_DATA` |
| Function pointers | Declare, assign, call through variable, pass as arg, return from function; indirect call via `__call_hl` runtime trampoline |
| `inline`, `_Noreturn` | Accepted, ignored |
| `_Complex` / `_Imaginary` | 8-byte soft-float pair; `+`/`-`/`*` lowered to component FADD/FSUB/FMUL; `<complex.h>` with `I`, `CMPLXF`, `creal`, `cimag`, `conj`, `cabs`, `carg` |
| `_Alignas` | Accepted, ignored (Z80 has no alignment requirements) |
| `_Thread_local` | Per-thread globals via `__tls_base()` OS hook; compiler emits TLS-address sequence; OS must copy `__tls_template` at thread start |
| `_Atomic` / `<stdatomic.h>` | `lib/libc/include/stdatomic.h` with `_Generic` macros; 18 `DI`/`EI`-wrapped stubs in `lib/runtime/`; OS can replace with lock-free versions |
| Global aggregate initializers `{…}` | `int a[3] = {1,2,3};` at file scope; per-element `.dw`/`.db` in `_DATA` |
| Compound assignment on array subscript | `a[i] += n`, `a[i] -= n`, etc. — write-back via ADD+SET_VALUE_AT |
| Designated initializers | `.field = value` and `[N] = value`; struct fields by name, array elements by absolute offset; `init_list_expr::elem` carries designation info |
| Floating-point arithmetic (soft-float stubs) | FADD/FSUB/FMUL/FDIV/FITOSF/FSTOI opcodes; `gen_float_arith()` calls `__fsadd/__fssub/__fsmul/__fsdiv/__fitosf/__fstoi` stubs |
| `long long` ADD / SUB | Inline 4-word carry/borrow chain in z80gen; `is_llong_op()` guard |
| `long long` MUL / DIV / MOD | `__mulll`, `__divll`, `__modll` stubs; args pushed as 4 words each |
| `_Static_assert` with enum constants | `eval_const_expr()` resolves `ident_expr` with `sym_kind::ENUM_CONST` via `sym->enum_val` |
| `__asm__` / `__asm` inline assembly | Basic form: `__asm__("nop");` — verbatim text emitted to output |
| 32-bit `long` ADD / SUB | Inline carry-chain in z80gen; DE:HL register pair |
| 32-bit `long` MUL / DIV / MOD | `__mul32`, `__div32`, `__mod32` runtime helpers in `lib/runtime.s` |
| `\x` hex escape sequences | `\x41` → 65; `lex_escape()` in `lexer.cpp` handles arbitrary hex width |
| Octal escape sequences | `\101` → 65; up to 3 octal digits after `\` |
| `\uXXXX` / `\UXXXXXXXX` escapes | Unicode code points narrowed to low 8 bits (Z80 is 8-bit) |
| GNU keyword aliases | `__volatile__`, `__const__`, `__signed__`, `__inline__`, `__inline`, `__restrict__` → standard keywords |
| `__extension__` no-op | Accepted and discarded as a unary prefix (GCC extension marker) |
| `__attribute__((...))` | Parsed and ignored everywhere declarations/declarators appear |
| `static` in array params | `void f(int a[static 10])` — `static` and qualifiers after `[` accepted and ignored |
| `_Pragma("...")` no-op | Parsed and discarded as an empty statement |
| Integer suffix type widening | `100L` → `long`; `42U` → `unsigned int`; `0xFFUL` → `unsigned long` |
| `__builtin_expect(e, hint)` | Returns `e` unchanged; hint discarded |
| `# N "filename"` line directives | Lexer updates `line_` and `file_` from preprocessor line markers |
| C preprocessor | Object/function-like macros, `#include`, `#ifdef/#if/#elif/#else/#endif`, `#error`, `#define`/`#undef`, `defined()`, `#`, `##`, `__FILE__`/`__LINE__`/`__DATE__`/`__TIME__`, variadic macros |
| `-O2` IR optimizer | Constant fold, copy-prop (TEMP+INT_CONST), DCE (fixed-point), strength reduction (MUL/DIV/MOD by power-of-two → shift) |
| `-O2` register allocator | Stable bounded BC allocator for one short-lived 16-bit temp; broader local / byte allocation still pending |
| `-O1` / `-O2` peephole | 12 rules including push/pop elimination, `ex de,hl` substitution, `xor a` for zero loads, inline shifts |

---

## Missing features

### Not yet implemented

| Feature | Notes |
|---------|-------|
| `_BitInt` | C23 bit-precise integers |
| `_Generic` with non-identity compatible types | Current matching is exact kind+tag |
| Soft-float arithmetic | Codegen calls `__fsadd` etc.; link SDCC's `libsdcc` for real IEEE 754 |
| `long long` mul/div/mod | Codegen calls `__mulll/__divll/__modll`; stubs return 0 |

### Z80 N/A — in C11 standard but not practical on bare-metal Z80

| Feature | Notes |
|---------|-------|
| `_Thread_local` | Stub; requires OS hook `__tls_base()` to be meaningful |
| `_Atomic` | Stub; DI/EI correct only on non-preemptive Z80 |

---

## Completed infrastructure

| Item | Notes |
|------|-------|
| **`_Thread_local`** | Per-thread globals via `__tls_base()` OS hook. Compiler emits `call __tls_base; ld bc, #offset; add hl, bc` then loads/stores through HL. OS must initialise TLS block (size = `__tls_size`) from `__tls_template` at thread start. |
| **`_Atomic` / `<stdatomic.h>`** | `lib/libc/include/stdatomic.h` — full C11 API via `_Generic` macros mapping to 18 runtime stubs in `lib/runtime/`. All stubs use `DI`/`EI` for atomicity on preemptive Z80. Covers load/store/exchange/CAS/fetch-add/sub/and/or/xor for 1-byte and 2-byte types + `atomic_flag`. OS can override any stub with a lock-free version. |
| **Register allocator (-O2)** | `regalloc_prepass()` in `z80gen.cpp`: bounded linear-scan over BC for short straight-line 16-bit temp windows, with backend hazard checks. IR optimizer (`src/opt/iropt.cpp`): CFG/value/loop-aware IR pass pipeline activated by `-O2`. |
| **Built-in preprocessor** | `src/frontend/preproc.cpp`: object/function-like macros, `#include`, `#ifdef/#ifndef/#if/#elif/#else/#endif`, `#error`, `#pragma` (no-op), `__FILE__`/`__LINE__`/`__DATE__`/`__TIME__`, `defined()` in `#if`, variadic macros, stringify `#`, token-paste `##`, recursion guard, depth limit 32, `# linenum "file"` markers for lexer |
| **`<stdarg.h>` header** | `lib/libc/include/stdarg.h` — `va_list = char*`; `va_start`/`va_arg`/`va_end`/`va_copy` macros using xcc Z80 ABI stack layout |
| **Global aggregate init in codegen** | `emit_globals()` in `z80gen.cpp` emits per-element `.db`/`.dw` from `g.init_vals`; 4-byte values split into two `.dw` words |
| **Peephole rule set** | `z80peep.cpp`: temp-store/reload elimination (29% overall reduction), push/pop HL pairs, redundant `ld`, self-store no-op, dead HL load; runs up to 10 passes to fixed point |
| **Soft-float runtime stubs** | `lib/runtime.s` exports `__fitosf/__fstoi/__fsadd/__fssub/__fsmul/__fsdiv` as zero-returning stubs. Link SDCC's `libsdcc` for real IEEE 754. |
| **32-bit runtime helpers** | `__div32`/`__mod32` full 32-bit shift-subtract (32 iterations, alternate register set for quotient); `__mul32` shift-and-add with 16-bit cross terms |
| **CPP-CODING-STYLE.md coding conventions** | All 22 source files updated: file header comments, full function documentation in headers, snake_case naming for all types and classes, public headers moved to `include/` subdirectories with redirect stubs in `src/` |
| **`__call_hl` trampoline** | `lib/runtime.s` — enables indirect function-pointer calls (`jp (hl)` trick) |
| **Global aggregate initializers** | `int a[3]={1,2,3};` at file scope; irgen flattens `init_list_expr` into per-element `init_elem` pairs; z80gen emits `.db`/`.dw` per element |
| **Compound assignment on array subscript** | `a[i] += n` — missing `index_expr` write-back added to `gen_compound_assign` in `irgen.cpp` |
| **`__asm__` / `__asm` inline assembly** | `KW___ASM__` token; `asm_stmt` AST node; `parse_asm_statement()` in parser; `INLINE_ASM` icode op; verbatim emission in `z80gen.cpp` |
| **32-bit `long` arithmetic** | Inline carry-chain ADD/SUB; `__mul32` (shift-and-add + cross terms via `__mul16`); `__div32`/`__mod32` stubs; `gen_return`/`gen_assign` handle 4-byte operands |
| **DWARF 2 debug info (`-g`)** | `dwarf_emitter` class in `src/backend/z80/dwarf.cpp`; `.file`/`.loc` directives inline; `.debug_abbrev`, `.debug_info`, `.debug_aranges` sections at module end; `icode::line` populated from `stmt.loc.line` in irgen; activated by `-g` flag |
| **Variable-length arrays (VLA)** | `is_vla` flag on type; hidden size + pointer locals; `ALLOCA` icode bumps SP at runtime; `sizeof(vla)` returns stored byte count; `vla_size_sym` on symbol |
| **Flexible array members** | `struct S { int n; int data[]; }` — already correct via `array_size = 0`; `sizeof(S)` excludes the tail; field offsets correct |
| **Unicode string types** | `u""` → char16_t (2 bytes/element, `.dw`); `U""` → char32_t (4 bytes/element, `.dl`); `L""` → wchar_t (2 bytes). `char_width` tracked through token → AST → IR → z80gen |
| **Hex float literals** | `0x1.8p+1` — lexer now consumes optional `.`+hex-fraction before `p`; uses `strtod` (C99-guaranteed hex float parsing) |
| **`__func__` outside a function** | Emits `warning: '__func__' used outside a function` on stderr; yields `""` as before |
| **Compound literals** | `(struct Point){1, 2}` — `compound_literal_expr` AST node; anonymous local allocated by parser; lowered via `gen_init_list()` in irgen |
| **Variadic `va_list` / `va_arg`** | `lib/libc/include/stdarg.h` header with `va_list = char*`; `va_start`/`va_arg`/`va_end`/`va_copy` macros for xcc Z80 ABI stack layout |
| **Bit-fields** | `struct_field::bit_width` + `bit_offset`; packed within storage units in `parse_struct_body()`; read uses SHR+BAND mask; write uses load-modify-store in irgen |
| **Anonymous structs/unions** | Field promotion in `parse_struct_body()` via `promote_anon()` lambda; handles both unnamed and tag-only (no declarator) inner structs/unions |
| **`const` enforcement** | New semantic pass `src/frontend/sema.cpp`; `sema::check()` walks the full AST; reports error for any assignment to a `const`-qualified lvalue |
| **`_Generic` selection** | `parse_generic_selection()` in parser; `types_compatible()` compares unqualified types by kind/tag; `default:` fallback; matched expression returned directly (no runtime overhead) |
| **`_Complex` / `_Imaginary`** | `type_kind::COMPLEX` (8 bytes: re+im as 4-byte soft-floats each); `+`/`-`/`*` lowered in `irgen` to component `FADD`/`FSUB`/`FMUL` + `MAKE_COMPLEX` pack; `byte_offset` on `operand` for component addressing; `sz=8` alloc in `alloc_temp`; `lib/libc/include/complex.h` with `I`, `CMPLXF`, `creal`, `cimag`, `conj`, `cabs`, `carg`; `__fsneg`/`__creal`/`__cimag` in `lib/runtime/`, with the remaining helpers supplied by libc; binary-op type inference from operands when parser annotation absent |
