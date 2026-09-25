#!/bin/sh
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec python3 "$base/scripts/run-yos.py" 48 \
    --rom "$base/arch/48/yos-kernel.rom" \
    --shell "$base/arch/48/shell.sys" "$@"
