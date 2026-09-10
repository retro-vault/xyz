import pathlib
import re
import subprocess
import sys

compiler, source, output = sys.argv[1:]
output = pathlib.Path(output)
output.mkdir(parents=True, exist_ok=True)
for abi in (0, 1):
    for profile in ("Os", "Of"):
        result = subprocess.run(
            [compiler, "-" + profile, "--sdcccall", str(abi), "--dump-ir",
             "-S", "-DADDRESS_CORE_ONLY", source,
             "-o", str(output / f"{profile}-abi{abi}.s")],
            text=True, capture_output=True, check=True,
        )
        (output / f"{profile}-abi{abi}.ir").write_text(result.stdout)
        body = result.stdout.split("; === function append ===", 1)[1]
        assert not re.search(r"=\s*&objects(?:\s|$)", body), body
        assert re.search(r"(?:objects ADD #2\b|&objects\+2\b)", body), body
        assert re.search(r"(?:objects ADD #76\b|&objects\+76\b)", body), body
print("PASS: four size/speed and ABI combinations fold both immutable member offsets")
