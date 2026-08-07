# xcc — X C Compiler for Z80

A clean, modern C11 compiler for the Z80 processor.  Built from scratch in
C++17 with a hand-written parser, typed IR, and GCC-compatible command line.

---

## Quick start

xcc is a GNU-style compiler driver: it compiles C itself and spawns
`xas` to assemble and `xld` to link, exactly like gcc drives `as`/`ld`.

```bash
# Build
make

# Compile, assemble, and link in one step (default output: a.out)
xcc hello.c -o hello.xl

# Multiple translation units, mixed input kinds
xcc main.c util.s extra.rel -o app.xl

# Compile and assemble only (-c), emit hello.rel
xcc -c hello.c

# Compile only (-S), emit hello.s
xcc -S hello.c

# Inspect the assembly on stdout
xcc -S hello.c -o -

# Compile preprocessed stdin directly to assembly (SDCC c1-mode compatibility)
cpp -Iinclude -DVALUE=1 hello.c | xcc --c1mode -mz80 --opt-code-size -o hello.s

# Forward flags to the linker
xcc hello.c --oformat=binary -Ttext=0x8000 -o hello.bin
xcc hello.c -nostdlib -Wl,-Map=hello.map -o bare.xl
```

---

## Installation

After `make dist`, the staging directories under `build/dist/` mirror
`/usr/local/`:

```
build/dist/
  bin/
    xcc                  <- the compiler binary
  lib/xcc/runtime/
    *.rel                <- per-helper Z80 runtime objects
  z80/include/
    *.h                  <- canonical target-side libc headers
```

To install system-wide on Linux:

```bash
make dist
sudo cp build/dist/bin/xcc                /usr/local/bin/
sudo mkdir -p /usr/local/lib/xcc/runtime
sudo cp build/dist/lib/xcc/runtime/*.rel  /usr/local/lib/xcc/runtime/
sudo mkdir -p /usr/local/z80/include
sudo cp -r build/dist/z80/include/.       /usr/local/z80/include/
```

---

## Building from source

Requirements: `g++` ≥ 7 with C++17, GNU Make.
Optional (for runtime and linking): `sdasz80` (Z80 assembler) and `sdldz80` (Z80 linker).

```bash
make                   # debug build  (build/bin/xcc)
make BUILD=release     # release build
make dist              # stage under build/dist/
make test              # run regression suite (47 tests)
make clean             # remove build artefacts
```

---

## Status vocabulary

The feature tables below use the following labels:

| Label | Meaning |
|-------|---------|
| `tested` | Feature works correctly and is covered by a regression test |
| `codegen-complete` | Code is generated correctly but no dedicated regression test |
| `runtime-stub` | Frontend + codegen work; runtime helper is a non-functional stub — link a real library to use |
| `sema-complete` | Parsed and type-checked but code generation is incomplete or wrong |
| `parsed` | Syntax accepted; no semantic checking or code generation |

---

## Supported C11 subset

### Types

| Type | Status |
|------|--------|
| `int`, `char`, `short`, `long` | `tested` |
| `unsigned` variants | `tested` |
| `long long` (add/sub inline; mul/div/mod via stubs) | `runtime-stub` |
| `float`, `double` (stored as 4-byte IEEE 754; arithmetic via stubs) | `runtime-stub` |
| `void`, `_Bool` | `tested` |
| `_Complex` / `_Imaginary` (8-byte soft-float pair; arithmetic via stubs) | `runtime-stub` |

### Declarations

| Feature | Status |
|---------|--------|
| Global variables (with initializer) | `tested` |
| Local variables (`auto`) | `tested` |
| `static` local variables | `tested` — mangled global with static storage duration, no `.globl` export |
| File-scope `static` (internal linkage) | `codegen-complete` — no `.globl` emitted for static functions/variables |
| `extern` declarations | `codegen-complete` — no storage allocated; references external symbol |
| `typedef` | `tested` |
| `const` enforcement | `tested` — assignments to `const` lvalues are compile errors |
| `volatile`, `restrict` | `parsed` |
| `_Alignas` | `parsed` |
| `_Atomic` / `<stdatomic.h>` | `runtime-stub` — DI/EI-wrapped stubs; correct only if no preemption between DI and EI |

### Aggregate types

