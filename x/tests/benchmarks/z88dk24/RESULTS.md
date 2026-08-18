# Locked z88dk Benchmark Report

Final seven-lane run: 2026-08-18. Each cell is the complete linked BIN size
and upstream `z88dk-ticks -b msx` cycle count. Binaries, maps, logs, hashes,
and raw CSV are retained under
`build/x/benchmarks/z88dk24-format-final/`. A second XCC-only run from the
final rebuilt compiler, `z88dk24-format-final-xcc/`, reproduced every XCC byte
and cycle value exactly.

## Result

| lane | correct | size wins vs zsdcc | speed wins vs zsdcc | strict size wins vs best SDCC/80cc |
|---|---:|---:|---:|---:|
| XCC M `-Os` | 24/24 | 23/23 | 13/23 | **24/24** |
| XCC M `-Of` | 24/24 | 23/23 | 18/23 | **22/24** |
| sccz80 | 24/24 | - | - | - |
| current zsdcc | 23/24 | - | - | - |
| current zsdcc-max | 6/6 | - | - | - |
| latest 80cc, frame pointer | 24/24 | - | - | - |
| latest 80cc, stack pointer | 24/24 | - | - | - |

The primary-competitor envelope uses the best valid result from current
zsdcc, 80cc-fp, and 80cc-sp on each row. XCC `-Os` is strictly smallest on
every program. `-Of` loses size only on `md5` and `bitfieldbench`, where its
speed-oriented transformations deliberately enlarge code. The earlier speed
result is unchanged: `-Os` is strictly fastest on 7/24 and `-Of` on 13/24.
Failed executions never participate in comparisons.

## Why the size gap closed

XCC previously made z88dk's CRT select its broad default printf table. Most
programs use only the test framework's `%s` and `%d`, but still linked roughly
793 bytes of unused handlers. 80cc and sccz80 already communicated literal
format requirements to the CRT through zcc's per-link option file.

XCC now implements that runtime contract generically:

- `--runtime=z88dk-classic` selects external classic-runtime behavior and
  leaves calls as ordinary `printf`/`scanf` family calls;
- literal formats are parsed for conversions, flags, width, precision,
  length modifiers, scansets, and classic long-long handlers;
- every translation unit appends an OR-able capability block to the
  `-zcc-opt=<file>` supplied by zcc, before the CRT is assembled;
- dynamic formats and escaped formatter addresses select the supported
  classic fallback surface;
- functions defined by the program are excluded even if named `printf` or
  `scanf`, and conversion-free literals still request the formatter core.

No benchmark name, path, source fragment, checksum, or precomputed mask is
used. The runner passes only the public runtime profile; the masks are derived
from normal C semantics. This changes linked library selection rather than
pretending compiler-emitted payload bytes disappeared. `charbench`, for
example, falls from 5,410 to 4,617 bytes while retaining essentially the same
39.2M cycles. MD5 contains a genuinely dynamic `vsnprintf` path, so the
conservative fallback keeps its broad formatter and changes by only four
bytes; it still passes at 13,964 bytes.

## Structural code-generation work

The independent speed improvements remain ordinary data-flow and
register-pressure transformations:

- fuse an unsigned word right shift followed by a compatible low-byte mask;
- retain proven call-free loop reductions and dense-dispatch source values in
  register pairs;
- retain immutable incoming byte arguments in `E` across proven-safe regions;
- schedule pointer-walk reductions in `DE`, short-lived addends through `IY`,
  and eligible cursor updates in place;
- forward word producers directly to immediate indirect stores;
- schedule consecutive word-load/add chains in `HL`/`DE`.

The remaining performance losses are also structural. Byte-heavy programs
still expose avoidable widening and reloads; several loops need better
induction-pointer residence; MD5 needs stronger cross-word rotate/add/xor
scheduling; fixed-point code needs profitable multiply/shift fusion; and
bitfields need storage-unit reuse and adjacent update coalescing. In
particular, XCC's correct `bitfieldbench` remains 75.0M–85.9M cycles versus
80cc's correct 42.8M–43.3M. zsdcc's faster row is excluded because its
checksum is wrong.

## Correctness repairs

### `fixedbench`: exact ABI mismatch

The original checksum failed at `fixedbench.c:75`. Its map proved
`___muluint2ulong` came from z88dk's `code_l_sdcc` implementation, which pops
both operands from the stack. XCC instead supplies unsigned 16-bit operands
in `HL` and `DE` and expects the unsigned 32-bit result in `DEHL`.

[z88dk-base-xcc.patch](z88dk-base-xcc.patch) rewrites only XCC's call to a
separate `___xcc_muluint2ulong` archive member. The register-ABI replacement
contributes 23 code bytes only when selected and leaves SDCC's helper intact.
For z88dk integration: add the routine under `libsrc/l/xcc/`, list it in
`libsrc/l/xcc.lst`, and make XCC's copt rule rename the call and declare the
new symbol `EXTERN`.

The earlier “38-byte compatibility object” was separate. Its `.o` container
was 616 filesystem bytes, but one force-linked code section retained all five
bodies: a 3-byte printf jump, 1/2/4-byte indirect-call helpers, and a 28-byte
multiply routine, totaling 38 linked bytes. The current runner uses zero-byte
aliases plus the independently archived 23-byte multiply member, so unused
code is not hidden in the measurements.

### `bitfieldbench`: optimizer defects

