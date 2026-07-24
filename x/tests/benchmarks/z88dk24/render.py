#!/usr/bin/env python3
import csv
import sys

src, dst = sys.argv[1:]
with open(src, newline="", encoding="utf-8") as f:
    rows = list(csv.DictReader(f))

modes = [
    "xcc_Os",
    "xcc_Os_sdcc0",
    "xcc_Of",
    "xcc_Of_sdcc0",
    "sccz80",
    "sdcc",
    "80cc_fp",
    "80cc_sp",
]
labels = [
    "xcc -Os (M, sdcc1)",
    "xcc -Os (M, sdcc0)",
    "xcc -Of (M, sdcc1)",
    "xcc -Of (M, sdcc0)",
    "sccz80",
    "sdcc",
    "80cc-fp",
    "80cc-sp",
]

def cell(row, mode):
    status = row[f"{mode}_status"]
    if status != "OK":
        return status
    cycles = int(row[f"{mode}_cycles"])
    size = int(row[f"{mode}_bytes"])
    return f"{cycles / 1_000_000:.1f}M/{size}"

lines = [
    "# z88dk Full-Program Integer Benchmarks",
    "",
    "XCC uses the M model. The `sdcc1` rows keep the current default ABI and",
    "the `sdcc0` rows force `--sdcccall 0`. Every size is the complete linked",
    "binary; every row runs the original z88dk test framework and each",
    "compiler's own CRT/libc.",
    "",
    "| bench | " + " | ".join(labels) + " |",
    "|---|" + "---:|" * len(labels),
]
for row in rows:
    lines.append("| " + row["benchmark"] + " | " + " | ".join(cell(row, m) for m in modes) + " |")

lines += ["", "## Correctness", ""]
for mode, label in zip(modes, labels):
    passed = sum(r[f"{mode}_status"] == "OK" for r in rows)
    lines.append(f"- {label}: {passed}/{len(rows)}")

with open(dst, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
