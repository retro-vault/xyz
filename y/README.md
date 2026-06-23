# y

First-pass product copy for YOS.

## Layout

- `src/` — the copied YOS source tree
- `include/` — public headers used by YOS and Y-adjacent tooling
- `pkg/` — copied YOS-adjacent distribution tools (`appmake`, `microdrive`, `serial`)
- `tests/` — copied YOS-oriented samples and media/emulator harnesses
- `docs/` — copied YOS documentation

## Notes

- This is an additive migration step. The legacy top-level layout still exists for now.
- The copied tree is intended to make ownership clearer before the build is fully rewired around `y/`.
