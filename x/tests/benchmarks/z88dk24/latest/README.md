# Fresh upstream snapshot

`latest.lock` records the nightly downloaded on 2026-09-09 and the official
SDCC trunk fetched that day. It is an immutable dated snapshot, separate from
`current.lock` and `toolchains.lock`. The nightly's sccz80 and 80cc sources match
master `895dc1366573b792d98b0607fb1048d2db248b3d`; its 80cc includes newer code
than the former `80cc-multi-fixes` pin. The nightly bundles zsdcc revision 16639,
while the separate official SDCC lane uses revision 16858.

From the repository root:

```sh
bash x/tests/benchmarks/z88dk24/prepare-latest.sh
bash x/tests/benchmarks/z88dk24/run-latest.sh
```

Preparation builds the external host tools and shared `+test` library. It never
builds or replaces XCC. Use `--download-only` to prepare just source inputs;
`JOBS` controls build parallelism, and `LATEST_TOOLCHAINS` selects an alternative
directory for the complete snapshot. The run script accepts the existing
`--filter`, `--lanes`, `--outdir`, and `--xcc` options.

All nine lanes use the same fresh nightly headers, CRT and classic library.
The additional `zsdcc` and `zsdcc_max` columns distinguish the bundled compiler
from official `sdcc` and `sdcc_max`. Both expensive allocation probes retain the
existing six-workload subset. The original 24 sources, corpus hash, correctness
checks, compiler flags, tick machine and execution budget are preserved.

The compatibility patches forward XCC's per-link options and adapt official
SDCC to the z88dk ABI. The small predicate alias preserves compatibility with
the nightly's peephole files after official SDCC renamed an existing predicate;
both names invoke the same upstream implementation.

`toolchain-receipt.json` records hashes of the source bundles, external binaries
and shared target inputs. Each run verifies that receipt and writes an
`xcc-source-manifest.json` containing the actual XCC binary hash, source file
hashes, Git head and dirty status. Those inputs are checked again after the run.
`versions.txt` includes the manifest hash as well as compiler hashes and banners;
the source head alone does not identify a build from a dirty working tree.
