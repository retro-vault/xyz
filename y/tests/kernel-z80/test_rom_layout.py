#!/usr/bin/env python3
"""Exercise ROM ownership checks, including zero-valued live data."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

rom_path, map_path, work = map(Path, sys.argv[1:4])
patcher = Path(__file__).resolve().parents[2] / "scripts" / "patch_rom.py"
image = rom_path.read_bytes()
mapping = map_path.read_text()
reserved = ((0x04C6, 0x04C7), (0x0562, 0x0563),
            (0x09F0, 0x09F7), (0x3CE1, 0x3E00))
raw = bytearray(image)
for lo, hi in reserved:
    raw[lo:hi] = bytes(hi - lo)
work.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="layout-", dir=work) as directory:
    rom, linkmap = Path(directory) / "test.rom", Path(directory) / "test.map"

    def check(data, text, error=None):
        rom.write_bytes(data)
        linkmap.write_text(text)
        result = subprocess.run([sys.executable, str(patcher), str(rom),
                                 str(linkmap)], capture_output=True, text=True)
        if error is None:
            assert result.returncode == 0, result.stderr
            assert rom.read_bytes() == image, "patcher changed live ROM bytes"
        else:
            assert result.returncode != 0 and error in result.stderr, result
            assert rom.read_bytes() == data, "failed validation modified ROM"

    check(raw, mapping)
    for lo, hi in reserved:
        # All-zero live data must be rejected even though byte checks pass.
        fake_area = f"  _CONST            {lo:04X}   0001   REL CON\n"
        check(raw, mapping + fake_area, "overlaps reserved")
        damaged = bytearray(raw)
        damaged[lo] = 1
        check(damaged, mapping, "is not empty")

    damaged = bytearray(raw)
    damaged[0x3fff] = 1
    check(damaged, mapping, "tail is not zero-filled")
    oversized = re.sub(r"[0-9A-F]{8} s__GSFINAL",
                       "00004001 s__GSFINAL", mapping)
    check(raw, oversized, "exceeds 16 KiB")
    check(raw, mapping + "  _CONST            0100   0001   REL CON\n",
          "areas overlap")
print("PASS: reserved ROM ownership, zero-valued live data, "
      "16 KiB boundary protection and non-destructive validation failures")
