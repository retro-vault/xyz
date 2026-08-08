# Canonical Shared-z88dk Benchmark

This is the only 23-program table used for XCC optimization decisions. Every
lane links z88dk's `+test` CRT and classic library, and every image executes in
the repository's `z80_exec` Z80 model. Do not mix these figures with the older
XCC private-runtime matrix or the retired SCCZ80 lane.

The corpus is pinned at `460ec34769f01324ca49b66145f09babdbe507fc` and the
external z88dk checkout at `94bc327720721402eee7ffc1c4794245fe65ebb1`.
The machine-readable frozen table is [reference.csv](reference.csv).

## Frozen Reference

Each cell is complete linked bytes / executed cycles in millions.

| bench | xcc-Os | xcc-Of | sdcc | 80cc-fp | 80cc-sp |
|---|---:|---:|---:|---:|---:|
| charbench | 5434B/39.1M | 5542B/38.8M | 5325B/32.5M | 5389B/35.8M | 5493B/39.6M |
| crcbench | 5582B/107.1M | 5808B/86.1M | 5730B/138.6M | 5666B/122.3M | 5777B/128.4M |
| intbench | 5690B/46.1M | 5849B/37.4M | 5610B/50.7M | 5552B/37.3M | 5689B/38.3M |
| ptrbench | 7656B/16.6M | 8135B/13.2M | 7629B/18.2M | 8412B/13.6M | 8694B/16.1M |
| md5 | 14842B/38.3M | 22404B/32.4M | 29148B/53.4M | 20663B/26.2M | 21843B/29.2M |
| sieve | 12225B/5.2M | 12327B/5.2M | 12150B/5.5M | 11992B/5.3M | 12103B/5.9M |
| rle | 7667B/9.4M | 7761B/9.4M | 7417B/11.4M | 7490B/16.8M | 7673B/19.0M |
| sortbench | 5909B/41.5M | 6173B/35.7M | 5794B/36.8M | 6029B/31.8M | 6274B/34.4M |
| queenbench | 4453B/41.4M | 4558B/40.4M | 4303B/22.8M | 4221B/29.2M | 4342B/32.3M |
| searchbench | 5604B/38.6M | 5711B/38.3M | 5398B/22.3M | 5381B/24.0M | 5510B/30.3M |
| switchbench | 5022B/43.3M | 5457B/33.4M | 5708B/65.2M | 5282B/35.5M | 5391B/35.1M |
| hashbench | 8919B/54.7M | 9100B/46.0M | 8708B/35.6M | 8894B/44.2M | 9066B/45.8M |
| strbench | 6747B/28.3M | 6931B/26.1M | 6540B/18.4M | 6702B/26.7M | 6827B/31.6M |
| histbench | 4605B/38.0M | 4734B/37.8M | 4438B/29.3M | 4443B/46.5M | 4525B/49.2M |
| fixedbench | 4933B/47.0M | 5044B/42.2M | 4646B/34.4M | 4720B/39.2M | 4871B/39.9M |
| vecbench | 5891B/16.6M | 6033B/17.1M | 5669B/16.8M | 5735B/21.8M | 5847B/22.2M |
| matrixbench | 11128B/40.3M | 11442B/32.8M | 10899B/33.4M | 11131B/38.2M | 11341B/42.0M |
| interpbench | 4770B/56.4M | 4944B/38.5M | 4470B/31.2M | 4875B/41.2M | 4973B/40.8M |
| structbench | 5578B/3.6M | 5668B/3.6M | 5419B/3.8M | 5361B/3.3M | 5484B/3.7M |
| recordbench | 4405B/8.8M | 4516B/7.5M | 4432B/10.5M | 4153B/8.1M | 4264B/9.0M |
| listbench | 7523B/44.3M | 7698B/38.5M | 7398B/42.8M | 7484B/39.8M | 7566B/44.4M |
| lexbench | 5370B/55.3M | 5578B/49.2M | 5147B/55.5M | 5206B/59.8M | 5374B/61.9M |
| maskbench | 5619B/39.9M | 5728B/39.6M | 5284B/25.1M | 5277B/26.8M | 5394B/34.6M |

## Measurement Setup

Build a fresh z88dk checkout with submodules using `./build.sh`, then build
the normally omitted SDCC driver with:

```sh
make BUILD_SDCC=1 BUILD_SDCC_HTTP=1 bin/z88dk-zsdcc
```

The external checkout needs these six compatibility fixes. They are not part
of this repository:

1. In `src/zcc/zcc.c`, define `__SDCC` alongside `__XCC` in the XCC branch.
   This selects declarations matching XCC's SDCC-compatible calling
   conventions instead of the unrelated generic/newlib fallback.
