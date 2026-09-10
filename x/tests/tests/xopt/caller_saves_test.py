#!/usr/bin/env python3
"""Exercise final caller-save proofs through the complete optimizer pipeline."""

import pathlib
import subprocess
import sys
import tempfile


def check(optimizer, directory, name, body, preserved, marked=True):
    marker = lambda pair: f" ; xcc-caller-save:{pair}" if marked else ""
    source = "\t.area _CODE\n_caller:\n"
    source += "\tpush iy" + marker("iy") + "\n"
    source += "\tpush bc" + marker("bc") + "\n"
    source += "\t.globl _callee\n\tcall _callee\n"
    source += "\tpop bc" + marker("bc") + "\n"
    source += "\tpop iy" + marker("iy") + "\n"
    source += "\tld (49152), bc\n\tld (49154), iy\n\tret\n"
    source += "_callee:\n" + body
    path = directory / f"{name}.s"
    path.write_text(source)
    for profile in ("-Os", "-Of"):
        output = directory / f"{name}{profile}.s"
        subprocess.run([optimizer, profile, str(path), "-o", str(output)],
                       check=True, stdout=subprocess.DEVNULL)
        assembly = output.read_text()
        for pair in ("bc", "iy"):
            if marked:
                actual = assembly.count(f"xcc-caller-save:{pair}")
                expected = 0 if pair in preserved else 2
            else:
                actual = assembly.count(f"push\t{pair}")
                expected = 1
            assert actual == expected, (name, profile, pair, actual,
                                        expected, assembly)


def main():
    optimizer = str(pathlib.Path(sys.argv[1]).resolve())
    cases = [
        ("leaf_reads", "\tld a, b\n\tld d, c\n\tld e, iyl\n\tret\n",
         {"bc", "iy"}),
        ("transitive", "\tcall _leaf\n\tret\n_leaf:\n\tinc hl\n\tret\n",
         {"bc", "iy"}),
        ("bc_write", "\tinc bc\n\tld (49156), bc\n\tret\n", {"iy"}),
        ("subregister", "\tld iyh, a\n\tld (49156), iy\n\tret\n", {"bc"}),
        ("branch", "\tor a\n\tjr z, _leaf\n\tinc bc\n"
         "\tld (49156), bc\n_leaf:\n\tret\n", {"iy"}),
        ("block", "\tldir\n\tret\n", {"iy"}),
        ("counted_loop", "_loop:\n\tld (hl), a\n\tinc hl\n"
         "\tdjnz _loop\n\tret\n", {"iy"}),
        ("unknown_call", "\tcall _external\n\tret\n", set()),
        ("unknown_tail", "\tjp _external\n", set()),
        ("indirect_tail", "\tjp (hl)\n", set()),
        ("opaque_asm", "\t; xcc-opaque-asm\n\tret\n", set()),
        ("data", "\t.db 201\n", set()),
        ("recursive", "\tor a\n\tret z\n\tdec a\n"
         "\tcall _callee\n\tret\n", {"bc", "iy"}),
        ("recursive_clobber", "\tor a\n\tjr z, _leaf\n\tdec a\n"
         "\tcall _callee\n\tret\n_leaf:\n\tinc bc\n"
         "\tld (49156), bc\n\tret\n", {"iy"}),
        ("duplicate_target", "\tret\n_callee:\n\tret\n", set()),
        ("alternate_bank", "\texx\n\tld (49156), bc\n\tret\n", {"iy"}),
    ]
    with tempfile.TemporaryDirectory() as temporary:
        directory = pathlib.Path(temporary)
        for name, body, preserved in cases:
            check(optimizer, directory, name, body, preserved)
        # Ordinary pushes may be arguments or stack cleanup even when the
        # callee happens to preserve both register pairs.
        check(optimizer, directory, "unmarked_arguments",
              "\tld hl, 2\n\tadd hl, sp\n\tld a, (hl)\n\tret\n",
              set(), marked=False)
    print("xopt caller-save proofs: ok")


if __name__ == "__main__":
    main()
