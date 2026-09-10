#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
UPSTREAM="$CORPUS/upstream"
FRAMEWORK="$CORPUS/framework"
MANIFEST="$CORPUS/manifest.tsv"
LOCK="$CORPUS/latest.lock"

# shellcheck disable=SC1090
source "$LOCK"
SNAPSHOT_TOOLCHAINS="${LATEST_TOOLCHAINS:-$ROOT/build/toolchains/$TOOLCHAIN_SNAPSHOT}"

Z88DK_CURRENT="${Z88DK_CURRENT:-$SNAPSHOT_TOOLCHAINS/nightly-source/z88dk}"
Z88DK_80CC_CURRENT="${Z88DK_80CC_CURRENT:-$SNAPSHOT_TOOLCHAINS/nightly-source/z88dk}"
SDCC_CURRENT="${SDCC_CURRENT:-$SNAPSHOT_TOOLCHAINS/sdcc-trunk}"
XCC="${XCC:-$ROOT/bin/x-m/bin/xcc}"
OUT="${OUT:-$ROOT/build/sccz80-campaign/latest-z88dk24}"
FILTER="${FILTER:-}"
LANES="${LANES:-sccz80,xcc_Os,xcc_Of,sdcc,sdcc_max,80cc_fp,80cc_sp,zsdcc,zsdcc_max}"
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
  --z88dk DIR          pinned current z88dk checkout and target sysroot
  --sdcc DIR           pinned official SDCC trunk checkout
  --80cc DIR           pinned active 80cc branch checkout
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
    --z88dk) Z88DK_CURRENT="$2"; shift 2 ;;
    --sdcc) SDCC_CURRENT="$2"; shift 2 ;;
    --80cc) Z88DK_80CC_CURRENT="$2"; shift 2 ;;
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

SNAPSHOT_TOOLCHAINS="$(abspath "$SNAPSHOT_TOOLCHAINS")"
XCC="$(abspath "$XCC")"
Z88DK_CURRENT="$(abspath "$Z88DK_CURRENT")"
Z88DK_80CC_CURRENT="$(abspath "$Z88DK_80CC_CURRENT")"
SDCC_CURRENT="$(abspath "$SDCC_CURRENT")"
OUT="$(abspath "$OUT")"

[[ "$Z88DK_CURRENT" == "$SNAPSHOT_TOOLCHAINS/nightly-source/z88dk" &&
   "$Z88DK_80CC_CURRENT" == "$SNAPSHOT_TOOLCHAINS/nightly-source/z88dk" &&
   "$SDCC_CURRENT" == "$SNAPSHOT_TOOLCHAINS/sdcc-trunk" ]] || {
    echo "use LATEST_TOOLCHAINS to select a complete verified snapshot" >&2
    exit 2
}

[[ "$X_MODEL" == M ]] || {
    echo "current comparison requires X_MODEL=M, got $X_MODEL" >&2
    exit 2
}
[[ -x "$XCC" ]] || {
    echo "missing M-model xcc: $XCC (run: make x-m)" >&2
    exit 2
}
[[ -x "$Z88DK_CURRENT/bin/zcc" ]] || {
    echo "missing current z88dk: $Z88DK_CURRENT (run prepare-current.sh)" >&2
    exit 2
}
[[ -x "$Z88DK_CURRENT/bin/z88dk-ticks" ]] || {
    echo "missing current z88dk-ticks: $Z88DK_CURRENT/bin/z88dk-ticks" >&2
    exit 2
}
CC80="$Z88DK_80CC_CURRENT/bin/z88dk-80cc"
[[ -x "$CC80" ]] || CC80="$Z88DK_80CC_CURRENT/src/80cc/z88dk-80cc"
[[ -x "$CC80" ]] || {
    echo "missing current 80cc in $Z88DK_80CC_CURRENT" >&2
    exit 2
}
SDCC_BIN="$SDCC_CURRENT/src/sdcc"
[[ -x "$SDCC_BIN" ]] || {
    echo "missing current patched SDCC in $SDCC_CURRENT" >&2
    exit 2
}

