#!/usr/bin/env bash
#
# run_benchmarks.sh — unified X benchmark entrypoint.
#
# Runs the active benchmark suites under x/tests/benchmarks and prints their
# markdown summaries directly in the terminal.
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TEST_ROOT="$REPO_ROOT/x/tests"
BENCH_ROOT="$TEST_ROOT/benchmarks"
BARE_ROOT="$BENCH_ROOT/bare"
NUMERIC_ROOT="$BENCH_ROOT/numeric"
PORTABLE_ROOT="$BENCH_ROOT/portable"
Z88DK24_ROOT="$BENCH_ROOT/z88dk24"
BARE_INCLUDE_DIR="$BARE_ROOT/include"
NUMERIC_INCLUDE_DIR="$NUMERIC_ROOT/include"
PORTABLE_INCLUDE_DIR="$PORTABLE_ROOT/include"
EXPECTED_CSV="$BARE_ROOT/expected.csv"
PORTABLE_EXPECTED_CSV="$PORTABLE_ROOT/expected.csv"
CRT0_S="$TEST_ROOT/tests/c23/xcc/tools/z80emu/crt0_sdasz80.s"
NUMERIC_CRT0="$NUMERIC_ROOT/crt0.s"
IHX2BIN="$TEST_ROOT/tests/runtime/tools/ihx2bin.py"
RUNNER_BIN="$REPO_ROOT/build/bin/z80_exec"
PORTABLE_GENERATOR="$PORTABLE_ROOT/generate_portable_benchmarks.py"
PORTABLE_RENDERER="$PORTABLE_ROOT/render_portable_summary.py"
ORIG_Z88DK_ROOT="${ORIG_Z88DK_ROOT:-$REPO_ROOT/orig/z88dk}"
ORIG_Z88DK_ZCC="$ORIG_Z88DK_ROOT/bin/zcc"

DEFAULT_XCC=""
for candidate in \
    "$REPO_ROOT/bin/x/bin/xcc" \
    "$REPO_ROOT/x/bin/x/bin/xcc" \
    "$REPO_ROOT/bin/x-l/bin/xcc" \
    "$REPO_ROOT/bin/x-m/bin/xcc" \
    "$REPO_ROOT/bin/x-s/bin/xcc"; do
    if [[ -x "$candidate" ]]; then
        DEFAULT_XCC="$candidate"
        break
    fi
done

XLD_DEFAULT=""
for candidate in \
    "$REPO_ROOT/bin/x/bin/xld" \
    "$REPO_ROOT/x/bin/x/bin/xld" \
    "$REPO_ROOT/bin/x-l/bin/xld" \
    "$REPO_ROOT/bin/x-m/bin/xld" \
    "$REPO_ROOT/bin/x-s/bin/xld"; do
    if [[ -x "$candidate" ]]; then
        XLD_DEFAULT="$candidate"
        break
    fi
done

RUNTIME_LIB=""
for candidate in \
    "$REPO_ROOT/bin/x/z80/lib/libruntime.a" \
    "$REPO_ROOT/x/bin/x/z80/lib/libruntime.a" \
    "$REPO_ROOT/bin/x-l/z80/lib/libruntime.a" \
    "$REPO_ROOT/bin/x-m/z80/lib/libruntime.a" \
    "$REPO_ROOT/bin/x-s/z80/lib/libruntime.a"; do
    if [[ -f "$candidate" ]]; then
        RUNTIME_LIB="$candidate"
        break
    fi
done

XCC="$DEFAULT_XCC"
XLD="$XLD_DEFAULT"
OUTDIR="$REPO_ROOT/build/x/benchmarks"
SUITE="all"
FILTER=""
BARE_FILTER=""
NUMERIC_FILTER=""
PORTABLE_FILTER=""
Z88DK24_FILTER=""
BARE_CYCLES=20000000
NUMERIC_CYCLES=200000000
PORTABLE_CYCLES=800000000
Z88DK_CYCLES=800000000
NUMERIC_OPT="-Of"
XCC_ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}detect_leaks=0"
CURRENT_INCLUDE_DIR="$BARE_INCLUDE_DIR"
CURRENT_CYCLES="$BARE_CYCLES"
CURRENT_SIZE_METRIC="flat"
CURRENT_ARTIFACTS_DIR=""

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

BARE_BENCHMARKS_RUN="n/a"
NUMERIC_BENCHMARKS_RUN="n/a"
NUMERIC_KINDS_RUN="n/a"
PORTABLE_BENCHMARKS_RUN="n/a"
Z88DK24_BENCHMARKS_RUN="n/a"

declare -A EXPECTED_RETURNS

usage() {
    cat <<EOF
Usage: $0 [path-to-xcc] [options]

Run the active X benchmark suites from x/tests/benchmarks.

Arguments:
  path-to-xcc            Optional path to the xcc binary.
                         Default: $DEFAULT_XCC

Options:
  --suite <name>         Select suite: all, bare, numeric, portable, or
                         z88dk (complete upstream full programs).
                         z88dk24 is a backward-compatible alias.
                         Default: $SUITE
  --outdir <dir>         Output root for generated reports.
                         Default: $OUTDIR
  --filter <regex>       Apply the same regex filter to every suite.
  --bare-filter <regex>  Filter only bare-metal benchmarks.
  --numeric-filter <re>  Filter only numeric benchmarks.
  --portable-filter <re> Filter only portable benchmarks.
  --z88dk-filter <regex> Filter full-program z88dk benchmarks.
  --z88dk24-filter <re>  Backward-compatible alias for --z88dk-filter.
  --cycles <n>           Backward-compatible alias for --bare-cycles.
  --bare-cycles <n>      Cycle budget for bare-metal benchmarks.
                         Default: $BARE_CYCLES
  --numeric-cycles <n>   Cycle budget for numeric benchmarks.
                         Default: $NUMERIC_CYCLES
  --portable-cycles <n>  Cycle budget for portable benchmarks.
                         Default: $PORTABLE_CYCLES
  --z88dk-cycles <n>     Cycle budget for z88dk benchmarks.
                         Default: $Z88DK_CYCLES
  --opt <flag>           Backward-compatible alias for --numeric-opt.
  --numeric-opt <flag>   Optimization flag for numeric benchmarks.
                         Default: $NUMERIC_OPT
  --help                 Show this help.
EOF
}

die() {
    echo "${RED}ERROR:${RESET} $*" >&2
    exit 1
}

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

publish_artifact_copy() {
    local source="$1"
    local bench_name="$2"
    local mode_name="$3"
    local filename="$4"
    local dest_dir

    [[ -n "$CURRENT_ARTIFACTS_DIR" ]] || return 0
    [[ -f "$source" ]] || return 0

    dest_dir="$CURRENT_ARTIFACTS_DIR/$bench_name/$mode_name"
    mkdir -p "$dest_dir"
    cp "$source" "$dest_dir/$filename"
}

run_xcc() {
    env ASAN_OPTIONS="$XCC_ASAN_OPTIONS" "$XCC" "$@"
}

