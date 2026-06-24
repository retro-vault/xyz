# Archived Test Manifests

This tree holds legacy suite manifests and harness entrypoints that are no longer
part of the default `x/tests/run_tests.sh` discovery set.

Current intent:

- keep the active unified run focused on manifest-driven semantic/compiler cases
- retain older suite wrappers, smoke scripts, and codegen-only harnesses for reference
- make it obvious which coverage is still legacy work that can be migrated later

`xcc-legacy/` contains the old suite entry scripts that used to front the larger
compiler regression harness before per-case `xemutest` manifests were generated.
