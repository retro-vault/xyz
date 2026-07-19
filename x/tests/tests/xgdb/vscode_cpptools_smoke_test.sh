#!/usr/bin/env bash
#
# vscode_cpptools_smoke_test.sh
#
# Drive VS Code's cpptools debug adapter headlessly against xgdb/xemu and
# verify that the adapter reaches a source-mapped C stack frame.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
XGDB="${1:-$REPO_ROOT/bin/x/bin/xgdb}"
TOOL_BIN_DIR="$(cd "$(dirname "$XGDB")" && pwd)"
XCC="${XCC:-$TOOL_BIN_DIR/xcc}"
XLD="${XLD:-$TOOL_BIN_DIR/xld}"
XEMU="${XEMU:-$TOOL_BIN_DIR/xemu}"
XGDB="$(cd "$(dirname "$XGDB")" && pwd)/$(basename "$XGDB")"
XCC="$(cd "$(dirname "$XCC")" && pwd)/$(basename "$XCC")"
XLD="$(cd "$(dirname "$XLD")" && pwd)/$(basename "$XLD")"
XEMU="$(cd "$(dirname "$XEMU")" && pwd)/$(basename "$XEMU")"

GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

skip() {
    echo "${YELLOW}SKIP${RESET}: $1"
    exit 0
}

for tool in "$XGDB" "$XCC" "$XLD" "$XEMU"; do
    [[ -x "$tool" ]] || skip "missing executable: $tool"
done

command -v code >/dev/null 2>&1 || skip "VS Code CLI not installed"
CPPTOOLS_EXT="$(code --locate-extension ms-vscode.cpptools 2>/dev/null || true)"
[[ -n "$CPPTOOLS_EXT" ]] || skip "VS Code cpptools extension not installed"
ADAPTER="$CPPTOOLS_EXT/debugAdapters/bin/OpenDebugAD7"
[[ -x "$ADAPTER" ]] || skip "cpptools OpenDebugAD7 adapter not executable"

python3 - "$REPO_ROOT" "$XGDB" "$XCC" "$XLD" "$XEMU" "$ADAPTER" <<'PY'
import json
import os
import pathlib
import select
import socket
import stat
import subprocess
import sys
import tempfile
import time

repo = pathlib.Path(sys.argv[1])
xgdb = pathlib.Path(sys.argv[2])
xcc = pathlib.Path(sys.argv[3])
xld = pathlib.Path(sys.argv[4])
xemu_bin = pathlib.Path(sys.argv[5])
adapter = pathlib.Path(sys.argv[6])

tmp = pathlib.Path(tempfile.mkdtemp(prefix="xgdb_vscode_"))
src = tmp / "source_level.c"
obj = tmp / "source_level.o"
elf = tmp / "source_level.elf"
src.write_text(
    'int main(void) {\n'
    '    __asm__("nop");\n'
    '    __asm__("halt");\n'
    '    return 0;\n'
    '}\n',
    encoding="utf-8",
)

subprocess.check_call(
    [str(xcc), "-c", "-g", "--mode=gnu", "-nostdlib", "-o", str(obj), str(src)],
    cwd=repo,
)
subprocess.check_call(
    [
        str(xld), "--mode=gnu", "--oformat=elf", "-g",
        "-Ttext=0x0100", "-e", "_main", "-o", str(elf), str(obj),
    ],
    cwd=repo,
)

sock = socket.socket()
sock.bind(("127.0.0.1", 0))
port = sock.getsockname()[1]
sock.close()

xemu = subprocess.Popen(
    [str(xemu_bin), "--quiet", "--listen", f"127.0.0.1:{port}"],
    cwd=repo,
    stdout=subprocess.DEVNULL,
    stderr=subprocess.DEVNULL,
)
time.sleep(0.2)

mi_in = tmp / "mi.in.log"
mi_out = tmp / "mi.out.log"
mi_err = tmp / "mi.err.log"
wrapper = tmp / "xgdb-mi-log.sh"
wrapper.write_text(
    f'#!/usr/bin/env bash\n'
    f'tee {mi_in} | {xgdb} "$@" 2>{mi_err} | tee {mi_out}\n',
    encoding="utf-8",
)
wrapper.chmod(wrapper.stat().st_mode | stat.S_IXUSR)