ensure_runner() {
    local runner_dir
    local runner_src="$TEST_ROOT/tests/c23/xcc/tools/z80emu/z80_exec.cpp"

    if [[ -x "$RUNNER_BIN" && "$RUNNER_BIN" -nt "$runner_src" ]]; then
        return
    fi

    need_cmd g++
    runner_dir="$(dirname "$RUNNER_BIN")"
    mkdir -p "$runner_dir"
    g++ -std=c++17 -I "$TEST_ROOT/tests/xcc/tools/z80emu" \
        -o "$RUNNER_BIN" "$runner_src"
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
        --bare-filter)
            [[ $# -ge 2 ]] || die "--bare-filter requires a value"
            BARE_FILTER="$2"
            shift 2
            ;;
        --numeric-filter)
            [[ $# -ge 2 ]] || die "--numeric-filter requires a value"
            NUMERIC_FILTER="$2"
            shift 2
            ;;
        --portable-filter)
            [[ $# -ge 2 ]] || die "--portable-filter requires a value"
            PORTABLE_FILTER="$2"
            shift 2
            ;;
        --z88dk-filter|--z88dk24-filter)
            [[ $# -ge 2 ]] || die "$1 requires a value"
            Z88DK24_FILTER="$2"
            shift 2
            ;;
        --cycles|--bare-cycles)
            [[ $# -ge 2 ]] || die "$1 requires a value"
            BARE_CYCLES="$2"
            shift 2
            ;;
        --numeric-cycles)
            [[ $# -ge 2 ]] || die "--numeric-cycles requires a value"
            NUMERIC_CYCLES="$2"
            shift 2
            ;;
        --portable-cycles)
            [[ $# -ge 2 ]] || die "--portable-cycles requires a value"
            PORTABLE_CYCLES="$2"
            shift 2
            ;;
        --z88dk-cycles)
            [[ $# -ge 2 ]] || die "--z88dk-cycles requires a value"
            Z88DK_CYCLES="$2"
            shift 2
            ;;
        --opt|--numeric-opt)
            [[ $# -ge 2 ]] || die "$1 requires a value"
            NUMERIC_OPT="$2"
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

run_image() {
    local cycle_limit="$1"
    local ihx="$2"
    local output
    local rc
    local done=0
    local ret=0
    local cycles=0

    set +e
    output="$("$RUNNER_BIN" --cycles "$cycle_limit" --ihx "$ihx" 2>&1)"
    rc=$?
    set -e

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

payload_bytes_from_ihx() {
    local ihx="$1"
    local bin="$2"
    local crt0_size="$3"
    local total

    python3 "$IHX2BIN" "$ihx" "$bin"
    total="$(wc -c < "$bin" | tr -d '[:space:]')"
    if (( total > crt0_size )); then
        printf '%s\n' "$((total - crt0_size))"
    else
        printf '0\n'
    fi
}

linked_non_bss_payload_bytes_from_map() {
    local map_file="$1"
    local crt0_size="$2"
    local sum=0
    local value
    local line

    while IFS= read -r line; do
        if [[ "$line" =~ ^[[:space:]]+(_CODE|_HOME|_GSINIT|_GSFINAL|_INITIALIZER|_INITIALIZED|_RODATA)[[:space:]]+[0-9A-Fa-f]+[[:space:]]+([0-9A-Fa-f]+)[[:space:]] ]]; then
            value="${BASH_REMATCH[2]}"
            sum=$((sum + 16#$value))
        fi
    done < "$map_file"

    if (( sum > crt0_size )); then
        printf '%s\n' "$((sum - crt0_size))"
    else
        printf '0\n'
    fi
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

collect_bare_benchmarks() {
    find "$BARE_ROOT" -mindepth 2 -maxdepth 2 -type f -name 'main.c' | sort
}

collect_z88dk_benchmarks() {
    find "$Z88DK_ROOT" -mindepth 2 -maxdepth 2 -type f -name 'main.c' | sort
}

collect_portable_benchmarks() {
    find "$PORTABLE_ROOT/generated" -mindepth 2 -maxdepth 2 -type f -name 'main.c' | sort
}

ensure_portable_suite_generated() {
    [[ -f "$PORTABLE_GENERATOR" ]] || die "missing portable benchmark generator: $PORTABLE_GENERATOR"
    if [[ "${PORTABLE_SKIP_GENERATE:-0}" != "1" ]]; then
        python3 "$PORTABLE_GENERATOR" >/dev/null
    fi
    [[ -f "$PORTABLE_EXPECTED_CSV" ]] || die "portable benchmark generator did not produce $PORTABLE_EXPECTED_CSV"
}

load_expected_returns() {
    load_expected_returns_from "$EXPECTED_CSV"
}

load_expected_returns_from() {
    local expected_csv="$1"

    EXPECTED_RETURNS=()
    [[ -f "$expected_csv" ]] || die "missing benchmark oracle file: $expected_csv"

    while IFS=, read -r bench_name expected_return; do
        [[ "$bench_name" == "benchmark" ]] && continue
        [[ -n "$bench_name" ]] || continue
        EXPECTED_RETURNS["$bench_name"]="$expected_return"
    done < "$expected_csv"
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

build_xcc_rel() {
    local c_file="$1"
    local mode="$2"
    local out_rel="$3"
    local asm_file="${out_rel%.rel}.s"

    run_xcc -S -masm=sdasz80 "-$mode" "-I$CURRENT_INCLUDE_DIR" "$c_file" -o "$asm_file" >/dev/null
    sdasz80 -o "$out_rel" "$asm_file" >/dev/null
}

link_benchmark() {
    local crt0_rel="$1"
    local main_rel="$2"
    local out_ihx="$3"
    local out_map="${4:-}"
    local map_args=()

    if [[ -n "$out_map" ]]; then
        map_args=("-Map=$out_map")
    fi

    "$XLD" --mode=sdcc --oformat=ihx -nostdlib -e __xc_test_start \
        -Ttext=0x0000 "${map_args[@]}" -o "$out_ihx" \
        "$crt0_rel" "$main_rel" "$RUNTIME_LIB" >/dev/null
}

run_xcc_mode() {
    local c_file="$1"
    local mode="$2"
    local bench_dir="$3"
    local crt0_rel="$4"
    local crt0_size="$5"
    local tag="xcc_${mode}"
    local main_rel="$bench_dir/main.$tag.rel"
    local ihx="$bench_dir/$tag.ihx"
    local map="$bench_dir/$tag.map"
    local bin="$bench_dir/$tag.bin"
    local status
    local ret
    local cycles
    local bytes
    local bench_name

    bench_name="$(basename "$bench_dir")"

    if ! build_xcc_rel "$c_file" "$mode" "$main_rel" >/dev/null 2>&1; then
        printf 'build_error,0,0,0\n'
        return
    fi
    if ! link_benchmark "$crt0_rel" "$main_rel" "$ihx" "$map" >/dev/null 2>&1; then
        printf 'link_error,0,0,0\n'
        return
    fi
    if [[ "$CURRENT_SIZE_METRIC" == "non_bss" ]]; then
        bytes="$(linked_non_bss_payload_bytes_from_map "$map" "$crt0_size")"
        python3 "$IHX2BIN" "$ihx" "$bin" >/dev/null
    else
        bytes="$(payload_bytes_from_ihx "$ihx" "$bin" "$crt0_size")"
    fi
    publish_artifact_copy "$ihx" "$bench_name" "$tag" "program.ihx"
    publish_artifact_copy "$bin" "$bench_name" "$tag" "program.bin"
    publish_artifact_copy "$map" "$bench_name" "$tag" "program.map"
    IFS=, read -r status ret cycles <<< "$(run_image "$CURRENT_CYCLES" "$ihx")"
    printf '%s,%s,%s,%s\n' "$status" "$ret" "$bytes" "$cycles"
}

run_sdcc_mode() {
    local c_file="$1"
    local mode="$2"
    local bench_dir="$3"
    local crt0_rel="$4"
    local crt0_size="$5"
    local tag="sdcc_${mode}"
    local main_rel="$bench_dir/main.$tag.rel"
    local ihx="$bench_dir/$tag.ihx"
    local map="$bench_dir/$tag.map"
    local bin="$bench_dir/$tag.bin"
    local status
    local ret
    local cycles
    local bytes
    local bench_name

    bench_name="$(basename "$bench_dir")"

    if ! sdcc -mz80 --sdcccall 1 --fomit-frame-pointer \
        "$( [[ "$mode" == "size" ]] && printf '%s' '--opt-code-size' || printf '%s' '--opt-code-speed' )" \
        "-I$CURRENT_INCLUDE_DIR" -c "$c_file" -o "$main_rel" >/dev/null 2>&1; then
        printf 'build_error,0,0,0\n'
        return
    fi
    if ! link_benchmark "$crt0_rel" "$main_rel" "$ihx" "$map" >/dev/null 2>&1; then
        printf 'link_error,0,0,0\n'
        return
    fi
    if [[ "$CURRENT_SIZE_METRIC" == "non_bss" ]]; then
        bytes="$(linked_non_bss_payload_bytes_from_map "$map" "$crt0_size")"
        python3 "$IHX2BIN" "$ihx" "$bin" >/dev/null
    else
        bytes="$(payload_bytes_from_ihx "$ihx" "$bin" "$crt0_size")"
    fi
    publish_artifact_copy "$ihx" "$bench_name" "$tag" "program.ihx"
    publish_artifact_copy "$bin" "$bench_name" "$tag" "program.bin"
    publish_artifact_copy "$map" "$bench_name" "$tag" "program.map"
    IFS=, read -r status ret cycles <<< "$(run_image "$CURRENT_CYCLES" "$ihx")"
    printf '%s,%s,%s,%s\n' "$status" "$ret" "$bytes" "$cycles"
}

z88dk_env() {
    env Z88DK_ROOT="$ORIG_Z88DK_ROOT" \
        ZCCCFG="$ORIG_Z88DK_ROOT/lib/config" \
        PATH="$ORIG_Z88DK_ROOT/bin:$PATH" \
        "$@"
}

z88dk_non_bss_bytes_from_map() {
    local map_file="$1"
    local sum=0
    local value
    local line

    while IFS= read -r line; do
        if [[ "$line" =~ ^__(code|data|rodata)_[[:alnum:]_]+_size[[:space:]]+=[[:space:]]+\$([0-9A-Fa-f]+) ]]; then
            value="${BASH_REMATCH[2]}"
            sum=$((sum + 16#$value))
        fi
    done < "$map_file"
    printf '%s\n' "$sum"
}

build_z88dk_baseline() {
    local compiler="$1"
    local bench_dir="$2"
    local src="$bench_dir/empty_$compiler.c"
    local out="$bench_dir/empty_$compiler"
    local map="$out.map"

    printf 'int main(void) { return 0; }\n' > "$src"
    if ! z88dk_env "$ORIG_Z88DK_ZCC" +test -vn "-compiler=$compiler" \
        -m -s "$src" -o "$out" >"$bench_dir/empty_$compiler.log" 2>&1; then
        printf '0\n'
        return
    fi
    z88dk_non_bss_bytes_from_map "$map"
}

run_z88dk_image() {
    local cycle_limit="$1"
    local bin="$2"
    local output
    local rc
    local done=0
    local ret=0
    local cycles=0

    set +e
    output="$("$RUNNER_BIN" --bin --z88dk-trap --cycles "$cycle_limit" "$bin" 2>&1)"
    rc=$?
    set -e

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

run_z88dk_mode() {
    local c_file="$1"
    local compiler="$2"
    local bench_dir="$3"
    local baseline_bytes="$4"
    local tag="z88dk_$compiler"
    local out="$bench_dir/$tag"
    local map="$out.map"
    local status
    local ret
    local cycles
    local bytes
    local total_bytes
    local bench_name

    bench_name="$(basename "$bench_dir")"

    if ! z88dk_env "$ORIG_Z88DK_ZCC" +test -vn "-compiler=$compiler" \
        "-I$CURRENT_INCLUDE_DIR" -m -s "$c_file" -o "$out" >"$bench_dir/$tag.log" 2>&1; then
        printf 'build_error,0,0,0\n'
        return
    fi

    total_bytes="$(z88dk_non_bss_bytes_from_map "$map")"
    if (( total_bytes > baseline_bytes )); then
        bytes="$((total_bytes - baseline_bytes))"
    else
        bytes=0
    fi
    publish_artifact_copy "$out" "$bench_name" "$tag" "program.bin"
    publish_artifact_copy "$map" "$bench_name" "$tag" "program.map"
    IFS=, read -r status ret cycles <<< "$(run_z88dk_image "$CURRENT_CYCLES" "$out")"
    printf '%s,%s,%s,%s\n' "$status" "$ret" "$bytes" "$cycles"
}

run_bare_suite() {
    local suite_outdir="$OUTDIR/bare"
    local results_csv="$suite_outdir/results.csv"
    local summary_md="$suite_outdir/summary.md"
    local versions_txt="$suite_outdir/versions.txt"
    local workdir="$suite_outdir/work"
    local artifacts_dir="$suite_outdir/artifacts"
    local crt0_rel
    local crt0_hex
    local crt0_size
    local -a benchmarks
    local -a filtered=()
    local c_file
    local rel
    local bench_name
    local expected_ret
    local bench_dir
    local xcc_o2_status xcc_o2_ret xcc_o2_bytes xcc_o2_cycles
    local xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles
    local xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles
    local xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles
    local sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles
    local sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles
    local xcc_o2_match xcc_of_match xcc_o3_match xcc_os_match
    local sdcc_size_match sdcc_speed_match
    local total_xcc_o2_bytes=0 total_xcc_o2_cycles=0 pass_xcc_o2=0 correct_xcc_o2=0
    local total_xcc_of_bytes=0 total_xcc_of_cycles=0 pass_xcc_of=0 correct_xcc_of=0
    local total_xcc_o3_bytes=0 total_xcc_o3_cycles=0 pass_xcc_o3=0 correct_xcc_o3=0
    local total_xcc_os_bytes=0 total_xcc_os_cycles=0 pass_xcc_os=0 correct_xcc_os=0
    local total_sdcc_size_bytes=0 total_sdcc_size_cycles=0 pass_sdcc_size=0 correct_sdcc_size=0
    local total_sdcc_speed_bytes=0 total_sdcc_speed_cycles=0 pass_sdcc_speed=0 correct_sdcc_speed=0
    local o2_smaller_than_sdcc_size=0 o2_faster_than_sdcc_size=0
    local of_smaller_than_sdcc_size=0 of_faster_than_sdcc_size=0
    local o3_smaller_than_sdcc_size=0 o3_faster_than_sdcc_size=0
    local os_smaller_than_sdcc_size=0 os_faster_than_sdcc_size=0
    local o2_sdcc_size_common=0 of_sdcc_size_common=0 o3_sdcc_size_common=0 os_sdcc_size_common=0
    local common_o2_bytes=0 common_o2_cycles=0 common_o2_sdcc_size_bytes=0 common_o2_sdcc_size_cycles=0
    local common_of_bytes=0 common_of_cycles=0 common_of_sdcc_size_bytes=0 common_of_sdcc_size_cycles=0
    local common_o3_bytes=0 common_o3_cycles=0 common_o3_sdcc_size_bytes=0 common_o3_sdcc_size_cycles=0
    local common_os_bytes=0 common_os_cycles=0 common_os_sdcc_size_bytes=0 common_os_sdcc_size_cycles=0
    local common_o3_vs_o2_bytes=0 common_o3_vs_o2_cycles=0 common_o2_vs_o3_bytes=0 common_o2_vs_o3_cycles=0
    local common_of_vs_o2_bytes=0 common_of_vs_o2_cycles=0 common_o2_vs_of_bytes=0 common_o2_vs_of_cycles=0
    local common_os_vs_o2_bytes=0 common_os_vs_o2_cycles=0 common_o2_vs_os_bytes=0 common_o2_vs_os_cycles=0
    local o2_of_common=0 o2_o3_common=0 o2_os_common=0
    local rel_o2_vs_sdcc_size="n/a" rel_of_vs_sdcc_size="n/a" rel_o3_vs_sdcc_size="n/a"
    local rel_os_vs_sdcc_size="n/a" rel_of_vs_o2="n/a" rel_o3_vs_o2="n/a" rel_os_vs_o2="n/a"

    CURRENT_INCLUDE_DIR="$BARE_INCLUDE_DIR"
    CURRENT_CYCLES="$BARE_CYCLES"
    CURRENT_SIZE_METRIC="flat"

    need_cmd sdasz80
    need_cmd "$XLD"
    need_cmd sdcc
    [[ -f "$RUNTIME_LIB" ]] || die "missing runtime library: $RUNTIME_LIB"

    rm -rf "$suite_outdir"
    mkdir -p "$workdir"
    mkdir -p "$artifacts_dir"
    CURRENT_ARTIFACTS_DIR="$artifacts_dir"

    crt0_rel="$workdir/crt0.rel"
    sdasz80 -o "$crt0_rel" "$CRT0_S" >/dev/null
    crt0_hex="$(awk '
        /^A _CODE size / { print $4; found = 1; exit 0 }
        END { if (!found) exit 1 }
    ' "$crt0_rel")" || die "could not determine crt0 _CODE size"
    crt0_size="$((16#$crt0_hex))"

    mapfile -t benchmarks < <(collect_bare_benchmarks)
    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no bare-metal benchmarks found"
    load_expected_returns

    if [[ -n "$BARE_FILTER" ]]; then
        for c_file in "${benchmarks[@]}"; do
            rel="${c_file#$BARE_ROOT/}"
            if [[ "$rel" =~ $BARE_FILTER ]]; then
                filtered+=("$c_file")
            fi
        done
        benchmarks=("${filtered[@]}")
    fi

    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no bare-metal benchmarks matched the selected filter"
    BARE_BENCHMARKS_RUN="${#benchmarks[@]}"

    {
        printf 'benchmark,expected_return,'
        printf 'xcc_O2_status,xcc_O2_return,xcc_O2_match,xcc_O2_bytes,xcc_O2_cycles,'
        printf 'xcc_Of_status,xcc_Of_return,xcc_Of_match,xcc_Of_bytes,xcc_Of_cycles,'
        printf 'xcc_O3_status,xcc_O3_return,xcc_O3_match,xcc_O3_bytes,xcc_O3_cycles,'
        printf 'xcc_Os_status,xcc_Os_return,xcc_Os_match,xcc_Os_bytes,xcc_Os_cycles,'
        printf 'sdcc_size_status,sdcc_size_return,sdcc_size_match,sdcc_size_bytes,sdcc_size_cycles,'
        printf 'sdcc_speed_status,sdcc_speed_return,sdcc_speed_match,sdcc_speed_bytes,sdcc_speed_cycles\n'
    } > "$results_csv"

    {
        echo "xcc_path: $XCC"
        echo "xcc: $("$XCC" --version 2>/dev/null | head -1 || basename "$XCC")"
        echo "sdcc: $(sdcc --version 2>/dev/null | head -1)"
        echo "runner: $RUNNER_BIN"
        echo "crt0_code_bytes: $crt0_size"
        echo "cycle_limit: $BARE_CYCLES"
        echo "benchmarks: ${#benchmarks[@]}"
    } > "$versions_txt"

    for c_file in "${benchmarks[@]}"; do
        bench_name="$(basename "$(dirname "$c_file")")"
        expected_ret="${EXPECTED_RETURNS[$bench_name]-}"
        [[ -n "$expected_ret" ]] || die "missing expected return for benchmark '$bench_name'"
        bench_dir="$workdir/$bench_name"
        mkdir -p "$bench_dir"

        IFS=, read -r xcc_o2_status xcc_o2_ret xcc_o2_bytes xcc_o2_cycles <<< "$(run_xcc_mode "$c_file" "O2" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles <<< "$(run_xcc_mode "$c_file" "Of" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles <<< "$(run_xcc_mode "$c_file" "O3" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles <<< "$(run_xcc_mode "$c_file" "Os" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles <<< "$(run_sdcc_mode "$c_file" "size" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles <<< "$(run_sdcc_mode "$c_file" "speed" "$bench_dir" "$crt0_rel" "$crt0_size")"

        xcc_o2_match="$(match_expected "$xcc_o2_status" "$xcc_o2_ret" "$expected_ret")"
        xcc_of_match="$(match_expected "$xcc_of_status" "$xcc_of_ret" "$expected_ret")"
        xcc_o3_match="$(match_expected "$xcc_o3_status" "$xcc_o3_ret" "$expected_ret")"
        xcc_os_match="$(match_expected "$xcc_os_status" "$xcc_os_ret" "$expected_ret")"
        sdcc_size_match="$(match_expected "$sdcc_size_status" "$sdcc_size_ret" "$expected_ret")"
        sdcc_speed_match="$(match_expected "$sdcc_speed_status" "$sdcc_speed_ret" "$expected_ret")"

        echo "$bench_name,$expected_ret,$xcc_o2_status,$xcc_o2_ret,$xcc_o2_match,$xcc_o2_bytes,$xcc_o2_cycles,$xcc_of_status,$xcc_of_ret,$xcc_of_match,$xcc_of_bytes,$xcc_of_cycles,$xcc_o3_status,$xcc_o3_ret,$xcc_o3_match,$xcc_o3_bytes,$xcc_o3_cycles,$xcc_os_status,$xcc_os_ret,$xcc_os_match,$xcc_os_bytes,$xcc_os_cycles,$sdcc_size_status,$sdcc_size_ret,$sdcc_size_match,$sdcc_size_bytes,$sdcc_size_cycles,$sdcc_speed_status,$sdcc_speed_ret,$sdcc_speed_match,$sdcc_speed_bytes,$sdcc_speed_cycles" \
            >> "$results_csv"

        if [[ "$xcc_o2_status" == "ok" ]]; then
            total_xcc_o2_bytes=$((total_xcc_o2_bytes + xcc_o2_bytes))
            total_xcc_o2_cycles=$((total_xcc_o2_cycles + xcc_o2_cycles))
            pass_xcc_o2=$((pass_xcc_o2 + 1))
            [[ "$xcc_o2_match" == "ok" ]] && correct_xcc_o2=$((correct_xcc_o2 + 1))
        fi
        if [[ "$xcc_of_status" == "ok" ]]; then
            total_xcc_of_bytes=$((total_xcc_of_bytes + xcc_of_bytes))
            total_xcc_of_cycles=$((total_xcc_of_cycles + xcc_of_cycles))
            pass_xcc_of=$((pass_xcc_of + 1))
            [[ "$xcc_of_match" == "ok" ]] && correct_xcc_of=$((correct_xcc_of + 1))
        fi
        if [[ "$xcc_o3_status" == "ok" ]]; then
            total_xcc_o3_bytes=$((total_xcc_o3_bytes + xcc_o3_bytes))
            total_xcc_o3_cycles=$((total_xcc_o3_cycles + xcc_o3_cycles))
            pass_xcc_o3=$((pass_xcc_o3 + 1))
            [[ "$xcc_o3_match" == "ok" ]] && correct_xcc_o3=$((correct_xcc_o3 + 1))
        fi
        if [[ "$xcc_os_status" == "ok" ]]; then
            total_xcc_os_bytes=$((total_xcc_os_bytes + xcc_os_bytes))
            total_xcc_os_cycles=$((total_xcc_os_cycles + xcc_os_cycles))
            pass_xcc_os=$((pass_xcc_os + 1))
            [[ "$xcc_os_match" == "ok" ]] && correct_xcc_os=$((correct_xcc_os + 1))
        fi
        if [[ "$sdcc_size_status" == "ok" ]]; then
            total_sdcc_size_bytes=$((total_sdcc_size_bytes + sdcc_size_bytes))
            total_sdcc_size_cycles=$((total_sdcc_size_cycles + sdcc_size_cycles))
            pass_sdcc_size=$((pass_sdcc_size + 1))
            [[ "$sdcc_size_match" == "ok" ]] && correct_sdcc_size=$((correct_sdcc_size + 1))
        fi
        if [[ "$sdcc_speed_status" == "ok" ]]; then
            total_sdcc_speed_bytes=$((total_sdcc_speed_bytes + sdcc_speed_bytes))
            total_sdcc_speed_cycles=$((total_sdcc_speed_cycles + sdcc_speed_cycles))
            pass_sdcc_speed=$((pass_sdcc_speed + 1))
            [[ "$sdcc_speed_match" == "ok" ]] && correct_sdcc_speed=$((correct_sdcc_speed + 1))
        fi

        if [[ "$xcc_o2_status" == "ok" && "$xcc_o2_match" == "ok" &&
              "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
            o2_sdcc_size_common=$((o2_sdcc_size_common + 1))
            common_o2_bytes=$((common_o2_bytes + xcc_o2_bytes))
            common_o2_cycles=$((common_o2_cycles + xcc_o2_cycles))
            common_o2_sdcc_size_bytes=$((common_o2_sdcc_size_bytes + sdcc_size_bytes))
            common_o2_sdcc_size_cycles=$((common_o2_sdcc_size_cycles + sdcc_size_cycles))
            (( xcc_o2_bytes < sdcc_size_bytes )) && o2_smaller_than_sdcc_size=$((o2_smaller_than_sdcc_size + 1))
            (( xcc_o2_cycles < sdcc_size_cycles )) && o2_faster_than_sdcc_size=$((o2_faster_than_sdcc_size + 1))
        fi
        if [[ "$xcc_of_status" == "ok" && "$xcc_of_match" == "ok" &&
              "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
            of_sdcc_size_common=$((of_sdcc_size_common + 1))
            common_of_bytes=$((common_of_bytes + xcc_of_bytes))
            common_of_cycles=$((common_of_cycles + xcc_of_cycles))
            common_of_sdcc_size_bytes=$((common_of_sdcc_size_bytes + sdcc_size_bytes))
            common_of_sdcc_size_cycles=$((common_of_sdcc_size_cycles + sdcc_size_cycles))
            (( xcc_of_bytes < sdcc_size_bytes )) && of_smaller_than_sdcc_size=$((of_smaller_than_sdcc_size + 1))
            (( xcc_of_cycles < sdcc_size_cycles )) && of_faster_than_sdcc_size=$((of_faster_than_sdcc_size + 1))
        fi
        if [[ "$xcc_o3_status" == "ok" && "$xcc_o3_match" == "ok" &&
              "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
            o3_sdcc_size_common=$((o3_sdcc_size_common + 1))
            common_o3_bytes=$((common_o3_bytes + xcc_o3_bytes))
            common_o3_cycles=$((common_o3_cycles + xcc_o3_cycles))
            common_o3_sdcc_size_bytes=$((common_o3_sdcc_size_bytes + sdcc_size_bytes))
            common_o3_sdcc_size_cycles=$((common_o3_sdcc_size_cycles + sdcc_size_cycles))
            (( xcc_o3_bytes < sdcc_size_bytes )) && o3_smaller_than_sdcc_size=$((o3_smaller_than_sdcc_size + 1))
            (( xcc_o3_cycles < sdcc_size_cycles )) && o3_faster_than_sdcc_size=$((o3_faster_than_sdcc_size + 1))
        fi
        if [[ "$xcc_os_status" == "ok" && "$xcc_os_match" == "ok" &&
              "$sdcc_size_status" == "ok" && "$sdcc_size_match" == "ok" ]]; then
            os_sdcc_size_common=$((os_sdcc_size_common + 1))
            common_os_bytes=$((common_os_bytes + xcc_os_bytes))
            common_os_cycles=$((common_os_cycles + xcc_os_cycles))
            common_os_sdcc_size_bytes=$((common_os_sdcc_size_bytes + sdcc_size_bytes))
            common_os_sdcc_size_cycles=$((common_os_sdcc_size_cycles + sdcc_size_cycles))
            (( xcc_os_bytes < sdcc_size_bytes )) && os_smaller_than_sdcc_size=$((os_smaller_than_sdcc_size + 1))
            (( xcc_os_cycles < sdcc_size_cycles )) && os_faster_than_sdcc_size=$((os_faster_than_sdcc_size + 1))
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

    if (( o2_sdcc_size_common > 0 )); then
        rel_o2_vs_sdcc_size="$(format_delta "$common_o2_bytes" "$common_o2_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_o2_cycles" "$common_o2_sdcc_size_cycles" "fewer cycles" "more cycles") on $o2_sdcc_size_common common benchmarks"
    fi
    if (( of_sdcc_size_common > 0 )); then
        rel_of_vs_sdcc_size="$(format_delta "$common_of_bytes" "$common_of_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_of_cycles" "$common_of_sdcc_size_cycles" "fewer cycles" "more cycles") on $of_sdcc_size_common common benchmarks"
    fi
    if (( o3_sdcc_size_common > 0 )); then
        rel_o3_vs_sdcc_size="$(format_delta "$common_o3_bytes" "$common_o3_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_o3_cycles" "$common_o3_sdcc_size_cycles" "fewer cycles" "more cycles") on $o3_sdcc_size_common common benchmarks"
    fi
    if (( os_sdcc_size_common > 0 )); then
        rel_os_vs_sdcc_size="$(format_delta "$common_os_bytes" "$common_os_sdcc_size_bytes" "smaller" "larger"), $(format_delta "$common_os_cycles" "$common_os_sdcc_size_cycles" "fewer cycles" "more cycles") on $os_sdcc_size_common common benchmarks"
    fi
    if (( o2_of_common > 0 )); then
        rel_of_vs_o2="$(format_delta "$common_of_vs_o2_bytes" "$common_o2_vs_of_bytes" "smaller" "larger"), $(format_delta "$common_of_vs_o2_cycles" "$common_o2_vs_of_cycles" "fewer cycles" "more cycles") on $o2_of_common common benchmarks"
    fi
    if (( o2_o3_common > 0 )); then
        rel_o3_vs_o2="$(format_delta "$common_o3_vs_o2_bytes" "$common_o2_vs_o3_bytes" "smaller" "larger"), $(format_delta "$common_o3_vs_o2_cycles" "$common_o2_vs_o3_cycles" "fewer cycles" "more cycles") on $o2_o3_common common benchmarks"
    fi
    if (( o2_os_common > 0 )); then
        rel_os_vs_o2="$(format_delta "$common_os_vs_o2_bytes" "$common_o2_vs_os_bytes" "smaller" "larger"), $(format_delta "$common_os_vs_o2_cycles" "$common_o2_vs_os_cycles" "fewer cycles" "more cycles") on $o2_os_common common benchmarks"
    fi

    cat > "$summary_md" <<EOF
# Bare-Metal Benchmarks

Benchmarks run: ${#benchmarks[@]}
Cycle limit per image: $BARE_CYCLES

The size numbers below are **payload bytes**:

- flat linked binary size
- minus the shared crt0 bytes ($crt0_size)
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

- \`xcc -O2\`: $pass_xcc_o2 / ${#benchmarks[@]}
- \`xcc -Of\`: $pass_xcc_of / ${#benchmarks[@]}
- \`xcc -O3\`: $pass_xcc_o3 / ${#benchmarks[@]}
- \`xcc -Os\`: $pass_xcc_os / ${#benchmarks[@]}
- \`sdcc --opt-code-size\`: $pass_sdcc_size / ${#benchmarks[@]}
- \`sdcc --opt-code-speed\`: $pass_sdcc_speed / ${#benchmarks[@]}

## Correct Checksums

- \`xcc -O2\`: $correct_xcc_o2 / ${#benchmarks[@]}
- \`xcc -Of\`: $correct_xcc_of / ${#benchmarks[@]}
- \`xcc -O3\`: $correct_xcc_o3 / ${#benchmarks[@]}
- \`xcc -Os\`: $correct_xcc_os / ${#benchmarks[@]}
- \`sdcc --opt-code-size\`: $correct_sdcc_size / ${#benchmarks[@]}
- \`sdcc --opt-code-speed\`: $correct_sdcc_speed / ${#benchmarks[@]}

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

- [results.csv](results.csv)
- [versions.txt](versions.txt)
- [artifacts/](artifacts/)
- \`work/\` contains the intermediate \`.s\`, \`.rel\`, \`.ihx\`, and \`.bin\` files for inspection
EOF

    {
        echo ""
        echo "## Per Benchmark"
        echo ""
        echo "| benchmark | xcc -O2 | xcc -Of | xcc -O3 | xcc -Os | sdcc size | sdcc speed |"
        echo "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"
        awk -F, '
            NR == 1 { next }

            function cell(status, verdict, bytes, cycles, text) {
                if (status == "ok") {
                    text = bytes "/" cycles;
                    if (verdict != "ok")
                        text = text " (" verdict ")";
                    return text;
                }
                return status;
            }

            {
                printf "| `%s` | %s | %s | %s | %s | %s | %s |\n", \
                    $1, \
                    cell($3,  $5,  $6,  $7), \
                    cell($8,  $10, $11, $12), \
                    cell($13, $15, $16, $17), \
                    cell($18, $20, $21, $22), \
                    cell($23, $25, $26, $27), \
                    cell($28, $30, $31, $32)
            }
        ' "$results_csv"
    } >> "$summary_md"

    cat "$summary_md"
    echo ""
    echo "${GREEN}Wrote bare benchmark results to:${RESET} $suite_outdir"
    CURRENT_ARTIFACTS_DIR=""
}

run_z88dk_suite() {
    local suite_outdir="$OUTDIR/z88dk"
    local results_csv="$suite_outdir/results.csv"
    local summary_md="$suite_outdir/summary.md"
    local versions_txt="$suite_outdir/versions.txt"
    local workdir="$suite_outdir/work"
    local artifacts_dir="$suite_outdir/artifacts"
    local crt0_rel
    local crt0_hex
    local crt0_size
    local -a benchmarks
    local -a filtered=()
    local c_file
    local rel
    local bench_name
    local expected_ret
    local bench_dir
    local xcc_o2_status xcc_o2_ret xcc_o2_bytes xcc_o2_cycles
    local xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles
    local xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles
    local xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles
    local sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles
    local sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles
    local z88dk_sdcc_status z88dk_sdcc_ret z88dk_sdcc_bytes z88dk_sdcc_cycles
    local z88dk_80cc_status z88dk_80cc_ret z88dk_80cc_bytes z88dk_80cc_cycles
    local z88dk_sccz80_status z88dk_sccz80_ret z88dk_sccz80_bytes z88dk_sccz80_cycles
    local xcc_o2_match xcc_of_match xcc_o3_match xcc_os_match
    local sdcc_size_match sdcc_speed_match
    local z88dk_sdcc_match z88dk_80cc_match z88dk_sccz80_match
    local z88dk_sdcc_baseline=0 z88dk_80cc_baseline=0 z88dk_sccz80_baseline=0

    CURRENT_INCLUDE_DIR="$Z88DK_INCLUDE_DIR"
    CURRENT_CYCLES="$Z88DK_CYCLES"
    CURRENT_SIZE_METRIC="non_bss"

    need_cmd sdasz80
    need_cmd "$XLD"
    need_cmd sdcc
    [[ -x "$ORIG_Z88DK_ZCC" ]] || die "missing local z88dk zcc: $ORIG_Z88DK_ZCC"
    [[ -f "$ORIG_Z88DK_ROOT/lib/clibs/z80/test_clib.lib" ]] || die "missing z88dk test library: $ORIG_Z88DK_ROOT/lib/clibs/z80/test_clib.lib"
    [[ -f "$ORIG_Z88DK_ROOT/lib/clibs/z80/z80_crt0.lib" ]] || die "missing z88dk crt library: $ORIG_Z88DK_ROOT/lib/clibs/z80/z80_crt0.lib"
    [[ -f "$RUNTIME_LIB" ]] || die "missing runtime library: $RUNTIME_LIB"

    rm -rf "$suite_outdir"
    mkdir -p "$workdir"
    mkdir -p "$artifacts_dir"
    CURRENT_ARTIFACTS_DIR="$artifacts_dir"

    z88dk_sdcc_baseline="$(build_z88dk_baseline "sdcc" "$workdir")"
    z88dk_80cc_baseline="$(build_z88dk_baseline "80cc" "$workdir")"
    z88dk_sccz80_baseline="$(build_z88dk_baseline "sccz80" "$workdir")"

    crt0_rel="$workdir/crt0.rel"
    sdasz80 -o "$crt0_rel" "$CRT0_S" >/dev/null
    crt0_hex="$(awk '
        /^A _CODE size / { print $4; found = 1; exit 0 }
        END { if (!found) exit 1 }
    ' "$crt0_rel")" || die "could not determine crt0 _CODE size"
    crt0_size="$((16#$crt0_hex))"

    mapfile -t benchmarks < <(collect_z88dk_benchmarks)
    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no z88dk benchmarks found"
    load_expected_returns_from "$Z88DK_EXPECTED_CSV"

    if [[ -n "$Z88DK_FILTER" ]]; then
        for c_file in "${benchmarks[@]}"; do
            rel="${c_file#$Z88DK_ROOT/}"
            if [[ "$rel" =~ $Z88DK_FILTER ]]; then
                filtered+=("$c_file")
            fi
        done
        benchmarks=("${filtered[@]}")
    fi

    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no z88dk benchmarks matched the selected filter"
    Z88DK_BENCHMARKS_RUN="${#benchmarks[@]}"

    {
        printf 'benchmark,expected_return,'
        printf 'xcc_O2_status,xcc_O2_return,xcc_O2_match,xcc_O2_bytes,xcc_O2_cycles,'
        printf 'xcc_Of_status,xcc_Of_return,xcc_Of_match,xcc_Of_bytes,xcc_Of_cycles,'
        printf 'xcc_O3_status,xcc_O3_return,xcc_O3_match,xcc_O3_bytes,xcc_O3_cycles,'
        printf 'xcc_Os_status,xcc_Os_return,xcc_Os_match,xcc_Os_bytes,xcc_Os_cycles,'
        printf 'sdcc_size_status,sdcc_size_return,sdcc_size_match,sdcc_size_bytes,sdcc_size_cycles,'
        printf 'sdcc_speed_status,sdcc_speed_return,sdcc_speed_match,sdcc_speed_bytes,sdcc_speed_cycles,'
        printf 'z88dk_sdcc_status,z88dk_sdcc_return,z88dk_sdcc_match,z88dk_sdcc_bytes,z88dk_sdcc_cycles,'
        printf 'z88dk_80cc_status,z88dk_80cc_return,z88dk_80cc_match,z88dk_80cc_bytes,z88dk_80cc_cycles,'
        printf 'z88dk_sccz80_status,z88dk_sccz80_return,z88dk_sccz80_match,z88dk_sccz80_bytes,z88dk_sccz80_cycles\n'
    } > "$results_csv"

    {
        echo "xcc_path: $XCC"
        echo "xcc: $("$XCC" --version 2>/dev/null | head -1 || basename "$XCC")"
        echo "sdcc: $(sdcc --version 2>/dev/null | head -1)"
        echo "z88dk: $("$ORIG_Z88DK_ZCC" -h 2>&1 | sed -n '/^zcc /p' | head -1 || basename "$ORIG_Z88DK_ZCC")"
        echo "z88dk_root: $ORIG_Z88DK_ROOT"
        echo "xld: $("$XLD" --version 2>/dev/null | head -1 || basename "$XLD")"
        echo "runtime: $RUNTIME_LIB"
        echo "crt0_code_bytes: $crt0_size"
        echo "z88dk_sdcc_non_bss_baseline_bytes: $z88dk_sdcc_baseline"
        echo "z88dk_80cc_non_bss_baseline_bytes: $z88dk_80cc_baseline"
        echo "z88dk_sccz80_non_bss_baseline_bytes: $z88dk_sccz80_baseline"
        echo "cycle_limit: $Z88DK_CYCLES"
        echo "benchmarks: ${#benchmarks[@]}"
    } > "$versions_txt"

    for c_file in "${benchmarks[@]}"; do
        bench_name="$(basename "$(dirname "$c_file")")"
        expected_ret="${EXPECTED_RETURNS[$bench_name]-}"
        [[ -n "$expected_ret" ]] || die "missing expected return for z88dk benchmark '$bench_name'"
        bench_dir="$workdir/$bench_name"
        mkdir -p "$bench_dir"

        IFS=, read -r xcc_o2_status xcc_o2_ret xcc_o2_bytes xcc_o2_cycles <<< "$(run_xcc_mode "$c_file" "O2" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles <<< "$(run_xcc_mode "$c_file" "Of" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles <<< "$(run_xcc_mode "$c_file" "O3" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles <<< "$(run_xcc_mode "$c_file" "Os" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles <<< "$(run_sdcc_mode "$c_file" "size" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles <<< "$(run_sdcc_mode "$c_file" "speed" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r z88dk_sdcc_status z88dk_sdcc_ret z88dk_sdcc_bytes z88dk_sdcc_cycles <<< "$(run_z88dk_mode "$c_file" "sdcc" "$bench_dir" "$z88dk_sdcc_baseline")"
        IFS=, read -r z88dk_80cc_status z88dk_80cc_ret z88dk_80cc_bytes z88dk_80cc_cycles <<< "$(run_z88dk_mode "$c_file" "80cc" "$bench_dir" "$z88dk_80cc_baseline")"
        IFS=, read -r z88dk_sccz80_status z88dk_sccz80_ret z88dk_sccz80_bytes z88dk_sccz80_cycles <<< "$(run_z88dk_mode "$c_file" "sccz80" "$bench_dir" "$z88dk_sccz80_baseline")"

        xcc_o2_match="$(match_expected "$xcc_o2_status" "$xcc_o2_ret" "$expected_ret")"
        xcc_of_match="$(match_expected "$xcc_of_status" "$xcc_of_ret" "$expected_ret")"
        xcc_o3_match="$(match_expected "$xcc_o3_status" "$xcc_o3_ret" "$expected_ret")"
        xcc_os_match="$(match_expected "$xcc_os_status" "$xcc_os_ret" "$expected_ret")"
        sdcc_size_match="$(match_expected "$sdcc_size_status" "$sdcc_size_ret" "$expected_ret")"
        sdcc_speed_match="$(match_expected "$sdcc_speed_status" "$sdcc_speed_ret" "$expected_ret")"
        z88dk_sdcc_match="$(match_expected "$z88dk_sdcc_status" "$z88dk_sdcc_ret" "$expected_ret")"
        z88dk_80cc_match="$(match_expected "$z88dk_80cc_status" "$z88dk_80cc_ret" "$expected_ret")"
        z88dk_sccz80_match="$(match_expected "$z88dk_sccz80_status" "$z88dk_sccz80_ret" "$expected_ret")"

        echo "$bench_name,$expected_ret,$xcc_o2_status,$xcc_o2_ret,$xcc_o2_match,$xcc_o2_bytes,$xcc_o2_cycles,$xcc_of_status,$xcc_of_ret,$xcc_of_match,$xcc_of_bytes,$xcc_of_cycles,$xcc_o3_status,$xcc_o3_ret,$xcc_o3_match,$xcc_o3_bytes,$xcc_o3_cycles,$xcc_os_status,$xcc_os_ret,$xcc_os_match,$xcc_os_bytes,$xcc_os_cycles,$sdcc_size_status,$sdcc_size_ret,$sdcc_size_match,$sdcc_size_bytes,$sdcc_size_cycles,$sdcc_speed_status,$sdcc_speed_ret,$sdcc_speed_match,$sdcc_speed_bytes,$sdcc_speed_cycles,$z88dk_sdcc_status,$z88dk_sdcc_ret,$z88dk_sdcc_match,$z88dk_sdcc_bytes,$z88dk_sdcc_cycles,$z88dk_80cc_status,$z88dk_80cc_ret,$z88dk_80cc_match,$z88dk_80cc_bytes,$z88dk_80cc_cycles,$z88dk_sccz80_status,$z88dk_sccz80_ret,$z88dk_sccz80_match,$z88dk_sccz80_bytes,$z88dk_sccz80_cycles" \
            >> "$results_csv"
    done

    awk -F, -v cycle_limit="$Z88DK_CYCLES" '
        function add(mode, status, verdict, bytes, cycles) {
            seen[mode]++
            if (status == "ok") {
                ok[mode]++
                if (verdict == "ok") {
                    correct[mode]++
                    total_bytes[mode] += bytes
                    total_cycles[mode] += cycles
                }
            } else {
                bad[mode]++
            }
        }

        NR == 1 { next }
        {
            add("xcc -O2", $3, $5, $6, $7)
            add("xcc -Of", $8, $10, $11, $12)
            add("xcc -O3", $13, $15, $16, $17)
            add("xcc -Os", $18, $20, $21, $22)
            add("sdcc size", $23, $25, $26, $27)
            add("sdcc speed", $28, $30, $31, $32)
            add("z88dk sdcc", $33, $35, $36, $37)
            add("z88dk 80cc", $38, $40, $41, $42)
            add("z88dk sccz80", $43, $45, $46, $47)

            if ($18 == "ok" && $20 == "ok" && $23 == "ok" && $25 == "ok") {
                os_common++
                os_bytes += $21
                os_cycles += $22
                sdcc_bytes += $26
                sdcc_cycles += $27
                if ($21 < $26) os_smaller++
                if ($22 < $27) os_faster++
            }
        }
        END {
            print "# z88dk Benchmarks"
            print ""
            print "Cycle limit per image: " cycle_limit
            print ""
            print "These are libc-free adaptations of the z88dk compiler-comparison kernels."
            print "The runner links startup plus `libruntime.a`, but not `libc.a`."
            print "`xcc`/SDCC payload bytes are linked non-BSS code/initializer bytes from xld maps, minus crt0."
            print "z88dk modes use local `orig/z88dk` with the `+test` target; their payload bytes are non-BSS code/data/rodata minus an empty-main baseline."
            print ""
            print "## Totals"
            print ""
            print "| Mode | Correct | Payload Bytes | Cycles |"
            print "| --- | ---: | ---: | ---: |"
            order[1] = "xcc -O2"
            order[2] = "xcc -Of"
            order[3] = "xcc -O3"
            order[4] = "xcc -Os"
            order[5] = "sdcc size"
            order[6] = "sdcc speed"
            order[7] = "z88dk sdcc"
            order[8] = "z88dk 80cc"
            order[9] = "z88dk sccz80"
            for (i = 1; i <= 9; ++i) {
                mode = order[i]
                printf "| `%s` | %d/%d | %d | %d |\n", \
                    mode, correct[mode], seen[mode], total_bytes[mode], total_cycles[mode]
            }
            print ""
            print "## Relative"
            print ""
            if (os_common > 0) {
                size_delta = 100.0 * (sdcc_bytes - os_bytes) / sdcc_bytes
                cycle_delta = 100.0 * (sdcc_cycles - os_cycles) / sdcc_cycles
                printf "- `xcc -Os` vs `sdcc --opt-code-size`: %+0.2f%% bytes, %+0.2f%% cycles on %d common passing benchmarks\n", size_delta, cycle_delta, os_common
                printf "- `xcc -Os` wins size on %d/%d and speed on %d/%d common passing benchmarks\n", os_smaller, os_common, os_faster, os_common
            } else {
                print "- `xcc -Os` vs `sdcc --opt-code-size`: n/a"
            }
        }
    ' "$results_csv" > "$summary_md"

    {
        echo ""
        echo "## Per Benchmark"
        echo ""
        echo "| benchmark | xcc -O2 | xcc -Of | xcc -O3 | xcc -Os | sdcc size | sdcc speed | z88dk sdcc | z88dk 80cc | z88dk sccz80 |"
        echo "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"
        awk -F, '
            NR == 1 { next }

            function cell(status, verdict, bytes, cycles, text) {
                if (status == "ok") {
                    text = bytes "/" cycles;
                    if (verdict != "ok")
                        text = text " (" verdict ")";
                    return text;
                }
                return status;
            }

            {
                printf "| `%s` | %s | %s | %s | %s | %s | %s | %s | %s | %s |\n", \
                    $1, \
                    cell($3,  $5,  $6,  $7), \
                    cell($8,  $10, $11, $12), \
                    cell($13, $15, $16, $17), \
                    cell($18, $20, $21, $22), \
                    cell($23, $25, $26, $27), \
                    cell($28, $30, $31, $32), \
                    cell($33, $35, $36, $37), \
                    cell($38, $40, $41, $42), \
                    cell($43, $45, $46, $47)
            }
        ' "$results_csv"
        echo ""
        echo "## Outputs"
        echo ""
        echo "- [results.csv](results.csv)"
        echo "- [versions.txt](versions.txt)"
        echo "- [artifacts/](artifacts/)"
        echo "- \`work/\` contains the intermediate \`.s\`, \`.rel\`, \`.ihx\`, and \`.bin\` files for inspection"
    } >> "$summary_md"

    cat "$summary_md"
    echo ""
    echo "${GREEN}Wrote z88dk benchmark results to:${RESET} $suite_outdir"
    CURRENT_ARTIFACTS_DIR=""
}

run_portable_suite() {
    local suite_outdir="$OUTDIR/portable"
    local results_csv="$suite_outdir/results.csv"
    local summary_md="$suite_outdir/summary.md"
    local versions_txt="$suite_outdir/versions.txt"
    local workdir="$suite_outdir/work"
    local artifacts_dir="$suite_outdir/artifacts"
    local crt0_rel
    local crt0_hex
    local crt0_size
    local -a benchmarks
    local -a filtered=()
    local c_file
    local rel
    local bench_name
    local expected_ret
    local bench_dir
    local xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles
    local xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles
    local xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles
    local sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles
    local sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles
    local z88dk_sdcc_status z88dk_sdcc_ret z88dk_sdcc_bytes z88dk_sdcc_cycles
    local z88dk_80cc_status z88dk_80cc_ret z88dk_80cc_bytes z88dk_80cc_cycles
    local z88dk_sccz80_status z88dk_sccz80_ret z88dk_sccz80_bytes z88dk_sccz80_cycles
    local xcc_of_match xcc_o3_match xcc_os_match
    local sdcc_size_match sdcc_speed_match
    local z88dk_sdcc_match z88dk_80cc_match z88dk_sccz80_match
    local z88dk_sdcc_baseline=0 z88dk_80cc_baseline=0 z88dk_sccz80_baseline=0

    CURRENT_INCLUDE_DIR="$PORTABLE_INCLUDE_DIR"
    CURRENT_CYCLES="$PORTABLE_CYCLES"
    CURRENT_SIZE_METRIC="non_bss"

    need_cmd sdasz80
    need_cmd "$XLD"
    need_cmd sdcc
    [[ -x "$ORIG_Z88DK_ZCC" ]] || die "missing local z88dk zcc: $ORIG_Z88DK_ZCC"
    [[ -f "$ORIG_Z88DK_ROOT/lib/clibs/z80/test_clib.lib" ]] || die "missing z88dk test library: $ORIG_Z88DK_ROOT/lib/clibs/z80/test_clib.lib"
    [[ -f "$ORIG_Z88DK_ROOT/lib/clibs/z80/z80_crt0.lib" ]] || die "missing z88dk crt library: $ORIG_Z88DK_ROOT/lib/clibs/z80/z80_crt0.lib"
    [[ -f "$RUNTIME_LIB" ]] || die "missing runtime library: $RUNTIME_LIB"

    ensure_portable_suite_generated

    rm -rf "$suite_outdir"
    mkdir -p "$workdir"
    mkdir -p "$artifacts_dir"
    CURRENT_ARTIFACTS_DIR="$artifacts_dir"

    z88dk_sdcc_baseline="$(build_z88dk_baseline "sdcc" "$workdir")"
    z88dk_80cc_baseline="$(build_z88dk_baseline "80cc" "$workdir")"
    z88dk_sccz80_baseline="$(build_z88dk_baseline "sccz80" "$workdir")"

    crt0_rel="$workdir/crt0.rel"
    sdasz80 -o "$crt0_rel" "$CRT0_S" >/dev/null
    crt0_hex="$(awk '
        /^A _CODE size / { print $4; found = 1; exit 0 }
        END { if (!found) exit 1 }
    ' "$crt0_rel")" || die "could not determine crt0 _CODE size"
    crt0_size="$((16#$crt0_hex))"

    mapfile -t benchmarks < <(collect_portable_benchmarks)
    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no portable benchmarks found"
    load_expected_returns_from "$PORTABLE_EXPECTED_CSV"

    if [[ -n "$PORTABLE_FILTER" ]]; then
        for c_file in "${benchmarks[@]}"; do
            rel="${c_file#$PORTABLE_ROOT/}"
            if [[ "$rel" =~ $PORTABLE_FILTER ]]; then
                filtered+=("$c_file")
            fi
        done
        benchmarks=("${filtered[@]}")
    fi

    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no portable benchmarks matched the selected filter"
    PORTABLE_BENCHMARKS_RUN="${#benchmarks[@]}"

    {
        printf 'benchmark,expected_return,'
        printf 'xcc_Of_status,xcc_Of_return,xcc_Of_match,xcc_Of_bytes,xcc_Of_cycles,'
        printf 'xcc_O3_status,xcc_O3_return,xcc_O3_match,xcc_O3_bytes,xcc_O3_cycles,'
        printf 'xcc_Os_status,xcc_Os_return,xcc_Os_match,xcc_Os_bytes,xcc_Os_cycles,'
        printf 'sdcc_size_status,sdcc_size_return,sdcc_size_match,sdcc_size_bytes,sdcc_size_cycles,'
        printf 'sdcc_speed_status,sdcc_speed_return,sdcc_speed_match,sdcc_speed_bytes,sdcc_speed_cycles,'
        printf 'z88dk_sdcc_status,z88dk_sdcc_return,z88dk_sdcc_match,z88dk_sdcc_bytes,z88dk_sdcc_cycles,'
        printf 'z88dk_80cc_status,z88dk_80cc_return,z88dk_80cc_match,z88dk_80cc_bytes,z88dk_80cc_cycles,'
        printf 'z88dk_sccz80_status,z88dk_sccz80_return,z88dk_sccz80_match,z88dk_sccz80_bytes,z88dk_sccz80_cycles\n'
    } > "$results_csv"

    {
        echo "xcc_path: $XCC"
        echo "xcc: $("$XCC" --version 2>/dev/null | head -1 || basename "$XCC")"
        echo "sdcc: $(sdcc --version 2>/dev/null | head -1)"
        echo "z88dk: $("$ORIG_Z88DK_ZCC" -h 2>&1 | sed -n '/^zcc /p' | head -1 || basename "$ORIG_Z88DK_ZCC")"
        echo "z88dk_root: $ORIG_Z88DK_ROOT"
        echo "xld: $("$XLD" --version 2>/dev/null | head -1 || basename "$XLD")"
        echo "runtime: $RUNTIME_LIB"
        echo "generator: $PORTABLE_GENERATOR"
        echo "crt0_code_bytes: $crt0_size"
        echo "z88dk_sdcc_non_bss_baseline_bytes: $z88dk_sdcc_baseline"
        echo "z88dk_80cc_non_bss_baseline_bytes: $z88dk_80cc_baseline"
        echo "z88dk_sccz80_non_bss_baseline_bytes: $z88dk_sccz80_baseline"
        echo "cycle_limit: $PORTABLE_CYCLES"
        echo "benchmarks: ${#benchmarks[@]}"
    } > "$versions_txt"

    for c_file in "${benchmarks[@]}"; do
        bench_name="$(basename "$(dirname "$c_file")")"
        expected_ret="${EXPECTED_RETURNS[$bench_name]-}"
        [[ -n "$expected_ret" ]] || die "missing expected return for portable benchmark '$bench_name'"
        bench_dir="$workdir/$bench_name"
        mkdir -p "$bench_dir"

        IFS=, read -r xcc_of_status xcc_of_ret xcc_of_bytes xcc_of_cycles <<< "$(run_xcc_mode "$c_file" "Of" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_o3_status xcc_o3_ret xcc_o3_bytes xcc_o3_cycles <<< "$(run_xcc_mode "$c_file" "O3" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r xcc_os_status xcc_os_ret xcc_os_bytes xcc_os_cycles <<< "$(run_xcc_mode "$c_file" "Os" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r sdcc_size_status sdcc_size_ret sdcc_size_bytes sdcc_size_cycles <<< "$(run_sdcc_mode "$c_file" "size" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r sdcc_speed_status sdcc_speed_ret sdcc_speed_bytes sdcc_speed_cycles <<< "$(run_sdcc_mode "$c_file" "speed" "$bench_dir" "$crt0_rel" "$crt0_size")"
        IFS=, read -r z88dk_sdcc_status z88dk_sdcc_ret z88dk_sdcc_bytes z88dk_sdcc_cycles <<< "$(run_z88dk_mode "$c_file" "sdcc" "$bench_dir" "$z88dk_sdcc_baseline")"
        IFS=, read -r z88dk_80cc_status z88dk_80cc_ret z88dk_80cc_bytes z88dk_80cc_cycles <<< "$(run_z88dk_mode "$c_file" "80cc" "$bench_dir" "$z88dk_80cc_baseline")"
        IFS=, read -r z88dk_sccz80_status z88dk_sccz80_ret z88dk_sccz80_bytes z88dk_sccz80_cycles <<< "$(run_z88dk_mode "$c_file" "sccz80" "$bench_dir" "$z88dk_sccz80_baseline")"

        xcc_of_match="$(match_expected "$xcc_of_status" "$xcc_of_ret" "$expected_ret")"
        xcc_o3_match="$(match_expected "$xcc_o3_status" "$xcc_o3_ret" "$expected_ret")"
        xcc_os_match="$(match_expected "$xcc_os_status" "$xcc_os_ret" "$expected_ret")"
        sdcc_size_match="$(match_expected "$sdcc_size_status" "$sdcc_size_ret" "$expected_ret")"
        sdcc_speed_match="$(match_expected "$sdcc_speed_status" "$sdcc_speed_ret" "$expected_ret")"
        z88dk_sdcc_match="$(match_expected "$z88dk_sdcc_status" "$z88dk_sdcc_ret" "$expected_ret")"
        z88dk_80cc_match="$(match_expected "$z88dk_80cc_status" "$z88dk_80cc_ret" "$expected_ret")"
        z88dk_sccz80_match="$(match_expected "$z88dk_sccz80_status" "$z88dk_sccz80_ret" "$expected_ret")"

        echo "$bench_name,$expected_ret,$xcc_of_status,$xcc_of_ret,$xcc_of_match,$xcc_of_bytes,$xcc_of_cycles,$xcc_o3_status,$xcc_o3_ret,$xcc_o3_match,$xcc_o3_bytes,$xcc_o3_cycles,$xcc_os_status,$xcc_os_ret,$xcc_os_match,$xcc_os_bytes,$xcc_os_cycles,$sdcc_size_status,$sdcc_size_ret,$sdcc_size_match,$sdcc_size_bytes,$sdcc_size_cycles,$sdcc_speed_status,$sdcc_speed_ret,$sdcc_speed_match,$sdcc_speed_bytes,$sdcc_speed_cycles,$z88dk_sdcc_status,$z88dk_sdcc_ret,$z88dk_sdcc_match,$z88dk_sdcc_bytes,$z88dk_sdcc_cycles,$z88dk_80cc_status,$z88dk_80cc_ret,$z88dk_80cc_match,$z88dk_80cc_bytes,$z88dk_80cc_cycles,$z88dk_sccz80_status,$z88dk_sccz80_ret,$z88dk_sccz80_match,$z88dk_sccz80_bytes,$z88dk_sccz80_cycles" \
            >> "$results_csv"
    done

    python3 "$PORTABLE_RENDERER" \
        --results "$results_csv" \
        --summary "$summary_md" \
        --versions "$versions_txt" \
        --cycle-limit "$PORTABLE_CYCLES" \
        --bench-count "${#benchmarks[@]}"
    {
        echo ""
        echo "## Outputs"
        echo ""
        echo "- [results.csv](results.csv)"
        echo "- [versions.txt](versions.txt)"
        echo "- [artifacts/](artifacts/)"
    } >> "$summary_md"
    echo ""
    echo "${GREEN}Wrote portable benchmark results to:${RESET} $suite_outdir"
    CURRENT_ARTIFACTS_DIR=""
}

collect_numeric_benchmarks() {
    find "$NUMERIC_ROOT" -mindepth 2 -maxdepth 2 -type f -name 'main.c' | sort
}

run_numeric_case() {
    local c_file="$1"
    local kind_name="$2"
    local kind_value="$3"
    local suite_outdir="$4"
    local bench_name
    local work
    local ihx
    local bin
    local output
    local rc
    local status
    local ret=0
    local cycles=0
    local bytes
    local mode_name

    bench_name="$(basename "$(dirname "$c_file")")"
    work="$suite_outdir/work/$bench_name/$kind_name"
    mode_name="xcc_${kind_name}"
    mkdir -p "$work"
    ihx="$work/image.ihx"
    bin="$work/image.bin"

    if ! run_xcc --platform=none -nostartfiles "$NUMERIC_OPT" --oformat=ihx \
        -e __xc_test_start -I"$NUMERIC_INCLUDE_DIR" "-DNUM_KIND=$kind_value" \
        "$NUMERIC_CRT0" "$c_file" -lfixed -o "$ihx" >"$work/build.log" 2>&1; then
        printf '%s,%s,build_error,0,0,0\n' "$bench_name" "$kind_name"
        return
    fi

    python3 "$IHX2BIN" "$ihx" "$bin"
    bytes="$(wc -c < "$bin" | tr -d '[:space:]')"
    publish_artifact_copy "$ihx" "$bench_name" "$mode_name" "program.ihx"
    publish_artifact_copy "$bin" "$bench_name" "$mode_name" "program.bin"

    set +e
    output="$("$RUNNER_BIN" --cycles "$NUMERIC_CYCLES" --ihx "$ihx" 2>&1)"
    rc=$?
    set -e
    printf '%s\n' "$output" > "$work/run.log"

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
        "$bench_name" "$kind_name" "$status" "$ret" "$bytes" "$cycles"
}

run_numeric_suite() {
    local suite_outdir="$OUTDIR/numeric"
    local results_csv="$suite_outdir/results.csv"
    local summary_md="$suite_outdir/summary.md"
    local artifacts_dir="$suite_outdir/artifacts"
    local -a benchmarks
    local -a filtered=()
    local -a kinds=(
        "fixed8_8:1"
        "fixed16_16:2"
        "fixed24_8:3"
        "float:4"
        "double:5"
    )
    local c_file
    local rel
    local kind
    local kind_name
    local kind_value

    rm -rf "$suite_outdir"
    mkdir -p "$suite_outdir/work"
    mkdir -p "$artifacts_dir"
    CURRENT_ARTIFACTS_DIR="$artifacts_dir"

    printf 'benchmark,kind,status,return,bytes,cycles\n' > "$results_csv"

    mapfile -t benchmarks < <(collect_numeric_benchmarks)
    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no numeric benchmarks found"

    if [[ -n "$NUMERIC_FILTER" ]]; then
        for c_file in "${benchmarks[@]}"; do
            rel="${c_file#$NUMERIC_ROOT/}"
            if [[ "$rel" =~ $NUMERIC_FILTER ]]; then
                filtered+=("$c_file")
            fi
        done
        benchmarks=("${filtered[@]}")
    fi

    [[ "${#benchmarks[@]}" -gt 0 ]] || die "no numeric benchmarks matched the selected filter"
    NUMERIC_BENCHMARKS_RUN="${#benchmarks[@]}"
    NUMERIC_KINDS_RUN="${#kinds[@]}"

    for c_file in "${benchmarks[@]}"; do
        for kind in "${kinds[@]}"; do
            IFS=: read -r kind_name kind_value <<< "$kind"
            run_numeric_case "$c_file" "$kind_name" "$kind_value" "$suite_outdir" >> "$results_csv"
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
            print "Optimization: `'"$NUMERIC_OPT"'`"
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
    ' "$results_csv" > "$summary_md"

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
        ' "$results_csv"
    } >> "$summary_md"

    {
        echo ""
        echo "## Outputs"
        echo ""
        echo "- [results.csv](results.csv)"
        echo "- [artifacts/](artifacts/)"
    } >> "$summary_md"

    cat "$summary_md"
    echo ""
    echo "${GREEN}Wrote numeric benchmark results to:${RESET} $suite_outdir"
    CURRENT_ARTIFACTS_DIR=""
}

run_z88dk24_suite() {
    local suite_outdir="$OUTDIR/z88dk24"
    XCC="$XCC" OUT="$suite_outdir" FILTER="$Z88DK24_FILTER" CYCLES="$Z88DK_CYCLES" \
        bash "$Z88DK24_ROOT/run.sh"
    Z88DK24_BENCHMARKS_RUN="$(awk 'END { print (NR > 0 ? NR - 1 : 0) }' \
        "$suite_outdir/results.csv")"
}

write_top_summary() {
    local summary_md="$OUTDIR/summary.md"

    {
        echo "# X Benchmarks"
        echo ""
        echo "| suite | benchmarks | matrix | summary | results |"
        echo "| --- | ---: | --- | --- | --- |"
        if [[ "$SUITE" == "all" || "$SUITE" == "bare" ]]; then
            printf '| `%s` | %s | `%s` | `%s` | `%s` |\n' \
                "bare-metal" \
                "$BARE_BENCHMARKS_RUN" \
                "xcc O2/Of/O3/Os + sdcc size/speed" \
                "$OUTDIR/bare/summary.md" \
                "$OUTDIR/bare/results.csv"
        fi
        if [[ "$SUITE" == "all" || "$SUITE" == "numeric" ]]; then
            printf '| `%s` | %s | `%s` | `%s` | `%s` |\n' \
                "numeric" \
                "$NUMERIC_BENCHMARKS_RUN" \
                "$NUMERIC_KINDS_RUN kinds @ $NUMERIC_OPT" \
                "$OUTDIR/numeric/summary.md" \
                "$OUTDIR/numeric/results.csv"
        fi
        if [[ "$SUITE" == "all" || "$SUITE" == "portable" ]]; then
            printf '| `%s` | %s | `%s` | `%s` | `%s` |\n' \
                "portable" \
                "$PORTABLE_BENCHMARKS_RUN" \
                "xcc Of/O3/Os + sdcc size/speed + z88dk sdcc/80cc/sccz80" \
                "$OUTDIR/portable/summary.md" \
                "$OUTDIR/portable/results.csv"
        fi
        if [[ "$SUITE" == "all" || "$SUITE" == "z88dk" ||
              "$SUITE" == "z88dk24" ]]; then
            printf '| `%s` | %s | `%s` | `%s` | `%s` |\n' \
                "z88dk-full-program" \
                "$Z88DK24_BENCHMARKS_RUN" \
                "xcc M Os/Of + z88dk sccz80/sdcc/80cc frame/stack" \
                "$OUTDIR/z88dk24/summary.md" \
                "$OUTDIR/z88dk24/results.csv"
        fi
    } > "$summary_md"

    cat "$summary_md"
    echo ""
    echo "${GREEN}Wrote benchmark results to:${RESET} $OUTDIR"
}

main() {
    parse_args "$@"

    case "$SUITE" in
    all|bare|numeric|portable|z88dk|z88dk24) ;;
    *) die "unknown suite '$SUITE' (expected: all, bare, numeric, portable, z88dk)" ;;
    esac

    [[ -x "$XCC" ]] || die "xcc not found at '$XCC'"
    [[ -x "$XLD" ]] || die "xld not found at '$XLD'"
    [[ -f "$CRT0_S" ]] || die "missing crt0: $CRT0_S"
    [[ -f "$NUMERIC_CRT0" ]] || die "missing numeric crt0: $NUMERIC_CRT0"

    if [[ -z "$BARE_FILTER" ]]; then
        BARE_FILTER="$FILTER"
    fi
    if [[ -z "$NUMERIC_FILTER" ]]; then
        NUMERIC_FILTER="$FILTER"
    fi
    if [[ -z "$PORTABLE_FILTER" ]]; then
        PORTABLE_FILTER="$FILTER"
    fi
    if [[ -z "$Z88DK24_FILTER" ]]; then
        Z88DK24_FILTER="$FILTER"
    fi

    need_cmd python3
    need_cmd awk
    need_cmd sort
    need_cmd wc
    ensure_runner

    rm -rf "$OUTDIR"
    mkdir -p "$OUTDIR"

    if [[ "$SUITE" == "all" || "$SUITE" == "bare" ]]; then
        echo "Running bare-metal benchmarks..."
        echo ""
        run_bare_suite
        echo ""
    fi

    if [[ "$SUITE" == "all" || "$SUITE" == "numeric" ]]; then
        echo "Running numeric benchmarks..."
        echo ""
        run_numeric_suite
        echo ""
    fi

    if [[ "$SUITE" == "all" || "$SUITE" == "portable" ]]; then
        echo "Running portable benchmarks..."
        echo ""
        run_portable_suite
        echo ""
    fi

    if [[ "$SUITE" == "all" || "$SUITE" == "z88dk" ||
          "$SUITE" == "z88dk24" ]]; then
        echo "Running complete z88dk full-program benchmarks..."
        echo ""
        run_z88dk24_suite
        echo ""
    fi

    write_top_summary
}

main "$@"
