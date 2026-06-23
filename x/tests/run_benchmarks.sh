#!/usr/bin/env bash
#
# run_benchmarks.sh — bare-metal executable benchmark runner for xcc and SDCC.
#
# The suite builds each benchmark as a standalone Z80 executable with the same
# tiny crt0 and no libc/stdio. Each program returns a 16-bit checksum in the
# emulator mailbox. The runner compares:
#   - payload bytes: flat binary bytes minus the shared crt0 bytes
#   - execution cycles: reported by z80_exec
#   - return checksum: must match the explicit oracle in expected.csv
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BENCH_ROOT="$REPO_ROOT/x/tests/benchmarks"
INCLUDE_DIR="$BENCH_ROOT/include"
EXPECTED_CSV="$BENCH_ROOT/expected.csv"
CRT0_S="$REPO_ROOT/x/src/xcc/tests/tools/z80emu/crt0_sdasz80.s"
IHX2BIN="$REPO_ROOT/x/tests/runtime/tools/ihx2bin.py"
RUNNER_BIN="$REPO_ROOT/build/bin/z80_exec"
DEFAULT_XCC="$REPO_ROOT/bin/x/bin/xcc"

XCC="$DEFAULT_XCC"
OUTDIR="$REPO_ROOT/build/x/benchmarks"
FILTER=""
CYCLE_LIMIT=20000000
XCC_ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

