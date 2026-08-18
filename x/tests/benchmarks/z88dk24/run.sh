#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
UPSTREAM="$CORPUS/upstream"
FRAMEWORK="$CORPUS/framework"
MANIFEST="$CORPUS/manifest.tsv"
LOCK="$CORPUS/toolchains.lock"

# shellcheck disable=SC1090
source "$LOCK"

Z88DK_BASE="${Z88DK_BASE:-$ROOT/build/toolchains/z88dk-benchmark-base}"
Z88DK_NIGHTLY="${Z88DK_NIGHTLY:-$ROOT/build/toolchains/z88dk-nightly}"
Z88DK_80CC="${Z88DK_80CC:-$ROOT/build/toolchains/z88dk-80cc-latest}"
XCC="${XCC:-$ROOT/bin/x-m/bin/xcc}"
OUT="${OUT:-$ROOT/build/x/benchmarks/z88dk24-nightly-m}"
FILTER="${FILTER:-}"
LANES="${LANES:-sccz80,xcc_Os,xcc_Of,sdcc,sdcc_max,80cc_fp,80cc_sp}"
ALLOW_UNLOCKED="${ALLOW_UNLOCKED:-0}"
REPORT_ONLY=0
MACHINE="${MACHINE:-$TICKS_MACHINE}"
BUDGET="${BUDGET:-$TICKS_BUDGET}"

usage() {
    cat <<EOF
usage: $0 [options]
  --filter REGEX       run matching benchmarks only
  --lanes LIST         comma-separated lane names
  --outdir DIR         result directory
  --xcc PATH           M-model xcc executable
  --base DIR           pinned z88dk CRT/library checkout
  --nightly DIR        pinned nightly z88dk checkout
  --80cc DIR           pinned latest-80cc checkout
  --machine NAME       z88dk-ticks machine (default: $TICKS_MACHINE)
  --budget N           z88dk-ticks -w value (default: $TICKS_BUDGET)
  --report-only        regenerate versions.txt and summary.md from results.csv
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
    --filter) FILTER="$2"; shift 2 ;;
    --lanes) LANES="$2"; shift 2 ;;
    --outdir) OUT="$2"; shift 2 ;;
    --xcc) XCC="$2"; shift 2 ;;
    --base) Z88DK_BASE="$2"; shift 2 ;;
    --nightly) Z88DK_NIGHTLY="$2"; shift 2 ;;
    --80cc) Z88DK_80CC="$2"; shift 2 ;;
    --machine) MACHINE="$2"; shift 2 ;;
    --budget) BUDGET="$2"; shift 2 ;;
    --report-only) REPORT_ONLY=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

