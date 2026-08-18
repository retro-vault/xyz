#!/usr/bin/env python3
"""Build and exercise the ZX48 RAM, ROM, TAP, and TZX targets via MCP."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


class Mcp:
    def __init__(self, executable: Path, rom: Path):
        self.process = subprocess.Popen(
            [str(executable), "--rom", str(rom)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=None,
            text=True,
            bufsize=1,
        )
        self.request_id = 0
        self.request(
            "initialize",
            {
                "protocolVersion": "2025-06-18",
                "capabilities": {},
                "clientInfo": {"name": "x-zx48-test", "version": "1"},
            },
        )
        self.notify("notifications/initialized")

    def send(self, message: dict) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(json.dumps(message, separators=(",", ":")) + "\n")
        self.process.stdin.flush()

    def notify(self, method: str, params: dict | None = None) -> None:
        message = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            message["params"] = params
        self.send(message)

    def request(self, method: str, params: dict) -> dict:
        self.request_id += 1
        wanted = self.request_id
        self.send(
            {
                "jsonrpc": "2.0",
                "id": wanted,
                "method": method,
                "params": params,
            }
        )
        assert self.process.stdout is not None
        while True:
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError("ZX Spectrum MCP closed its output")
            response = json.loads(line)
            if response.get("id") != wanted:
                continue
            if "error" in response:
                raise RuntimeError(str(response["error"]))
            return response["result"]

    def tool(self, name: str, arguments: dict | None = None) -> dict:
        result = self.request(
            "tools/call", {"name": name, "arguments": arguments or {}}
        )
        if result.get("isError"):
            text = result.get("content", [{}])[0].get("text", "tool failed")
            raise RuntimeError(text)
        return result.get("structuredContent", {})

    def close(self) -> None:
        if self.process.stdin is not None:
            self.process.stdin.close()
        try:
            self.process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            self.process.terminate()
            self.process.wait(timeout=2)
        if self.process.returncode not in (0, -15):
            raise RuntimeError(
                f"ZX Spectrum MCP exited with {self.process.returncode}"
            )

    def __enter__(self) -> "Mcp":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close()


def command(argv: list[str]) -> None:
    subprocess.run(argv, check=True)


def symbol(map_path: Path, name: str) -> int:
    pattern = re.compile(rf"^([0-9A-Fa-f]{{8}}) {re.escape(name)}$")
    for line in map_path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line)
        if match:
            return int(match.group(1), 16)
    raise RuntimeError(f"{name} is absent from {map_path}")


def expect_marker(
    machine: Mcp, address: int, input_address: int, label: str
) -> None:
    memory = machine.tool("read_memory", {"address": address, "length": 1})
    value = memory["bytes"][0]
    if value != 0xA5:
        typed = machine.tool(
            "read_memory", {"address": input_address, "length": 4}
        )["bytes"]
        raise RuntimeError(
            f"{label}: smoke marker is 0x{value:02X}, expected 0xA5; "
            f"keyboard bytes are {typed}"
        )
    print(f"PASS {label} (marker 0x{address:04X}=0xA5)")


def type_test_keys(machine: Mcp) -> None:
    # The first key is consumed by the public non-blocking polling API. The
    # remaining four are consumed by blocking read/getchar built on it.
    machine.tool(
        "press_keys",
        {"text": "q", "hold_frames": 5, "gap_frames": 8},
    )
    # Keep identical A-key strokes in distinct calls so the polling backend
    # observes a complete release before CAPS SHIFT+A follows plain A.
    for character in ("a", "A", "!", "\n"):
        machine.tool(
            "press_keys",
            {"text": character, "hold_frames": 5, "gap_frames": 8},
        )


def finish_keyboard_test(
    machine: Mcp, marker: int, input_address: int, label: str
) -> None:
    machine.tool("run", {"frames": 100})
    type_test_keys(machine)
    machine.tool("run", {"frames": 100, "stop_on_halt": True})
    expect_marker(machine, marker, input_address, label)


def type_rom_load(machine: Mcp) -> None:
    # Consecutive SYMBOL SHIFT+P chords need a longer release interval than
    # ordinary typing for the real 48K ROM keyboard debounce.
    machine.tool("press_keys", {"text": "j "})
    quote = {"text": '"', "hold_frames": 5, "gap_frames": 8}
    machine.tool("press_keys", quote)
    machine.tool("press_keys", quote)
    machine.tool(
        "press_keys",
        {"keys": ["ENTER"], "hold_frames": 5, "gap_frames": 8},
    )


def test_tape(
    mcp: Path, rom: Path, tape: Path, marker: int, input_address: int
) -> None:
    label = tape.suffix[1:].upper()
    with Mcp(mcp, rom) as machine:
        machine.tool("run", {"frames": 100})
        machine.tool("load", {"path": str(tape), "autoplay": False})
        type_rom_load(machine)
        machine.tool("tape", {"action": "play"})
        for _ in range(3):
            machine.tool("run", {"frames": 1000})
        status = machine.tool("tape", {"action": "status"})
        if not status.get("finished"):
            raise RuntimeError(f"{label}: tape did not reach its end")
        machine.tool("run", {"frames": 100})
        type_test_keys(machine)
        machine.tool("run", {"frames": 100, "stop_on_halt": True})
        expect_marker(machine, marker, input_address, label)


def main() -> int:
    script = Path(__file__).resolve()
    repo_root = script.parents[4]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--mcp",
        type=Path,
        default=os.environ.get("ZX_SPECTRUM_MCP"),
        required="ZX_SPECTRUM_MCP" not in os.environ,
        help="path to zx-spectrum-mcp (or set ZX_SPECTRUM_MCP)",
    )
    parser.add_argument(
        "--rom",
        type=Path,
        default=os.environ.get("ZX_SPECTRUM_ROM"),
        required="ZX_SPECTRUM_ROM" not in os.environ,
        help="path to the standard 16K 48.rom (or set ZX_SPECTRUM_ROM)",
    )
    parser.add_argument(
        "--xcc", type=Path, default=repo_root / "bin/x/bin/xcc"
    )
    parser.add_argument(
        "--xprog", type=Path, default=repo_root / "bin/x/bin/xprog"
    )
    args = parser.parse_args()

    smoke = script.with_name("smoke.c")
    with tempfile.TemporaryDirectory(prefix="x-zx48-") as temporary:
        work = Path(temporary)
        ram_binary = work / "smoke.bin"
        ram_map = work / "smoke-ram.map"
        rom_binary = work / "smoke.rom"
        rom_map = work / "smoke-rom.map"
        tap = work / "smoke.tap"
        tzx = work / "smoke.tzx"

        command(
            [
                str(args.xcc), "-Os", "--platform=zx-ram",
                "--oformat=binary", f"-Map={ram_map}", str(smoke),
                "-o", str(ram_binary),
            ]
        )
        command(
            [
                str(args.xcc), "-Os", "--platform=zx-rom",
                "--oformat=binary", f"-Map={rom_map}", str(smoke),
                "-o", str(rom_binary),
            ]
        )
        command(
            [str(args.xprog), "--tap", str(ram_binary), "-o", str(tap),
             "--name", "ZX48SMOKE"]
        )
        command(
            [str(args.xprog), "--tzx", str(ram_binary), "-o", str(tzx),
             "--name", "ZX48SMOKE"]
        )

        ram_marker = symbol(ram_map, "_zx_smoke_result")
        ram_input = symbol(ram_map, "_zx_smoke_input")
        rom_marker = symbol(rom_map, "_zx_smoke_result")
        rom_input = symbol(rom_map, "_zx_smoke_input")

        with Mcp(args.mcp, args.rom) as machine:
            machine.tool(
                "load",
                {
                    "path": str(ram_binary), "format": "binary",
                    "address": 0x5CCB, "start": 0x5CCB, "reset": True,
                },
            )
            finish_keyboard_test(machine, ram_marker, ram_input, "RAM binary")

        with Mcp(args.mcp, rom_binary) as machine:
            finish_keyboard_test(
                machine, rom_marker, rom_input, "replacement ROM"
            )

        test_tape(args.mcp, args.rom, tap, ram_marker, ram_input)
        test_tape(args.mcp, args.rom, tzx, ram_marker, ram_input)

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
