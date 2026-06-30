#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def default_xcc(suite_root: Path) -> Path:
    repo_root = suite_root.parents[3]
    return repo_root / "bin" / "x" / "bin" / "xcc"


def load_cases(path: Path) -> dict[str, object]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"missing blocked-case catalog: {path}") from exc


def build_command(entry: dict[str, object], suite_root: Path, xcc: Path, out_path: Path) -> list[str]:
    cmd = [str(xcc)]
    if entry["stage"] == "compile":
        cmd.append("-S")
    else:
        cmd.extend(["--platform=emu", "--oformat=binary"])

    for arg in entry.get("compiler_args", []):
        cmd.append(str(arg))
    for rel in entry["sources"]:
        cmd.append(str((suite_root / rel).resolve()))
    cmd.extend(["-o", str(out_path)])
    return cmd


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--suite-root", required=True)
    parser.add_argument("--case-id", required=True)
    parser.add_argument("--xcc")
    args = parser.parse_args()

    suite_root = Path(args.suite_root).resolve()
    xcc = Path(args.xcc).resolve() if args.xcc else default_xcc(suite_root)
    catalog = load_cases(suite_root / "generated" / "blocked_cases.json")

    entry = catalog.get(args.case_id)
    if entry is None:
        print(f"unknown blocked z88dk case id: {args.case_id}", file=sys.stderr)
        return 1

    if not entry.get("expect_build_failure", True):
        return 0

    case_dir = suite_root / "build" / "blocked-checks" / args.case_id
    case_dir.mkdir(parents=True, exist_ok=True)
    out_path = case_dir / ("probe.s" if entry["stage"] == "compile" else "probe.bin")

    cmd = build_command(entry, suite_root, xcc, out_path)
    result = subprocess.run(
        cmd,
        cwd=suite_root,
        capture_output=True,
        text=True,
        timeout=int(entry.get("timeout_seconds", 120)),
        check=False,
    )

    combined = (result.stdout or "") + "\n" + (result.stderr or "")
    if result.returncode == 0:
        print(
            f"{args.case_id}: build unexpectedly succeeded for blocked case "
            f"({entry['reason']})",
            file=sys.stderr,
        )
        return 1

    marker = entry.get("marker") or ""
    if marker and marker not in combined:
        print(f"{args.case_id}: build failed, but expected marker was missing:", file=sys.stderr)
        print(marker, file=sys.stderr)
        if combined.strip():
            print(combined.strip(), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
