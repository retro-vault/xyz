#!/usr/bin/env bash
#
# include_test.sh
#
# Verify xas expands .include during assembly, honoring both relative paths
# and -I search directories.
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

tmpdir="$(mktemp -d /tmp/xas_include_XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

mkdir -p "$tmpdir/sub" "$tmpdir/inc"

cat > "$tmpdir/sub/local.inc" <<'EOF'
            ld a,#0x12
EOF

cat > "$tmpdir/inc/shared.inc" <<'EOF'
            ld de,#0x3456
EOF

cat > "$tmpdir/main.s" <<'EOF'
            .globl _start
            .area _CODE
_start:
            .include "sub/local.inc"
            .include "shared.inc"
            ret
EOF

"$XAS" --mode=sdcc -I "$tmpdir/inc" -o "$tmpdir/main.rel" "$tmpdir/main.s" \
    || fail "assembly with .include failed"
"$XLD" -nostdlib -e _start --oformat=ihx \
       --section-start=_CODE=0x9000 \
       "$tmpdir/main.rel" -o "$tmpdir/main.ihx"
grep -qi "3e12115634c9" "$tmpdir/main.ihx" \
    || fail "included instructions were not assembled in order"

cat > "$tmpdir/z80.inc" <<'EOF'
            ld bc,#0x789a
EOF

cat > "$tmpdir/sub/parent.s" <<'EOF'
            .globl _parent
            .area _CODE
_parent:
            .include "../z80.inc"
            ret
EOF

# Exercise the bare-input-name case: parent.s has no parent_path() here, but
# its include must still be based on the directory containing parent.s.
(
    cd "$tmpdir/sub"
    "$XAS" --mode=sdcc -o parent.rel parent.s
) || fail 'assembly with relative parent include "../z80.inc" failed'
"$XLD" -nostdlib -e _parent --oformat=ihx \
       --section-start=_CODE=0x9000 \
       "$tmpdir/sub/parent.rel" -o "$tmpdir/sub/parent.ihx"
grep -qi "019a78c9" "$tmpdir/sub/parent.ihx" \
    || fail "relative parent include was not assembled"

cat > "$tmpdir/missing.s" <<'EOF'
            .area _CODE
            .include "does-not-exist.inc"
EOF

if "$XAS" --mode=sdcc -o "$tmpdir/missing.rel" "$tmpdir/missing.s" \
        >"$tmpdir/missing.out" 2>"$tmpdir/missing.err"; then
    fail "missing include unexpectedly succeeded"
fi
grep -Fq 'cannot resolve include "does-not-exist.inc"' "$tmpdir/missing.err" \
    || fail "missing include error was not reported"

echo "${GREEN}PASS${RESET}: xas include expansion"
