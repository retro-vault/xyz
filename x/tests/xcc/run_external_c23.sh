#!/usr/bin/env bash
# run_external_c23.sh — WG14 conformance test runner for xcc
# Tests WG14 paper test files from the Clang test suite against xcc.
# Clang-specific annotations (// RUN:, // CHECK:, // expected-error:) are
# comments to xcc and are ignored; only the actual C code is compiled.
#
# Usage: run_external_c23.sh [--mode=sdcc|gnu]

# Note: no set -e here — we need to handle non-zero exit from xcc ourselves

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
X_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
REPO_ROOT="$(cd "$X_ROOT/.." && pwd)"
XCC="${XCC:-$REPO_ROOT/bin/x/bin/xcc}"
C23_DIR="$SCRIPT_DIR/data/external/C23"
C2Y_DIR="$SCRIPT_DIR/data/external/C2y"
INCLUDE_DIR="$X_ROOT/libc/include"

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
MODE=sdcc
for arg in "$@"; do
    case "$arg" in
        --mode=sdcc) MODE=sdcc ;;
        --mode=gnu)  MODE=gnu  ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

if [ "$MODE" = "gnu" ]; then
    XCC_FLAGS="-S -O0 --mode=gnu"
else
    XCC_FLAGS="-S -O0 --mode=sdcc"
fi

XCC_ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"

run_xcc() {
    env ASAN_OPTIONS="$XCC_ASAN_OPTIONS" "$XCC" "$@"
}

# ---------------------------------------------------------------------------
# Feature detection patterns (grep -E extended regex, applied to file content)
# FAIL-FEATURE = known unsupported / outside xcc scope
# ---------------------------------------------------------------------------
# xcc supports: typeof, typeof_unqual, nullptr, constexpr, [[attributes]], _BitInt
# xcc does NOT support: #embed, __int128, _DecimalN, __builtin_unreachable (as ICE)
FEATURE_PATTERNS=(
    "#embed"
    "__has_embed"
    "__int128"
    "_Decimal[0-9]"
    "_Countof"
    "__builtin_complex"
    "__builtin_inf"
    "__builtin_nans"
    "triple=x86_64.*emit-llvm"
    "case [0-9]+ \.\.\. [0-9]+"
)

FEATURE_LABELS=(
    "#embed"
    "__has_embed"
    "__int128"
    "_DecimalN"
    "_Countof (C2y)"
    "__builtin_complex"
    "__builtin_inf"
    "__builtin_nansf/nans (IEEE754 signaling NaN)"
    "LLVM IR x86_64 test (not compilable)"
    "GCC case range extension"
)

# ---------------------------------------------------------------------------
# Counters and result arrays
# ---------------------------------------------------------------------------
count_pass=0
count_warn=0
count_fail_feature=0
count_fail_syntax=0
count_fail_other=0
count_total=0

fail_other_files=()
fail_other_errors=()
fail_feature_files=()
fail_feature_reasons=()

