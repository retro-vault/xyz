# Big Numbers: long long and double

This document describes everything needed to bring 64-bit integer (`long long`)
and IEEE-754 double-precision (`double`) arithmetic to the xcc runtime on Z80.

It covers the calling convention, the complete function inventory, what
assembly to write, how to slot each function into the build, and how to
activate the pre-written test suite once the code is ready.

---

## Why the Z80 can carry 64 bits in registers

The Z80 exposes two banks of general-purpose registers:

```
Main bank:       BC   DE   HL   (+ AF)
Alternate bank:  BC'  DE'  HL'  (+ AF')
Index:           IX   IY
```

`EXX` swaps the three main pairs with their alternates in one instruction.
Together the main and alternate DE and HL pairs provide exactly 64 bits
of register space — enough to pass the first argument and return a result
without touching the stack at all.

```
Proposed 64-bit register layout
────────────────────────────────
Register  │  Bits  │  Notes
──────────┼────────┼──────────────────────────────────
   DE     │ 15: 0  │ lsb word, main bank
   HL     │ 31:16  │ main bank
   DE'    │ 47:32  │ alternate DE  (cpu_state::de2)
   HL'    │ 63:48  │ alternate HL  (cpu_state::hl2)
```

This is identical for both `long long` (two's-complement integer) and
`double` (IEEE-754 64-bit float) — the register layout is bit-for-bit
the same regardless of type.

---

## Calling convention (proposed)

### One-arg and two-arg 64-bit functions

```
Argument 1 / return value:  DE : HL : DE' : HL'  (lsb → msb)

Argument 2 (if present):    8 bytes on stack.

After function's  push ix / ld ix,#0 / add ix,sp:
  ix+ 4  = arg2 byte 0  (lsb)
  ix+ 5  = arg2 byte 1
  ix+ 6  = arg2 byte 2
  ix+ 7  = arg2 byte 3
  ix+ 8  = arg2 byte 4
  ix+ 9  = arg2 byte 5
  ix+10  = arg2 byte 6
  ix+11  = arg2 byte 7  (msb)
```

Caller push order for arg2 (consistent with the existing 32-bit push pattern):

```asm
; Push arg2 = {b7..b0}
ld  hl, (b7<<8)|b6 : push hl   ; high32 high-word first
ld  hl, (b5<<8)|b4 : push hl   ; high32 low-word
ld  hl, (b3<<8)|b2 : push hl   ; low32  high-word
ld  hl, (b1<<8)|b0 : push hl   ; low32  low-word  (closest to return addr)
call  __funcname
```

### Small-type arguments (conversions *to* 64-bit)

| Source type | Where it arrives | Follows existing… |
|-------------|-----------------|-------------------|
| `int16_t`   | HL              | `___sint2fs` ABI  |
| `uint16_t`  | HL              | `___uint2fs` ABI  |
| `int32_t`   | DE=lo16, HL=hi16| `___slong2fs` ABI |
| `uint32_t`  | DE=lo16, HL=hi16| `___ulong2fs` ABI |
| `float`     | HL=hi16, DE=lo16| float32 ABI       |

All of these return 64 bits in DE:HL:DE':HL'.

### Small-type results (conversions *from* 64-bit)

| Target type | Where it lands | Follows existing… |
|-------------|---------------|-------------------|
| `int16_t`   | DE            | `___fs2sint` ABI  |
| `uint16_t`  | DE            | `___fs2uint` ABI  |
| `int32_t`   | DE=lo16, HL=hi16 | 32-bit return |
| `uint32_t`  | DE=lo16, HL=hi16 | 32-bit return |
| `float`     | HL=hi16, DE=lo16 | float32 return |

For all of these, the 64-bit source arrives in DE:HL:DE':HL' (same as arg1).

### Comparison results (same as the existing float convention)

```
___dbcmp   →  DE = -1, 0, or +1  (same as ___fscmp)
___dbeq    →  A  = 0 or 1        (same as ___fseq)
___dblt    →  A  = 0 or 1        (same as ___fslt)
```

### Register preservation

The function may freely clobber DE', HL' (that is how it returns the high32
result).  BC' is free scratch.  The caller must save any alternate registers it
wishes to keep.  Main AF, BC are caller-saved in the existing convention and
remain so here.

---

## Assembly skeleton for a 64-bit function

```asm
        ; __examplell
        ; inputs:  a in DE:HL:DE':HL' (DE=lo16 … HL'=hi16)
        ;          b at ix+4..ix+11   (lsb..msb, stack)
        ; outputs: DE:HL:DE':HL' = result
        ; clobbers: af, bc, de, hl, ix, de', hl'

        .module examplell
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __examplell

__examplell:
        push    ix
        ld      ix, #0
        add     ix, sp

        ; Read arg1 high32 from alternates into local vars
        exx
        ; DE is now arg1_bits47:32, HL is arg1_bits63:48
        ; ... do work ...
        exx

        ; Restore main DE:HL for low32 of result
        ; Set alternates for high32 of result
        exx
        ld      de, <result_bits47:32>
        ld      hl, <result_bits63:48>
        exx
        ld      de, <result_bits15:0>
        ld      hl, <result_bits31:16>

        ld      sp, ix          ; tear down frame
        pop     ix
        ret
```

---

## Function inventory

### Already in the binary as stubs

These three files exist and export valid symbols, but contain only `ret`.
Replace the body; the linker address in `runtime_symbols.hpp` stays the same.

| File | Symbol | Operation |
|------|--------|-----------|
| `mulll.s`  | `__mulll`  | 64-bit multiply (signed = unsigned for low 64 bits) |
| `divll.s`  | `__divll`  | 64-bit divide — *current stub is unsigned; pick a name* |
| `modll.s`  | `__modll`  | 64-bit modulo — *current stub; same note* |

Recommendation: repurpose `divll`→`__divull` (unsigned) and add a separate
`divsll.s` for signed, matching the 32-bit pattern
(`divulong`/`divslong`).

---

### New files to create (long long integer)

#### Arithmetic

| New file | Symbol | ABI summary |
|----------|--------|-------------|
| `mulll.s`   | `__mulll`   | DE:HL:DE':HL' × (ix+4..11) → DE:HL:DE':HL' |
| `divull.s`  | `__divull`  | unsigned 64-bit divide |
| `divsll.s`  | `__divsll`  | signed 64-bit divide |
| `modull.s`  | `__modull`  | unsigned 64-bit modulo |
| `modsll.s`  | `__modsll`  | signed 64-bit modulo (C11: sign = sign of dividend) |

#### Shifts

| New file | Symbol | ABI summary |
|----------|--------|-------------|
| `shl64.s`  | `__shl64`  | HL = value low32, DE:HL:DE':HL' = value, B = count |
| `shr64u.s` | `__shr64u` | logical right shift |
| `shr64s.s` | `__shr64s` | arithmetic right shift (sign-extends) |

For shifts, the shift count can stay in B (same register as `__shl16`/`__shr16s`/`__shr16u`).

#### Conversions → `long long`

| New file | Symbol | Input | Output |
|----------|--------|-------|--------|
| `sint2ll.s`  | `___sint2ll`  | HL = int16  | sign-extend to DE:HL:DE':HL' |
| `uint2ll.s`  | `___uint2ll`  | HL = uint16 | zero-extend |
| `slong2ll.s` | `___slong2ll` | DE=lo16, HL=hi16 (int32) | sign-extend |
| `ulong2ll.s` | `___ulong2ll` | DE=lo16, HL=hi16 (uint32) | zero-extend |

All four output DE:HL:DE':HL'.

#### Conversions from `long long`

| New file | Symbol | Input | Output |
|----------|--------|-------|--------|
| `ll2sint.s`  | `___ll2sint`  | DE:HL:DE':HL' | DE = low 16 (truncate) |
| `ll2uint.s`  | `___ll2uint`  | DE:HL:DE':HL' | DE = low 16 |
| `ll2slong.s` | `___ll2slong` | DE:HL:DE':HL' | DE=lo16, HL=hi16 (truncate to low 32) |
| `ll2ulong.s` | `___ll2ulong` | DE:HL:DE':HL' | DE=lo16, HL=hi16 |

---

### New files to create (double)

#### Arithmetic

| New file | Symbol | ABI summary |
|----------|--------|-------------|
| `dbadd.s`  | `__dbadd`  | a in regs, b on stack → result in regs |
| `dbsub.s`  | `__dbsub`  | flip b's sign bit, tail-call `__dbadd` (same trick as `__fssub`) |
| `dbmul.s`  | `__dbmul`  | a in regs, b on stack → result in regs |
| `dbdiv.s`  | `__dbdiv`  | a in regs, b on stack → result in regs |
| `dbneg.s`  | `__dbneg`  | a in regs → flip bit 63 of HL' → return in regs |
| `dbsqrt.s` | `__dbsqrt` | stub returning 0.0 (32-byte frame minimum) |

#### Comparisons

| New file | Symbol | ABI summary |
|----------|--------|-------------|
| `dbcmp.s` | `___dbcmp` | a in regs, b on stack → DE = -1/0/+1 |
| `dbeq.s`  | `___dbeq`  | calls `___dbcmp`, A = (DE==0) ? 1 : 0 |
| `dblt.s`  | `___dblt`  | calls `___dbcmp`, A = (DE==-1) ? 1 : 0 |

#### Conversions → `double`

| New file | Symbol | Input | Output |
|----------|--------|-------|--------|
| `sint2db.s`  | `___sint2db`  | HL = int16  | DE:HL:DE':HL' (double) |
| `uint2db.s`  | `___uint2db`  | HL = uint16 | same |
| `slong2db.s` | `___slong2db` | DE=lo16, HL=hi16 | same |
| `ulong2db.s` | `___ulong2db` | DE=lo16, HL=hi16 | same |
| `sll2db.s`   | `___sll2db`   | DE:HL:DE':HL' (ll) | same |
| `ull2db.s`   | `___ull2db`   | DE:HL:DE':HL' (ull) | same |
| `fs2db.s`    | `___fs2db`    | HL=hi16, DE=lo16 (float32) | same |

#### Conversions from `double`

| New file | Symbol | Input | Output |
|----------|--------|-------|--------|
| `db2sint.s`  | `___db2sint`  | DE:HL:DE':HL' | DE = int16 (truncate) |
| `db2uint.s`  | `___db2uint`  | DE:HL:DE':HL' | DE = uint16 |
| `db2slong.s` | `___db2slong` | DE:HL:DE':HL' | DE=lo16, HL=hi16 (int32) |
| `db2ulong.s` | `___db2ulong` | DE:HL:DE':HL' | DE=lo16, HL=hi16 |
| `db2sll.s`   | `___db2sll`   | DE:HL:DE':HL' | DE:HL:DE':HL' (int64, truncate) |
| `db2ull.s`   | `___db2ull`   | DE:HL:DE':HL' | DE:HL:DE':HL' (uint64) |
| `db2fs.s`    | `___db2fs`    | DE:HL:DE':HL' | HL=hi16, DE=lo16 (float32) |

---

## Implementation order

Start with the pieces that have no dependencies on other new code.

**Phase 1 — zero dependencies (implement first)**

1. `___sint2ll`, `___uint2ll`, `___slong2ll`, `___ulong2ll`  
   Simple sign-extend / zero-extend.  Good first Z80 exercise with the
   alternate registers.  Tests: `roundtrip_sint16` and `roundtrip_slong` in
   `test_ll.cpp` immediately verify them.

2. `___ll2sint`, `___ll2uint`, `___ll2slong`, `___ll2ulong`  
   Trivial truncations.  Low 16 bits live in main DE; nothing to shift.

3. `__dbneg`  
   Flip bit 63: `EXX; ld a,h; xor #0x80; ld h,a; EXX`.  One of the
   simplest possible 64-bit functions.

4. `__dbsub`  
   Same one-liner trick as `__fssub`: flip the sign bit of arg2 on the
   stack, tail-call `__dbadd`.

5. `___sint2db`, `___uint2db`, `___slong2db`, `___ulong2db`  
   Widen the value to `long long` (Phase 1), then call `___sll2db` /
   `___ull2db` (Phase 2).  Or implement directly as scaled-exponent
   construction.

**Phase 2 — depend only on Phase 1**

6. `__mulll`  
   64-bit shift-add (extend `__mullong`'s 32-iteration loop to 64 bits).
   Replace the stub.

7. `__dbadd`  
   Port `__fsadd` to 64-bit: 11-bit exponent (bias 1023), 52-bit mantissa.
   Largest single function; build on top of working `__dbneg`.

8. `___dbcmp`, `___dbeq`, `___dblt`  
   Port `___fscmp`/`___fseq`/`___fslt` to 64-bit exponent.

9. `___sll2db`, `___ull2db`, `___fs2db`

**Phase 3 — depend on Phase 2**

10. `__dbmul` — port `__fsmul` to 64-bit (52-bit mantissa product).
11. `__dbdiv` — port `__fsdiv` to 64-bit.
12. `__divull`, `__divsll` — extend 32-bit restoring division to 64 iterations.
13. `__modull`, `__modsll` — call the respective divide core; return remainder.
14. `___db2sint`, `___db2slong`, `___db2sll`, `___db2ulong`, `___db2ull`, `___db2fs`
15. `__shl64`, `__shr64u`, `__shr64s`

---

## Adding a new function to the build

1. **Write the assembly** in the matching grouped runtime folder such as
   `x/runtime/int64/<name>.s` or
   `x/runtime/double/<name>.s`. Follow the
   existing file style: `.module`, `.optsdcc -mz80 sdcccall(1)`, `.area _CODE`,
   `.globl` declarations, function body.

2. **Rebuild the runtime binary** — `x/tests/tests/runtime/Makefile` picks up all
   `*.s` files under the runtime tree recursively:

   ```bash
   make -C x/tests/tests/runtime clean test
   ```

3. **Update the symbol address** — after `make`, look up the new symbol in the
   regenerated `build/tests/runtime/runtime_symbols.hpp` (or the `.noi` file)
   and copy it into `x/tests/tests/runtime/runtime_symbols_future.hpp`:

   ```cpp
   // Before:
   static constexpr uint16_t divull = 0x0000;   // placeholder

   // After (address from runtime_symbols.hpp):
   static constexpr uint16_t divull = 0x14A2;   // __divull
   ```

4. **Activate the test** — in `test_ll.cpp` or `test_double.cpp`, change the
   relevant `PENDING_TEST` entries to `TEST`:

   ```cpp
   // Activate all tests for __divull:
   TEST(ll_divull_basic)            { ... }
   TEST(ll_divull_remainder_discarded) { ... }
   ...
   ```

   Or enable an entire file at once by adding one line at the top:

   ```cpp
   #define PENDING_TEST TEST   // activates the whole file
   ```

5. **Run** — `make -C x/tests/tests/runtime test` rebuilds and runs. All
   existing tests and the newly activated tests must pass.

---

## Test file locations

| File | Contents | Status |
|------|----------|--------|
| `x/tests/tests/runtime/test_ll.cpp`     | 50 long long tests  | direct runtime coverage |
| `x/tests/tests/runtime/test_double.cpp` | 68 double tests     | direct runtime coverage |
| `x/tests/tests/runtime/runtime_symbols_future.hpp` | placeholder addresses | update per function |

---

## Notes on `EXX` and the alternate register bank

Z80 assembly tips for working with 64-bit values:

```asm
; Read a 64-bit value from DE:HL:DE':HL' into four working pairs:
        ; main DE = low16, main HL = mid-low16
        ; To read the high half:
        exx                     ; main ↔ alternate: now DE=DE', HL=HL'
        ; main DE = mid-high16, main HL = high16
        ; ... read or write high32 here ...
        exx                     ; swap back
        ; main DE = low16, main HL = mid-low16 again

; Place a 64-bit result in DE:HL:DE':HL':
        ld  de, result_lo16     ; bits 15:0
        ld  hl, result_mid_lo16 ; bits 31:16
        exx
        ld  de, result_mid_hi16 ; bits 47:32  → stored as DE'
        ld  hl, result_hi16     ; bits 63:48  → stored as HL'
        exx
        ; Function can now ret safely with result in both banks.
```

`EXX` is a single-byte instruction (0xD9) and takes only 4 T-states.
Using it to access the alternate bank has negligible cost compared to
the arithmetic itself.

BC' is available as a scratch register throughout; it is not part of the
argument or return convention.  AF' should be treated as scratch too
(`EX AF, AF'` is rarely needed here).

---

## Key IEEE-754 double constants

```
Sign bit:      bit 63                   (HL' bit 15, H' register)
Exponent:      bits 62:52  (11 bits, biased by 1023)
Mantissa:      bits 51:0   (52 explicit bits + 1 implicit leading 1)

Bias:          1023  (0x3FF)
Exponent +∞:   0x7FF (all 11 bits set)
Smallest norm: exponent = 1, mantissa = 0 → ~2.2e−308
Largest finite: exponent = 0x7FE, mantissa = all 1s → ~1.8e+308
```

The exponent field spans the top of HL' (bits 63:52): 4 bits in H' (bits 63:60)
plus the full DE' word contributes bits 47:32 of the value.

More precisely:
```
bit 63    = sign               (H' bit 7)
bits 62:52 = biased exponent   (H' bits 6:0 = exp[10:4], L' bits 7:4 = exp[3:0])
bits 51:0  = mantissa          (L' bits 3:0 = mant[51:48], DE' = mant[47:32],
                                HL = mant[31:16], DE = mant[15:0])
```

---

## Summary checklist

- [ ] Phase 1: conversions (ll↔int/long, dbneg, dbsub skeleton)
- [ ] Phase 2: mulll, dbadd, dbcmp/eq/lt, sll2db/ull2db/fs2db
- [ ] Phase 3: dbmul, dbdiv, divull/divsll, modull/modsll, db2* conversions
- [ ] Shift helpers (shl64, shr64u, shr64s) — implement alongside Phase 3
- [ ] All 50 `PENDING_TEST` entries in `test_ll.cpp` → `TEST` and passing
- [ ] All 68 `PENDING_TEST` entries in `test_double.cpp` → `TEST` and passing
- [ ] 319 existing tests still green throughout
