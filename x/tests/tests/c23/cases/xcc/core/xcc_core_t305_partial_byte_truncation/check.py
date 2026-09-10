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
driver = work / "partial_byte_driver"
host_command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O2",
                "-I" + str(root / "x/lib/xz80/include"), str(suite / "driver.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(driver)]
with (work / "host-build.log").open("w") as log:
    subprocess.run(host_command, stdout=log, stderr=subprocess.STDOUT, check=True)

# Generate the same generic arithmetic forms for every supported partial-byte
# destination.  The independent machine driver computes the mathematical low
# bits and sign extension; it does not inspect the generated assembly.
expressions = ("(int)x+129", "(int)x-129", "(int)x&213", "(int)x|129",
               "(int)x^213", "-(int)x", "~(int)x", "(int)x<<1",
               "(int)x>>1", "((int)x<<1)^85", "((int)x*3)+129",
               "((int)x+91)^183")
functions = []
source_lines = []
for signed in (0, 1):
    for width in range(2 if signed else 1, 8):
        for kind, expression in enumerate(expressions):
            name = f"f_{signed}_{width}_{kind}"
            qualifier = "signed" if signed else "unsigned"
            functions.append((name, signed, width, kind))
            source_lines.append(f"unsigned short {name}(unsigned char x) {{ "
                                f"return ({qualifier} _BitInt({width}))({expression}); }}")
source = work / "arithmetic.c"
source.write_text("\n".join(source_lines) + "\n")
results = []
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        stem = work / f"abi{abi}-{profile}"
        commands = [
            [str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
             str(source), "-o", str(stem.with_suffix(".s"))],
            [str(prefix / "bin/xas"), str(stem.with_suffix(".s")),
             "-o", str(stem.with_suffix(".rel"))],
            [str(prefix / "bin/xld"), "-nostdlib", "--no-default-runtime",
             "--oformat=binary", "--section-start=_CODE=0x100",
             "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
             "-e", "_f_0_1_0", "-Map=" + str(stem.with_suffix(".map")),
             str(stem.with_suffix(".rel")), str(prefix / "z80/lib/libruntime.a"),
             "-o", str(stem.with_suffix(".bin"))],
        ]
        with stem.with_suffix(".log").open("w") as log:
            for command in commands:
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        symbols = {match[2]: int(match[1], 16) for match in re.finditer(
            r"^([0-9A-Fa-f]{8}) (\S+)", stem.with_suffix(".map").read_text(), re.M)}
        stem.with_suffix(".cases").write_text("".join(
            f"{symbols['_'+name]} {signed} {width} {kind}\n"
            for name, signed, width, kind in functions))
        command = [str(driver), str(stem.with_suffix(".bin")),
                   str(stem.with_suffix(".cases")), str(abi)]
        result = subprocess.run(command, capture_output=True, text=True)
        with stem.with_suffix(".log").open("a") as log:
            log.write(result.stdout + result.stderr)
        assert result.returncode == 0, (abi, profile, result.stdout, result.stderr)
        assert result.stdout.strip() == "39936 independent narrow arithmetic values passed"
        results.append({"abi": abi, "profile": profile, "checks": 39936})

(work / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(sum(row["checks"] for row in results), "partial-byte arithmetic checks passed across both ABIs and all profiles")
