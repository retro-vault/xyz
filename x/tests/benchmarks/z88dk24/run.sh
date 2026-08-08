#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
UPSTREAM="$CORPUS/upstream"
FRAMEWORK="$CORPUS/framework"
MANIFEST="$CORPUS/manifest.tsv"
XCC="${XCC:-$ROOT/bin/x/bin/xcc}"
Z88DK="${Z88DK:-$ROOT/orig/z88dk}"
ZCC="$Z88DK/bin/zcc"
XCC_SHIM="${XCC_SHIM:-}"
RUNNER="$ROOT/build/bin/z80_exec"
OUT="${OUT:-$ROOT/build/x/benchmarks/z88dk24}"
CYCLES="${CYCLES:-800000000}"
FILTER="${FILTER:-}"
ARTIFACTS=""

usage() {
    echo "usage: $0 [--filter REGEX] [--cycles N] [--outdir DIR] [--xcc PATH] [--z88dk DIR]"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
    --filter) FILTER="$2"; shift 2 ;;
    --cycles) CYCLES="$2"; shift 2 ;;
    --outdir) OUT="$2"; shift 2 ;;
    --xcc) XCC="$2"; shift 2 ;;
    --z88dk) Z88DK="$2"; ZCC="$2/bin/zcc"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

# Each program is executed from its work directory so fixtures remain local.
# Normalize a caller-provided relative output path before that directory
# change; otherwise the runner looks for work/work/.../program.bin and every
# compiler is incorrectly reported as hanging.
if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
[[ "$OUT" != / && "$OUT" != "$ROOT" ]] || {
    echo "refusing unsafe output directory: $OUT" >&2
    exit 2
}
if [[ -z "$XCC_SHIM" ]]; then
    XCC_SHIM="$Z88DK/lib/xcc_benchmark_shim.o"
fi

[[ -x "$XCC" ]] || { echo "missing xcc: $XCC (run: make -C x)" >&2; exit 2; }
XCC="$(cd -- "$(dirname -- "$XCC")" && pwd -P)/$(basename -- "$XCC")"
[[ -x "$ZCC" ]] || { echo "missing z88dk zcc: $ZCC" >&2; exit 2; }
[[ -f "$XCC_SHIM" ]] || {
    echo "missing patched-z88dk XCC shim: $XCC_SHIM" >&2
    exit 2
}
[[ -x "$RUNNER" ]] || { echo "missing runner: $RUNNER" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/work"
mkdir -p "$OUT/artifacts"
ARTIFACTS="$OUT/artifacts"
RESULTS="$OUT/results.csv"
printf 'benchmark,xcc_Os_status,xcc_Os_bytes,xcc_Os_cycles,xcc_Of_status,xcc_Of_bytes,xcc_Of_cycles,sdcc_status,sdcc_bytes,sdcc_cycles,80cc_fp_status,80cc_fp_bytes,80cc_fp_cycles,80cc_sp_status,80cc_sp_bytes,80cc_sp_cycles\n' > "$RESULTS"

decode_fixture() {
    local src="$1" dst="$2"
    if base64 --decode "$src" > "$dst" 2>/dev/null; then return; fi
    base64 -D "$src" > "$dst"
}

stage_sources() {
    local src="$1" work="$2"
    local local_src
    cp "$FRAMEWORK/test.c" "$work/test.c"
    cp "$FRAMEWORK/test.h" "$work/test.h"
    local_src="$(basename "$src")"
    cp "$src" "$work/$local_src"
    if [[ "$src" == "$CORPUS/compat/md5.c" ]]; then
        cp "$UPSTREAM/md5/md5sum.c" "$work/md5sum.c"
    fi
    printf '%s\n' "$local_src"
}

run_z88dk_mode() {
    local name="$1" rel="$2" label="$3" flags="$4" work="$5"
    local src="$UPSTREAM/$rel" bin="$work/$label.bin"
    local log="$work/$label.build.log" run_log="$work/$label.run.log"
    local output="$work/$label.output" summary status bytes cycles ret done
    local artifact_dir local_src
    local -a args
    local -a extra_sources=()
    read -r -a args <<< "$flags"
    local_src="$(stage_sources "$src" "$work")"
    if [[ "$label" == xcc_* ]]; then
        args+=('-D__preserves_regs\(...\)=')
        extra_sources+=("$XCC_SHIM")
    fi
    if ! (cd "$work" && env ZCCCFG="$Z88DK/lib/config" \
        PATH="$(dirname "$XCC"):$Z88DK/bin:$PATH" zcc +test -vn "${args[@]}" \
        -I. -DNO_LOG_RUNNING -DNO_LOG_PASSED \
        test.c "$local_src" "${extra_sources[@]}" -o "$bin" -m) >"$log" 2>&1; then
        printf 'BUILD,0,0\n'; return
    fi
    artifact_dir="$ARTIFACTS/$name/$label"
    mkdir -p "$artifact_dir"
    cp "$bin" "$artifact_dir/program.bin"
    bytes="$(wc -c < "$bin" | tr -d ' ')"
    set +e
    summary="$(cd "$work" && "$RUNNER" --bin --z88dk-trap \
        --cycles "$CYCLES" --fs-root "$work" --stdout "$output" "$bin" 2>&1)"
    set -e
    printf '%s\n' "$summary" > "$run_log"
    done="$(sed -n 's/.*done=\([0-9][0-9]*\).*/\1/p' <<< "$summary" | tail -1)"
    ret="$(sed -n 's/.*return=\([0-9][0-9]*\).*/\1/p' <<< "$summary" | tail -1)"
    cycles="$(sed -n 's/.*cycles=\([0-9][0-9]*\).*/\1/p' <<< "$summary" | tail -1)"
    if [[ "$done" == 1 && "$ret" == 0 ]] &&
        grep -qE '[0-9]+ run, [0-9]+ passed, 0 failed' "$output"; then
        status=OK
    elif [[ "$done" == 1 ]] &&
        grep -qE '[1-9][0-9]* failed' "$output"; then
        status=FAIL
    elif [[ "$done" != 1 || -z "$cycles" ]]; then
        status=HANG
    else
        status=ERROR
    fi
    printf '%s,%s,%s\n' "$status" "$bytes" "${cycles:-0}"
}

while IFS=$'\t' read -r name rel; do
    [[ -n "$name" ]] || continue
    if [[ -n "$FILTER" && ! "$name" =~ $FILTER ]]; then continue; fi
    work="$OUT/work/$name"
    mkdir -p "$work"
    if [[ "$name" == md5 ]]; then
        decode_fixture "$UPSTREAM/md5/md5test.bin.b64" "$work/md5test.bin"
    fi
    echo "z88dk24: $name"
    xos="$(run_z88dk_mode "$name" "$rel" xcc_Os '-compiler=xcc -Cx-Os' "$work")"
    xof="$(run_z88dk_mode "$name" "$rel" xcc_Of '-compiler=xcc -Cx-Of' "$work")"
    sdc="$(run_z88dk_mode "$name" "$rel" sdcc '-compiler=sdcc' "$work")"
    ofp="$(run_z88dk_mode "$name" "$rel" 80cc_fp '-compiler=80cc -Cc-frameix' "$work")"
    osp="$(run_z88dk_mode "$name" "$rel" 80cc_sp '-compiler=80cc' "$work")"
    printf '%s,%s,%s,%s,%s,%s\n' \
        "$name" "$xos" "$xof" "$sdc" "$ofp" "$osp" \
        >> "$RESULTS"
done < "$MANIFEST"

python3 "$CORPUS/render.py" "$RESULTS" "$OUT/summary.md"
cat "$OUT/summary.md"
