#!/usr/bin/env bash
# Executable xcc regression runner.
# Builds each self-checking C test twice:
#   1. SDCC/ASxxxx flow: xcc -> sdasz80 -> sdldz80 -> .ihx
#   2. GNU ELF flow:     xcc -> gnu as -> gnu ld -> raw binary
# Then executes the linked image in the bundled header-only emulator.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
REPO_ROOT="$(cd "$ROOT_DIR/../../.." && pwd)"
XCC="${1:-$ROOT_DIR/build/bin/xcc}"
TEST_ROOT="$ROOT_DIR/tests/data/exec"
INCLUDE_DIR="$TEST_ROOT/include"
LIBC_INCLUDE_DIR="$REPO_ROOT/lib/libc/include"
COMMON_INCLUDE_DIR="$REPO_ROOT/include"
RUNNER_SRC="$ROOT_DIR/tests/tools/z80emu/z80_exec.cpp"
RUNNER_BIN="$ROOT_DIR/build/bin/z80_exec"
CRT0_SDAS="$ROOT_DIR/tests/tools/z80emu/crt0_sdasz80.s"
CRT0_GNU="$ROOT_DIR/tests/tools/z80emu/crt0_gnuas.s"
GNU_LD_SCRIPT="$ROOT_DIR/tests/tools/z80emu/z80_exec.ld"
TEST_SYS_EXIT="$ROOT_DIR/tests/tools/z80emu/sys_exit.s"
EXEC_BUILD="$ROOT_DIR/build/exec"
GNU_PREFIX="${Z80_GNU_PREFIX:-/usr/local/z80-elf/bin/z80-unknown-elf-}"
GNU_AS="${GNU_PREFIX}as"
GNU_LD="${GNU_PREFIX}ld"
GNU_OBJCOPY="${GNU_PREFIX}objcopy"
XAS="${XAS:-}"
RUNTIME_DIR="$ROOT_DIR/lib/runtime"
LIBC_SRC_DIR="$REPO_ROOT/lib/libc/src"
SYS_NONE_DIR="$REPO_ROOT/lib/sys/none"

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

