#!/usr/bin/env python3
"""
Generate an assembler module that creates link-time references to the public
symbols exercised only by the host-side libc harness.
"""

from __future__ import annotations

import re
import sys
import os
from pathlib import Path


def read_rt_names(test_main: Path) -> list[str]:
    text = test_main.read_text()
    return sorted(set(re.findall(r"rt_sym::([A-Za-z_][A-Za-z0-9_]*)", text)))


def scan_exported_symbols(roots: list[Path]) -> dict[str, str]:
    exported: dict[str, str] = {}
    globl_re = re.compile(r"\.globl\s+([A-Za-z0-9_.$]+)")
    for root in roots:
        for path in sorted(root.rglob("*.s")):
            for line in path.read_text().splitlines():
                m = globl_re.search(line)
                if not m:
                    continue
                sym = m.group(1)
                cid = re.sub(r"[^A-Za-z0-9_]", "_", sym.lstrip("_"))
                if cid and not cid[0].isdigit():
                    exported.setdefault(cid, sym)
    return exported


def emit(out_path: Path, refs: list[tuple[str, str]]) -> None:
    lines = [
        "        ;; core_force_refs.s",
        "        ;;",
        "        ;; Generated helper that creates link-time references for the",
        "        ;; host-side libc harness so xld extracts the required members",
        "        ;; from libc.a while we still keep real archive semantics.",
        "",
        "        .module core_force_refs",
        "        .optsdcc -mz80 sdcccall(1)",
        "",
    ]
    for _, sym in refs:
        lines.append(f"        .globl  {sym}")
    lines += [
        "",
        "        .area   _CONST",
        "",
        "__core_force_refs__:",
    ]
    for name, sym in refs:
        lines.append(f"        .dw     {sym}        ; force {name}")
    lines.append("")
    out_path.write_text("\n".join(lines))


def looks_like_x_root(path: Path) -> bool:
    return (path / "libc/src").exists() and (path / "platforms/none").exists()


def find_x_root(test_main: Path) -> Path:
    candidates: list[Path] = []

    env_x_root = os.environ.get("X_ROOT")
    if env_x_root:
        candidates.append(Path(env_x_root).resolve())

    candidates.append(Path(__file__).resolve().parents[3])

    probe = test_main.resolve()
    candidates.extend(probe.parents)

    for candidate in candidates:
        if looks_like_x_root(candidate):
            return candidate

    raise SystemExit("could not locate x root for libc force-ref generation")


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} test_main.cpp out.s", file=sys.stderr)
        return 1

    test_main = Path(sys.argv[1])
    out_path = Path(sys.argv[2])
    x_root = find_x_root(test_main)
    exported = scan_exported_symbols(
        [
            x_root / "libc/src",
            x_root / "platforms/none",
        ]
    )
    refs = [(name, exported[name]) for name in read_rt_names(test_main) if name in exported]
    emit(out_path, refs)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
