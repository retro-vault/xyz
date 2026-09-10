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
driver = work / "byte_view_driver"
host_command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O2",
                "-I" + str(root / "x/lib/xz80/include"), str(suite / "driver.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(driver)]
with (work / "host-build.log").open("w") as log:
    subprocess.run(host_command, stdout=log, stderr=subprocess.STDOUT, check=True)

functions = json.loads((suite / "functions.json").read_text())
checks = len(functions) * 256
results = []
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        stem = work / f"abi{abi}-{profile}"
        commands = [
            [str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
             str(suite / "views.c"), "-o", str(stem.with_suffix(".s"))],
            [str(prefix / "bin/xas"), str(stem.with_suffix(".s")),
             "-o", str(stem.with_suffix(".rel"))],
            [str(prefix / "bin/xld"), "-nostdlib", "--no-default-runtime",
             "--oformat=binary", "--section-start=_CODE=0x100",
             "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
             "-e", "_" + functions[0][0], "-Map=" + str(stem.with_suffix(".map")),
             str(stem.with_suffix(".rel")), str(prefix / "z80/lib/libruntime.a"),
             "-o", str(stem.with_suffix(".bin"))],
        ]
        with stem.with_suffix(".log").open("w") as log:
            for command in commands:
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        symbols = {match[2]: int(match[1], 16) for match in re.finditer(
            r"^([0-9A-Fa-f]{8}) (\S+)", stem.with_suffix(".map").read_text(), re.M)}
        stem.with_suffix(".cases").write_text("".join(
            f"{name} {symbols['_'+name]} {family} {source_sign} {width} {sign} {wide}\n"
            for name, family, source_sign, width, sign, wide in functions))
        result = subprocess.run([str(driver), str(stem.with_suffix(".bin")),
                                 str(stem.with_suffix(".cases")), str(abi)], capture_output=True, text=True)
        with stem.with_suffix(".log").open("a") as log:
            log.write(result.stdout + result.stderr)
        assert result.returncode == 0, (abi, profile, result.stdout, result.stderr)
        assert result.stdout.strip() == f"{checks} byte numeric-view checks passed"
        print(f"ABI{abi} {profile}: {checks} byte numeric-view checks passed", flush=True)
        results.append({"abi": abi, "profile": profile, "checks": checks})

# Matching ordinary byte views retain direct register widening. These small
# independent functions need no helper call or temporary home; the speed
# profiles also eliminate the ABI frame (Os retains its existing frame policy).
for kind in ("unsigned", "signed"):
    for profile in ("O3", "Of", "Os"):
        assembly = work / ("pure-" + kind + "-" + profile + ".s")
        subprocess.run([str(compiler), "-S", "-" + profile, "--sdcccall", "1",
                        str(suite / ("pure_" + kind + ".c")), "-o", str(assembly)], check=True)
        text = assembly.read_text()
        assert "locals=0, temp_frame=0, stack_params=0" in text, (profile, text)
        assert not re.search(r"\(ix\)|\bcall\b", text), (profile, text)
        if profile != "Os":
            assert not re.search(r"(?:push|pop)\s+ix|add\s+hl,\s*sp", text), (profile, text)

(work / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(sum(row["checks"] for row in results), "byte numeric-view checks passed across both ABIs and all profiles")
