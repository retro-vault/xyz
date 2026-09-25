#!/bin/sh
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec python3 "$base/scripts/run-yos.py" 128 \
    --rom "$base/arch/128/yos-kernel.rom" \
    --shell "$base/arch/128/shell.sys" "$@"
