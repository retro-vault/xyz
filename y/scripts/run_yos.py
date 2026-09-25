#!/usr/bin/env python3
"""Prepare private esxDOS media and run YOS in ZEsarUX."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import tempfile
import time


PROMPT = re.compile(rb"\ncommand[^\n]*> $")
NEXT_CONFIG = "\r\n".join((
    "scandoubler=1", "50_60hz=0", "timex=1", "psgmode=1", "intsnd=1",
    "stereomode=0", "turbosound=1", "divmmc=0", "mf=0",
    "joystick1=1", "joystick2=3", "ps2=0", "dma=0", "scanlines=0",
    "turbokey=1", "default=0", "timing=7", "keyb_issue=0",
    "divports=1", "dac=1", "ay48=0", "uart_i2c=1", "kmouse=1",
    "ulaplus=1", "hdmisound=1", "beepmode=0", "buttonswap=0",
    "mousedpi=1", ""))


def executable(name: str, override: Path | None = None) -> str:
    result = str(override.resolve()) if override else shutil.which(name)
    if not result:
        raise RuntimeError(f"required executable not found: {name}")
    return result


def installed_asset(zesarux: Path, name: str) -> Path | None:
    candidates = (
        zesarux.parent.parent / "share/zesarux" / name,
        Path("/usr/local/share/zesarux") / name,
        Path("/usr/share/zesarux") / name,
    )
    return next((path.resolve() for path in candidates if path.is_file()), None)


def bundled_esxdos() -> Path | None:
    """Find esxDOS beside a staged launcher or in the source tree."""
    y_root = Path(__file__).resolve().parent.parent
    candidates = (
        y_root / "firmware/esxdos089",
        y_root / "third_party/esxdos089",
    )
    return next((path.resolve() for path in candidates if path.is_dir()), None)


def run(command: list[str]) -> None:
    subprocess.run(command, check=True)


def prepare_classic(work: Path, firmware: Path, shell: Path,
                    hdfmonkey: str) -> Path:
    hdf = work / "yos.hdf"
    ide = work / "yos.ide"
    run([hdfmonkey, "create", "--fat16", str(hdf), "128M", "YOS"])
    for name in ("SYS", "BIN", "TMP"):
        run([hdfmonkey, "put", str(hdf), str(firmware / name), "/"])
    run([hdfmonkey, "put", str(hdf), str(shell), "/SHELL.SYS"])
    run([hdfmonkey, "clone", str(hdf), str(ide)])
    return ide


def partition_offset(image: Path) -> int:
    sector = image.read_bytes()[:512]
    if len(sector) != 512 or sector[510:512] != b"\x55\xaa":
        raise RuntimeError(f"not a partitioned MMC image: {image}")
    start_sector = int.from_bytes(sector[454:458], "little")
    if not start_sector:
        raise RuntimeError(f"MMC image has no first partition: {image}")
    return start_sector * 512


def mcopy(mcopy_exe: str, image_spec: str, source: Path,
          destination: str, recursive: bool = False) -> None:
    command = [mcopy_exe, "-o"]
    if recursive:
        command.append("-s")
    command += ["-i", image_spec, str(source), destination]
    run(command)


def prepare_next(work: Path, firmware: Path, rom: Path, shell: Path,
                 source_mmc: Path, mcopy_exe: str) -> Path:
    mmc = work / "yos-next.mmc"
    shutil.copyfile(source_mmc, mmc)
    image_spec = f"{mmc}@@{partition_offset(mmc)}"

    menu = work / "menu.def"
    run([mcopy_exe, "-o", "-i", image_spec,
         "::/machines/next/menu.def", str(menu)])
    old_menu = [line for line in menu.read_text(errors="replace").splitlines()
                if not line.startswith("menu=YOS,")]
    menu.write_text("menu=YOS,2,8,YOS.rom,YOSMMC.rom,<none>\n" +
                    "\n".join(old_menu) + "\n")

    config = work / "config.ini"
    config.write_bytes(NEXT_CONFIG.encode())

    personality = work / "YOS.rom"
    kernel = rom.read_bytes()
    if len(kernel) != 0x4000:
        raise RuntimeError(f"YOS ROM must be exactly 16384 bytes: {rom}")
    personality.write_bytes(kernel * 4)

    mcopy(mcopy_exe, image_spec, menu, "::/machines/next/menu.def")
    mcopy(mcopy_exe, image_spec, config, "::/machines/next/config.ini")
    mcopy(mcopy_exe, image_spec, personality, "::/machines/next/YOS.rom")
    mcopy(mcopy_exe, image_spec, firmware / "ESXMMC.BIN",
          "::/machines/next/YOSMMC.rom")
    for name in ("SYS", "BIN", "TMP"):
        mcopy(mcopy_exe, image_spec, firmware / name, "::/", recursive=True)
    mcopy(mcopy_exe, image_spec, shell, "::/SHELL.SYS")
    return mmc


def connect_remote(process: subprocess.Popen[bytes], port: int) -> socket.socket:
    deadline = time.monotonic() + 20
    connection = None
    while time.monotonic() < deadline and process.poll() is None:
        try:
            connection = socket.create_connection(("127.0.0.1", port), 1)
            break
        except OSError:
            time.sleep(0.1)
    if connection is None:
        process.terminate()
        raise RuntimeError("ZEsarUX did not open its remote interface")
    connection.settimeout(10)
    receive_remote(connection)
    return connection


def receive_remote(connection: socket.socket) -> bytes:
    data = b""
    while not PROMPT.search(data):
        part = connection.recv(65536)
        if not part:
            raise RuntimeError("ZEsarUX closed its remote interface")
        data += part
    return data


def request_remote(connection: socket.socket, command: str) -> bytes:
    connection.sendall((command + "\n").encode())
    return receive_remote(connection)


def read_remote(connection: socket.socket, address: int, size: int) -> bytes:
    response = request_remote(connection, f"read-memory {address} {size}")
    payload = response.split(b"\ncommand", 1)[0].strip()
    return bytes.fromhex(payload.decode())


def remote_command(command: list[str], port: int, headless: bool) -> list[str]:
    result = command + ["--enable-remoteprotocol", "--remoteprotocol-port",
                        str(port)]
    if headless:
        result += ["--vo", "null", "--ao", "null"]
    return result


def run_and_check(command: list[str], port: int, model: str,
                  polls: int) -> int:
    process = subprocess.Popen(command)
    try:
        with connect_remote(process, port) as connection:
            for poll in range(1, polls + 1):
                time.sleep(0.5)
                if model == "next" and poll % 4 == 0:
                    request_remote(connection, "send-keys-ascii 100 13")
                pixels = read_remote(connection, 0x4000, 0x1800)
                os_heap = read_remote(connection, 0x5f01, 0x60ff)
                if any(pixels) and b"shell\0" in os_heap:
                    print(f"PASS: YOS {model} rendered its shell after "
                          f"{poll} poll(s)")
                    return 0
        raise RuntimeError(f"YOS {model} did not render its shell")
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def run_visible_next(command: list[str], port: int) -> int:
    process = subprocess.Popen(remote_command(command, port, False))
    try:
        with connect_remote(process, port) as connection:
            for _ in range(3):
                time.sleep(2)
                request_remote(connection, "send-keys-ascii 100 13")
        return process.wait()
    except BaseException:
        if process.poll() is None:
            process.terminate()
        raise


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", choices=("48", "128", "next"))
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--shell", required=True, type=Path)
    parser.add_argument(
        "--esxdos", type=Path,
        help="override bundled esxDOS directory (or set ESXDOS)")
    parser.add_argument("--zesarux", type=Path)
    parser.add_argument("--rom128", type=Path,
                        help="stock 32 KiB 128K ROM (auto-detected)")
    parser.add_argument("--next-mmc", type=Path,
                        help="stock tbblue.mmc image (auto-detected)")
    parser.add_argument("--work-dir", type=Path,
                        help="parent for private run media")
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--headless", action="store_true",
                        help="run without video and require rendered pixels")
    parser.add_argument("--polls", type=int, default=120,
                        help="headless timeout in half-second polls")
    args = parser.parse_args()

    firmware_option = args.esxdos
    if firmware_option is None and os.environ.get("ESXDOS"):
        firmware_option = Path(os.environ["ESXDOS"])
    firmware = (firmware_option.resolve() if firmware_option else
                bundled_esxdos())
    if firmware is None:
        parser.error("bundled esxDOS runtime is missing; rebuild the release "
                     "or pass --esxdos")
    needed = ["ESXIDE.BIN", "ESXMMC.BIN", "SYS", "BIN", "TMP"]
    missing = [name for name in needed if not (firmware / name).exists()]
    if missing:
        parser.error("missing esxDOS entries: " + ", ".join(missing))
    for path in (args.rom, args.shell):
        if not path.is_file():
            parser.error(f"missing YOS release file: {path}")

    try:
        zesarux_name = executable("zesarux", args.zesarux)
        zesarux = Path(zesarux_name)
        parent = (args.work_dir.resolve() if args.work_dir else
                  Path(tempfile.gettempdir()) / "yos-zesarux")
        parent.mkdir(parents=True, exist_ok=True)
        work = Path(tempfile.mkdtemp(prefix=f"{args.model}-", dir=parent))

        if args.model in ("48", "128"):
            hdfmonkey = executable("hdfmonkey")
            disk = prepare_classic(work, firmware, args.shell.resolve(),
                                   hdfmonkey)
            machine_rom = args.rom.resolve()
            if args.model == "128":
                rom128 = args.rom128.resolve() if args.rom128 else \
                    installed_asset(zesarux, "128.rom")
                if not rom128:
                    parser.error("128.rom was not found; pass --rom128")
                stock = rom128.read_bytes()
                if len(stock) != 0x8000:
                    parser.error(f"128K ROM must be 32768 bytes: {rom128}")
                machine_rom = work / "yos-128.rom"
                machine_rom.write_bytes(stock[:0x4000] + args.rom.read_bytes())
            command = [zesarux_name, "--noconfigfile", "--machine",
                       "48k" if args.model == "48" else "128k",
                       "--romfile", str(machine_rom), "--ide-file", str(disk),
                       "--enable-ide", "--enable-divide", "--divide-rom",
                       str(firmware / "ESXIDE.BIN"),
                       "--ide-no-persistent-writes", "--nowelcomemessage",
                       "--no-saveconf-on-exit"]
        else:
            mcopy_exe = executable("mcopy")
            next_mmc = args.next_mmc.resolve() if args.next_mmc else \
                installed_asset(zesarux, "tbblue.mmc")
            if not next_mmc:
                parser.error("tbblue.mmc was not found; pass --next-mmc")
            disk = prepare_next(
                work, firmware, args.rom.resolve(), args.shell.resolve(),
                next_mmc, mcopy_exe)
            command = [zesarux_name, "--noconfigfile", "--machine", "TBBlue",
                       "--mmc-file", str(disk), "--enable-mmc",
                       "--enable-divmmc-ports", "--nowelcomemessage",
                       "--no-saveconf-on-exit"]

        print(f"YOS {args.model} run directory: {work}")
        print("Launch command:", " ".join(command), flush=True)
        if args.prepare_only:
            return 0
        if args.headless:
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]
            return run_and_check(remote_command(command, port, True), port,
                                 args.model, args.polls)
        if args.model == "next":
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]
            return run_visible_next(command, port)
        os.execv(command[0], command)
    except RuntimeError as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
