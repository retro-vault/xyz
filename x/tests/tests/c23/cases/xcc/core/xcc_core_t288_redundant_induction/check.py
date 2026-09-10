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
            [compiler, "-" + profile, "--sdcccall", str(abi), "--dump-ir", "-S",
             "-DINDUCTION_CORE_ONLY", source, "-o", str(output / f"{profile}-{abi}.s")],
            text=True, capture_output=True, check=True,
        )
        (output / f"{profile}-{abi}.ir").write_text(result.stdout)
        functions = dict(re.findall(r"; === function (\w+) ===\n(.*?)(?=; === function|$)",
                                    result.stdout, re.S))
        for name in ("stride_two", "stride_seven", "stride_thirteen"):
            body = functions[name]
            assert not re.search(r"\bi\(ix", body), body
            assert re.search(r"\bNE #\d+", body), body
        # A word recurrence that crosses 65535 cannot replace the control.
        assert re.search(r"\bLT #9\b", functions["wrapping_word"]), functions["wrapping_word"]
print("PASS: four profile/ABI combinations remove private control counters and retain wrapping recurrences")