usage() {
    cat <<EOF
Usage: $0 [path-to-xcc] [options]

Build and run the bare-metal benchmark suite with xcc and SDCC.

Arguments:
  path-to-xcc         Optional path to the xcc binary.
                      Default: $DEFAULT_XCC

Options:
  --outdir <dir>      Output directory for CSV, summary, and work files.
                      Default: $OUTDIR
  --filter <regex>    Only benchmark subdirectories whose path matches regex.
  --cycles <n>        Emulator cycle budget per benchmark image.
                      Default: $CYCLE_LIMIT
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

parse_args() {
    if [[ $# -gt 0 && "$1" != --* ]]; then
        XCC="$1"
        shift
    fi

    while [[ $# -gt 0 ]]; do
        case "$1" in
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
        --cycles)
            [[ $# -ge 2 ]] || die "--cycles requires a value"
            CYCLE_LIMIT="$2"
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

collect_benchmarks() {
    find "$BENCH_ROOT" -mindepth 2 -maxdepth 2 -type f -name 'main.c' | sort
}

payload_bytes_from_ihx() {
    local ihx="$1"
    local bin="$2"
    local total

    python3 "$IHX2BIN" "$ihx" "$bin"
    total="$(wc -c < "$bin" | tr -d '[:space:]')"
    if (( total > CRT0_SIZE )); then
        printf '%s\n' "$((total - CRT0_SIZE))"
    else
        printf '0\n'
    fi
}

declare -A EXPECTED_RETURNS

load_expected_returns() {
    [[ -f "$EXPECTED_CSV" ]] || die "missing benchmark oracle file: $EXPECTED_CSV"

    while IFS=, read -r bench_name expected_return; do
        [[ "$bench_name" == "benchmark" ]] && continue
        [[ -n "$bench_name" ]] || continue
        EXPECTED_RETURNS["$bench_name"]="$expected_return"
    done < "$EXPECTED_CSV"
}

match_expected() {
    local status="$1"
    local ret="$2"
    local expected="$3"

    if [[ "$status" != "ok" ]]; then
        printf 'n/a\n'
    elif [[ "$ret" == "$expected" ]]; then
        printf 'ok\n'
    else
        printf 'wrong\n'
    fi
}

run_image() {
    local ihx="$1"
    local output
    local rc
    local done
    local ret
    local cycles

    set +e
    output="$("$RUNNER_BIN" --cycles "$CYCLE_LIMIT" --ihx "$ihx" 2>&1)"
    rc=$?
    set -e

    done=0
    ret=0
    cycles=0

    if [[ "$output" =~ done=([0-9]+) ]]; then
        done="${BASH_REMATCH[1]}"
    fi
    if [[ "$output" =~ return=([0-9]+) ]]; then
        ret="${BASH_REMATCH[1]}"
    fi
    if [[ "$output" =~ cycles=([0-9]+) ]]; then
        cycles="${BASH_REMATCH[1]}"
    fi

    if [[ "$done" == "1" ]]; then
        printf 'ok,%s,%s\n' "$ret" "$cycles"
    elif [[ "$rc" -ne 0 ]]; then
        printf 'timeout,%s,%s\n' "$ret" "$cycles"
    else
        printf 'run_error,%s,%s\n' "$ret" "$cycles"
    fi
}

build_xcc_rel() {
    local c_file="$1"
    local mode="$2"
    local out_rel="$3"
    local asm_file="${out_rel%.rel}.s"

    run_xcc -S -masm=sdasz80 "-$mode" "-I$INCLUDE_DIR" "$c_file" -o "$asm_file" >/dev/null
    sdasz80 -o "$out_rel" "$asm_file" >/dev/null
}

build_sdcc_rel() {
    local c_file="$1"
    local mode="$2"
    local out_rel="$3"
    local opt_flag

    case "$mode" in
    size)  opt_flag="--opt-code-size" ;;
    speed) opt_flag="--opt-code-speed" ;;
    *) die "unknown SDCC mode: $mode" ;;
    esac

    sdcc -mz80 --sdcccall 1 "$opt_flag" "-I$INCLUDE_DIR" -c "$c_file" -o "$out_rel" >/dev/null
}

link_benchmark() {
    local crt0_rel="$1"
    local main_rel="$2"
    local out_ihx="$3"

    sdldz80 -i "$out_ihx" "$crt0_rel" "$main_rel" >/dev/null
}

run_xcc_mode() {
    local c_file="$1"
    local mode="$2"
    local bench_dir="$3"
    local tag="xcc_${mode}"
    local main_rel="$bench_dir/main.$tag.rel"
    local ihx="$bench_dir/$tag.ihx"
    local bin="$bench_dir/$tag.bin"
    local status
    local ret
    local cycles
    local bytes

    if ! build_xcc_rel "$c_file" "$mode" "$main_rel" >/dev/null 2>&1; then
        printf 'build_error,0,0,0\n'
        return
    fi
    if ! link_benchmark "$CRT0_REL" "$main_rel" "$ihx" >/dev/null 2>&1; then
        printf 'link_error,0,0,0\n'
        return
    fi
    bytes="$(payload_bytes_from_ihx "$ihx" "$bin")"
    IFS=, read -r status ret cycles <<< "$(run_image "$ihx")"
    printf '%s,%s,%s,%s\n' "$status" "$ret" "$bytes" "$cycles"
}

run_sdcc_mode() {
    local c_file="$1"
    local mode="$2"
    local bench_dir="$3"
    local tag="sdcc_${mode}"
    local main_rel="$bench_dir/main.$tag.rel"
    local ihx="$bench_dir/$tag.ihx"
    local bin="$bench_dir/$tag.bin"
    local status
    local ret
    local cycles
    local bytes

    if ! sdcc -mz80 --sdcccall 1 --fomit-frame-pointer \
        "$( [[ "$mode" == "size" ]] && printf '%s' '--opt-code-size' || printf '%s' '--opt-code-speed' )" \
        "-I$INCLUDE_DIR" -c "$c_file" -o "$main_rel" >/dev/null 2>&1; then
        printf 'build_error,0,0,0\n'
        return
    fi
    if ! link_benchmark "$CRT0_REL" "$main_rel" "$ihx" >/dev/null 2>&1; then
        printf 'link_error,0,0,0\n'
        return
    fi
    bytes="$(payload_bytes_from_ihx "$ihx" "$bin")"
    IFS=, read -r status ret cycles <<< "$(run_image "$ihx")"
    printf '%s,%s,%s,%s\n' "$status" "$ret" "$bytes" "$cycles"
}

pct_improvement() {
    local lhs="$1"
    local rhs="$2"
    awk -v lhs="$lhs" -v rhs="$rhs" '
        BEGIN {
            if (rhs == 0) printf "0.00";
            else printf "%.2f", (100.0 * (rhs - lhs)) / rhs;
        }'
}

format_delta() {
    local lhs="$1"
    local rhs="$2"
    local positive_word="$3"
    local negative_word="$4"
    awk -v lhs="$lhs" -v rhs="$rhs" -v pos="$positive_word" -v neg="$negative_word" '
        BEGIN {
            if (rhs == 0) {
                printf "n/a";
            } else {
                delta = (100.0 * (rhs - lhs)) / rhs;
                if (delta >= 0)
                    printf "%.2f%% %s", delta, pos;
                else
                    printf "%.2f%% %s", -delta, neg;
            }
        }'
}

parse_args "$@"

[[ -x "$XCC" ]] || die "xcc not found at '$XCC'"
[[ -f "$CRT0_S" ]] || die "missing crt0: $CRT0_S"
[[ -x "$RUNNER_BIN" ]] || die "missing emulator: $RUNNER_BIN"

need_cmd sdasz80
need_cmd sdldz80
need_cmd sdcc
need_cmd python3
need_cmd awk
need_cmd sort
need_cmd wc

RESULTS_CSV="$OUTDIR/results.csv"
SUMMARY_MD="$OUTDIR/summary.md"
VERSIONS_TXT="$OUTDIR/versions.txt"
WORKDIR="$OUTDIR/work"

rm -rf "$OUTDIR"
mkdir -p "$WORKDIR"

CRT0_REL="$WORKDIR/crt0.rel"
sdasz80 -o "$CRT0_REL" "$CRT0_S" >/dev/null
CRT0_HEX="$(awk '
    /^A _CODE size / { print $4; found = 1; exit 0 }
    END { if (!found) exit 1 }
' "$CRT0_REL")" || die "could not determine crt0 _CODE size"
CRT0_SIZE="$((16#$CRT0_HEX))"

mapfile -t BENCHMARKS < <(collect_benchmarks)
[[ "${#BENCHMARKS[@]}" -gt 0 ]] || die "no benchmarks found"
load_expected_returns

if [[ -n "$FILTER" ]]; then
    filtered=()
    for c_file in "${BENCHMARKS[@]}"; do
        rel="${c_file#$BENCH_ROOT/}"
        if [[ "$rel" =~ $FILTER ]]; then
            filtered+=("$c_file")
        fi
    done
    BENCHMARKS=("${filtered[@]}")
fi

[[ "${#BENCHMARKS[@]}" -gt 0 ]] || die "no benchmarks matched the selected filter"

{
    printf 'benchmark,expected_return,'
    printf 'xcc_O2_status,xcc_O2_return,xcc_O2_match,xcc_O2_bytes,xcc_O2_cycles,'
    printf 'xcc_Of_status,xcc_Of_return,xcc_Of_match,xcc_Of_bytes,xcc_Of_cycles,'
    printf 'xcc_O3_status,xcc_O3_return,xcc_O3_match,xcc_O3_bytes,xcc_O3_cycles,'
    printf 'xcc_Os_status,xcc_Os_return,xcc_Os_match,xcc_Os_bytes,xcc_Os_cycles,'
    printf 'sdcc_size_status,sdcc_size_return,sdcc_size_match,sdcc_size_bytes,sdcc_size_cycles,'
    printf 'sdcc_speed_status,sdcc_speed_return,sdcc_speed_match,sdcc_speed_bytes,sdcc_speed_cycles\n'
} > "$RESULTS_CSV"

{
    echo "xcc: $("$XCC" --version 2>/dev/null | head -1 || basename "$XCC")"
    echo "sdcc: $(sdcc --version 2>/dev/null | head -1)"
    echo "runner: $RUNNER_BIN"
    echo "crt0_code_bytes: $CRT0_SIZE"
    echo "cycle_limit: $CYCLE_LIMIT"
    echo "benchmarks: ${#BENCHMARKS[@]}"
} > "$VERSIONS_TXT"

total_xcc_o2_bytes=0
total_xcc_o2_cycles=0
pass_xcc_o2=0
correct_xcc_o2=0
total_xcc_of_bytes=0
total_xcc_of_cycles=0
pass_xcc_of=0
correct_xcc_of=0
total_xcc_o3_bytes=0
total_xcc_o3_cycles=0
pass_xcc_o3=0
correct_xcc_o3=0
total_xcc_os_bytes=0
total_xcc_os_cycles=0
pass_xcc_os=0
correct_xcc_os=0
total_sdcc_size_bytes=0
total_sdcc_size_cycles=0
pass_sdcc_size=0
correct_sdcc_size=0
total_sdcc_speed_bytes=0
total_sdcc_speed_cycles=0
pass_sdcc_speed=0
correct_sdcc_speed=0

o2_smaller_than_sdcc_size=0
o2_faster_than_sdcc_size=0
of_smaller_than_sdcc_size=0
of_faster_than_sdcc_size=0
o3_smaller_than_sdcc_size=0
o3_faster_than_sdcc_size=0
os_smaller_than_sdcc_size=0
os_faster_than_sdcc_size=0
o2_sdcc_size_common=0
of_sdcc_size_common=0
o3_sdcc_size_common=0
os_sdcc_size_common=0
common_o2_bytes=0
common_o2_cycles=0
common_o2_sdcc_size_bytes=0
common_o2_sdcc_size_cycles=0
common_of_bytes=0
common_of_cycles=0
common_of_sdcc_size_bytes=0
common_of_sdcc_size_cycles=0
common_o3_bytes=0
common_o3_cycles=0
common_o3_sdcc_size_bytes=0
common_o3_sdcc_size_cycles=0
common_os_bytes=0
common_os_cycles=0
common_os_sdcc_size_bytes=0
common_os_sdcc_size_cycles=0
common_o3_vs_o2_bytes=0
common_o3_vs_o2_cycles=0
common_o2_vs_o3_bytes=0
common_o2_vs_o3_cycles=0
common_of_vs_o2_bytes=0
common_of_vs_o2_cycles=0
common_o2_vs_of_bytes=0
common_o2_vs_of_cycles=0
common_os_vs_o2_bytes=0
common_os_vs_o2_cycles=0
common_o2_vs_os_bytes=0
common_o2_vs_os_cycles=0
o2_of_common=0
o2_o3_common=0
o2_os_common=0

for c_file in "${BENCHMARKS[@]}"; do
    bench_name="$(basename "$(dirname "$c_file")")"
    expected_ret="${EXPECTED_RETURNS[$bench_name]-}"
    [[ -n "$expected_ret" ]] || die "missing expected return for benchmark '$bench_name'"
    bench_dir="$WORKDIR/$bench_name"
    mkdir -p "$bench_dir"

    IFS=, read -r xcc_o2_status xcc_o2_ret xcc_o2_bytes xcc_o2_cycles <<< "$(run_xcc_mode "$c_file" "O2" "$bench_dir")"
    IFS=, read -r xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles <<< "$(run_xcc_mode "$c_file" "Of" "$bench_dir")"
    IFS=, read -r xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles <<< "$(run_xcc_mode "$c_file" "O3" "$bench_dir")"
    IFS=, read -r xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles <<< "$(run_xcc_mode "$c_file" "Os" "$bench_dir")"
    IFS=, read -r sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles <<< "$(run_sdcc_mode "$c_file" "size" "$bench_dir")"
    IFS=, read -r sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles <<< "$(run_sdcc_mode "$c_file" "speed" "$bench_dir")"

    xcc_o2_match="$(match_expected "$xcc_o2_status" "$xcc_o2_ret" "$expected_ret")"
    xcc_of_match="$(match_expected "$xcc_of_status" "$xcc_of_ret" "$expected_ret")"
    xcc_o3_match="$(match_expected "$xcc_o3_status" "$xcc_o3_ret" "$expected_ret")"
    xcc_os_match="$(match_expected "$xcc_os_status" "$xcc_os_ret" "$expected_ret")"
    sdcc_size_match="$(match_expected "$sdcc_size_status" "$sdcc_size_ret" "$expected_ret")"
    sdcc_speed_match="$(match_expected "$sdcc_speed_status" "$sdcc_speed_ret" "$expected_ret")"

    echo "$bench_name,$expected_ret,$xcc_o2_status,$xcc_o2_ret,$xcc_o2_match,$xcc_o2_bytes,$xcc_o2_cycles,$xcc_of_status,$xcc_of_ret,$xcc_of_match,$xcc_of_bytes,$xcc_of_cycles,$xcc_o3_status,$xcc_o3_ret,$xcc_o3_match,$xcc_o3_bytes,$xcc_o3_cycles,$xcc_os_status,$xcc_os_ret,$xcc_os_match,$xcc_os_bytes,$xcc_os_cycles,$sdcc_size_status,$sdcc_size_ret,$sdcc_size_match,$sdcc_size_bytes,$sdcc_size_cycles,$sdcc_speed_status,$sdcc_speed_ret,$sdcc_speed_match,$sdcc_speed_bytes,$sdcc_speed_cycles" \
        >> "$RESULTS_CSV"

    if [[ "$xcc_o2_status" == "ok" ]]; then
        total_xcc_o2_bytes=$((total_xcc_o2_bytes + xcc_o2_bytes))
        total_xcc_o2_cycles=$((total_xcc_o2_cycles + xcc_o2_cycles))
        pass_xcc_o2=$((pass_xcc_o2 + 1))
        if [[ "$xcc_o2_match" == "ok" ]]; then
            correct_xcc_o2=$((correct_xcc_o2 + 1))
        fi
    fi
    if [[ "$xcc_of_status" == "ok" ]]; then
        total_xcc_of_bytes=$((total_xcc_of_bytes + xcc_of_bytes))
        total_xcc_of_cycles=$((total_xcc_of_cycles + xcc_of_cycles))
        pass_xcc_of=$((pass_xcc_of + 1))
        if [[ "$xcc_of_match" == "ok" ]]; then
            correct_xcc_of=$((correct_xcc_of + 1))
        fi
    fi
    if [[ "$xcc_o3_status" == "ok" ]]; then
        total_xcc_o3_bytes=$((total_xcc_o3_bytes + xcc_o3_bytes))
        total_xcc_o3_cycles=$((total_xcc_o3_cycles + xcc_o3_cycles))
        pass_xcc_o3=$((pass_xcc_o3 + 1))
        if [[ "$xcc_o3_match" == "ok" ]]; then
            correct_xcc_o3=$((correct_xcc_o3 + 1))
        fi
    fi
    if [[ "$xcc_os_status" == "ok" ]]; then
        total_xcc_os_bytes=$((total_xcc_os_bytes + xcc_os_bytes))
        total_xcc_os_cycles=$((total_xcc_os_cycles + xcc_os_cycles))
        pass_xcc_os=$((pass_xcc_os + 1))
        if [[ "$xcc_os_match" == "ok" ]]; then
            correct_xcc_os=$((correct_xcc_os + 1))
        fi
    fi
    if [[ "$sdcc_size_status" == "ok" ]]; then
        total_sdcc_size_bytes=$((total_sdcc_size_bytes + sdcc_size_bytes))
        total_sdcc_size_cycles=$((total_sdcc_size_cycles + sdcc_size_cycles))
        pass_sdcc_size=$((pass_sdcc_size + 1))
        if [[ "$sdcc_size_match" == "ok" ]]; then
            correct_sdcc_size=$((correct_sdcc_size + 1))
        fi
    fi
    if [[ "$sdcc_speed_status" == "ok" ]]; then
        total_sdcc_speed_bytes=$((total_sdcc_speed_bytes + sdcc_speed_bytes))
        total_sdcc_speed_cycles=$((total_sdcc_speed_cycles + sdcc_speed_cycles))
        pass_sdcc_speed=$((pass_sdcc_speed + 1))
        if [[ "$sdcc_speed_match" == "ok" ]]; then
            correct_sdcc_speed=$((correct_sdcc_speed + 1))
        fi
    fi

    if [[ "$xcc_o2_status" == "ok" && "$xcc_o2_match" == "ok" &&
          "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
        o2_sdcc_size_common=$((o2_sdcc_size_common + 1))
        common_o2_bytes=$((common_o2_bytes + xcc_o2_bytes))
        common_o2_cycles=$((common_o2_cycles + xcc_o2_cycles))
        common_o2_sdcc_size_bytes=$((common_o2_sdcc_size_bytes + sdcc_size_bytes))
        common_o2_sdcc_size_cycles=$((common_o2_sdcc_size_cycles + sdcc_size_cycles))
        if (( xcc_o2_bytes < sdcc_size_bytes )); then
            o2_smaller_than_sdcc_size=$((o2_smaller_than_sdcc_size + 1))
        fi
        if (( xcc_o2_cycles < sdcc_size_cycles )); then
            o2_faster_than_sdcc_size=$((o2_faster_than_sdcc_size + 1))
        fi
    fi
    if [[ "$xcc_of_status" == "ok" && "$xcc_of_match" == "ok" &&
          "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
        of_sdcc_size_common=$((of_sdcc_size_common + 1))
        common_of_bytes=$((common_of_bytes + xcc_of_bytes))
        common_of_cycles=$((common_of_cycles + xcc_of_cycles))
        common_of_sdcc_size_bytes=$((common_of_sdcc_size_bytes + sdcc_size_bytes))
        common_of_sdcc_size_cycles=$((common_of_sdcc_size_cycles + sdcc_size_cycles))
        if (( xcc_of_bytes < sdcc_size_bytes )); then
            of_smaller_than_sdcc_size=$((of_smaller_than_sdcc_size + 1))
        fi
        if (( xcc_of_cycles < sdcc_size_cycles )); then
            of_faster_than_sdcc_size=$((of_faster_than_sdcc_size + 1))
        fi
    fi
    if [[ "$xcc_o3_status" == "ok" && "$xcc_o3_match" == "ok" &&
          "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
        o3_sdcc_size_common=$((o3_sdcc_size_common + 1))
        common_o3_bytes=$((common_o3_bytes + xcc_o3_bytes))
        common_o3_cycles=$((common_o3_cycles + xcc_o3_cycles))
        common_o3_sdcc_size_bytes=$((common_o3_sdcc_size_bytes + sdcc_size_bytes))
        common_o3_sdcc_size_cycles=$((common_o3_sdcc_size_cycles + sdcc_size_cycles))
        if (( xcc_o3_bytes < sdcc_size_bytes )); then
            o3_smaller_than_sdcc_size=$((o3_smaller_than_sdcc_size + 1))
        fi
        if (( xcc_o3_cycles < sdcc_size_cycles )); then
            o3_faster_than_sdcc_size=$((o3_faster_than_sdcc_size + 1))
        fi
    fi
    if [[ "$xcc_os_status" == "ok" && "$xcc_os_match" == "ok" &&
          "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
        os_sdcc_size_common=$((os_sdcc_size_common + 1))
        common_os_bytes=$((common_os_bytes + xcc_os_bytes))
        common_os_cycles=$((common_os_cycles + xcc_os_cycles))
        common_os_sdcc_size_bytes=$((common_os_sdcc_size_bytes + sdcc_size_bytes))
        common_os_sdcc_size_cycles=$((common_os_sdcc_size_cycles + sdcc_size_cycles))
        if (( xcc_os_bytes < sdcc_size_bytes )); then
            os_smaller_than_sdcc_size=$((os_smaller_than_sdcc_size + 1))
        fi
        if (( xcc_os_cycles < sdcc_size_cycles )); then
            os_faster_than_sdcc_size=$((os_faster_than_sdcc_size + 1))
        fi
    fi
    if [[ "$xcc_o2_status" == "ok" && "$xcc_o2_match" == "ok" &&
          "$xcc_of_status" == "ok" && "$xcc_of_match" == "ok" ]]; then
        o2_of_common=$((o2_of_common + 1))
        common_o2_vs_of_bytes=$((common_o2_vs_of_bytes + xcc_o2_bytes))
        common_o2_vs_of_cycles=$((common_o2_vs_of_cycles + xcc_o2_cycles))
        common_of_vs_o2_bytes=$((common_of_vs_o2_bytes + xcc_of_bytes))
        common_of_vs_o2_cycles=$((common_of_vs_o2_cycles + xcc_of_cycles))
    fi
    if [[ "$xcc_o2_status" == "ok" && "$xcc_o2_match" == "ok" &&
          "$xcc_o3_status" == "ok" && "$xcc_o3_match" == "ok" ]]; then
        o2_o3_common=$((o2_o3_common + 1))
        common_o2_vs_o3_bytes=$((common_o2_vs_o3_bytes + xcc_o2_bytes))
        common_o2_vs_o3_cycles=$((common_o2_vs_o3_cycles + xcc_o2_cycles))
        common_o3_vs_o2_bytes=$((common_o3_vs_o2_bytes + xcc_o3_bytes))
        common_o3_vs_o2_cycles=$((common_o3_vs_o2_cycles + xcc_o3_cycles))
    fi
    if [[ "$xcc_o2_status" == "ok" && "$xcc_o2_match" == "ok" &&
          "$xcc_os_status" == "ok" && "$xcc_os_match" == "ok" ]]; then
        o2_os_common=$((o2_os_common + 1))
        common_o2_vs_os_bytes=$((common_o2_vs_os_bytes + xcc_o2_bytes))
        common_o2_vs_os_cycles=$((common_o2_vs_os_cycles + xcc_o2_cycles))
        common_os_vs_o2_bytes=$((common_os_vs_o2_bytes + xcc_os_bytes))
        common_os_vs_o2_cycles=$((common_os_vs_o2_cycles + xcc_os_cycles))
    fi
done

rel_o2_vs_sdcc_size="n/a"
rel_of_vs_sdcc_size="n/a"
rel_o3_vs_sdcc_size="n/a"
rel_os_vs_sdcc_size="n/a"
rel_of_vs_o2="n/a"
rel_o3_vs_o2="n/a"
rel_os_vs_o2="n/a"

if (( pass_xcc_o2 > 0 && pass_sdcc_size > 0 )); then
    if (( o2_sdcc_size_common > 0 )); then
        rel_o2_vs_sdcc_size="$(format_delta "$common_o2_bytes" "$common_o2_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_o2_cycles" "$common_o2_sdcc_size_cycles" "fewer cycles" "more cycles") on $o2_sdcc_size_common common benchmarks"
    fi
fi
if (( pass_xcc_of > 0 && pass_sdcc_size > 0 )); then
    if (( of_sdcc_size_common > 0 )); then
        rel_of_vs_sdcc_size="$(format_delta "$common_of_bytes" "$common_of_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_of_cycles" "$common_of_sdcc_size_cycles" "fewer cycles" "more cycles") on $of_sdcc_size_common common benchmarks"
    fi
fi
if (( pass_xcc_os > 0 && pass_sdcc_size > 0 )); then
    if (( os_sdcc_size_common > 0 )); then
        rel_os_vs_sdcc_size="$(format_delta "$common_os_bytes" "$common_os_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_os_cycles" "$common_os_sdcc_size_cycles" "fewer cycles" "more cycles") on $os_sdcc_size_common common benchmarks"
    fi
fi
if (( pass_xcc_o3 > 0 && pass_sdcc_size > 0 )); then
    if (( o3_sdcc_size_common > 0 )); then
        rel_o3_vs_sdcc_size="$(format_delta "$common_o3_bytes" "$common_o3_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_o3_cycles" "$common_o3_sdcc_size_cycles" "fewer cycles" "more cycles") on $o3_sdcc_size_common common benchmarks"
    fi
fi
if (( pass_xcc_of > 0 && pass_xcc_o2 > 0 )); then
    if (( o2_of_common > 0 )); then
        rel_of_vs_o2="$(format_delta "$common_of_vs_o2_bytes" "$common_o2_vs_of_bytes" "smaller" "larger"), $(format_delta "$common_of_vs_o2_cycles" "$common_o2_vs_of_cycles" "fewer cycles" "more cycles") on $o2_of_common common benchmarks"
    fi
fi
if (( pass_xcc_o3 > 0 && pass_xcc_o2 > 0 )); then
    if (( o2_o3_common > 0 )); then
        rel_o3_vs_o2="$(format_delta "$common_o3_vs_o2_bytes" "$common_o2_vs_o3_bytes" "smaller" "larger"), $(format_delta "$common_o3_vs_o2_cycles" "$common_o2_vs_o3_cycles" "fewer cycles" "more cycles") on $o2_o3_common common benchmarks"
    fi
fi
if (( pass_xcc_os > 0 && pass_xcc_o2 > 0 )); then
    if (( o2_os_common > 0 )); then
        rel_os_vs_o2="$(format_delta "$common_os_vs_o2_bytes" "$common_o2_vs_os_bytes" "smaller" "larger"), $(format_delta "$common_os_vs_o2_cycles" "$common_o2_vs_os_cycles" "fewer cycles" "more cycles") on $o2_os_common common benchmarks"
    fi
fi

cat > "$SUMMARY_MD" <<EOF
# Bare-Metal Benchmarks

Benchmarks run: ${#BENCHMARKS[@]}
Cycle limit per image: $CYCLE_LIMIT

The size numbers below are **payload bytes**:

- flat linked binary size
- minus the shared crt0 bytes ($CRT0_SIZE)
- no libc
- no \`printf\`

## Totals

| Mode | Payload Bytes | Cycles |
| --- | ---: | ---: |
| \`xcc -O2\` | $total_xcc_o2_bytes | $total_xcc_o2_cycles |
| \`xcc -Of\` | $total_xcc_of_bytes | $total_xcc_of_cycles |
| \`xcc -O3\` | $total_xcc_o3_bytes | $total_xcc_o3_cycles |
| \`xcc -Os\` | $total_xcc_os_bytes | $total_xcc_os_cycles |
| \`sdcc --opt-code-size\` | $total_sdcc_size_bytes | $total_sdcc_size_cycles |
| \`sdcc --opt-code-speed\` | $total_sdcc_speed_bytes | $total_sdcc_speed_cycles |

## Successful Runs

- \`xcc -O2\`: $pass_xcc_o2 / ${#BENCHMARKS[@]}
- \`xcc -Of\`: $pass_xcc_of / ${#BENCHMARKS[@]}
- \`xcc -O3\`: $pass_xcc_o3 / ${#BENCHMARKS[@]}
- \`xcc -Os\`: $pass_xcc_os / ${#BENCHMARKS[@]}
- \`sdcc --opt-code-size\`: $pass_sdcc_size / ${#BENCHMARKS[@]}
- \`sdcc --opt-code-speed\`: $pass_sdcc_speed / ${#BENCHMARKS[@]}

## Correct Checksums

- \`xcc -O2\`: $correct_xcc_o2 / ${#BENCHMARKS[@]}
- \`xcc -Of\`: $correct_xcc_of / ${#BENCHMARKS[@]}
- \`xcc -O3\`: $correct_xcc_o3 / ${#BENCHMARKS[@]}
- \`xcc -Os\`: $correct_xcc_os / ${#BENCHMARKS[@]}
- \`sdcc --opt-code-size\`: $correct_sdcc_size / ${#BENCHMARKS[@]}
- \`sdcc --opt-code-speed\`: $correct_sdcc_speed / ${#BENCHMARKS[@]}

## Relative

- \`xcc -O2\` vs \`sdcc --opt-code-size\`: $rel_o2_vs_sdcc_size
- \`xcc -Of\` vs \`sdcc --opt-code-size\`: $rel_of_vs_sdcc_size
- \`xcc -O3\` vs \`sdcc --opt-code-size\`: $rel_o3_vs_sdcc_size
- \`xcc -Os\` vs \`sdcc --opt-code-size\`: $rel_os_vs_sdcc_size
- \`xcc -Of\` vs \`xcc -O2\`: $rel_of_vs_o2
- \`xcc -O3\` vs \`xcc -O2\`: $rel_o3_vs_o2
- \`xcc -Os\` vs \`xcc -O2\`: $rel_os_vs_o2

## Win Counts vs SDCC Size Mode

- \`xcc -O2\` smaller on $o2_smaller_than_sdcc_size / $o2_sdcc_size_common benchmarks with both modes successful
- \`xcc -O2\` faster on $o2_faster_than_sdcc_size / $o2_sdcc_size_common benchmarks with both modes successful
- \`xcc -Of\` smaller on $of_smaller_than_sdcc_size / $of_sdcc_size_common benchmarks with both modes successful
- \`xcc -Of\` faster on $of_faster_than_sdcc_size / $of_sdcc_size_common benchmarks with both modes successful
- \`xcc -O3\` smaller on $o3_smaller_than_sdcc_size / $o3_sdcc_size_common benchmarks with both modes successful
- \`xcc -O3\` faster on $o3_faster_than_sdcc_size / $o3_sdcc_size_common benchmarks with both modes successful
- \`xcc -Os\` smaller on $os_smaller_than_sdcc_size / $os_sdcc_size_common benchmarks with both modes successful
- \`xcc -Os\` faster on $os_faster_than_sdcc_size / $os_sdcc_size_common benchmarks with both modes successful

## Outputs

- [results.csv]($(basename "$RESULTS_CSV"))
- [versions.txt]($(basename "$VERSIONS_TXT"))
- \`work/\` contains the intermediate \`.s\`, \`.rel\`, \`.ihx\`, and \`.bin\` files for inspection
EOF

echo "${GREEN}Wrote benchmark results to:${RESET} $OUTDIR"
