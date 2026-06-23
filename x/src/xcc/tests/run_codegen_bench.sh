#!/usr/bin/env bash
#
# run_codegen_bench.sh — compare xcc and SDCC codegen on exec-suite inputs.
#
# This runner measures translation-unit code size only. It does not link a
# CRT, does not pull in runtime helper objects, and does not execute the
# result. Each test is compiled to an XL4/ASxxxx object and the `_CODE` area
# size is recorded.
#
# Default benchmark:
#   - suite: src/xc/xcc/tests/data/exec/int
#   - xcc  : -O0, -O1, -O2, -Of, -O3, -Os
#   - sdcc : --opt-code-size, --opt-code-speed
#
# Outputs:
#   build/xc/xcc/bench/codegen/<suite>/
#     results.csv
#     summary.md
#     versions.txt
#     work/...
#
# Usage:
#   bash src/xc/xcc/tests/run_codegen_bench.sh ./bin/x/bin/xcc
#   bash src/xc/xcc/tests/run_codegen_bench.sh ./bin/x/bin/xcc --suite all
#
set -euo pipefail

XCC_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "$XCC_ROOT/../.." && pwd)"
TEST_ROOT="$XCC_ROOT/tests/data/exec"
INCLUDE_DIR="$TEST_ROOT/include"
DEFAULT_XCC="$REPO_ROOT/bin/x/bin/xcc"

XCC="$DEFAULT_XCC"
SUITE="int"
OUTDIR=""
FILTER=""
XCC_ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

usage() {
    cat <<EOF
Usage: $0 [path-to-xcc] [options]

Compare generated _CODE bytes for xcc and SDCC without linking any runtime.

Arguments:
  path-to-xcc         Optional path to the xcc binary.
                      Default: $DEFAULT_XCC

Options:
  --suite <name>      Exec suite to benchmark: int, long, float, runtime, all.
                      Default: int
  --outdir <dir>      Output directory for CSV, summary, and work files.
                      Default: $REPO_ROOT/build/xc/xcc/bench/codegen/<suite>
  --filter <regex>    Only benchmark tests whose relative path matches regex.
  --help              Show this help.
EOF
}

die() {
    echo "${RED}ERROR:${RESET} $*" >&2
    exit 1
}

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

run_xcc() {
    env ASAN_OPTIONS="$XCC_ASAN_OPTIONS" "$XCC" "$@"
}

version_line() {
    local line=""

    if line="$(run_xcc --version 2>/dev/null | head -1)"; then
        :
    else
        line=""
    fi

    if [[ -z "$line" ]]; then
        line="$(basename "$XCC")"
    fi

    printf '%s\n' "$line"
}

