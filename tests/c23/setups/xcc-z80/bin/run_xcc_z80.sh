#!/usr/bin/env bash
#
# run_xcc_z80.sh
#
# Placeholder runner for the xcc-z80 C23 suite profile. It converts the Intel
# HEX payload into a flat binary so the build artifacts are ready for an
# emulator, then exits with a clear message because no emulator is wired here.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../../.." && pwd)"

artifact_path="${1:-}"
build_dir="${2:-}"
ihx2bin="${repo_root}/tests/runtime/tools/ihx2bin.py"

if [[ -z "${artifact_path}" || -z "${build_dir}" ]]; then
    echo "usage: run_xcc_z80.sh ARTIFACT_PATH BUILD_DIR" >&2
    exit 2
fi

if [[ ! -f "${artifact_path}" ]]; then
    echo "run_xcc_z80.sh: artifact not found: ${artifact_path}" >&2
    exit 1
fi

mkdir -p "${build_dir}"

if [[ -f "${ihx2bin}" ]]; then
    python3 "${ihx2bin}" "${artifact_path}" "${build_dir}/$(basename "${artifact_path%.ihx}").bin"
fi

cat >&2 <<EOF
run_xcc_z80.sh is a placeholder.
It converted ${artifact_path} to a flat binary in ${build_dir}, but this setup
does not yet launch an emulator and capture stdout for the C23 suite.
EOF
exit 2
