#!/usr/bin/env python3
"""Compile and cold boot a zx-esxdos-rom program with real esxDOS/divIDE.

The replacement 16-KiB ROM is the machine's only base ROM. No snapshot,
register changes, CPU reset, RAM injection, or host filesystem handler is
used. Every run receives a private copy of the supplied IDE image. Code and
constants must execute in ROM; only writable data and 48 bytes of disk-call
gates may be copied into RAM.
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


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def symbols(path: Path) -> dict[str, int]:
    result = {}
    for line in path.read_text().splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]{8}) (\S+)(?:\s+;.*)?", line)
        if match:
            result[match[2]] = int(match[1], 16)
    return result


def area_layout(path: Path) -> list[dict]:
    result = []
    for line in path.read_text().splitlines():
        match = re.fullmatch(r"\s+(\S+)\s+([0-9a-fA-F]{4,8})\s+([0-9a-fA-F]{4,8})\s+.*", line)
        if match and int(match[3], 16):
            result.append({"name": match[1], "address": int(match[2], 16),
                           "bytes": int(match[3], 16)})
    return result


def check_xip_layout(syms: dict[str, int], areas: list[dict], image: bytes) -> dict:
    writable = {"_DATA", ".data", "_INITIALIZED", "_BSS", ".bss", "_HEAP"}
    ram_start, stack_start = syms["s__DATA"], syms["s__STACK"]
    if not 0x5b00 <= ram_start < stack_start <= 0xffff:
        raise RuntimeError("invalid application RAM or stack boundary")
    for area in areas:
        start, end = area["address"], area["address"] + area["bytes"]
        if area["name"] in writable:
            if start < ram_start or end > stack_start:
                raise RuntimeError(f"writable area is outside program RAM: {area}")
        elif end > 0x4000:
            raise RuntimeError(f"application code or constants were placed in RAM: {area}")
    for name in ("_entry", "_main", "__exit"):
        if not 0 <= syms[name] < 0x4000:
            raise RuntimeError(f"{name} must execute in ROM")
    start = syms["__zx_esx_gates_start"]
    end = syms["__zx_esx_gate_b0"] + 3
    if end - start != 48:
        raise RuntimeError("expected exactly 48 bytes of RAM syscall gates")
    data_start, data_length = syms["s__DATA"], syms["l__DATA"]
    if not data_start <= start < end <= data_start + data_length:
        raise RuntimeError("RAM syscall gates must reside in initialized _DATA")
    load = syms["s__DATA_LOAD"] + start - data_start
    expected = image[load:load + 48]
    services = (0x88, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
                0xa1, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xb0)
    actual_services = []
    for offset in range(0, 48, 3):
        if expected[offset] != 0xcf or expected[offset + 2] != 0xc9:
            raise RuntimeError("RAM gate must contain RST08, service byte, RET")
        actual_services.append(expected[offset + 1])
        if syms[f"__zx_esx_gate_{expected[offset + 1]:02x}"] != start + offset:
            raise RuntimeError("RAM gate symbol does not match its service bytes")
    if sorted(actual_services) != list(services):
        raise RuntimeError("unexpected RAM syscall gate services")
    return {"ram_start": ram_start, "stack_start": stack_start,
            "main_address": syms["_main"], "entry_address": syms["_entry"],
            "exit_address": syms["__exit"], "gate_ram_address": start,
            "gate_rom_address": load, "gate_bytes": 48,
            "gate_expected_hex": expected.hex(), "areas": areas}


class ColdZrcp:
    def __init__(self, executable: Path, base_rom: Path, firmware: Path,
                 disk: Path, work: Path):
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        self.transcript = []
        self.log = (work / "emulator.log").open("w")
        self.command = [str(executable), "--noconfigfile", "--machine", "48k",
                        "--romfile", str(base_rom), "--vo", "null", "--ao", "null",
                        "--nowelcomemessage", "--ide-file", str(disk),
                        "--enable-ide", "--enable-divide", "--divide-rom", str(firmware),
                        "--enable-remoteprotocol", "--remoteprotocol-port", str(port)]
        try:
            self.process = subprocess.Popen(self.command, cwd=executable.parent,
                                            stdout=self.log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 15
            while True:
                try:
                    self.socket = socket.create_connection(("127.0.0.1", port), timeout=1)
                    break
                except OSError:
                    if self.process.poll() is not None or time.monotonic() > deadline:
                        raise RuntimeError("ZEsarUX did not open its remote interface")
                    time.sleep(0.05)
            self.socket.settimeout(30)
            self.receive()
            self.request("enter-cpu-step")
        except Exception:
            self.close()
            raise

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
        if "ERROR" in result or result.startswith("Error"):
            raise RuntimeError(result)
        return result

    def bytes(self, address: int, count: int) -> bytes:
        result = self.request(f"read-memory {address} {count}")
        value = bytes.fromhex(result.split("\ncommand", 1)[0].strip())
        if len(value) != count:
            raise RuntimeError(f"bad memory response: {result}")
        return value

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


def input_files(args, root: Path) -> list[Path]:
    prefix = args.xcc.resolve().parents[1]
    inputs = [Path(__file__).resolve(), args.source.resolve(), args.rom.resolve(),
              args.ide.resolve(), args.zesarux.resolve()]
    inputs += [path.resolve() for path in args.asm]
    inputs += [prefix / "bin" / name for name in ("xcc", "xas", "xld")]
    inputs += list((prefix / "z80/lib").glob("*zx-esxdos-rom*"))
    inputs += [prefix / "z80/lib" / name for name in ("libc.a", "libruntime.a", "libfixed.a")]
    inputs += list((prefix / "z80/include").rglob("*.h"))
    # Include source platform inputs as well as the staged objects actually used.
    inputs += [p for p in (root / "x/platforms/zx-esxdos-rom").rglob("*")
               if p.is_file() and p.suffix in (".s", ".ld", ".lk", ".h")]
    return sorted(set(inputs))


def main() -> int:
    here = Path(__file__).resolve().parent
    root = here.parents[4]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--zesarux", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path, help="esxDOS ESXIDE.BIN firmware")
    parser.add_argument("--ide", required=True, type=Path)
    parser.add_argument("--xcc", type=Path, default=root / "bin/x/bin/xcc")
    parser.add_argument("--profile", choices=["-O0", "-Os", "-Of"], default="-Os")
    parser.add_argument("--abi", choices=[0, 1], type=int, default=1)
    parser.add_argument("--mode", choices=["sdcc", "gnu"], default="sdcc")
    parser.add_argument("--source", type=Path, default=here / "smoke.c")
    parser.add_argument("--asm", type=Path, action="append", default=[], help="additional assembly input; repeatable")
    parser.add_argument("--work", required=True, type=Path)
    parser.add_argument("--chunks", type=int, default=200, help="maximum one-million-opcode chunks")
    args = parser.parse_args()
    if args.chunks < 1:
        parser.error("--chunks must be positive")
    if args.asm and args.mode == "gnu":
        parser.error("--asm fixtures use SDCC assembly; run them with --mode=sdcc")
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=False)
    inputs = {str(p): sha256(p) for p in input_files(args, root)}
    report = {"passed": False, "profile": args.profile, "abi": args.abi, "mode": args.mode,
              "source": str(args.source.resolve()), "inputs": inputs,
              "boot": "fresh 48K machine with replacement base ROM and real esxDOS/divIDE; no Sinclair ROM, CPU reset, register writes, RAM injection, snapshots, or host filesystem handler"}
    (work / "inputs-before.json").write_text(json.dumps(inputs, indent=2) + "\n")
    machine = None
    try:
        disk = work / "disk.ide"
        shutil.copyfile(args.ide, disk)
        image, mapfile = work / "program.rom", work / "program.map"
        command = [str(args.xcc.resolve()), args.profile, "--sdcccall", str(args.abi),
                   "--mode=" + args.mode,
                   "--platform=zx-esxdos-rom", "--oformat=binary", f"-Map={mapfile}"]
        if args.mode == "gnu":
            library = args.xcc.resolve().parents[1] / "z80/lib"
            command += ["-nostartfiles", "-T", str(library / "linker-zx-esxdos-rom.ld"),
                        str(library / "crt0-zx-esxdos-rom.rel"), "-L" + str(library),
                        "-lzx-esxdos-rom", "-lc", "-lruntime"]
        command += [str(args.source.resolve()), *[str(p.resolve()) for p in args.asm],
                    "-o", str(image)]
        report["compiler_command"] = command
        with (work / "compile.log").open("w") as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        if image.stat().st_size != 16384:
            raise RuntimeError("replacement ROM must be exactly 16384 bytes")
        report["rom_sha256"] = sha256(image)
        report["rom_bytes"] = image.stat().st_size
        report["map_sha256"] = sha256(mapfile)
        syms = symbols(mapfile)
        exit_address, status_address = syms["__exit"], syms["_zx_exit_status"]
        marker, phase = syms.get("_zx_disk_result"), syms.get("_zx_disk_phase")
        report["symbols"] = syms
        report["xip"] = check_xip_layout(syms, area_layout(mapfile), image.read_bytes())
        report["relocated_areas"] = []
        for name, value in syms.items():
            if not name.startswith("s_") or not name.endswith("_LOAD"):
                continue
            length = syms.get("l" + name[1:-5], 0)
            if not length or name[:-5] not in syms:
                continue
            area = name[2:-5]
            if area.startswith("_."):
                area = area[1:]
            report["relocated_areas"].append({"name": area, "rom_address": value,
                                              "ram_address": syms[name[:-5]], "bytes": length})
        if any(a["name"] not in ("_DATA", ".data") for a in report["relocated_areas"]):
            raise RuntimeError("ROM startup must copy only writable data and RAM gates")
        machine = ColdZrcp(args.zesarux.resolve(), image, args.rom.resolve(), disk, work)
        report["emulator_command"] = machine.command
        report["attach_registers"] = machine.request("get-registers")
        report["memory_zones"] = machine.request("get-memory-zones")
        machine.request("set-memory-zone 1")
        machine.request(f"save-binary {work / 'physical-base-rom.bin'} 0 16384")
        machine.request("set-memory-zone -1")
        report["physical_base_rom_sha256"] = sha256(work / "physical-base-rom.bin")
        if report["physical_base_rom_sha256"] != report["rom_sha256"]:
            raise RuntimeError("physical base ROM differs from compiled replacement image")
        report["initial_memory_pages"] = machine.request("get-memory-pages verbose")
        for setting in ("enabled yes", "set-max-size 65536", "ignrephalt yes",
                        "ignrepldxr yes", "started yes"):
            machine.request("cpu-history " + setting)
        report["observations"] = []
        for chunk in range(args.chunks):
            machine.request("run 1000000")
            registers = machine.request("get-registers")
            match = re.search(r"\bPC=([0-9a-fA-F]{4})\b", registers)
            if not match:
                raise RuntimeError("missing PC in register response")
            pc = int(match[1], 16)
            item = {"chunk": chunk + 1, "registers": registers,
                    "exit_status": int.from_bytes(machine.bytes(status_address, 2), "little")}
            if marker is not None:
                item["result"] = int.from_bytes(machine.bytes(marker, 2), "little")
            if phase is not None:
                item["phase"] = int.from_bytes(machine.bytes(phase, 2), "little")
            report["observations"].append(item)
            # Locate the platform exit's HALT/JR loop in ROM.
            # Requiring both this PC and status zero avoids an uninitialized
            # zero result being mistaken for successful termination.
            exit_code = machine.bytes(exit_address, 64)
            halt_offset = exit_code.find(bytes.fromhex("76 18 fd"))
            at_halt = halt_offset >= 0 and pc in (exit_address + halt_offset,
                                                  exit_address + halt_offset + 1)
            if at_halt:
                report["halt_address"] = exit_address + halt_offset
                if pc >= 0x4000:
                    raise RuntimeError("program termination must execute in ROM")
                if item["exit_status"] != 0:
                    raise RuntimeError(f"program terminated with status {item['exit_status']}")
                if marker is not None and item["result"] != 0xa55a:
                    raise RuntimeError(f"program halted without success marker: {item['result']}")
                report.update(passed=True, opcodes_budget_used=(chunk + 1) * 1000000)
                break
        else:
            raise RuntimeError(f"program did not finish before the opcode budget: {report['observations'][-1]}")
    except Exception as error:
        report.update(passed=False, failure=str(error))
    finally:
        if machine is not None:
            try:
                report["final_registers"] = machine.request("get-registers")
                report["final_memory_pages"] = machine.request("get-memory-pages verbose")
                report["final_disassembly"] = machine.request("disassemble")
                report["history_pc"] = machine.request("cpu-history get-pc 0 4096")
                report["history_tail"] = [machine.request(f"cpu-history get-extended {i}") for i in range(32)]
                machine.request(f"save-binary {work / 'final-memory.bin'} 0 65536")
                machine.request(f"save-screen {work / 'screen.scr'}")
                memory = (work / "final-memory.bin").read_bytes()
                xip = report["xip"]
                xip["mapped_rom_unchanged"] = memory[:0x4000] == image.read_bytes()
                start = xip["gate_ram_address"]
                xip["gate_actual_hex"] = memory[start:start + 48].hex()
                xip["ram_gates_unchanged"] = xip["gate_actual_hex"] == xip["gate_expected_hex"]
                if not xip["mapped_rom_unchanged"] or not xip["ram_gates_unchanged"]:
                    raise RuntimeError("final mapped ROM or RAM syscall gates differ from compiled bytes")
            except Exception as error:
                report["diagnostic_failure"] = str(error)
                report["passed"] = False
            (work / "transcript.json").write_text(json.dumps(machine.transcript, indent=2) + "\n")
            machine.close()
        changed = [p for p, digest in inputs.items()
                   if not Path(p).is_file() or sha256(Path(p)) != digest]
        report["input_changes"] = changed
        report["inputs_unchanged"] = not changed
        report["source_disk_unchanged"] = sha256(args.ide.resolve()) == inputs[str(args.ide.resolve())]
        if changed:
            report.update(passed=False, failure="test inputs changed during execution")
        if (work / "disk.ide").is_file():
            report["result_disk_sha256"] = sha256(work / "disk.ide")
        (work / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": report["passed"], "profile": args.profile, "abi": args.abi,
                      "mode": args.mode,
                      "source": str(args.source), "failure": report.get("failure"),
                      "inputs_unchanged": report["inputs_unchanged"]}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