| Feature | Status |
|---------|--------|
| `struct` — field read/write, `->`, compound-assign, nested | `tested` |
| `union` | `codegen-complete` |
| `enum` | `tested` |
| Bit-fields | `codegen-complete` |
| Anonymous struct/union members | `codegen-complete` |
| Variable-length arrays (VLA) | `codegen-complete` |
| Flexible array members | `codegen-complete` |

### Statements

| Feature | Status |
|---------|--------|
| `if` / `else` | `tested` |
| `while`, `do`…`while`, `for` | `tested` |
| `switch` / `case` / `default` | `tested` — chain-of-if-else dispatch |
| `break`, `continue` | `tested` |
| `goto` and named labels | `tested` |
| `return` | `tested` |

### Expressions

| Feature | Status |
|---------|--------|
| All integer arithmetic `+ - * / %` | `tested` (`*`/`/`/`%` via runtime helpers for 16-bit) |
| Bitwise `& \| ^ ~ << >>` | `tested` |
| Logical `&& \|\| !` (short-circuit) | `tested` |
| All comparisons `== != < <= > >=` | `tested` |
| Assignment `=`, compound `+=` etc. | `tested` |
| Pre/post `++` / `--` | `tested` |
| Ternary `?:` | `tested` |
| Address-of `&`, dereference `*` | `tested` |
| Array subscript `[]` | `tested` |
| Struct member `.` and `->` | `tested` |
| Function call | `tested` |
| Function pointers | `tested` |
| `sizeof`, `_Alignof` | `tested` |
| Explicit casts | `tested` |
| Comma operator | `codegen-complete` |
| `_Generic` | `codegen-complete` |
| `__builtin_expect(e, hint)` | `codegen-complete` — returns `e`, hint discarded |

### Literals and strings

| Feature | Status |
|---------|--------|
| String literals, adjacent concatenation | `tested` |
| Wide/Unicode prefixes `L"" u"" U"" u8""` | `codegen-complete` |
| Integer suffix `L` `U` `LL` `UL` | `tested` |
| Hex/octal/unicode escapes `\x` `\NNN` `\uXXXX` | `tested` |
| Hex float literals `0x1.8p+1` | `codegen-complete` |
| Aggregate initializers `{…}` — local and global | `tested` |
| Designated initializers `.field = v`, `[N] = v` | `codegen-complete` |
| Compound literals `(T){…}` | `codegen-complete` |

### Functions

| Feature | Status |
|---------|--------|
| Declarations, definitions, recursion | `tested` |
| Variadic (`...`) — syntax accepted; `<stdarg.h>` macros | `codegen-complete` |
| Inline assembly `__asm__` / `__asm` | `codegen-complete` |
| `inline`, `_Noreturn` | `parsed` |

### Preprocessor

| Feature | Status |
|---------|--------|
| Object/function-like macros, `#define` / `#undef` | `tested` |
| `#include` | `tested` |
| `#if` / `#ifdef` / `#ifndef` / `#elif` / `#else` / `#endif` | `tested` |
| `#error` | `tested` |
| `__FILE__` / `__LINE__` / `__DATE__` / `__TIME__` | `tested` |
| `defined()` in `#if` | `tested` |
| Variadic macros, `#` stringify, `##` paste | `tested` |
| `_Pragma("...")` | `parsed` — accepted as no-op |
| Line markers `# N "file"` | `codegen-complete` |

### GNU extensions

| Feature | Status |
|---------|--------|
| `__attribute__((...))` everywhere | `parsed` — ignored |
| `__extension__` prefix | `parsed` — ignored |
| GNU keyword aliases (`__inline__`, `__volatile__`, etc.) | `codegen-complete` |
| `static` in array params `f(int a[static 10])` | `parsed` — ignored |
| `__func__` predefined identifier | `codegen-complete` |

### C11 keywords not yet supported

| Feature | Status |
|---------|--------|
| `_Thread_local` | `runtime-stub` — TLS address sequence emitted; requires OS hook `__tls_base()` |
| `_Static_assert` | `tested` |
| Bit-precise integers `_BitInt` | not implemented |

---

## ABI — xcc Z80 calling convention

