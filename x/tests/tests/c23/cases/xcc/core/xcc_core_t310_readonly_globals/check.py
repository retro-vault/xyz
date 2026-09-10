"""Check section ownership and linked constant bytes in both object formats."""
from pathlib import Path
import json
import re
import subprocess
import sys

compiler, source, output = map(Path, sys.argv[1:])
compiler = compiler.resolve()
work = output.resolve()
work.mkdir(parents=True, exist_ok=True)
tools = compiler.parent
readonly = ["ro_scalar", "ro_zero", "ro_implicit", "ro_zero_array", "ro_array",
            "ro_nested", "ro_typedef_array", "ro_record", "ro_union", "ro_pointer",
            "ro_const_pointer", "ro_pointer_array", "ro_volatile_pointee"]
writable = ["rw_word", "rw_pointee_const", "rw_pointer_array", "rw_cv", "rw_atomic",
            "rw_cv_array", "rw_volatile_record", "rw_atomic_record"]
results = []


def run(command, log):
    result = subprocess.run([str(arg) for arg in command], capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr)
    assert result.returncode == 0, (command, result.stdout, result.stderr)


for mode in ("sdcc", "gnu"):
    ro, data, bss, tls = (("_CONST", "_DATA", "_BSS", "_TLS") if mode == "sdcc"
                          else (".rodata", ".data", ".bss", ".tdata"))
    for profile in ("O0", "O1", "O2", "O3", "Os", "Of"):
        for abi in (0, 1):
            stem = work / f"{mode}-{profile}-{abi}"
            assembly = stem.with_suffix(".s")
            run([compiler, "-" + profile, "--sdcccall", abi, "--mode=" + mode,
                 "-S", source, "-o", assembly], stem.with_suffix(".compile.log"))
            section, labels = None, {}
            text = assembly.read_text()
            for line in text.splitlines():
                match = re.match(r"\s*\.(?:area|section)\s+([^,\s]+)", line)
                if match:
                    section = match[1]
                elif re.fullmatch(r"\s*\.(?:data|bss|text)\s*", line):
                    section = line.strip()
                match = re.fullmatch(r"(_[A-Za-z0-9_]+):+", line)
                if match:
                    assert match[1] not in labels, match[1]
                    labels[match[1]] = section
            for symbol in readonly:
                assert labels.get("_" + symbol) == ro, (stem, symbol, labels)
            for symbol in writable:
                assert labels.get("_" + symbol) == data, (stem, symbol, labels)
            assert labels["_rw_zero"] == bss
            assert labels["_bank_const"] == "_DATA_BANK_2"
            assert labels["__tls_template"] == tls
            assert "_absolute_const" not in labels and "_port_const" not in labels
            assert re.search(r"_absolute_const(?:,|\s*=)\s*37120", text)
            assert re.search(r"_port_const(?:,|\s*=)\s*254", text)
            if mode == "gnu":
                # GNU absolute-symbol linking is separate from section
                # selection. Keep those override checks above; link the
                # ordinary objects without at/SFR/TLS absolute aliases.
                assembly = stem.with_suffix(".link.s")
                run([compiler, "-" + profile, "--sdcccall", abi,
                     "--mode=" + mode, "-DREADONLY_LINK_ONLY", "-S", source,
                     "-o", assembly], stem.with_suffix(".link-compile.log"))
            obj, image, mapfile = (stem.with_suffix(suffix) for suffix in (".rel", ".bin", ".map"))
            run([tools / "xas", "--mode=" + mode, assembly, "-o", obj], stem.with_suffix(".assemble.log"))
            command = [tools / "xld", "--mode=" + mode, "-nostdlib", "--no-default-runtime",
                       "--oformat=binary", "--binary-range=0x0000-0xffff", "-e", "_ro_scalar"]
            for area, address in [(ro, "0x1000"), (data, "0x8000"), (bss, "0xa000"),
                                  (tls, "0xb000"), ("_DATA_BANK_2", "0x9000")]:
                command += ["-b", area + "=" + address]
            run(command + ["-Map=" + str(mapfile), obj, "-o", image], stem.with_suffix(".link.log"))
            symbols = {match[2]: int(match[1], 16) for line in mapfile.read_text().splitlines()
                       if (match := re.fullmatch(r"([0-9a-fA-F]{8}) (\S+)(?:\s*;.*)?", line))}
            blob = image.read_bytes()
            assert len(blob) == 65536
            for symbol, count in [("ro_zero", 2), ("ro_implicit", 2), ("ro_zero_array", 5)]:
                start = symbols["_" + symbol]
                assert blob[start:start + count] == bytes(count), (stem, symbol)
            for symbol, target in [("ro_pointer", "rw_word"), ("ro_const_pointer", "ro_scalar"),
                                   ("rw_pointee_const", "ro_scalar")]:
                start = symbols["_" + symbol]
                assert int.from_bytes(blob[start:start + 2], "little") == symbols["_" + target]
            start = symbols["_ro_nested"]
            assert blob[start:start + 8] == bytes([1, 0, 2, 0, 3, 0, 4, 0])
            results.append({"mode": mode, "profile": profile, "abi": abi, "passed": True})
(work / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(f"readonly global placement and linked initializers: {len(results)} cases passed")
