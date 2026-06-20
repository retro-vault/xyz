# TOFIX

Date: 2026-06-19

This file captures the full-suite run state so the next pass can continue
without rediscovering the failures.

## Current Full-Suite Sweep

Started from a fresh `make -j$(nproc) xtools`, which completed.

Logs are under:

```sh
build/test-all-logs/
```

Command runner summary so far:

```text
PASS lib_xz80_test        make -C lib/xz80 test
PASS lib_xgdb_test        make -C lib/xgdb test
PASS xgdb_vsix_check      make -C tools/xgdb-vsix check
PASS xobjcopy_test        make -C src/xc/xobjcopy test
PASS xcc_test             make -C src/xc/xcc test
PASS xcc_check            make -C src/xc/xcc check
FAIL xcc_external         bash src/xc/xcc/tests/run_external_tests.sh bin/x/bin/xcc
RUNNING xcc_exec          bash src/xc/xcc/tests/run_exec_tests.sh bin/x/bin/xcc
```

At the time this file was written, `xcc_exec` was still running and had
already emitted 112 passing sub-runs. Continue watching:

```sh
tail -f build/test-all-logs/xcc_exec.log
cat build/test-all-logs/summary.txt
```

## External Acceptance Failures

The external suite result was:

```text
External results: 264 passed, 5 failed
```

Rerun:

```sh
bash src/xc/xcc/tests/run_external_tests.sh bin/x/bin/xcc
```

### 1. Old-Style Function Prototype Handling

Failures:

```text
src/xc/xcc/tests/data/external/tsuite/00209.c
src/xc/xcc/tests/data/external/drs/dr206.c
```

Observed diagnostics:

```text
wrong number of arguments to function call
```

Likely issue:

The frontend is treating non-prototype function declarations such as:

```c
void dr206_unprototyped();
typedef int (*fptr1)();
```

as fixed zero-argument prototypes. In C, an empty parameter list in a function
declaration is an old-style non-prototype declaration and calls may supply
arguments with default promotions. This probably needs a distinct function type
state for "prototype known" vs "old-style no prototype".

Attack points:

```text
src/xc/xcc/include/frontend/types.h
src/xc/xcc/src/frontend/parser_declarator.cpp
src/xc/xcc/src/frontend/sema.cpp
src/xc/xcc/src/ir/irgen_expr.cpp
```

### 2. VLA Feature Macro / VLA Support

Failure:

```text
src/xc/xcc/tests/data/external/C11/n1460.c
```

Observed diagnostic:

```text
#error "it's unexpected that we don't support VLAs"
```

Likely issue:

`__STDC_NO_VLA__` is defined, and this external test expects mainstream C11
behavior. To pass honestly, add enough VLA support for:

```c
void func(int n, int m[n]) {
    int array[n];
}
```

If full VLA support is too large for the next pass, do not simply hide the
macro unless parsing and lowering this case works.

Attack points:

```text
src/xc/xcc/src/frontend/preproc.cpp
src/xc/xcc/src/frontend/parser_declarator.cpp
src/xc/xcc/src/frontend/sema.cpp
src/xc/xcc/src/ir/irgen_decl.cpp
```

### 3. `_Generic` Type Matching Edge Case

Failure:

```text
src/xc/xcc/tests/data/external/drs/dr209.c
```

Observed diagnostics:

```text
_Generic: no matching association
```

Likely issue:

The generic selection type comparison is missing one of the DR209-compatible
normalization rules. Inspect the failing lines first:

```sh
sed -n '70,100p' src/xc/xcc/tests/data/external/drs/dr209.c
```

Attack points:

```text
src/xc/xcc/src/frontend/parser_expr.cpp
src/xc/xcc/src/frontend/types.cpp
src/xc/xcc/src/frontend/sema.cpp
```

### 4. `__STDC_VERSION__` Macro Value

Failure:

```text
src/xc/xcc/tests/data/external/drs/dr411.c
```

Observed diagnostic:

```text
static assertion failed
```

Likely issue:

The runner does not pass `-std=...`, so xcc's default `__STDC_VERSION__`
probably does not match the C2X/C23 expected value in this imported test.
Decide whether the default language mode should advertise C23:

```text
__STDC_VERSION__ == 202311L
```

or whether the external runner should supply a standard mode per test. Since xcc
aims at C23 by default, prefer fixing the predefined macro if that is currently
wrong.

Attack points:

```text
src/xc/xcc/src/frontend/preproc.cpp
src/xc/xcc/src/driver/options.cpp
```

## Continue The Sweep

If the current long runner is still active, let it finish. Otherwise continue
with the remaining commands from the full sweep:

```sh
make -C src/xc/xopt test
make -C src/xc/xas test
make -C src/xc/xas test-libs
make -C src/xc/xld test
make -C tests/libc test
make -C tests/hello-cpm test
make -C tests/runtime test
make -C tests/mdr-emu test
make -C tests/c23 matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json RUN_MODE=never
COMPILE_TIMEOUT=30s CYCLES=200000000 tests/corpus/c23-projects/run.sh
bash tests/e2e_test.sh --no-build
```

After fixes, rerun at minimum:

```sh
make -C src/xc/xcc test
bash src/xc/xcc/tests/run_external_tests.sh bin/x/bin/xcc
bash src/xc/xcc/tests/run_exec_tests.sh bin/x/bin/xcc
```
