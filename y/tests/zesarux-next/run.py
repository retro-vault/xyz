#!/usr/bin/env python3
"""Verify the YOS Spectrum Next banking backend in ZEsarUX."""
# MIT License (see: LICENSE)
# Copyright (C) 2026 tomaz stih

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import tempfile
import time


PROMPT = re.compile(rb"\ncommand[^\n]*> $")


def symbols(path: Path) -> dict[str, int]:
    result = {}
    for line in path.read_text().splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]{8}) (\S+)(?:\s+;.*)?", line)
        if match:
            result[match[2]] = int(match[1], 16)
    return result


class Zrcp:
    def __init__(self, command: list[str], work: Path, port: int,
                 pause: bool = True, log_name: str = "zesarux.log"):
        self.transcript = []
        self.log = (work / log_name).open("w")
        self.process = subprocess.Popen(command, cwd=work, stdout=self.log,
                                        stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 15
            while True:
                try:
                    self.socket = socket.create_connection(
                        ("127.0.0.1", port), timeout=1)
                    break
                except OSError:
                    if (self.process.poll() is not None or
                            time.monotonic() > deadline):
                        raise RuntimeError(
                            "ZEsarUX did not open its remote interface")
                    time.sleep(0.05)
            self.socket.settimeout(30)
            self.receive()
            if pause:
                self.request("enter-cpu-step")
        except Exception:
            self.close()
            raise

    def receive(self) -> str:
        data = b""
        while not PROMPT.search(data):
            part = self.socket.recv(65536)
            if not part:
                raise RuntimeError("ZEsarUX closed its remote interface")
            data += part
        return data.decode(errors="replace")

    def request(self, command: str) -> str:
        self.socket.sendall((command + "\n").encode())
        response = self.receive()
        self.transcript.append({"command": command, "response": response})
        if "ERROR" in response or response.startswith("Error"):
            raise RuntimeError(response)
        return response

    def bytes(self, address: int, count: int) -> bytes:
        response = self.request(f"read-memory {address} {count}")
        value = bytes.fromhex(response.split("\ncommand", 1)[0].strip())
        if len(value) != count:
            raise RuntimeError(f"bad memory response: {response}")
        return value

    def word(self, address: int) -> int:
        return int.from_bytes(self.bytes(address, 2), "little")

    def write(self, address: int, value: bytes):
        self.request(f"write-memory-raw {address} {value.hex()}")

    def set_register(self, register: str, value: int):
        self.request(f"set-register {register}={value}")

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
        self.log.close()


def run(command: list[str], **kwargs):
    subprocess.run(command, check=True, **kwargs)


def main() -> int:
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esxdos", type=Path,
                        help="override the vendored esxDOS 0.8.9 runtime")
    parser.add_argument("--zesarux", type=Path,
                        help="ZEsarUX executable; defaults to PATH")
    parser.add_argument("--rom128", type=Path,
                        help="stock 32 KiB Spectrum 128 ROM; defaults to "
                             "ZEsarUX's installed 128.rom")
    parser.add_argument("--prepare-only", action="store_true",
                        help="build private media but do not start ZEsarUX")
    parser.add_argument("--headless", action="store_true",
                        help="run the automated banking and cold-boot checks")
    parser.add_argument("--polls", type=int, default=120,
                        help="cold-boot limit in 500 ms polling intervals")
    args = parser.parse_args()
    if args.polls < 1:
        parser.error("--polls must be positive")

    firmware = (args.esxdos or
                root / "y/third_party/esxdos089").resolve()
    for name in ("ESXIDE.BIN", "SYS", "BIN", "TMP"):
        if not (firmware / name).exists():
            parser.error(f"missing esxDOS distribution entry: {firmware / name}")
    zesarux_name = str(args.zesarux) if args.zesarux else shutil.which("zesarux")
    if not zesarux_name:
        parser.error("ZEsarUX was not found; pass --zesarux")
    zesarux = Path(zesarux_name).resolve()
    rom_128_source = args.rom128
    if not rom_128_source:
        candidates = (
            zesarux.parent.parent / "share/zesarux/128.rom",
            Path("/usr/local/share/zesarux/128.rom"),
            Path("/usr/share/zesarux/128.rom"),
        )
        rom_128_source = next((path for path in candidates if path.is_file()),
                              None)
    if not rom_128_source or not rom_128_source.is_file():
        parser.error("Spectrum 128 ROM was not found; pass --rom128")
    rom_128_source = rom_128_source.resolve()
    hdfmonkey = shutil.which("hdfmonkey")
    if not hdfmonkey:
        parser.error("hdfmonkey was not found")

    parent = root / "build/yos-zesarux-next"
    parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
    with (work / "build.log").open("w") as log:
        target = "test" if args.headless else "all"
        command = ["make", "-C", str(root / "y/src/z80"), target]
        if args.headless:
            command.append("YOS_BANK_BACKEND=next")
        run(command, stdout=log, stderr=subprocess.STDOUT)

    output = root / "bin/y/arch/48"
    rom = output / "yos-kernel.rom"
    rom_128 = work / "yos-kernel-128.rom"
    stock_128 = rom_128_source.read_bytes()
    if len(stock_128) != 0x8000:
        parser.error(f"Spectrum 128 ROM must be 32768 bytes: {rom_128_source}")
    # ESXIDE boots the 48 BASIC ROM slot on 128K hardware. Preserve the stock
    # editor ROM in slot 0 and install YOS in that slot 1 position.
    rom_128.write_bytes(stock_128[:0x4000] + rom.read_bytes())
    mapfile = root / "build/yos-z80/yos-kernel.map"
    disk_hdf = work / "yos.hdf"
    disk = work / "yos.ide"
    run([hdfmonkey, "create", "--fat16", str(disk_hdf), "128M", "YOS"])
    for name in ("SYS", "BIN", "TMP"):
        run([hdfmonkey, "put", str(disk_hdf), str(firmware / name), "/"])
    for source, destination in (("shell.sys", "/SHELL.SYS"),):
        run([hdfmonkey, "put", str(disk_hdf), str(output / source),
             destination])
    # Retain the Fuse-style HDF and clone the raw form used by these ZEsarUX
    # IDE-card runs; hdfmonkey selects the latter from the .ide suffix.
    run([hdfmonkey, "clone", str(disk_hdf), str(disk)])

    syms = symbols(mapfile)
    required = ("__bank_model", "__bank_count", "__bank_map",
                "__bank_map_next", "__bank_current", "__bank_call_rst20",
                "__sys_stack", "_boot_shell", "_process_first",
                "_process_last_error", "__errno_value")
    missing = [name for name in required if name not in syms]
    if missing:
        raise RuntimeError("missing map symbols: " + ", ".join(missing))

    core_command = [str(zesarux), "--noconfigfile", "--machine", "TBBlue",
                    "--tbblue-fast-boot-mode", "--tbblue-machine-id", "10",
                    "--romfile", str(rom)]
    base_command = core_command + ["--ide-file", str(disk),
                    "--enable-ide", "--enable-divide", "--divide-rom",
                    str(firmware / "ESXIDE.BIN"),
                    "--ide-no-persistent-writes", "--nowelcomemessage",
                    "--no-saveconf-on-exit", "--ao", "null"]
    print(f"ZEsarUX Next media: {work}", flush=True)
    if args.prepare_only:
        print("Launch command:", " ".join(base_command), flush=True)
        return 0
    if not args.headless:
        os.execv(base_command[0], base_command)

    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    command = base_command + [
        "--vo", "null", "--enable-remoteprotocol",
        "--remoteprotocol-port", str(port), "--enable-breakpoints",
        "--set-breakpoint", "1", f"PC={syms['_boot_shell']:04X}H",
        "--set-breakpointaction", "1", "break"]

    machine = None
    observation = {}
    try:
        machine = Zrcp(command, work, port)
        model = machine.bytes(syms["__bank_model"], 1)[0]
        count = machine.bytes(syms["__bank_count"], 1)[0]
        trampoline = machine.bytes(syms["__bank_map"], 3)

        # Run a fixed-RAM probe before _boot_shell performs any disk I/O. It
        # puts executable bytes in the highest logical bank, restores bank 0,
        # then enters the ordinary far-call gate. A HALT at the same virtual
        # address in bank 0 makes an unmapped call unable to pass by accident.
        probe = 0x4000
        marker = 0x4200
        target = 0xc100
        highest_bank = 125
        stack = syms["__sys_stack"] - 64
        mapper = syms["__bank_map"]
        far_gate = syms["__bank_call_rst20"]
        current = syms["__bank_current"]
        lo = lambda value: value & 0xff
        hi = lambda value: value >> 8
        code = bytes((
            0xf3,                         # DI
            0x31, lo(stack), hi(stack),   # LD SP,stack
            0x3e, 0x00, 0xcd, lo(mapper), hi(mapper),
            0x21, lo(target), hi(target), 0x36, 0x76,
            0x3e, highest_bank, 0xcd, lo(mapper), hi(mapper),
            0x21, lo(target), hi(target),
            0x36, 0x11, 0x23, 0x36, 0x0d,
            0x23, 0x36, 0x60, 0x23, 0x36, 0xc9,
            0x3e, 0x00, 0xcd, lo(mapper), hi(mapper),
            0xcd, lo(far_gate), hi(far_gate),
            highest_bank, lo(target), hi(target),
            0xed, 0x53, lo(marker), hi(marker),
            0x3a, lo(current), hi(current),
            0x32, lo(marker + 2), hi(marker + 2),
            0x76,
        ))
        halt = probe + len(code) - 1
        machine.write(probe, code)
        machine.request(f"set-breakpoint 2 PC={halt:04X}H")
        machine.request("set-breakpointaction 2 break")
        machine.set_register("SP", stack)
        machine.set_register("PC", probe)
        machine.request("run 100000")

        result = machine.bytes(marker, 3)
        bank_zero_target = machine.bytes(target, 1)[0]
        observation = {
            "model": model,
            "bank_count": count,
            "trampoline": trampoline.hex(),
            "highest_far_call_bank": highest_bank,
            "far_call_de": int.from_bytes(result[:2], "little"),
            "restored_bank": result[2],
            "bank_zero_target": bank_zero_target,
            "registers": machine.request("get-registers").split(
                "\ncommand", 1)[0].strip(),
            "paging": machine.request("get-paging-state").split(
                "\ncommand", 1)[0].strip(),
        }
        passed = (model == 2 and count == 126 and
                  trampoline[0] == 0xc3 and
                  int.from_bytes(trampoline[1:], "little") ==
                  syms["__bank_map_next"] and
                  int.from_bytes(result[:2], "little") == 0x600d and
                  result[2] == 0 and bank_zero_target == 0x76)
        if not passed:
            raise RuntimeError("YOS Next hardware probe failed: " +
                               json.dumps(observation, sort_keys=True))
        (work / "zrcp-banking.json").write_text(
            json.dumps(machine.transcript, indent=2) + "\n")
        machine.close()
        machine = None

        # TBBlue fast-boot mode explicitly excludes native divMMC. Validate
        # the universal ROM's real firmware/storage path separately on both
        # classic machines, where the replacement ROM and divIDE coexist.
        def cold_boot(machine_name: str, expected_model: int,
                      expected_banks: int):
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1", 0))
                boot_port = reservation.getsockname()[1]
            machine_rom = rom_128 if machine_name == "128k" else rom
            boot_command = [
                str(zesarux), "--noconfigfile", "--machine", machine_name,
                "--romfile", str(machine_rom), "--ide-file", str(disk),
                "--enable-ide", "--enable-divide", "--divide-rom",
                str(firmware / "ESXIDE.BIN"),
                "--ide-no-persistent-writes", "--nowelcomemessage",
                "--no-saveconf-on-exit", "--ao", "null", "--vo", "null",
                "--enable-remoteprotocol", "--remoteprotocol-port",
                str(boot_port)]
            guest = Zrcp(boot_command, work, boot_port, pause=False,
                         log_name=f"zesarux-{machine_name}-divide.log")
            boot = {}
            payload_seen = False
            try:
                for poll in range(1, args.polls + 1):
                    time.sleep(0.5)
                    process = guest.word(syms["_process_first"])
                    load_error = guest.bytes(
                        syms["_process_last_error"], 1)[0]
                    name = b""
                    process_seen = 0
                    visited = set()
                    node = process
                    # Processes and libraries share this intrusive list. A
                    # process has flags bit 1 clear; a library has it set.
                    while (0x5f01 <= node < 0xc000 and node not in visited and
                           len(visited) < 16):
                        visited.add(node)
                        record = guest.bytes(node, 16)
                        if not record[4] & 0x02:
                            process_seen = node
                            name = record[5:13].split(b"\0", 1)[0]
                            if name == b"shell":
                                break
                        node = int.from_bytes(record[:2], "little")
                    pixels = False
                    if name == b"shell":
                        pixels = any(guest.bytes(0x4000, 0x1800))
                    boot = {
                        "poll": poll,
                        "list_head": process,
                        "process": process_seen,
                        "process_name": name.decode(errors="replace"),
                        "display_pixels": pixels,
                        "process_load_error": load_error,
                    }
                    if name == b"shell" and pixels:
                        payload_seen = True
                        break

                boot["model"] = guest.bytes(syms["__bank_model"], 1)[0]
                boot["bank_count"] = guest.bytes(
                    syms["__bank_count"], 1)[0]
                boot["errno"] = guest.word(syms["__errno_value"])
                boot["registers"] = guest.request("get-registers").split(
                    "\ncommand", 1)[0].strip()
                passed = (payload_seen and boot["model"] == expected_model and
                          boot["bank_count"] == expected_banks)
                return boot_command, boot, passed
            finally:
                (work / f"zrcp-{machine_name}-divide.json").write_text(
                    json.dumps(guest.transcript, indent=2) + "\n")
                guest.close()

        boot_48_command, boot_48, passed_48 = cold_boot("48k", 0, 1)
        boot_128_command, boot_128, passed_128 = cold_boot("128k", 1, 6)
        boot_passed = passed_48 and passed_128

        report = {
            "passed": boot_passed,
            "media": {"hdf": str(disk_hdf), "raw_ide": str(disk)},
            "banking_command": command,
            "banking": observation,
            "divide_48": {
                "command": boot_48_command,
                "passed": passed_48,
                "observation": boot_48,
            },
            "divide_128": {
                "command": boot_128_command,
                "passed": passed_128,
                "observation": boot_128,
            },
        }
        (work / "result.json").write_text(
            json.dumps(report, indent=2) + "\n")
        if not boot_passed:
            raise RuntimeError(
                "YOS real-divIDE boot did not complete: " +
                json.dumps({"48k": boot_48, "128k": boot_128},
                           sort_keys=True))
        print("PASS: libxz80 exercised all 126 Next user heaps; ZEsarUX "
              "TBBlue executed a bank-0 to bank-125 RST20 call; real 48K "
              "and 128K divIDE instances loaded shell.sys "
              "and rendered the shell; the 128K instance exposed six banks")
        return 0
    finally:
        if machine:
            (work / "zrcp-cold-boot.json").write_text(
                json.dumps(machine.transcript, indent=2) + "\n")
            machine.close()


if __name__ == "__main__":
    raise SystemExit(main())
