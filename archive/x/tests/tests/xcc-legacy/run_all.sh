#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
XCC="${1:?usage: run_all.sh /path/to/xcc}"

bash "$SCRIPT_DIR/run_tests.sh" "$XCC"
bash "$SCRIPT_DIR/run_exec_tests.sh" "$XCC"
bash "$SCRIPT_DIR/run_external_tests.sh" "$XCC"
XCC="$XCC" bash "$SCRIPT_DIR/run_external_c23.sh"
bash "$SCRIPT_DIR/run_xemutests.sh" "$XCC" --filter c23_
