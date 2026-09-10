#!/usr/bin/env bash
# branch_addend_test.sh
#
# Relocation addends through the real xas -> xld chain.
#
# ASxxxx keeps a branch addend in the displacement byte and measures the
# branch from the end of the field; GNU keeps it in the relocation record and
# measures from the field itself, filling the data with a branch-to-self
# placeholder. Getting either wrong sends the branch somewhere else, so every
# case here checks the address the branch actually lands on.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
BIN="$REPO_ROOT/bin/x/bin"
XLD="${XLD:-$BIN/xld}"
XAS="${XAS:-$BIN/xas}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

for tool in "$XLD" "$XAS"; do
    if [[ ! -x "$tool" ]]; then
        echo "${YELLOW}SKIP${RESET}: $(basename "$tool") not built"
        exit 0
    fi
done

tmpdir=$(mktemp -d /tmp/xld_addend_XXXXXX)
trap 'rm -rf "$tmpdir"' EXIT

# far_target sits at 0x0012: _CODE2 is based at 0x0010 and opens with 2 nops.
cat > "$tmpdir/a.s" <<'EOF'
        .module a
        .globl _start
        .globl far_target
        .area _CODE
_start:
        jr far_target           ; 0x0000
        jr far_target-2         ; 0x0002
        jr far_target+3         ; 0x0004
        ret
EOF

cat > "$tmpdir/b.s" <<'EOF'
        .module b
        .globl far_target
        .area _CODE2
        nop
        nop
far_target:
        ret
        nop
        nop
        ret
EOF

# landing_address <file> <branch offset> -> address the JR jumps to
landing_address() {
    python3 - "$1" "$2" <<'PY'
import sys
image = open(sys.argv[1], "rb").read()
at = int(sys.argv[2], 0)
disp = int.from_bytes(image[at + 1:at + 2], "little", signed=True)
print("0x%04X" % (at + 2 + disp))
PY
}

check_landings() {
    local image="$1" mode="$2"
    local got want
    for probe in "0x0000 0x0012" "0x0002 0x0010" "0x0004 0x0015"; do
        set -- $probe
        got=$(landing_address "$image" "$1")
        want=$2
        [[ "$got" == "$want" ]] \
            || fail "$mode: branch at $1 lands on $got, expected $want"
    done
}

# --- ASxxxx / SDCC objects ------------------------------------------------
"$XAS" --mode=sdcc "$tmpdir/a.s" -o "$tmpdir/a.rel"
"$XAS" --mode=sdcc "$tmpdir/b.s" -o "$tmpdir/b.rel"
"$XLD" --mode=sdcc -nostdlib -f bin -e _start \
    -b _CODE=0000 -b _CODE2=0010 -x 0000-0017 \
    -o "$tmpdir/sdcc.bin" "$tmpdir/a.rel" "$tmpdir/b.rel" \
    || fail "sdcc: link failed"
check_landings "$tmpdir/sdcc.bin" "sdcc"

# --- GNU / ELF objects ----------------------------------------------------
"$XAS" --mode=gnu "$tmpdir/a.s" -o "$tmpdir/a.o"
"$XAS" --mode=gnu "$tmpdir/b.s" -o "$tmpdir/b.o"
"$XLD" --mode=gnu -nostdlib -f bin -e _start \
    -b _CODE=0000 -b _CODE2=0010 -x 0000-0017 \
    -o "$tmpdir/gnu.bin" "$tmpdir/a.o" "$tmpdir/b.o" \
    || fail "gnu: link failed"
check_landings "$tmpdir/gnu.bin" "gnu"

echo "${GREEN}ok${RESET}: branch addends (sdcc and gnu, signed and positive)"
