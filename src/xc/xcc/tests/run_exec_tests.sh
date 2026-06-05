#!/usr/bin/env bash
# Executable xcc regression runner.
# Builds each self-checking C test twice:
#   1. SDCC/ASxxxx flow: xcc -> sdasz80 -> sdldz80 -> .ihx
#   2. GNU ELF flow:     xcc -> gnu as -> gnu ld -> raw binary
# Then executes the linked image in the bundled header-only emulator.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
XCC="${1:-$ROOT_DIR/build/bin/xcc}"
TEST_ROOT="$ROOT_DIR/tests/data/exec"
INCLUDE_DIR="$TEST_ROOT/include"
RUNNER_SRC="$ROOT_DIR/tests/tools/z80emu/z80_exec.cpp"
RUNNER_BIN="$ROOT_DIR/build/bin/z80_exec"
CRT0_SDAS="$ROOT_DIR/tests/tools/z80emu/crt0_sdasz80.s"
CRT0_GNU="$ROOT_DIR/tests/tools/z80emu/crt0_gnuas.s"
GNU_LD_SCRIPT="$ROOT_DIR/tests/tools/z80emu/z80_exec.ld"
EXEC_BUILD="$ROOT_DIR/build/exec"
GNU_PREFIX="${Z80_GNU_PREFIX:-/usr/local/z80-elf/bin/z80-unknown-elf-}"
GNU_AS="${GNU_PREFIX}as"
GNU_LD="${GNU_PREFIX}ld"
GNU_OBJCOPY="${GNU_PREFIX}objcopy"
RUNTIME_DIR="$ROOT_DIR/lib/runtime"

PASS=0
FAIL=0

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

die() {
    echo "${RED}ERROR:${RESET} $*" >&2
    exit 1
}

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "missing required tool: $1"
}

build_runner() {
    mkdir -p "$ROOT_DIR/build/bin"
    if [[ ! -x "$RUNNER_BIN" || "$RUNNER_SRC" -nt "$RUNNER_BIN" ]]; then
        g++ -std=c++17 -I "$ROOT_DIR/tests/tools/z80emu" \
            -o "$RUNNER_BIN" "$RUNNER_SRC"
    fi
}

translate_sdas_to_gnu() {
    local src="$1"
    local dst="$2"

    perl -pe '
        s/^\s*\.module[^\n]*\n//;
        s/^\s*\.optsdcc[^\n]*\n//;
        s/^\s*\.area\s+_CODE/\t.text/;
        s/^\s*\.area\s+_DATA/\t.data/;
        s/^\s*\.area\s+_CONST/\t.section\t.rodata/;
        s/^\s*\.area\s+_TLS/\t.section\t.tdata,\"aw\"/;
        s/\b\.globl\b/.global/g;
        s/\b\.db\b/.byte/g;
        s/\b\.dw\b/.short/g;
        s/\b\.dl\b/.long/g;
        s/\b\.ds\b/.space/g;
        s/::/:/g;
        s/#//g;
        s/([+-]?\d+)\s*\((i[xy])\)/sprintf("(%s%s%d)", $2, ($1 < 0 ? "" : "+"), $1)/eg;
    ' "$src" > "$dst"
}

extra_opts_for_test() {
    local c_file="$1"
    local base="${c_file%.c}"

    if [[ -f "${base}.opts" ]]; then
        cat "${base}.opts"
        return
    fi
}

declare -a MODULES=()
declare -A SEEN=()

add_module() {
    local path="$1"
    if [[ -z "${SEEN[$path]+x}" ]]; then
        MODULES+=("$path")
        SEEN["$path"]=1
    fi
}

add_runtime_module() {
    add_module "$RUNTIME_DIR/$1"
}

