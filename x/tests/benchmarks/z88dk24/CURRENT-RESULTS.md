# Current-Upstream z88dk Full-Program Integer Benchmarks

Every lane uses the pinned current z88dk headers, `+test` CRT and classic
library. Official SDCC trunk is patched only for z88dk ABI compatibility;
80cc comes from its independently pinned active branch. XCC is the M
distribution. Images execute with the same upstream `z88dk-ticks -b msx`
method as the locked suite; each cell is complete linked bytes / cycles.

## Snapshot

This result was captured on 2026-08-27 at the revisions in
[current.lock](current.lock). Exact executable hashes and version banners are
in [current-versions.txt](current-versions.txt). The raw matrix is
[current-results.csv](current-results.csv).

## Summary

The primary competitor is 80cc. Each comparison uses the better valid
result of its frame-pointer and stack-pointer modes for that row.

| XCC lane | Correct | Size wins vs 80cc | Speed wins vs 80cc |
|---|---:|---:|---:|
| xcc -Os | 24/24 | 24/24 | 8/24 |
| xcc -Of | 24/24 | 23/24 | 17/24 |

### Broader valid-competitor envelope

This secondary envelope contains SDCC, 80cc-fp and 80cc-sp. sccz80 is
retained as a historical control and sdcc-max is a limited expensive-allocation
probe.

| XCC lane | Size strict best | Size within 5% | Speed strict best | Speed within 5% |
|---|---:|---:|---:|---:|
| xcc -Os | 24/24 | 24/24 | 8/24 | 10/24 |
| xcc -Of | 23/24 | 23/24 | 17/24 | 17/24 |

## Full results

| bench | sccz80 | xcc -Os | xcc -Of | sdcc | sdcc-max | 80cc-fp | 80cc-sp |
|---|---:|---:|---:|---:|---:|---:|---:|
| charbench | 5261B / 171.4M | 4669B / 26.6M | 4726B / 26.8M | 5414B / 32.5M | 5354B / 26.5M | 5082B / 29.9M | 5110B / 28.2M |
| crcbench | 5167B / 231.8M | 4852B / 107.4M | 5030B / 86.1M | 5812B / 138.6M | 5768B / 140.6M | 5387B / 114.6M | 5463B / 121.4M |
| intbench | 5187B / 127.4M | 4904B / 40.3M | 4993B / 33.0M | 5693B / 50.7M | 5623B / 47.2M | 5245B / 33.6M | 5332B / 34.3M |
| ptrbench | 7241B / 47.0M | 7031B / 13.6M | 7345B / 11.7M | 7699B / 18.0M | 7637B / 15.5M | 7679B / 12.0M | 7897B / 14.8M |
| md5 | 15040B / 42.8M | 14042B / 34.4M | 21798B / 30.2M | 29321B / 51.1M | 19760B / 31.6M | 17569B / 22.5M | 19031B / 24.4M |
| sieve | 11472B / 9.6M | 11501B / 5.3M | 11551B / 5.3M | 12234B / 5.4M | 12189B / 5.9M | 11742B / 4.5M | 11823B / 4.4M |
| rle | 7013B / 40.5M | 6837B / 15.0M | 6907B / 15.2M | 7514B / 13.8M | - | 7153B / 13.7M | 7228B / 13.2M |
| sortbench | 5129B / 59.0M | 5166B / 40.7M | 5379B / 35.6M | 5880B / 36.8M | - | 5492B / 28.6M | 5730B / 29.7M |
| queenbench | 3670B / 60.3M | 3639B / 20.0M | 3692B / 18.5M | 4383B / 22.8M | - | 3975B / 25.2M | 4034B / 27.9M |
| searchbench | 4895B / 69.4M | 4791B / 22.2M | 4851B / 21.9M | 5483B / 22.2M | - | 5111B / 22.1M | 5147B / 26.7M |
| switchbench | 4318B / 37.6M | 4282B / 39.7M | 4561B / 31.0M | 5757B / 63.1M | - | 4740B / 32.7M | 4777B / 31.3M |
| hashbench | 8091B / 61.5M | 8103B / 42.5M | 8280B / 39.8M | 8784B / 35.6M | - | 8421B / 38.5M | 8480B / 38.2M |
| strbench | 6024B / 54.1M | 6030B / 27.5M | 6123B / 25.1M | 6621B / 18.4M | - | 6293B / 22.4M | 6455B / 28.3M |
| histbench | 3773B / 86.5M | 3797B / 28.7M | 3854B / 28.7M | 4519B / 29.3M | - | 4143B / 32.1M | 4184B / 32.5M |
| fixedbench | 4154B / 43.1M | 4141B / 43.4M | 4189B / 39.9M | 4744B / 34.4M | - | 4406B / 38.1M | 4447B / 36.9M |
| bitfieldbench | 4115B / 57.8M | 4353B / 49.5M | 4473B / 42.8M | 4706B / 24.6M FAIL | - | 4634B / 42.8M | 4709B / 43.3M |
| vecbench | 5009B / 25.8M | 5153B / 16.0M | 5245B / 16.6M | 5766B / 17.6M | - | 5402B / 19.6M | 5470B / 19.1M |
| matrixbench | 10317B / 68.1M | 10419B / 41.1M | 10645B / 28.5M | 10999B / 33.5M | - | 10727B / 31.7M | 10852B / 29.9M |
| interpbench | 3867B / 46.7M | 3825B / 39.7M | 3972B / 28.2M | 4550B / 31.2M | - | 4312B / 37.7M | 4405B / 38.7M |
| structbench | 4816B / 11.7M | 4830B / 3.0M | 4871B / 3.0M | 5514B / 3.7M | - | 5116B / 3.2M | 5200B / 3.6M |
| recordbench | 3667B / 15.5M | 3661B / 7.7M | 3705B / 6.9M | 4512B / 10.4M | - | 3954B / 7.6M | 4033B / 8.0M |
| listbench | 6836B / 92.9M | 6807B / 38.2M | 6925B / 29.8M | 7482B / 42.8M | - | 7137B / 33.9M | 7235B / 43.9M |
| lexbench | 4641B / 73.2M | 4526B / 40.8M | 4677B / 38.2M | 5267B / 67.3M | - | 4794B / 42.5M | 4876B / 43.6M |
| maskbench | 4790B / 75.9M | 4692B / 23.2M | 4750B / 22.9M | 5368B / 25.0M | - | 5004B / 24.8M | 5072B / 31.8M |

## Correctness

- sccz80: 24/24
- xcc -Os: 24/24
- xcc -Of: 24/24
- sdcc: 23/24
- sdcc-max: 6/6
- 80cc-fp: 24/24
- 80cc-sp: 24/24

## Generated outputs

The runner writes reproducible binaries, maps, and build/run logs beneath
`build/x/benchmarks/z88dk24-current-m/`. They are build artifacts and are not
checked into the source tree.