# ---------------------------------------------------------------------------
# classify_file FILE
# ---------------------------------------------------------------------------
classify_file() {
    local filepath="$1"
    local filename
    filename=$(basename "$filepath")
    local suite
    suite=$(basename "$(dirname "$filepath")")
    local label="${suite}/${filename}"

    count_total=$((count_total + 1))

    # 1. Check for deliberate syntax/semantic error tests.
    # Match both standard Clang expected-* annotations and non-standard prefixed
    # variants (may appear in both // line comments and /* block comments),
    # including hyphenated prefixes like "no-trigraphs-error", "compat-warning".
    if grep -qE "(//|/\*) *(expected-error|expected-warning|expected-note|[a-z][a-z0-9_-]+-error|[a-z][a-z0-9_-]+-warning)" \
            "$filepath" 2>/dev/null; then
        count_fail_syntax=$((count_fail_syntax + 1))
        printf "%-15s %s [deliberate error/warning test]\n" "FAIL-SYNTAX" "$label"
        return
    fi
    # Also catch "expected-no-diagnostics" tests — these should pass cleanly;
    # leave them to be classified by actual compilation.
    # n3033.c and similar preprocessor-only tests: macro expansions at file scope
    # produce non-compilable C; classify based on RUN flags.
    if grep -qE "^// RUN:.*-E " "$filepath" 2>/dev/null; then
        count_fail_syntax=$((count_fail_syntax + 1))
        printf "%-15s %s [preprocessor-only test, not compilable]\n" "FAIL-SYNTAX" "$label"
        return
    fi

    # 2. Check for known unsupported features (stop at first match)
    local feature_found=0
    for i in "${!FEATURE_PATTERNS[@]}"; do
        if grep -qE "${FEATURE_PATTERNS[$i]}" "$filepath" 2>/dev/null; then
            local reason="${FEATURE_LABELS[$i]}"
            count_fail_feature=$((count_fail_feature + 1))
            fail_feature_files+=("$label")
            fail_feature_reasons+=("$reason")
            printf "%-15s %s [unsupported: %s]\n" "FAIL-FEATURE" "$label" "$reason"
            feature_found=1
            break
        fi
    done
    if [ "$feature_found" -eq 1 ]; then
        return
    fi

    # 3. Compile with xcc — capture stderr, preserve exit code
    local tmp_out
    tmp_out=$(mktemp /tmp/xcc_run_XXXXXX.txt)
    # shellcheck disable=SC2086
    run_xcc $XCC_FLAGS -I"$INCLUDE_DIR" "$filepath" -o /dev/null >"$tmp_out" 2>&1
    local exit_code=$?
    local output
    output=$(cat "$tmp_out")
    rm -f "$tmp_out"

    if [ "$exit_code" -eq 0 ]; then
        # Check if stderr had any content (warnings)
        if [ -n "$output" ]; then
            count_warn=$((count_warn + 1))
            printf "%-15s %s [warnings only]\n" "WARN" "$label"
        else
            count_pass=$((count_pass + 1))
            printf "%-15s %s\n" "PASS" "$label"
        fi
    else
        # Genuine unexpected compile failure
        local first_error
        first_error=$(printf '%s' "$output" | grep -m1 "error:" 2>/dev/null || printf '%s' "$output" | head -1)
        count_fail_other=$((count_fail_other + 1))
        fail_other_files+=("$label")
        fail_other_errors+=("$first_error")
        printf "%-15s %s [%s]\n" "FAIL-OTHER" "$label" "$first_error"
    fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
echo "========================================================"
echo " xcc WG14 External Conformance Test Runner"
echo " Mode: $MODE  (flags: $XCC_FLAGS)"
echo " Date: $(date '+%Y-%m-%d %H:%M:%S')"
echo "========================================================"
echo ""
echo "--- C23 tests ---"
for f in "$C23_DIR"/*.c; do
    [ -f "$f" ] || continue
    classify_file "$f"
done

echo ""
echo "--- C2y tests ---"
for f in "$C2Y_DIR"/*.c; do
    [ -f "$f" ] || continue
    classify_file "$f"
done

echo ""
echo "========================================================"
echo " SUMMARY"
echo "========================================================"
printf "  %-20s %d\n" "PASS"         "$count_pass"
printf "  %-20s %d\n" "WARN"         "$count_warn"
printf "  %-20s %d\n" "FAIL-FEATURE" "$count_fail_feature"
printf "  %-20s %d\n" "FAIL-SYNTAX"  "$count_fail_syntax"
printf "  %-20s %d\n" "FAIL-OTHER"   "$count_fail_other"
printf "  %-20s %d\n" "TOTAL"        "$count_total"
echo ""

if [ "${#fail_other_files[@]}" -gt 0 ]; then
    echo "--- FAIL-OTHER (genuine compiler bugs) ---"
    for i in "${!fail_other_files[@]}"; do
        printf "  %-42s %s\n" "${fail_other_files[$i]}" "${fail_other_errors[$i]}"
    done
    echo ""
fi

if [ "${#fail_feature_files[@]}" -gt 0 ]; then
    echo "--- FAIL-FEATURE breakdown ---"
    declare -A feature_count
    for i in "${!fail_feature_files[@]}"; do
        local_reason="${fail_feature_reasons[$i]}"
        feature_count["$local_reason"]=$(( ${feature_count["$local_reason"]:-0} + 1 ))
    done
    for feature in "${!feature_count[@]}"; do
        printf "  %-30s %d file(s)\n" "$feature" "${feature_count[$feature]}"
    done
    echo ""
fi

echo "Note: FAIL-SYNTAX files contain deliberate error/warning test annotations."
echo "      These are expected failures by design, not compiler bugs."
echo "========================================================"
