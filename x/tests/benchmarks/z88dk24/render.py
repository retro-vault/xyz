#!/usr/bin/env python3
import csv
import sys

src, dst = sys.argv[1:]
with open(src, newline="", encoding="utf-8") as f:
    rows = list(csv.DictReader(f))

modes = [
    "xcc_Os",
    "xcc_Of",
    "sdcc",
    "80cc_fp",
    "80cc_sp",
]
labels = [
    "xcc -Os",
    "xcc -Of",
    "sdcc",
    "80cc-fp",
    "80cc-sp",
]
xcc_modes = modes[:2]
xcc_labels = labels[:2]
competitor_modes = modes[2:]

def cell(row, mode):
    status = row[f"{mode}_status"]
    size = int(row[f"{mode}_bytes"])
    if status == "BUILD" or size <= 0:
        return "build-fail"
    cycles = int(row[f"{mode}_cycles"])
    ticks = "?" if cycles <= 0 else f"{cycles / 1_000_000:.1f}M"
    suffix = f"{size}B / {ticks}"
    if status == "FAIL":
        return suffix + " FAIL"
    if status == "ERROR":
        return suffix + " ERR"
    return suffix

def strict_wins(mode, metric):
    comparable = [
        row for row in rows
        if row[f"{mode}_status"] == "OK"
        and row["sdcc_status"] == "OK"
    ]
    wins = sum(
        int(row[f"{mode}_{metric}"]) < int(row[f"sdcc_{metric}"])
        for row in comparable
    )
    return wins, len(comparable)

def competitive_counts(mode, metric):
    comparable = []
    for row in rows:
        if row[f"{mode}_status"] != "OK":
            continue
        competitor_values = [
            int(row[f"{competitor}_{metric}"])
            for competitor in competitor_modes
            if row[f"{competitor}_status"] == "OK"
            and int(row[f"{competitor}_{metric}"]) > 0
        ]
        if competitor_values:
            comparable.append(
                (int(row[f"{mode}_{metric}"]), min(competitor_values))
            )
    strict = sum(value < best for value, best in comparable)
    within_five = sum(value <= best * 1.05 for value, best in comparable)
    return strict, within_five, len(comparable)

lines = [
    "# z88dk Full-Program Integer Benchmarks",
    "",
    "Every compiler uses z88dk's `+test` CRT and classic library, so complete",
    "linked-image sizes are directly comparable. Every size is measured with",
    "`wc -c` from the benchmark work dir. Every image executes in",
    "the same `z80_exec` Z80 model; its z88dk test-CRT trap support handles",
    "competitor console and file I/O without changing target instruction",
    "counts. Each table cell is `sizeB / cycles`.",
    "",
    "## Summary",
    "",
    "A win means a strictly smaller complete binary or a strictly lower cycle",
    "count than upstream SDCC; ties are not counted as wins.",
    "",
    "| XCC lane | Correct | Size wins vs SDCC | Speed wins vs SDCC |",
    "|---|---:|---:|---:|",
]
for mode, label in zip(xcc_modes, xcc_labels):
    passed = sum(row[f"{mode}_status"] == "OK" for row in rows)
    size_wins, size_total = strict_wins(mode, "bytes")
    speed_wins, speed_total = strict_wins(mode, "cycles")
    lines.append(
        f"| {label} | {passed}/{len(rows)} | "
        f"{size_wins}/{size_total} | {speed_wins}/{speed_total} |"
    )

lines += [
    "",
    "The tables report measured outcomes without assuming that either XCC",
    "profile wins; size and speed claims must follow the complete current run.",
    "",
    "### Best-Competitor Envelope",
    "",
    "The best competitor is the smallest or fastest successful non-XCC lane",
    "for each program. Strict-best ties are not counted as wins.",
    "",
    "| XCC lane | Size strict best | Size within 5% | Speed strict best | Speed within 5% |",
    "|---|---:|---:|---:|---:|",
]
for mode, label in zip(xcc_modes, xcc_labels):
    size_best, size_five, size_total = competitive_counts(mode, "bytes")
    speed_best, speed_five, speed_total = competitive_counts(mode, "cycles")
    lines.append(
        f"| {label} | {size_best}/{size_total} | {size_five}/{size_total} | "
        f"{speed_best}/{speed_total} | {speed_five}/{speed_total} |"
    )

lines += [
    "",
    "## Full Results",
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

lines += [
    "",
    "## Outputs",
    "",
    "- [results.csv](results.csv)",
    "- [artifacts/](artifacts/)",
]

with open(dst, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
