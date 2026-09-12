#!/usr/bin/env python3
"""Build YOS and launch a visible, cold-booted Fuse with real esxDOS."""
# MIT License (see: LICENSE)
# Copyright (C) 2026 tomaz stih

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esxdos", required=True, type=Path,
                        help="extracted esxDOS 0.8.9 distribution directory")
    parser.add_argument("--prepare-only", action="store_true",
                        help="build media and print its directory without launching")
    args = parser.parse_args()
    firmware = args.esxdos.resolve()
    for name in ("ESXIDE.BIN", "SYS", "BIN", "TMP"):
        if not (firmware / name).exists():
            parser.error(f"missing esxDOS distribution entry: {firmware / name}")
    for command in ("make", "cc", "hdfmonkey", "fuse"):
        if not shutil.which(command):
            parser.error(f"required executable not found: {command}")

    # Each run owns new media: never overwrite a disk mounted by another Fuse.
    parent = root / "build/yos-fuse"
    parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
    with (work / "build.log").open("w") as log:
        subprocess.run(["make", "-C", str(root / "y/src/z80")], check=True,
                       stdout=log, stderr=subprocess.STDOUT)
    snapshot_tool = work / "cold_snapshot"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    str(Path(__file__).with_name("cold_snapshot.c")),
                    "-lspectrum", "-o", str(snapshot_tool)], check=True)
    output = root / "bin/y/z80/spectrum/bin"
    rom = output / "yos-kernel.rom"
    snapshot = work / "yos.szx"
    subprocess.run([str(snapshot_tool), str(firmware / "ESXIDE.BIN"),
                    str(rom), str(snapshot)], check=True)
    disk = work / "yos.hdf"
    subprocess.run(["hdfmonkey", "create", "--fat16", str(disk),
                    "128M", "YOS"], check=True)
    for name in ("SYS", "BIN", "TMP"):
        subprocess.run(["hdfmonkey", "put", str(disk),
                        str(firmware / name), "/"], check=True)
    for name in ("shell.sys", "shelllib.svc"):
        subprocess.run(["hdfmonkey", "put", str(disk), str(output / name),
                        "/" + name.upper()], check=True)
    print(f"Fuse cold-boot media: {work}", flush=True)
    if args.prepare_only:
        return

    # Snap-packaged IDEs can export incompatible GTK library/cache settings.
    env = os.environ.copy()
    if "SNAP" in env:
        for name in list(env):
            if name.startswith(("SNAP", "GTK_", "GDK_PIXBUF_")) or name in (
                    "XDG_DATA_HOME", "XDG_DATA_DIRS"):
                del env[name]
    if env.get("DISPLAY"):
        env["GDK_BACKEND"] = "x11"
    command = [shutil.which("fuse"), "--machine", "48", "--rom-48", str(rom),
               "--divide", "--divide-write-protect", "--divide-masterfile",
               str(disk), "--snapshot", str(snapshot), "--no-sound",
               "--no-autosave-settings", "--no-confirm-actions",
               "--graphics-filter", "3x"]
    os.execve(command[0], command, env)


if __name__ == "__main__":
    main()
