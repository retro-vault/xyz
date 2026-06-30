# z88dk Import Suite

This directory vendors the upstream z88dk `test/` and `testsuite/` trees under:

- `upstream/test/`
- `upstream/testsuite/`

The imported `test/suites/zx/` subtree is intentionally excluded from this
suite because it is ZX Spectrum target-specific rather than general xcc
compatibility coverage.

Aside from the intentional `test/suites/zx/` exclusion, the imported sources
are kept unchanged. All compatibility work for `xcc` lives outside the
vendored trees:

- `scripts/generate_z88dk_cases.py` probes the imported cases with our toolchain
- `scripts/verify_blocked_case.py` checks expected upstream-extension blockers
- `cases/` holds the generated `xemutest` manifests
- `generated/` holds auto-generated wrapper sources and metadata
- `docs/status.md` summarizes supported and blocked coverage

Upstream aggregate harness files such as `suites/*/main.c` are not imported as
standalone manifests. The generated suite runs the underlying test sources
directly and uses generated wrappers where needed.

## Usage

```sh
make -C x/tests/tests/z88dk generate
make -C x/tests/tests/z88dk test
```

To refresh the vendored upstream trees from a local checkout:

```sh
make -C x/tests/tests/z88dk sync-upstream UPSTREAM=/path/to/z88dk
make -C x/tests/tests/z88dk sync-upstream-git
```

The upstream reference used for this import is:

- https://github.com/z88dk/z88dk/tree/master
