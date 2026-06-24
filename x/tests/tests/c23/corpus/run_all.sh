#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

bash "$SCRIPT_DIR/c23-projects/run.sh"
bash "$SCRIPT_DIR/thealgorithms-c/run.sh"
