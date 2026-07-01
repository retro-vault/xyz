#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/../../.." && pwd)

IMAGE=${WINDOWS_MINGW_IMAGE:-wischner/gcc-x86_64-windows-mingw-w64:1.0.0}
TARGET_CXX=${WINDOWS_MINGW_CXX:-x86_64-w64-mingw32-g++}
JOBS=${WINDOWS_MINGW_JOBS:-$(nproc)}

BUILD_DIR_IN_CONTAINER=${WINDOWS_MINGW_BUILD_DIR_IN_CONTAINER:-/work/build/x-mingw-docker}
DIST_DIR_IN_CONTAINER=${WINDOWS_MINGW_DIST_DIR_IN_CONTAINER:-/work/bin/x-mingw-docker}
HOST_BIN_DIR_IN_CONTAINER=${WINDOWS_MINGW_HOST_BIN_DIR_IN_CONTAINER:-$DIST_DIR_IN_CONTAINER/bin}
PUBLIC_LIB_DIR_IN_CONTAINER=${WINDOWS_MINGW_PUBLIC_LIB_DIR_IN_CONTAINER:-$DIST_DIR_IN_CONTAINER/lib}

docker run --rm \
  -u "$(id -u):$(id -g)" \
  -v "$REPO_ROOT":/work \
  -w /work \
  "$IMAGE" \
  bash -lc "
    $TARGET_CXX --version | head -n 1
    make -C x/src \
      ROOT=/work/x \
      BUILD_DIR=$BUILD_DIR_IN_CONTAINER \
      DIST_DIR=$DIST_DIR_IN_CONTAINER \
      HOST_BIN_DIR=$HOST_BIN_DIR_IN_CONTAINER \
      PUBLIC_LIB_DIR=$PUBLIC_LIB_DIR_IN_CONTAINER \
      CXX=$TARGET_CXX \
      -j$JOBS
  "
