#!/usr/bin/env bash
# External test runner for xcc
# Tests in tests/data/external/{tsuite,C99,C11,drs}/
# Pass/fail based on whether the compiler accepts the source.

XCC="${1:-./build/bin/xcc}"
EXT_ROOT="$(dirname "$0")/data/external"
COMMON_INC="-Itests/data/external/include"
PASS=0
FAIL=0
SKIP=0
XCC_ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

run_xcc() {
    env ASAN_OPTIONS="$XCC_ASAN_OPTIONS" "$XCC" "$@"
}

run_ext_test() {
    local c_file="$1"
    local extra_incs="$2"
    local base="${c_file%.c}"
    local name="$(basename "$base")"

    # Per-test flags from .opts file (overrides everything including -S -O0)
    local opts="-S -O0 $COMMON_INC $extra_incs"
    if [[ -f "${base}.opts" ]]; then
        opts="$(cat "${base}.opts") $COMMON_INC $extra_incs"
    fi

    local tmpfile
    tmpfile=$(mktemp /tmp/xcc_ext_XXXXXX.s)
    if run_xcc $opts "$c_file" -o "$tmpfile" 2>/dev/null; then
        echo "${GREEN}PASS${RESET} $name"
        PASS=$((PASS + 1))
    else
        echo "${RED}FAIL${RESET} $name"
        run_xcc $opts "$c_file" -o "$tmpfile" 2>&1 | head -2 | sed 's/^/       /'
        FAIL=$((FAIL + 1))
    fi
    rm -f "$tmpfile"
}

for suite in tsuite C99 C11 drs; do
    dir="$EXT_ROOT/$suite"
    [[ -d "$dir" ]] || continue
    extra=""
    [[ -d "$dir/Inputs" ]] && extra="-I$dir/Inputs"
    echo "--- Suite: $suite ---"
    for f in "$dir"/*.c; do
        [[ -f "$f" ]] || continue
        run_ext_test "$f" "$extra"
    done
done

echo ""
echo "External results: ${GREEN}${PASS} passed${RESET}, ${RED}${FAIL} failed${RESET}"
[[ "$FAIL" -eq 0 ]]
