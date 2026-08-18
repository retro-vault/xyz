# xcc Runtime Library Test Plan

## Purpose

The runtime library (`x/runtime/`) contains hand-optimised Z80
assembly routines that the SDCC compiler emits calls to for operations that
have no single Z80 instruction: integer arithmetic grouped under `int8/`,
`int16/`, `int32/`, and `int64/`, floating-point helpers under `float/`,
atomic helpers under `atomic/`, jump shims under `jumps/`, and shared/system
support under `common/` and `sys/`.

Before optimising any of these routines we need a reliable test harness.
The suite in `x/tests/tests/runtime/` assembles the real runtime `.s` files, links
them at address 0x0000, and exercises each entry point through the xz80
Z80 CPU emulator.  No CP/M faking, no patching — the actual machine code
runs and register state is inspected.

---

## How it works

```
x/tests/tests/runtime/
├── Makefile              — orchestrates the full build+test flow
├── tools/
│   ├── ihx2bin.py        — Intel HEX → flat binary
│   └── gen_symbols.py    — sdldz80 .noi → C++ constexpr address header
├── runtime_machine.hpp   — Z80 harness (xz80 wrapper + call helpers)
├── float_helpers.hpp     — shared feq() tolerance comparison
├── test_main.cpp         — framework main + #include of all test files
├── test_int16.cpp        — 16-bit integer tests
├── test_shifts.cpp       — variable-count shift tests
├── test_int32.cpp        — 32-bit integer tests
├── test_float_arith.cpp  — float arithmetic + comparison tests
└── test_float_conv.cpp   — int⟷float conversion tests
```

### Build flow

1. `xas` assembles every `.s` under `x/runtime/`
   recursively to a `.rel`.
2. `sdldz80` links all `.rel` files at `_CODE=0x0000`, producing
   the direct runtime image and its symbol map under `build/tests/runtime/`.
3. `tools/ihx2bin.py` converts the IHX to a flat `runtime.bin`.
4. `tools/gen_symbols.py` parses the `.noi` file (which contains full
   symbol names, unlike the truncated `.map`) and writes
   `build/tests/runtime/runtime_symbols.hpp` with `rt_sym::symbol_name`
   constants.
5. The C++ test binary is compiled against the xz80 sources, loading
   `runtime.bin` at startup.

### Calling convention (sdcccall(1), Z80)

| Type       | Arg 1       | Arg 2           | Return          |
|------------|-------------|-----------------|-----------------|
| uint8_t    | A register  | L register      | A (or DE low)   |
| uint16_t   | HL          | DE              | HL or DE        |
| int16_t    | HL (s.e.)   | DE (s.e.)       | DE (mul/div) or HL (mod/shift) |
| uint32_t   | DE=lo,HL=hi | stack (4 bytes) | DE=lo, HL=hi    |
| float      | HL=hi,DE=lo | stack (4 bytes) | HL=hi, DE=lo    |

Stack argument layout (32-bit, float): push high-word (b3,b2) first, then
low-word (b1,b0).  After `push ix; ld ix,#0; add ix,sp` the function sees
`ix+4=b0`, `ix+5=b1`, `ix+6=b2`, `ix+7=b3`.

---

## What is tested (210 tests)

### 16-bit integer (`test_int16.cpp`)

| Entry point          | Tested operations                                    |
|----------------------|------------------------------------------------------|
| `__mul16`/`__mulint` | zero, identity, 255×255, 16-bit wrap, signed inputs  |
| `__div16`/`__divuint`| exact, remainder, large, divisor > dividend          |
| `__sdiv16`/`__divsint`| ±/±, remainder (unsigned HL), divide-by-1          |
| `__mod16`/`__moduint`| exact, non-exact, large, zero-dividend               |
| `__smod16`           | ±/± C-semantics, exact, dividend-smaller             |
| `__mulschar`         | pos×pos, neg×pos, neg×neg, max values                |
| `__divschar`         | pos, neg-dividend                                    |
| `__divuschar`        | basic                                                |
| `__modschar`         | basic                                                |
| `__moduschar`        | basic                                                |

Note: `__divsint` leaves HL = unsigned remainder from `__divuint`.  The
signed remainder requires a subsequent call to `__get_remainder`, which is
what `__modsint` does.

### 16-bit shifts (`test_shifts.cpp`)

| Entry point | Tested                                              |
|-------------|-----------------------------------------------------|
| `__shl16`   | count=0, 1, 4, 8, 15, 16 (shift-out)               |
| `__shr16s`  | count=0, positive value, negative (arithmetic), min |
| `__shr16u`  | count=0, MSB (no sign extend), all-ones             |

