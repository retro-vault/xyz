#!/bin/sh
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec python3 "$base/scripts/run-yos.py" next \
    --rom "$base/arch/next/yos-kernel.rom" \
    --shell "$base/arch/next/shell.sys" "$@"
