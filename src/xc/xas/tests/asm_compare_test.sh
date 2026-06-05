#!/usr/bin/env bash
# asm_compare_test.sh
#
# For each xcc core test: compile C → assembly with xcc, then assemble
# with both xas and sdasz80, link both with xld, and compare the
# resulting Z80 code bytes (stripping the XL binary header).
#
# A test is SKIPPED (not FAILED) when both assemblers produce output that
# xld can't link for the same reason (e.g. missing runtime library).
#
# Usage: ./asm_compare_test.sh [path-to-xcc] [path-to-xas] [path-to-xld]

set -euo pipefail

XCC="${1:-$(dirname "$0")/../../../../bin/bin/xcc}"
XAS="${2:-$(dirname "$0")/../../../../bin/bin/xas}"
XLD="${3:-$(dirname "$0")/../../../../bin/bin/xld}"
SDAS="${SDAS:-sdasz80}"
TESTS_DIR="$(dirname "$0")/../../xcc/tests/data/core"
RUNTIME_DIR="$(dirname "$0")/../../xcc/lib/runtime"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

PASS=0
FAIL=0
SKIP=0
XCC_ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"

XAR="${XAR:-$(dirname "$0")/../../../../bin/bin/xar}"

run_xcc() {
    env ASAN_OPTIONS="$XCC_ASAN_OPTIONS" "$XCC" "$@"
}

# Check tools exist.
for tool in "$XCC" "$XAS" "$XLD" "$XAR" "$SDAS"; do
    if ! command -v "$tool" &>/dev/null && ! [[ -x "$tool" ]]; then
        echo "${RED}ERROR${RESET}: tool not found: $tool"
        exit 1
    fi
done


# Extract code bytes from an XL binary, skipping the 12-byte base header
# and any reloc table entries (4 bytes each, count at bytes 8-9).
xl_code_bytes() {
    local f="$1"
    python3 - "$f" <<'EOF'
import sys, struct
data = open(sys.argv[1], 'rb').read()
if data[:2] != b'XL':
    sys.exit(1)
reloc_count = struct.unpack_from('<H', data, 8)[0]
code_start = 12 + reloc_count * 4
sys.stdout.buffer.write(data[code_start:])
EOF
}

# Detect entry point: symbol present in both .rel files.
# Prefer _main, then first single-underscore function (not __) found in both.
detect_entry() {
    local rel_xas="$1"
    local rel_sdcc="$2"
    # Check for _main in xas rel.
    if grep -q '^S _main Def' "$rel_xas" 2>/dev/null; then
        echo "_main"; return
    fi
    # Single-underscore Def symbols in xas, filtered to those present in sdcc.
    local sdcc_syms
    sdcc_syms=$(grep '^S ' "$rel_sdcc" | grep ' Def' | awk '{print $2}')
    while IFS= read -r sym; do
        if echo "$sdcc_syms" | grep -qx "$sym"; then
            echo "$sym"; return
        fi
    done < <(grep '^S ' "$rel_xas" | grep ' Def' | awk '{print $2}' | grep '^_[^_]')
    # Fallback: first Def in xas.
    grep '^S ' "$rel_xas" | grep ' Def' | head -1 | awk '{print $2}'
}

TMPDIR=$(mktemp -d /tmp/xas_test_XXXXXX)
trap 'rm -rf "$TMPDIR"' EXIT
RTDIR="$TMPDIR/runtime"
mkdir -p "$RTDIR"

