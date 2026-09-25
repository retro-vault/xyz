#!/usr/bin/env python3
"""Check that bin/y is a complete, flat, deployable YOS release."""

from pathlib import Path
import stat
import sys


release = Path(sys.argv[1]).resolve()
release_root = release
source_root = Path(__file__).resolve().parents[2]
firmware_source = source_root / "third_party/esxdos089"
required = {
    "README.md",
    "include/yos.h",
    "include/yos.inc",
    "run-48.sh",
    "run-128.sh",
    "run-next.sh",
    "scripts/run-yos.py",
    "samples/c/Makefile",
    "samples/c/shell.c",
    "samples/asm/Makefile",
    "samples/asm/linker.lk",
    "samples/asm/shell.s",
}

binary_names = (
    "yos-kernel.rom",
    "yos-kernel.rom.sha256",
    "shell.sys",
    "shell-asm.sys",
)
for model in ("48", "128", "next"):
    required.update(f"arch/{model}/{name}" for name in binary_names)

if not firmware_source.is_dir():
    raise SystemExit(f"missing vendored esxDOS runtime: {firmware_source}")
if not (release / "firmware/esxdos089/TMP").is_dir():
    raise SystemExit("missing deployment directory: firmware/esxdos089/TMP")
firmware_files = {
    "firmware/esxdos089/" + str(path.relative_to(firmware_source))
    for path in firmware_source.rglob("*") if path.is_file()
}
required.update(firmware_files)

missing = [name for name in required if not (release / name).is_file()]
if missing:
    raise SystemExit("missing distribution files: " + ", ".join(missing))
actual = {str(path.relative_to(release)) for path in release.rglob("*")
          if path.is_file()}
unexpected = sorted(actual - required)
if unexpected:
    raise SystemExit("non-deployment files in Spectrum release: " +
                     ", ".join(unexpected))
for model in ("48", "128", "next"):
    rom = release / "arch" / model / "yos-kernel.rom"
    if rom.stat().st_size != 0x4000:
        raise SystemExit(f"{model}/yos-kernel.rom is not exactly 16 KiB")
    for name in binary_names:
        if (release / "arch" / model / name).read_bytes() != \
                (release / "arch/48" / name).read_bytes():
            raise SystemExit(f"model-labelled shared binary differs: {model}/{name}")
for name in ("run-48.sh", "run-128.sh", "run-next.sh", "scripts/run-yos.py"):
    if not (release / name).stat().st_mode & stat.S_IXUSR:
        raise SystemExit(f"launcher is not executable: {name}")
for path in release_root.rglob("*"):
    if "size-audit" in path.parts:
        raise SystemExit(f"non-deployable audit artifact in bin/y: {path}")
    if path.name.lower() == "o" + "p.sys":
        raise SystemExit(f"obsolete boot filename in bin/y: {path}")

print("PASS: bin/y keeps shared assets at its root and arch/48, arch/128, "
      "and arch/next contain only identical model-labelled binaries")
