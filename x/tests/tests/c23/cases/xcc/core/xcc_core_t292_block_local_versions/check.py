import collections
import pathlib
import re
import subprocess
import sys

compiler, source, output = sys.argv[1:]
output = pathlib.Path(output)
output.mkdir(parents=True, exist_ok=True)
pressure_source = output / "pressure.c"
pressure_source.write_text("""extern unsigned external_value;
unsigned pressure_symbol(unsigned input) {
    unsigned state = input + 1;
    state ^= external_value;
    state ^= state >> 3;
    return state;
}
unsigned pressure_parameter(unsigned input, unsigned other) {
    unsigned state = input + 1;
    state ^= other;
    state ^= state >> 3;
    return state;
}
unsigned consumes_completed(unsigned input) {
    unsigned state = input + 1;
    state ^= state >> 3;
    state += 7;
    return state + external_value;
}
""")

for abi in (0, 1):
    for profile in ("Os", "Of"):
        result = subprocess.run(
            [compiler, "-" + profile, "--sdcccall", str(abi), "--dump-ir", "-S",
             source, "-o", str(output / f"{profile}-{abi}.s")],
            text=True, capture_output=True, check=True,
        )
        (output / f"{profile}-{abi}.ir").write_text(result.stdout)
        functions = dict(re.findall(r"; === function (\w+) ===\n(.*?)(?=; === function|$)",
                                    result.stdout, re.S))
        for name in ("version_word", "version_byte"):
            body = functions[name]
            assert "state(ix" not in body, body
            definitions = collections.Counter(re.findall(r"^  (t\d+) =", body, re.M))
            assert all(count == 1 for count in definitions.values()), body
        assert "state(ix" in functions["volatile_local"], functions["volatile_local"]
        assert "&state(ix" in functions["escaped_word"], functions["escaped_word"]
        for name in ("loop_word", "partial_word", "branch_word"):
            body = functions[name]
            incoming = re.search(r"^  (t\d+) = recv\(0", body, re.M)
            # ABI1 can coalesce the unchanged incoming word and its local
            # copy before this pass. That remains one mutable home, with
            # complete and partial definitions referring to the same temp.
            shared_incoming = incoming and len(re.findall(
                rf"^  {incoming[1]}(?:\+\d+)? =", body, re.M)) >= 2
            assert "state(ix" in body or shared_incoming, body
        assert "state(ix" not in functions["block_each_visit"], functions["block_each_visit"]

        pressure = subprocess.run(
            [compiler, "-" + profile, "--sdcccall", str(abi), "--dump-ir", "-S",
             str(pressure_source), "-o", str(output / f"pressure-{profile}-{abi}.s")],
            text=True, capture_output=True, check=True,
        )
        (output / f"pressure-{profile}-{abi}.ir").write_text(pressure.stdout)
        pressure_functions = dict(re.findall(
            r"; === function (\w+) ===\n(.*?)(?=; === function|$)",
            pressure.stdout, re.S))
        for name in ("pressure_symbol", "pressure_parameter"):
            body = pressure_functions[name]
            definitions = collections.Counter(re.findall(r"^  (t\d+) =", body, re.M))
            assert ("state(ix" in body or
                    any(count > 1 for count in definitions.values())), body
        completed = pressure_functions["consumes_completed"]
        assert "state(ix" not in completed, completed
print("PASS: four profile/ABI combinations produce unique values and retain observable homes")
