#!/usr/bin/env python3
"""Build and test CPC 464/664/6128 firmware targets through their real media."""

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
    def __init__(
        self, executable: Path, model: str, roms: Path, media: Path
    ) -> None:
        model_name = model.removeprefix("cpc-")
        arguments = [
            str(executable), "--model", model.replace("-", ""),
            "--os-rom", str(roms / f"cpc{model_name}-os.rom"),
            "--basic-rom", str(roms / f"cpc{model_name}-basic.rom"),
        ]
        if model != "cpc-464":
            arguments.extend(["--amsdos-rom", str(roms / "amsdos.rom")])
        arguments.extend(["--load", str(media)])
        self.process = subprocess.Popen(
            arguments,
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
                "clientInfo": {"name": "x-cpc-test", "version": "1"},
            },
        )
        self.notify("notifications/initialized")

    def send(self, message: dict) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(json.dumps(message, separators=(",", ":")) + "\n")
        self.process.stdin.flush()

    def notify(self, method: str) -> None:
        self.send({"jsonrpc": "2.0", "method": method})

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
                raise RuntimeError("Amstrad CPC MCP closed its output")
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
            message = result.get("content", [{}])[0].get("text", "tool failed")
            raise RuntimeError(message)
        structured = result.get("structuredContent", {})
        if not structured and result.get("content"):
            structured = {"text": result["content"][0].get("text", "")}
        return structured

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
                f"Amstrad CPC MCP exited with {self.process.returncode}"
            )

    def __enter__(self) -> "Mcp":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close()


def command(arguments: list[str]) -> None:
    subprocess.run(arguments, check=True)


def symbol(map_path: Path, name: str) -> int:
    pattern = re.compile(rf"^([0-9A-Fa-f]{{8}}) {re.escape(name)}$")
    for line in map_path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line)
        if match:
            return int(match.group(1), 16)
    raise RuntimeError(f"{name} is absent from {map_path}")


def marker(machine: Mcp, address: int) -> int:
    return machine.tool("read_memory", {"address": address, "length": 1})[
        "bytes"
    ][0]


def run_model(
    mcp: Path, roms: Path, model: str, media: Path, address: int,
    phase_address: int, seek_address: int, state_address: int | None,
) -> None:
    with Mcp(mcp, model, roms, media) as machine:
        machine.tool("run", {"frames": 100})
        if model != "cpc-464":
            machine.tool(
                "press_keys",
                {"text": "|disc\n", "hold_frames": 3, "gap_frames": 2},
            )
            machine.tool("run", {"frames": 50})
        command_text = (
            'run"!SMOKE"\n' if model == "cpc-464"
            else 'run"SMOKE"\n'
        )
        machine.tool(
            "press_keys",
            {"text": command_text, "hold_frames": 3, "gap_frames": 2},
        )
        for _ in range(32):
            machine.tool("run", {"frames": 500})
            value = marker(machine, address)
            if value not in (0, 1):
                break
        value = marker(machine, address)
        if value != 0x40:
            debug_image = Path(tempfile.gettempdir()) / f"x-{model}-failure.png"
            machine.tool("screenshot", {"path": str(debug_image)})
            cas_vectors = machine.tool(
                "read_memory", {"address": 0xBC77, "length": 36}
            )["bytes"]
            phase = marker(machine, phase_address)
            seek_result = machine.tool(
                "read_memory", {"address": seek_address, "length": 4}
            )["bytes"]
            file_state = ([] if state_address is None else machine.tool(
                "read_memory", {"address": state_address, "length": 32}
            )["bytes"])
            tape = machine.tool("tape", {"action": "status"})
            status = machine.tool("status")
            preview = machine.tool("screen_text")
            raise RuntimeError(
                f"{model}: pre-keyboard marker is 0x{value:02X}, expected 0x40; "
                f"tape={tape}; pc={status.get('pc')}; screen={preview}; "
                f"phase=0x{phase:02X}; seek_result={seek_result}; "
                f"file_state={file_state}; "
                f"cas_vectors={cas_vectors}; "
                f"screenshot={debug_image}"
            )
        machine.tool(
            "press_keys", {"text": "q", "hold_frames": 5, "gap_frames": 8}
        )
        machine.tool("run", {"frames": 100})
        value = marker(machine, address)
        if value != 0xA5:
            raise RuntimeError(
                f"{model}: final marker is 0x{value:02X}, expected 0xA5"
            )
        print(f"PASS {model} ({media.suffix.upper()}, libc marker 0xA5)")


def main() -> int:
    script = Path(__file__).resolve()
    repo_root = script.parents[4]
    mcp_root = repo_root.parent / "amstrad-cpc-mcp"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--mcp",
        type=Path,
        default=os.environ.get(
            "AMSTRAD_CPC_MCP", mcp_root / "bin/bin/amstrad-cpc-mcp"
        ),
    )
    parser.add_argument(
        "--roms",
        type=Path,
        default=os.environ.get("AMSTRAD_CPC_ROMS", mcp_root / "data/roms"),
    )
    parser.add_argument("--xcc", type=Path, default=repo_root / "bin/x/bin/xcc")
    parser.add_argument(
        "--xprog", type=Path, default=repo_root / "bin/x/bin/xprog"
    )
    parser.add_argument(
        "--allow-missing", action="store_true",
        help="report an optional skip when the MCP executable or ROMs are absent",
    )
    args = parser.parse_args()

    required = [args.mcp, args.roms / "cpc464-os.rom",
                args.roms / "cpc464-basic.rom",
                args.roms / "cpc664-os.rom",
                args.roms / "cpc664-basic.rom",
                args.roms / "cpc6128-os.rom",
                args.roms / "cpc6128-basic.rom",
                args.roms / "amsdos.rom"]
    missing = [path for path in required if not path.exists()]
    if missing:
        if args.allow_missing:
            print("SKIP CPC MCP/ROM coverage: "
                  + ", ".join(str(path) for path in missing))
            return 0
        raise RuntimeError("missing CPC MCP/ROM input: "
                           + ", ".join(str(path) for path in missing))

    smoke = script.with_name("smoke.c")
    with tempfile.TemporaryDirectory(prefix="x-cpc-") as temporary:
        work = Path(temporary)
        for model in ("cpc-464", "cpc-664", "cpc-6128"):
            binary = work / f"{model}.bin"
            map_path = work / f"{model}.map"
            disk = model != "cpc-464"
            media = work / f"{model}.{'dsk' if disk else 'cdt'}"
            command(
                [
                    str(args.xcc), "-Os", f"--platform={model}",
                    "--oformat=binary", f"-DCPC_HAS_DISK={int(disk)}",
                    f"-Map={map_path}", str(smoke), "-o", str(binary),
                ]
            )
            command(
                [
                    str(args.xprog), "--dsk" if disk else "--cdt",
                    str(binary), "-o", str(media), "--name",
                    "SMOKE.BIN" if disk else "SMOKE",
                ]
            )
            run_model(
                args.mcp, args.roms, model, media,
                symbol(map_path, "_cpc_smoke_result"),
                symbol(map_path, "_cpc_smoke_phase"),
                symbol(map_path, "_cpc_smoke_seek_result"),
                symbol(map_path, "__cpc_input_open") if disk else None,
            )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
