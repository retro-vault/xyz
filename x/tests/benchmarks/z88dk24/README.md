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

Useful focused forms are:

```sh
FILTER='^(charbench|crcbench)$' x/tests/benchmarks/z88dk24/run.sh
LANES=xcc_Os,xcc_Of,sdcc,80cc_fp,80cc_sp \
  x/tests/benchmarks/z88dk24/run.sh
```

The seven lanes and their material flags are:

| lane | compiler flags |
|---|---|
| sccz80 | `-compiler=sccz80` |
| xcc -Os | M-model XCC, `-compiler=xcc -Cx-Os -Cx--runtime=z88dk-classic` |
| xcc -Of | M-model XCC, `-compiler=xcc -Cx-Of -Cx--runtime=z88dk-classic` |
| sdcc | nightly `z88dk-zsdcc`, `-compiler=sdcc` |
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

Both XCC M lanes pass 24/24. Against the best valid result from current zsdcc,
80cc-fp, and 80cc-sp on each program, `-Os` is strictly smallest on 24/24
and `-Of` on 22/24. `-Of` remains strictly fastest on 13/24 and `-Os` on
7/24. The size result combines generic compiler output with literal-driven
z88dk runtime pruning; the speed gains come from generic data-flow,
register-allocation, pointer-walk, producer/store, and word-load/add
scheduling rules. The compiler contains no benchmark-name or source-pattern
recognition.

## Outputs

The default output directory is `build/x/benchmarks/z88dk24-nightly-m/`:

- `results.csv` — raw statuses, bytes and cycles
- `summary.md` — comparison and exact-target-match tables
- `versions.txt` — commits, flags, versions and executable SHA-256 values
- `artifacts/<benchmark>/<lane>/program.bin` — every measured executable
- `work/<benchmark>/` — build logs, run logs and maps

See [RESULTS.md](RESULTS.md) for the diagnosis and most recent complete run.
