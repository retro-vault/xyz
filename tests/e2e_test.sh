#!/usr/bin/env bash
# tests/e2e_test.sh
#
# End-to-end regression test for the XYZ toolchain.
# Runs in phases: build, xcc unit tests, xlink unit tests,
# xas parity tests, xar smoke tests, full-chain integration.
#
# Usage: ./tests/e2e_test.sh [--no-build] [--phase <name>]
#   --no-build   skip the build step (assume binaries are current)
#   --phase      run only the named phase (xz80|xcc|xlink|xas|xar|chain)
#
# Exit: 0 if all selected phases pass, 1 otherwise.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/bin"
XCC="$BIN/xcc"
XAS="$BIN/xas"
XAR="$BIN/xar"
XLINK="$BIN/xlink"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
BOLD=$'\033[1m'
RESET=$'\033[0m'

DO_BUILD=true
ONLY_PHASE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-build) DO_BUILD=false; shift ;;
        --phase)    ONLY_PHASE="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

PHASE_PASS=0
PHASE_FAIL=0

run_phase() {
    local name="$1"
    local label="$2"
    shift 2

    [[ -n "$ONLY_PHASE" && "$ONLY_PHASE" != "$name" ]] && return 0

    echo ""
    echo "${BOLD}=== Phase: $label ===${RESET}"
    if "$@"; then
        echo "${GREEN}PHASE PASS${RESET}: $label"
        PHASE_PASS=$((PHASE_PASS+1))
    else
        echo "${RED}PHASE FAIL${RESET}: $label"
        PHASE_FAIL=$((PHASE_FAIL+1))
    fi
}

# ---------------------------------------------------------------------------
# Phase: build
# ---------------------------------------------------------------------------
phase_build() {
    local dirs=(
        "$ROOT/lib/xz80"
        "$ROOT/src/xc/xcc"
        "$ROOT/src/xc/xas"
        "$ROOT/src/xc/xar"
        "$ROOT/src/xc/xlink"
    )
    local ok=true
    for d in "${dirs[@]}"; do
        local tgt
        tgt="$(basename "$d")"
        echo -n "  Building $tgt ... "
        if make -C "$d" -j"$(nproc)" 2>/tmp/e2e_build_err.txt; then
            echo "${GREEN}ok${RESET}"
        else
            echo "${RED}FAILED${RESET}"
            cat /tmp/e2e_build_err.txt | tail -10 | sed 's/^/    /'
            ok=false
        fi
    done
    $ok
}

# ---------------------------------------------------------------------------
# Phase: xz80 unit tests (CPU emulator + disassembler)
# ---------------------------------------------------------------------------
phase_xz80() {
    make -C "$ROOT/lib/xz80" test
}

# ---------------------------------------------------------------------------
# Phase: xcc unit tests
# ---------------------------------------------------------------------------
phase_xcc() {
    "$ROOT/src/xc/xcc/tests/run_tests.sh" "$XCC"
}

# ---------------------------------------------------------------------------
# Phase: xlink unit tests
# ---------------------------------------------------------------------------
phase_xlink() {
    make -C "$ROOT/src/xc/xlink" test
}

# ---------------------------------------------------------------------------
# Phase: xas parity (compare with sdasz80 across all xcc test inputs)
# ---------------------------------------------------------------------------
phase_xas() {
    if ! command -v sdasz80 &>/dev/null; then
        echo "${YELLOW}SKIP${RESET}: sdasz80 not found, skipping xas parity phase"
        return 0
    fi
    "$ROOT/src/xc/xas/tests/asm_compare_test.sh" "$XCC" "$XAS" "$XLINK"
}

# ---------------------------------------------------------------------------
# Phase: xar smoke tests
# ---------------------------------------------------------------------------
phase_xar() {
    local tmpdir
    tmpdir=$(mktemp -d /tmp/xar_smoke_XXXXXX)

    local ok=true

    # Create a trivial relocatable: two files with one symbol each.
    cat > "$tmpdir/a.s" <<'EOF'
    .module a
    .area _CODE
    .globl _foo
_foo:
    ld  a, #1
    ret
EOF
    cat > "$tmpdir/b.s" <<'EOF'
    .module b
    .area _CODE
    .globl _bar
_bar:
    ld  a, #2
    ret
EOF

    # Assemble both.
    "$XAS" --mode=sdcc "$tmpdir/a.s" -o "$tmpdir/a.rel" 2>/dev/null || { echo "  xas failed on a.s"; ok=false; }
    "$XAS" --mode=sdcc "$tmpdir/b.s" -o "$tmpdir/b.rel" 2>/dev/null || { echo "  xas failed on b.s"; ok=false; }
    $ok || return 1

    # Create archive.
    "$XAR" rcs "$tmpdir/test.lib" "$tmpdir/a.rel" "$tmpdir/b.rel" 2>/dev/null \
        || { echo "  xar create failed"; return 1; }
    [[ -f "$tmpdir/test.lib" ]] || { echo "  archive not created"; return 1; }
    echo "  ${GREEN}create${RESET}: ok"

    # List archive — must mention both modules.
    local listing
    listing=$("$XAR" t "$tmpdir/test.lib" 2>/dev/null)
    echo "$listing" | grep -q 'a' || { echo "  list: 'a' not found in listing"; return 1; }
    echo "$listing" | grep -q 'b' || { echo "  list: 'b' not found in listing"; return 1; }
    echo "  ${GREEN}list${RESET}: ok"

    # Link using the archive (pull in _foo only).
    cat > "$tmpdir/main.s" <<'EOF'
    .module main
    .area _CODE
    .globl _main
_main:
    .globl _foo
    call _foo
    ret
EOF
    "$XAS" --mode=sdcc "$tmpdir/main.s" -o "$tmpdir/main.rel" 2>/dev/null \
        || { echo "  xas failed on main.s"; return 1; }
    "$XLINK" -e _main -o "$tmpdir/out.bin" "$tmpdir/main.rel" "$tmpdir/test.lib" 2>/dev/null \
        || { echo "  xlink failed to link against archive"; return 1; }
    [[ -f "$tmpdir/out.bin" ]] || { rm -rf "$tmpdir"; echo "  no output binary"; return 1; }
    echo "  ${GREEN}link-with-archive${RESET}: ok"

    rm -rf "$tmpdir"
    true
}

