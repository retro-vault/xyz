# xcc — X C Compiler for Z80

xcc is a clean-room C11 compiler targeting the Z80 processor, built from scratch
in modern C++17, with a hand-written recursive descent parser, a typed three-address IR, and GCC-compatible command-line flags.

CLI note:

- The authoritative end-user switch summary is `x/docs/dist/man/XCC.md`
  together with `xcc --help`.
- This document remains useful for architecture and ABI context, but the
  packaged manpage is the current CLI contract.

---

## Architecture

```
Source (.c)
    │
    ▼
┌─────────────────────────────────────────────────┐
│  Frontend                                        │
│                                                  │
│  Lexer          hand-written, no flex/bison      │
│   → tokens                                       │
│  Parser         recursive descent, C11           │
│   → AST (typed node hierarchy)                   │
│  Sema           type resolution, symbol table    │
└──────────────────────────────┬──────────────────┘
                               │ AST
                               ▼
┌─────────────────────────────────────────────────┐
│  IR (three-address code)                         │
│                                                  │
│  IrGen          AST → ICode stream               │
│  IRModOpt       TU-local static/helper cleanup   │
│  IRModule       list of IRFunctions              │
│  ICode ops:     ASSIGN, ADD, SUB, MUL, CALL,    │
│                 IFX, GOTO, LABEL, SEND, RECEIVE  │
└──────────────────────────────┬──────────────────┘
                               │ IR
                               ▼
┌─────────────────────────────────────────────────┐
│  Z80 Backend                                     │
│                                                  │
│  Z80Gen         IR → GNU-as assembly             │
│  Z80Peep        peephole optimizer               │
└──────────────────────────────┬──────────────────┘
                               │ .s
                               ▼
                     z80-elf-as / sdas
                               │ .o
                               ▼
                     z80-elf-ld / sdld
```

---

## ABI: xcc Z80 calling convention

| Item               | Convention                            |
|--------------------|---------------------------------------|
| Parameters         | Right-to-left push on stack           |
| Return (8-bit)     | L register                            |
| Return (16-bit)    | HL register                           |
| Return (32-bit)    | DEHL (DE = high, HL = low)            |
| Frame pointer      | IX                                    |
| First param (stack)| IX+4                                  |
| Second param       | IX+4+sizeof(param0)                   |
| First local        | IX-2                                  |

### Stack frame layout

```
[high addresses]
  ... caller frame ...
  paramN           (last pushed, highest addr)
  ...
  param1           (first pushed)
  return_addr      [IX+2]
  saved_IX         [IX+0]    ← IX points here
  local_1          [IX-2]
  local_2          [IX-4]
  temp_1           [IX-6]    ← dynamically allocated
  ...              ← SP
[low addresses]
```

---

## Type sizes (Z80 target)

| Type               | Size  |
|--------------------|-------|
| `char`             | 1     |
| `short`, `int`     | 2     |
| `long`             | 4     |
| `long long`        | 8     |
| `float`            | selected by `--float-format` (4 by default) |
| `double`           | same as `float` in M; 8 in L |
| pointer            | 2     |

The M distribution aliases `double` and `long double` to `float`. The L
distribution provides a distinct 8-byte software IEEE-754 double type.

---

## Command line (GCC-compatible)

