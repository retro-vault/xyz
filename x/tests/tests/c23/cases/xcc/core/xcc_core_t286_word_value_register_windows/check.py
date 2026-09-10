import pathlib
import re
import subprocess
import sys

xcc, source, work = sys.argv[1:]
work = pathlib.Path(work)
work.mkdir(parents=True, exist_ok=True)

def body(text, name):
    return text.split("_" + name + ":\n", 1)[1].split("\n\t.area", 1)[0]

for abi in (0, 1):
    for profile in ("Os", "Of", "O3"):
        assembly = work / f"windows-{profile}-abi{abi}.s"
        subprocess.run([xcc, "-S", "-" + profile, "--sdcccall", str(abi),
                        source, "-o", str(assembly)], check=True)
        text = assembly.read_text()
        words = body(text, "accumulate_words")
        assert re.search(r"ld\s+b,\s*d\s+ld\s+c,\s*e", words), \
            f"{profile}/ABI{abi}: first word was spilled across the other address"
        product = re.search(r"call\s+__mul16(?P<uses>.*?)add\s+hl,\s*(?:bc|de)",
                            words, re.S)
        assert product and not re.search(r"ld\s+-?\d+\s*\(ix\)\s*,",
                                         product["uses"]), \
            f"{profile}/ABI{abi}: product was spilled before its sole consumer"
        decision = body(text, "capture_decision")
        assert len(re.findall(r"ld\s+hl,\s*\([^\n]*decision_input[^\n]*\)",
                              decision)) == 1, \
            f"{profile}/ABI{abi}: source must be captured exactly once"
        assert re.search(r"ld\s+b,\s*h\s+ld\s+c,\s*l", decision), \
            f"{profile}/ABI{abi}: decision capture lacks BC home"
        assert not re.search(r"\b(?:push|pop)\s+ix\b|\(ix\)|__sdcc_enter_ix",
                             decision), \
            f"{profile}/ABI{abi}: decision chain retained an IX frame"
print("numeric and decision windows: 6 profile/ABI combinations passed")