dap = subprocess.Popen(
    [str(adapter)],
    cwd=tmp,
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)

seq = 1
buffer = b""

def send(command, arguments=None):
    global seq
    message = {"seq": seq, "type": "request", "command": command}
    if arguments is not None:
        message["arguments"] = arguments
    payload = json.dumps(message, separators=(",", ":")).encode("utf-8")
    dap.stdin.write(b"Content-Length: " + str(len(payload)).encode("ascii") + b"\r\n\r\n" + payload)
    dap.stdin.flush()
    seq += 1

def drain_dap():
    global buffer
    while True:
        ready, _, _ = select.select([dap.stdout], [], [], 0)
        if not ready:
            return
        chunk = os.read(dap.stdout.fileno(), 8192)
        if not chunk:
            return
        buffer += chunk
        while True:
            header_end = buffer.find(b"\r\n\r\n")
            if header_end < 0:
                break
            header = buffer[:header_end].decode("utf-8", errors="replace")
            length = None
            for line in header.split("\r\n"):
                if line.lower().startswith("content-length:"):
                    length = int(line.split(":", 1)[1].strip())
            if length is None or len(buffer) < header_end + 4 + length:
                break
            buffer = buffer[header_end + 4 + length:]

def mi_log_contains_source_stack():
    if not mi_out.exists():
        return False
    text = mi_out.read_text(encoding="utf-8", errors="replace")
    return (
        "stack=[" in text
        and f'fullname="{src}"' in text
        and ('line="3"' in text or 'line="4"' in text)
    )

try:
    send("initialize", {
        "clientID": "xgdb-vscode-smoke",
        "adapterID": "cppdbg",
        "pathFormat": "path",
        "linesStartAt1": True,
        "columnsStartAt1": True,
        "supportsRunInTerminalRequest": False,
    })
    send("launch", {
        "name": "xgdb source smoke",
        "type": "cppdbg",
        "request": "launch",
        "program": str(elf),
        "cwd": str(tmp),
        "MIMode": "gdb",
        "miDebuggerPath": str(wrapper),
        "miDebuggerServerAddress": f"127.0.0.1:{port}",
        "externalConsole": False,
        "stopAtEntry": False,
        "setupCommands": [],
        "logging": {
            "engineLogging": True,
            "trace": True,
            "traceResponse": True,
        },
    })
    send("setBreakpoints", {
        "source": {"path": str(src), "name": src.name},
        "breakpoints": [{"line": 4}],
        "sourceModified": False,
    })
    send("configurationDone", {})

    deadline = time.time() + 120
    while time.time() < deadline:
        drain_dap()
        if mi_log_contains_source_stack():
            print("PASS: VS Code cpptools source smoke")
            sys.exit(0)
        if dap.poll() is not None:
            break
        time.sleep(0.05)

    drain_dap()
    print("--- MI input ---", file=sys.stderr)
    print(mi_in.read_text(encoding="utf-8", errors="replace") if mi_in.exists() else "", file=sys.stderr)
    print("--- MI output ---", file=sys.stderr)
    print(mi_out.read_text(encoding="utf-8", errors="replace") if mi_out.exists() else "", file=sys.stderr)
    print("--- MI stderr ---", file=sys.stderr)
    print(mi_err.read_text(encoding="utf-8", errors="replace") if mi_err.exists() else "", file=sys.stderr)
    raise SystemExit("VS Code cpptools source smoke did not reach a source stack frame")
finally:
    try:
        send("disconnect", {"terminateDebuggee": True})
        time.sleep(0.2)
    except Exception:
        pass
    for proc in (dap, xemu):
        try:
            proc.terminate()
            proc.wait(timeout=2)
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass
PY

echo "${GREEN}PASS${RESET}: xgdb VS Code cpptools smoke"
