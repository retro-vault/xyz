#!/usr/bin/env python3
"""
Generate an assembler module that creates link-time references to the public
symbols exercised only by the host-side libc harness.
"""

from __future__ import annotations

import re
import sys
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


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} test_main.cpp out.s", file=sys.stderr)
        return 1

    test_main = Path(sys.argv[1])
    out_path = Path(sys.argv[2])
    repo = test_main.resolve().parent
    while repo != repo.parent and not (repo / "lib/libc/src").exists():
        repo = repo.parent
    if not (repo / "lib/libc/src").exists():
        raise SystemExit("could not locate repo root from test source path")
    exported = scan_exported_symbols(
        [
            repo / "lib/libc/src",
            repo / "lib/sys/none",
        ]
    )
    refs = [(name, exported[name]) for name in read_rt_names(test_main) if name in exported]
    emit(out_path, refs)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