| Item                        | Value                           |
|-----------------------------|---------------------------------|
| Frame pointer               | **IX**                          |
| First local variable        | `IX - 2`                        |
| First parameter (stack)     | `IX + 4`                        |
| Return value ≤ 1 byte       | **L** register                  |
| Return value ≤ 2 bytes      | **HL** register                 |
| Return value ≤ 4 bytes      | **DE:HL** (DE = high word)      |
| Parameter passing           | right-to-left push on stack     |
| Callee-saved registers      | IX only                         |
| Stack cleanup               | ABI-sensitive; `sdcccall(1)` follows SDCC's return-sensitive callee-clean rule |

See [docs/HOWTO.md](docs/HOWTO.md) for calling xcc functions from assembly and
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the compiler internals.

---

## Runtime library

The runtime modules under `/usr/local/lib/xcc/runtime/` provide:

| Symbol        | Purpose                                               | Status |
|---------------|-------------------------------------------------------|--------|
| `__mul16`     | 16-bit signed multiply (HL × DE → HL)                | working |
| `__div16`     | 16-bit signed divide   (HL ÷ DE → HL)                | working |
| `__mod16`     | 16-bit signed modulo   (HL % DE → DE)                | working |
| `__mul32`     | 32-bit multiply                                       | working |
| `__div32`     | 32-bit divide                                         | working |
| `__mod32`     | 32-bit modulo                                         | working |
| `__call_hl`   | Indirect call trampoline (`jp (hl)`)                  | working |
| `__fsadd`     | Soft-float add                                        | **stub** |
| `__fssub`     | Soft-float subtract                                   | **stub** |
| `__fsmul`     | Soft-float multiply                                   | **stub** |
| `__fsdiv`     | Soft-float divide                                     | **stub** |
| `__fitosf`    | Integer to soft-float conversion                      | **stub** |
| `__fstoi`     | Soft-float to integer conversion                      | **stub** |
| `__mulll`     | 64-bit multiply                                       | **stub** |
| `__divll`     | 64-bit divide                                         | **stub** |
| `__modll`     | 64-bit modulo                                         | **stub** |
| `__atomic_*`  | Atomic load/store/exchange/CAS (DI/EI-wrapped)        | **stub** |

Stub symbols return zero and are present only to satisfy the linker.  Replace
`__fs*` with a real soft-float library (e.g., SDCC's `libsdcc`) and `__mulll`
etc. with 64-bit helpers before using `float`, `double`, or `long long`
arithmetic.

---

## Project structure

```
xc/
├── Makefile
├── README.md
├── CPP-CODING-STYLE.md     C++ coding conventions for this project
├── Z80-CODING-STYLE.md     Z80 assembly coding conventions
├── build/                  compiler and staging output
├── lib/
│   ├── runtime/            Z80 runtime helper sources
│   ├── include/            bundled C headers (stdarg.h, stdatomic.h, complex.h)
│   └── ...                 other project libraries
├── include/                public compiler headers
│   ├── frontend/           token, lexer, types, symtab, ast, parser, sema, preproc
│   ├── ir/                 icode, irgen, iropt
│   ├── backend/            asm_emitter (abstract), sdasz80_emitter, gnuas_emitter
│   │   └── z80/            z80gen, z80peep, dwarf
│   └── driver/             options
├── src/                    implementation files (.cpp)
│   ├── frontend/
│   ├── ir/
│   ├── backend/z80/
│   └── driver/
├── ../../docs/reference/   SDCC / GNU reference sources (not compiled)
├── docs/
│   ├── HOWTO.md            compilation switches, C/asm interop guide
│   ├── ARCHITECTURE.md     compiler internals and design decisions
│   ├── TODO.md             C11 compliance tracker
│   └── PROPOSALS.md        refactoring and improvement roadmap
└── tests/
    ├── run_tests.sh
    └── data/
        └── core/           47 regression tests (t001 … t047)
```

---

## Tests

```bash
make test                                          # run all 47 regression tests
GENERATE=1 bash tests/run_tests.sh ./build/bin/xcc # update baselines
```

When you intentionally change code-generation output, regenerate baselines with
`GENERATE=1` before committing.

Most snapshot tests run with `-O0` by default and compare normalized
assembly snapshots. Some optimization regressions pin explicit `-O2`
or `-Os` outputs.

---

## License

xcc is original work released under the MIT License.  The SDCC sources in `../../docs/reference/sdcc/` are used only as a Z80 reference and are not compiled into xcc.
