# Locked z88dk Benchmark Report

Final clean run: 2026-08-18. Each cell is the complete linked BIN size and
upstream `z88dk-ticks -b msx` cycle count. The supplied historical figures
remain in [target.csv](target.csv); the reproducible runner retains binaries,
maps, logs, hashes, and raw CSV under
`build/x/benchmarks/z88dk24-final-2/`.

## Result

| lane | correct | size wins vs zsdcc | speed wins vs zsdcc |
|---|---:|---:|---:|
| XCC M `-Os` | 24/24 | 4/23 | 13/23 |
| XCC M `-Of` | 24/24 | 3/23 | 18/23 |
| sccz80 | 24/24 | - | - |
| current zsdcc | 23/24 | - | - |
| current zsdcc-max | 6/6 | - | - |
| latest 80cc, frame pointer | 24/24 | - | - |
| latest 80cc, stack pointer | 24/24 | - | - |

Against the primary-competitor envelope—the best result from zsdcc, 80cc-fp,
and 80cc-sp on each valid row—XCC `-Os` is strictly fastest on 7/24 and
strictly smallest on 1/24. XCC `-Of` is strictly fastest on **13/24**, so it
wins a majority of this suite without excluding any passing competitor result.
Failed executions never participate in a performance comparison.

The `-Of` strict speed wins are `crcbench`, `ptrbench`, `queenbench`,
`searchbench`, `switchbench`, `histbench`, `vecbench`, `matrixbench`,
`interpbench`, `structbench`, `recordbench`, `lexbench`, and `maskbench`.

## Structural optimizer work

No benchmark names, source text, constants, function names, or whole-program
fingerprints are recognized by the compiler. The improvements are ordinary
data-flow and register-pressure transformations:

- fuse an unsigned word right shift followed by a compatible low-byte mask;
- keep proven call-free loop reductions and dense-dispatch source values in
  available register pairs instead of repeatedly spilling them;
- retain immutable incoming byte arguments in `E` across compare/control-flow
  regions when every clobber and exit is proven safe;
- schedule simple pointer-walk reductions in `DE`, load their short-lived
  addends directly through `IY`, and update eligible cursors in place;
- forward a word producer directly to an immediately following indirect store;
- schedule generic consecutive word-load/add chains in `HL`/`DE` and forward
  the final value to its store.

Every rule is guarded by def/use, lifetime, control-flow, alias-shape, and
register-clobber checks. Permanent regressions exercise the individual
miscompilations and the new scheduling shapes independently of this corpus.

## What the remaining losses show

The programs XCC does not win point to ordinary backend work still worth
doing; they are not candidates for workload recognition:

- `charbench` and `strbench` show the largest byte-code gap. SDCC and 80cc
  keep byte accumulators, terminator tests, and pointer increments in their
  natural 8/16-bit registers for longer. XCC still widens some byte operations
  and reloads pointer or byte state around branches.
- `sieve`, `rle`, `sortbench`, and `listbench` expose loop-addressing pressure.
  For example, XCC's saved `sieve_count` disassembly reloads the inner index
  and count from `IX` slots and repeatedly materializes the indexed address
  through `IY`; its competitors more often retain an induction pointer or
  counter across the back edge.
- `md5` is dominated by 32-bit rotate/add/xor scheduling. XCC `-Of` expands
  the program to 21,725 bytes yet still takes 30.3M cycles, while 80cc-fp uses
  17,526 bytes and 22.8M cycles. Better cross-word carry/rotate scheduling and
  register allocation—not more inlining—is the remaining opportunity.
- `fixedbench` still pays for a general 16x16-to-32 multiply and extraction on
  every Q8.8 operation. The 23-byte adapter fixes the ABI and correctness; it
  does not specialize the multiply/shift expression.
- `bitfieldbench` is correct but XCC performs separate guarded masks and
  read-modify-writes for many fields. 80cc is about 43M cycles versus XCC's
  75M, indicating that storage-unit value reuse and adjacent bitfield-update
  coalescing are the largest remaining structural gap. The faster zsdcc number
  is excluded because its checksum is wrong.

Those observations come from the retained maps and disassemblies. They also
explain why this report claims a majority win only for this locked suite; the
independent portable and bare-metal holdouts remain documented separately.

## Correctness repairs

### `fixedbench`: the exact ABI mismatch

