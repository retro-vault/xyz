# Fresh 2026-09-09 z88dk24 comparison

All lanes share the fresh nightly headers, +test CRT and classic library. The 24 original workload sources and correctness checks are unchanged. sccz80 and 80cc are built from the downloaded nightly archive matching master 895dc1366573b792d98b0607fb1048d2db248b3d. The latest 80cc source change is e56f2c22b321e55a852c3c159d9cbfdaa3533bac. The nightly bundles zsdcc revision16639; the separate official SDCC trunk lane is revision16858 with the retained z88dk ABI compatibility patch.

Each cell reports complete linked bytes and measured Z80 T-states. Invalid results remain visible and are excluded from winning comparisons. Both expensive max-allocation probes retain the original six-workload subset.

| XCC profile | Correct / attempted | Smaller than best valid 80cc | Faster than best valid 80cc | Smaller than every valid competitor | Faster than every valid competitor |
|---|---:|---:|---:|---:|---:|
| xcc_Os | 24/24 | 24/24 | 11/24 | 24/24 | 11/24 |
| xcc_Of | 24/24 | 23/24 | 24/24 | 11/24 | 24/24 |

| Benchmark | nightly sccz80 | XCC -Os | XCC -Of | official SDCC | official SDCC max | nightly 80cc FP | nightly 80cc SP | nightly zsdcc | nightly zsdcc max |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| charbench | 5261 B / 171398664 T | 4644 B / 26485104 T | 4722 B / 26375916 T | 5414 B / 32470244 T | 5354 B / 26519054 T | 5082 B / 29926524 T | 5110 B / 28187527 T | 5414 B / 32470244 T | 5354 B / 26519054 T |
| crcbench | 5167 B / 231849299 T | 4820 B / 112513604 T | 5023 B / 86035675 T | 5812 B / 138570854 T | 5768 B / 140597295 T | 5387 B / 114635406 T | 5463 B / 121411823 T | 5812 B / 138570854 T | 5768 B / 140597295 T |
| intbench | 5187 B / 127390985 T | 4861 B / 43072474 T | 4988 B / 32960132 T | 5693 B / 50670703 T | 5623 B / 47168997 T | 5245 B / 33594542 T | 5332 B / 34325768 T | 5693 B / 50670703 T | 5623 B / 47168997 T |
| ptrbench | 7241 B / 46963202 T | 6764 B / 13424564 T | 7042 B / 11267197 T | 7699 B / 18005556 T | 7637 B / 15548249 T | 7679 B / 12027589 T | 7897 B / 14841161 T | 7699 B / 18005556 T | 7637 B / 15548249 T |
| md5 | 15013 B / 42814023 T | 11550 B / 29678619 T | 18525 B / 21689787 T | 29321 B / 51135306 T | 19760 B / 31577670 T | 17569 B / 22478882 T | 19031 B / 24414626 T | 29321 B / 51135306 T | 19760 B / 31577670 T |
| sieve | 11472 B / 9582895 T | 11439 B / 5262662 T | 11527 B / 3757371 T | 12234 B / 5438799 T | 12189 B / 5872808 T | 11742 B / 4494261 T | 11823 B / 4379718 T | 12234 B / 5438799 T | 12189 B / 5872808 T |
| rle | 7013 B / 40461580 T | 6802 B / 12577553 T | 6906 B / 12068928 T | 7514 B / 13779245 T | — | 7153 B / 13688642 T | 7228 B / 13200276 T | 7514 B / 13779245 T | — |
| sortbench | 5129 B / 58978701 T | 5044 B / 41076592 T | 5220 B / 28208692 T | 5880 B / 36803808 T | — | 5492 B / 28580132 T | 5730 B / 29740803 T | 5880 B / 36803808 T | — |
| queenbench | 3670 B / 60295130 T | 3612 B / 21041821 T | 3685 B / 18455856 T | 4383 B / 22770315 T | — | 3975 B / 25222306 T | 4034 B / 27896090 T | 4383 B / 22770315 T | — |
| searchbench | 4895 B / 69359211 T | 4759 B / 22054149 T | 4837 B / 21763678 T | 5483 B / 22180122 T | — | 5111 B / 22062540 T | 5147 B / 26731153 T | 5483 B / 22180122 T | — |
| switchbench | 4318 B / 37639841 T | 4242 B / 46680092 T | 4584 B / 31007401 T | 5757 B / 63123983 T | — | 4740 B / 32686606 T | 4777 B / 31344825 T | 5757 B / 63123983 T | — |
| hashbench | 8091 B / 61450366 T | 8032 B / 45145049 T | 8177 B / 24031267 T | 8784 B / 35626638 T | — | 8421 B / 38482129 T | 8480 B / 38215339 T | 8784 B / 35626638 T | — |
| strbench | 6024 B / 54116977 T | 5882 B / 23664363 T | 5955 B / 18012089 T | 6621 B / 18386361 T | — | 6293 B / 22358179 T | 6455 B / 28273640 T | 6621 B / 18386361 T | — |
| histbench | 3773 B / 86484812 T | 3735 B / 64577891 T | 3817 B / 22777916 T | 4519 B / 29265158 T | — | 4143 B / 32052250 T | 4184 B / 32465762 T | 4519 B / 29265158 T | — |
| fixedbench | 4154 B / 43145270 T | 4008 B / 31114937 T | 4223 B / 19564886 T | 4744 B / 34371657 T | — | 4406 B / 38067934 T | 4447 B / 36858542 T | 4744 B / 34371657 T | — |
| bitfieldbench | 4115 B / 57795714 T | 3984 B / 31667173 T | 4093 B / 26270970 T | 4706 B / 24645240 T FAIL | — | 4634 B / 42828210 T | 4709 B / 43329992 T | 4706 B / 24645240 T FAIL | — |
| vecbench | 5009 B / 25810894 T | 4971 B / 12059144 T | 5155 B / 14142180 T | 5766 B / 17587321 T | — | 5402 B / 19557629 T | 5470 B / 19111822 T | 5766 B / 17587321 T | — |
| matrixbench | 10317 B / 68146122 T | 10276 B / 44328525 T | 10546 B / 28260276 T | 10999 B / 33470970 T | — | 10727 B / 31725241 T | 10852 B / 29861870 T | 10999 B / 33470970 T | — |
| interpbench | 3867 B / 46652580 T | 3809 B / 40965725 T | 3966 B / 28134199 T | 4550 B / 31195663 T | — | 4312 B / 37719531 T | 4405 B / 38703994 T | 4550 B / 31195663 T | — |
| structbench | 4816 B / 11732787 T | 4723 B / 2965621 T | 4815 B / 2961312 T | 5514 B / 3745784 T | — | 5116 B / 3246073 T | 5200 B / 3606025 T | 5514 B / 3745784 T | — |
| recordbench | 3667 B / 15463407 T | 3604 B / 8696571 T | 3672 B / 6934066 T | 4512 B / 10361514 T | — | 3954 B / 7571325 T | 4033 B / 7977758 T | 4512 B / 10361514 T | — |
| listbench | 6836 B / 92868049 T | 6762 B / 38083585 T | 6876 B / 29269475 T | 7482 B / 42792501 T | — | 7137 B / 33941702 T | 7235 B / 43947005 T | 7482 B / 42792501 T | — |
| lexbench | 4641 B / 73236220 T | 4449 B / 39964001 T | 4630 B / 38399932 T | 5267 B / 67252186 T | — | 4794 B / 42506136 T | 4876 B / 43576437 T | 5267 B / 67252186 T | — |
| maskbench | 4790 B / 75920827 T | 4656 B / 24527212 T | 4736 B / 22737508 T | 5368 B / 25018994 T | — | 5004 B / 24804236 T | 5072 B / 31763880 T | 5368 B / 25018994 T | — |

[Raw CSV](latest-2026-09/results.csv), [compiler versions](latest-2026-09/versions.txt), [source and toolchain provenance](latest-2026-09/README.md).

These are complete linked images with a common fresh z88dk target library. Native XCC libc/runtime improvements are measured separately in [the campaign report](../../../docs/xcc/SCCZ80-OPTIMIZATION-CAMPAIGN-2026-09.md).
