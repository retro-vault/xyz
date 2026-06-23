# C Project Corpus For `xcc --platform=emu`

This directory contains shallow upstream checkouts used as a real-world C
compile-and-run corpus for xcc.  The upstream trees are intentionally left
unchanged; local Z80 harnesses live under `tests/corpus/c23-projects/`.

## Policy

The active corpus is for practical compiler coverage, not miracles.  A project
is active only when the runner compiles its real upstream source or a genuine
header-only implementation and the resulting Z80 binary fits the emulator smoke
test envelope.  If a project is too large, requires major unsupported hosted
features, or only passes through an API-only adapter, remove it from the active
runner and replace it with a smaller project later.

## Green Corpus

`tests/corpus/c23-projects/run.sh` builds and runs these projects with
`xcc --platform=emu` and the Z80 emulator test layer.  The runner also contains
one local `emu-fs` sanity case, so the current green run is 22 compile-and-run
cases total.

| Project | Upstream | Revision | Why it is useful |
| --- | --- | --- | --- |
| argparse | https://github.com/cofyc/argparse | 4e30aba | Option parser; aggregate initializers, callback-style option tables, and argv walking. |
| base64.c | https://github.com/joedf/base64.c | 7896e28 | C encoder path; BOM header handling, global string-array tables, and byte packing. |
| branchless-utf8 | https://github.com/skeeto/branchless-utf8 | e4d82fd | Header-only UTF-8 decoder; static tables and 32-bit byte classification. |
| cwalk | https://github.com/likle/cwalk | e98d23f | Path manipulation library; out-parameters and pointer-heavy string slices. |
| inih | https://github.com/benhoyt/inih | 577ae2d | INI file parser; hosted file I/O and callback dispatch. |
| jsmn | https://github.com/zserge/jsmn | 25647e6 | Header-only JSON tokenizer; pointer-heavy parser state and token arrays. |
| map | https://github.com/rxi/map | d6c355f | Hash map implementation; heap allocation, string keys, and chained buckets. |
| optparse | https://github.com/skeeto/optparse | a86877e | Reentrant option parser; long-option tables and pointer-heavy argv walking. |
| rxi/ini | https://github.com/rxi/ini | 13a254c | Heap-backed INI parser; hosted file I/O, `fseek`/`ftell`, and forward struct typedefs. |
| tiny-json | https://github.com/rafagafe/tiny-json | 025cdde | JSON parser; token walking and string value extraction. |
| tiny-regex-c | https://github.com/kokke/tiny-regex-c | f2632c6 | Small regex engine; recursive/state-machine string matching. |
| utf8.h | https://github.com/sheredom/utf8.h | 3821317 | Header-only UTF-8 helpers; pointer scanning and string comparison. |
| vec | https://github.com/rxi/vec | 20e8422 | Dynamic vector implementation; growth, insert, and remove behavior. |
| whereami | https://github.com/gpakosz/whereami | dcb52a0 | Platform-path smoke using the emulator environment shim. |
| uthash | https://github.com/troydhanson/uthash | 5ada598 | Header-only hash-table macros; null-table lookup and macro-heavy compile coverage. |
| klib | https://github.com/attractivechaos/klib | 97a0fcb | Header-only vector macros; heap-backed growth and indexed access. |
| nanoprintf | https://github.com/charlesnicholson/nanoprintf | a48cdb8 | Single-header printf core; formatter compile coverage and basic output path. |
| log.c | https://github.com/rxi/log.c | f9ea349 | Full logging implementation; exported API, static state, and `time`/stdio references. |
| picohttpparser | https://github.com/h2o/picohttpparser | f4d94b4 | Full HTTP parser source; octal character escapes, callback parser state, and string slices. |
| c-algorithms | https://github.com/fragglet/c-algorithms | 23d4537 | Queue implementation; opaque structs, heap nodes, and front/back accessors. |
| portable-snippets | https://github.com/nemequ/portable-snippets | 84abba9 | Header-only exact-width integer detection; preprocessor portability coverage. |

## Pruned Checkouts

Some downloaded projects remain in this upstream corpus tree for manual experiments,
but they are intentionally not in the active runner because their full source is
too large for the 64K binary smoke test, too slow for this corpus, or currently
requires frontend/runtime work.  Do not count adapter-only rows as corpus wins;
prefer deleting them from `run.sh` and adding smaller real projects.