# ---------------------------------------------------------------------------
# Phase: full-chain integration (xcc → xas → xlink produces valid XL binary)
# ---------------------------------------------------------------------------
phase_chain() {
    local tmpdir
    tmpdir=$(mktemp -d /tmp/e2e_chain_XXXXXX)

    local runtime_dir="$ROOT/src/xc/xcc/lib/runtime"
    local rtdir="$tmpdir/rt"
    mkdir -p "$rtdir"

    # Build runtime library.
    echo -n "  Building runtime library ... "
    local rt_ok=true
    for s in "$runtime_dir"/*.s; do
        local base
        base=$(basename "${s%.s}")
        "$XAS" --mode=sdcc "$s" -o "$rtdir/${base}.rel" 2>/dev/null || true
    done
    "$XAR" rcs "$rtdir/runtime.lib" "$rtdir"/*.rel 2>/dev/null \
        || { echo "${RED}FAILED${RESET}"; rt_ok=false; }
    $rt_ok && echo "${GREEN}ok${RESET}"
    $rt_ok || return 1

    # Representative programs covering key language features.
    local pass=0 fail=0

    # run_chain_prog <dir> <rtlib> <name> <source>
    run_chain_prog() {
        local dir="$1" rtlib="$2" name="$3" src="$4"

        printf '%s\n' "$src" > "$dir/${name}.c"

        if ! "$XCC" -S -O0 "$dir/${name}.c" -o "$dir/${name}.s" 2>/dev/null; then
            echo "  ${RED}FAIL${RESET} $name [xcc]"; return 1
        fi
        if ! "$XAS" --mode=sdcc "$dir/${name}.s" -o "$dir/${name}.rel" 2>/dev/null; then
            echo "  ${RED}FAIL${RESET} $name [xas]"; return 1
        fi
        if ! "$XLINK" -e _main -o "$dir/${name}.bin" "$dir/${name}.rel" "$rtlib" 2>/dev/null; then
            echo "  ${RED}FAIL${RESET} $name [xlink]"; return 1
        fi
        local magic
        magic=$(python3 -c "import sys; d=open('$dir/$name.bin','rb').read(); print(d[:2])" 2>/dev/null)
        if [[ "$magic" != "b'XL'" ]]; then
            echo "  ${RED}FAIL${RESET} $name [bad XL header: $magic]"; return 1
        fi
        echo "  ${GREEN}PASS${RESET} $name"
    }

    local rtlib="$rtdir/runtime.lib"
    local _p=0 _f=0
    chain_test() {
        if run_chain_prog "$tmpdir" "$rtlib" "$1" "$2"; then _p=$((_p+1)); else _f=$((_f+1)); fi
    }

    chain_test "return_const"  'int main(void){ return 42; }'
    chain_test "arithmetic"    'int main(void){ int a=3,b=4; return a+b*2-1; }'
    chain_test "globals"       'int g; int main(void){ g=7; return g; }'
    chain_test "pointer"       'int x; int *p; int main(void){ p=&x; *p=5; return *p; }'
    chain_test "loop"          'int main(void){ int i,s=0; for(i=0;i<10;i++) s+=i; return s; }'
    chain_test "function_call" 'int add(int a,int b){return a+b;} int main(void){return add(3,4);}'
    chain_test "array"         'int a[4]; int main(void){ a[0]=1;a[1]=2;a[2]=3;a[3]=4; return a[2]; }'
    chain_test "if_else"       'int main(void){ int x=5; if(x>3) return 1; else return 0; }'

    echo ""
    echo "  Chain results: ${GREEN}${_p} passed${RESET}  ${RED}${_f} failed${RESET}"
    rm -rf "$tmpdir"
    [[ $_f -eq 0 ]]
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
if $DO_BUILD && [[ -z "$ONLY_PHASE" || "$ONLY_PHASE" == "build" ]]; then
    run_phase "build" "Build all tools" phase_build
    # Abort if build failed — nothing else will work.
    if [[ $PHASE_FAIL -gt 0 ]]; then
        echo ""
        echo "${RED}Build failed — aborting.${RESET}"
        exit 1
    fi
fi

run_phase "xz80"  "xz80 unit tests"             phase_xz80
run_phase "xcc"   "xcc unit tests"             phase_xcc
run_phase "xlink" "xlink unit tests"           phase_xlink
run_phase "xas"   "xas parity (vs sdasz80)"    phase_xas
run_phase "xar"   "xar smoke tests"            phase_xar
run_phase "chain" "Full-chain integration"     phase_chain

echo ""
echo "${BOLD}=== E2E Summary ===${RESET}"
echo "Phases: ${GREEN}${PHASE_PASS} passed${RESET}  ${RED}${PHASE_FAIL} failed${RESET}"
echo ""

[[ $PHASE_FAIL -eq 0 ]]