resolve_modules() {
    local asm_file="$1"
    MODULES=()
    SEEN=()

    local helpers
    helpers="$(grep -o '__[A-Za-z0-9_]\+' "$asm_file" | sort -u || true)"
    while IFS= read -r helper; do
        [[ -n "$helper" ]] || continue
        case "$helper" in
        __mul16)
            add_runtime_module "mul16_bridge.s"
            add_runtime_module "mulint.s"
            ;;
        __div16)
            add_runtime_module "div16_bridge.s"
            add_runtime_module "divunsigned.s"
            ;;
        __mod16)
            add_runtime_module "mod16_bridge.s"
            add_runtime_module "divunsigned.s"
            add_runtime_module "modunsigned.s"
            ;;
        __divsint)
            add_runtime_module "divsigned.s"
            add_runtime_module "divunsigned.s"
            ;;
        __divuint)
            add_runtime_module "divunsigned.s"
            ;;
        __smod16)
            add_runtime_module "smod16.s"
            add_runtime_module "divunsigned.s"
            ;;
        __mul32)
            add_runtime_module "mul32_bridge.s"
            add_runtime_module "mullong.s"
            ;;
        __div32)
            add_runtime_module "div32_bridge.s"
            add_runtime_module "divulong.s"
            ;;
        __mod32)
            add_runtime_module "mod32_bridge.s"
            add_runtime_module "modulong.s"
            ;;
        __sdiv32)
            add_runtime_module "sdiv32_bridge.s"
            add_runtime_module "divslong.s"
            add_runtime_module "divulong.s"
            ;;
        __smod32)
            add_runtime_module "smod32_bridge.s"
            add_runtime_module "modslong.s"
            add_runtime_module "divulong.s"
            ;;
        __fsadd)
            add_runtime_module "fsadd_bridge.s"
            add_runtime_module "fsadd_core.s"
            add_runtime_module "fp.s"
            add_runtime_module "fppack.s"
            add_runtime_module "fpret.s"
            ;;
        __fssub)
            add_runtime_module "fssub_bridge.s"
            add_runtime_module "fssub_core.s"
            add_runtime_module "fsadd_core.s"
            add_runtime_module "fp.s"
            add_runtime_module "fppack.s"
            add_runtime_module "fpret.s"
            ;;
        __fsmul)
            add_runtime_module "fsmul_bridge.s"
            add_runtime_module "fsmul_core.s"
            add_runtime_module "fp.s"
            add_runtime_module "fppack.s"
            add_runtime_module "fpret.s"
            add_runtime_module "fpunpack.s"
            add_runtime_module "fpmant.s"
            ;;
        __fsdiv)
            add_runtime_module "fsdiv_bridge.s"
            add_runtime_module "fsdiv_core.s"
            add_runtime_module "fp.s"
            add_runtime_module "fppack.s"
            add_runtime_module "fpret.s"
            add_runtime_module "fpunpack.s"
            add_runtime_module "fpmant.s"
            ;;
        __call_hl)
            add_runtime_module "call_hl_bridge.s"
            ;;
        __sdcc_call_hl)
            add_runtime_module "call_hl_runtime.s"
            ;;
        __sdcc_call_bc)
            add_runtime_module "call_bc_runtime.s"
            ;;
        esac
    done <<< "$helpers"
}

compile_xcc() {
    local c_file="$1"
    local asm_file="$2"
    local dialect="$3"
    local opt_level="$4"
    local extra_opts
    extra_opts="$(extra_opts_for_test "$c_file")"

    env ASAN_OPTIONS=detect_leaks=0 "$XCC" -S "-$opt_level" $extra_opts \
        "-I$INCLUDE_DIR" \
        "-masm=$dialect" "$c_file" -o "$asm_file"
}

run_sdasz80() {
    local c_file="$1"
    local name="$2"
    local workdir="$3"
    local opt_level="$4"
    local asm_file="$workdir/$name.s"
    local test_rel="$workdir/$name.rel"
    local crt_rel="$workdir/crt0.rel"
    local outbase="$workdir/$name"

    compile_xcc "$c_file" "$asm_file" "sdasz80" "$opt_level"
    resolve_modules "$asm_file"

    sdasz80 -o "$crt_rel" "$CRT0_SDAS" >/dev/null
    sdasz80 -o "$test_rel" "$asm_file" >/dev/null

    local rels=("$crt_rel" "$test_rel")
    local src
    for src in "${MODULES[@]}"; do
        local rel="$workdir/$(basename "${src%.s}").rel"
        sdasz80 -o "$rel" "$src" >/dev/null
        rels+=("$rel")
    done

    (
        cd "$workdir"
        sdldz80 -n -i "$name" "${rels[@]}" >/dev/null
    )

    "$RUNNER_BIN" --ihx "$outbase.ihx"
}

