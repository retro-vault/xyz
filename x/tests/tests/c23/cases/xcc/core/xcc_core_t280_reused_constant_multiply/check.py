import pathlib
import re
import subprocess
import sys


compiler, output = sys.argv[1:]
output = pathlib.Path(output)
output.mkdir(parents=True, exist_ok=True)
scaled = "unsigned scaled(unsigned x) { return x * 0x1357u; }\n"
dynamic = "unsigned dynamic(unsigned x, unsigned y) { return x * y; }\n"
cases = {
    "empty": ("unsigned identity(unsigned x) { return x; }\n", 0, 0),
    "cold": (scaled, 0, 0),
    "warm": (dynamic + scaled, 2, 1),
    "short": (dynamic + "unsigned scaled(unsigned x) { return x * 257u; }\n", 1, 1),
    "dead": ("static " + dynamic + scaled, 0, 0),
    "different_helper": (
        "unsigned middle(unsigned x, unsigned y) {\n"
        "  return (unsigned)(((unsigned long)x * (unsigned long)y) >> 8);\n"
        "}\n" + scaled, 0, 0),
    "banked": ("[[xcc::bank(3)]] " + dynamic + scaled, 1, 1),
}
checked = 0
for name, (source, size_calls, speed_calls) in cases.items():
    source_path = output / f"{name}.c"
    source_path.write_text(source)
    for abi in (0, 1):
        for profile, expected_calls in (("Os", size_calls), ("Of", speed_calls)):
            for runtime in ("x", "z88dk-classic"):
                asm_path = output / f"{name}-{abi}-{profile}-{runtime}.s"
                subprocess.run(
                    [compiler, f"-{profile}", "--sdcccall", str(abi),
                     f"--runtime={runtime}", "-fno-peephole", "-S",
                     str(source_path), "-o", str(asm_path)],
                    text=True, capture_output=True, check=True,
                )
                assembly = asm_path.read_text()
                calls = len(re.findall(r"^\s*call\s+__mul16\s*$", assembly, re.M))
                assert calls == expected_calls, (name, abi, profile, runtime, calls, assembly)
                if name == "warm" and profile == "Os":
                    body = assembly.split("_scaled:", 1)[1]
                    assert re.search(r"push\s+bc[\s\S]*call\s+__mul16[\s\S]*pop\s+bc", body), body
                checked += 1
print(f"PASS: {checked} empty/cold/warm/short/dead/widened/banked helper-cost checks")
