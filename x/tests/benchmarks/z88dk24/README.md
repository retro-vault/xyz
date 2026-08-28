# Locked z88dk Full-Program Benchmark

This directory vendors the 23 integer programs captured while z88dk's former
`80cc-codegen` work was at commit
`460ec34769f01324ca49b66145f09babdbe507fc`. That commit identifies the
compiler-integration context but did not itself track every benchmark file,
and this suite adds the host-verified `bitfieldbench` compiler comparison as
its 24th program. The runner therefore verifies the complete corpus content
hash
`f74f02cd8852d5b0c9bb7241d0a9b0f384ccfe887105b0412a5064b4d3bdd2b4`.
It compares the XCC M model at
`-Os` and `-Of` with zsdcc and the latest active 80cc development branch.
`sccz80` remains as a historical control. The requested reference numbers
are stored verbatim in [target.csv](target.csv).

## Why this is a hybrid toolchain

The supplied table cannot come from one current z88dk checkout. Its sccz80
`charbench` value (`5169B / 171.4M`) is reproduced exactly only by the older
headers, `+test` CRT and classic libraries at commit
`94bc327720721402eee7ffc1c4794245fe65ebb1`. Its recent 80cc timings require
the active 80cc branch, and a current comparison needs the nightly zsdcc and
host tools. The runner therefore locks these pieces separately:

- frozen z88dk target sysroot and sccz80: `94bc327720721402eee7ffc1c4794245fe65ebb1`
- nightly z88dk tools and zsdcc r16639: `89f94c99865eeaa3f69b860b93d8c95b028f4c23`
- active 80cc branch: `dd1093e6149f1de0051dc5662ffeaab8b7cbeb4a`
- XCC source: `b3c1eefd26d23ee4b73dbf9c58013418302261ba`, model `M`
- execution model: upstream `z88dk-ticks -w 60 -b msx`

The authoritative values are in [toolchains.lock](toolchains.lock). The
runner checks every Git commit and writes executable hashes and version
banners to `versions.txt`. Set `ALLOW_UNLOCKED=1` only when intentionally
measuring a different compiler revision.

## Prepare and run

```sh
x/tests/benchmarks/z88dk24/prepare.sh
x/tests/benchmarks/z88dk24/run.sh
```

### Current-upstream snapshot

The locked hybrid run above remains the reproducible historical comparison.
For a same-date comparison on the newest upstream target environment, use:

```sh
x/tests/benchmarks/z88dk24/prepare-current.sh
x/tests/benchmarks/z88dk24/run-current.sh
```

[`current.lock`](current.lock) records the snapshot captured on 2026-08-27:
z88dk master `fce9a75f105337c5bcd4e054838a3636d2e1a851`, the active
`80cc-multi-fixes` head `e3aeca0dc09b7b9bbf968b70fc8c29c1f1da208a`, and
official SDCC trunk `8650e2b21cdbe555c9f1119c498ff59f21693b25`. The 80cc
revision is already merged into that z88dk master, but remains independently
checked out and hashed. Official SDCC trunk is built from source after applying
[`sdcc-z88dk-current.patch`](sdcc-z88dk-current.patch), which changes only the
symbol syntax, ABI revision, classic-library helper knowledge, and other
z88dk compatibility required to link it fairly against the shared `+test`
sysroot.

The current run uses the same corpus, lane flags, complete-image byte count,
self-checking execution, and `z88dk-ticks -w 60 -b msx` cycle method. It uses
the current z88dk headers, CRT, libraries, rewrite rules, and host tools for
every lane, so its values form a new comparison snapshot and must not be mixed
with the historical `target.csv` table. Its default output is
`build/x/benchmarks/z88dk24-current-m/`.

The first complete snapshot is recorded in
[`CURRENT-RESULTS.md`](CURRENT-RESULTS.md), with the raw matrix in
[`current-results.csv`](current-results.csv) and exact executable hashes in
[`current-versions.txt`](current-versions.txt). Both XCC and both 80cc lanes
pass 24/24. Official SDCC trunk passes 23/24; its retained failure is the same
packed-bitfield checksum exposed by the historical suite.

