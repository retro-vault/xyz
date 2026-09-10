#!/usr/bin/env python3
"""Check typed byte-cache reuse and the corresponding observable reads."""
import os
import pathlib
import re
import subprocess
import sys

compiler, source, output_dir = sys.argv[1:]
output = pathlib.Path(output_dir)
output.mkdir(parents=True, exist_ok=True)
compiler = pathlib.Path(compiler).resolve()
repository = next(p for p in pathlib.Path(__file__).resolve().parents
                  if (p / "x/lib/xz80/include/xz80/xz80.h").exists())
prefix = compiler.parent.parent
if not (prefix / "lib/libxz80.a").exists(): prefix = repository / "bin/x"
tools = prefix / "bin"
probe = output / "observable-probe"
subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2",
    "-I" + str(repository / "x/lib/xz80/include"),
    str(repository / "x/tests/tests/xopt/observable_memory_probe.cpp"),
    str(prefix / "lib/libxz80.a"), "-o", str(probe)], check=True)
value_checks = 0
failures = []
checks = 0
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        for case in (1, 2, 3):
            if case == 1 and profile not in ("O3", "Of", "Os"):
                continue
            path = output / f"case{case}-{profile}-abi{abi}.s"
            subprocess.run([compiler, "-S", f"-{profile}", "--sdcccall", str(abi),
                            f"-DTEST_CASE={case}", source, "-o", str(path)],
                           check=True, capture_output=True, text=True)
            assembly = path.read_text()
            if case == 1:
                # A typed nonvolatile byte survives the cursor increment
                # and feeds the arithmetic directly, with no indexed reload.
                pattern = (r"ld\s+-\d+\(ix\),\s*a\s*\n"
                           r"\s*inc\s+(?:bc|iy)\s*\n\s*add\s+a,\s*a")
                ok = re.search(pattern, assembly) is not None
            else:
                obj, binary, map_file = [path.with_suffix(ext) for ext in (".rel", ".bin", ".map")]
                subprocess.run([str(tools/"xas"), str(path), "-o", str(obj)], check=True)
                entry = "_observable_byte_step" if case == 2 else "_observable_direct_byte_step"
                subprocess.run([str(tools/"xld"), "-nostdlib", "--no-default-runtime",
                    "--oformat=binary", "--section-start=_CODE=0x100", "--binary-range=0-0xffff",
                    "-e", entry, "-Map="+str(map_file), str(obj),
                    str(prefix/"z80/lib/libruntime.a"), "-o", str(binary)], check=True)
                symbols = {m[2]: int(m[1],16) for m in re.finditer(
                    r"^([0-9A-Fa-f]{8}) (\S+)", map_file.read_text(), re.MULTILINE)}
                result = subprocess.run([str(probe), str(binary), "byte_cache", str(case), str(abi),
                    str(symbols[entry]), "0", "0"], capture_output=True, text=True)
                ok = result.returncode == 0
                if ok: value_checks += int(result.stdout.split()[0])
                else: failures.append(result.stdout + result.stderr)
            checks += 1
            if not ok:
                failures.append(f"case {case}, {profile}, ABI {abi}: inspect {path}")
if failures:
    raise AssertionError("\n".join(failures))
print(f"{checks} typed byte-cache codegen checks and {value_checks} exact value/access checks passed")
