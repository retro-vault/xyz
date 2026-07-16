#!/usr/bin/env bash
#
# sdcc_compat_test.sh
#
# Verify xas/xld compatibility for a few important sdasz80 source forms.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
XAS="${XAS:-$REPO_ROOT/bin/x/bin/xas}"
XLD="${XLD:-$REPO_ROOT/bin/x/bin/xld}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

tmpdir="$(mktemp -d /tmp/xas_sdcc_compat_XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/export.s" <<'EOF'
            .area _CODE
foo::
            ret
EOF
"$XAS" -o "$tmpdir/export.rel" "$tmpdir/export.s"
grep -Fq "S foo Def00000000" "$tmpdir/export.rel" \
    || fail "foo:: did not export a global definition"

cat > "$tmpdir/retc.s" <<'EOF'
            .area _CODE
_t:
            ret c
EOF
"$XAS" -o "$tmpdir/retc.rel" "$tmpdir/retc.s" \
    || fail "ret c was rejected"

cat > "$tmpdir/index_halves.s" <<'EOF'
            .area _CODE
_index_halves:
            ld a,iyh
            or iyl
            ld iyh,d
            ld iyl,e
            ld ixh,#0x12
            ld b,ixl
            and ixh
            ret
EOF
"$XAS" -o "$tmpdir/index_halves.rel" "$tmpdir/index_halves.s" \
    || fail "IX/IY half-register instructions were rejected"
"$XLD" -nostdlib -e _index_halves --oformat=ihx \
       --section-start=_CODE=0x9000 \
       "$tmpdir/index_halves.rel" -o "$tmpdir/index_halves.ihx"
grep -qi "fd7cfdb5fd62fd6bdd2612dd45dda4c9" "$tmpdir/index_halves.ihx" \
    || fail "IX/IY half-register instructions encoded incorrectly"

cat > "$tmpdir/abs.s" <<'EOF'
            .area _CODE
K == 5
            ld de,#K
EOF
"$XAS" -o "$tmpdir/abs.rel" "$tmpdir/abs.s"
grep -Fq "S K Def00000005" "$tmpdir/abs.rel" \
    || fail "K == 5 did not emit a global absolute definition"

cat > "$tmpdir/diff.s" <<'EOF'
            .globl _start
            .area _CODE
msg:  .db 1,2,3,4,5
            .equ msg_len, . - msg
_start:  ld de,#msg_len
         ret
EOF
"$XAS" -o "$tmpdir/diff.rel" "$tmpdir/diff.s"
"$XLD" -nostdlib -e _start --oformat=ihx \
       --section-start=_CODE=0x9000 \
       "$tmpdir/diff.rel" -o "$tmpdir/diff.ihx"
grep -qi "110500c9" "$tmpdir/diff.ihx" \
    || fail "named label-difference constant did not link as ld de,#5"

echo "${GREEN}PASS${RESET}: xas sdasz80 compatibility"
