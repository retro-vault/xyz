#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
UPSTREAM="$CORPUS/upstream"
FRAMEWORK="$CORPUS/framework"
MANIFEST="$CORPUS/manifest.tsv"
XCC="${XCC:-$ROOT/bin/x-m/bin/xcc}"
Z88DK_ROOT="${ORIG_Z88DK_ROOT:-$ROOT/orig/z88dk}"
ZCC="$Z88DK_ROOT/bin/zcc"
TICKS="$Z88DK_ROOT/bin/z88dk-ticks"
RUNNER="$ROOT/build/bin/z80_exec"
OUT="${OUT:-$ROOT/build/x/benchmarks/z88dk24}"
CYCLES="${CYCLES:-800000000}"
FILTER="${FILTER:-}"

usage() {
    echo "usage: $0 [--filter REGEX] [--cycles N] [--outdir DIR] [--xcc PATH]"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
    --filter) FILTER="$2"; shift 2 ;;
    --cycles) CYCLES="$2"; shift 2 ;;
    --outdir) OUT="$2"; shift 2 ;;
    --xcc) XCC="$2"; shift 2 ;;
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

[[ -x "$XCC" ]] || { echo "missing M-model xcc: $XCC (run: make x-m)" >&2; exit 2; }
[[ -x "$ZCC" ]] || { echo "missing z88dk zcc: $ZCC" >&2; exit 2; }
[[ -x "$TICKS" ]] || { echo "missing z88dk-ticks: $TICKS" >&2; exit 2; }
[[ -x "$RUNNER" ]] || { echo "missing runner: $RUNNER" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/work"
RESULTS="$OUT/results.csv"
printf 'benchmark,xcc_Os_status,xcc_Os_bytes,xcc_Os_cycles,xcc_Of_status,xcc_Of_bytes,xcc_Of_cycles,sccz80_status,sccz80_bytes,sccz80_cycles,sdcc_status,sdcc_bytes,sdcc_cycles,80cc_fp_status,80cc_fp_bytes,80cc_fp_cycles,80cc_sp_status,80cc_sp_bytes,80cc_sp_cycles\n' > "$RESULTS"

decode_fixture() {
    local src="$1" dst="$2"
    if base64 --decode "$src" > "$dst" 2>/dev/null; then return; fi
    base64 -D "$src" > "$dst"
}

run_xcc_mode() {
    local name="$1" rel="$2" mode="$3" work="$4"
    local src="$UPSTREAM/$rel"
    local build_src="$src"
    local bin="$work/xcc_$mode.bin" map="$work/xcc_$mode.map"
    local log="$work/xcc_$mode.build.log" output="$work/xcc_$mode.output"
    local run_log="$work/xcc_$mode.run.log"
    local summary status bytes cycles ret done
    [[ "$name" == md5 ]] && build_src="$CORPUS/compat/md5.c"
    if ! "$XCC" "-$mode" --platform=emu --oformat=binary \
        -I"$FRAMEWORK" -DNO_LOG_RUNNING -DNO_LOG_PASSED \
        -Map="$map" "$FRAMEWORK/test.c" "$build_src" -o "$bin" >"$log" 2>&1; then
        printf 'BUILD,0,0\n'; return
    fi
    bytes="$(wc -c < "$bin" | tr -d ' ')"
    set +e
    summary="$(cd "$work" && "$RUNNER" --bin --cycles "$CYCLES" \
        --fs-root "$work" --stdout "$output" "$bin" 2>&1)"
    set -e
    printf '%s\n' "$summary" > "$run_log"
    done="$(sed -n 's/.*done=\([0-9][0-9]*\).*/\1/p' <<< "$summary" | tail -1)"
    ret="$(sed -n 's/.*return=\([0-9][0-9]*\).*/\1/p' <<< "$summary" | tail -1)"
    cycles="$(sed -n 's/.*cycles=\([0-9][0-9]*\).*/\1/p' <<< "$summary" | tail -1)"
    if [[ "$done" == 1 && "$ret" == 0 ]] && grep -qE '[0-9]+ run, [0-9]+ passed, 0 failed' "$output"; then
        status=OK
    elif [[ "$done" == 1 ]]; then
        status=FAIL
    else
        status=HANG
    fi
    printf '%s,%s,%s\n' "$status" "$bytes" "${cycles:-0}"
}

run_z88dk_mode() {
    local name="$1" rel="$2" label="$3" flags="$4" work="$5"
    local src="$UPSTREAM/$rel" bin="$work/$label.bin"
    local log="$work/$label.build.log" run_log="$work/$label.run.log"
    local output status bytes cycles
    local -a args
    read -r -a args <<< "$flags"
    if ! env Z88DK_ROOT="$Z88DK_ROOT" ZCCCFG="$Z88DK_ROOT/lib/config" \
        PATH="$Z88DK_ROOT/bin:$PATH" "$ZCC" +test -vn "${args[@]}" \
        -I"$FRAMEWORK" -DNO_LOG_RUNNING -DNO_LOG_PASSED \
        "$FRAMEWORK/test.c" "$src" -o "$bin" -m >"$log" 2>&1; then
        printf 'BUILD,0,0\n'; return
    fi
    bytes="$(wc -c < "$bin" | tr -d ' ')"
    set +e
    output="$(cd "$work" && "$TICKS" -w 120 -b msx "$bin" 2>&1)"
    set -e
    printf '%s\n' "$output" > "$run_log"
    cycles="$(sed -n 's/.*Ticks: \([0-9][0-9]*\).*/\1/p' <<< "$output" | tail -1)"
    if grep -qE '[0-9]+ run, [0-9]+ passed, 0 failed' <<< "$output"; then
        status=OK
    elif grep -qE '[1-9][0-9]* failed' <<< "$output"; then
        status=FAIL
    elif [[ -z "$cycles" ]]; then
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
    xos="$(run_xcc_mode "$name" "$rel" Os "$work")"
    xof="$(run_xcc_mode "$name" "$rel" Of "$work")"
    scc="$(run_z88dk_mode "$name" "$rel" sccz80 '-compiler=sccz80' "$work")"
    sdc="$(run_z88dk_mode "$name" "$rel" sdcc '-compiler=sdcc' "$work")"
    ofp="$(run_z88dk_mode "$name" "$rel" 80cc_fp '-compiler=80cc -Cc-frameix' "$work")"
    osp="$(run_z88dk_mode "$name" "$rel" 80cc_sp '-compiler=80cc' "$work")"
    printf '%s,%s,%s,%s,%s,%s,%s\n' "$name" "$xos" "$xof" "$scc" "$sdc" "$ofp" "$osp" >> "$RESULTS"
done < "$MANIFEST"

python3 "$CORPUS/render.py" "$RESULTS" "$OUT/summary.md"
cat "$OUT/summary.md"
