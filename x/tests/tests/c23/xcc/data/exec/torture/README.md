# z88dk torture sources

The files under `upstream/` were imported from
<https://github.com/z88dk/z88dk> at commit
`dc0e6d322140b3b2dc484181e9f5ed80f1c2193c` (2026-07-13).

- `examples/console/umchess.c`, git blob
  `47bb1d1f916cfd499e8de44222f4564a9f950a33`
- `libsrc/regex/cimpl/regexp.c`, git blob
  `8262e85aeac78dab3ea4f83c2313385496fe1eea`
- `libsrc/regex/cimpl/regmagic.h`, git blob
  `df1b1cfaef03a4ef8e3c300d066c23b5627035b5`

The C implementation bodies are not specialized for XCC. The micro-Max shim
only makes its infinite interactive `main` an unused private inline function,
and the test supplies an integer-equivalent board initialization followed by
one deterministic engine search. The regexp
test supplies a portable declaration header and `regerror`, then compiles and
executes the original implementation. Terminal empty-line normalization does
not change the imported C tokens.

`regexp.c` retains Henry Spencer's permission notice. The z88dk distribution
license applicable to `umchess.c` is reproduced as `upstream/Z88DK-LICENSE`.
