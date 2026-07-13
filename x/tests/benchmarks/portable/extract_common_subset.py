#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from pathlib import Path


MODE_ORDER = [
    ("xcc -Os", "xcc_Os"),
    ("sdcc size", "sdcc_size"),
    ("sdcc speed", "sdcc_speed"),
    ("z88dk sdcc", "z88dk_sdcc"),
    ("z88dk 80cc", "z88dk_80cc"),
    ("z88dk sccz80", "z88dk_sccz80"),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", required=True, type=Path)
    parser.add_argument("--outdir", required=True, type=Path)
    parser.add_argument("--versions", type=Path)
    return parser.parse_args()


def mode_ok(row: dict[str, str], prefix: str) -> bool:
    return row[f"{prefix}_status"] == "ok" and row[f"{prefix}_match"] == "ok"


def family_of(benchmark: str) -> str:
    return benchmark.rsplit("_", 1)[0]


def main() -> None:
    args = parse_args()
    args.outdir.mkdir(parents=True, exist_ok=True)

    with args.results.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
        fieldnames = list(rows[0].keys()) if rows else []

    common_rows = [
        row for row in rows
        if all(mode_ok(row, prefix) for _, prefix in MODE_ORDER)
    ]

    common_rows.sort(key=lambda row: row["benchmark"])

    common_csv = args.outdir / "results_common.csv"
    with common_csv.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(common_rows)

    common_list = args.outdir / "common_benchmarks.txt"
    common_list.write_text(
        "".join(f"{row['benchmark']}\n" for row in common_rows),
        encoding="utf-8",
    )

    if args.versions is not None and args.versions.exists():
        (args.outdir / "versions.txt").write_text(
            args.versions.read_text(encoding="utf-8"),
            encoding="utf-8",
        )

    family_counts: dict[str, int] = {}
    for row in common_rows:
        family = family_of(row["benchmark"])
        family_counts[family] = family_counts.get(family, 0) + 1

    summary_lines = [
        "# Portable Common-Pass Subset",
        "",
        f"Input rows: {len(rows)}",
        f"Common-pass rows: {len(common_rows)}",
        "",
        "## Families",
        "",
    ]
    if family_counts:
        for family in sorted(family_counts):
            summary_lines.append(f"- `{family}`: {family_counts[family]}")
    else:
        summary_lines.append("- none")
    summary_lines.extend([
        "",
        "## Outputs",
        "",
        f"- [results_common.csv]({common_csv.name})",
        f"- [common_benchmarks.txt]({common_list.name})",
    ])

    (args.outdir / "common_summary.md").write_text(
        "\n".join(summary_lines),
        encoding="utf-8",
    )

    print(f"common-pass rows: {len(common_rows)}")
    print(common_csv)


if __name__ == "__main__":
    main()
