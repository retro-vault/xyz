#!/usr/bin/env python3
"""Keep source-object reads while optimizing proven private spill slots."""

import pathlib
import re
import subprocess
import sys
import tempfile


cases = {
    "word": (2, 2, """
    ld -2(ix), l
    ld -1(ix), h
    ld l, -2(ix)
    ld h, -1(ix)
    ld (49152), hl
    ret
"""),
    "word32_low": (4, 2, """
    ld -4(ix), l
    ld -3(ix), h
    ld -2(ix), e
    ld -1(ix), d
    ld l, -4(ix)
    ld h, -3(ix)
    ld (49152), hl
    ret
"""),
    "word_zero_test": (2, 2, """
    ld -2(ix), l
    ld -1(ix), h
    inc bc
    ld l, -2(ix)
    ld h, -1(ix)
    ld a, h
    or l
    jr z, done
    ld (49152), hl
done:
    ld (49154), bc
    ret
"""),
    "byte": (1, 1, """
    ld -1(ix), a
    ld a, -1(ix)
    ld (49152), a
    ret
"""),
    "byte_forward": (1, 1, """
    ld -1(ix), a
    ld b, -1(ix)
    ld a, b
    ld (49152), a
    ret
"""),
    "word_to_byte": (2, 2, """
    ld l, -2(ix)
    ld h, -1(ix)
    ld a, l
    ld hl, #0
    ld (49152), a
    ret
"""),
    "dead_word": (2, 2, """
    ld l, -2(ix)
    ld h, -1(ix)
    ld l, 4(ix)
    ld h, 5(ix)
    ld (49152), hl
    ret
"""),
}
read = re.compile(r"^\s*ld\s+[abcdehl],\s*-\d+\(ix\)\s*$")


def word_load_values(assembly):
    """Interpret the small load/exchange fixture, preserving reads as events."""
    registers = dict.fromkeys("abcdehl", 0)
    memory = {-4: 0x34, -3: 0x12, -2: 0xcd, -1: 0xab}
    reads = []
    stores = {}
    for raw in assembly.splitlines():
        line = raw.split(";", 1)[0].strip()
        if not line or line.startswith(".") or line.endswith(":"):
            continue
        if line == "ret":
            break
        mnemonic, operands = line.split(None, 1)
        destination, source = map(str.strip, operands.split(","))
        if mnemonic == "ex":
            assert (destination, source) == ("de", "hl")
            registers["d"], registers["h"] = registers["h"], registers["d"]
            registers["e"], registers["l"] = registers["l"], registers["e"]
            continue
        assert mnemonic == "ld", line
        indexed = re.fullmatch(r"(-?\d+)\s*\(ix\)", source)
        if indexed:
            offset = int(indexed[1])
            reads.append(offset)
            registers[destination] = memory[offset]
        elif destination.startswith("("):
            stores[int(destination[1:-1])] = (
                registers[source[0]] * 256 + registers[source[1]])
        else:
            registers[destination] = registers[source]
    return reads, stores