The host-verified checksum is 60004. XCC's layout was correct; optimized runs
failed because spill-slot liveness followed lexical fallthrough instead of
the actual CFG, and two word fusions bypassed guarded lowering for an
`IY`-derived pointer. CFG liveness and pointer materialization are now fixed,
with permanent regressions. Both XCC lanes pass; current zsdcc returns 34924.

## Full final matrix

| bench | sccz80 | XCC `-Os` | XCC `-Of` | zsdcc | zsdcc-max | 80cc-fp | 80cc-sp |
|---|---:|---:|---:|---:|---:|---:|---:|
| charbench | 5169/171.4M | 4617/39.2M | 4677/39.4M | 5322/32.5M | 5262/26.5M | 4996/29.9M | 5024/28.2M |
| crcbench | 5075/231.9M | 4760/107.4M | 4941/86.1M | 5720/138.6M | 5676/140.6M | 5301/114.7M | 5385/122.8M |
| intbench | 5095/127.4M | 4890/56.0M | 4978/47.4M | 5601/50.7M | 5531/47.2M | 5159/33.6M | 5246/34.3M |
| ptrbench | 7149/47.0M | 6939/13.6M | 7256/11.7M | 7607/18.0M | 7545/15.6M | 7593/12.0M | 7811/14.9M |
| md5 | 14948/43.0M | 13964/34.5M | 21723/30.3M | 29229/51.3M | 19668/31.7M | 17526/22.8M | 19936/27.2M |
| sieve | 11380/9.6M | 11407/5.3M | 11460/5.3M | 12142/5.5M | 12097/5.9M | 11656/4.5M | 11737/4.4M |
| rle | 6921/40.5M | 6743/14.9M | 6817/15.1M | 7422/13.8M | - | 7067/13.7M | 7142/13.2M |
| sortbench | 5037/59.0M | 5074/40.7M | 5290/35.6M | 5788/36.8M | - | 5410/28.6M | 5648/29.8M |
| queenbench | 3578/60.3M | 3547/20.0M | 3603/18.5M | 4291/22.8M | - | 3889/25.2M | 3948/27.9M |
| searchbench | 4803/69.4M | 4699/22.2M | 4762/21.9M | 5391/22.2M | - | 5025/22.1M | 5061/26.7M |
| switchbench | 4226/37.7M | 4190/39.7M | 4472/31.0M | 5665/63.1M | - | 4654/32.7M | 4759/33.2M |
| hashbench | 7999/61.5M | 8020/42.5M | 8200/39.8M | 8692/35.6M | - | 8335/38.5M | 8394/38.2M |
| strbench | 5932/54.1M | 5949/28.0M | 6046/25.6M | 6529/18.4M | - | 6207/22.4M | 6369/28.3M |
| histbench | 3681/86.5M | 3705/28.7M | 3765/28.7M | 4427/29.3M | - | 4057/32.1M | 4098/32.5M |
| fixedbench | 4062/43.2M | 4084/43.4M | 4135/39.9M | 4652/34.4M | - | 4320/38.1M | 4369/37.9M |
| bitfieldbench | 4023/57.8M | 4437/85.9M | 4789/75.0M | 4614/24.7M FAIL | - | 4548/42.8M | 4623/43.3M |
| vecbench | 4917/25.8M | 5061/16.0M | 5156/16.6M | 5674/17.6M | - | 5316/19.6M | 5384/19.1M |
| matrixbench | 10225/68.2M | 10327/41.1M | 10556/28.5M | 10907/33.5M | - | 10641/31.7M | 10766/29.9M |
| interpbench | 3775/46.7M | 3733/39.7M | 3883/28.2M | 4458/31.2M | - | 4226/37.7M | 4319/38.7M |
| structbench | 4724/11.7M | 4738/3.0M | 4782/3.0M | 5422/3.8M | - | 5030/3.3M | 5114/3.6M |
| recordbench | 3575/15.5M | 3569/7.7M | 3616/7.0M | 4420/10.4M | - | 3868/7.6M | 3947/8.0M |
| listbench | 6744/92.9M | 6715/38.2M | 6834/38.1M | 7390/42.8M | - | 7051/34.0M | 7149/44.0M |
| lexbench | 4549/73.3M | 4443/40.8M | 4597/38.2M | 5175/67.3M | - | 4708/42.5M | 4790/43.6M |
| maskbench | 4698/75.9M | 4600/23.2M | 4661/22.9M | 5276/25.0M | - | 4918/24.8M | 4986/31.8M |

## Locked configuration

- corpus SHA-256: `f74f02cd8852d5b0c9bb7241d0a9b0f384ccfe887105b0412a5064b4d3bdd2b4`
- frozen target sysroot/sccz80: `94bc327720721402eee7ffc1c4794245fe65ebb1`
- nightly z88dk/zsdcc: `89f94c99865eeaa3f69b860b93d8c95b028f4c23`, zsdcc r16639
- latest 80cc head: `dd1093e6149f1de0051dc5662ffeaab8b7cbeb4a`
- XCC: development tree at `83b45d735e4139407e6e9ddade19c8d4a67a649b`, M model, with the documented working-tree changes
- execution: `z88dk-ticks -w 60 -b msx`

The run intentionally used `ALLOW_UNLOCKED=1` because the active development
tree is newer than the XCC commit recorded in `toolchains.lock`; the three
z88dk repositories and corpus remain pinned. `versions.txt` records the exact
executable hashes. Run [prepare.sh](prepare.sh) once and [run.sh](run.sh) for a
fresh checked measurement.
