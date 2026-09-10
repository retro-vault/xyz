#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
CORPUS="$ROOT/x/tests/benchmarks/z88dk24"
source "$CORPUS/latest.lock"
TOOLS="${LATEST_TOOLCHAINS:-$ROOT/build/toolchains/$TOOLCHAIN_SNAPSHOT}"
NIGHTLY="$TOOLS/nightly-source/z88dk"
SDCC="$TOOLS/sdcc-trunk"
DOWNLOAD_ONLY=0
if [[ "${1:-}" == --download-only ]]; then
    DOWNLOAD_ONLY=1
elif [[ $# != 0 ]]; then
    echo "usage: $0 [--download-only]" >&2
    exit 2
fi
mkdir -p "$TOOLS"

download() {
    local url="$1" path="$2" expected="$3"
    if [[ ! -f "$path" ]]; then
        curl -fL --retry 2 --dump-header "$path.headers" -o "$path" "$url"
    fi
    [[ "$(sha256sum "$path" | awk '{print $1}')" == "$expected" ]] || {
        echo "source archive checksum mismatch: $path" >&2
        exit 2
    }
}

checkout() {
    local directory="$1" url="$2" commit="$3"
    if [[ ! -d "$directory/.git" ]]; then
        git clone --no-checkout "$url" "$directory"
        git -C "$directory" checkout --detach "$commit"
    fi
    [[ "$(git -C "$directory" rev-parse HEAD)" == "$commit" ]] || {
        echo "refusing to replace a different source snapshot: $directory" >&2
        exit 2
    }
}

apply_once() {
    local directory="$1" patch="$2"
    if (cd "$directory" && patch -p1 --dry-run --forward < "$patch") >/dev/null 2>&1; then
        (cd "$directory" && patch -p1 --forward < "$patch")
    elif ! (cd "$directory" && patch -p1 --dry-run --reverse < "$patch") >/dev/null 2>&1; then
        echo "compatibility patch does not match $directory: $patch" >&2
        exit 2
    fi
}

download "$NIGHTLY_ARCHIVE_URL" "$TOOLS/z88dk-latest.tgz" "$NIGHTLY_SHA256"
if [[ ! -d "$NIGHTLY" ]]; then
    mkdir -p "$TOOLS/nightly-source"
    tar xzf "$TOOLS/z88dk-latest.tgz" -C "$TOOLS/nightly-source"
    printf '%s\n' "$NIGHTLY_SHA256" > "$NIGHTLY/.latest-source-sha256"
fi
[[ "$(cat "$NIGHTLY/.latest-source-sha256")" == "$NIGHTLY_SHA256" ]] || {
    echo "nightly source directory lacks the expected archive identity" >&2
    exit 2
}
checkout "$TOOLS/z88dk-master" https://github.com/z88dk/z88dk.git "$Z88DK_CURRENT_COMMIT"
checkout "$SDCC" https://git.code.sf.net/p/sdcc/git-mirror "$SDCC_CURRENT_COMMIT"
apply_once "$NIGHTLY" "$CORPUS/latest/nightly-xcc-options.patch"
apply_once "$SDCC" "$CORPUS/sdcc-z88dk-current.patch"
apply_once "$SDCC" "$CORPUS/latest/official-sdcc-predicate-alias.patch"
download "http://nightly.z88dk.org/zsdcc/zsdcc_r${NIGHTLY_SDCC_REVISION}_src.tar.gz" \
    "$NIGHTLY/zsdcc_r${NIGHTLY_SDCC_REVISION}_src.tar.gz" "$NIGHTLY_SDCC_SOURCE_SHA256"

if [[ "$DOWNLOAD_ONLY" == 1 ]]; then
    exit 0
fi

# Reuse a local Perl dependency prefix when one has been prepared in this
# workspace; otherwise build.sh checks the host's ordinary dependencies.
if [[ -d "$ROOT/build/optimization-campaign/perl5/lib/perl5" ]]; then
    export PERL5LIB="$ROOT/build/optimization-campaign/perl5/lib/perl5${PERL5LIB:+:$PERL5LIB}"
fi
export MAKEFLAGS="-j${JOBS:-2}"
(cd "$NIGHTLY" && ./build.sh -l)
env PATH="$NIGHTLY/bin:$PATH" ZCCCFG="$NIGHTLY/lib/config" \
    make -C "$NIGHTLY/libsrc" TARGETS=test
env PATH="$NIGHTLY/bin:$PATH" ZCCCFG="$NIGHTLY/lib/config" \
    make -C "$NIGHTLY/libsrc" install
make -C "$NIGHTLY" BUILD_SDCC=1 BUILD_SDCC_HTTP=1 bin/z88dk-zsdcc

(cd "$SDCC" && ./configure \
    --disable-ds390-port --disable-ds400-port \
    --disable-hc08-port --disable-s08-port --disable-mcs51-port \
    --disable-pic-port --disable-pic14-port --disable-pic16-port \
    --disable-tlcs90-port --disable-xa51-port --disable-stm8-port \
    --disable-pdk13-port --disable-pdk14-port --disable-pdk15-port --disable-pdk16-port \
    --disable-mos6502-port --disable-mos65c02-port \
    --disable-r2k-port --disable-f8-port --disable-f8l-port \
    --disable-non-free --disable-device-lib --disable-ucsim --disable-packihx \
    --disable-sdcpp --disable-sdcdb --disable-sdbinutils)
make -C "$SDCC" all
python3 "$CORPUS/latest/receipt.py" capture "$TOOLS"
printf '%s\n' "prepared fresh nightly and official SDCC in $TOOLS"
