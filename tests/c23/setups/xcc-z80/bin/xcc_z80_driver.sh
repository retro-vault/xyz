#!/usr/bin/env bash
#
# xcc_z80_driver.sh
#
# Thin driver for the C23 suite's xcc-z80 profile. It compiles and links one
# suite case into an Intel HEX image using the staged X toolchain.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../../.." && pwd)"

compiler="${repo_root}/bin/x/bin/xcc"
source_path=""
output_path=""
extra_flags=()

usage() {
    cat >&2 <<'EOF'
usage: xcc_z80_driver.sh [--compiler PATH] --source FILE --output FILE -- [xcc flags...]
EOF
    exit 2
}

while (($#)); do
    case "$1" in
        --compiler)
            (($# >= 2)) || usage
            compiler="$2"
            shift 2
            ;;
        --source)
            (($# >= 2)) || usage
            source_path="$2"
            shift 2
            ;;
        --output)
            (($# >= 2)) || usage
            output_path="$2"
            shift 2
            ;;
        --)
            shift
            extra_flags=("$@")
            break
            ;;
        *)
            usage
            ;;
    esac
done

[[ -n "${source_path}" && -n "${output_path}" ]] || usage
[[ -x "${compiler}" ]] || {
    echo "xcc_z80_driver.sh: compiler not executable: ${compiler}" >&2
    exit 1
}
[[ -f "${source_path}" ]] || {
    echo "xcc_z80_driver.sh: source file not found: ${source_path}" >&2
    exit 1
}

mkdir -p "$(dirname "${output_path}")"

"${compiler}" \
    --platform=none \
    --oformat=ihx \
    "${extra_flags[@]}" \
    "${source_path}" \
    -o "${output_path}"

[[ -f "${output_path}" ]] || {
    echo "xcc_z80_driver.sh: expected output not produced: ${output_path}" >&2
    exit 1
}
