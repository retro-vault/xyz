#!/usr/bin/env bash
# reserved_holes_test.sh
#
# End-to-end check of xld reserved address ranges (-r).
#
# Links one program across five reserved ranges -- two of which abut and must
# fuse -- then fills every reserved byte with RST 38 and runs the image.  Any
# guard that jumps short, any branch that was not promoted, and any byte the
# linker let slip into reserved space turns into a trap instead of output.
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
XEMU="${XEMU:-$BIN/xemu}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

for tool in "$XLD" "$XAS" "$XEMU"; do
    if [[ ! -x "$tool" ]]; then
        echo "${YELLOW}SKIP${RESET}: $(basename "$tool") not built"
        exit 0
    fi
done

tmpdir=$(mktemp -d /tmp/xld_holes_XXXXXX)
trap 'rm -rf "$tmpdir"' EXIT

# Reserved ranges.  The last two abut and must behave as one 0x140 byte range.
RESERVED=(0010-001F 0040-0140 0180-0280 02C0-03BF 03C0-03FF)

cat > "$tmpdir/multi.s" <<'EOF'
        .module multi

;; _CODE @ 0x0000 -- runs off the end into guard 1 (JR over a 16 byte range)
        .area _CODE
_start:
        ld a, #0x41             ; 'A'
        out (1), a

;; _C2 @ 0x0020 -- conditional forward branch over a 0x101 byte range
        .area _C2
        ld a, #0x42             ; 'B'
        out (1), a
        cp #0x42
        jr z, third             ; must be promoted to JP Z
        ld a, #0x21             ; '!' -- only reached if the branch is wrong
        out (1), a
        halt

;; _C3 @ 0x0150 -- loop body, runs off the end into guard 3
        .area _C3
third:
        ld a, #0x43             ; 'C'
        out (1), a
        ld b, #3
lp:
        ld a, #0x2E             ; '.'
        out (1), a

;; _C4 @ 0x0290 -- backward DJNZ over a 0x101 byte range
        .area _C4
        djnz lp                 ; must become a DJNZ/JR/JP trampoline
        ld a, #0x45             ; 'E'
        out (1), a
        jr mid                  ; no reserved range in between: stays a JR

;; _C4B @ 0x02A0 -- runs off the end into guard 4, over both fused ranges
        .area _C4B
mid:
        ld a, #0x46             ; 'F'
        out (1), a

;; _C5 @ 0x0400
        .area _C5
        ld a, #0x5A             ; 'Z'
        out (1), a
        halt
EOF

"$XAS" --mode=sdcc "$tmpdir/multi.s" -o "$tmpdir/multi.rel" \
    || fail "xas could not assemble the test program"

reserve_args=()
for range in "${RESERVED[@]}"; do
    reserve_args+=(-r "$range")
done

"$XLD" -nostdlib -f bin -e _start \
    "${reserve_args[@]}" \
    -x 0000-040F \
    -b _C2=0020 -b _C3=0150 -b _C4=0290 -b _C4B=02A0 -b _C5=0400 \
    -o "$tmpdir/multi.bin" "$tmpdir/multi.rel" \
    || fail "xld could not link across the reserved ranges"

# --- static checks --------------------------------------------------------
byte_at() {
    xxd -p -s "$1" -l "$2" "$tmpdir/multi.bin"
}

check_bytes() {
    local addr="$1" want="$2" what="$3" got
    got=$(byte_at "$addr" $(( ${#want} / 2 )))
    [[ "$got" == "$want" ]] || fail "$what at $(printf '0x%04X' "$addr"): expected $want, got $got"
}

# JR over 0x0010..0x001F, then JP over each of the larger ranges.  The last
# guard must clear both abutting ranges in one jump, landing at 0x0400.
check_bytes $((0x000E)) "1810"   "JR guard"
check_bytes $((0x003D)) "c34101" "JP guard"
check_bytes $((0x017D)) "c38102" "JP guard"
check_bytes $((0x02BD)) "c30004" "fused-range JP guard"

# DJNZ promoted to djnz +2 / jr +3 / jp lp.
check_bytes $((0x0290)) "10021803c35601" "DJNZ trampoline"

# Conditional branch promoted to JP Z.
check_bytes $((0x0026)) "ca5001" "promoted JP Z"

# --- reserved bytes must be untouched, then become traps ------------------
python3 - "$tmpdir/multi.bin" "$tmpdir/trapped.bin" "${RESERVED[@]}" <<'PY'
import sys

src, dst, *ranges = sys.argv[1:]
image = bytearray(open(src, "rb").read())

for spec in ranges:
    lo, hi = (int(part, 16) for part in spec.split("-"))
    for addr in range(lo, hi + 1):
        if image[addr] != 0x00:
            sys.exit("reserved byte 0x%04X is not zero: 0x%02X"
                     % (addr, image[addr]))
        image[addr] = 0xFF          # RST 38: entering the range is fatal

open(dst, "wb").write(image)
PY

output=$("$XEMU" --run --quiet --load-bin "$tmpdir/trapped.bin" \
    --origin 0x0000 --pc 0x0000 --stdout-port 1)

[[ "$output" == "ABC...EFZ" ]] \
    || fail "expected 'ABC...EFZ' from the trapped image, got '$output'"

echo "${GREEN}ok${RESET}: xld reserved ranges (5 ranges, 2 fused)"
