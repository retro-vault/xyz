#!/usr/bin/env python3
"""Test comparison casts independently from arbitrary-value truncation casts."""
import os
import pathlib
import re
import subprocess
import sys

compiler, source, directory = map(pathlib.Path, sys.argv[1:])
output = directory.resolve()
output.mkdir(parents=True, exist_ok=True)
repository = next(p for p in pathlib.Path(__file__).resolve().parents
                  if (p / "x/lib/xz80/include/xz80/xz80.h").exists())
prefix = compiler.resolve().parent.parent
if not (prefix / "lib/libxz80.a").exists():
    prefix = repository / "bin/x"
tools = prefix / "bin"
probe = output / "comparison-probe"
subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2",
                "-I" + str(repository / "x/lib/xz80/include"),
                str(repository / "x/tests/tests/xopt/comparison_cast_probe.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(probe)], check=True)
cases = [(f"cmp{i}", i % 6, 1 if i < 7 else 2, 0) for i in range(12)]
cases += [("truncate_byte", 0, 1, 1), ("truncated_truth", 1, 1, 1),
          ("truncate_bit", 2, 1, 1), ("truncated_bit_truth", 3, 1, 1)]
# Some older frontends accept signed _BitInt(1) as an extension. If it is
# accepted, 1 converts to -1, so the comparison-specific proof must decline
# the direct0/1 handoff. A conforming rejection is accepted too.
invalid = output / "signed-bitint1.c"
invalid.write_text("_BitInt(1) signed_bitint1(unsigned char a, unsigned char b) { return a == b; }\n")
checks = 0
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        stem = output / f"{profile}-abi{abi}"
        assembly, obj, binary, map_file = [stem.with_suffix(ext)
                                         for ext in (".s", ".rel", ".bin", ".map")]
        subprocess.run([str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
                        "-DCOMPILE_ONLY", str(source), "-o", str(assembly)], check=True)
        if abi == 1 and profile in ("O3", "Of"):
            first = assembly.read_text().split("_cmp0:", 1)[1].split(".globl", 1)[0]
            assert "frameless function" in first, (profile, "comparison gained a frame")
            assert "(ix)" not in first, (profile, "comparison gained a spill")
        subprocess.run([str(tools / "xas"), str(assembly), "-o", str(obj)], check=True)
        extra_objects = []
        lane_cases = list(cases)
        extra_asm, extra_obj = stem.with_suffix(".signed.s"), stem.with_suffix(".signed.rel")
        extension = subprocess.run([str(compiler), "-S", "-" + profile,
                                    "--sdcccall", str(abi), str(invalid),
                                    "-o", str(extra_asm)], text=True, capture_output=True)
        if extension.returncode == 0:
            subprocess.run([str(tools / "xas"), str(extra_asm), "-o", str(extra_obj)], check=True)
            extra_objects.append(str(extra_obj))
            lane_cases.append(("signed_bitint1", 0, 1, 2))
        else:
            assert "_BitInt" in extension.stderr, extension.stderr
        subprocess.run([str(tools / "xld"), "-nostdlib", "--no-default-runtime",
                        "--oformat=binary", "--section-start=_CODE=0x100",
                        "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
                        "-e", "_cmp0", "-Map=" + str(map_file), str(obj), *extra_objects,
                        str(prefix / "z80/lib/libruntime.a"), "-o", str(binary)], check=True)
        symbols = {m[2]: int(m[1], 16) for m in re.finditer(
            r"^([0-9A-Fa-f]{8}) (\S+)", map_file.read_text(), re.MULTILINE)}
        case_file = stem.with_suffix(".cases")
        case_file.write_text("\n".join(
            f"{symbols['_'+name]} {operation} {size} {kind}"
            for name, operation, size, kind in lane_cases) + "\n")
        result = subprocess.run([str(probe), str(binary), str(case_file), str(abi)],
                                text=True, capture_output=True)
        assert result.returncode == 0, (profile, abi, result.stdout, result.stderr)
        checks += int(result.stdout.split()[0])
print(f"{checks} exhaustive comparison-cast value/ABI checks passed")
