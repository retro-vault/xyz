#!/usr/bin/env bash
#
# Build and run numeric runtime benchmarks for fixed and floating formats.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BENCH_ROOT="$ROOT/tests/numeric_benchmarks"
INCLUDE_DIR="$BENCH_ROOT/include"
CRT0="$BENCH_ROOT/crt0.s"
XCC="$ROOT/bin/x/bin/xcc"
RUNNER="$ROOT/build/bin/z80_exec"
IHX2BIN="$ROOT/tests/runtime/tools/ihx2bin.py"
OUTDIR="$ROOT/build/numeric-benchmarks"
OPT="-O3"
CYCLES=200000000

usage() {
    cat <<EOF
Usage: $0 [options]

Options:
  --outdir <dir>   Output directory (default: $OUTDIR)
  --opt <flag>     xcc optimization flag (default: $OPT)
  --cycles <n>     Emulator cycle budget per image (default: $CYCLES)
  --help           Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
    --outdir)
        OUTDIR="$2"
        shift 2
        ;;
    --opt)
        OPT="$2"
        shift 2
        ;;
    --cycles)
        CYCLES="$2"
        shift 2
        ;;
    --help|-h)
        usage
        exit 0
        ;;
    *)
        echo "unknown option: $1" >&2
        exit 2
        ;;
    esac
done

[[ -x "$XCC" ]] || { echo "missing xcc: $XCC" >&2; exit 1; }
[[ -x "$RUNNER" ]] || { echo "missing z80_exec: $RUNNER" >&2; exit 1; }
[[ -f "$CRT0" ]] || { echo "missing CRT: $CRT0" >&2; exit 1; }

rm -rf "$OUTDIR"
mkdir -p "$OUTDIR/work"

RESULTS="$OUTDIR/results.csv"
SUMMARY="$OUTDIR/summary.md"

printf 'benchmark,kind,status,return,bytes,cycles\n' > "$RESULTS"

KINDS=(
    "fixed8_8:1"
    "fixed16_16:2"
    "fixed24_8:3"
    "float:4"
    "double:5"
)

mapfile -t BENCHES < <(find "$BENCH_ROOT" -mindepth 2 -maxdepth 2 -name main.c -type f | sort)

run_one() {
    local c_file="$1"
    local kind_name="$2"
    local kind_value="$3"
    local bench_name
    local work
    local ihx
    local bin
    local output
    local status
    local ret
    local cycles
    local bytes

    bench_name="$(basename "$(dirname "$c_file")")"
    work="$OUTDIR/work/$bench_name/$kind_name"
    mkdir -p "$work"
    ihx="$work/image.ihx"
    bin="$work/image.bin"

    if ! "$XCC" --platform=none -nostartfiles "$OPT" --oformat=ihx \
        -e __xc_test_start -I"$INCLUDE_DIR" "-DNUM_KIND=$kind_value" \
        "$CRT0" "$c_file" -lfixed -o "$ihx" >"$work/build.log" 2>&1; then
        printf '%s,%s,build_error,0,0,0\n' "$bench_name" "$kind_name" >> "$RESULTS"
        return
    fi

    python3 "$IHX2BIN" "$ihx" "$bin"
    bytes="$(wc -c < "$bin" | tr -d '[:space:]')"

    set +e
    output="$("$RUNNER" --cycles "$CYCLES" --ihx "$ihx" 2>&1)"
    rc=$?
    set -e
    printf '%s\n' "$output" > "$work/run.log"

    ret=0
    cycles=0
    if [[ "$output" =~ return=([0-9]+) ]]; then
        ret="${BASH_REMATCH[1]}"
    fi
    if [[ "$output" =~ cycles=([0-9]+) ]]; then
        cycles="${BASH_REMATCH[1]}"
    fi
    if [[ "$output" =~ done=1 ]]; then
        status="ok"
    elif [[ "$rc" -ne 0 ]]; then
        status="timeout"
    else
        status="run_error"
    fi

    printf '%s,%s,%s,%s,%s,%s\n' \
        "$bench_name" "$kind_name" "$status" "$ret" "$bytes" "$cycles" >> "$RESULTS"
}

for c_file in "${BENCHES[@]}"; do
    for kind in "${KINDS[@]}"; do
        IFS=: read -r kind_name kind_value <<< "$kind"
        run_one "$c_file" "$kind_name" "$kind_value"
    done
done

awk -F, '
    NR == 1 { next }
    {
        seen[$2] = 1
        if ($3 == "ok") {
            ok[$2]++
            bytes[$2] += $5
            cycles[$2] += $6
        } else {
            bad[$2]++
        }
    }
    END {
        print "# Numeric Benchmarks"
        print ""
        print "Optimization: `'"$OPT"'`"
        print ""
        print "| kind | ok | failures | total bytes | total cycles | bytes vs 8.8 | cycles vs 8.8 |"
        print "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"
        base_b = bytes["fixed8_8"]
        base_c = cycles["fixed8_8"]
        order[1] = "fixed8_8"
        order[2] = "fixed16_16"
        order[3] = "fixed24_8"
        order[4] = "float"
        order[5] = "double"
        for (i = 1; i <= 5; ++i) {
            k = order[i]
            db = (base_b > 0) ? (100.0 * (bytes[k] - base_b) / base_b) : 0
            dc = (base_c > 0) ? (100.0 * (cycles[k] - base_c) / base_c) : 0
            printf "| `%s` | %d | %d | %d | %d | %+0.2f%% | %+0.2f%% |\n", \
                k, ok[k], bad[k], bytes[k], cycles[k], db, dc
        }
    }
' "$RESULTS" > "$SUMMARY"

{
    echo ""
    echo "## Per Benchmark"
    echo ""
    echo "| benchmark | fixed8_8 bytes/cycles | fixed16_16 bytes/cycles | fixed24_8 bytes/cycles | float bytes/cycles | double bytes/cycles |"
    echo "| --- | ---: | ---: | ---: | ---: | ---: |"
    awk -F, '
        NR == 1 { next }
        {
            key = $1 SUBSEP $2
            cell[key] = ($3 == "ok") ? ($5 "/" $6) : $3
            benches[$1] = 1
        }
        END {
            n = 0
            for (b in benches)
                names[++n] = b
            for (i = 1; i <= n; ++i)
                for (j = i + 1; j <= n; ++j)
                    if (names[j] < names[i]) {
                        t = names[i]; names[i] = names[j]; names[j] = t
                    }
            for (i = 1; i <= n; ++i) {
                b = names[i]
                printf "| `%s` | %s | %s | %s | %s | %s |\n", b, \
                    cell[b SUBSEP "fixed8_8"], \
                    cell[b SUBSEP "fixed16_16"], \
                    cell[b SUBSEP "fixed24_8"], \
                    cell[b SUBSEP "float"], \
                    cell[b SUBSEP "double"]
            }
        }
    ' "$RESULTS"
} >> "$SUMMARY"

cat "$SUMMARY"
