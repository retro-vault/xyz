#!/usr/bin/env bash
#
# dap_smoke_test.sh
#
# Drive xgdb's native Debug Adapter Protocol frontend against xemu and verify
# source-level breakpoint/stack behavior for both C and assembly sources.
#
# MIT License (see: LICENSE)
# Copyright (C) 2026 tomaz stih
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
XGDB="${1:-$REPO_ROOT/bin/x/bin/xgdb}"
TOOL_BIN_DIR="$(cd "$(dirname "$XGDB")" && pwd)"
XCC="${XCC:-$TOOL_BIN_DIR/xcc}"
XAS="${XAS:-$TOOL_BIN_DIR/xas}"
XLD="${XLD:-$TOOL_BIN_DIR/xld}"
XEMU="${XEMU:-$TOOL_BIN_DIR/xemu}"
XGDB="$(cd "$(dirname "$XGDB")" && pwd)/$(basename "$XGDB")"
XCC="$(cd "$(dirname "$XCC")" && pwd)/$(basename "$XCC")"
XAS="$(cd "$(dirname "$XAS")" && pwd)/$(basename "$XAS")"
XLD="$(cd "$(dirname "$XLD")" && pwd)/$(basename "$XLD")"
XEMU="$(cd "$(dirname "$XEMU")" && pwd)/$(basename "$XEMU")"

GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

skip() {
    echo "${YELLOW}SKIP${RESET}: $1"
    exit 0
}

for tool in "$XGDB" "$XCC" "$XAS" "$XLD" "$XEMU"; do
    [[ -x "$tool" ]] || skip "missing executable: $tool"
done

python3 - "$REPO_ROOT" "$XGDB" "$XCC" "$XAS" "$XLD" "$XEMU" <<'PY'
import json
import os
import pathlib
import select
import socket
import subprocess
import sys
import tempfile
import time

repo = pathlib.Path(sys.argv[1])
xgdb = pathlib.Path(sys.argv[2])
xcc = pathlib.Path(sys.argv[3])
xas = pathlib.Path(sys.argv[4])
xld = pathlib.Path(sys.argv[5])
xemu_bin = pathlib.Path(sys.argv[6])


def free_port():
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


def wait_port(port, timeout=5.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=0.15)
            s.close()
            return
        except OSError:
            time.sleep(0.05)
    raise RuntimeError(f"port {port} did not open")


class DAP:
    def __init__(self, proc):
        self.proc = proc
        self.seq = 1
        self.buffer = b""
        self.inbox = []

    def send(self, command, arguments=None):
        msg = {"seq": self.seq, "type": "request", "command": command}
        if arguments is not None:
            msg["arguments"] = arguments
        payload = json.dumps(msg, separators=(",", ":")).encode()
        os.write(
            self.proc.stdin.fileno(),
            b"Content-Length: " + str(len(payload)).encode() + b"\r\n\r\n" + payload
        )
        self.seq += 1

    def poll(self, timeout=0.0):
        ready, _, _ = select.select([self.proc.stdout.fileno()], [], [], timeout)
        if not ready:
            return []
        chunk = os.read(self.proc.stdout.fileno(), 65536)
        if not chunk:
            return []
        self.buffer += chunk
        messages = []
        while True:
            end = self.buffer.find(b"\r\n\r\n")
            if end < 0:
                break
            header = self.buffer[:end].decode(errors="replace")
            length = None
            for line in header.split("\r\n"):
                if line.lower().startswith("content-length:"):
                    length = int(line.split(":", 1)[1].strip())
            if length is None or len(self.buffer) < end + 4 + length:
                break
            payload = self.buffer[end + 4:end + 4 + length]
            self.buffer = self.buffer[end + 4 + length:]
            messages.append(json.loads(payload.decode()))
        return messages

    def wait_for(self, label, predicate, timeout=10.0):
        deadline = time.time() + timeout
        seen = []
        while time.time() < deadline:
            pending = self.inbox
            self.inbox = []
            for msg in pending:
                seen.append(msg)
                if predicate(msg):
                    index = pending.index(msg)
                    self.inbox = pending[index + 1:] + self.inbox
                    return msg, seen
            polled = self.poll(0.1)
            for index, msg in enumerate(polled):
                seen.append(msg)
                if predicate(msg):
                    self.inbox.extend(polled[index + 1:])
                    return msg, seen
                self.inbox.append(msg)
            if self.proc.poll() is not None:
                break
        stderr = b""
        try:
            ready, _, _ = select.select([self.proc.stderr], [], [], 0)
            if ready:
                stderr = os.read(self.proc.stderr.fileno(), 65536)
        except Exception:
            pass
        raise AssertionError(
            "timed out waiting for DAP message; poll="
            + repr(self.proc.poll())
            + " label="
            + label
            + " stderr="
            + stderr.decode(errors="replace")
            + " seen="
            + json.dumps(seen)
        )