check_commit() {
    local dir="$1" expected="$2" label="$3" actual
    if [[ "$dir" == "$SNAPSHOT_TOOLCHAINS/nightly-source/z88dk" ]]; then
        # Official source archives have no .git directory. The receipt verifies
        # archive/binary hashes and the matching independently fetched Git head.
        actual="$(git -C "$SNAPSHOT_TOOLCHAINS/z88dk-master" rev-parse HEAD)"
    else
        actual="$(git -C "$dir" rev-parse HEAD 2>/dev/null || true)"
    fi
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

check_commit "$Z88DK_CURRENT" "$Z88DK_CURRENT_COMMIT" "z88dk"
check_commit "$Z88DK_80CC_CURRENT" "$Z88DK_80CC_COMMIT" "80cc"
check_commit "$SDCC_CURRENT" "$SDCC_CURRENT_COMMIT" "SDCC"
check_commit "$ROOT" "$XCC_COMMIT" "XCC source tree"

check_patch() {
    local dir="$1" patch="$2" label="$3"
    if ! git -C "$dir" apply --reverse --check "$patch" 2>/dev/null; then
        echo "$label compatibility patch is missing; run prepare-current.sh" >&2
        exit 2
    fi
}

python3 "$CORPUS/latest/receipt.py" verify "$SNAPSHOT_TOOLCHAINS"
check_patch "$SDCC_CURRENT" "$CORPUS/sdcc-z88dk-current.patch" "SDCC/z88dk ABI"

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
    python3 "$CORPUS/latest/xcc_manifest.py" --verify "$ROOT" "$XCC" "$OUT/xcc-source-manifest.json"
    {
        printf 'xcc_source_manifest_sha256=%s\n' "$(sha256sum "$OUT/xcc-source-manifest.json" | awk '{print $1}')"
        printf 'corpus_commit=%s\n' "$CORPUS_COMMIT"
        printf 'corpus_sha256=%s\n' "$CORPUS_SHA256"
        printf 'z88dk_current_commit=%s\n' "$Z88DK_CURRENT_COMMIT"
        printf 'z88dk_80cc_commit=%s\n' "$Z88DK_80CC_COMMIT"
        printf 'sdcc_current_commit=%s\n' "$SDCC_CURRENT_COMMIT"
        printf 'xcc_commit=%s\n' "$XCC_COMMIT"
        printf 'x_model=%s\n' "$X_MODEL"
        printf 'machine=%s\n' "$MACHINE"
        printf 'budget=%s\n' "$BUDGET"
        printf 'lanes=%s\n' "$LANES"
        printf 'nightly_archive=%s\nnightly_sha256=%s\n' "$NIGHTLY_ARCHIVE" "$NIGHTLY_SHA256"
        "$Z88DK_CURRENT/bin/z88dk-zsdcc" --version 2>&1 | head -8 || true
        sha256sum "$XCC" "$Z88DK_CURRENT/bin/zcc" "$SDCC_BIN" "$CC80" \
            "$Z88DK_CURRENT/bin/z88dk-ticks" "$Z88DK_CURRENT/bin/z88dk-zsdcc"
        "$XCC" --version 2>&1 || true
        "$SDCC_BIN" --version 2>&1 | head -8 || true
        "$CC80" -h 2>&1 | head -3 || true
    } > "$OUT/versions.txt"
    python3 "$CORPUS/latest/render.py" "$results" "$OUT/summary.md"
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
mkdir -p "$OUT/work" "$OUT/artifacts" "$OUT/toolchain/bin"
python3 "$CORPUS/latest/xcc_manifest.py" "$ROOT" "$XCC" "$OUT/xcc-source-manifest.json"

# zcc resolves its selected compiler beside argv[0].  Keep the current z88dk
# host tools and target sysroot intact, and inject only the independently
# pinned compiler executables into a private facade.
for tool in "$Z88DK_CURRENT"/bin/*; do
    [[ -f "$tool" && -x "$tool" ]] || continue
    ln -s "$tool" "$OUT/toolchain/bin/$(basename "$tool")"
done
ln -sfn "$XCC" "$OUT/toolchain/bin/xcc"
ln -sfn "$CC80" "$OUT/toolchain/bin/z88dk-80cc"
ln -sfn "$SDCC_BIN" "$OUT/toolchain/bin/z88dk-zsdcc"
ZCC="$OUT/toolchain/bin/zcc"
mkdir -p "$OUT/nightly-toolchain/bin"
for tool in "$OUT"/toolchain/bin/*; do
    ln -s "$tool" "$OUT/nightly-toolchain/bin/$(basename "$tool")"
done
ln -sfn "$Z88DK_CURRENT/bin/z88dk-zsdcc" "$OUT/nightly-toolchain/bin/z88dk-zsdcc"
NIGHTLY_ZCC="$OUT/nightly-toolchain/bin/zcc"
TICKS="$Z88DK_CURRENT/bin/z88dk-ticks"

cp "$CORPUS/compat/xcc_current_shim.asm" "$OUT/toolchain/xcc_current_shim.asm"
"$Z88DK_CURRENT/bin/z88dk-z80asm" -s -mz80 \
    -o="$OUT/toolchain/xcc_current_shim.o" \
    "$OUT/toolchain/xcc_current_shim.asm"
XCC_SHIM="$OUT/toolchain/xcc_current_shim.o"

RESULTS="$OUT/results.csv"
printf '%s\n' 'benchmark,sccz80_status,sccz80_bytes,sccz80_cycles,xcc_Os_status,xcc_Os_bytes,xcc_Os_cycles,xcc_Of_status,xcc_Of_bytes,xcc_Of_cycles,sdcc_status,sdcc_bytes,sdcc_cycles,sdcc_max_status,sdcc_max_bytes,sdcc_max_cycles,80cc_fp_status,80cc_fp_bytes,80cc_fp_cycles,80cc_sp_status,80cc_sp_bytes,80cc_sp_cycles,zsdcc_status,zsdcc_bytes,zsdcc_cycles,zsdcc_max_status,zsdcc_max_bytes,zsdcc_max_cycles' > "$RESULTS"

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
    if [[ "$lane" == sdcc_max || "$lane" == zsdcc_max ]] &&
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

    if ! (cd "$work" && env \
        ZCCCFG="$Z88DK_CURRENT/lib/config" \
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
    scc="$(run_mode "$name" "$rel" sccz80 "$ZCC" '-compiler=sccz80' "$work")"
    xos="$(run_mode "$name" "$rel" xcc_Os "$ZCC" '-compiler=xcc -Cx-Os -Cx--runtime=z88dk-classic' "$work")"
    xof="$(run_mode "$name" "$rel" xcc_Of "$ZCC" '-compiler=xcc -Cx-Of -Cx--runtime=z88dk-classic' "$work")"
    sdc="$(run_mode "$name" "$rel" sdcc "$ZCC" '-compiler=sdcc' "$work")"
    sdm="$(run_mode "$name" "$rel" sdcc_max "$ZCC" '-compiler=sdcc -SO3 --max-allocs-per-node200000' "$work")"
    ofp="$(run_mode "$name" "$rel" 80cc_fp "$ZCC" '-compiler=80cc -Cc-fframe-pointer' "$work")"
    osp="$(run_mode "$name" "$rel" 80cc_sp "$ZCC" '-compiler=80cc' "$work")"
    zsd="$(run_mode "$name" "$rel" zsdcc "$NIGHTLY_ZCC" '-compiler=sdcc' "$work")"
    zsm="$(run_mode "$name" "$rel" zsdcc_max "$NIGHTLY_ZCC" '-compiler=sdcc -SO3 --max-allocs-per-node200000' "$work")"
    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "$name" "$scc" "$xos" "$xof" "$sdc" "$sdm" "$ofp" "$osp" "$zsd" "$zsm" \
        >> "$RESULTS"
done < "$MANIFEST"

write_report
cat "$OUT/summary.md"