Useful focused forms are:

```sh
FILTER='^(charbench|crcbench)$' x/tests/benchmarks/z88dk24/run.sh
LANES=xcc_Os,xcc_Of,sdcc,80cc_fp,80cc_sp \
  x/tests/benchmarks/z88dk24/run.sh

FILTER='^(charbench|crcbench)$' \
  x/tests/benchmarks/z88dk24/run-current.sh
```

The seven lane names and material flags are shared by both workflows. The
locked run's `sdcc` executable is its pinned nightly zsdcc; the current run's
is the pinned official trunk build:

For future reports, 80cc is the primary competitor: headline win counts use
the better valid 80cc frame-pointer or stack-pointer result for each program.
SDCC remains in the full table and broader valid-competitor envelope.

| lane | compiler flags |
|---|---|
| sccz80 | `-compiler=sccz80` |
| xcc -Os | M-model XCC, `-compiler=xcc -Cx-Os -Cx--runtime=z88dk-classic` |
| xcc -Of | M-model XCC, `-compiler=xcc -Cx-Of -Cx--runtime=z88dk-classic` |
| sdcc | `-compiler=sdcc` |
| sdcc-max | `-compiler=sdcc -SO3 --max-allocs-per-node200000` |
| 80cc-fp | latest 80cc, `-compiler=80cc -Cc-fframe-pointer` |
| 80cc-sp | latest 80cc, `-compiler=80cc` |

`sdcc-max` runs only for the six rows for which the supplied table has a
value; the new `bitfieldbench` row is intentionally blank in that historical
table. Current 80cc renamed its old `-frameix` switch to
`-fframe-pointer`; using the old switch changes or breaks this comparison.

Each cell is the complete linked BIN byte count and executed Z80 cycles. No
empty-image baseline is subtracted. All source files are built in their own
working directory so the MD5 fixture is visible. Only self-checking `OK`
executions participate in compiler comparisons.

## Automatic z88dk format selection

The XCC z88dk runtime profile performs the same kind of link-capability
inference expected by the classic CRT. It parses literal formats throughout
the complete `printf`/`scanf` family, including flags, widths, precision,
length modifiers, scansets, and the classic long-long mask. Each compilation
appends an OR-able block to zcc's per-link `zcc_opt.def`; the CRT therefore
builds only the handlers used anywhere in the program. Non-literal formats
and escaped formatter addresses select a conservative classic fallback, while
program-defined functions named `printf` or `scanf` are left alone.

This is enabled by the public `--runtime=z88dk-classic` compiler profile and
the `-zcc-opt=<file>` path supplied by the patched zcc driver. No benchmark
flags contain a precomputed mask, and the compiler does not inspect benchmark
names, paths, source fragments, or checksums. The ordinary X runtime remains
the default and retains its native small-`printf` call specialization.

[z88dk-xcc-points-2-3.patch](z88dk-xcc-points-2-3.patch) is the minimal
z88dk-side patch corresponding to the documented `fixedbench` repair and
automatic formatter selection. It adds only the XCC multiply archive member,
its XCC-only rewrite, and zcc's internal runtime/option-file arguments. It is
based on clean z88dk master `0a73a0209ee78b0b57e231a6819b2123133994fa`.
The larger [z88dk-base-xcc.patch](z88dk-base-xcc.patch) remains the
reproducibility patch for the frozen benchmark revision and also contains
older XCC header, section, IX-helper, macro, and `-Cx` compatibility changes.

## XCC compatibility aliases and fixedbench

The earlier local runner explicitly passed
`orig/z88dk/lib/xcc_benchmark_shim.o` on every XCC link. The z80asm object
container is 616 bytes on disk; “38-byte object” refers specifically to the
38 bytes of machine code that its single `code_compiler` section contributes
to the final BIN. It was assembled from
`orig/z88dk/lib/xcc_benchmark_shim.asm` and contained:

