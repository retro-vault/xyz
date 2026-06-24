#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
X_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
REPO_ROOT="$(cd "$X_ROOT/.." && pwd)"
XCC="${XCC:-$REPO_ROOT/bin/x/bin/xcc}"
RUNNER="${RUNNER:-$REPO_ROOT/build/bin/z80_exec}"
BUILD="$REPO_ROOT/build/corpus/c23-projects"
ORIG="$X_ROOT/tests/tests/c23/corpus/upstream/c23-corpus"
SRC="$SCRIPT_DIR/src"
CYCLES="${CYCLES:-200000000}"
COMPILE_TIMEOUT="${COMPILE_TIMEOUT:-120s}"

mkdir -p "$BUILD/bin" "$BUILD/fs"

if [[ ! -x "$RUNNER" || "$X_ROOT/tests/tests/c23/xcc/tools/z80emu/z80_exec.cpp" -nt "$RUNNER" ]]; then
    mkdir -p "$(dirname "$RUNNER")"
    g++ -std=c++17 -I "$X_ROOT/tests/tests/c23/xcc/tools/z80emu" \
        -o "$RUNNER" "$X_ROOT/tests/tests/c23/xcc/tools/z80emu/z80_exec.cpp"
fi

pass=0
fail=0

run_case_opt() {
    local opt="$1"
    shift
    local name="$1"
    shift
    local opt_flags=()
    read -r -a opt_flags <<< "$opt"
    local fs="$BUILD/fs/$name"
    local bin="$BUILD/bin/$name.bin"
    local map="$BUILD/bin/$name.map"
    local out="$BUILD/bin/$name.out"
    rm -rf "$fs"
    mkdir -p "$fs"

    if [[ "$name" == "rxi-ini" ]]; then
        printf '[main]\ntitle=X Tools\nanswer=42\nquoted="z80\\tc23"\n' > "$fs/settings.ini"
    elif [[ "$name" == "inih" ]]; then
        printf '[main]\ntitle=X Tools\nanswer=42\n\n[paths]\nroot=/z80\n' > "$fs/sample.ini"
    elif [[ "$name" == "emu-fs" ]]; then
        printf 'alpha' > "$fs/seed.txt"
    fi

    echo "==> $name: compile ($opt)"
    set +e
    timeout "$COMPILE_TIMEOUT" env ASAN_OPTIONS=detect_leaks=0 \
        "$XCC" --platform=emu --oformat=binary "${opt_flags[@]}" -Wl,-Map,"$map" "$@" -o "$bin" \
        >"$BUILD/bin/$name.compile.log" 2>&1
    local compile_rc=$?
    set -e
    if [[ "$compile_rc" -ne 0 ]]; then
        if [[ "$compile_rc" -eq 124 ]]; then
            echo "FAIL $name [compile-timeout: $COMPILE_TIMEOUT]"
        else
            echo "FAIL $name [compile: exit $compile_rc]"
        fi
        sed -n '1,40p' "$BUILD/bin/$name.compile.log"
        fail=$((fail + 1))
        return
    fi
    if [[ -f "$map" ]]; then
        local size_hex
        size_hex="$(sed -n 's/.*Total code size: 0x\([0-9A-Fa-f][0-9A-Fa-f]*\).*/\1/p' "$map" | tail -n 1)"
        if [[ -n "$size_hex" && $((16#$size_hex)) -gt 65536 ]]; then
            echo "FAIL $name [image-too-large: 0x$size_hex > 0x10000]"
            fail=$((fail + 1))
            return
        fi
    fi

    echo "==> $name: run"
    local summary
    if ! summary="$("$RUNNER" --bin --cycles "$CYCLES" --fs-root "$fs" --stdout "$out" "$bin")"; then
        echo "FAIL $name [emulator]"
        echo "$summary"
        fail=$((fail + 1))
        return
    fi
    echo "$summary"
    local ret
    ret="$(printf '%s\n' "$summary" | sed -n 's/.*return=\([0-9][0-9]*\).*/\1/p')"
    if [[ "$ret" == "0" ]]; then
        echo "PASS $name"
        pass=$((pass + 1))
    else
        echo "FAIL $name [return=$ret]"
        if [[ -s "$out" ]]; then
            sed -n '1,20p' "$out"
        fi
        fail=$((fail + 1))
    fi
}

run_case() {
    run_case_opt "-O0" "$@"
}

run_case emu-fs \
    "$SRC/emu_fs_harness.c"

run_case argparse \
    -I"$ORIG/argparse" \
    "$SRC/argparse_harness.c" \
    "$ORIG/argparse/argparse.c"

run_case base64 \
    -I"$ORIG/base64" \
    "$SRC/base64_harness.c" \
    "$ORIG/base64/base64.c"

run_case branchless-utf8 \
    -I"$ORIG/branchless-utf8" \
    "$SRC/branchless_utf8_harness.c"

run_case cwalk \
    -I"$ORIG/cwalk/include" \
    "$SRC/cwalk_harness.c" \
    "$ORIG/cwalk/src/cwalk.c"

run_case inih \
    -I"$ORIG/inih" \
    "$SRC/inih_harness.c" \
    "$ORIG/inih/ini.c"

run_case jsmn \
    -I"$ORIG/jsmn" \
    "$SRC/jsmn_harness.c"

run_case map \
    -I"$ORIG/map/src" \
    "$SRC/map_harness.c" \
    "$ORIG/map/src/map.c"

run_case optparse \
    -I"$ORIG/optparse" \
    "$SRC/optparse_harness.c"

run_case rxi-ini \
    -I"$ORIG/ini/src" \
    "$SRC/rxi_ini_harness.c" \
    "$ORIG/ini/src/ini.c"

run_case tiny-json \
    -I"$ORIG/tiny-json" \
    "$SRC/tiny_json_harness.c" \
    "$ORIG/tiny-json/tiny-json.c"

run_case tiny-regex-c \
    -I"$ORIG/tiny-regex-c" \
    "$SRC/tiny_regex_harness.c" \
    "$ORIG/tiny-regex-c/re.c"

run_case utf8.h \
    -I"$ORIG/utf8.h" \
    "$SRC/utf8_harness.c"

run_case vec \
    -I"$ORIG/vec/src" \
    "$SRC/vec_harness.c" \
    "$ORIG/vec/src/vec.c"

run_case whereami \
    -I"$ORIG/whereami/src" \
    "$SRC/whereami_harness.c" \
    "$SRC/whereami_emu.c"

run_case uthash \
    -I"$ORIG/uthash/src" \
    "$SRC/uthash_harness.c"

run_case klib \
    -I"$ORIG/klib" \
    "$SRC/klib_harness.c"

run_case nanoprintf \
    -I"$ORIG/nanoprintf" \
    "$SRC/nanoprintf_harness.c"

run_case log.c \
    -I"$ORIG/log.c/src" \
    "$SRC/logc_harness.c" \
    "$ORIG/log.c/src/log.c"

run_case picohttpparser \
    -I"$ORIG/picohttpparser" \
    "$SRC/picohttpparser_harness.c" \
    "$ORIG/picohttpparser/picohttpparser.c"

run_case c-algorithms \
    -I"$ORIG/c-algorithms/src" \
    "$SRC/c_algorithms_harness.c" \
    "$ORIG/c-algorithms/src/queue.c"

run_case portable-snippets \
    -I"$ORIG/portable-snippets/exact-int" \
    "$SRC/portable_snippets_harness.c"

echo
echo "Results: $pass passed, $fail failed"
[[ "$fail" -eq 0 ]]
