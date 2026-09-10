#!/usr/bin/env python3
"""Count observable accesses, independently of the values computed from them."""

import pathlib
import re
import subprocess
import sys

compiler, source, output_dir = sys.argv[1:]
output = pathlib.Path(output_dir)
output.mkdir(parents=True, exist_ok=True)

# Stack-frame traffic uses IX. Pointer reads may use HL, BC, DE, or IY.
indirect_read = re.compile(
    r"^\s*ld\s+[abcdehl],\s*(?:\((?:hl|bc|de|iy)\)|-?\d+\(iy\))\s*$"
)
direct_read = re.compile(r"^\s*ld\s+[abcdehl],\s*\(_observable_byte\)\s*$")
signed_read = re.compile(r"^\s*ld\s+[abcdehl],\s*\(_observable_signed_byte\)\s*$")
local_write = re.compile(r"^\s*ld\s*-\d+\(ix\),\s*(?:[abcdehl]|#[0-9]+)\s*$")
local_read = re.compile(r"^\s*ld\s+[abcdehl],\s*-1\(ix\)\s*$")
port_read = re.compile(r"^\s*in\s+a,\s*\([^)]+\)\s*$")
port_memory_access = re.compile(
    r"^\s*ld\s+.*\(_observable_(?:wide_)?port\)"
)

cases = {
    1: (indirect_read, 1),
    2: (indirect_read, 2),
    3: (indirect_read, 2),
    4: (direct_read, 1),
    5: (indirect_read, 1),
    6: (local_write, 2),
    7: (direct_read, 2),
    8: (signed_read, 1),
    9: (direct_read, 1),
    10: (local_read, 2),
    11: (port_read, 1),
    12: (port_read, 2),
    13: (port_read, 1),
    14: (port_read, 1),
    15: (port_read, 1),
    16: (port_read, 1),
    17: (direct_read, 3),
}
failures = []
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        for case, (pattern, expected) in cases.items():
            assembly = output / f"case{case}-{profile}-abi{abi}.s"
            subprocess.run(
                [compiler, "-S", f"-{profile}", "--sdcccall", str(abi),
                 f"-DTEST_CASE={case}", source, "-o", str(assembly)],
                check=True, capture_output=True, text=True,
            )
            instructions = [line.split(";", 1)[0]
                            for line in assembly.read_text().splitlines()]
            count = sum(bool(pattern.fullmatch(line)) for line in instructions)
            if count != expected:
                failures.append(
                    f"case {case}, {profile}, ABI {abi}: expected {expected} "
                    f"observable accesses, got {count}; inspect {assembly}"
                )
            if case >= 11 and any(port_memory_access.match(line)
                                  for line in instructions):
                failures.append(
                    f"case {case}, {profile}, ABI {abi}: SFR accessed through "
                    f"the memory address space; inspect {assembly}"
                )
if failures:
    raise AssertionError("\n".join(failures))
print(f"{len(cases) * 12} exact volatile/SFR-access checks passed")