parse_args() {
    if [[ $# -gt 0 && "$1" != --* ]]; then
        XCC="$1"
        shift
    fi

    while [[ $# -gt 0 ]]; do
        case "$1" in
        --suite)
            [[ $# -ge 2 ]] || die "--suite requires a value"
            SUITE="$2"
            shift 2
            ;;
        --outdir)
            [[ $# -ge 2 ]] || die "--outdir requires a value"
            OUTDIR="$2"
            shift 2
            ;;
        --filter)
            [[ $# -ge 2 ]] || die "--filter requires a value"
            FILTER="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
        esac
    done
}

collect_tests() {
    if [[ "$SUITE" == "all" ]]; then
        find "$TEST_ROOT" -mindepth 2 -maxdepth 2 -type f -name '*.c' | sort
    else
        [[ -d "$TEST_ROOT/$SUITE" ]] || die "unknown suite '$SUITE'"
        find "$TEST_ROOT/$SUITE" -maxdepth 1 -type f -name '*.c' | sort
    fi
}

extra_opts_for_test() {
    local c_file="$1"
    local base="${c_file%.c}"

    if [[ -f "${base}.opts" ]]; then
        cat "${base}.opts"
    fi
}

code_size_from_xl4() {
    local obj="$1"
    local size

    size="$(awk '
        /^A _CODE size / {
            print $4;
            found = 1;
            exit 0;
        }
        END {
            if (!found) exit 1;
        }
    ' "$obj")" \
        || die "could not read _CODE size from $obj"
    printf '%s\n' "$((16#$size))"
}

check_xcc_abi() {
    local asm_file="$1"
    grep -q 'sdcccall(1) prologue' "$asm_file" \
        || die "expected sdcccall(1) prologue comment in $asm_file"
}

check_sdcc_abi() {
    local obj="$1"
    grep -q '^O .*sdcccall(1)' "$obj" \
        || die "expected sdcccall(1) metadata in $obj"
}

bench_xcc_mode() {
    local c_file="$1"
    local workdir="$2"
    local base="$3"
    local opt="$4"
    local asm_file="$workdir/$base.$opt.s"
    local rel_file="$workdir/$base.$opt.rel"
    local extra_raw
    local -a extra_args=()

    extra_raw="$(extra_opts_for_test "$c_file")"
    if [[ -n "$extra_raw" ]]; then
        read -r -a extra_args <<< "$extra_raw"
    fi

    run_xcc -S "-$opt" "${extra_args[@]}" "-I$INCLUDE_DIR" "$c_file" -o "$asm_file" >/dev/null
    check_xcc_abi "$asm_file"
    sdasz80 -o "$rel_file" "$asm_file" >/dev/null
    code_size_from_xl4 "$rel_file"
}

bench_sdcc_mode() {
    local c_file="$1"
    local workdir="$2"
    local base="$3"
    local mode="$4"
    local obj_file="$workdir/$base.sdcc_$mode.o"
    local err_file="$workdir/$base.sdcc_$mode.err"
    local opt_flag

    case "$mode" in
    size)  opt_flag="--opt-code-size" ;;
    speed) opt_flag="--opt-code-speed" ;;
    *) die "unknown SDCC benchmark mode: $mode" ;;
    esac

    if ! sdcc -mz80 --sdcccall 1 "$opt_flag" "-I$INCLUDE_DIR" \
            -c "$c_file" -o "$obj_file" > /dev/null 2>"$err_file"; then
        echo "WARN: SDCC $mode failed for ${c_file#$TEST_ROOT/}; recorded n/a" >&2
        printf 'n/a\n'
        return 0
    fi
    if ! grep -q '^O .*sdcccall(1)' "$obj_file"; then
        echo "WARN: SDCC $mode missing sdcccall(1) metadata for ${c_file#$TEST_ROOT/}; recorded n/a" >&2
        printf 'n/a\n'
        return 0
    fi
    code_size_from_xl4 "$obj_file"
}

pct() {
    local num="$1"
    local den="$2"
    awk -v num="$num" -v den="$den" 'BEGIN { if (den == 0) printf "0.00"; else printf "%.2f", (100.0 * num) / den; }'
}

parse_args "$@"

[[ -x "$XCC" ]] || die "xcc not found at '$XCC'"
need_cmd sdasz80
need_cmd sdcc
need_cmd perl
need_cmd awk
need_cmd sort

if [[ -z "$OUTDIR" ]]; then
    OUTDIR="$REPO_ROOT/build/xc/xcc/bench/codegen/$SUITE"
fi

WORKDIR="$OUTDIR/work"
RESULTS_CSV="$OUTDIR/results.csv"
SUMMARY_MD="$OUTDIR/summary.md"
VERSIONS_TXT="$OUTDIR/versions.txt"
rm -rf "$OUTDIR"
mkdir -p "$WORKDIR"

mapfile -t TESTS < <(collect_tests)
[[ "${#TESTS[@]}" -gt 0 ]] || die "no tests found"

if [[ -n "$FILTER" ]]; then
    filtered=()
    for c_file in "${TESTS[@]}"; do
        rel="${c_file#$TEST_ROOT/}"
        if [[ "$rel" =~ $FILTER ]]; then
            filtered+=("$c_file")
        fi
    done
    TESTS=("${filtered[@]}")
fi

[[ "${#TESTS[@]}" -gt 0 ]] || die "no tests matched the selected suite/filter"

{
    echo "suite,test,xcc_O0,xcc_O1,xcc_O2,xcc_Of,xcc_O3,xcc_Os,sdcc_size,sdcc_speed"
} > "$RESULTS_CSV"

{
    echo "date=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "suite=$SUITE"
    echo "filter=${FILTER:-<none>}"
    echo "xcc=$XCC"
    echo "xcc_version=$(version_line)"
    echo "sdcc_version=$(sdcc --version | head -1)"
} > "$VERSIONS_TXT"

total_o0=0
total_o1=0
total_o2=0
total_of=0
total_o3=0
total_os=0
total_sdcc_size=0
total_sdcc_speed=0
total_o2_sdcc_size_common=0
total_o2_sdcc_speed_common=0
sdcc_size_count=0
sdcc_speed_count=0
os_better=0
os_equal=0
os_worse=0
o3_better=0
o3_equal=0
o3_worse=0
count=0

for c_file in "${TESTS[@]}"; do
    rel="${c_file#$TEST_ROOT/}"
    suite_name="${rel%%/*}"
    base="$(basename "${c_file%.c}")"
    test_workdir="$WORKDIR/$suite_name/$base"
    mkdir -p "$test_workdir"

    echo "BENCH $rel"

    x0="$(bench_xcc_mode "$c_file" "$test_workdir" "$base" O0)"
    x1="$(bench_xcc_mode "$c_file" "$test_workdir" "$base" O1)"
    x2="$(bench_xcc_mode "$c_file" "$test_workdir" "$base" O2)"
    xf="$(bench_xcc_mode "$c_file" "$test_workdir" "$base" Of)"
    x3="$(bench_xcc_mode "$c_file" "$test_workdir" "$base" O3)"
    xs="$(bench_xcc_mode "$c_file" "$test_workdir" "$base" Os)"
    ss="$(bench_sdcc_mode "$c_file" "$test_workdir" "$base" size)"
    sp="$(bench_sdcc_mode "$c_file" "$test_workdir" "$base" speed)"

    echo "$suite_name,$base,$x0,$x1,$x2,$xf,$x3,$xs,$ss,$sp" >> "$RESULTS_CSV"

    total_o0=$((total_o0 + x0))
    total_o1=$((total_o1 + x1))
    total_o2=$((total_o2 + x2))
    total_of=$((total_of + xf))
    total_o3=$((total_o3 + x3))
    total_os=$((total_os + xs))
    if [[ "$ss" =~ ^[0-9]+$ ]]; then
        total_sdcc_size=$((total_sdcc_size + ss))
        total_o2_sdcc_size_common=$((total_o2_sdcc_size_common + x2))
        sdcc_size_count=$((sdcc_size_count + 1))
    fi
    if [[ "$sp" =~ ^[0-9]+$ ]]; then
        total_sdcc_speed=$((total_sdcc_speed + sp))
        total_o2_sdcc_speed_common=$((total_o2_sdcc_speed_common + x2))
        sdcc_speed_count=$((sdcc_speed_count + 1))
    fi
    count=$((count + 1))

    if (( xs < x2 )); then
        os_better=$((os_better + 1))
    elif (( xs == x2 )); then
        os_equal=$((os_equal + 1))
    else
        os_worse=$((os_worse + 1))
    fi

    if (( x3 < xs )); then
        o3_better=$((o3_better + 1))
    elif (( x3 == xs )); then
        o3_equal=$((o3_equal + 1))
    else
        o3_worse=$((o3_worse + 1))
    fi
done

o2_reduction="$(pct $((total_o0 - total_o2)) "$total_o0")"
o3_vs_os="$(pct $((total_os - total_o3)) "$total_os")"
of_vs_os="$(pct $((total_os - total_of)) "$total_os")"
sdcc_size_adv="$(pct $((total_o2_sdcc_size_common - total_sdcc_size)) "$total_o2_sdcc_size_common")"
sdcc_speed_adv="$(pct $((total_o2_sdcc_speed_common - total_sdcc_speed)) "$total_o2_sdcc_speed_common")"

{
    echo "# Codegen Benchmark"
    echo
    echo "- Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "- Suite: \`$SUITE\`"
    echo "- Filter: \`${FILTER:-<none>}\`"
    echo "- Tests: \`$count\`"
    echo "- xcc: \`$(version_line)\`"
    echo "- SDCC: \`$(sdcc --version | head -1)\`"
    echo
    echo "This benchmark measures translation-unit \`_CODE\` bytes only."
    echo "It does not link a CRT or runtime helper objects."
    echo
    echo "## Totals"
    echo
    echo "| Mode | Bytes | Compiled |"
    echo "|------|-------|----------|"
    echo "| xcc -O0 | $total_o0 | $count / $count |"
    echo "| xcc -O1 | $total_o1 | $count / $count |"
    echo "| xcc -O2 | $total_o2 | $count / $count |"
    echo "| xcc -Of | $total_of | $count / $count |"
    echo "| xcc -O3 | $total_o3 | $count / $count |"
    echo "| xcc -Os | $total_os | $count / $count |"
    echo "| sdcc --opt-code-size | $total_sdcc_size | $sdcc_size_count / $count |"
    echo "| sdcc --opt-code-speed | $total_sdcc_speed | $sdcc_speed_count / $count |"
    echo
    echo "## Relative"
    echo
    echo "- xcc -O2 vs -O0: \`$o2_reduction%\` smaller"
    echo "- xcc -O3 vs -Os: \`$o3_vs_os%\` smaller"
    echo "- xcc -Of vs -Os: \`$of_vs_os%\` smaller"
    echo "- SDCC size vs xcc -O2: \`$sdcc_size_adv%\` smaller on \`$sdcc_size_count\` common tests"
    echo "- SDCC speed vs xcc -O2: \`$sdcc_speed_adv%\` smaller on \`$sdcc_speed_count\` common tests"
    echo
    echo "## Os vs O2"
    echo
    echo "- better: \`$os_better\`"
    echo "- equal: \`$os_equal\`"
    echo "- worse: \`$os_worse\`"
    echo
    echo "## O3 vs Os"
    echo
    echo "- better: \`$o3_better\`"
    echo "- equal: \`$o3_equal\`"
    echo "- worse: \`$o3_worse\`"
    echo
    echo "## Files"
    echo
    echo "- CSV: \`$RESULTS_CSV\`"
    echo "- Summary: \`$SUMMARY_MD\`"
    echo "- Tool versions: \`$VERSIONS_TXT\`"
    echo "- Work dir: \`$WORKDIR\`"
} > "$SUMMARY_MD"

echo
echo "${GREEN}Benchmark complete.${RESET}"
echo "Summary: $SUMMARY_MD"
echo "CSV:     $RESULTS_CSV"
echo
cat "$SUMMARY_MD"
