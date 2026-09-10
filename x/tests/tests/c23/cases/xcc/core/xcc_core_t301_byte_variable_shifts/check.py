import json
import os
import pathlib
import re
import shlex
import subprocess
import sys

compiler = pathlib.Path(sys.argv[1]).resolve()
work = pathlib.Path(sys.argv[2]).resolve()
suite = pathlib.Path(__file__).resolve().parent
root = next(parent for parent in suite.parents if (parent / "x/lib/xz80/include").is_dir())
prefix = compiler.parent.parent
work.mkdir(parents=True, exist_ok=True)
driver = work / "byte_shift_driver"
host_command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O2",
                "-I" + str(root / "x/lib/xz80/include"), str(suite / "driver.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(driver)]
with (work / "host-build.log").open("w") as log:
    subprocess.run(host_command, stdout=log, stderr=subprocess.STDOUT, check=True)

results = []
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        stem = work / f"abi{abi}-{profile}"
        commands = [
            [str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
             str(suite / "shifts.c"), "-o", str(stem.with_suffix(".s"))],
            [str(prefix / "bin/xas"), str(stem.with_suffix(".s")),
             "-o", str(stem.with_suffix(".rel"))],
            [str(prefix / "bin/xld"), "-nostdlib", "--no-default-runtime",
             "--oformat=binary", "--section-start=_CODE=0x100",
             "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
             "-e", "_shift_left", "-Map=" + str(stem.with_suffix(".map")),
             str(stem.with_suffix(".rel")), str(prefix / "z80/lib/libruntime.a"),
             "-o", str(stem.with_suffix(".bin"))],
        ]
        with stem.with_suffix(".log").open("w") as log:
            for command in commands:
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        symbols = {match[2]: int(match[1], 16) for match in re.finditer(
            r"^([0-9A-Fa-f]{8}) (\S+)", stem.with_suffix(".map").read_text(), re.M)}
        functions = ("_shift_left", "_shift_logical", "_shift_arithmetic")
        stem.with_suffix(".cases").write_text("".join(
            f"{symbols[name]} {kind}\n" for kind, name in enumerate(functions)))
        command = [str(driver), str(stem.with_suffix(".bin")),
                   str(stem.with_suffix(".cases")), str(abi)]
        result = subprocess.run(command, capture_output=True, text=True)
        with stem.with_suffix(".log").open("a") as log:
            log.write(result.stdout + result.stderr)
        assert result.returncode == 0, (abi, profile, result.stdout, result.stderr)
        assert result.stdout.strip() == "6144 exhaustive byte-shift checks passed"
        results.append({"abi": abi, "profile": profile, "checks": 6144})

(work / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(sum(row["checks"] for row in results), "exhaustive byte-shift checks passed across both ABIs and all profiles")