| symbol | implementation | bytes |
|---|---|---:|
| `___printf_sd` | `jp _printf` | 3 |
| `__sdcc_call_hl` | `jp (hl)` | 1 |
| `__sdcc_call_bc` | `push bc; ret` | 2 |
| `__sdcc_call_iy` | `push iy; pop hl; jp (hl)` | 4 |
| `___muluint2ulong` | register-ABI 16x16-to-32 multiply | 28 |
| total | raw object retained as one unit | 38 |

Because it was an explicitly linked normal object with all five bodies in one
section, rather than an archive containing one object per routine, z80asm
retained the whole section even when a program needed only `printf` or an
indirect-call trampoline. This explains the exact 38-byte inflation in the
old XCC `charbench` measurements; it does not mean that the `.o` file itself
was only 38 bytes long.

The canonical runner retains a zero-byte alias object for compatibility:
`___printf_sd` aliases `_printf`, while XCC's double-underscore HL/IY call
names alias the corresponding z88dk triple-underscore helpers. The runtime
profile now leaves `printf` calls intact, so its printf alias is normally
unused; indirect-call aliases remain available when generated. z88dk already
exports the BC spelling directly. The helper routines selected from the
classic library still contribute their real bytes, so `results.csv` remains
an honest complete-image measurement.

`fixedbench` exposed a real ABI collision: z88dk's `___muluint2ulong` is an
SDCC stack-argument routine, but XCC supplies its operands in `HL` and `DE`
and expects the result in `DEHL`. [z88dk-base-xcc.patch](z88dk-base-xcc.patch)
adds a distinct `___xcc_muluint2ulong` archive member and changes only XCC's
rewrite rule to select it. The replacement contributes 23 code bytes only to
programs that call it; the original z88dk helper remains unchanged for SDCC.
This fixes the checksum without reintroducing the force-linked 38-byte shim.

## `bitfieldbench` correctness status

The added benchmark's GCC-verified checksum is 60004. Its two XCC failures
were traced to CFG-incorrect spill-slot liveness and optimized word fusions
that bypassed normal lowering for an `IY`-derived pointer. Both defects now
have permanent regressions, and XCC passes at every measured optimization
level. Current zsdcc returns 34924 and is retained as `FAIL`; sccz80 and both
80cc modes pass. [RESULTS.md](RESULTS.md) contains the exact diagnosis and the
final matrix.

## Current comparison

Both XCC M lanes pass 24/24. Against the better valid 80cc frame-pointer or
stack-pointer result on each program, `-Os` is strictly smallest on 24/24
and fastest on 11/24; `-Of` is smallest on 23/24 and fastest on 24/24. Against
the broader valid SDCC/80cc envelope, the specialized `-Os` and `-Of` lanes
are respectively strict size and speed winners on 24/24. The
size result combines generic compiler output with literal-driven
z88dk runtime pruning; the speed gains come from generic data-flow,
register-allocation, bit-width, bitfield, affine-address, loop-induction,
fixed-trip, pointer-walk, producer/store, and wide-carry scheduling rules. The
compiler contains no benchmark-name, source-fragment, magic-constant, or
program-fingerprint recognition.

| XCC lane | Correct | Size wins vs 80cc | Speed wins vs 80cc |
|---|---:|---:|---:|
| `xcc -Os` | 24/24 | 24/24 | 11/24 |
| `xcc -Of` | 24/24 | 23/24 | 24/24 |

This table is the final post-optimization rerun and supersedes the earlier
intermediate 8/24 and 17/24 speed counts. The authoritative per-row values are
in [`CURRENT-RESULTS.md`](CURRENT-RESULTS.md) and
[`current-results.csv`](current-results.csv).

## Outputs

The default output directory is `build/x/benchmarks/z88dk24-nightly-m/`:

- `results.csv` — raw statuses, bytes and cycles
- `summary.md` — comparison and exact-target-match tables
- `versions.txt` — commits, flags, versions and executable SHA-256 values
- `artifacts/<benchmark>/<lane>/program.bin` — every measured executable
- `work/<benchmark>/` — build logs, run logs and maps

See [RESULTS.md](RESULTS.md) for the diagnosis and most recent complete run.
