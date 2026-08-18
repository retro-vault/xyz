#!/usr/bin/env python3
import csv
import sys


src, dst, target_path = sys.argv[1:]
with open(src, newline="", encoding="utf-8") as f:
    rows = list(csv.DictReader(f))
with open(target_path, newline="", encoding="utf-8") as f:
    target_rows = {row["benchmark"]: row for row in csv.DictReader(f)}

modes = [
    "sccz80",
    "xcc_Os",
    "xcc_Of",
    "sdcc",
    "sdcc_max",
    "80cc_fp",
    "80cc_sp",
]
labels = [
    "sccz80",
    "xcc -Os",
    "xcc -Of",
    "sdcc",
    "sdcc-max",
    "80cc-fp",
    "80cc-sp",
]
xcc_modes = ["xcc_Os", "xcc_Of"]
xcc_labels = ["xcc -Os", "xcc -Of"]
primary_competitors = ["sdcc", "80cc_fp", "80cc_sp"]


def cell(row, mode):
    status = row[f"{mode}_status"]
    size = int(row[f"{mode}_bytes"])
    if status == "SKIP":
        return "-"
    if status == "BUILD" or size <= 0:
        return "build-fail"
    cycles = int(row[f"{mode}_cycles"])
    ticks = "?" if cycles <= 0 else f"{cycles / 1_000_000:.1f}M"
    value = f"{size}B / {ticks}"
    if status == "FAIL":
        return value + " FAIL"
    if status == "ERROR":
        return value + " ERR"
    return value


def strict_wins(mode, metric):
    comparable = [
        row for row in rows
        if row[f"{mode}_status"] == "OK" and row["sdcc_status"] == "OK"
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
        values = [
            int(row[f"{competitor}_{metric}"])
            for competitor in primary_competitors
            if row[f"{competitor}_status"] == "OK"
            and int(row[f"{competitor}_{metric}"]) > 0
        ]
        if values:
            comparable.append((int(row[f"{mode}_{metric}"]), min(values)))
    strict = sum(value < best for value, best in comparable)
    within_five = sum(value <= best * 1.05 for value, best in comparable)
    return strict, within_five, len(comparable)


def target_matches(mode):
    size_exact = cycle_exact = size_total = cycle_total = 0
    for row in rows:
        target = target_rows.get(row["benchmark"])
        if target is None:
            continue
        status_ok = row[f"{mode}_status"] == "OK"
        target_size = target.get(f"{mode}_bytes", "")
        target_cycles = target.get(f"{mode}_cycles_m", "")
        if target_size:
            size_total += 1
            size_exact += status_ok and int(row[f"{mode}_bytes"]) == int(target_size)
        if target_cycles:
            cycle_total += 1
            measured = round(int(row[f"{mode}_cycles"]) / 1_000_000, 1)
            cycle_exact += status_ok and measured == float(target_cycles)
    return size_exact, size_total, cycle_exact, cycle_total


lines = [
    "# Locked z88dk Full-Program Integer Benchmarks",
    "",
    "Every lane uses the pinned z88dk headers, `+test` CRT and classic library.",
    "XCC is the M distribution. Current zsdcc and 80cc executables are injected",
    "without changing that target-library baseline. Images execute with upstream",
    "`z88dk-ticks -b msx`; each cell is complete linked bytes / cycles.",
    "",
    "## Summary",
    "",
    "| XCC lane | Correct | Size wins vs SDCC | Speed wins vs SDCC |",
    "|---|---:|---:|---:|",
]
for mode, label in zip(xcc_modes, xcc_labels):
    passed = sum(row[f"{mode}_status"] == "OK" for row in rows)
    attempted = sum(row[f"{mode}_status"] != "SKIP" for row in rows)
    size_wins, size_total = strict_wins(mode, "bytes")
    speed_wins, speed_total = strict_wins(mode, "cycles")
    lines.append(
        f"| {label} | {passed}/{attempted} | "
        f"{size_wins}/{size_total} | {speed_wins}/{speed_total} |"
    )

lines += [
    "",
    "### Best primary-competitor envelope",
    "",
    "The envelope contains SDCC, 80cc-fp and 80cc-sp. sccz80 is retained as",
    "a historical control and sdcc-max is a limited expensive-allocation probe.",
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
    "### Match to supplied target table",
    "",
    "Cycle matching uses the published one-decimal-million rounding. Blank target",
    "cells are not counted; a failed execution cannot count as an exact match.",
    "",
    "| lane | Exact bytes | Exact rounded cycles |",
    "|---|---:|---:|",
]
for mode, label in zip(modes, labels):
    size_exact, size_total, cycle_exact, cycle_total = target_matches(mode)
    lines.append(
        f"| {label} | {size_exact}/{size_total} | {cycle_exact}/{cycle_total} |"
    )

lines += [
    "",
    "## Full results",
    "",
    "| bench | " + " | ".join(labels) + " |",
    "|---|" + "---:|" * len(labels),
]
for row in rows:
    lines.append(
        "| " + row["benchmark"] + " | "
        + " | ".join(cell(row, mode) for mode in modes) + " |"
    )

lines += ["", "## Correctness", ""]
for mode, label in zip(modes, labels):
    passed = sum(row[f"{mode}_status"] == "OK" for row in rows)
    attempted = sum(row[f"{mode}_status"] != "SKIP" for row in rows)
    lines.append(f"- {label}: {passed}/{attempted}")

lines += [
    "",
    "## Outputs",
    "",
    "- [results.csv](results.csv)",
    "- [versions.txt](versions.txt)",
    "- [artifacts/](artifacts/)",
]

with open(dst, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
