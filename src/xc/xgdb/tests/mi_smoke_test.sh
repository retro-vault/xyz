#!/usr/bin/env bash
#
# mi_smoke_test.sh
#
# Verify that xgdb accepts the MI entrypoint and answers a minimal
# command set without a connected target.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
XGDB="${1:-$ROOT/bin/x/bin/xgdb}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

need_fragment() {
    local fragment="$1"
    local file="$2"
    if ! grep -Fq "$fragment" "$file"; then
        echo "expected fragment: $fragment" >&2
        echo "--- $file ---" >&2
        cat "$file" >&2
        fail "missing expected MI output"
    fi
}

tmpdir="$(mktemp -d /tmp/xgdb_mi_XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/in.mi" <<'EOF'
1-gdb-version
2-list-features
3-data-evaluate-expression 0x1234
4-interpreter-exec console "show version"
5-gdb-exit
EOF

"$XGDB" --interpreter=mi2 < "$tmpdir/in.mi" > "$tmpdir/out.txt" 2> "$tmpdir/err.txt"

if [[ -s "$tmpdir/err.txt" ]]; then
    echo "--- stderr ---" >&2
    cat "$tmpdir/err.txt" >&2
    fail "MI smoke emitted stderr"
fi

need_fragment '(gdb)' "$tmpdir/out.txt"
need_fragment '1^done,version="xgdb 0.1"' "$tmpdir/out.txt"
need_fragment '2^done,features=[]' "$tmpdir/out.txt"
need_fragment '3^done,value="0x1234"' "$tmpdir/out.txt"
need_fragment '~"GNU gdb (xgdb) 0.1\n"' "$tmpdir/out.txt"
need_fragment '4^done' "$tmpdir/out.txt"
need_fragment '5^exit' "$tmpdir/out.txt"

echo "${GREEN}PASS${RESET}: xgdb MI smoke"
