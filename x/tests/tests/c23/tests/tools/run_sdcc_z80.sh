#!/usr/bin/env bash
#
# Template runner for an SDCC Z80 profile.
# Adapt this script to convert the compiler output into the format your
# emulator expects, then execute the emulator and forward only the target
# program's stdout/stderr.
#

set -eu

artifact_path="${1:-}"
build_dir="${2:-}"

printf '%s\n' \
    "run_sdcc_z80.sh is a template." \
    "Edit tests/tools/run_sdcc_z80.sh to pack ${artifact_path} and run it in your emulator." \
    "Suggested inputs: artifact=${artifact_path} build_dir=${build_dir}" >&2
exit 2
