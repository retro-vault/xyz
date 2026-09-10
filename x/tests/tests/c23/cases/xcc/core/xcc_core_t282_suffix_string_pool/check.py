#!/usr/bin/env python3
"""Check literal storage and relocation independently of IR label numbering."""
import pathlib
import re
import subprocess
import sys


def literal_storage(assembly):
    section = None
    data = bytearray()
    labels = {}
    for raw in assembly.splitlines():
        line = raw.split(";", 1)[0].strip()
        if line.startswith(".area"):
            section = line.split()[1]
        if section not in ("_CODE", "_CONST"):
            continue
        if line.endswith(":"):
            labels[line[:-1]] = len(data)
        match = re.fullmatch(r"\.(db|dw|dl)\s+(.+)", line)
        if match:
            width = {"db": 1, "dw": 2, "dl": 4}[match[1]]
            for word in match[2].split(","):
                data.extend(int(word.strip(), 0).to_bytes(width, "little"))
    return data, labels


compiler, source, output_directory = sys.argv[1:]
output = pathlib.Path(output_directory)
output.mkdir(parents=True, exist_ok=True)
checks = 0
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        assembly = output / f"pool-{profile}-abi{abi}.s"
        subprocess.run([compiler, "-S", f"-{profile}", "--sdcccall", str(abi),
                        "-DPOOL_DATA_ONLY", source, "-o", str(assembly)],
                       check=True, capture_output=True, text=True)
        data, labels = literal_storage(assembly.read_text())
        # Narrow/16/32-bit: 40 units separately, or 24 units sharing tails.
        # UTF-8: 30 bytes separately, or 18 bytes sharing tails. The u8 tag
        # must not cause code units to expand to 32 bits.
        optimized = profile in ("O3", "Of", "Os")
        expected = (24 * 7 + 18) if optimized else (40 * 7 + 30)
        assert len(data) == expected, (profile, abi, len(data), expected)
        assert len(labels) == 26, (profile, abi, labels)
        # Every independently encoded payload must occur whole. Unlike
        # strncmp, suffix matching must inspect bytes after an embedded zero.
        for width in (1, 2, 4):
            for value in (b"alphabet\0", b"bet\0", b"\0", b"a\0tail\0",
                          b"tail\0", b"a\0other\0", b"other\0"):
                encoded = b"".join(c.to_bytes(width, "little") for c in value)
                assert any(data[start:start + len(encoded)] == encoded
                           for start in labels.values()), (width, value, assembly)
        assert b"\xc3\xa9suffix\0" in data, assembly
        checks += 1
print(f"{checks} literal suffix/encoding/label checks passed")