```
xcc [options] <input>... [-o output]

  -o <file>                     Output filename
  -c                            Compile and assemble only, emit .rel
  -S                            Compile only, emit assembly
  --c1mode, -c1-mode           Read preprocessed C from stdin, emit assembly
  -O0/-O1/-O2/-Of/-O3/-Os       Optimization level
  --opt-code-size              Alias for -Os
  --opt-code-speed             Alias for -Of
  -f<name>, -fno-<name>         Fine-grained optimization family control
  -w, -W0..-W3, -Wall, -Wextra,
  -Wpedantic, -Werror[=name],
  -Wno-error[=name]             Driver warning controls
  -mz80                         Accepted for SDCC/z88dk compatibility
  --runtime=<name>              Runtime: x (default) or z88dk-classic
  -zcc-opt=<file>               Append inferred z88dk capabilities (zcc internal)
  -I<dir>                       Add include directory
  -D<macro>[=val]               Define macro
  --nostdinc                    Do not add default target include path
  -std=c11                      Language standard (other -std= forms are tolerated)
  -masm <dialect>, -masm=<dialect>
                                Assembler dialect: sdasz80 or gnuas
  --mode=sdcc, --mode=gnu       Assembler-output dialect aliases
  --platform <name>, --platform=<name>
                                Select target platform include defaults
  --float-format <fmt>, --float-format=<fmt>
                                Select the C float ABI
  --sdcccall <0|1>              Select the default calling convention
  --dump-ir                     Dump lowered IR to stderr
  -L<dir>, -l<name>, -B <prefix>,
  -nostdlib, -nostartfiles,
  --no-default-runtime,
  --oformat=<fmt>, -T*, --script=<file>,
  --section-start=<name>=<addr>,
  --binary-range=<lo>-<hi>, --reserve=<lo>-<hi>,
  -e <sym>, -Map=<file>, -M,
  -Wl,<args>                    Forwarded to xld
  -g                            Emit debug info
  -v                            Verbose
  --version                     Print version
  -h, --help                    Help
```

Unless `--nostdinc` is used, the driver searches the selected target's private
headers in `<prefix>/z80/include/<platform>` before the common libc headers in
`<prefix>/z80/include`. For example, `<sys/bdos.h>` is supplied only by
`--platform=cpm3`.

---

## IR opcodes (src/ir/icode.h)

xcc IR opcodes:

| Op             | Meaning                          |
|----------------|----------------------------------|
| `LABEL`        | label:                           |
| `GOTO`         | unconditional jump               |
| `IFX`          | conditional branch               |
| `FUNCTION`     | function prologue marker         |
| `ENDFUNCTION`  | function epilogue marker         |
| `RETURN`       | return [value]                   |
| `SEND`         | push argument (param passing)    |
| `RECEIVE`      | receive parameter into local     |
| `CALL`         | call function                    |
| `ASSIGN`       | result = left                    |
| `ADDRESS_OF`   | result = &left                   |
| `GET_VALUE_AT` | result = *left (load)            |
| `SET_VALUE_AT` | *result = left (store)           |
| `ADD/SUB/MUL`  | arithmetic                       |
| `DIV/MOD`      | division (calls runtime helpers) |
| `NEG`          | unary minus                      |
| `BAND/BOR/BXOR`| bitwise                          |
| `BNOT`         | bitwise complement               |
| `SHL/SHR`      | shifts                           |
| `EQ/NE/LT/LE/GT/GE` | comparisons (→ 0 or 1)    |
| `CAST`         | type conversion                  |

---

## Runtime library (lib/runtime.s)

| Symbol     | Purpose                    |
|------------|----------------------------|
| `__mul16`  | 16-bit unsigned multiply   |
| `__div16`  | 16-bit unsigned divide     |
| `__mod16`  | 16-bit unsigned modulo     |

---

## Building

```bash
make              # debug build
make BUILD=release
make test
```

Requirements: `g++` ≥ 7 with C++17 support.

---

## Adding a new codegen feature

1. Add IR opcode to `src/ir/icode.h` if needed
2. Add IrGen visitor in `src/ir/irgen.cpp`
3. Add Z80Gen handler in `src/backend/z80/z80gen.cpp`
4. Add a test in `tests/data/tNNN_feature.c`
5. Run `make test`

---

## Limitations (initial version)

- Built-in preprocessor in normal mode; use `--c1mode` for externally
  preprocessed stdin
- Struct/union member access code-gen is minimal
- 32-bit arithmetic uses runtime helpers (not inline)
- No register allocation (all temporaries spill to stack)
- No link-time optimization
- `switch` compiles as if/else chain (no jump tables yet)