The original failure is reproducible at the checksum assertion in
`fixedbench.c:75`. Its link map proves that `___muluint2ulong` resolved to
z88dk's `code_l_sdcc` implementation. That routine pops two operands from the
stack, whereas XCC supplies the two unsigned 16-bit operands in `HL` and `DE`
and expects the unsigned 32-bit result in `DEHL`.

[z88dk-base-xcc.patch](z88dk-base-xcc.patch) now rewrites only XCC's call to a
separate `___xcc_muluint2ulong` archive member. The replacement follows XCC's
register ABI, contributes **23 code bytes only when selected**, and fixes the
benchmark at 4,877B/43.4M (`-Os`) and 4,928B/39.9M (`-Of`). This is an ABI
adapter, not an optimization and not a benchmark oracle.

For z88dk authors, the required integration is:

1. Add the 23-byte routine as its own member under `libsrc/l/xcc/` and list it
   in `libsrc/l/xcc.lst`.
2. Add an XCC copt rule that changes `call ___muluint2ulong` to
   `EXTERN ___xcc_muluint2ulong` followed by `call ___xcc_muluint2ulong`.
3. Keep z88dk's existing `code_l_sdcc` helper unchanged for SDCC callers.

The earlier “38-byte compatibility object” was a different issue. The raw
force-linked `orig/z88dk/lib/xcc_benchmark_shim.o` placed five bodies in one
section, so all 38 linked bytes were retained for every XCC program:

| body | linked bytes |
|---|---:|
| `jp _printf` | 3 |
| HL indirect call | 1 |
| BC indirect call | 2 |
| IY indirect call | 4 |
| old register-ABI multiply | 28 |
| total forced section | 38 |

The `.o` container itself was 616 filesystem bytes. The canonical runner uses
zero-byte aliases for the call/printf spellings and the separate lazy 23-byte
multiply member, so no unused code is hidden in the measurements.

### `bitfieldbench`: optimizer defects

The GCC-verified checksum is 60004. XCC's base bitfield layout was already
correct; the failures were two optimized lowering defects:

- a linear-scan liveness test treated a branch as if execution fell through,
  allowing an `IX` spill slot to be reused while it was still live on another
  CFG edge;
- optimized word load/mask and read-modify-write fusions bypassed the normal
  lowering for pointers derived from an `IY`-resident base.

The liveness proof now walks the actual CFG, and the fusions materialize
derived pointers through the normal guarded path. Both XCC profiles return
60004 and pass. Current zsdcc independently returns 34924 and remains the
suite's sole failed primary lane.

## Why the supplied historical table differs

The requested table was not produced by building every lane from one current
z88dk checkout:

1. Its sccz80 values identify target sysroot commit
   `94bc327720721402eee7ffc1c4794245fe65ebb1`; 22/23 byte counts and 22/23
   rounded cycle counts reproduce exactly there.
2. Current compiler executables require their matching rewrite rules, so the
   runner overlays nightly zsdcc and active 80cc rules on that frozen target
   sysroot without changing the CRT, headers, or classic library.
3. Current zsdcc r16639 produces materially different images and timings from
   the published SDCC columns; changing only allocation limits does not
   reproduce them.
4. Latest 80cc is usually five bytes larger but reproduces nearly every
   rounded cycle value, which identifies compiler-revision drift rather than
   a timing-model mismatch.
5. The supplied sccz80 `fixedbench` size (4,367 bytes) is a historical anomaly:
   the reproducible binary is 4,062 bytes while its 43.2M cycles match exactly.

## Full final matrix

