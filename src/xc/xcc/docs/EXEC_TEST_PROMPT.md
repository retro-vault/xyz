## Executable Z80 Test Prompt

Use this prompt when adding more executable regression tests that must
work exactly like the existing `tests/data/exec` suite.

### Directory Layout

- `tests/data/exec/include/xcc_exec_test.h`
  Shared assertion macros for executable tests.
- `tests/data/exec/int/*.c`
  16-bit integer execution tests.
- `tests/data/exec/long/*.c`
  32-bit integer execution tests.
- `tests/data/exec/float/*.c`
  Soft-float execution tests.
- `tests/data/exec/runtime/*.c`
  Runtime and calling-convention tests.
- `tests/tools/z80emu/z80.h`
  Header-only Z80 emulator core.
- `tests/tools/z80emu/z80_exec.cpp`
  Runner that loads `.ihx` or `.bin`, executes, and reads the mailbox.
- `tests/tools/z80emu/crt0_sdasz80.s`
  SDCC/ASxxxx startup for executable tests.
- `tests/tools/z80emu/crt0_gnuas.s`
  GNU binutils startup for executable tests.
- `tests/tools/z80emu/z80_exec.ld`
  GNU linker script for executable tests.
- `tests/run_exec_tests.sh`
  Main script that builds, links, runs, and checks all executable tests.
- `lib/runtime/*.s`
  Merged xcc ABI bridge modules and imported runtime cores.
- `build/exec/...`
  Per-test generated assembly, objects, images, and debug artifacts.

Rule:

- Put new C tests only under `tests/data/exec/<suite>/`.
- Do not put helper tools under `tests/data`.
- Keep new runtime/helper assembly outside `tests/data`, usually under
  `lib/runtime/`.

### Current Passing Test Set

- `tests/data/exec/int/t001_add_sub.c`
- `tests/data/exec/int/t002_mul16_basic.c`
- `tests/data/exec/int/t003_mul16_signed.c`
- `tests/data/exec/int/t004_div16_unsigned.c`
- `tests/data/exec/int/t005_mod16_unsigned.c`
- `tests/data/exec/int/t006_shift_compare.c`
- `tests/data/exec/long/t007_long_add_sub.c`
- `tests/data/exec/long/t008_mul32_basic.c`
- `tests/data/exec/long/t009_mul32_signed.c`
- `tests/data/exec/long/t010_div32_unsigned.c`
- `tests/data/exec/long/t011_mod32_unsigned.c`
- `tests/data/exec/long/t012_long_compare.c`
- `tests/data/exec/float/t013_fsadd.c`
- `tests/data/exec/float/t014_fssub.c`
- `tests/data/exec/float/t015_fsmul.c`
- `tests/data/exec/float/t016_fsdiv.c`
- `tests/data/exec/float/t017_float_chain.c`
- `tests/data/exec/runtime/t018_funcptr_call.c`
- `tests/data/exec/runtime/t019_recursion_mul.c`
- `tests/data/exec/runtime/t020_mixed_helpers.c`

### How The Harness Works

Each test is a self-checking C program:

- `main()` returns `0` on success.
- `main()` returns a small explicit failure code on failure.

The test runner builds each program twice:

1. `xcc -> sdasz80 -> sdldz80 -> .ihx -> z80_exec`
2. `xcc -masm=gnuas -> z80-unknown-elf-as/ld/objcopy -> .bin -> z80_exec`

The startup stub writes:

- return value to `0xff00`
- completion byte `0xa5` to `0xff02`

The emulator watches `0xff02`, stops when it sees `0xa5`, and prints:

- `done=<0|1>`
- `return=<u16>`
- `cycles=<count>`
- `pc=<final pc>`

### Normal Commands

Build the compiler:

```bash
make -j4
```

Run the executable suite:

```bash
bash tests/run_exec_tests.sh ./build/bin/xcc
```

Expected success:

```text
Results: 40 passed, 0 failed
```

### Per-Test Debug Loop

When a new test fails, use this loop:

1. Run the full suite once.
2. Inspect the generated artifacts under:
   - `build/exec/sdasz80/<suite>/<test>/`
   - `build/exec/gnuas/<suite>/<test>/`
