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
MODE = "indirect"
CASES = [1, 2, 3, 4, 5]
checks = 0
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        for case in CASES:
            stem = output / f"case{case}-{profile}-abi{abi}"
            assembly, obj, binary, map_file = [stem.with_suffix(ext)
                                             for ext in (".s", ".rel", ".bin", ".map")]
            subprocess.run([str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
                            "-DCOMPILE_ONLY", "-DTEST_CASE=" + str(case), str(source),
                            "-o", str(assembly)], check=True)
            subprocess.run([str(tools / "xas"), str(assembly), "-o", str(obj)], check=True)
            entry = "_choose" if MODE == "switch" else "_increment"
            if MODE == "indirect":
                entry = {1: "_increment", 2: "_add_one", 3: "_post_increment",
                         4: "_volatile_increment", 5: "_increment_twice"}[case]
            subprocess.run([str(tools / "xld"), "-nostdlib", "--no-default-runtime",
                            "--oformat=binary", "--section-start=_CODE=0x100",
                            "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
                            "-e", entry, "-Map=" + str(map_file), str(obj),
                            str(prefix / "z80/lib/libruntime.a"),
                            "-o", str(binary)], check=True)
            symbols = {m[2]: int(m[1], 16) for m in re.finditer(
                r"^([0-9A-Fa-f]{8}) (\S+)", map_file.read_text(), re.MULTILINE)}
            result = subprocess.run([str(probe), str(binary), MODE, str(case), str(abi),
                                     str(symbols[entry]), str(symbols.get("_control", 0xc000)),
                                     str(symbols.get("_calls", 0xc002))],
                                    check=False, text=True, capture_output=True)
            assert result.returncode == 0, (case, profile, abi, result.stdout, result.stderr,
                                           str(assembly))
            checks += int(result.stdout.split()[0])
print(f"{checks} {MODE} value/access checks passed")