2. In `include/sys/compiler.h`, map XCC's `__smallc`, `__z88dk_callee`, and
   `__z88dk_fastcall` macros to XCC's modern attribute spelling instead of
   erasing them before XCC parses the source.
3. Blank z88dk's unsupported `__preserves_regs(...)` declaration attribute
   for XCC (`-D'__preserves_regs(...)='`).
4. Put a short, plain-text copt rule at the start of `lib/xcc_rules.1` that
   rewrites `SECTION bss_compiler` to `SECTION code_compiler`. It must precede
   `enter_ix`/`leave_ix`; quoted rule titles trigger a copt lexer quirk.
5. Link a shim defining `___printf_sd` and the indirect-call trampolines
   `__sdcc_call_hl`, `__sdcc_call_bc`, and `__sdcc_call_iy`.
6. Put XCC's register-ABI `___muluint2ulong` body in that shim so the linker
   does not resolve z88dk's same-named, stack-ABI SDCC implementation.

The genuine XCC Small-C/fastcall return-register correction (DE family to HL
family) is already in this repository. The section rule fixes initialized
data in `switchbench`, `interpbench`, and the optimized `md5`; the multiply
shim fixes `fixedbench`. These are compatibility/correctness fixes, not
optimizer wins.

Run the matrix with:

```sh
Z88DK=/path/to/patched/z88dk \
  OUT=build/x/benchmarks/z88dk24-final \
  bash x/tests/benchmarks/z88dk24/run.sh
```

## Current Result (2026-08-07)

All XCC and SDCC images pass 23/23 at both XCC profiles. Both local 80CC
lanes pass 22/23; their failed `lexbench` executions are excluded from the
best-successful-competitor envelope.

| XCC lane | Correct | Size wins vs SDCC | Speed wins vs SDCC | Size strict best | Speed strict best |
|---|---:|---:|---:|---:|---:|
| `-Os` | 23/23 | 4/23 | 14/23 | 3/23 | 8/23 |
| `-Of` | 23/23 | 2/23 | 17/23 | 0/23 | 15/23 |

Geometric means use all 23 successful XCC/SDCC pairs:

| lane | bytes vs frozen XCC | cycles vs frozen XCC | bytes vs SDCC | cycles vs SDCC |
|---|---:|---:|---:|---:|
| `-Os` | -0.81% | -10.11% | -1.85% | +0.24% |
| `-Of` | -1.76% | -10.72% | +1.78% | -11.17% |

Thus the size profile is smaller than SDCC in aggregate and the speed profile
is substantially faster, on the same CRT/library baseline. This is not a
claim that every program wins both metrics: `charbench`, `strbench`,
`fixedbench`, and a few smaller gaps remain useful optimization targets.

The largest general speed corrections relative to the frozen XCC table are:

| bench | `-Os` change | `-Of` change |
|---|---:|---:|
| queenbench | -94B / -50.9% cycles | -154B / -52.7% cycles |
| searchbench | -69B / -38.5% cycles | -124B / -42.5% cycles |
| maskbench | -188B / -38.0% cycles | -245B / -41.9% cycles |
| histbench | -84B / -22.9% cycles | -160B / -22.5% cycles |
| interpbench | -159B / -24.2% cycles | -188B / -14.9% cycles |
| hashbench | -45B / -9.6% cycles | -121B / -17.8% cycles |

The changes responsible are source-independent:

- `ADDRESS_OF` now uses the pointer value width during register-allocation
  eligibility. Taking the address of a large array no longer disables all
  physical allocation in the containing function.
- A conservative no-call loop recurrence can occupy DE when its operations,
  CFG, and the BC/IY homes around it prove DE preservation.
- Shift/add/unsigned-byte accumulation accepts both equivalent IR forms: an
  explicit widening cast and the direct byte operand left by value
  propagation.
- Two independent byte cursors can occupy BC and IY across byte comparisons
  and early returns when every operation in the live window is proven safe.
- Data emitters split long byte directives into 16-value lines, avoiding the
  z80asm/copt long-line failure that previously broke `interpbench` builds.

No rule checks a source path, function name, benchmark constant, checksum, or
complete-function signature. Raw CSV, maps, binaries, output, and run logs for
this measurement are under `build/x/benchmarks/z88dk24-final/`. The staged
compiler executable used for it has SHA-256
`4bb846d39292b0f6d2d58f0db717f7cab71be7f632a00db1d9e32c634a9aee14`.

Focused code-generation and execution regressions pass 20/20 across the
requested optimization matrices and include both assembler dialects. A clean
broader XCC run completed the core, integer, and long execution partitions
and reached runtime `t114` without an observed failure before that
deliberately non-required exhaustive run was stopped. The definitive
acceptance test is the complete shared-z88dk matrix above.
