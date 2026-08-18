#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
# shellcheck disable=SC1091
source "$CORPUS/toolchains.lock"

TOOLCHAINS="${TOOLCHAINS:-$ROOT/build/toolchains}"
BASE="${Z88DK_BASE:-$TOOLCHAINS/z88dk-benchmark-base}"
NIGHTLY="${Z88DK_NIGHTLY:-$TOOLCHAINS/z88dk-nightly}"
CC80="${Z88DK_80CC:-$TOOLCHAINS/z88dk-80cc-latest}"
DOWNLOAD_ONLY=0

if [[ "${1:-}" == --download-only ]]; then
    DOWNLOAD_ONLY=1
elif [[ $# -ne 0 ]]; then
    echo "usage: $0 [--download-only]" >&2
    exit 2
fi

mkdir -p "$TOOLCHAINS"

checkout() {
    local dir="$1" commit="$2" actual
    if [[ ! -d "$dir/.git" ]]; then
        git clone --recursive https://github.com/z88dk/z88dk.git "$dir"
    fi
    actual="$(git -C "$dir" rev-parse HEAD)"
    if [[ "$actual" != "$commit" ]]; then
        if ! git -C "$dir" diff --quiet --ignore-submodules -- ||
           ! git -C "$dir" diff --cached --quiet --ignore-submodules --; then
            echo "refusing to replace tracked changes in $dir" >&2
            exit 2
        fi
        git -C "$dir" fetch origin "$commit"
        git -C "$dir" checkout --detach "$commit"
    fi
    git -C "$dir" submodule update --init --recursive
}

checkout "$BASE" "$Z88DK_BASE_COMMIT"
if git -C "$BASE" apply --check "$CORPUS/z88dk-base-xcc.patch" 2>/dev/null; then
    git -C "$BASE" apply "$CORPUS/z88dk-base-xcc.patch"
elif ! git -C "$BASE" apply --reverse --check "$CORPUS/z88dk-base-xcc.patch" 2>/dev/null; then
    echo "base XCC patch does not apply cleanly in $BASE" >&2
    exit 2
fi

checkout "$NIGHTLY" "$Z88DK_NIGHTLY_COMMIT"
checkout "$CC80" "$Z88DK_80CC_COMMIT"
if git -C "$CC80" apply --check "$CORPUS/z88dk-80cc.patch" 2>/dev/null; then
    git -C "$CC80" apply "$CORPUS/z88dk-80cc.patch"
elif ! git -C "$CC80" apply --reverse --check "$CORPUS/z88dk-80cc.patch" 2>/dev/null; then
    echo "80cc compatibility patch does not apply cleanly in $CC80" >&2
    exit 2
fi

if [[ "$DOWNLOAD_ONLY" == 1 ]]; then
    exit 0
fi

# The base supplies the host driver plus only the `+test` CRT/classic library
# needed here.  Avoid spending time building every unrelated z88dk target.
(cd "$BASE" && ./build.sh -l)
env PATH="$BASE/bin:$PATH" ZCCCFG="$BASE/lib/config" \
    make -C "$BASE/libsrc" TARGETS=test
env PATH="$BASE/bin:$PATH" ZCCCFG="$BASE/lib/config" \
    make -C "$BASE/libsrc" install

# The nightly supplies host tools and zsdcc; target libraries deliberately
# remain those from BASE, so a host-only build is sufficient here.
(cd "$NIGHTLY" && ./build.sh -l)
make -C "$NIGHTLY" BUILD_SDCC=1 BUILD_SDCC_HTTP=1 bin/z88dk-zsdcc

# 80cc is developed in the same repository.  Build the separately pinned
# active branch without rebuilding a second copy of every target library.
make -C "$CC80" bin/z88dk-80cc

# XCC must be the M distribution even though these integer programs link the
# shared z88dk library rather than X's own libc.
make -C "$ROOT" x-m

printf '%s\n' \
    "prepared base:    $BASE ($Z88DK_BASE_COMMIT)" \
    "prepared nightly: $NIGHTLY ($Z88DK_NIGHTLY_COMMIT)" \
    "prepared 80cc:    $CC80 ($Z88DK_80CC_COMMIT)" \
    "prepared XCC:     $ROOT/bin/x-m/bin/xcc (model $X_MODEL)"
