# Latest z88dk24: staged XCC V6

All 48 XCC cells passed. `-Os` is strictly smaller on 24/24 rows and `-Of` strictly faster on 24/24 rows against every valid competitor result. Against the better valid 80cc FP/SP result per row, `-Of` is also smaller on 23/24 rows. See `comparison-table.md` and the unrounded CSV metrics.

## Toolchain snapshot

The official September 9, 2026 nightly is `z88dk-20260909-895dc13665-25851.tgz`, from z88dk master `895dc1366573b792d98b0607fb1048d2db248b3d`. It includes current master 80cc (latest compiler-source commit `e56f2c22b321e55a852c3c159d9cbfdaa3533bac`), sccz80, and bundled zsdcc 4.6.0 revision 16639. The independent official SDCC lane is 4.6.2 revision 16858 at `a1ab477f6e5117e74964e7088db566521f7aed7a`. These are separate SDCC lanes. Exact archive, source, binary and shared target-input identities are in `snapshot.lock`, `versions.txt` and `toolchain-receipt.json`.

The executed M-model XCC SHA256 is `098acc39b843a6a755f6c35dc4fb1ea4384b89ae848c25cc04a336b3d720446a`. `xcc-source-manifest.json` records the actual dirty working-tree source hashes; a Git commit name alone does not identify this build. All four XCC partitions had identical source manifests and versions. `staged-tools.json` records the complete 548-file staged tool/library identity.

## Measurement and reuse

V6 freshly measured only `xcc_Os` and `xcc_Of` using the unchanged independent 24-program corpus and the same freshly built nightly test CRT, target headers and classic library used by every competitor lane. `xcc-results.csv` retains those measurements separately.

The seven competitor columns in `competitor-results.csv` were compiled and executed from this exact fresh toolchain snapshot during the fresh-toolchain setup. They were reused unchanged for V4, V5 and V6; no competitor timing was inferred or substituted. Their immutable CSV SHA256 is `de3e1038f37ff96636df867ca54573131c7613353faf93f394f57b23fed5780f`. V6 verified the fresh toolchain receipt before and after execution. `merge-provenance.json` records which columns were remeasured and reused.

The retained competitor versions files differ only in an incidental XCC executable hash recorded while those competitor-only partitions ran. XCC was not selected or executed in those runs. Their competitor tool hashes are identical; the actual V6 XCC identity is the one in `versions.txt` and `xcc-source-manifest.json`.

The standard sccz80/80cc lanes pass 24/24. Both SDCC implementations retain the bitfieldbench correctness failure and therefore count 23/24; their invalid bitfield results are excluded from comparisons. Both maximum-allocation SDCC lanes pass their original six-program subset; the remaining rows stay SKIP. `results.csv` preserves FAIL and SKIP explicitly.

## Archived evidence

Full V6 XCC build/run logs, per-partition source manifests and binaries remain under `build/sccz80-campaign/final-validation-v6/latest-z88dk24/`. Original competitor raw artifacts remain under `build/sccz80-campaign/latest-z88dk24-competitors/`. The broader 35,839-file build-source freeze remains under `build/sccz80-campaign/integrated-v6/`; its hash is retained in the original merge provenance, while this compact publication includes the compiler-specific source manifest instead of copying the full freeze.

The correctness repairs have one measured cost: bitfieldbench `-Os` grows from 3,960 to 3,984 bytes and from 29,677,913 to 31,667,173 T-states (+24 bytes, +1,989,260 T-states). The other 47 XCC cells are identical to V5 in size and T-states. The 24/24 size and speed headline wins remain intact. `comparison-analysis.json` and `comparison-table.md` retain the exact comparison. The seven reused competitor columns match V5 and their original raw measurements exactly.
