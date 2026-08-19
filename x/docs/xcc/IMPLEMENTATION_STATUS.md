# xcc Implementation Status

This document is the canonical record of what is fully supported, what is
partial, and what is stub-backed or intentionally simplified in xcc.

Update this document whenever a pass boundary changes, a stub is replaced,
or a known limitation is fixed.

---

## Frontend

| Feature | Status | Notes |
|---------|--------|-------|
| Integer arithmetic (8/16/32 bit) | Complete | All ops through runtime helpers |
| 64-bit integers (`long long`) | Complete | Lowered through 64-bit runtime helpers |
| Floating-point (`float`, `double`) | Complete | M aliases `double` to the selected float ABI; L uses 64-bit soft-double helpers |
| `_Complex` / `_Imaginary` | Partial | Type exists; component access works; arithmetic is stub-backed |
| `_Bool` | Complete | |
| Pointers | Complete | Including pointer arithmetic, function pointers |
| Arrays | Complete | Fixed-size; VLA (runtime-sized) is partial |
| VLA (variable-length arrays) | Partial | Size local allocated; no `alloca`-style stack bump yet |
| Structs and unions | Complete | Including bit-fields, anonymous members, nested aggregates |
| Enums | Complete | Treated as `int` |
| `typedef` | Complete | |
| `const`, `volatile`, `restrict` | Partial | `const` enforced; `volatile`/`restrict` parsed but not enforced |
| `_Atomic` | Parsed only | No memory model semantics; atomic ops are stubs |
| `_Thread_local` | Partial | TLS base pointer scheme; no actual thread-local storage in runtime |
| `inline` | Parsed only | No inlining; treated as a hint with no effect |
| `_Noreturn` | Parsed only | |
| `static` locals | Complete | Mangled to global storage |
| `extern` declarations | Complete | Reference only; no storage allocated |
| `register` | Parsed only | Ignored |
| Designated initializers | Complete | Field (`.name =`) and array index (`[N] =`) |
| Compound literals | Complete | |
| `sizeof` / `_Alignof` | Complete | Returns target byte size; align always 1 on Z80 |
| Generic selection (`_Generic`) | Complete | |
| `goto` / labeled statements | Complete | |
| `switch` / `case` / `default` | Complete | Linear compare chain (no jump table) |
| `_Static_assert` | Complete | |
| Inline assembly (`__asm__`) | Complete | Basic form; no operand constraints |
| `__attribute__((...))` | Parsed only | Ignored |
| `__builtin_expect` | Complete | Returns first argument unchanged |
| `__func__` | Complete | |
| Variadic functions | Complete | `va_list` via `lib/libc/include/stdarg.h`; variadic callees are forced to stack-only `sdcccall(0)` |
| Function prototypes | Complete | |
| Multiple declarators | Partial | `typedef int a, b;` works; global multi-declarator vars partially handled |
| Wide string/char literals | Complete | `L""`, `u""`, `U""`, `u8""` |

---

## IR Generation and Optimization

| Feature | Status | Notes |
|---------|--------|-------|
| Arithmetic, logic, shifts | Complete | All binary and unary ops |
| Comparisons | Complete | All six relational ops |
| Logical AND / OR (short-circuit) | Complete | |
| Ternary (`?:`) | Complete | |
| Lvalue reads and writes | Complete | Including pointer dereference, member access, index |
| `SET_VALUE_AT` (write-through-pointer) | Complete | `result` = address, `left` = value |
| Casts | Complete | Truncating and sign-extending |
| Address-of | Complete | |
| Calls (direct and indirect) | Complete | |
| Global variable emission | Partial | Only first declarator in a multi-declarator global line |
| Aggregate initialization | Complete | For locals and globals |
| Compound literals | Complete | |
| Switch lowering | Partial | Linear compare chain; no jump table or binary search |
| Constant fold | Complete | All foldable binary/unary ops on INT_CONST |
| Algebraic simplify | Complete | `x+0→x`, `x*0→0`, `x&0→0`, `x^0→x`, etc. |
| Copy propagation | Complete | TEMP→TEMP and TEMP→INT_CONST |
| Dead code elimination | Complete | Pure instructions with unused result |
| Strength reduction | Complete | MUL/DIV/MOD by power-of-two → SHL/SHR/BAND |

---

## Z80 Backend

| Feature | Status | Notes |
|---------|--------|-------|
| IX-based stack frame | Complete | |
| 8-bit values (A register) | Complete | |
| 16-bit values (HL/DE/BC) | Complete | |
| 32-bit values (DE:HL) | Complete | |
| 64-bit values | Complete | Runtime ABI uses `DE:HL:DE':HL'` |
| Floating-point codegen | Complete | Delegates to runtime soft-float helpers |
| Register allocation (-O2) | Partial | Bounded stable allocator for one 16-bit temp in BC; wider/local-byte allocation still pending |
| Alternate register set (EXX regions) | Reserved | `temp_home` enum extended; no EXX regions yet |
| Calling convention | Complete | Right-to-left push; IX frame; `sdcccall(1)` uses SDCC-compatible return-sensitive stack cleanup |
| Indirect calls | Complete | Via `__call_hl` trampoline |
| Inline assembly passthrough | Complete | |
| DWARF 2 debug info | Complete | When `-g` is passed |
| sdasz80 output | Complete | Default `-masm=sdasz80` |
| GNU as output | Complete | `-masm=gnuas` |

---

## Peephole Optimizer

Activated at `-O1` and above.  19 hand-written rules run to fixed-point (up to 10 passes).
See `x/src/xcc/docs/ARCHITECTURE.md` for the complete rule table.

---

## Runtime library (`x/runtime/`)

| Symbol | Status |
|--------|--------|
| `__mul16`, `__div16`, `__mod16` | Working |
| `__mul32`, `__div32`, `__mod32` | Working |
| `__call_hl` | Working |
| `__fsadd`, `__fssub`, `__fsmul`, `__fsdiv` | Working |
| `__fitosf`, `__fstoi` | Working |
| `__mulll`, `__divll`, `__modll` | Working |
| `__atomic_*` (18 symbols) | Stub (DI/EI guards, no real atomicity) |

---

## Test Coverage

| Area | Test count | Notes |
|------|------------|-------|
| End-to-end assembly snapshot | 50 | `tests/data/core/t001` – `t050` |
| Parser unit tests | 0 | Planned (see `docs/SIMPLIFICATION.md`) |
| IR snapshot tests | 0 | Planned |
| Peephole rule tests | 0 | Planned (one in/out case per rule) |
| Backend instruction-selection tests | 0 | Planned |
