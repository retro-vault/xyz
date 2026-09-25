#!/usr/bin/env python3
"""Finish YOS's fixed ROM entry points and byte-tight tables, then checksum it."""
from hashlib import sha256
from pathlib import Path
import re
import sys

rom_path, map_path = map(Path, sys.argv[1:3])
symbols = {}
areas = []
for line in map_path.read_text().splitlines():
    area = re.fullmatch(r"\s+(_\w+)\s+([0-9A-Fa-f]+)\s+"
                        r"([0-9A-Fa-f]+)\s+REL CON\s*", line)
    if area:
        name, address, size = area.groups()
        address, size = int(address, 16), int(size, 16)
        if size and address < 0x4000:
            areas.append((address, address + size, name))
    fields = line.split()
    if len(fields) >= 2 and len(fields[0]) == 8:
        try:
            symbols[fields[1]] = int(fields[0], 16)
        except ValueError:
            pass
expected = {
    "__sys_reti": 0x09F0,
    "__sys_retn": 0x09F2,
    "__sys_vectors_start": 0x3CE1,
    "__sys_vectors_end": 0x3CF9,
}
for name, address in expected.items():
    if symbols.get(name) != address:
        raise SystemExit(f"{name} is not at {address:04x}h")
rom = bytearray(rom_path.read_bytes())
if len(rom) != 16384:
    raise SystemExit(f"YOS ROM must be exactly 16384 bytes, got {len(rom)}")
if symbols.get("s__GSFINAL", 0x10000) > 0x4000:
    raise SystemExit("YOS ROM content exceeds 16 KiB")
rom_end = symbols["s__GSFINAL"]
# Zero bytes can be live code/data. Check ownership from the map, not just
# the bytes, before allowing the patcher to install fixed-slot contents.
reserved = ((0x04C6, 0x04C7), (0x0562, 0x0563),
            (0x09F0, 0x09F7), (0x3CE1, 0x3E00))
for start, end, name in areas:
    for lo, hi in reserved:
        if start < hi and end > lo:
            raise SystemExit(f"{name} {start:04x}h..{end-1:04x}h "
                             f"overlaps reserved {lo:04x}h..{hi-1:04x}h")
if not areas or max(end for _, end, _ in areas) != rom_end:
    raise SystemExit("ROM occupied end disagrees with linked code/data areas")
for previous, following in zip(sorted(areas), sorted(areas)[1:]):
    if previous[1] > following[0]:
        raise SystemExit("linked ROM code/data areas overlap")
for start, end in reserved:
    if any(rom[start:end]):
        raise SystemExit(f"fixed ROM slot {start:04x}h..{end-1:04x}h is not empty")
# When all linked content ends before the 3CE1h reservation, xld emits its
# three-byte jump-over-reservation trampoline at 3CDEh. It is linker-owned
# content even though no relocatable area covers it.
tail_start = rom_end
if rom_end <= 0x3CDE:
    if rom[0x3CDE:0x3CE1] != bytes((0xC3, 0x00, 0x3E)):
        raise SystemExit("missing linker reservation trampoline")
    if any(rom[rom_end:0x3CDE]) or any(rom[0x3CE1:]):
        raise SystemExit("unused ROM tail is not zero-filled")
    tail_start = 0x3CF9
elif any(rom[rom_end:]):
    raise SystemExit("unused ROM tail is not zero-filled")
print_address = symbols["__esx_print"]
if not 0x0100 <= print_address < 0x4000:
    raise SystemExit("print handler is outside ROM")
rom[0x09F0:0x09F4] = bytes((0xED, 0x4D, 0xED, 0x45))  # RETI, RETN
if rom[0x0010:0x0016] != bytes((0xE5, 0xCD, print_address & 0xFF, print_address >> 8, 0xE1, 0xC9)):
    raise SystemExit("RST 10 print wrapper changed")
rom[0x09F4:0x09F7] = bytes((0xC3, 0x10, 0x00))  # same preserving wrapper as RST 10
rom[0x3CE1:0x3CF9] = bytes((0xC3, 0xF0, 0x09)) * 7 + bytes((0xC3, 0xF2, 0x09))
rom_path.write_bytes(rom)
checksum_path = rom_path.with_suffix(rom_path.suffix + ".sha256")
checksum_path.write_text(f"{sha256(rom).hexdigest()}  {rom_path.name}\n")
print(f"YOS ROM: linked content ends at {rom_end:04X}h; "
      f"{0x4000 - max(rom_end, tail_start)} contiguous zero-filled bytes free")
