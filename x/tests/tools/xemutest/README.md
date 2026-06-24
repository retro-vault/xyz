# xemutest

`xemutest` is the unified `x` test runner built on `xemu`.

It is the canonical manifest runner for:

- compile-only tests that just need to pass or fail compilation
- executable tests that build a flat `--platform=emu` binary
- emulator-backed checks on stdout, exit status, registers, memory, and debug-visible variables
- host-gcc golden generation for expected stdout / exit results
- optimization and float-format matrix expansion from one manifest

Current layout:

- tool source: [x/tests/tools/xemutest/src/main.cpp](/home/tstih/data/retro-vault/xyz/x/tests/tools/xemutest/src/main.cpp)
- unified suite root: [x/tests/tests/c23](/home/tstih/data/retro-vault/xyz/x/tests/tests/c23)
- convenience wrapper: [x/tests/run_tests.sh](/home/tstih/data/retro-vault/xyz/x/tests/run_tests.sh)

## Manifest

Each active unified test lives under `x/tests/tests/c23/cases/` in its own
directory with a `test.cfg`.

Supported keys:

- `id = c23_0001_stdio_echo`
- `runner = xemu` or `runner = command`
- `component = xcc`
- `summary = short human-readable description`
- `alias = xcc`
- `legacy_path = x/src/xcc/tests`
- `kind = compile` or `kind = run`
- `source = main.c`
- `compiler_arg = -O2`
- `matrix_opt = O1`
- `matrix_float = ieee32`
- `host_golden = gcc`
- `host_arg = -std=c2x`
- `timeout_seconds = 30`
- `command = bash`
- `command_arg = run_all.sh`
- `workdir = .`
- `stdin = stdin.txt`
- `stdout = stdout.txt`
- `expect_exit = 0`
- `stderr_contains = some text`
- `stderr_not_contains = some text`
- `assert_reg = hl=0x1234`
- `assert_mem = 0xff02: 0xa5`
- `assert_var = sum:int=42`

For run tests, the default console wiring matches the current `emu` platform:

- stdin status port: `0xe2`
- stdin data port: `0xe3`
- stdout port: `0xe1`

Use one `compiler_arg = ...` line per compiler/linker argument.
`xemutest --list` shows the fully expanded runnable variants.

## Build

```sh
make -C x/tests/tools/xemutest all
make -C x/tests/tools/xemutest test
```

## Run

```sh
XCC=bin/x/bin/xcc bash x/tests/run_tests.sh
```