abspath() {
    local path="$1"
    if [[ "$path" = /* ]]; then
        printf '%s\n' "$path"
    else
        printf '%s/%s\n' "$ROOT" "$path"
    fi
}

XCC="$(abspath "$XCC")"
Z88DK_BASE="$(abspath "$Z88DK_BASE")"
Z88DK_NIGHTLY="$(abspath "$Z88DK_NIGHTLY")"
Z88DK_80CC="$(abspath "$Z88DK_80CC")"
OUT="$(abspath "$OUT")"

[[ "$X_MODEL" == M ]] || {
    echo "locked comparison requires X_MODEL=M, got $X_MODEL" >&2
    exit 2
}
[[ -x "$XCC" ]] || {
    echo "missing M-model xcc: $XCC (run: make x-m)" >&2
    exit 2
}
[[ -x "$Z88DK_BASE/bin/zcc" ]] || {
    echo "missing base z88dk: $Z88DK_BASE" >&2
    exit 2
}
[[ -x "$Z88DK_NIGHTLY/bin/zcc" ]] || {
    echo "missing nightly z88dk: $Z88DK_NIGHTLY" >&2
    exit 2
}
[[ -x "$Z88DK_NIGHTLY/bin/z88dk-zsdcc" ]] || {
    echo "missing nightly zsdcc; run prepare.sh or build bin/z88dk-zsdcc" >&2
    exit 2
}
[[ -x "$Z88DK_NIGHTLY/bin/z88dk-ticks" ]] || {
    echo "missing nightly z88dk-ticks: $Z88DK_NIGHTLY/bin/z88dk-ticks" >&2
    exit 2
}

CC80="$Z88DK_80CC/bin/z88dk-80cc"
[[ -x "$CC80" ]] || CC80="$Z88DK_80CC/src/80cc/z88dk-80cc"
[[ -x "$CC80" ]] || {
    echo "missing latest 80cc in $Z88DK_80CC" >&2
    exit 2
}

check_commit() {
    local dir="$1" expected="$2" label="$3" actual
    actual="$(git -C "$dir" rev-parse HEAD 2>/dev/null || true)"
    if [[ "$actual" != "$expected" ]]; then
        if [[ "$ALLOW_UNLOCKED" == 1 ]]; then
            echo "warning: $label is $actual, lock requires $expected" >&2
        else
            echo "$label commit mismatch: $actual (required $expected)" >&2
            echo "set ALLOW_UNLOCKED=1 only for an intentionally unpinned run" >&2
            exit 2
        fi
    fi
}

check_commit "$Z88DK_BASE" "$Z88DK_BASE_COMMIT" "z88dk base"
check_commit "$Z88DK_NIGHTLY" "$Z88DK_NIGHTLY_COMMIT" "z88dk nightly"
check_commit "$Z88DK_80CC" "$Z88DK_80CC_COMMIT" "80cc"
check_commit "$ROOT" "$XCC_COMMIT" "XCC source tree"

corpus_hash="$({
    cd "$CORPUS"
    sha256sum manifest.tsv framework/test.c framework/test.h \
        upstream/md5/md5test.bin.b64
    while IFS=$'\t' read -r _ rel; do
        sha256sum "upstream/$rel"
    done < manifest.tsv
} | LC_ALL=C sort | sha256sum | awk '{print $1}')"
[[ "$corpus_hash" == "$CORPUS_SHA256" ]] || {
    echo "benchmark corpus hash mismatch: $corpus_hash" >&2
    echo "required: $CORPUS_SHA256" >&2
    exit 2
}

write_report() {
    local results="$OUT/results.csv"
    [[ -f "$results" ]] || {
        echo "missing result file for --report-only: $results" >&2
        return 2
    }
    {
        printf 'corpus_commit=%s\n' "$CORPUS_COMMIT"
        printf 'corpus_sha256=%s\n' "$CORPUS_SHA256"
        printf 'z88dk_base_commit=%s\n' "$Z88DK_BASE_COMMIT"
        printf 'z88dk_nightly_commit=%s\n' "$Z88DK_NIGHTLY_COMMIT"
        printf 'z88dk_80cc_commit=%s\n' "$Z88DK_80CC_COMMIT"
        printf 'zsdcc_revision=%s\n' "$ZSDCC_REVISION"
        printf 'xcc_commit=%s\n' "$XCC_COMMIT"
        printf 'x_model=%s\n' "$X_MODEL"
        printf 'machine=%s\n' "$MACHINE"
        printf 'budget=%s\n' "$BUDGET"
        printf 'lanes=%s\n' "$LANES"
        sha256sum "$XCC" "$Z88DK_BASE/bin/zcc" "$Z88DK_NIGHTLY/bin/zcc" \
            "$Z88DK_NIGHTLY/bin/z88dk-zsdcc" "$CC80" \
            "$Z88DK_NIGHTLY/bin/z88dk-ticks"
        "$XCC" --version 2>&1 || true
        "$Z88DK_NIGHTLY/bin/z88dk-zsdcc" --version 2>&1 | head -5 || true
        "$CC80" --version 2>&1 | head -5 || true
    } > "$OUT/versions.txt"
    python3 "$CORPUS/render.py" "$results" "$OUT/summary.md" "$CORPUS/target.csv"
}

if [[ "$REPORT_ONLY" == 1 ]]; then
    write_report
    cat "$OUT/summary.md"
    exit 0
fi

case "$OUT" in
    /|"$ROOT") echo "refusing unsafe output directory: $OUT" >&2; exit 2 ;;
esac
rm -rf "$OUT"
mkdir -p "$OUT/work" "$OUT/artifacts" "$OUT/toolchain/bin" \
    "$OUT/base-bin"

# The older zcc also resolves its selected compiler next to argv[0].  Give it
# a private facade containing the frozen host tools plus the explicitly
# selected M-model XCC; do not modify the clean base checkout.
for tool in "$Z88DK_BASE"/bin/*; do
    [[ -f "$tool" && -x "$tool" ]] || continue
    ln -s "$tool" "$OUT/base-bin/$(basename "$tool")"
done
ln -sfn "$XCC" "$OUT/base-bin/xcc"
XCC_ZCC="$OUT/base-bin/zcc"

# Build an overlay sysroot: target headers, CRT and libraries stay on the
# frozen base, while compiler-specific rewrite rules follow the compiler that
# emits the assembly.  New zsdcc output is not valid with the old sdcc rules.
mkdir -p "$OUT/toolchain/lib"
for item in "$Z88DK_BASE"/lib/*; do
    ln -s "$item" "$OUT/toolchain/lib/$(basename "$item")"
done
rm "$OUT/toolchain/lib/config"
cp -R "$Z88DK_BASE/lib/config" "$OUT/toolchain/lib/config"
ln -s "$Z88DK_BASE/include" "$OUT/toolchain/include"
ln -sfn "$Z88DK_NIGHTLY/lib/sdcc" "$OUT/toolchain/lib/sdcc"
ln -sfn "$Z88DK_NIGHTLY/lib/z80rules.9" "$OUT/toolchain/lib/z80rules.9"
ln -sfn "$Z88DK_80CC/lib/80cc_rules.1" "$OUT/toolchain/lib/80cc_rules.1"
HYBRID_ZCCCFG="$OUT/toolchain/lib/config"

# zcc resolves its compiler and helper programs next to argv[0].  This small
# facade keeps the nightly tools intact and substitutes only the separately
# pinned latest 80cc executable.
for tool in "$Z88DK_NIGHTLY"/bin/*; do
    [[ -f "$tool" && -x "$tool" ]] || continue
    ln -s "$tool" "$OUT/toolchain/bin/$(basename "$tool")"
done
ln -sfn "$CC80" "$OUT/toolchain/bin/z88dk-80cc"
NIGHTLY_ZCC="$OUT/toolchain/bin/zcc"
TICKS="$Z88DK_NIGHTLY/bin/z88dk-ticks"

cp "$CORPUS/compat/xcc_benchmark_shim.asm" "$OUT/toolchain/xcc_benchmark_shim.asm"
"$Z88DK_BASE/bin/z88dk-z80asm" -s -mz80 \
    -o="$OUT/toolchain/xcc_benchmark_shim.o" \
    "$OUT/toolchain/xcc_benchmark_shim.asm"
XCC_SHIM="$OUT/toolchain/xcc_benchmark_shim.o"

RESULTS="$OUT/results.csv"
printf '%s\n' 'benchmark,sccz80_status,sccz80_bytes,sccz80_cycles,xcc_Os_status,xcc_Os_bytes,xcc_Os_cycles,xcc_Of_status,xcc_Of_bytes,xcc_Of_cycles,sdcc_status,sdcc_bytes,sdcc_cycles,sdcc_max_status,sdcc_max_bytes,sdcc_max_cycles,80cc_fp_status,80cc_fp_bytes,80cc_fp_cycles,80cc_sp_status,80cc_sp_bytes,80cc_sp_cycles' > "$RESULTS"

lane_enabled() {
    [[ ",$LANES," == *",$1,"* ]]
}

decode_fixture() {
    local src="$1" dst="$2"
    if base64 --decode "$src" > "$dst" 2>/dev/null; then return; fi
    base64 -D "$src" > "$dst"
}

stage_sources() {
    local src="$1" work="$2"
    cp "$FRAMEWORK/test.c" "$work/test.c"
    cp "$FRAMEWORK/test.h" "$work/test.h"
    cp "$src" "$work/$(basename "$src")"
    printf '%s\n' "$(basename "$src")"
}

run_mode() {
    local name="$1" rel="$2" lane="$3" zcc="$4" flags="$5" work="$6"
    local src="$UPSTREAM/$rel" bin="$work/$lane.bin"
    local build_log="$work/$lane.build.log" run_log="$work/$lane.run.log"
    local local_src result ticks failed status bytes artifact_dir
    local -a args extra_sources=()

    if ! lane_enabled "$lane"; then
        printf 'SKIP,0,0\n'
        return
    fi
    if [[ "$lane" == sdcc_max ]] &&
       [[ ! "$name" =~ ^(charbench|crcbench|intbench|ptrbench|md5|sieve)$ ]]; then
        printf 'SKIP,0,0\n'
        return
    fi

    read -r -a args <<< "$flags"
    local_src="$(stage_sources "$src" "$work")"
    if [[ "$lane" == xcc_* ]]; then
        args+=('-D__preserves_regs\(...\)=')
        extra_sources+=("$XCC_SHIM")
    fi

    local zcccfg="$Z88DK_BASE/lib/config"
    [[ "$zcc" == "$NIGHTLY_ZCC" ]] && zcccfg="$HYBRID_ZCCCFG"
    if ! (cd "$work" && env \
        ZCCCFG="$zcccfg" \
        PATH="$(dirname "$XCC"):$(dirname "$zcc"):$PATH" \
        "$zcc" +test -vn "${args[@]}" \
        -I. -DNO_LOG_RUNNING -DNO_LOG_PASSED \
        test.c "$local_src" "${extra_sources[@]}" -o "$bin" -m) \
        >"$build_log" 2>&1; then
        printf 'BUILD,0,0\n'
        return
    fi

    bytes="$(wc -c < "$bin" | tr -d ' ')"
    artifact_dir="$OUT/artifacts/$name/$lane"
    mkdir -p "$artifact_dir"
    cp "$bin" "$artifact_dir/program.bin"

    set +e
    result="$(cd "$work" && "$TICKS" -w "$BUDGET" -b "$MACHINE" "$bin" 2>&1)"
    set -e
    printf '%s\n' "$result" > "$run_log"
    ticks="$(sed -n 's/^Ticks: \([0-9][0-9]*\)$/\1/p' <<< "$result" | tail -1)"
    failed="$(sed -n 's/.*passed, \([0-9][0-9]*\) failed.*/\1/p' <<< "$result" | tail -1)"

    if [[ -n "$ticks" && "${failed:-}" == 0 ]]; then
        status=OK
    elif [[ -n "$ticks" && -n "${failed:-}" && "$failed" != 0 ]]; then
        status=FAIL
    else
        status=ERROR
    fi
    printf '%s,%s,%s\n' "$status" "$bytes" "${ticks:-0}"
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
    scc="$(run_mode "$name" "$rel" sccz80 "$Z88DK_BASE/bin/zcc" '-compiler=sccz80' "$work")"
    xos="$(run_mode "$name" "$rel" xcc_Os "$XCC_ZCC" '-compiler=xcc -Cx-Os -Cx--runtime=z88dk-classic' "$work")"
    xof="$(run_mode "$name" "$rel" xcc_Of "$XCC_ZCC" '-compiler=xcc -Cx-Of -Cx--runtime=z88dk-classic' "$work")"
    sdc="$(run_mode "$name" "$rel" sdcc "$NIGHTLY_ZCC" '-compiler=sdcc' "$work")"
    sdm="$(run_mode "$name" "$rel" sdcc_max "$NIGHTLY_ZCC" '-compiler=sdcc -SO3 --max-allocs-per-node200000' "$work")"
    ofp="$(run_mode "$name" "$rel" 80cc_fp "$NIGHTLY_ZCC" '-compiler=80cc -Cc-fframe-pointer' "$work")"
    osp="$(run_mode "$name" "$rel" 80cc_sp "$NIGHTLY_ZCC" '-compiler=80cc' "$work")"
    printf '%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "$name" "$scc" "$xos" "$xof" "$sdc" "$sdm" "$ofp" "$osp" \
        >> "$RESULTS"
done < "$MANIFEST"

write_report
cat "$OUT/summary.md"
