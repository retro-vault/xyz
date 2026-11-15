#!/usr/bin/env python3
"""Check the paging kernel's fixed RAM, bounce area and startup stack limits."""
from pathlib import Path
import re
import sys
image, mapping = map(Path, sys.argv[1:])
if len(image.read_bytes()) != 16384:
    raise SystemExit("divMMC kernel must be exactly 16 KiB")
areas=[(name,int(start,16),int(size,16)) for name,start,size in re.findall(
    r"^\s+(_\w+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+REL",mapping.read_text(),re.M)]
end=max(start+size for name,start,size in areas if start>=0xc000 and size)
assert end<=0xfff7, f"RAM kernel overlaps startup stack: {end:04X}"
assert all(start+size<=0x5e00 for name,start,size in areas if name in ("_DATA","_BSS") and size)
assert all(0x5e00<=start and start+size<=0x5efd for name,start,size in areas if name=="_DIVMMC_STATE")
print(f"PASS divMMC RAM layout; code ends at {end:04X}, startup stack reserved")
