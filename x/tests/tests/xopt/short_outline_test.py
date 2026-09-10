#!/usr/bin/env python3
"""Check the size break-even point and observable behavior of short outlines."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile


def main():
    optimizer = pathlib.Path(sys.argv[1]).resolve()
    if optimizer.name == "xcc":
        optimizer = optimizer.with_name("xopt")
    assembly_only = "--assembly-only" in sys.argv[2:]
    prefix = optimizer.parent.parent
    repository = next(p for p in pathlib.Path(__file__).resolve().parents
                      if (p / "x/lib/xz80/include/xz80/xz80.h").exists())
    with tempfile.TemporaryDirectory() as temporary:
        work = pathlib.Path(temporary)
        probe = work / "probe"
        if not assembly_only:
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2",
                            "-I" + str(repository / "x/lib/xz80/include"),
                            str(pathlib.Path(__file__).with_name("short_outline_probe.cpp")),
                            str(prefix / "lib/libxz80.a"), "-o", str(probe)], check=True)
        for count in (2, 3):
            source = work / "input.s"
            source.write_text("\t.area _CODE\n\t.globl _entry\n_entry:\n" + "".join(
                f"\tld l, 0(ix)\n\tld h, 1(ix)\n\tcall _consume{i}\n"
                for i in range(count)) + "\tret\n")
            for profile in ("-Os", "-Of", "-O3"):
                output = work / "output.s"
                subprocess.run([str(optimizer), profile, str(source), "-o", str(output)],
                               check=True)
                assembly = output.read_text()
                outlined = "__xopt_outline_" in assembly
                assert outlined == (count == 3 and profile == "-Os"), (count, profile, assembly)
                if assembly_only:
                    continue
                # Define callees only after optimization, so their memory and
                # register effects are unknown to the optimizer under test.
                output.write_text(assembly + "".join(
                    f"\t.globl _consume{i}\n_consume{i}:\n\tret\n" for i in range(count)))
                obj, binary, mapping = [work / name for name in ("out.rel", "out.bin", "out.map")]
                subprocess.run([str(prefix / "bin/xas"), str(output), "-o", str(obj)], check=True)
                subprocess.run([str(prefix / "bin/xld"), "-nostdlib", "--no-default-runtime",
                                "--oformat=binary", "--section-start=_CODE=0x100",
                                "--binary-range=0-0xffff", "-e", "_entry",
                                "-Map=" + str(mapping), str(obj), "-o", str(binary)], check=True)
                symbols = {m[2]: int(m[1], 16) for m in re.finditer(
                    r"^([0-9A-Fa-f]{8}) (\S+)", mapping.read_text(), re.MULTILINE)}
                subprocess.run([str(probe), str(binary), str(symbols["_entry"])] +
                               [str(symbols[f"_consume{i}"]) for i in range(count)], check=True)

        # Register arguments are spilled below SP before the frame allocation.
        # Outlining those stores would overwrite the outline's return address.
        # Once SP has moved below the complete frame, repeated reads are safe.
        for interior_label in (False, True):
            source = work / "inline-frame.s"
            chunks = []
            for function in range(3):
                chunks.append(
                    f"\t.area _CODE\n\t.globl _entry{function}\n_entry{function}:\n"
                    "; sdcccall(1) prologue: frame (locals=0, temp_frame=4, stack_params=0)\n"
                    "\tpush ix\n\tld ix,#0\n" +
                    (f"_interior{function}:\n" if interior_label else "") +
                    "\tadd ix,sp\n\tld -2(ix),e\n\tld -1(ix),d\n"
                    "\tld hl,#-4\n\tadd hl,sp\n\tld sp,hl\n" + "".join(
                        f"\tld l,-2(ix)\n\tld h,-1(ix)\n\tcall _consume{function * 3 + i}\n"
                        for i in range(3)) +
                    "\tld sp,ix\n\tpop ix\n\tret\n")
            source.write_text("".join(chunks))
            output = work / "inline-frame-output.s"
            subprocess.run([str(optimizer), "-Os", str(source), "-o", str(output)], check=True)
            assembly = output.read_text()
            assert ("__xopt_outline_" in assembly) != interior_label, assembly
            for function in range(3):
                prefix_body = re.split(
                    r"\bld\s+sp\s*,\s*hl", assembly.split(f"_entry{function}:", 1)[1],
                    maxsplit=1)[0]
                assert "__xopt_outline_" not in prefix_body, assembly
            if assembly_only or interior_label:
                continue
            output.write_text(assembly + "".join(
                f"\t.globl _consume{i}\n_consume{i}:\n\tret\n" for i in range(9)))
            obj, binary, mapping = [work / name for name in ("frame.rel", "frame.bin", "frame.map")]
            subprocess.run([str(prefix / "bin/xas"), str(output), "-o", str(obj)], check=True)
            subprocess.run([str(prefix / "bin/xld"), "-nostdlib", "--no-default-runtime",
                            "--oformat=binary", "--section-start=_CODE=0x100",
                            "--binary-range=0-0xffff", "-e", "_entry0",
                            "-Map=" + str(mapping), str(obj), "-o", str(binary)], check=True)
            symbols = {m[2]: int(m[1], 16) for m in re.finditer(
                r"^([0-9A-Fa-f]{8}) (\S+)", mapping.read_text(), re.MULTILINE)}
            for function in range(3):
                subprocess.run([str(probe), str(binary), str(symbols[f"_entry{function}"]),
                                "--frame"] +
                               [str(symbols[f"_consume{function * 3 + i}"])
                                for i in range(3)], check=True)


if __name__ == "__main__":
    main()