### 32-bit integer (`test_int32.cpp`)

| Entry point                  | Tested                                  |
|------------------------------|-----------------------------------------|
| `__mul32`/`__mullong`        | basic, large, ±, overflow wraps         |
| `__div32`/`__divulong`       | basic, exact, large, power-of-2         |
| `__sdiv32`/`__divslong`      | ±/±, large, INT32_MIN/-1 (no hang)      |
| `__mod32`/`__modulong`       | basic, exact, large, power-of-2         |
| `__smod32`/`__modslong`      | ±/± C-semantics, exact                  |
| `___mulsint2slong`           | basic, neg×pos, max int16 square        |
| `___muluint2ulong`           | basic, max uint16 square, zero          |

### Float arithmetic (`test_float_arith.cpp`)

| Entry point          | Tested                                               |
|----------------------|------------------------------------------------------|
| `__fsadd`            | basic, identity zero, fractions, cancellation, large |
| `__fssub`            | basic, self=0, negative result, zero−n               |
| `__fsmul`            | basic, by-zero, ±, fractions, identity, large        |
| `__fsdiv`            | basic, ±, fractions, identity, large                 |
| `___fscmp`           | <, >, =, neg<pos, 0<pos, neg vs neg                  |
| `___fseq`            | equal, not-equal, both-zero, sign-differ             |
| `___fslt`            | less, greater, equal, neg<zero                       |
| `__fsneg`            | ±1.0, large, sign flip                               |
| `__fssqrt`           | stub — always returns 0.0f                           |
| `__fsatan2`          | stub — always returns 0.0f                           |

`__fsneg` is unique: entire float on stack, result in DE=high, HL=low
(opposite of all other float results).

### Float conversions (`test_float_conv.cpp`)

| Entry point         | Tested                                               |
|---------------------|------------------------------------------------------|
| `___uint2fs`        | 0, 1, 100, 255, 1000, 32768, 65535                   |
| `___sint2fs`        | 0, 1, -1, -100, 32767, -32768                        |
| `___uchar2fs`       | 0, 100, 255                                          |
| `___schar2fs`       | 0, 127, -1, -128                                     |
| `___ulong2fs`       | 0, 1, 1e6, 65535, 0x7FFFFFFF                         |
| `___slong2fs`       | 0, 100000, -100000, INT32_MIN                        |
| `___fs2sint`        | 1, -1, trunc, 0, clamp ±32767/-32768, frac<1         |
| `___fs2uint`        | 1, neg→0, truncate, clamp 65535, zero                |
| `___fs2schar`       | 1, -50, exact -128, exact 127                        |
| `___fs2uchar`       | 100, neg→0, exact 255                                |
| `___fs2slong`       | 1, -1, 1e6, trunc, clamp max/min                     |
| `___fs2ulong`       | 1, neg→0, 4e9, clamp 0xFFFFFFFF                      |
| `__fstoi`           | stub — returns 0                                     |
| `__fitosf`          | stub — returns 0.0f                                  |

Note: `___fs2schar` and `___fs2uchar` return the *low byte* of the int16
result — there is no saturation at int8 bounds.  `___fs2schar(-128.0f) = -128`
because -128 happens to fit in one byte.

---

## Running the tests

```bash
make -C x/tests/tests/runtime test
make -C x/tests/tests/runtime clean
```

The binary path can be overridden: `./build/tests/runtime/runtime_tests path/to/runtime.bin`.

---

## Bug found and fixed during testing

**`__moduint` returned quotient in HL, not remainder.**

`x/runtime/int16/modunsigned.s` had a spurious `ex de,hl` after
`call __divuint`.  `__divuint` already returns the remainder in HL and
quotient in DE; the swap reversed them.  All other unsigned-modulo callers
(`__smod16`, `__modsint` via `__get_remainder`) expect and receive HL =
remainder without a swap.  The `ex de,hl` was removed.

---

## Optimisation guidance

The following routines are the most performance-critical and are the primary
targets for future optimisation passes:

1. `__mullong` (32-bit multiply): shift-add with 32 iterations.
2. `__divulong`/`__divslong` (32-bit divide): restoring division, 32 iterations.
3. `__fsadd`/`__fssub`: large IX stack frame, many local accesses.
4. `__fsmul`: 6-byte × 6-byte product accumulation, very register-intensive.
5. `__fsdiv`: 24-bit mantissa division loop.

Run `make test` before and after any change.  All 210 tests must pass.
