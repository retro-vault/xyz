#!/usr/bin/env python3
"""Render the already linked RAM kernel as bytes in its validated XPRG carrier."""
from pathlib import Path
import sys
source, target = map(Path, sys.argv[1:])
data = source.read_bytes()
if len(data) != 16384:
    raise SystemExit("divMMC kernel must occupy exactly C000h..FFFFh")
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text("".join("        .db " + ",".join(f"0x{b:02x}" for b in data[n:n+16]) + "\n"
                          for n in range(0, len(data), 16)))
