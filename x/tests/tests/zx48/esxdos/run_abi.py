#!/usr/bin/env python3
"""Build and run deterministic esxDOS ABI/error tests with staged X tools."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main() -> int:
    here = Path(__file__).resolve().parent
    root = here.parents[4]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xcc", type=Path, default=root / "bin/x/bin/xcc")
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--platform", choices=["zx-esxdos", "zx-esxdos-rom"], default="zx-esxdos")
    parser.add_argument("--libxz80", type=Path, default=root / "bin/x/lib/libxz80.a")
    args = parser.parse_args()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=False)
    machine = work / "abi_machine"
    build = [args.cxx, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror",
             "-I", str(root / "x/lib/xz80/include"), str(here / "abi_machine.cpp"),
             str(args.libxz80.resolve()), "-o", str(machine)]
    subprocess.run(build, check=True)
    inputs = [args.xcc.resolve(), args.libxz80.resolve(), here / "abi_machine.cpp", here / "smoke.c"]
    library = args.xcc.resolve().parents[1] / "z80/lib"
    inputs += [library / name for name in
               (f"crt0-{args.platform}.rel", f"crt0-{args.platform}.s",
                f"lib{args.platform}.a", f"linker-{args.platform}.ld", f"linker-{args.platform}.lk")]
    inputs += [args.xcc.resolve().parent / name for name in ("xas", "xld")]
    inputs += [library / name for name in ("libc.a", "libruntime.a", "libfixed.a")]
    inputs += list((library.parent / "include").rglob("*.h"))
    inputs += [p for p in (root / "x/platforms" / args.platform).rglob("*")
               if p.is_file() and p.suffix in (".s", ".h", ".ld", ".lk")]
    inputs += [Path(__file__).resolve()]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    result = {"host_build": build, "host_machine_sha256": hashlib.sha256(machine.read_bytes()).hexdigest(),
              "platform": args.platform, "inputs": hashes, "lanes": []}
    profiles = ("-Os", "-Of") if args.platform == "zx-esxdos-rom" else ("-O0", "-Os", "-Of")
    for profile in profiles:
        for abi in (0, 1):
            name = f"{profile[1:]}-abi{abi}"
            image, mapfile = work / (name + ".bin"), work / (name + ".map")
            command = [str(args.xcc.resolve()), profile, "--sdcccall", str(abi),
                       "--platform=" + args.platform, "--oformat=binary", f"-Map={mapfile}",
                       str(here / "smoke.c"), "-o", str(image)]
            with (work / (name + ".compile.log")).open("w") as log:
                subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
            run_command = [str(machine), str(image), str(mapfile)]
            if args.platform == "zx-esxdos-rom": run_command.append("--rom")
            run = subprocess.run(run_command,
                                 text=True, capture_output=True)
            (work / (name + ".run.log")).write_text(run.stdout + run.stderr)
            result["lanes"].append({"name": name, "compiler_command": command,
                                     "run_command": run_command,
                                     "exit_code": run.returncode,
                                     "image_sha256": hashlib.sha256(image.read_bytes()).hexdigest(),
                                     "output": run.stdout + run.stderr})
            (work / "results.json").write_text(json.dumps(result, indent=2) + "\n")
            print(name + ": " + run.stdout + run.stderr, end="")
            run.check_returncode()
    result["inputs_unchanged"] = all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == digest
                                     for p, digest in hashes.items())
    (work / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    if not result["inputs_unchanged"]:
        raise RuntimeError("test input changed during the ABI matrix")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
