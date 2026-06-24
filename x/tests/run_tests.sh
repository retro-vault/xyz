#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
X_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$X_ROOT/.." && pwd)"
RUNNER_DIR="$X_ROOT/tests/tools/xemutest"
RUNNER_BIN="$REPO_ROOT/build/tests/tools/xemutest/xemutest"
SUITE_ROOT="$X_ROOT/tests/tests/c23"
XCC_BIN="${XCC:-$REPO_ROOT/bin/x/bin/xcc}"

if [[ $# -gt 0 && "${1:-}" != --* ]]; then
    XCC_BIN="$1"
    shift
fi

make -C "$RUNNER_DIR" \
    X_ROOT="$X_ROOT" REPO_ROOT="$REPO_ROOT" \
    BUILD_DIR="$REPO_ROOT/build" DIST_DIR="$REPO_ROOT/bin/x" \
    all >/dev/null

"$RUNNER_BIN" --xcc "$XCC_BIN" --suite "$SUITE_ROOT" "$@"
