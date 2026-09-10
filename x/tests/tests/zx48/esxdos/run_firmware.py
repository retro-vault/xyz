#!/usr/bin/env python3
"""Run the public C disk probe on real esxDOS firmware and emulated divIDE.

ZEsarUX must have its ROM/resource files alongside the executable. The IDE
image must contain the matching esxDOS SYS files. A private image copy is
used; this never changes the supplied filesystem image.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import socket
import subprocess
import time

PROMPT = re.compile(rb"\ncommand[^\n]*> $")

class Zrcp:
    def __init__(self, executable: Path, rom: Path, disk: Path, work: Path,
                 base_rom: Path | None = None):
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        self.log = (work / "emulator.log").open("w")
        self.command = [str(executable), "--noconfigfile", "--machine", "48k",
                        "--vo", "null", "--ao", "null", "--nowelcomemessage",
                        "--ide-file", str(disk), "--enable-ide", "--enable-divide",
                        "--divide-rom", str(rom), "--enable-remoteprotocol",
                        "--remoteprotocol-port", str(port)]
        if base_rom is not None:
            self.command += ["--romfile", str(base_rom)]
        self.process = subprocess.Popen(self.command, cwd=executable.parent,
                                        stdout=self.log, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 15
        while True:
            try:
                self.socket = socket.create_connection(("127.0.0.1", port), timeout=1)
                break
            except OSError:
                if self.process.poll() is not None or time.monotonic() > deadline:
                    self.close()
                    raise RuntimeError("ZEsarUX did not open its remote interface")
                time.sleep(0.05)
        self.socket.settimeout(30)
        self.transcript = []
        self.receive()
        # Real firmware must complete reset, initialize IDE and load its SYS
        # modules before an application loaded from BASIC can call it.
        time.sleep(3)
        self.request("enter-cpu-step")

    def receive(self) -> str:
        data = b""
        while not PROMPT.search(data):
            part = self.socket.recv(65536)
            if not part:
                raise RuntimeError("ZEsarUX closed the remote interface")
            data += part
        return data.decode(errors="replace")

    def request(self, command: str) -> str:
        self.socket.sendall((command + "\n").encode())
        result = self.receive()
        self.transcript.append({"command": command, "response": result})
        if "ERROR" in result:
            raise RuntimeError(result)
        return result

    def bytes(self, address: int, count: int) -> bytes:
        result = self.request(f"read-memory {address} {count}")
        payload = result.split("\ncommand", 1)[0].strip()
        values = bytes.fromhex(payload)
        if len(values) != count:
            raise RuntimeError(f"bad memory response: {result}")
        return values

    def key(self, *keys: str):
        rows = ("CS z x c v", "a s d f g", "q w e r t", "1 2 3 4 5",
                "0 9 8 7 6", "p o i u y", "ENTER l k j h",
                "SPACE SS m n b")
        ports = [255] * 8 + [0]
        for key in keys:
            matches = [(row, names.split().index(key))
                       for row, names in enumerate(rows) if key in names.split()]
            if len(matches) != 1:
                raise ValueError(f"unknown Spectrum key {key}")
            row, bit = matches[0]
            ports[row] &= ~(1 << bit)
        self.request("set-ui-io-ports " + bytes(ports).hex())
        self.request("run 50000")
        self.request("set-ui-io-ports ffffffffffffffff00")
        self.request("run 50000")

    def basic_load(self, image: Path, entry: int) -> dict:
        """Type the BASIC disk loader using only the keyboard matrix."""
        if entry != 0x8000:
            raise RuntimeError("BASIC disk loader expects entry at 0x8000")
        lines = []
        for _ in range(20):
            self.request("run 1000000")
            address = int.from_bytes(self.bytes(0x5c59, 2), "little")
            if 0x5b00 <= address < 0x7f80 and self.bytes(address, 1) == b"\r":
                break
        else:
            raise RuntimeError("Sinclair BASIC did not reach an empty input line")

        def finish(expected_tokens: tuple[int, ...]):
            address = int.from_bytes(self.bytes(0x5c59, 2), "little")
            if not 0x5b00 <= address < 0x7f80:
                raise RuntimeError("Sinclair BASIC input pointer is invalid")
            line = self.bytes(address, 96).split(b"\r", 1)[0]
            if not line or line[0] != expected_tokens[0] or not all(
                    token in line for token in expected_tokens):
                raise RuntimeError(f"BASIC keyboard tokens missing: {line.hex()}")
            lines.append(line.hex())
            self.key("ENTER")

        self.key("x")                    # CLEAR
        for key in "32767":
            self.key(key)
        finish((0xfd,))
        self.request("run 1000000")

        self.key("j")                    # LOAD
        self.key("SS", "b")              # current disk: *
        self.key("SS", "p")              # opening quote
        for key in "xccprobe":
            self.key(key)
        self.key("SS", "m")              # period
        for key in "bin":
            self.key(key)
        self.key("SS", "p")
        self.key("CS", "SS")             # extended mode
        self.key("i")                    # CODE
        for key in "32768":
            self.key(key)
        finish((0xef, 0xaf))
        payload = image.read_bytes()
        for _ in range(20):
            self.request("run 1000000")
            if self.bytes(entry, len(payload)) == payload:
                break
        else:
            raise RuntimeError("BASIC LOAD did not reproduce the linked binary")

        self.key("t")                    # RANDOMIZE
        self.key("CS", "SS")
        self.key("l")                    # USR
        for key in "32768":
            self.key(key)
        finish((0xf9, 0xc0))
        return {"commands": ['CLEAR 32767', 'LOAD *"XCCPROBE.BIN" CODE 32768',
                             'RANDOMIZE USR 32768'],
                "typed_basic_hex": lines, "loaded_image_byte_identical": True,
                "method": "Sinclair BASIC keyboard and real esxDOS disk loader"}

    def close(self):
        if hasattr(self, "socket"):
            self.socket.close()
        if hasattr(self, "process") and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        if hasattr(self, "log"):
            self.log.close()


def symbol(path: Path, name: str) -> int:
    for line in path.read_text().splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]{8}) " + re.escape(name), line)
        if match:
            return int(match[1], 16)
    raise RuntimeError(f"missing symbol {name} in {path}")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    root = Path(__file__).resolve().parents[5]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zesarux", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--ide", required=True, type=Path)
    parser.add_argument("--xcc", type=Path, default=root / "bin/x/bin/xcc")
    parser.add_argument("--profile", choices=["-Os", "-Of", "-O0"], default="-Os")
    parser.add_argument("--sdcccall", type=int, choices=[0, 1], default=1)
    parser.add_argument("--source", type=Path,
                        default=Path(__file__).with_name("smoke.c"))
    parser.add_argument("--base-rom", type=Path,
                        help="Sinclair 48K ROM; defaults to ZEsarUX's 48.rom")
    parser.add_argument("--basic-loader", action="store_true",
                        help="load from the private IDE through BASIC keyboard commands")
    parser.add_argument("--work", required=True, type=Path)
    args = parser.parse_args()
    base_rom = (args.base_rom or args.zesarux.resolve().parent / "48.rom").resolve()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=False)
    disk = work / "disk.ide"
    shutil.copyfile(args.ide, disk)
    image, mapfile = work / "smoke.bin", work / "smoke.map"
    command = [str(args.xcc.resolve()), args.profile, "--sdcccall", str(args.sdcccall),
               "--platform=zx-esxdos",
               "--oformat=binary", f"-Map={mapfile}",
               str(args.source.resolve()), "-o", str(image)]
    library = args.xcc.resolve().parents[1] / "z80/lib"
    inputs = [args.xcc, args.zesarux, args.rom, args.ide, base_rom,
              args.source, Path(__file__).resolve()]
    inputs += [args.xcc.resolve().parent / name for name in ("xas", "xld")]
    inputs += [library / name for name in
               ("crt0-zx-esxdos.rel", "crt0-zx-esxdos.s", "libzx-esxdos.a",
                "linker-zx-esxdos.ld", "linker-zx-esxdos.lk",
                "libc.a", "libruntime.a", "libfixed.a")]
    inputs += list((library.parent / "include").rglob("*.h"))
    input_hashes = {str(p.resolve()): sha256(p) for p in inputs}
    with (work / "compile.log").open("w") as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    disk_copy_command = None
    if args.basic_loader:
        with disk.open("rb") as stream:
            mbr = stream.read(512)
        offset = int.from_bytes(mbr[0x1c6:0x1ca], "little") * 512
        if mbr[510:] != b"\x55\xaa" or not offset:
            raise RuntimeError("BASIC loader requires a partitioned FAT IDE image")
        disk_copy_command = ["mcopy", "-o", "-i", f"{disk}@@{offset}",
                             str(image), "::XCCPROBE.BIN"]
        subprocess.run(disk_copy_command, check=True, capture_output=True)
    machine = Zrcp(args.zesarux.resolve(), args.rom.resolve(), disk, work, base_rom)
    report = {"compiler_command": command, "emulator_command": machine.command,
              "inputs": input_hashes, "image_sha256": sha256(image),
              "map_sha256": sha256(mapfile), "sdcccall": args.sdcccall,
              "disk_copy_command": disk_copy_command,
              "firmware_emulation": "real divIDE IDE + esxDOS ROM, no host filesystem handler"}
    try:
        marker = symbol(mapfile, "_zx_disk_result")
        phase = symbol(mapfile, "_zx_disk_phase")
        if args.basic_loader:
            report["basic_loader"] = machine.basic_load(image, symbol(mapfile, "_entry"))
        else:
            machine.request(f"load-binary {image} 32768 0")
            machine.request(f"set-register PC={symbol(mapfile, '_entry'):04X}H")
        for chunk in range(200):
            machine.request("run 1000000")
            result = int.from_bytes(machine.bytes(marker, 2), "little")
            if result:
                if result != 0xa55a:
                    raise RuntimeError(f"public disk probe failed at phase {result}")
                report.update(passed=True, opcodes_budget_used=(chunk + 1) * 1000000)
                report["final_phase"] = int.from_bytes(machine.bytes(phase, 2), "little")
                break
        else:
            raise RuntimeError(f"disk probe timed out at phase {int.from_bytes(machine.bytes(phase, 2), 'little')}")
        print(f"PASS real esxDOS/divIDE public disk probe ({args.profile}, ABI {args.sdcccall})")
    except Exception as error:
        report.update(passed=False, failure=str(error))
        report["registers"] = machine.request("get-registers")
        machine.request(f"save-binary {work / 'failure-memory.bin'} 0 65536")
        raise
    finally:
        report["input_changes"] = [name for name, digest in input_hashes.items()
                                   if not Path(name).is_file() or sha256(Path(name)) != digest]
        report["inputs_unchanged"] = not report["input_changes"]
        if report["input_changes"]:
            report.update(passed=False, failure="validation input changed")
        (work / "transcript.json").write_text(json.dumps(machine.transcript, indent=2) + "\n")
        (work / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        machine.close()
    if not report["inputs_unchanged"]:
        raise RuntimeError("validation input changed")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