def link(obj, elf, entry):
    subprocess.check_call(
        [
            str(xld), "--mode=gnu", "--oformat=elf", "-g",
            "-Ttext=0x0100", "-e", entry, "-o", str(elf), str(obj),
        ],
        cwd=repo,
    )


def run_case(kind):
    tmp = pathlib.Path(tempfile.mkdtemp(prefix=f"xgdb_dap_{kind}_"))
    port = free_port()

    if kind == "c":
        src = tmp / "dap_c.c"
        obj = tmp / "dap_c.o"
        elf = tmp / "dap_c.elf"
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
        link(obj, elf, "_main")
        bp_line = 3
    elif kind == "s":
        src = tmp / "dap_s.s"
        obj = tmp / "dap_s.o"
        elf = tmp / "dap_s.elf"
        src.write_text(
            '        .file 1 "{}"\n'
            '        .globl _start\n'
            '        .text\n'
            '_start:\n'
            '        .loc 1 5 0\n'
            '        nop\n'
            '        .loc 1 7 0\n'
            '        halt\n'.format(src),
            encoding="utf-8",
        )
        subprocess.check_call(
            [str(xas), "--mode=gnu", "-g", "-o", str(obj), str(src)],
            cwd=repo,
        )
        link(obj, elf, "_start")
        bp_line = 7
    elif kind == "bin":
        src = None
        elf = tmp / "dap_raw.bin"
        elf.write_bytes(bytes([0x00, 0x76]))  # nop; halt
        bp_line = None
        expected_pc = "0x0100"
        expected_memory = bytes([0x00, 0x76])
    elif kind == "xl":
        src = tmp / "dap_xl.s"
        obj = tmp / "dap_xl.o"
        elf = tmp / "dap_xl.xl"
        src.write_text(
            '        .globl _start\n'
            '        .text\n'
            '_start:\n'
            '        nop\n'
            '        halt\n',
            encoding="utf-8",
        )
        subprocess.check_call(
            [str(xas), "--mode=gnu", "-g", "-o", str(obj), str(src)],
            cwd=repo,
        )
        subprocess.check_call(
            [
                str(xld), "--mode=gnu", "--oformat=xl",
                "-Ttext=0x0000", "-e", "_start", "-o", str(elf), str(obj),
            ],
            cwd=repo,
        )
        bp_line = None
        expected_pc = "0x0100"
        expected_memory = bytes([0x00, 0x76])
    elif kind == "xlrel":
        src = None
        elf = tmp / "dap_reloc.xl"
        # XL v1: header, one word relocation at payload offset 1, then
        # "ld hl,0x0004; halt; nop". Loading at 0x0100 patches HL to 0x0104.
        elf.write_bytes(
            b"XL"
            + bytes([0x01, 0x00])
            + bytes([0x00, 0x00])
            + bytes([0x05, 0x00])
            + bytes([0x01, 0x00])
            + bytes([0x00, 0x00])
            + bytes([0x01, 0x00, 0x02, 0x00])
            + bytes([0x21, 0x04, 0x00, 0x76, 0x00])
        )
        bp_line = None
        expected_pc = "0x0100"
        expected_memory = bytes([0x21, 0x04, 0x01, 0x76, 0x00])
    elif kind == "xl_symbols":
        src = tmp / "dap_xl_symbols.s"
        obj = tmp / "dap_xl_symbols.o"
        elf = tmp / "dap_xl_symbols.xl"
        symbols = tmp / "dap_xl_symbols.elf"
        src.write_text(
            '        .file 1 "{}"\n'
            '        .globl _start\n'
            '        .text\n'
            '_start:\n'
            '        .loc 1 5 0\n'
            '        nop\n'
            '        .loc 1 7 0\n'
            '        halt\n'.format(src),
            encoding="utf-8",
        )
        subprocess.check_call(
            [str(xas), "--mode=gnu", "-g", "-o", str(obj), str(src)],
            cwd=repo,
        )
        subprocess.check_call(
            [
                str(xld), "--mode=gnu", "--oformat=xl", "-g",
                "-Ttext=0x0000", "-e", "_start", "-o", str(elf), str(obj),
            ],
            cwd=repo,
        )
        subprocess.check_call(
            [
                str(xld), "--mode=gnu", "--oformat=elf", "-g",
                "-Ttext=0x0000", "-e", "_start", "-o", str(symbols), str(obj),
            ],
            cwd=repo,
        )
        bp_line = 7
    else:
        raise AssertionError(f"unknown case kind: {kind}")

    xemu = subprocess.Popen(
        [str(xemu_bin), "--quiet", "--listen", f"127.0.0.1:{port}"],
        cwd=repo,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    xgdb_proc = None
    try:
        wait_port(port)
        xgdb_proc = subprocess.Popen(
            [str(xgdb), "--interpreter=dap", "--quiet"],
            cwd=tmp,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        dap = DAP(xgdb_proc)
        dap.send("initialize", {"adapterID": "xgdb", "clientID": "smoke"})
        dap.wait_for("initialize response", lambda m: m.get("type") == "response" and m.get("command") == "initialize")
        dap.wait_for("initialized event", lambda m: m.get("type") == "event" and m.get("event") == "initialized")

        launch_args = {
            "program": str(elf),
            "remoteTarget": f"127.0.0.1:{port}",
            "origin": "0x0100",
            "stopOnEntry": False,
        }
        if kind == "xl_symbols":
            launch_args["symbols"] = str(symbols)
        if kind in ("c", "s"):
            launch_args["pc"] = "0x0100"
        dap.send("launch", launch_args)
        dap.wait_for("launch response", lambda m: m.get("type") == "response" and m.get("command") == "launch")

        if kind in ("bin", "xl", "xlrel"):
            dap.send("stackTrace", {"threadId": 1})
            stack, _ = dap.wait_for(
                "raw stackTrace response",
                lambda m: m.get("type") == "response" and m.get("command") == "stackTrace"
            )
            frames = stack.get("body", {}).get("stackFrames", [])
            if not frames or frames[0].get("instructionPointerReference") != expected_pc:
                raise AssertionError(f"{kind} expected PC {expected_pc}, got stack {stack}")
            dap.send(
                "readMemory",
                {"memoryReference": "0x0100", "count": len(expected_memory)}
            )
            mem, _ = dap.wait_for(
                "raw readMemory response",
                lambda m: m.get("type") == "response" and m.get("command") == "readMemory"
            )
            import base64
            expected_data = base64.b64encode(expected_memory).decode("ascii")
            if mem.get("body", {}).get("data") != expected_data:
                raise AssertionError(f"{kind} expected downloaded bytes, got {mem}")
            dap.send("disconnect", {})
            dap.wait_for("disconnect response", lambda m: m.get("type") == "response" and m.get("command") == "disconnect")
            return

        dap.send("setBreakpoints", {
            "source": {"path": str(src), "name": src.name},
            "breakpoints": [{"line": bp_line}],
        })
        bp_response, _ = dap.wait_for(
            "setBreakpoints response",
            lambda m: m.get("type") == "response" and m.get("command") == "setBreakpoints"
        )
        bps = bp_response.get("body", {}).get("breakpoints", [])
        if not bps or not bps[0].get("verified"):
            raise AssertionError(f"{kind} breakpoint was not verified: {bp_response}")

        dap.send("configurationDone", {})
        dap.wait_for(
            "configurationDone response",
            lambda m: m.get("type") == "response" and m.get("command") == "configurationDone"
        )
        dap.send("continue", {"threadId": 1})
        dap.wait_for("continue response", lambda m: m.get("type") == "response" and m.get("command") == "continue")
        dap.wait_for("stopped event", lambda m: m.get("type") == "event" and m.get("event") == "stopped")

        dap.send("stackTrace", {"threadId": 1})
        stack, _ = dap.wait_for(
            "stackTrace response",
            lambda m: m.get("type") == "response" and m.get("command") == "stackTrace"
        )
        frames = stack.get("body", {}).get("stackFrames", [])
        if not frames:
            raise AssertionError(f"{kind} missing stack frame: {stack}")
        frame = frames[0]
        if frame.get("line") != bp_line:
            raise AssertionError(f"{kind} expected line {bp_line}, got frame {frame}")
        if frame.get("source", {}).get("path") != str(src):
            raise AssertionError(f"{kind} expected source {src}, got frame {frame}")

        dap.send("scopes", {"frameId": 1})
        scopes, _ = dap.wait_for(
            "scopes response",
            lambda m: m.get("type") == "response" and m.get("command") == "scopes"
        )
        if not scopes.get("body", {}).get("scopes"):
            raise AssertionError(f"{kind} missing scopes")

        dap.send("disconnect", {})
        dap.wait_for("disconnect response", lambda m: m.get("type") == "response" and m.get("command") == "disconnect")
    finally:
        if xgdb_proc is not None and xgdb_proc.poll() is None:
            xgdb_proc.kill()
            xgdb_proc.wait(timeout=3)
        if xemu.poll() is None:
            xemu.kill()
            xemu.wait(timeout=3)


run_case("s")
run_case("c")
run_case("bin")
run_case("xl")
run_case("xlrel")
run_case("xl_symbols")
PY

echo "${GREEN}PASS${RESET}: xgdb DAP smoke"
