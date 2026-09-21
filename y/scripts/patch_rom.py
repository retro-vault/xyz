#!/usr/bin/env python3
"""Finish YOS's fixed ROM entry points and byte-tight tables, then checksum it."""
from hashlib import sha256
from pathlib import Path
import sys

rom_path, map_path = map(Path, sys.argv[1:3])
symbols = {}
for line in map_path.read_text().splitlines():
    fields = line.split()
    if len(fields) >= 2 and len(fields[0]) == 8:
        try:
            symbols[fields[1]] = int(fields[0], 16)
        except ValueError:
            pass
expected = {
    "__sys_reti": 0x09F0,
    "__sys_retn": 0x09F2,
    "__gpx_name": 0x3CE1,
    "__sys_vectors_start": 0x3CE5,
    "__sys_vectors_end": 0x3CFD,
}
for name, address in expected.items():
    if symbols.get(name) != address:
        raise SystemExit(f"{name} is not at {address:04x}h")
rom = bytearray(rom_path.read_bytes())
if len(rom) != 16384:
    raise SystemExit(f"YOS ROM must be exactly 16384 bytes, got {len(rom)}")
if symbols.get("s__GSFINAL", 0x10000) > 0x4000:
    raise SystemExit("YOS ROM content exceeds 16 KiB")
for start, end in ((0x09F0, 0x09F2), (0x09F4, 0x09F7), (0x3CE1, 0x3CFD)):
    if any(rom[start:end]):
        raise SystemExit(f"fixed ROM slot {start:04x}h..{end-1:04x}h is not empty")
print_address = symbols["__esx_print"]
if not 0x0100 <= print_address < 0x4000:
    raise SystemExit("print handler is outside ROM")
timer_end = symbols["__yos_install_timer"] + 8
if timer_end > 0x09F0 or rom[timer_end - 1] != 0xC9 or rom[0x09F2:0x09F4] != bytes((0x18, 0x03)):
    raise SystemExit("timer return or 09F4h paging bridge changed")
rom[0x09F0:0x09F4] = bytes((0xED, 0x4D, 0xED, 0x45))  # RETI, RETN
if rom[0x0010:0x0016] != bytes((0xE5, 0xCD, print_address & 0xFF, print_address >> 8, 0xE1, 0xC9)):
    raise SystemExit("RST 10 print wrapper changed")
rom[0x09F4:0x09F7] = bytes((0xC3, 0x10, 0x00))  # same preserving wrapper as RST 10
rom[0x3CE1:0x3CE5] = b"gpx\0"
rom[0x3CE5:0x3CFD] = bytes((0xC3, 0xF0, 0x09)) * 7 + bytes((0xC3, 0xF2, 0x09))
rom_path.write_bytes(rom)
checksum_path = rom_path.with_suffix(rom_path.suffix + ".sha256")
checksum_path.write_text(f"{sha256(rom).hexdigest()}  {rom_path.name}\n")
