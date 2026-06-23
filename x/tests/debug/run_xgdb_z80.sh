#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
x_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
repo_root=$(CDPATH= cd -- "$x_root/.." && pwd)
pid_file="$script_dir/.xgdb-z80.pid"

echo $$ > "$pid_file"
exec "$repo_root/bin/x/bin/xgdb-z80" \
    --listen 127.0.0.1:9000 \
    --load-bin "$repo_root/bin/z/z80/bin/apps/debug/sieve.bin" \
    --origin 0x0100 \
    --pc 0x0100 \
    --sp 0xFFFE