| bench | sccz80 | XCC `-Os` | XCC `-Of` | zsdcc | zsdcc-max | 80cc-fp | 80cc-sp |
|---|---:|---:|---:|---:|---:|---:|---:|
| charbench | 5169/171.4M | 5410/39.3M | 5470/39.5M | 5322/32.5M | 5262/26.5M | 4996/29.9M | 5024/28.2M |
| crcbench | 5075/231.9M | 5553/107.4M | 5734/86.1M | 5720/138.6M | 5676/140.6M | 5301/114.7M | 5385/122.8M |
| intbench | 5095/127.4M | 5683/56.0M | 5771/47.4M | 5601/50.7M | 5531/47.2M | 5159/33.6M | 5246/34.3M |
| ptrbench | 7149/47.0M | 7732/13.6M | 8049/11.7M | 7607/18.0M | 7545/15.6M | 7593/12.0M | 7811/14.9M |
| md5 | 14948/43.0M | 13968/34.5M | 21725/30.3M | 29229/51.3M | 19668/31.7M | 17526/22.8M | 19936/27.2M |
| sieve | 11380/9.6M | 12200/5.3M | 12253/5.3M | 12142/5.5M | 12097/5.9M | 11656/4.5M | 11737/4.4M |
| rle | 6921/40.5M | 7536/14.9M | 7610/15.1M | 7422/13.8M | - | 7067/13.7M | 7142/13.2M |
| sortbench | 5037/59.0M | 5867/40.7M | 6083/35.6M | 5788/36.8M | - | 5410/28.6M | 5648/29.8M |
| queenbench | 3578/60.3M | 4340/20.0M | 4396/18.5M | 4291/22.8M | - | 3889/25.2M | 3948/27.9M |
| searchbench | 4803/69.4M | 5492/22.2M | 5555/21.9M | 5391/22.2M | - | 5025/22.1M | 5061/26.7M |
| switchbench | 4226/37.7M | 4983/39.7M | 5265/31.0M | 5665/63.1M | - | 4654/32.7M | 4759/33.2M |
| hashbench | 7999/61.5M | 8813/42.5M | 8993/39.8M | 8692/35.6M | - | 8335/38.5M | 8394/38.2M |
| strbench | 5932/54.1M | 6742/28.0M | 6839/25.6M | 6529/18.4M | - | 6207/22.4M | 6369/28.3M |
| histbench | 3681/86.5M | 4498/28.7M | 4558/28.7M | 4427/29.3M | - | 4057/32.1M | 4098/32.5M |
| fixedbench | 4062/43.2M | 4877/43.4M | 4928/39.9M | 4652/34.4M | - | 4320/38.1M | 4369/37.9M |
| bitfieldbench | 4023/57.8M | 5230/85.9M | 5582/75.0M | 4614/24.7M FAIL | - | 4548/42.8M | 4623/43.3M |
| vecbench | 4917/25.8M | 5846/16.0M | 5941/16.6M | 5674/17.6M | - | 5316/19.6M | 5384/19.1M |
| matrixbench | 10225/68.2M | 11120/41.1M | 11349/28.5M | 10907/33.5M | - | 10641/31.7M | 10766/29.9M |
| interpbench | 3775/46.7M | 4526/39.7M | 4676/28.2M | 4458/31.2M | - | 4226/37.7M | 4319/38.7M |
| structbench | 4724/11.7M | 5531/3.0M | 5575/3.0M | 5422/3.8M | - | 5030/3.3M | 5114/3.6M |
| recordbench | 3575/15.5M | 4362/7.7M | 4409/7.0M | 4420/10.4M | - | 3868/7.6M | 3947/8.0M |
| listbench | 6744/92.9M | 7508/38.2M | 7627/38.1M | 7390/42.8M | - | 7051/34.0M | 7149/44.0M |
| lexbench | 4549/73.3M | 5236/40.8M | 5390/38.3M | 5175/67.3M | - | 4708/42.5M | 4790/43.6M |
| maskbench | 4698/75.9M | 5393/23.2M | 5454/22.9M | 5276/25.0M | - | 4918/24.8M | 4986/31.8M |

## Locked configuration

- corpus content SHA-256: `f74f02cd8852d5b0c9bb7241d0a9b0f384ccfe887105b0412a5064b4d3bdd2b4`
- frozen target sysroot/sccz80: `94bc327720721402eee7ffc1c4794245fe65ebb1`
- nightly z88dk/zsdcc: `89f94c99865eeaa3f69b860b93d8c95b028f4c23`, zsdcc r16639
- latest 80cc development head: `dd1093e6149f1de0051dc5662ffeaab8b7cbeb4a`
- XCC source/model: `b3c1eefd26d23ee4b73dbf9c58013418302261ba`, M, with this working-tree optimizer
- measured XCC executable SHA-256: `1d604b6756e3b54a319c1b7a21961aba56e5b7d5083a21dbb3b435c790f78311`
- execution: `z88dk-ticks -w 60 -b msx`

Run [prepare.sh](prepare.sh) once and [run.sh](run.sh) for a fresh checked
measurement. [toolchains.lock](toolchains.lock) is the machine-readable source
of truth.
