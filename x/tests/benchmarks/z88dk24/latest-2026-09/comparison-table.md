# V6 versus the latest downloaded competitors

All 48 XCC cells passed. The seven competitor lanes are the retained immutable fresh measurements; FAIL and SKIP results are excluded from each comparison. The benchmark corpus and fresh shared target inputs are identical to V5.

| Benchmark | XCC Os bytes | Smallest competitor bytes | XCC Of ticks | Fastest competitor ticks |
|---|---:|---:|---:|---:|
| charbench | 4644 | 5082 (80cc_fp) | 26375916 | 26519054 (sdcc_max) |
| crcbench | 4820 | 5167 (sccz80) | 86035675 | 114635406 (80cc_fp) |
| intbench | 4861 | 5187 (sccz80) | 32960132 | 33594542 (80cc_fp) |
| ptrbench | 6764 | 7241 (sccz80) | 11267197 | 12027589 (80cc_fp) |
| md5 | 11550 | 15013 (sccz80) | 21689787 | 22478882 (80cc_fp) |
| sieve | 11439 | 11472 (sccz80) | 3757371 | 4379718 (80cc_sp) |
| rle | 6802 | 7013 (sccz80) | 12068928 | 13200276 (80cc_sp) |
| sortbench | 5044 | 5129 (sccz80) | 28208692 | 28580132 (80cc_fp) |
| queenbench | 3612 | 3670 (sccz80) | 18455856 | 22770315 (sdcc) |
| searchbench | 4759 | 4895 (sccz80) | 21763678 | 22062540 (80cc_fp) |
| switchbench | 4242 | 4318 (sccz80) | 31007401 | 31344825 (80cc_sp) |
| hashbench | 8032 | 8091 (sccz80) | 24031267 | 35626638 (sdcc) |
| strbench | 5882 | 6024 (sccz80) | 18012089 | 18386361 (sdcc) |
| histbench | 3735 | 3773 (sccz80) | 22777916 | 29265158 (sdcc) |
| fixedbench | 4008 | 4154 (sccz80) | 19564886 | 34371657 (sdcc) |
| bitfieldbench | 3984 | 4115 (sccz80) | 26270970 | 42828210 (80cc_fp) |
| vecbench | 4971 | 5009 (sccz80) | 14142180 | 17587321 (sdcc) |
| matrixbench | 10276 | 10317 (sccz80) | 28260276 | 29861870 (80cc_sp) |
| interpbench | 3809 | 3867 (sccz80) | 28134199 | 31195663 (sdcc) |
| structbench | 4723 | 4816 (sccz80) | 2961312 | 3246073 (80cc_fp) |
| recordbench | 3604 | 3667 (sccz80) | 6934066 | 7571325 (80cc_fp) |
| listbench | 6762 | 6836 (sccz80) | 29269475 | 33941702 (80cc_fp) |
| lexbench | 4449 | 4641 (sccz80) | 38399932 | 42506136 (80cc_fp) |
| maskbench | 4656 | 4790 (sccz80) | 22737508 | 24804236 (80cc_fp) |

Same-input V5→V6 differences:

- bitfieldbench xcc_Os: +24 bytes, +1989260 ticks.

Raw partition artifacts, binaries, build/run logs, source manifests, versions and explicit merge provenance remain beside this report.
