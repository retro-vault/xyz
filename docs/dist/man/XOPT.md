# xopt

`xopt` optimizes Z80 assembly files using the shared `libxopt`
optimizer library.

## Synopsis

```bash
xopt [options] file.s...
```

## Common Usage

```bash
xopt -O2 input.s -o output.s
xopt -O3 --out-dir optimized *.s
xopt -Of --in-place generated/*.s
xopt -O3 --cross-file a.s b.s -o combined.s
xopt --stats -O3 *.s
xopt --reg-coverage *.s
```

Wildcards such as `*.s` and `generated/*.s` are expanded by the invoking
shell, so they behave like passing each matching file explicitly.

## Statistics

`--stats` reads the input files, optimizes them in memory, and prints a text
table showing estimated Z80 bytes/cycles before and after optimization. It does
not write optimized assembly output.

The cycle numbers are static instruction timing estimates. Conditional branch
paths and runtime loop counts still need benchmark/emulator measurement.

## Register Coverage

`--reg-coverage` reads the input files and prints a register-family pressure
report without writing optimized assembly. It groups touches into the Z80
families `A`, `BC`, `DE`, `HL`, `IX`, `IY`, `SP`, and `F`, then highlights
hot/cold families and the hottest straight-line windows.

The alternate register bank is measured separately as `A'`, `F'`, `BC'`,
`DE'`, and `HL'`. The `alt` column counts instructions that touch the
alternate bank; `swap` counts `exx` and `ex af,af'` bank switches.

The `press` column is the average number of register families touched per
instruction. High pressure windows are good places to look for register
staging, forwarding, or live-range cleanup opportunities.

## Options

- `-O0`, `-O2`, `-Os`, `-Of`, `-O3`: select the optimization level.
  `-Of` and `-O3` enable speed-biased peepholes; `-O0` copies input through.
- `-o FILE`: write one optimized output file.
- `--out-dir DIR`: optimize each input independently and write one file per
  input under `DIR`.
- `--in-place`: replace each input file with optimized output.
- `--stdout`: write optimized output to standard output.
- `--cross-file`: concatenate all inputs and optimize them as one unit. This
  allows label and branch optimizations to see across file boundaries, and
  writes one output stream. Inputs should use labels that are unique in the
  combined assembly unit.
- `--stats`: print a per-file byte/cycle savings table only.
- `--reg-coverage`: print register pressure/coverage tables only.
- `--version`: show version.
- `-h`, `--help`: show usage.
