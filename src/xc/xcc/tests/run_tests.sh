#!/usr/bin/env bash
# XCC test runner
# Usage: run_tests.sh [path-to-xcc]
# Test suites are discovered automatically from subdirectories of tests/data/.

XCC="${1:-./build/bin/xcc}"
TESTS_ROOT="$(dirname "$0")/data"
PASS=0
FAIL=0
SKIP=0

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

if [[ ! -x "$XCC" ]]; then
    echo "${RED}ERROR: xcc not found at '$XCC'${RESET}"
    exit 1
fi

run_test() {
    local c_file="$1"
    local base="${c_file%.c}"
    local name="$(basename "$base")"
    local expected="${base}.expected"
    local error_pat="${base}.error"
    local actual
    local tmpfile

    # Per-test compiler flags: read from .opts file if present (one line, space-separated)
    local opts="-S -O0"
    if [[ -f "${base}.opts" ]]; then
        opts="$(cat "${base}.opts")"
    fi

    # Warning tests: compiler must succeed AND stderr must contain expected pattern.
    local warn_pat="${base}.warning"
    if [[ -f "$warn_pat" ]]; then
        local pat
        pat="$(cat "$warn_pat")"
        tmpfile=$(mktemp /tmp/xcc_test_XXXXXX.s)
        if ! "$XCC" $opts "$c_file" -o "$tmpfile" 2>/tmp/xcc_err_$$.txt; then
            echo "${RED}FAIL${RESET} $name  [unexpected compile error]"
            head -5 /tmp/xcc_err_$$.txt | sed 's/^/       /'
            rm -f "$tmpfile" /tmp/xcc_err_$$.txt
            FAIL=$((FAIL + 1))
            return
        fi
        if grep -qF "$pat" /tmp/xcc_err_$$.txt; then
            echo "${GREEN}PASS${RESET} $name"
            PASS=$((PASS + 1))
        else
            echo "${RED}FAIL${RESET} $name  [warning not found: '$pat']"
            head -5 /tmp/xcc_err_$$.txt | sed 's/^/       /'
            FAIL=$((FAIL + 1))
        fi
        rm -f "$tmpfile" /tmp/xcc_err_$$.txt
        return
    fi

    # Error tests: compiler must reject and stderr must contain expected pattern.
    if [[ -f "$error_pat" ]]; then
        local pat
        pat="$(cat "$error_pat")"
        tmpfile=$(mktemp /tmp/xcc_test_XXXXXX.s)
        if "$XCC" $opts "$c_file" -o "$tmpfile" 2>/tmp/xcc_err_$$.txt; then
            echo "${RED}FAIL${RESET} $name  [expected compile error but succeeded]"
            rm -f "$tmpfile" /tmp/xcc_err_$$.txt
            FAIL=$((FAIL + 1))
            return
        fi
        if grep -qF "$pat" /tmp/xcc_err_$$.txt; then
            echo "${GREEN}PASS${RESET} $name"
            PASS=$((PASS + 1))
        else
            echo "${RED}FAIL${RESET} $name  [error output did not match '$pat']"
            head -5 /tmp/xcc_err_$$.txt | sed 's/^/       /'
            FAIL=$((FAIL + 1))
        fi
        rm -f "$tmpfile" /tmp/xcc_err_$$.txt
        return
    fi

    # Compile to assembly
    tmpfile=$(mktemp /tmp/xcc_test_XXXXXX.s)
    if ! "$XCC" $opts "$c_file" -o "$tmpfile" 2>/tmp/xcc_err_$$.txt; then
        echo "${RED}FAIL${RESET} $name  [compile error]"
        head -5 /tmp/xcc_err_$$.txt | sed 's/^/       /'
        rm -f "$tmpfile" /tmp/xcc_err_$$.txt
        FAIL=$((FAIL + 1))
        return
    fi
    rm -f /tmp/xcc_err_$$.txt

    # If --generate flag: write expected file and mark as GENERATED
    if [[ "${GENERATE:-0}" == "1" ]]; then
        cp "$tmpfile" "$expected"
        echo "${YELLOW}GEN${RESET}  $name"
        rm -f "$tmpfile"
        SKIP=$((SKIP + 1))
        return
    fi

    # Check expected output
    if [[ ! -f "$expected" ]]; then
        echo "${YELLOW}SKIP${RESET} $name  [no .expected file]"
        rm -f "$tmpfile"
        SKIP=$((SKIP + 1))
        return
    fi

    # Strip comments and whitespace for comparison
    actual=$(grep -v '^\s*;' "$tmpfile" | sed 's/;.*//' | sed '/^\s*$/d' | sed 's/\t/ /g' | sed 's/  */ /g' | sed 's/^ //')
    expected_clean=$(grep -v '^\s*;' "$expected" | sed 's/;.*//' | sed '/^\s*$/d' | sed 's/\t/ /g' | sed 's/  */ /g' | sed 's/^ //')

    if [[ "$actual" == "$expected_clean" ]]; then
        echo "${GREEN}PASS${RESET} $name"
        PASS=$((PASS + 1))
    else
        echo "${RED}FAIL${RESET} $name"
        echo "  Expected:"
        echo "$expected_clean" | head -20 | sed 's/^/    /'
        echo "  Got:"
        echo "$actual" | head -20 | sed 's/^/    /'
        FAIL=$((FAIL + 1))
    fi
    rm -f "$tmpfile"
}

# Run all .c tests from every suite subdirectory
for suite_dir in "$TESTS_ROOT"/*/; do
    [[ -d "$suite_dir" ]] || continue
    suite_name="$(basename "$suite_dir")"
    echo "--- Suite: $suite_name ---"
    for f in "$suite_dir"*.c; do
        [[ -f "$f" ]] || continue
        run_test "$f"
    done
done

echo ""
echo "Results: ${GREEN}${PASS} passed${RESET}, ${RED}${FAIL} failed${RESET}, ${YELLOW}${SKIP} skipped${RESET}"
echo ""

[[ "$FAIL" -eq 0 ]]
