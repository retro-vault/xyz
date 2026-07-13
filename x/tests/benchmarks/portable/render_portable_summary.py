#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ModeSpec:
    label: str
    prefix: str


@dataclass
class Result:
    label: str
    status: str
    ret: int
    match: str
    bytes: int
    cycles: int

    @property
    def correct(self) -> bool:
        return self.status == "ok" and self.match == "ok"

    def cell(self) -> str:
        if self.status != "ok":
            return self.status
        text = f"{self.bytes}/{self.cycles}"
        if self.match != "ok":
            text += f" ({self.match})"
        return text


MODE_SPECS = [
    ModeSpec("xcc -Of", "xcc_Of"),
    ModeSpec("xcc -O3", "xcc_O3"),
    ModeSpec("xcc -Os", "xcc_Os"),
    ModeSpec("sdcc size", "sdcc_size"),
    ModeSpec("sdcc speed", "sdcc_speed"),
    ModeSpec("z88dk sdcc", "z88dk_sdcc"),
    ModeSpec("z88dk 80cc", "z88dk_80cc"),
    ModeSpec("z88dk sccz80", "z88dk_sccz80"),
]

COMPETITOR_PREFIXES = [
    spec.prefix for spec in MODE_SPECS if not spec.prefix.startswith("xcc_")
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", required=True, type=Path)
    parser.add_argument("--summary", required=True, type=Path)
    parser.add_argument("--versions", required=True, type=Path)
    parser.add_argument("--cycle-limit", required=True, type=int)
    parser.add_argument("--bench-count", required=True, type=int)
    return parser.parse_args()


def parse_result(row: dict[str, str], spec: ModeSpec) -> Result:
    prefix = spec.prefix
    return Result(
        label=spec.label,
        status=row[f"{prefix}_status"],
        ret=int(row[f"{prefix}_return"]),
        match=row[f"{prefix}_match"],
        bytes=int(row[f"{prefix}_bytes"]),
        cycles=int(row[f"{prefix}_cycles"]),
    )


def result_totals(rows: list[dict[str, str]]) -> dict[str, dict[str, int]]:
    totals: dict[str, dict[str, int]] = {}
    for spec in MODE_SPECS:
        totals[spec.label] = {
            "seen": 0,
            "correct": 0,
            "bytes": 0,
            "cycles": 0,
        }

    for row in rows:
        for spec in MODE_SPECS:
            result = parse_result(row, spec)
            entry = totals[spec.label]
            entry["seen"] += 1
            if result.correct:
                entry["correct"] += 1
                entry["bytes"] += result.bytes
                entry["cycles"] += result.cycles
    return totals


def format_delta(numerator: int, denominator: int, positive: str, negative: str) -> str:
    if denominator == 0:
        return "n/a"
    delta = (100.0 * (denominator - numerator)) / denominator
    if delta >= 0:
        return f"{delta:.2f}% {positive}"
    return f"{-delta:.2f}% {negative}"


def best_by_size(results: list[Result]) -> Result | None:
    correct = [result for result in results if result.correct]
    if not correct:
        return None
    return min(correct, key=lambda result: (result.bytes, result.cycles, result.label))


def best_by_speed(results: list[Result]) -> Result | None:
    correct = [result for result in results if result.correct]
    if not correct:
        return None
    return min(correct, key=lambda result: (result.cycles, result.bytes, result.label))


def emit_summary(args: argparse.Namespace, rows: list[dict[str, str]]) -> str:
    totals = result_totals(rows)

    best_size_common = 0
    best_speed_common = 0
    xcc_best_size_bytes = 0
    xcc_best_size_cycles = 0
    comp_best_size_bytes = 0
    comp_best_size_cycles = 0
    xcc_best_speed_bytes = 0
    xcc_best_speed_cycles = 0
    comp_best_speed_bytes = 0
    comp_best_speed_cycles = 0
    xcc_size_wins = 0
    xcc_speed_wins = 0
    xcc_vs_fastest_speed_wins = 0
    xcc_vs_fastest_size_wins = 0

    size_violators: list[tuple[int, str, Result, Result]] = []
    speed_violators: list[tuple[int, str, Result, Result]] = []
    per_benchmark: list[tuple[str, dict[str, Result], Result | None, Result | None]] = []

    for row in rows:
        benchmark = row["benchmark"]
        results = {spec.prefix: parse_result(row, spec) for spec in MODE_SPECS}
        xcc_size = results["xcc_Os"]
        xcc_speed = results["xcc_Of"]
        competitors = [results[prefix] for prefix in COMPETITOR_PREFIXES]
        best_size = best_by_size(competitors)
        best_speed = best_by_speed(competitors)
        per_benchmark.append((benchmark, results, best_size, best_speed))

        if xcc_size.correct and best_size is not None:
            best_size_common += 1
            xcc_best_size_bytes += xcc_size.bytes
            xcc_best_size_cycles += xcc_size.cycles
            comp_best_size_bytes += best_size.bytes
            comp_best_size_cycles += best_size.cycles
            if xcc_size.bytes <= best_size.bytes:
                xcc_size_wins += 1
            if xcc_size.cycles <= best_size.cycles:
                xcc_speed_wins += 1
            if xcc_size.bytes > best_size.bytes:
                size_violators.append(
                    (xcc_size.bytes - best_size.bytes, benchmark, xcc_size, best_size)
                )

        if xcc_speed.correct and best_speed is not None:
            best_speed_common += 1
            xcc_best_speed_bytes += xcc_speed.bytes
            xcc_best_speed_cycles += xcc_speed.cycles
            comp_best_speed_bytes += best_speed.bytes
            comp_best_speed_cycles += best_speed.cycles
            if xcc_speed.cycles <= best_speed.cycles:
                xcc_vs_fastest_speed_wins += 1
            if xcc_speed.bytes <= best_speed.bytes:
                xcc_vs_fastest_size_wins += 1
            if xcc_speed.cycles > best_speed.cycles:
                speed_violators.append(
                    (xcc_speed.cycles - best_speed.cycles, benchmark, xcc_speed, best_speed)
                )

    size_violators.sort(key=lambda item: (-item[0], item[1]))
    speed_violators.sort(key=lambda item: (-item[0], item[1]))
    per_benchmark.sort(key=lambda item: item[0])

    lines: list[str] = []
    lines.append("# Portable Cross-Compiler Benchmarks")
    lines.append("")
    lines.append(f"Benchmarks: {args.bench_count}")
    lines.append(f"Cycle limit per image: {args.cycle_limit}")
    lines.append("")
    lines.append("These are generated, libc-free, self-checking C programs intended to run across the full compiler matrix.")
    lines.append("Each source returns `0` on success, so build/runtime correctness and performance stay in the same report.")
    lines.append("")
    lines.append("## Totals")
    lines.append("")
    lines.append("| Mode | Correct | Payload Bytes | Cycles |")
    lines.append("| --- | ---: | ---: | ---: |")
    for spec in MODE_SPECS:
        total = totals[spec.label]
        lines.append(
            f"| `{spec.label}` | {total['correct']}/{total['seen']} | {total['bytes']} | {total['cycles']} |"
        )
    lines.append("")
    lines.append("## Relative")
    lines.append("")
    if best_size_common:
        size_text = format_delta(
            xcc_best_size_bytes,
            comp_best_size_bytes,
            "smaller",
            "larger",
        )
        cycle_text = format_delta(
            xcc_best_size_cycles,
            comp_best_size_cycles,
            "fewer cycles",
            "more cycles",
        )
        lines.append(
            f"- `xcc -Os` vs best competing size result: {size_text}, {cycle_text} on {best_size_common} common passing benchmarks"
        )
        lines.append(
            f"- `xcc -Os` wins size on {xcc_size_wins}/{best_size_common} and speed on {xcc_speed_wins}/{best_size_common} against the best-size competitor"
        )
    else:
        lines.append("- `xcc -Os` vs best competing size result: n/a")
    if best_speed_common:
        size_text = format_delta(
            xcc_best_speed_bytes,
            comp_best_speed_bytes,
            "smaller",
            "larger",
        )
        cycle_text = format_delta(
            xcc_best_speed_cycles,
            comp_best_speed_cycles,
            "fewer cycles",
            "more cycles",
        )
        lines.append(
            f"- `xcc -Of` vs fastest competing result: {size_text}, {cycle_text} on {best_speed_common} common passing benchmarks"
        )
        lines.append(
            f"- `xcc -Of` wins speed on {xcc_vs_fastest_speed_wins}/{best_speed_common} and size on {xcc_vs_fastest_size_wins}/{best_speed_common} against the fastest competitor"
        )
    else:
        lines.append("- `xcc -Of` vs fastest competing result: n/a")
    lines.append("")
    lines.append("## Size Violators")
    lines.append("")
    lines.append("| benchmark | xcc -Os bytes/cycles | best competing size mode | best bytes/cycles | xcc byte gap |")
    lines.append("| --- | ---: | --- | ---: | ---: |")
    for gap, benchmark, xcc, best in size_violators[:20]:
        lines.append(
            f"| `{benchmark}` | {xcc.bytes}/{xcc.cycles} | `{best.label}` | {best.bytes}/{best.cycles} | +{gap} |"
        )
    if not size_violators:
        lines.append("| none | n/a | n/a | n/a | 0 |")
    lines.append("")
    lines.append("## Speed Violators")
    lines.append("")
    lines.append("| benchmark | xcc -Of bytes/cycles | fastest competing mode | best bytes/cycles | xcc cycle gap |")
    lines.append("| --- | ---: | --- | ---: | ---: |")
    for gap, benchmark, xcc, best in speed_violators[:20]:
        lines.append(
            f"| `{benchmark}` | {xcc.bytes}/{xcc.cycles} | `{best.label}` | {best.bytes}/{best.cycles} | +{gap} |"
        )
    if not speed_violators:
        lines.append("| none | n/a | n/a | n/a | 0 |")
    lines.append("")
    lines.append("## Per Benchmark")
    lines.append("")
    lines.append("| benchmark | xcc -Of | xcc -O3 | xcc -Os | sdcc size | sdcc speed | z88dk sdcc | z88dk 80cc | z88dk sccz80 | best size | fastest |")
    lines.append("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |")
    for benchmark, results, best_size, best_speed in per_benchmark:
        lines.append(
            f"| `{benchmark}` | "
            f"{results['xcc_Of'].cell()} | "
            f"{results['xcc_O3'].cell()} | "
            f"{results['xcc_Os'].cell()} | "
            f"{results['sdcc_size'].cell()} | "
            f"{results['sdcc_speed'].cell()} | "
            f"{results['z88dk_sdcc'].cell()} | "
            f"{results['z88dk_80cc'].cell()} | "
            f"{results['z88dk_sccz80'].cell()} | "
            f"{best_size.label if best_size else 'n/a'} | "
            f"{best_speed.label if best_speed else 'n/a'} |"
        )
    lines.append("")
    lines.append("## Outputs")
    lines.append("")
    lines.append(f"- [results.csv]({args.results.name})")
    lines.append(f"- [versions.txt]({args.versions.name})")
    lines.append("- `work/` contains the intermediate compiler outputs for inspection")
    lines.append("")

    return "\n".join(lines)


def main() -> None:
    args = parse_args()
    with args.results.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    summary = emit_summary(args, rows)
    args.summary.write_text(summary, encoding="utf-8")
    print(summary)


if __name__ == "__main__":
    main()
