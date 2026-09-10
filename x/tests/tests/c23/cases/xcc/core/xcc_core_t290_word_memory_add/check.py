#!/usr/bin/env python3
"""Execute generated code with instrumented observable memory and ports."""
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
probe = output / "observable-probe"
subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2",
                "-I" + str(repository / "x/lib/xz80/include"),
                str(repository / "x/tests/tests/xopt/observable_memory_probe.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(probe)], check=True)
MODE = "rmw_array"
CASES = [1, 2, 3, 4, 5, 6, 7, 8]
checks = 0
fused = 0
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        for case in CASES:
            stem = output / f"case{case}-{profile}-abi{abi}"
            assembly, obj, binary, map_file = [stem.with_suffix(ext)
                                             for ext in (".s", ".rel", ".bin", ".map")]
            subprocess.run([str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
                            "-DCOMPILE_ONLY", "-DTEST_CASE=" + str(case), str(source),
                            "-o", str(assembly)], check=True)
            fused_sequence = re.search(r"ex\s+de, hl\s+add\s+hl, bc\s+ex\s+de, hl\s+ld\s+\(hl\), d\s+dec\s+hl\s+ld\s+\(hl\), e", assembly.read_text())
            fused_sequence = fused_sequence or re.search(r"ld\s+l, (-?\d+)\(iy\)\s+ld\s+h, -?\d+\(iy\)\s+add\s+hl, bc\s+ld\s+\1\(iy\), l\s+ld\s+-?\d+\(iy\), h", assembly.read_text())
            if case == 2:
                assert not fused_sequence, (case, profile, abi, "volatile fusion")
            if case == 1 and profile in ("Os", "Of", "O3"):
                assert fused_sequence, (case, profile, abi, "missing ordinary fusion")
                fused += 1
            subprocess.run([str(tools / "xas"), str(assembly), "-o", str(obj)], check=True)
            entry = "_accumulate"
            subprocess.run([str(tools / "xld"), "-nostdlib", "--no-default-runtime",
                            "--oformat=binary", "--section-start=_CODE=0x100",
                            "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
                            "-e", entry, "-Map=" + str(map_file), str(obj),
                            str(prefix / "z80/lib/libruntime.a"),
                            "-o", str(binary)], check=True)
            symbols = {m[2]: int(m[1], 16) for m in re.finditer(
                r"^([0-9A-Fa-f]{8}) (\S+)", map_file.read_text(), re.MULTILINE)}
            result = subprocess.run([str(probe), str(binary), MODE, str(case + (100 if case == 1 and profile in ("Os", "Of") else 0)), str(abi),
                                     str(symbols[entry]), str(symbols["_destination"]),
                                     str(symbols["_source"])],
                                    check=False, text=True, capture_output=True)
            assert result.returncode == 0, (case, profile, abi, result.stdout, result.stderr,
                                           str(assembly))
            checks += int(result.stdout.split()[0])
assert fused == 6
print(f"{checks} {MODE} value/access checks passed; {fused} fusion shapes verified")
