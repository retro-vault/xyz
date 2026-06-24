#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
X_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

exec bash "$X_ROOT/tests/run_tests.sh" "$@"