if [[ "$XCC" != /* ]]; then
    XCC="$(cd "$(dirname "$XCC")" && pwd)/$(basename "$XCC")"
fi

if [[ -z "$XAS" ]]; then
    if [[ -x "$(dirname "$XCC")/xas" ]]; then
        XAS="$(dirname "$XCC")/xas"
    else
        XAS="$REPO_ROOT/bin/x/bin/xas"
    fi
fi

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
    "$XAS" --mode=sdcc --format=gnu -o "$dst" "$src"
}

extra_opts_for_test() {
    local c_file="$1"
    local base="${c_file%.c}"

    if [[ -f "${base}.opts" ]]; then
        cat "${base}.opts"
        return
    fi
}

opt_levels_for_test() {
    local c_file="$1"
    local base="${c_file%.c}"

    if [[ -f "${base}.levels" ]]; then
        cat "${base}.levels"
        return
    fi

    printf '%s\n' O0 O1 O2
}

declare -a MODULES=()
declare -A SEEN=()

reset_modules() {
    MODULES=()
    SEEN=()
}

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

add_libc_module() {
    add_module "$LIBC_SRC_DIR/$1"
}

add_sys_none_module() {
    add_module "$SYS_NONE_DIR/$1"
}

add_exec_tool_module() {
    add_module "$ROOT_DIR/tests/tools/z80emu/$1"
}

resolve_libc_modules() {
    local asm_file
    for asm_file in "$@"; do
        local public_refs
        public_refs="$(grep -o '_[A-Za-z0-9_]\+' "$asm_file" | sort -u || true)"
        while IFS= read -r ref; do
            [[ -n "$ref" ]] || continue
            case "$ref" in
        _aligned_alloc)
            add_libc_module "stdlib/aligned_alloc.s"
            add_libc_module "stdlib/heap_core.s"
            add_libc_module "string/memcpy.s"
            add_sys_none_module "sys_sbrk.s"
            add_runtime_module "int16/mulint.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        _malloc|_calloc|_realloc|_free)
            add_libc_module "stdlib/heap_core.s"
            add_libc_module "string/memcpy.s"
            add_sys_none_module "sys_sbrk.s"
            add_runtime_module "int16/mulint.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        _qsort)
            add_libc_module "stdlib/qsort.s"
            add_runtime_module "int16/mulint.s"
            add_runtime_module "jumps/call_bc_runtime.s"
            ;;
        _bsearch)
            add_libc_module "stdlib/bsearch.s"
            add_runtime_module "int16/mulint.s"
            add_runtime_module "jumps/call_bc_runtime.s"
            ;;
        _ldiv)
            add_libc_module "stdlib/ldiv.s"
            add_runtime_module "int32/divslong.s"
            add_runtime_module "int32/modslong.s"
            ;;
        _lldiv)
            add_libc_module "stdlib/lldiv.s"
            add_runtime_module "int64/divsll.s"
            add_runtime_module "int64/modsll.s"
            ;;
        _abort|_atexit|_exit|__Exit|_at_quick_exit|_quick_exit)
            add_libc_module "stdlib/exit_core.s"
            add_exec_tool_module "sys_exit.s"
            add_runtime_module "jumps/call_bc_runtime.s"
            ;;
        _atof)
            add_libc_module "stdlib/atof.s"
            ;;
        _strtof)
            add_libc_module "stdlib/strtof.s"
            ;;
        _strtod)
            add_libc_module "stdlib/strtod.s"
            ;;
        _strtold)
            add_libc_module "stdlib/strtold.s"
            ;;
        _mblen)
            add_libc_module "stdlib/mblen.s"
            ;;
        _mbtowc)
            add_libc_module "stdlib/mbtowc.s"
            ;;
        _wctomb)
            add_libc_module "stdlib/wctomb.s"
            ;;
        _mbrlen)
            add_libc_module "wchar/mbrlen.s"
            ;;
        _mbrtowc)
            add_libc_module "wchar/mbrtowc.s"
            ;;
        _wcrtomb)
            add_libc_module "wchar/wcrtomb.s"
            ;;
        _mbsrtowcs)
            add_libc_module "wchar/mbsrtowcs.s"
            ;;
        _wcsrtombs)
            add_libc_module "wchar/wcsrtombs.s"
            ;;
        _wcstof)
            add_libc_module "wchar/wcstof.s"
            ;;
        _wcstod)
            add_libc_module "wchar/wcstod.s"
            ;;
        _wcstold)
            add_libc_module "wchar/wcstold.s"
            ;;
        _wcstol)
            add_libc_module "wchar/wcstol.s"
            ;;
        _wcstoul)
            add_libc_module "wchar/wcstoul.s"
            ;;
        _wcstoll)
            add_libc_module "wchar/wcstoll.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        _wcstoull)
            add_libc_module "wchar/wcstoull.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        _wcstoimax)
            add_libc_module "inttypes/wcstoimax.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        _wcstoumax)
            add_libc_module "inttypes/wcstoumax.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        _strtoll)
            add_libc_module "stdlib/strtoll.s"
            add_libc_module "stdlib/strtox_core.s"
            add_runtime_module "int16/mulint.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        _strtoull)
            add_libc_module "stdlib/strtoull.s"
            add_libc_module "stdlib/strtox_core.s"
            add_runtime_module "int16/mulint.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        _strtoimax)
            add_libc_module "inttypes/strtoimax.s"
            ;;
        _strtoumax)
            add_libc_module "inttypes/strtoumax.s"
            ;;
        _imaxdiv)
            add_libc_module "inttypes/imaxdiv.s"
            ;;
        _wcscoll)
            add_libc_module "wchar/wcscoll.s"
            ;;
        _wcscmp)
            add_libc_module "wchar/wcscmp.s"
            ;;
        _wcsxfrm)
            add_libc_module "wchar/wcsxfrm.s"
            ;;
        _sqrtf)
            add_libc_module "math/sqrtf.s"
            ;;
        _fabsf)
            add_libc_module "math/fabsf.s"
            ;;
        _copysignf)
            add_libc_module "math/copysignf.s"
            ;;
        _sinf)
            add_libc_module "math/trigf.s"
            ;;
        _cosf)
            add_libc_module "math/trigf.s"
            ;;
        _atan2f)
            add_libc_module "math/atan2f.s"
            ;;
        _roundf)
            add_libc_module "math/roundf.s"
            add_libc_module "math/round_common.s"
            ;;
        _truncf)
            add_libc_module "math/truncf.s"
            ;;
        _frexpf)
            add_libc_module "math/frexpf.s"
            ;;
        _ldexpf)
            add_libc_module "math/ldexpf.s"
            ;;
        _expf)
            add_libc_module "math/expf.s"
            ;;
        _logf)
            add_libc_module "math/logf.s"
            ;;
        __libc_expf_core|__libc_logf_core)
            add_libc_module "math/transf_core.s"
            ;;
        ___libc_fpclassifyf|__libc_fpclassifyf)
            add_libc_module "math/libc_fpclassifyf.s"
            ;;
        ___libc_signbitf|__libc_signbitf)
            add_libc_module "math/libc_signbitf.s"
            ;;
        ___libc_isinff|__libc_isinff)
            add_libc_module "math/libc_isinff.s"
            ;;
        ___fsadd)
            add_runtime_module "float/fsadd.s"
            ;;
        ___fssub)
            add_runtime_module "float/fssub.s"
            ;;
        ___fsmul)
            add_runtime_module "float/fsmul.s"
            ;;
        ___fsdiv)
            add_runtime_module "float/fsdiv.s"
            ;;
        ___fscmp)
            add_runtime_module "float/fscmp.s"
            ;;
        ___sint2fs)
            add_runtime_module "int16/sint2fs.s"
            ;;
        ___uint2fs)
            add_runtime_module "int16/uint2fs.s"
            ;;
        _sinhf)
            add_libc_module "math/sinhf.s"
            ;;
        _coshf)
            add_libc_module "math/coshf.s"
            ;;
        _tanhf)
            add_libc_module "math/tanhf.s"
            ;;
        _asinhf)
            add_libc_module "math/asinhf.s"
            ;;
        _acoshf)
            add_libc_module "math/acoshf.s"
            ;;
        _atanhf)
            add_libc_module "math/atanhf.s"
            ;;
        _sinh|_sinhl)
            add_libc_module "math/sinh.s"
            add_libc_module "math/sinhf.s"
            ;;
        _cosh|_coshl)
            add_libc_module "math/cosh.s"
            add_libc_module "math/coshf.s"
            ;;
        _tanh|_tanhl)
            add_libc_module "math/tanh.s"
            add_libc_module "math/tanhf.s"
            ;;
        _asinh)
            add_libc_module "math/asinh.s"
            add_libc_module "math/asinhf.s"
            ;;
        _asinhl)
            add_libc_module "math/asinhl.s"
            add_libc_module "math/asinh.s"
            add_libc_module "math/asinhf.s"
            ;;
        _acosh)
            add_libc_module "math/acosh.s"
            add_libc_module "math/acoshf.s"
            ;;
        _acoshl)
            add_libc_module "math/acoshl.s"
            add_libc_module "math/acosh.s"
            add_libc_module "math/acoshf.s"
            ;;
        _atanh)
            add_libc_module "math/atanh.s"
            add_libc_module "math/atanhf.s"
            ;;
        _atanhl)
            add_libc_module "math/atanhl.s"
            add_libc_module "math/atanh.s"
            add_libc_module "math/atanhf.s"
            ;;
        _erff)
            add_libc_module "math/erff.s"
            add_libc_module "math/erff_core.s"
            ;;
        _erfcf)
            add_libc_module "math/erfcf.s"
            add_libc_module "math/erff.s"
            add_libc_module "math/erff_core.s"
            ;;
        _lgammaf)
            add_libc_module "math/lgammaf.s"
            add_libc_module "math/gammaf_core.s"
            ;;
        _tgammaf)
            add_libc_module "math/tgammaf.s"
            add_libc_module "math/gammaf_core.s"
            ;;
        _erf|_erfl)
            add_libc_module "math/erf.s"
            add_libc_module "math/erff.s"
            add_libc_module "math/erff_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _erfc)
            add_libc_module "math/erfc.s"
            add_libc_module "math/erf.s"
            add_libc_module "math/erff.s"
            add_libc_module "math/erff_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _erfcl)
            add_libc_module "math/erfcl.s"
            add_libc_module "math/erfc.s"
            add_libc_module "math/erf.s"
            add_libc_module "math/erff.s"
            add_libc_module "math/erff_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _lgamma)
            add_libc_module "math/lgamma.s"
            add_libc_module "math/lgammaf.s"
            add_libc_module "math/gammaf_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _lgammal)
            add_libc_module "math/lgammal.s"
            add_libc_module "math/lgamma.s"
            add_libc_module "math/lgammaf.s"
            add_libc_module "math/gammaf_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _tgamma)
            add_libc_module "math/tgamma.s"
            add_libc_module "math/tgammaf.s"
            add_libc_module "math/gammaf_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _tgammal)
            add_libc_module "math/tgammal.s"
            add_libc_module "math/tgamma.s"
            add_libc_module "math/tgammaf.s"
            add_libc_module "math/gammaf_core.s"
            add_libc_module "math/db1argf_common.s"
            ;;
        _cprojf)
            add_libc_module "complex/cprojf.s"
            ;;
        _conjf)
            add_libc_module "complex/conjf.s"
            ;;
        _cabsf)
            add_libc_module "complex/cabsf.s"
            ;;
        _cargf)
            add_libc_module "complex/cargf.s"
            ;;
        _cexpf)
            add_libc_module "complex/cexpf.s"
            ;;
        _clogf)
            add_libc_module "complex/clogf.s"
            ;;
        _cpowf)
            add_libc_module "complex/cpowf.s"
            ;;
        _csqrtf)
            add_libc_module "complex/csqrtf.s"
            ;;
        _csinf)
            add_libc_module "complex/csinf.s"
            ;;
        _ccosf)
            add_libc_module "complex/ccosf.s"
            ;;
        _ctanf)
            add_libc_module "complex/ctanf.s"
            ;;
        _csinhf)
            add_libc_module "complex/csinhf.s"
            ;;
        _ccoshf)
            add_libc_module "complex/ccoshf.s"
            ;;
        _ctanhf)
            add_libc_module "complex/ctanhf.s"
            ;;
        _casinf)
            add_libc_module "complex/casinf.s"
            add_libc_module "complex/casinhf.s"
            ;;
        _cacosf)
            add_libc_module "complex/cacosf.s"
            add_libc_module "complex/casinf.s"
            add_libc_module "complex/casinhf.s"
            ;;
        _catanf)
            add_libc_module "complex/catanf.s"
            add_libc_module "complex/catanhf.s"
            ;;
        _casinhf)
            add_libc_module "complex/casinhf.s"
            ;;
        _cacoshf)
            add_libc_module "complex/cacoshf.s"
            ;;
        _catanhf)
            add_libc_module "complex/catanhf.s"
            ;;
        __creal)
            add_libc_module "complex/creal.s"
            ;;
        __cimag)
            add_libc_module "complex/cimag.s"
            ;;
        __complex_I)
            add_libc_module "complex/complex_i.s"
            ;;
        _call_once|_cnd_broadcast|_cnd_destroy|_cnd_init|_cnd_signal|_cnd_timedwait|_cnd_wait|_mtx_destroy|_mtx_init|_mtx_lock|_mtx_timedlock|_mtx_trylock|_mtx_unlock|_thrd_create|_thrd_current|_thrd_detach|_thrd_equal|_thrd_exit|_thrd_join|_thrd_sleep|_thrd_yield|_tss_create|_tss_delete|_tss_get|_tss_set)
            add_libc_module "threads/threads_common.s"
            add_libc_module "threads/call_once.s"
            add_libc_module "threads/cnd_broadcast.s"
            add_libc_module "threads/cnd_destroy.s"
            add_libc_module "threads/cnd_init.s"
            add_libc_module "threads/cnd_signal.s"
            add_libc_module "threads/cnd_timedwait.s"
            add_libc_module "threads/cnd_wait.s"
            add_libc_module "threads/mtx_destroy.s"
            add_libc_module "threads/mtx_init.s"
            add_libc_module "threads/mtx_lock.s"
            add_libc_module "threads/mtx_timedlock.s"
            add_libc_module "threads/mtx_trylock.s"
            add_libc_module "threads/mtx_unlock.s"
            add_libc_module "threads/thrd_create.s"
            add_libc_module "threads/thrd_current.s"
            add_libc_module "threads/thrd_detach.s"
            add_libc_module "threads/thrd_equal.s"
            add_libc_module "threads/thrd_exit.s"
            add_libc_module "threads/thrd_join.s"
            add_libc_module "threads/thrd_sleep.s"
            add_libc_module "threads/thrd_yield.s"
            add_libc_module "threads/tss_create.s"
            add_libc_module "threads/tss_delete.s"
            add_libc_module "threads/tss_get.s"
            add_libc_module "threads/tss_set.s"
            add_libc_module "stdlib/exit_core.s"
            add_exec_tool_module "sys_exit.s"
            add_runtime_module "jumps/call_bc_runtime.s"
            ;;
            esac
        done <<< "$public_refs"
    done
}

resolve_modules() {
    local asm_file
    for asm_file in "$@"; do
        local helpers
        helpers="$(grep -o '__[A-Za-z0-9_]\+' "$asm_file" | sort -u || true)"
        while IFS= read -r helper; do
            [[ -n "$helper" ]] || continue
            case "$helper" in
        __mul16)
            add_runtime_module "int16/mulint.s"
            ;;
        __div16)
            add_runtime_module "int16/divunsigned.s"
            ;;
        __mod16)
            add_runtime_module "int16/divunsigned.s"
            add_runtime_module "int16/modunsigned.s"
            ;;
        __sdiv16)
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __divsint)
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __divschar)
            add_runtime_module "int8/divschar.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __divsuchar)
            add_runtime_module "int8/divsuchar.s"
            add_runtime_module "int8/divschar.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __divuschar)
            add_runtime_module "int8/divuschar.s"
            add_runtime_module "int8/divschar.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __divuint)
            add_runtime_module "int16/divunsigned.s"
            ;;
        __smod16)
            add_runtime_module "int16/smod16.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __modschar)
            add_runtime_module "int8/modschar.s"
            add_runtime_module "int8/divschar.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __modsint)
            add_runtime_module "int16/modsigned.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __modsuchar)
            add_runtime_module "int8/modsuchar.s"
            add_runtime_module "int8/divschar.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __moduschar)
            add_runtime_module "int8/moduschar.s"
            add_runtime_module "int8/divschar.s"
            add_runtime_module "int16/divsigned.s"
            add_runtime_module "int16/divunsigned.s"
            ;;
        __mulschar)
            add_runtime_module "int8/mulschar.s"
            add_runtime_module "int16/mulint.s"
            ;;
        __muluschar)
            add_runtime_module "int8/muluschar.s"
            add_runtime_module "int16/mulint.s"
            ;;
        __mulsuchar)
            add_runtime_module "int8/mulsuchar.s"
            add_runtime_module "int16/mulint.s"
            ;;
        __mul32)
            add_runtime_module "int32/mullong.s"
            ;;
        ___muluint2ulong)
            add_runtime_module "int32/muluint2slong.s"
            ;;
        __div32)
            add_runtime_module "int32/divulong.s"
            ;;
        __mod32)
            add_runtime_module "int32/modulong.s"
            ;;
        __sdiv32)
            add_runtime_module "int32/divslong.s"
            ;;
        __smod32)
            add_runtime_module "int32/modslong.s"
            ;;
        __mulll)
            add_runtime_module "int64/mulll.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        __divull|__div64)
            add_runtime_module "int64/divull.s"
            ;;
        __divsll|__sdiv64)
            add_runtime_module "int64/divsll.s"
            ;;
        __modull|__mod64)
            add_runtime_module "int64/modull.s"
            ;;
        __modsll|__smod64)
            add_runtime_module "int64/modsll.s"
            ;;
        __shl64)
            add_runtime_module "int64/shl64.s"
            ;;
        __shr64u)
            add_runtime_module "int64/shr64u.s"
            ;;
        __shr64s)
            add_runtime_module "int64/shr64s.s"
            ;;
        ___sint2ll)
            add_runtime_module "int64/sint2ll.s"
            ;;
        ___uint2ll)
            add_runtime_module "int64/uint2ll.s"
            ;;
        ___slong2ll)
            add_runtime_module "int64/slong2ll.s"
            ;;
        ___ulong2ll)
            add_runtime_module "int64/ulong2ll.s"
            ;;
        ___ll2sint)
            add_runtime_module "int64/ll2sint.s"
            ;;
        ___ll2uint)
            add_runtime_module "int64/ll2uint.s"
            ;;
        ___ll2slong)
            add_runtime_module "int64/ll2slong.s"
            ;;
        ___ll2ulong)
            add_runtime_module "int64/ll2ulong.s"
            ;;
        __fsadd)
            add_runtime_module "float/fsadd.s"
            add_runtime_module "float/fp_zero32.s"
            add_runtime_module "float/fppack.s"
            ;;
        __fssub)
            add_runtime_module "float/fssub.s"
            add_runtime_module "float/fsadd.s"
            add_runtime_module "float/fp_zero32.s"
            add_runtime_module "float/fppack.s"
            ;;
        __fsmul)
            add_runtime_module "float/fsmul.s"
            add_runtime_module "float/fp_zero32.s"
            add_runtime_module "float/fppack.s"
            add_runtime_module "float/fpunpack.s"
            add_runtime_module "float/fpmant.s"
            ;;
        __fsdiv)
            add_runtime_module "float/fsdiv.s"
            add_runtime_module "float/fp_zero32.s"
            add_runtime_module "float/fppack.s"
            add_runtime_module "float/fpunpack.s"
            add_runtime_module "float/fpmant.s"
            ;;
        ___fscmp)
            add_runtime_module "float/fscmp.s"
            add_runtime_module "float/fpret.s"
            ;;
        ___slong2fs)
            add_runtime_module "int32/slong2fs.s"
            add_runtime_module "int32/ulong2fs.s"
            ;;
        ___ulong2fs)
            add_runtime_module "int32/ulong2fs.s"
            ;;
        ___fs2slong)
            add_runtime_module "int32/fs2slong.s"
            add_runtime_module "float/fp_zero32.s"
            add_runtime_module "int32/fs2u32mag.s"
            ;;
        ___fs2ulong)
            add_runtime_module "int32/fs2ulong.s"
            add_runtime_module "float/fp_zero32.s"
            add_runtime_module "int32/fs2u32mag.s"
            ;;
        __dbadd|___dbadd)
            add_runtime_module "double/dbadd.s"
            add_runtime_module "double/db_zero.s"
            ;;
        __dbsub|___dbsub)
            add_runtime_module "double/dbsub.s"
            add_runtime_module "double/dbadd.s"
            add_runtime_module "double/db_zero.s"
            ;;
        __dbmul|___dbmul)
            add_runtime_module "double/dbmul.s"
            ;;
        __dbdiv|___dbdiv)
            add_runtime_module "double/dbdiv.s"
            ;;
        __dbneg|___dbneg)
            add_runtime_module "double/dbneg.s"
            ;;
        ___dbcmp)
            add_runtime_module "double/dbcmp.s"
            ;;
        ___dbeq)
            add_runtime_module "double/dbeq.s"
            add_runtime_module "double/dbcmp.s"
            ;;
        ___dblt)
            add_runtime_module "double/dblt.s"
            add_runtime_module "double/dbcmp.s"
            ;;
        ___fs2db)
            add_runtime_module "double/fs2db.s"
            add_runtime_module "double/db_zero.s"
            ;;
        ___db2fs)
            add_runtime_module "double/db2fs.s"
            ;;
        __strtod_core)
            add_libc_module "stdlib/strtod_core.s"
            ;;
        __wcstod_core)
            add_libc_module "wchar/wcstod_core.s"
            ;;
        __wcstox_core)
            add_libc_module "wchar/wcstox_core.s"
            add_runtime_module "int32/mulsint2slong.s"
            add_runtime_module "int32/muluint2slong.s"
            ;;
        __cmplxf)
            add_libc_module "complex/cmplxf.s"
            ;;
        ___sint2db|___uint2db)
            add_runtime_module "double/sint2db.s"
            add_runtime_module "double/ull2db.s"
            add_runtime_module "double/db_zero.s"
            ;;
        ___slong2db|___ulong2db)
            add_runtime_module "double/slong2db.s"
            add_runtime_module "double/ull2db.s"
            add_runtime_module "double/db_zero.s"
            ;;
        ___ull2db|___sll2db)
            add_runtime_module "double/ull2db.s"
            add_runtime_module "double/db_zero.s"
            ;;
        ___db2sint|___db2uint|___db2slong|___db2ulong|___db2ull)
            add_runtime_module "double/db2int.s"
            add_runtime_module "double/db2ll.s"
            add_runtime_module "double/db_zero.s"
            ;;
        ___db2sll)
            add_runtime_module "double/db2ll.s"
            add_runtime_module "double/db_zero.s"
            ;;
        __call_hl)
            add_runtime_module "jumps/call_hl_runtime.s"
            ;;
        __sdcc_call_hl)
            add_runtime_module "jumps/call_hl_runtime.s"
            ;;
        __sdcc_call_bc)
            add_runtime_module "jumps/call_bc_runtime.s"
            ;;
        __errno_value)
            add_libc_module "errno/errno.s"
            ;;
            esac
        done <<< "$helpers"
    done
}

# Generic fallback resolver: index every exported symbol in the library
# trees to the file defining it, then pull in files for still-unresolved
# references. Keeps the harness working as library files are split into
# one-routine-per-module granularity.
SYMBOL_INDEX_FILE=""

exported_symbols_of() {
    # exported = label:: definitions, plus .globl-declared symbols that
    # the same file defines with a single-colon label
    {
        grep -oE '^[A-Za-z_][A-Za-z0-9_]*::' "$1" 2>/dev/null | sed 's/::$//'
        comm -12 \
            <(grep -oE '^[[:space:]]*\.globl[[:space:]]+[A-Za-z_][A-Za-z0-9_]*' "$1" 2>/dev/null | awk '{print $2}' | sort -u) \
            <(grep -oE '^[A-Za-z_][A-Za-z0-9_]*:' "$1" 2>/dev/null | sed 's/:$//' | sort -u)
    } | sort -u
}

build_symbol_index() {
    [[ -n "$SYMBOL_INDEX_FILE" ]] && return
    SYMBOL_INDEX_FILE="$(mktemp /tmp/xcc_exec_symidx_XXXXXX)"
    local f sym
    while IFS= read -r f; do
        exported_symbols_of "$f" | while IFS= read -r sym; do
            printf '%s %s\n' "$sym" "$f"
        done
    done < <(find "$LIBC_SRC_DIR" "$SYS_NONE_DIR" "$RUNTIME_DIR" \
                  "$ROOT_DIR/tests/tools/z80emu" -name '*.s' 2>/dev/null) \
        >> "$SYMBOL_INDEX_FILE"
    sort -u -o "$SYMBOL_INDEX_FILE" "$SYMBOL_INDEX_FILE"
}

resolve_index_modules() {
    build_symbol_index
    local defined refs ref file f
    defined="$( { for f in "${MODULES[@]}" "$@"; do
                      [[ -f "$f" ]] || continue
                      grep -oE '^[A-Za-z_][A-Za-z0-9_]*::?' "$f" 2>/dev/null \
                          | sed 's/:*$//'
                  done; } | sort -u)"
    refs="$(cat /dev/null "${MODULES[@]}" "$@" 2>/dev/null \
        | sed 's/;.*//' \
        | grep -vE '^[[:space:]]*\.(module|file|globl|global|optsdcc|area|title)' \
        | grep -oE '[A-Za-z_][A-Za-z0-9_]*' | sort -u)"
    while IFS= read -r ref; do
        [[ -n "$ref" ]] || continue
        grep -qx "$ref" <<< "$defined" && continue
        file="$(grep -m1 "^$ref " "$SYMBOL_INDEX_FILE" | cut -d' ' -f2-)"
        [[ -n "$file" ]] && add_module "$file"
    done <<< "$refs"
}

resolve_module_closure() {
    local roots=("$@")
    local before after

    while :; do
        # explicit symbol maps first, to a fixed point
        while :; do
            before=${#MODULES[@]}
            resolve_modules "${roots[@]}"
            resolve_libc_modules "${roots[@]}"
            if [[ ${#MODULES[@]} -gt 0 ]]; then
                resolve_modules "${MODULES[@]}"
                resolve_libc_modules "${MODULES[@]}"
            fi
            after=${#MODULES[@]}
            [[ $after -eq $before ]] && break
        done
        # index-based fallback for anything the maps do not know
        before=${#MODULES[@]}
        resolve_index_modules "${roots[@]}"
        after=${#MODULES[@]}
        [[ $after -eq $before ]] && break
    done
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
        "-I$LIBC_INCLUDE_DIR" \
        "-I$COMMON_INCLUDE_DIR" \
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
    reset_modules
    resolve_module_closure "$asm_file"

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
    reset_modules
    resolve_module_closure "$asm_file"

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
need_cmd "$XAS"
need_cmd sdasz80
need_cmd sdldz80
need_cmd "$GNU_AS"
need_cmd "$GNU_LD"
need_cmd "$GNU_OBJCOPY"
build_runner
mkdir -p "$EXEC_BUILD"

while IFS= read -r c_file; do
    while IFS= read -r opt_level; do
        [[ -n "$opt_level" ]] || continue
        run_one "$c_file" "sdasz80" "$opt_level"
        run_one "$c_file" "gnuas" "$opt_level"
    done < <(opt_levels_for_test "$c_file")
done < <(find "$TEST_ROOT" -type f -name '*.c' | sort)

echo
echo "Results: ${GREEN}${PASS} passed${RESET}, ${RED}${FAIL} failed${RESET}"
echo

[[ "$FAIL" -eq 0 ]]
