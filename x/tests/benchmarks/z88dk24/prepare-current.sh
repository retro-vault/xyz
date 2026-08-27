#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
# shellcheck disable=SC1091
source "$CORPUS/current.lock"

TOOLCHAINS="${TOOLCHAINS:-$ROOT/build/toolchains}"
Z88DK="${Z88DK_CURRENT:-$TOOLCHAINS/z88dk-current}"
CC80="${Z88DK_80CC_CURRENT:-$TOOLCHAINS/z88dk-80cc-current}"
SDCC="${SDCC_CURRENT:-$TOOLCHAINS/sdcc-current}"
DOWNLOAD_ONLY=0

if [[ "${1:-}" == --download-only ]]; then
    DOWNLOAD_ONLY=1
elif [[ $# -ne 0 ]]; then
    echo "usage: $0 [--download-only]" >&2
    exit 2
fi

mkdir -p "$TOOLCHAINS"

checkout() {
    local dir="$1" url="$2" ref="$3" commit="$4" recursive="$5" actual
    if [[ ! -d "$dir/.git" ]]; then
        local -a clone_args=(--branch "$ref")
        [[ "$recursive" == 1 ]] && clone_args+=(--recursive)
        git clone "${clone_args[@]}" "$url" "$dir"
    fi
    actual="$(git -C "$dir" rev-parse HEAD)"
    if [[ "$actual" != "$commit" ]]; then
        if ! git -C "$dir" diff --quiet --ignore-submodules -- ||
           ! git -C "$dir" diff --cached --quiet --ignore-submodules --; then
            echo "refusing to replace tracked changes in $dir" >&2
            exit 2
        fi
        git -C "$dir" fetch origin "$ref"
        git -C "$dir" checkout --detach "$commit"
    fi
    if [[ "$recursive" == 1 ]]; then
        git -C "$dir" submodule update --init --recursive
    fi
}

apply_once() {
    local dir="$1" patch="$2" label="$3"
    if git -C "$dir" apply --check "$patch" 2>/dev/null; then
        git -C "$dir" apply "$patch"
    elif ! git -C "$dir" apply --reverse --check "$patch" 2>/dev/null; then
        echo "$label patch does not apply cleanly in $dir" >&2
        exit 2
    fi
}

checkout "$Z88DK" https://github.com/z88dk/z88dk.git master \
    "$Z88DK_CURRENT_COMMIT" 1
checkout "$CC80" https://github.com/z88dk/z88dk.git 80cc-multi-fixes \
    "$Z88DK_80CC_COMMIT" 1
checkout "$SDCC" https://git.code.sf.net/p/sdcc/git-mirror trunk \
    "$SDCC_CURRENT_COMMIT" 0

# Current z88dk already contains its public XCC integration.  This retained
# compatibility delta adds zcc's per-link option file and the one distinct
# register-ABI multiply helper required by fixedbench.
apply_once "$Z88DK" "$CORPUS/z88dk-xcc-points-2-3.patch" "z88dk XCC"

# Stock SDCC uses ABI revision 1 and asxxxx symbol conventions.  z88dk's
# classic library uses ABI revision 0, so an unmodified trunk compiler is not
# a valid drop-in lane.  This patch is refreshed against the exact trunk pin.
apply_once "$SDCC" "$CORPUS/sdcc-z88dk-current.patch" "SDCC/z88dk ABI"

if [[ "$DOWNLOAD_ONLY" == 1 ]]; then
    exit 0
fi

(cd "$Z88DK" && ./build.sh -l)
env PATH="$Z88DK/bin:$PATH" ZCCCFG="$Z88DK/lib/config" \
    make -C "$Z88DK/libsrc" TARGETS=test
env PATH="$Z88DK/bin:$PATH" ZCCCFG="$Z88DK/lib/config" \
    make -C "$Z88DK/libsrc" install

# Keep the active 80cc branch independently pinned even though this revision
# is already an ancestor of the selected z88dk master.
make -C "$CC80" bin/z88dk-80cc

(cd "$SDCC" && ./configure \
    --disable-ds390-port --disable-ds400-port \
    --disable-hc08-port --disable-s08-port --disable-mcs51-port \
    --disable-pic-port --disable-pic14-port --disable-pic16-port \
    --disable-tlcs90-port --disable-xa51-port --disable-stm8-port \
    --disable-pdk13-port --disable-pdk14-port \
    --disable-pdk15-port --disable-pdk16-port \
    --disable-mos6502-port --disable-mos65c02-port \
    --disable-r2k-port --disable-f8-port --disable-f8l-port \
    --disable-non-free --disable-device-lib \
    --disable-ucsim --disable-packihx \
    --disable-sdcpp --disable-sdcdb --disable-sdbinutils)
make -C "$SDCC" all

make -C "$ROOT" x-m

printf '%s\n' \
    "prepared z88dk: $Z88DK ($Z88DK_CURRENT_COMMIT)" \
    "prepared 80cc:  $CC80 ($Z88DK_80CC_COMMIT)" \
    "prepared SDCC:  $SDCC ($SDCC_CURRENT_COMMIT)" \
    "prepared XCC:   $ROOT/bin/x-m/bin/xcc (model $X_MODEL)"
