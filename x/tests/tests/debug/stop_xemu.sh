#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
pid_file="$script_dir/.xemu.pid"

if [ ! -f "$pid_file" ]; then
    echo "xemu pid file not found: $pid_file" >&2
    exit 1
fi

pid=$(cat "$pid_file")
kill "$pid"
rm -f "$pid_file"