# Build runtime library from source (assembled with xas).
RUNTIME_LIB="$RTDIR/xruntime.lib"
for s in "$RUNTIME_DIR"/*.s; do
    base=$(basename "${s%.s}")
    "$XAS" --mode=sdcc "$s" -o "$RTDIR/${base}.rel" 2>/dev/null || true
done
"$XAR" rcs "$RUNTIME_LIB" "$RTDIR"/*.rel 2>/dev/null || true

total=0
for c_file in "$TESTS_DIR"/*.c; do
    name=$(basename "${c_file%.c}")

    # Skip tests that are semantic-error-only.
    [[ -f "${c_file%.c}.error" ]] && { SKIP=$((SKIP+1)); continue; }

    total=$((total+1))

    asm="$TMPDIR/$name.s"
    rel_xas="$TMPDIR/${name}_xas.rel"
    rel_sdcc="$TMPDIR/${name}_sdcc.rel"
    bin_xas="$TMPDIR/${name}_xas.bin"
    bin_sdcc="$TMPDIR/${name}_sdcc.bin"
    code_xas="$TMPDIR/${name}_xas.code"
    code_sdcc="$TMPDIR/${name}_sdcc.code"

    # Step 1: compile C → SDCC assembly.
    if ! run_xcc -S -O0 "$c_file" -o "$asm" 2>"$TMPDIR/xcc_err.txt"; then
        echo "${YELLOW}SKIP${RESET} $name  [xcc failed: $(head -1 "$TMPDIR/xcc_err.txt")]"
        SKIP=$((SKIP+1)); total=$((total-1)); continue
    fi

    # Step 2a: assemble with xas.
    if ! "$XAS" --mode=sdcc "$asm" -o "$rel_xas" 2>"$TMPDIR/xas_err.txt"; then
        echo "${RED}FAIL${RESET} $name  [xas error: $(head -1 "$TMPDIR/xas_err.txt")]"
        FAIL=$((FAIL+1)); continue
    fi

    # Step 2b: assemble with sdasz80.
    if ! "$SDAS" -o "$rel_sdcc" "$asm" 2>"$TMPDIR/sdas_err.txt"; then
        echo "${YELLOW}SKIP${RESET} $name  [sdasz80 error: $(head -1 "$TMPDIR/sdas_err.txt")]"
        SKIP=$((SKIP+1)); total=$((total-1)); continue
    fi

    # Detect entry point — a symbol present in both .rel files.
    entry=$(detect_entry "$rel_xas" "$rel_sdcc")
    if [[ -z "$entry" ]]; then entry="_main"; fi

    # Step 3a: link xas output (with runtime library).
    xas_link_ok=true
    if ! "$XLD" -e "$entry" -o "$bin_xas" "$rel_xas" "$RUNTIME_LIB" 2>"$TMPDIR/xlink_xas_err.txt"; then
        xas_link_ok=false
    fi

    # Step 3b: link sdasz80 output (with runtime library).
    sdcc_link_ok=true
    if ! "$XLD" -e "$entry" -o "$bin_sdcc" "$rel_sdcc" "$RUNTIME_LIB" 2>"$TMPDIR/xlink_sdcc_err.txt"; then
        sdcc_link_ok=false
    fi

    # If both fail to link, it's a runtime/library issue — skip.
    if ! $xas_link_ok && ! $sdcc_link_ok; then
        echo "${YELLOW}SKIP${RESET} $name  [both linkers fail: $(head -1 "$TMPDIR/xlink_xas_err.txt")]"
        SKIP=$((SKIP+1)); total=$((total-1)); continue
    fi

    # If xas link fails but sdcc link succeeded, that's a real bug.
    if ! $xas_link_ok; then
        echo "${RED}FAIL${RESET} $name  [xld(xas) error: $(head -1 "$TMPDIR/xlink_xas_err.txt")]"
        FAIL=$((FAIL+1)); continue
    fi

    # If sdcc link fails but xas link succeeded, unexpected.
    if ! $sdcc_link_ok; then
        echo "${RED}FAIL${RESET} $name  [xld(sdcc) error: $(head -1 "$TMPDIR/xlink_sdcc_err.txt")]"
        FAIL=$((FAIL+1)); continue
    fi

    # Step 4: extract code bytes.
    if ! xl_code_bytes "$bin_xas" > "$code_xas" 2>/dev/null; then
        echo "${RED}FAIL${RESET} $name  [could not parse xas binary header]"
        FAIL=$((FAIL+1)); continue
    fi
    if ! xl_code_bytes "$bin_sdcc" > "$code_sdcc" 2>/dev/null; then
        echo "${RED}FAIL${RESET} $name  [could not parse sdcc binary header]"
        FAIL=$((FAIL+1)); continue
    fi

    # Step 5: compare code bytes.
    if cmp -s "$code_xas" "$code_sdcc"; then
        echo "${GREEN}PASS${RESET} $name"
        PASS=$((PASS+1))
    else
        echo "${RED}FAIL${RESET} $name  [code mismatch]"
        echo "       xas:  $(xxd -p "$code_xas" | tr -d '\n')"
        echo "       sdcc: $(xxd -p "$code_sdcc" | tr -d '\n')"
        FAIL=$((FAIL+1))
    fi
done

echo ""
echo "Results: ${GREEN}${PASS} passed${RESET}  ${RED}${FAIL} failed${RESET}  ${YELLOW}${SKIP} skipped${RESET}  (${total} tested)"
[[ $FAIL -eq 0 ]]
