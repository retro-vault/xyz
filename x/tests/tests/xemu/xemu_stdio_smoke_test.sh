#!/usr/bin/env bash
#
# xemu_stdio_smoke_test.sh
#
# Verify that xemu can run a tiny program directly, map stdin/stdout through
# Z80 I/O ports, and return successfully on HALT.
#
set -euo pipefail

XEMU="${1:?usage: xemu_stdio_smoke_test.sh /path/to/xemu}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

tmpdir="$(mktemp -d /tmp/xemu_stdio_XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

program="$tmpdir/echo.bin"
printf '\xDB\x00\xD3\x01\x76' > "$program"

output="$(
    printf 'Q' | "$XEMU" \
        --run \
        --quiet \
        --load-bin "$program" \
        --origin 0x0000 \
        --pc 0x0000 \
        --stdin-port 0 \
        --stdout-port 1
)"

[[ "$output" == "Q" ]] || fail "expected stdout 'Q', got '$output'"

emu_program="$tmpdir/emu_stdio.bin"
printf '\xDB\xE2\xB7\x28\xFB\xDB\xE3\xD3\xE1\x76' > "$emu_program"

emu_output="$(
    printf 'R' | "$XEMU" \
        --run \
        --quiet \
        --load-bin "$emu_program" \
        --origin 0x0000 \
        --pc 0x0000 \
        --emu-stdio
)"

[[ "$emu_output" == "R" ]] || fail "expected platform=emu stdout 'R', got '$emu_output'"

echo "${GREEN}PASS${RESET}: xemu stdio smoke"
