#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
pid_file="$script_dir/.xgdb-z80.pid"

if [ ! -f "$pid_file" ]; then
    exit 0
fi

pid=$(cat "$pid_file")

if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
    kill "$pid" 2>/dev/null || true
    count=0
    while kill -0 "$pid" 2>/dev/null; do
        count=$((count + 1))
        if [ "$count" -ge 20 ]; then
            kill -9 "$pid" 2>/dev/null || true
            break
        fi
        sleep 0.1
    done
fi

rm -f "$pid_file"
