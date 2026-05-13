#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
workspace_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
pid_file="$script_dir/.xdbg-z80.pid"

echo $$ > "$pid_file"
exec "$workspace_root/bin/xc/xdbg/xdbg-z80" \
    --listen 127.0.0.1:9000 \
    --load-bin "$workspace_root/bin/debug/sieve.bin" \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