3. Read the generated `.s` file first.
4. If assembly fails, fix the generated code or test pattern before
   looking at the emulator.
5. If assembly and link succeed, run the produced image and inspect the
   return code.
6. If needed, create a tiny one-off probe program in `/tmp` and compile
   it through the same flow to isolate one helper or one codegen path.

Most useful files while debugging one test:

- generated source:
  - `build/exec/.../<test>.s`
- SDCC objects and image:
  - `build/exec/.../<test>.rel`
  - `build/exec/.../<test>.ihx`
- GNU objects and image:
  - `build/exec/.../<test>.o`
  - `build/exec/.../<test>.elf`
  - `build/exec/.../<test>.bin`

### Rules For Writing New Tests

- Use the macros from `xcc_exec_test.h`.
- Use explicit failure IDs like `XCC_CHECK_EQ_INT_ID(3, ...)`.
- For 32-bit results, use `XCC_CHECK_EQ_U32_ID(code, actual, lo, hi)`.
- Keep tests small and single-purpose.
- Prefer values with easy-to-check expected words.
- For float tests, call the soft-float helpers directly with raw
  IEEE-754 bit patterns:
  - `__fsadd`
  - `__fssub`
  - `__fsmul`
  - `__fsdiv`
- For signed 32-bit tests, prefer explicit two's-complement `long`
  variables when negative literals are risky.
- Avoid relying on unsupported front-end behavior when the goal is to
  validate runtime helpers.

### Known Good Patterns

16-bit helper calls:

```c
extern unsigned int __mul16(unsigned int a, unsigned int b);
XCC_CHECK_EQ_UINT_ID(1, __mul16(37u, 11u), 407u);
```

32-bit result checks:

```c
extern unsigned long __mul32(unsigned long a, unsigned long b);
XCC_CHECK_EQ_U32_ID(1, __mul32(70000ul, 3ul), 0x3450u, 0x0003u);
```

Float helper checks:

```c
extern unsigned long __fsadd(unsigned long a, unsigned long b);
XCC_CHECK_EQ_U32_ID(1, __fsadd(0x3fc00000ul, 0x40100000ul),
                    0x0000u, 0x4070u);
```

Function-pointer runtime checks:

```c
static int add_one(int x) { return x + 1; }
static int call_it(int (*fn)(int), int value) { return fn(value); }
XCC_CHECK_EQ_INT_ID(1, call_it(add_one, 41), 42);
```

### Bugs Already Solved In This Harness

These were important to keep the current suite working:

- Direct and helper calls must declare external helper symbols for
  `sdasz80` with emitted `.globl` declarations.
- Call stack cleanup must not clobber `HL` or `DE` return registers.
- Taking the address of a stack local cannot use `add hl, ix`.
- 4-byte temp allocation must not clobber live return registers.
- Bridge wrappers that push words through `BC` must load
  `C=low-byte, B=high-byte`.
- Function designators used as values must load the function address,
  not dereference the function label as data.

### Copyable Prompt

```text
Add one or more new executable xcc regression tests using the existing
Z80 emulator harness.

Requirements:
- Put new C files only in tests/data/exec/<suite>/.
- Do not place tools or helper programs in tests/data.
- Reuse tests/data/exec/include/xcc_exec_test.h.
- Make each test self-checking: return 0 on pass, small explicit code on fail.
- Build and verify each test through both flows:
  1. xcc -> sdasz80 -> sdldz80 -> .ihx -> z80_exec
  2. xcc -masm=gnuas -> z80-unknown-elf-as/ld/objcopy -> .bin -> z80_exec
- Keep compatibility with tests/run_exec_tests.sh.
- If the test uses runtime helpers, make sure the emitted assembly
  still triggers the existing auto-module resolution.
- For 32-bit results, compare low/high 16-bit words with
  XCC_CHECK_EQ_U32_ID instead of plain long equality when appropriate.
- For float tests, prefer direct soft-float helper calls with raw
  IEEE-754 bit patterns.
- After adding the test, run:
  bash tests/run_exec_tests.sh ./build/bin/xcc
- Debug failures using build/exec/<toolchain>/<suite>/<test>/ artifacts
  until both toolchains pass.
```
