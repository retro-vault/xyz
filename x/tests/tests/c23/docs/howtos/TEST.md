# Testing A New Compiler

This guide explains how to run the C23 suite against a new compiler, including
cross compilers that need non-default switches or an emulator.

## Quick Start

Generate the test tree and run the matrix with a native compiler:

```sh
make generate
make matrix CC=gcc
make matrix CC=clang
```

Run through a compiler profile:

```sh
make matrix PROFILE=tests/config/profiles/gcc.json
```

Run the bundled SDCC Z80 setup:

```sh
make matrix-sdcc-z80
```

Compile only, without executing runnable tests:

```sh
make matrix PROFILE=tests/config/profiles/gcc.json RUN_MODE=never
```

Outputs:

- `docs/dist/c23-compatibility-matrix.md`
- `build/results/<compiler-label>.json`
- `build/results/<compiler-label>/...` per-feature artifacts

## Native vs Cross

There are two supported ways to onboard a compiler:

1. Native compiler
   Use `CC=<compiler>` when the compiler behaves like a hosted toolchain and
   can emit host executables that run directly on the current machine.
2. Compiler profile
   Use a JSON profile when the compiler needs a custom language switch, custom
   argument ordering, a non-default output artifact, or an emulator/wrapper for
   execution.

In practice:

- `CC=gcc` is enough for GCC-like host compilers.
- A profile is the right tool for SDCC, bare-metal cross compilers, emulator
  pipelines, and anything with unusual command-line conventions.

## Profile Files

Profiles live under `tests/config/profiles/`.

Included examples:

- `tests/config/profiles/gcc.json`
- `tests/config/profiles/example-cross-sdcc-z80.json`
- `setups/sdcc-z80/profile-ucsim.json`

The SDCC example is intentionally a template. It points at
`tests/tools/run_sdcc_z80.sh`, which you should edit for your emulator flow.
The setup in `setups/sdcc-z80/` is the working Z80-specific bundle used by
this repository.

## Profile Schema

A profile is a JSON object.

Minimal profile:

```json
{
  "compiler": "gcc",
  "label": "gcc"
}
```

Cross-compiler template:

```json
{
  "compiler": "sdcc",
  "label": "sdcc-z80",
  "std_flag": "",
  "artifact_name": "case.ihx",
  "compile_flags": ["-mz80"],
  "compile_template": [
    "{compiler}",
    "{profile_flags}",
    "{include_flags}",
    "{extra_cflags}",
    "-o",
    "{executable}",
    "{source}"
  ],
  "run_template": [
    "{root}/tests/tools/run_sdcc_z80.sh",
    "{executable}",
    "{build_dir}"
  ]
}
```

## Profile Fields

- `compiler`
  Compiler executable name or full path.
- `label`
  Optional stable result label. If omitted, the runner derives one from the
  compiler version output.
- `std_flag`
  Exact language-mode switch to use. Set this to `""` when the compiler has no
  C23 switch or you do not want the runner to probe for one.
- `detect_std_flags`
  Optional list of candidate language-mode switches. Use this when auto-detect
  should try something other than the built-in `-std=c23`, `-std=c2x`,
  `-std=gnu2x`.
- `artifact_name`
  Output artifact name. Native compilers usually keep the default `case.exe`.
  Cross compilers may want `case.ihx`, `case.hex`, `case.bin`, or similar.
- `compile_flags`
  Extra compiler-wide flags that apply to every feature.
- `common_flags`
  Replacement for the default warnings list. Leave this alone unless the
  compiler rejects GCC-style warnings.
- `compile_template`
  Full compile command template.
- `run_template`
  Full execution command template. This can be an emulator command or a wrapper
  script.
- `version_command`
  Optional command used to derive the compiler label when `label` is not set.
- `run_success_returncodes`
  Optional list of acceptable run return codes. Default: `[0]`.

## Template Placeholders

These placeholders can be used inside `compile_template`,
`run_template`, and `version_command`.

Single-value placeholders:

- `{compiler}`
- `{std_flag}`
- `{source}`
- `{executable}`
- `{case_dir}`
- `{build_dir}`
- `{tests_data_dir}`
- `{root}`

List placeholders:

- `{common_flags}`
- `{profile_flags}`
- `{extra_cflags}`
- `{include_flags}`

Important:

- List placeholders should be used as standalone array elements.
- Single-value placeholders can be standalone or embedded into a larger string.
- If `std_flag` is empty, it disappears from the command.

## Compile-Only Runs

If you do not yet have a runner or emulator, you can still survey compile
support:

```sh
make matrix PROFILE=tests/config/profiles/example-cross-sdcc-z80.json RUN_MODE=never
```

In this mode:

- `compile` tests still check that compilation succeeds.
- `negative-compile` tests still check that compilation fails.
- `run` tests compile, but are reported as `NOT-RUN`.

This is useful for early bring-up of a new toolchain.

## Using An Emulator Or Wrapper Script

For most cross compilers, the cleanest setup is:

1. Compile to the target artifact.
2. Call a project-local wrapper script from `run_template`.
3. Let that wrapper convert, package, upload, or emulate as needed.
4. Make the wrapper forward the target program's stdout and stderr.
5. Make the wrapper exit with `0` when the target program passed.

This keeps the runner generic and keeps target-specific logic out of the core
Python tool.

Example runner contract:

- Argument 1: emitted artifact path such as `case.ihx`
- Argument 2: feature build directory
- Output: target program stdout, ideally only the suite's `OK ...` line
- Exit code: `0` on success

The included `tests/tools/run_sdcc_z80.sh` is a template stub you can replace
with your emulator flow.

## Recommended Bring-Up Process

1. Start with `RUN_MODE=never` and confirm compile behavior.
2. Set `std_flag` or `detect_std_flags` correctly for the compiler.
3. Adjust `compile_template` until artifacts land in `build/results/...`.
4. Add a wrapper script or emulator command in `run_template`.
5. Re-run in normal mode and inspect the JSON report.
6. Only then compare the published matrix against other compilers.

## Result Statuses

- `PASS`: compile, and when applicable, run behavior matched the expectation.
- `FAIL`: compile or run behavior did not match the expectation.
- `NOT-CLAIMED`: optional IEC 60559 feature was not claimed by the compiler.
- `NOT-RUN`: compile succeeded, but execution was intentionally skipped or no
  runner was configured.

## Notes

- The current suite uses exact stdout matching for runnable tests. If your
  emulator prints banners or prompts, filter them out in the wrapper script.
- Per-feature special flags still come from the suite catalog, so the profile
  should describe compiler-wide behavior, not feature-specific hacks.
- For unusual targets, it is usually better to keep the profile simple and put
  complex orchestration into a shell script or helper program.
