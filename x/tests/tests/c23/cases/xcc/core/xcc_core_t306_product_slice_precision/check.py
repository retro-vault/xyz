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
driver = work / "product_slice_driver"
host_command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O2",
                "-I" + str(root / "x/lib/xz80/include"), str(suite / "driver.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(driver)]
with (work / "host-build.log").open("w") as log:
    subprocess.run(host_command, stdout=log, stderr=subprocess.STDOUT, check=True)

functions = json.loads((suite / "functions.json").read_text())
results = []
for abi in (0, 1):
    for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
        stem = work / f"abi{abi}-{profile}"
        commands = [
            [str(compiler), "-S", "-" + profile, "--sdcccall", str(abi),
             str(suite / "products.c"), "-o", str(stem.with_suffix(".s"))],
            [str(prefix / "bin/xas"), str(stem.with_suffix(".s")),
             "-o", str(stem.with_suffix(".rel"))],
            [str(prefix / "bin/xld"), "-nostdlib", "--no-default-runtime",
             "--oformat=binary", "--section-start=_CODE=0x100",
             "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
             "-e", "_dest_0_9_8", "-Map=" + str(stem.with_suffix(".map")),
             str(stem.with_suffix(".rel")), str(prefix / "z80/lib/libruntime.a"),
             "-o", str(stem.with_suffix(".bin"))],
        ]
        with stem.with_suffix(".log").open("w") as log:
            for command in commands:
                subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        symbols = {match[2]: int(match[1], 16) for match in re.finditer(
            r"^([0-9A-Fa-f]{8}) (\S+)", stem.with_suffix(".map").read_text(), re.M)}
        stem.with_suffix(".cases").write_text("".join(
            f"{symbols['_'+name]} {source_width} {product_width} {width} {signed} {shift} {factor}\n"
            for name, source_width, product_width, width, signed, shift, factor in functions))
        command = [str(driver), str(stem.with_suffix(".bin")),
                   str(stem.with_suffix(".cases")), str(abi)]
        result = subprocess.run(command, capture_output=True, text=True)
        with stem.with_suffix(".log").open("a") as log:
            log.write(result.stdout + result.stderr)
        assert result.returncode == 0, (abi, profile, result.stdout, result.stderr)
        assert result.stdout.strip() == "19008 product precision checks passed"
        results.append({"abi": abi, "profile": profile, "checks": 19008})

# Full storage-width arithmetic must retain the early fusion and frameless
# register ABI. Compile separately so identical-function merging in the large
# semantic oracle cannot replace this body with an alias to another case.
for profile in ("O3", "Of", "Os"):
    assembly = work / ("full-width-" + profile + ".s")
    subprocess.run([str(compiler), "-S", "-" + profile, "--sdcccall", "1",
                    str(suite / "full_width.c"), "-o", str(assembly)], check=True)
    text = assembly.read_text()
    assert re.search(r"call\s+___muluint2ulong", text), (profile, text)
    assert re.search(r"ld\s+e,\s*d", text), (profile, text)
    assert re.search(r"ld\s+d,\s*l", text), (profile, text)
    assert "frameless function" in text, (profile, text)
    assert not re.search(r"(?:push|pop)\s+ix|\(ix\)|add\s+hl,\s*sp", text), (profile, text)

(work / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(sum(row["checks"] for row in results), "product precision checks passed across both ABIs and all profiles")
