import pathlib
import re
import subprocess
import sys


compiler, source, output = sys.argv[1:]
output = pathlib.Path(output)
output.mkdir(parents=True, exist_ok=True)
for abi in (0, 1):
    result = subprocess.run(
        [compiler, "-Os", "--sdcccall", str(abi), "--dump-ir", "-S",
         "-DSCALE_CORE_ONLY", source, "-o", str(output / f"abi{abi}.s")],
        text=True, capture_output=True, check=True,
    )
    (output / f"abi{abi}.ir").write_text(result.stdout)
    functions = re.split(r"; === function ", result.stdout)
    checked = 0
    for function in functions:
        if not function.startswith(("dispatch_words ", "collect_pairs ")):
            continue
        assert re.search(r"\bSHL #(?:1|2)\b", function), function
        assert not re.search(r"\b(t\d+) = \1 (?:ADD|SUB) #(?:2|4)\b", function), function
        assert " = call " in function, function
        checked += 1
    assert checked == 2, result.stdout
print("PASS: 4 call-containing word/record loops retain local scales without extra recurrence state")
