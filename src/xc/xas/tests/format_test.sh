#!/usr/bin/env bash
# format_test.sh
#
# Verify xas source-format emission in both dialect directions.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
XAS="${XAS:-$ROOT/bin/bin/xas}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

need_line() {
    local pattern="$1"
    local file="$2"
    if ! grep -Fqx "$pattern" "$file"; then
        echo "expected line: $pattern" >&2
        echo "--- $file ---" >&2
        cat "$file" >&2
        fail "missing expected formatted line"
    fi
}

tmpdir="$(mktemp -d /tmp/xas_format_XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/in.sdcc" <<'EOF'
; leading comment
.globl _start
.area _CODE
_start: ; entry
    ld a,#42 ; load literal
    ld 5(ix),a
    .dw _start
EOF

"$XAS" --mode=sdcc --format=gnu -o "$tmpdir/out.gnu" "$tmpdir/in.sdcc"
need_line "; leading comment" "$tmpdir/out.gnu"
need_line ".global _start" "$tmpdir/out.gnu"
need_line ".text" "$tmpdir/out.gnu"
need_line "_start: ; entry" "$tmpdir/out.gnu"
need_line $'ld\ta, 42 ; load literal' "$tmpdir/out.gnu"
need_line $'ld\t(ix+5), a' "$tmpdir/out.gnu"
need_line ".word _start" "$tmpdir/out.gnu"
"$XAS" --mode=gnu -o "$tmpdir/out.o" "$tmpdir/out.gnu"

cat > "$tmpdir/in.gnu" <<'EOF'
// leading comment
.global start
.text
start: // entry
    ld a, 42 // load literal
    ld (ix+5), a
    .word start
EOF

"$XAS" --mode=gnu --format=sdcc -o "$tmpdir/out.sdcc" "$tmpdir/in.gnu"
need_line "; leading comment" "$tmpdir/out.sdcc"
need_line ".globl start" "$tmpdir/out.sdcc"
need_line ".area _CODE" "$tmpdir/out.sdcc"
need_line "start: ; entry" "$tmpdir/out.sdcc"
need_line $'ld\ta, #42 ; load literal' "$tmpdir/out.sdcc"
need_line $'ld\t5(ix), a' "$tmpdir/out.sdcc"
need_line ".dw start" "$tmpdir/out.sdcc"
"$XAS" --mode=sdcc -o "$tmpdir/out.rel" "$tmpdir/out.sdcc"

cat > "$tmpdir/in.macro.gnu" <<'EOF'
.macro demo
    nop
.endm
EOF

if "$XAS" --mode=gnu -o "$tmpdir/macro.o" "$tmpdir/in.macro.gnu" \
        >"$tmpdir/macro.out" 2>"$tmpdir/macro.err"; then
    fail "GNU macro source unexpectedly accepted"
fi
grep -Fq "macro directives are not supported" "$tmpdir/macro.err" \
    || fail "missing GNU macro rejection diagnostic"

cat > "$tmpdir/in.macro.sdcc" <<'EOF'
.macro demo
    nop
.endm
EOF

if "$XAS" --mode=sdcc -o "$tmpdir/macro.rel" "$tmpdir/in.macro.sdcc" \
        >"$tmpdir/macro_sdcc.out" 2>"$tmpdir/macro_sdcc.err"; then
    fail "SDCC macro source unexpectedly accepted"
fi
grep -Fq "macro directives are not supported" "$tmpdir/macro_sdcc.err" \
    || fail "missing SDCC macro rejection diagnostic"

echo "${GREEN}PASS${RESET}: xas format conversion"