run_gnuas() {
    local c_file="$1"
    local name="$2"
    local workdir="$3"
    local opt_level="$4"
    local asm_file="$workdir/$name.s"
    local test_o="$workdir/$name.o"
    local crt_o="$workdir/crt0.o"
    local elf_file="$workdir/$name.elf"
    local bin_file="$workdir/$name.bin"
    local gnu_runtime_dir="$workdir/gnu_runtime"

    compile_xcc "$c_file" "$asm_file" "gnuas" "$opt_level"
    resolve_modules "$asm_file"

    mkdir -p "$gnu_runtime_dir"
    "$GNU_AS" -march=z80 -o "$crt_o" "$CRT0_GNU"
    "$GNU_AS" -march=z80 -o "$test_o" "$asm_file"

    local objs=("$crt_o" "$test_o")
    local src
    for src in "${MODULES[@]}"; do
        local base="$(basename "$src")"
        local gnu_src="$gnu_runtime_dir/$base"
        local obj="$gnu_runtime_dir/${base%.s}.o"
        translate_sdas_to_gnu "$src" "$gnu_src"
        "$GNU_AS" -march=z80 -o "$obj" "$gnu_src"
        objs+=("$obj")
    done

    "$GNU_LD" -m elf32z80 -T "$GNU_LD_SCRIPT" -o "$elf_file" "${objs[@]}"
    "$GNU_OBJCOPY" -O binary "$elf_file" "$bin_file"
    "$RUNNER_BIN" --bin "$bin_file"
}

run_one() {
    local c_file="$1"
    local mode="$2"
    local opt_level="$3"
    local rel="${c_file#$TEST_ROOT/}"
    local suite="${rel%%/*}"
    local name="$(basename "${c_file%.c}")"
    local workdir="$EXEC_BUILD/$opt_level/$mode/$suite/$name"
    mkdir -p "$workdir"

    local output
    if ! output="$(
        if [[ "$mode" == "sdasz80" ]]; then
            run_sdasz80 "$c_file" "$name" "$workdir" "$opt_level"
        else
            run_gnuas "$c_file" "$name" "$workdir" "$opt_level"
        fi
    )"; then
        echo "${RED}FAIL${RESET} $name [$mode $opt_level]"
        FAIL=$((FAIL + 1))
        return
    fi

    local ret
    ret="$(printf '%s\n' "$output" | sed -n 's/.*return=\([0-9][0-9]*\).*/\1/p')"
    if [[ -z "$ret" ]]; then
        echo "${RED}FAIL${RESET} $name [$mode $opt_level]"
        printf '       %s\n' "$output"
        FAIL=$((FAIL + 1))
        return
    fi

    if [[ "$ret" == "0" ]]; then
        echo "${GREEN}PASS${RESET} $name [$mode $opt_level]"
        PASS=$((PASS + 1))
    else
        echo "${RED}FAIL${RESET} $name [$mode $opt_level]  [return=$ret]"
        printf '       %s\n' "$output"
        FAIL=$((FAIL + 1))
    fi
}

need_cmd "$XCC"
need_cmd sdasz80
need_cmd sdldz80
need_cmd "$GNU_AS"
need_cmd "$GNU_LD"
need_cmd "$GNU_OBJCOPY"
build_runner
mkdir -p "$EXEC_BUILD"

while IFS= read -r c_file; do
    for opt_level in O0 O1 O2; do
        run_one "$c_file" "sdasz80" "$opt_level"
        run_one "$c_file" "gnuas" "$opt_level"
    done
done < <(find "$TEST_ROOT" -type f -name '*.c' | sort)

echo
echo "Results: ${GREEN}${PASS} passed${RESET}, ${RED}${FAIL} failed${RESET}"
echo

[[ "$FAIL" -eq 0 ]]
