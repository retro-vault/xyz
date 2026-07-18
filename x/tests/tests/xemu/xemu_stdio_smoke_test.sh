#!/usr/bin/env bash
#
# xemu_stdio_smoke_test.sh
#
# Verify that xemu can run a tiny program directly, map stdin/stdout through
# Z80 I/O ports, and return successfully on HALT.
#
set -euo pipefail

XEMU="$(realpath "${1:?usage: xemu_stdio_smoke_test.sh /path/to/xemu}")"
TOOL_BIN_DIR="$(cd "$(dirname "$XEMU")" && pwd)"
XAS="${XAS:-$TOOL_BIN_DIR/xas}"
XLD="${XLD:-$TOOL_BIN_DIR/xld}"

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

bank_program="$tmpdir/banked.bin"
printf '\x3E\x41\x32\x00\x80\x3E\x01\xD3\x10\x3E\x42\x32\x00\x80\xAF\xD3\x10\x3A\x00\x80\xD3\x01\x3E\x01\xD3\x10\x3A\x00\x80\xD3\x01\x76' > "$bank_program"

cat > "$tmpdir/xemu.conf" <<'EOF'
stdout_port = 1
store.low.size = 0x8000
store.banked.banks = 2
store.banked.size = 0x4000
store.high.size = 0x4000

selector.bank = 0

window.low.range = 0x0000-0x7fff
window.low.store = low
window.bank.range = 0x8000-0xbfff
window.bank.store = banked
window.bank.selector = bank
window.high.range = 0xc000-0xffff
window.high.store = high

port_rule.bank.port = 0x10
port_rule.bank.selector = bank
EOF

bank_output="$(
    cd "$tmpdir"
    "$XEMU" \
        --run \
        --quiet \
        --load-bin "$(basename "$bank_program")" \
        --origin 0x0000 \
        --pc 0x0000
)"

[[ "$bank_output" == "AB" ]] || fail "expected banked stdout 'AB', got '$bank_output'"

if [[ -x "$XAS" && -x "$XLD" ]]; then
    cat > "$tmpdir/halt_elf.s" <<'EOF'
        .globl _start
        .text
_start:
        .byte 0x76
EOF

    "$XAS" --mode=gnu -g -o "$tmpdir/halt_elf.o" "$tmpdir/halt_elf.s"
    "$XLD" --mode=gnu --oformat=elf -g -e _start -Ttext=0100 \
        -o "$tmpdir/halt_elf.elf" "$tmpdir/halt_elf.o"

    "$XEMU" --run --quiet --load-elf "$tmpdir/halt_elf.elf"
fi

echo "${GREEN}PASS${RESET}: xemu stdio smoke"