def main():
    optimizer = str(pathlib.Path(sys.argv[1]).resolve())
    checks = 0
    with tempfile.TemporaryDirectory() as temporary:
        directory = pathlib.Path(temporary)
        for profile in ("-O2", "-Of", "-Os"):
            for name, (size, accesses, body) in cases.items():
                for private in (False, True):
                    locals_, temps = (0, size) if private else (size, 0)
                    source = directory / f"{name}-{private}.s"
                    source.write_text(
                        "\t.area _CODE\n_test:\n"
                        f"\t; sdcccall(1) prologue: test (locals={locals_}, "
                        f"temp_frame={temps}, stack_params=0)\n" + body
                    )
                    output = directory / "out.s"
                    subprocess.run([optimizer, profile, str(source), "-o",
                                    str(output)], check=True)
                    assembly = output.read_text()
                    count = sum(bool(read.fullmatch(line.split(";", 1)[0]))
                                for line in assembly.splitlines())
                    expected = (1 if name == "word_to_byte" else 0) \
                        if private else accesses
                    assert count == expected, (name, private, profile,
                                               count, expected, assembly)
                    checks += 1
            # An IY cursor has no canonical private-frame proof. Narrowing
            # this load would lose half of an observable word access.
            source = directory / "iy-word.s"
            source.write_text(
                "\t.area _CODE\n_iy_word:\n" +
                cases["word_to_byte"][2].replace("-2(ix)", "0(iy)")
                                           .replace("-1(ix)", "1(iy)")
            )
            output = directory / "iy-word-out.s"
            subprocess.run([optimizer, profile, str(source), "-o",
                            str(output)], check=True)
            assembly = output.read_text()
            count = len(re.findall(
                r"^\s*ld\s+[abcdehl],\s*[01]\(iy\)\s*$",
                assembly, re.MULTILINE))
            assert count == 2, ("iy_word", profile, count, assembly)
            checks += 1
            source = directory / "word-load-order.s"
            source.write_text("""
    .area _CODE
_word_load_order:
    ld l, -4(ix)
    ld h, -3(ix)
    ex de, hl
    ld l, -2(ix)
    ld h, -1(ix)
    ex de, hl
    ld (49152), hl
    ld (49154), de
    ret
""")
            output = directory / "word-load-order-out.s"
            subprocess.run([optimizer, profile, str(source), "-o",
                            str(output)], check=True)
            assembly = output.read_text()
            reads, stores = word_load_values(assembly)
            assert reads == [-4, -3, -2, -1], (profile, reads, assembly)
            assert stores == {49152: 0x1234, 49154: 0xabcd}, (
                profile, stores, assembly)
            assert not re.search(r"^\s*ex\s", assembly, re.MULTILINE), assembly
            checks += 1
    with tempfile.TemporaryDirectory() as temporary:
        directory = pathlib.Path(temporary)
        for profile in ("-O2", "-Of", "-Os"):
            for word, addition in ((True, False), (True, True), (False, False)):
                for proof in ("unknown", "private", "ordinary", "previous_function", "naked", "interrupt", "critical"):
                    offset = -4 if word else -1
                    metadata = f"; sdcccall(1) prologue: check (locals=8, temp_frame=0, stack_params=0, return_regs={24 if word else 1})\n"
                    if proof == "private":
                        metadata = metadata.replace("locals=8, temp_frame=0", "locals=0, temp_frame=8")
                    ordinary = f"; xopt ordinary ix span: offset={offset} size={2 if word else 1}\n"
                    before = ""
                    if proof == "ordinary":
                        metadata += ordinary
                    elif proof in ("previous_function", "naked", "interrupt", "critical"):
                        before = "_previous:\n" + metadata + ordinary + "ret\n"
                        if proof == "naked":
                            metadata = "; naked: check\n"
                        elif proof != "previous_function":
                            metadata = "; " + proof + " prologue: check\n"
                    if word:
                        operation = "ld de, #1\nadd hl, de" if addition else "inc hl"
                        body = ("ld l, -4(ix)\nld h, -3(ix)\n" + operation +
                                "\nld -4(ix), l\nld -3(ix), h\nld de, #7\nxor a\nret\n")
                    else:
                        body = "ld a, -1(ix)\nadd a, #1\nld -1(ix), a\nor a\njr nz, done\nld a, #0\ndone:\nret\n"
                    source = directory / "access.s"
                    source.write_text(".area _CODE\n" + before + "_check:\n" + metadata + body)
                    output = directory / "out.s"
                    subprocess.run([optimizer, profile, str(source), "-o", str(output)], check=True)
                    assembly = output.read_text().split("_check:", 1)[1]
                    conditional = bool(re.search(r"inc\s+" + str(offset) + r"\(ix\)", assembly))
                    assert conditional == (proof in ("private", "ordinary")), (profile, word, addition, proof, assembly)
                    checks += 1
    print(f"xopt private-spill/observable-read proofs: {checks} passed")


if __name__ == "__main__":
    main()
