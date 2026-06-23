# xcc — How-To Guide

## Table of Contents

1. [Compilation switches](#compilation-switches)
2. [Quick compilation examples](#quick-compilation-examples)
3. [Supported C11 features](#supported-c11-features)
4. [ABI and calling convention](#abi-and-calling-convention)
5. [Calling xcc functions from assembly](#calling-xcc-functions-from-assembly)
6. [Calling assembly functions from C](#calling-assembly-functions-from-c)
7. [Stack frame layout](#stack-frame-layout)
8. [Runtime library](#runtime-library)
9. [Linking](#linking)

---

## Compilation switches

### Output control

| Switch           | Description |
|------------------|-------------|
| `-o FILE`        | Write output to FILE. Use `-o -` to write to stdout. |
| `-S`             | Stop after compiling; produce assembly (`.s`) output. This is xcc's primary output mode. |
| `-c`             | Accepted for compatibility; xcc produces assembly, not object files directly. Assemble with `sdasz80`. |

### Optimisation

| Switch      | Description |
|-------------|-------------|
| `-O0`       | No optimisation (default). |
| `-O1`       | Enable peephole optimiser. Removes redundant loads, dead jumps, temp store/reload pairs. The simplest fixed-window peepholes are now table-driven; the more context-sensitive ones still use custom matchers. |
| `-O2`       | Enable general optimisation: module-level dead static-function elimination, constant actual-argument propagation, translation-unit constant-call evaluation for eligible private integer helpers including nested private-helper chains, helper calls fed from constant-valued locals or temps, and a small whitelist of pure runtime helpers, whole-function constant evaluation for eligible zero-argument integer functions over that same safe subset, including straightforward 32-bit integer code, dead-parameter elimination, identical-helper merging for eligible internal callees, CFG jump threading through label-only and `goto`-only blocks, scalar local promotion for simple helper-free 16-bit locals, IR constant-fold/DCE, strength reduction (multiply/divide/mod by power-of-two → shift), conservative `sdcccall(1)` register-parameter promotion for simple helper-free straight-line callees, dead-local frame compaction, the bounded stable temp register allocator for short straight-line 16-bit temp windows, automatic TEMP preallocation inside functions that already need an IX frame, smaller nearby `&local` / `&temp` address materialization, frameless zero-frame functions when safe, plus all `-O1` peephole rules. |
| `-Of`       | Enable speed optimisation: the promoted aggressive baseline plus speed-biased codegen choices and O3-proven peepholes that may spend a little size for fewer cycles. |
| `-O3`       | Enable experimental optimisation: the same proven baseline as `-Os`, plus speed, size, shape-changing, and superoptimizer-inspired peephole experiments. Here be dragons. |
| `-Os`       | Enable size optimisation: the protected record-setting aggressive size baseline. |

`xcc` also supports fine-grained overrides with `-f<name>` and
`-fno-<name>`. Current names include:

- `peephole`
- `dead-static-functions`, `const-arg-prop`, `const-call-eval`, `function-const-eval`, `dead-params`
- `merge-identical-functions`, `inline-static-functions`
- `cfg-cleanup`, `jump-threading`, `address-deref-fold`, `value-propagation`
- `constant-fold`, `algebraic-simplify`
- `loop-licm`, `loop-induction`, `strength-reduction`
- `dead-code-elim`, `scalar-local-promotion`
- `reg-param-promotion`, `duplicate-block-merge`, `merge-tails`
- `local-frame-compaction`, `regalloc`
  `regalloc` is now part of the stable `-O2` / `-Os` presets, and the
  explicit flag remains useful for bisects and lower-level experiments.
- `compare-ifx-fusion`, `frame-omit`, `prealloc-temp-frame`

### Assembler dialect

| Switch              | Description |
|---------------------|-------------|
| `-masm=sdasz80`     | Emit sdasz80 syntax (default): `#N` immediates, `N(ix)` frame refs, `.area`/`.globl` directives. |
| `-masm=gnuas`       | Emit GNU as syntax: plain `N` immediates, `(ix+N)` frame refs, `.text`/`.global` directives. |

### Preprocessor

xcc has a built-in preprocessor that handles `#include`, `#define`, `#if`/`#ifdef`/`#elif`/`#else`/`#endif`, `#error`, variadic macros, `#`/`##`, and `__FILE__`/`__LINE__`/`__DATE__`/`__TIME__`.

| Switch    | Description |
|-----------|-------------|
| `-DNAME`  | Predefine macro `NAME` as `1`. |
| `-DNAME=VAL` | Predefine macro `NAME` as `VAL`. |
| `-IDIR`   | Add `DIR` to the `#include` search path. |

### Diagnostics

| Switch      | Description |
|-------------|-------------|
| `-g`        | Emit DWARF 2 debug info (`.debug_info`, `.debug_aranges`, `.debug_abbrev`). |
| `-v`        | Verbose: print each pipeline stage to stderr. |
| `--version` | Print version string and exit. |
| `--help`    | Print switch summary and exit. |

### Standard

| Switch      | Description |
|-------------|-------------|
| `-std=c11`  | Accepted (xcc always compiles C11; this switch is a no-op included for script compatibility). |

---

## Quick compilation examples

### Inspect assembly for a single file

```bash
xcc -S hello.c -o hello.s
```

### Print assembly to stdout

```bash
xcc -S hello.c -o -
```


### Full build pipeline (compile → assemble → link)

```bash
# 1. Compile C to assembly (preprocessor is built-in)
xcc -O1 hello.c -o hello.s

# 2. Assemble with the Z80 assembler
sdasz80 -o hello.rel hello.s

# 3. Link: runtime modules are staged by make dist
sdldz80 -i hello.ihx /usr/local/lib/xcc/crt0.rel hello.rel \
        /usr/local/lib/xcc/runtime/*.rel

# 4. Convert to binary if needed
hex2bin hello.ihx
```

### Multi-file project

```bash
xcc -O1 module_a.c -o module_a.s
xcc -O1 module_b.c -o module_b.s
sdasz80 -o module_a.rel module_a.s
sdasz80 -o module_b.rel module_b.s
sdldz80 -i program.ihx /usr/local/lib/xcc/crt0.rel module_a.rel module_b.rel \
        /usr/local/lib/xcc/runtime/*.rel
```

---

## Supported C11 features

### Types

| Type                              | Size (bytes) | Notes |
|-----------------------------------|-------------|-------|
| `char` / `signed char`            | 1           | |
| `unsigned char`                   | 1           | |
| `short` / `signed short`          | 2           | |
| `unsigned short`                  | 2           | |
| `int` / `signed int`              | 2           | Z80 native word |
| `unsigned int`                    | 2           | |
| `long` / `signed long`            | 4           | |
| `unsigned long`                   | 4           | |
| `long long` / `signed long long`  | 8           | lowered through 64-bit runtime helpers |
| `unsigned long long`              | 8           | lowered through 64-bit runtime helpers |
| `float`                           | 4           | IEEE 754 single (32-bit) |
| `double`                          | 8           | IEEE 754 double (64-bit soft-float) |
| `_Bool`                           | 1           | |
| `void`                            | —           | |
| Pointer (any)                     | 2           | 16-bit flat address space |

### Declarations and definitions

| Feature                           | Status |
|-----------------------------------|--------|
| Global variables                  | ✓ |
| Local variables (auto)            | ✓ |
| `static` local                    | ✓ mangled global with static storage duration, no `.globl` |
| `static` file-scope               | ✓ no `.globl` emitted for static functions and variables |
| `extern`                          | ✓ no storage allocated; references existing external symbol |
| `register`                        | parsed (ignored) |
| `typedef`                         | ✓ |
| `const`                           | parsed (not enforced) |
| `volatile`                        | parsed (not enforced) |
| `_Alignas`                        | parsed (not enforced) |
| `_Atomic`                         | parsed (not enforced) |
| Struct (basic field access)       | ✓ |
| Union                             | ✓ (basic) |
| Enum                              | ✓ |
| Bit-fields                        | not yet |
| Variable-length arrays (VLA)      | not yet |
| Flexible array members            | not yet |

### Expressions

| Feature                           | Status |
|-----------------------------------|--------|
| Integer arithmetic `+ - * / %`   | ✓ (`*` `/` `%` via runtime helpers for 16-bit) |
| Unary minus / plus                | ✓ |
| Bitwise `& \| ^ ~ << >>`         | ✓ |
| Logical `&& \|\| !`              | ✓ (short-circuit) |
| Comparisons `== != < <= > >=`    | ✓ |
| Assignment `=`                    | ✓ |
| Compound assignment `+= -= *= /= %= &= \|= ^= <<= >>=` | ✓ |
| Pre/post `++` / `--`             | ✓ |
| Ternary `?:`                      | ✓ |
| Address-of `&`                    | ✓ |
| Dereference `*`                   | ✓ |
| Array subscript `[]`             | ✓ |
| Struct member `.`                 | ✓ |
| Struct pointer member `->`        | ✓ |
| Function call                     | ✓ |
| `sizeof(type)` / `sizeof(expr)` | ✓ |
| Explicit cast `(type)expr`        | ✓ |
| String literals                   | ✓ (placed in `_DATA` area) |
| Adjacent string concatenation     | ✓ |
| Comma operator `,`               | ✓ |
| `_Generic`                        | not yet |

### Statements

| Feature                           | Status |
|-----------------------------------|--------|
| `if` / `else`                     | ✓ |
| `while`                           | ✓ |
| `do`…`while`                     | ✓ |
| `for`                             | ✓ |
| `switch` / `case` / `default`    | ✓ (chain-of-if-else dispatch) |
| `break`                           | ✓ |
| `continue`                        | ✓ |
| `goto` / labels                   | ✓ |
| `return`                          | ✓ |

### Functions

| Feature                           | Status |
|-----------------------------------|--------|
| Function declarations (prototype) | ✓ |
| Function definitions              | ✓ |
| Recursive calls                   | ✓ |
| Variadic `...`                    | ✓ stack-only ABI; `va_list` via `lib/libc/include/stdarg.h` |
| Inline `__asm__`                  | ✓ GNU-style `__asm__("...")` passthrough |

### Preprocessor

xcc has a built-in preprocessor.  Source files are preprocessed automatically before lexing; there is no need to run an external `cpp`.

---

## ABI and calling convention

`xcc` uses the modern SDCC-style `sdcccall(1)` register-based calling
convention by default on Z80.

Variadic functions are the one deliberate exception: any function
declared with `...` is forced to `sdcccall(0)` so every argument is
stack-passed in a stable layout for `va_list` handling.

### Parameter passing

- `sdcccall(1)` uses up to two register-passed arguments before spilling
  to the stack.
- First 8-bit argument: `A`
- First 16-bit argument: `HL`
- First 32-bit argument: `DEHL`
- Second 8-bit argument after an 8-bit first argument: `L`
- Second 16-bit argument after an 8-bit or 16-bit first argument: `DE`
- Remaining arguments are pushed **right-to-left** by the caller.
- Stack-passed `char` and `_Bool` arguments occupy one pushed byte.
- Stack cleanup follows the callee ABI rules already encoded in the IR:
  register-passed arguments need no cleanup, and stack-passed arguments
  are dropped according to the selected convention.

### Return values

| Return type size | Register(s) |
|-----------------|-------------|
| 1 byte (`char`, `bool`) | **A** |
| 2 bytes (`int`, `short`, any pointer) | **DE** |
| 4 bytes (`long`, `float`) | **DEHL** — DE = high word, HL = low word |
| `void` | none |

### Caller-saved vs callee-saved registers

| Registers | Ownership |
|-----------|-----------|
| AF, BC, DE, HL, IY | **Caller-saved** — may be trashed by any function call |
| IX | **Callee-saved** — every function preserves IX (it is the frame pointer) |

---

## Stack frame layout

For functions that actually need an IX frame, stack-passed arguments are
laid out relative to IX like this immediately after the prologue
(`push ix; ld ix,#0; add ix,sp`):

```
  Higher addresses
  ┌─────────────────┐
  │  ...            │
  │  arg2 (4 bytes if long, else 2) │  ← IX + 8 (or higher)
  │  arg1           │  ← IX + 6
  │  arg0           │  ← IX + 4  (first / leftmost argument)
  │  return address │  ← IX + 2
  │  saved old IX   │  ← IX + 0
  │─────────────────│  ← IX points here
  │  local0         │  ← IX - 2  (first declared local)
  │  local1         │  ← IX - 4
  │  ...            │
  │  temporaries    │  (allocated dynamically below locals)
  └─────────────────┘
  Lower addresses (stack grows down)
```

- The compiler subtracts the total local-variable bytes from SP in the prologue:
  `ld hl, #-N; add hl, sp; ld sp, hl`
- The epilogue restores SP from IX before popping: `ld sp, ix; pop ix; ret`
- Temporaries are spilled to the stack immediately below the locals.

---

## Calling xcc functions from assembly

Given this C function:

```c
/* math.c */
int add(int a, int b) {
    return a + b;
}
```

Compiled with `xcc -S math.c -o math.s`, it produces something like:

```asm
    .globl _add
_add:
    push    ix
    ld      ix, #0
    add     ix, sp
    ld      hl, 4 (ix)      ; a  (first arg = lowest IX-relative address)
    ld      de, 6 (ix)      ; b
    add     hl, de
    jp      _add_end
_add_end:
    ld      sp, ix
    pop     ix
    ret
```

To call `add(3, 5)` from hand-written Z80 assembly:

```asm
    ; Push arguments right-to-left (last arg first)
    ld      hl, #5          ; second argument
    push    hl
    ld      hl, #3          ; first argument
    push    hl
    call    _add            ; result in HL after return
    pop     de              ; clean first arg
    pop     de              ; clean second arg
    ; HL = 8
```

### Rules summary

1. Push arguments **right-to-left**: `push arg_N; ...; push arg_1; call _func`
2. **Caller** pops the arguments after the call returns (`pop` as many words as pushed).
3. Return value is in **HL** (16-bit), **L** (8-bit), or **DEHL** (32-bit).
4. Assume AF, BC, DE, HL, IY are **trashed** by the call.  IX is preserved.
5. Symbol names: C name `foo` becomes assembly label `_foo` (underscore prefix).

### Passing a pointer from assembly

```asm
    ld      hl, #_my_buffer   ; address of a buffer
    push    hl
    call    _process          ; void process(char *buf)
    pop     de
```

### Passing a 32-bit long from assembly

Push low word first, then high word (so low word sits at IX+4, high at IX+6):

```asm
    ld      hl, #0x0000     ; high word of 0x00001234
    push    hl
    ld      hl, #0x1234     ; low word
    push    hl
    call    _take_long      ; void take_long(long x)
    pop     de
    pop     de
```

---

## Calling assembly functions from C

Declare the assembly function as `extern` in C:

```c
/* In your C file */
extern int asm_mul(int a, int b);

int result = asm_mul(6, 7);   /* result == 42 */
```

Write the assembly function obeying the xcc Z80 ABI:

```asm
; asm_helpers.s
    .module asm_helpers
    .area _CODE

    .globl _asm_mul
_asm_mul:
    push    ix
    ld      ix, #0
    add     ix, sp
    ; a = 4(ix), b = 6(ix)
    ld      l, 4 (ix)
    ld      h, 5 (ix)       ; HL = a
    ld      e, 6 (ix)
    ld      d, 7 (ix)       ; DE = b  (not used in this simple example)
    ; ... perform operation, leave result in HL ...
    pop     ix
    ret
```

Assemble and link:

```bash
xcc -S myprog.c -o myprog.s
sdasz80 -o myprog.rel myprog.s
sdasz80 -o asm_helpers.rel asm_helpers.s
sdldz80 -i myprog.ihx /usr/local/lib/xcc/crt0.rel myprog.rel \
        asm_helpers.rel /usr/local/lib/xcc/runtime/*.rel
```

### Returning an 8-bit value from assembly to C

Put the value in **L** only:

```asm
_get_char:
    ld      l, #0x41        ; return 'A'
    ret
```

### Returning a 32-bit (long/float) value from assembly to C

Put the low 16 bits in **HL** and the high 16 bits in **DE**:

```asm
_get_long:
    ld      hl, #0x5678     ; low word
    ld      de, #0x1234     ; high word  → value is 0x12345678
    ret
```

---

## Runtime library

Link the needed runtime `.rel` modules from `/usr/local/lib/xcc/runtime/`
with every xcc project.

The helpers listed below are called automatically by the compiler.  They are
**not** called with the normal xcc ABI — arguments are passed via push-before-call
and results in HL (or DE:HL for 32-bit).

| Symbol      | Purpose                                      | Status |
|-------------|----------------------------------------------|--------|
| `__mul16`   | 16-bit signed multiply (HL × DE → HL)        | working |
| `__div16`   | 16-bit signed divide (HL ÷ DE → HL)          | working |
| `__mod16`   | 16-bit signed modulo (HL % DE → DE)          | working |
| `__mul32`   | 32-bit multiply (DE:HL × DE:HL → DE:HL)      | working |
| `__div32`   | 32-bit divide                                | working |
| `__mod32`   | 32-bit modulo                                | working |
| `__call_hl` | Indirect call trampoline (`jp (hl)`)          | working |
| `__fsadd`   | Soft-float add                               | working |
| `__fssub`   | Soft-float subtract                          | working |
| `__fsmul`   | Soft-float multiply                          | working |
| `__fsdiv`   | Soft-float divide                            | working |
| `__fitosf`  | Integer to soft-float conversion             | working |
| `__fstoi`   | Soft-float to integer conversion             | working |
| `__mulll`   | 64-bit multiply                              | working |
| `__divll`   | 64-bit divide                                | working |
| `__modll`   | 64-bit modulo                                | working |
| `__atomic_*`| Atomic load/store/exchange/CAS (DI/EI)       | **stub** |

`float`, `double`, and `long long` arithmetic are now lowered through the
runtime helpers above. Future runtime work should focus on coverage and
performance, not basic bring-up.

---

## Linking

### Minimal link command

```bash
sdldz80 -i output.ihx crt0.rel main.rel runtime/*.rel
```

- `-i` produces Intel HEX output.
- `crt0.rel` provides the hardware entry point and calls `_main`.
- The runtime directory provides helpers such as `__mul16`,
  `__div16`, and `__mod16`.
- Order matters: `crt0.rel` must come first so the linker places it at the base address.

### Setting the load address

By default sdldz80 links to address 0x0000.  To change it:

```bash
sdldz80 -i output.ihx -b _CODE=0x0100 crt0.rel main.rel runtime/*.rel
```

### Producing a raw binary

After linking:

```bash
objcopy --input-target=ihex --output-target=binary output.ihx output.bin
# or
hex2bin output.ihx         # if hex2bin is installed
```

### Mixing xcc and hand-written assembly modules

xcc-compiled `.rel` files use the same sdasz80 object format and can be linked freely with hand-written assembly modules assembled with sdasz80.
